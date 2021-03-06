/* 
 * File:   ST7540.c
 * Author: aece-engineering
 *
 * Created on November 26, 2014, 3:40 PM
 *
 * When the unit is powered on it generates a continious clock stream, this
 * means that we either have to make sure our SPI module is in sync when we want
 * to transmit for initialization or only turn it on after we have  signalled
 * that we are initializing, we use this second approach.
 *
 * Module works with header detection and no packet length detection
 *
 * With packet length detection SS is held low from when the header is detected
 * until n bytes have been read, this means that the unit is held in RX until
 * n bytes have been read regardless of how many bytes were sent. The clock is
 * generated continiously while the read operation is in progress but not
 * otherwise. This enables the use of the SS pin as would normally be done but
 * forces us to wait the full length of time the biggest packet would take.
 *
 * Without packet length detection SS is pulsed when a valid header is observed
 * and it is up to the program to distinguish how many bytes to read. The clock
 * is generated continiously regardless if there is data on the line or not.
 * This means we don't have to wait for a full frame but can't use the SS pin as
 * normal.
 *
 * Since a header can be detected in the middle of an frame if sync isn't spot
 * on we have to start the SPI module only when a header is observed, we can't
 * afford the wait time of using length detection (which would have allowed
 * using the SS pin normally) so instead monitor the state of the SS pin and
 * active the SPI module when a valid header pulse is received.
 *
 * Ideally we would want to use an int on chage pin but ISC-1V1 hardware didn't
 * allow this so the pin was monitored every 100us.
 */

#include "ST7540.h"
#include "main.h"

unsigned char flagST7540;
unsigned char bufferTXST7540UCA[ST7540_MAX_PACKET_LEN+ST7540_HEADER_LEN+ST7540_PREAM_LEN];
unsigned char bufferRXST7540UCA[ST7540_MAX_PACKET_LEN];
unsigned char bufferTXLenUCA;
unsigned char bufferTXNextUCA;
unsigned char bufferRXLenUCA;
unsigned char bufferRXNextUCA;
unsigned char packetNumberUC;

unsigned char InitST7540(void);
void InitST7540Pins(void);
void WriteConfigST7540(unsigned long short, unsigned long short);
void ReadConfigST7540(unsigned long short *, unsigned long short *);
void SPIISRHandlerST7540(void);
void RXReadyISRHandlerST7540(void);
unsigned char LineIdleST7540(void);
void CreateMessageST7540(unsigned short, unsigned short, unsigned char, unsigned char, char *);
void StartTransmitST7540(void);
unsigned char TransmitBusyST7540(void);
void ReceiveNewDataST7540(void);
unsigned char DataReadyST7540(void);
unsigned short PacketReadParamST7540(unsigned char);
unsigned char CheckCRCST7540(void);
unsigned short CalcCRCST7540(void);
void initST7540PPS(void);
void initST7540Interrupts(void);

extern void _delay_ms(unsigned int delay);
extern unsigned short CRC16(char *, unsigned short);
extern void ppsLock(void);
extern void ppsUnlock(void);

unsigned char InitST7540(void){
    unsigned long short frameDataTXUS;
    unsigned long short frameDataRXUS = 0;
    unsigned long short configDataTXUS;
    unsigned long short configDataRXUS = 0;

    InitST7540Pins();                                                           //Configure required pins

    frameDataTXUS = /*(ST7540_MAX_PACKET_LEN << 16) |*/ ST7540_HEADER;          //Build bits 48-24
    configDataTXUS = ST7540_FREQ_110KHZ |                                       //Build bits 23-0
                   ST7540_BAUD_2400 |                                           //Default
                   ST7540_DEVIA_05 |                                            //Default
                   ST7540_WATCHDOG_DIS |
                   ST7540_TRANS_TOUT_1S |                                       //Default
                   ST7540_FREQ_DET_TIME_1MS |                                   //Default
                   ST7540_PREAM_W_COND |
                   ST7540_SYNC |
                   ST7540_OUTP_CLOCK_OFF |
                   ST7540_OUTP_VOLT_FRZ_DIS |                                   //Default
                   ST7540_HEADER_RECOG_EN |
                   ST7540_FRAME_LEN_CNT_DIS |                                   //Default
                   ST7540_HEADER_LENGTH_16 |                                    //Default
                   ST7540_EXTENDED_REG_EN_48 |
                   ST7540_SENSITIV_HIGH |                                       //Default
                   ST7540_INP_FILTER_DIS;                                       //Default

    SSP1STATbits.CKE = 1;                                                       //SPI TX on high to low
    SSP1CON1bits.SSPM = 0b0101;                                                 //SPI Slave mode, SSX disabled

    WriteConfigST7540(frameDataTXUS, configDataTXUS);                           //First write to set extended registers
    WriteConfigST7540(frameDataTXUS, configDataTXUS);                           //Second write to fill in extended registers
    ReadConfigST7540(&frameDataRXUS, &configDataRXUS);                          //Read back config to confirm
    if((frameDataTXUS != frameDataRXUS) && (configDataTXUS != configDataRXUS))
        return 0;                                                               //Return false on error

    initST7540Interrupts();

    return 1;                                                                   //Return true if all went well
}

void WriteConfigST7540(unsigned long short frameDataUS, unsigned long short configDataUS){
    unsigned char discardUC;
    unsigned char bitsSentUC;

    bufferTXST7540UCA[0] = (frameDataUS & UNSIGNED_LONG_SHORT_23_16) >> 16;     //Format config data for TX
    bufferTXST7540UCA[1] = (frameDataUS & UNSIGNED_LONG_SHORT_15_8) >> 8;
    bufferTXST7540UCA[2] = frameDataUS & UNSIGNED_LONG_SHORT_7_0;
    bufferTXST7540UCA[3] = (configDataUS & UNSIGNED_LONG_SHORT_23_16) >> 16;
    bufferTXST7540UCA[4] = (configDataUS & UNSIGNED_LONG_SHORT_15_8) >> 8;
    bufferTXST7540UCA[5] = configDataUS & UNSIGNED_LONG_SHORT_7_0;

    LAT_REGnDATA = 1;                                                           //Register access
    LAT_RXnTX = 0;                                                              //Write to register
    while(PORT_CLR_T);                                                          //Wait for idle clock
    SSP1CON1bits.SSPEN = 1;                                                     //Enable SPI

    for(bitsSentUC = 0; bitsSentUC < 6; bitsSentUC++){
        discardUC = SSP1BUF;                                                    //Clear BF by reading
        SSP1BUF = bufferTXST7540UCA[bitsSentUC];                                //Send next byte
        while(!SSP1STATbits.BF);                                                //Wait till TX done
    }

    LAT_REGnDATA = 0;                                                           //Mains access
    LAT_RXnTX = 1;                                                              //Receive mode
    SSP1CON1bits.SSPEN = 0;                                                     //Disable SPI
    _delay_ms(2);
}

void ReadConfigST7540(unsigned long short *frameDataUS, unsigned long short *configDataUS){
    unsigned char bitsReceivedUC;
    unsigned char discardUC;

    LAT_REGnDATA = 1;                                                           //Register access
    LAT_RXnTX = 1;                                                              //Read from register
    while(PORT_CLR_T);                                                          //Wait for idle clock
    SSP1CON1bits.SSPEN = 1;                                                     //Enable SPI

    discardUC = SSP1BUF;                                                        //Clear BF by reading
    SSP1BUF = 0;                                                                //Send blank value
    for(bitsReceivedUC = 0; bitsReceivedUC < 6; bitsReceivedUC++){
        while(!SSP1STATbits.BF);                                                //Wait till RX done
        bufferRXST7540UCA[bitsReceivedUC] = SSP1BUF;                            //Save received byte
        SSP1BUF = 0;                                                            //Send blank value
    }
    
    LAT_REGnDATA = 0;                                                           //Mains access
    SSP1CON1bits.SSPEN = 0;                                                     //Disable SPI
    
    *frameDataUS |= ((unsigned long short) bufferRXST7540UCA[0] << 16);         //Format data for RX compare
    *frameDataUS |= ((unsigned long short) bufferRXST7540UCA[1] << 8);
    *frameDataUS |= bufferRXST7540UCA[2];
    *configDataUS |= ((unsigned long short) bufferRXST7540UCA[3] << 16);
    *configDataUS |= ((unsigned long short) bufferRXST7540UCA[4] << 8);
    *configDataUS |= bufferRXST7540UCA[5];
}

void SPIISRHandlerST7540(void){
    unsigned char dataReadUC;
    
    dataReadUC = SSP1BUF;
    SSP1BUF = 0;    

    if(flagST7540 & FLAG_ST7540_TX_ACTIVE){                                     //Are we in TX mode
        if(bufferTXNextUCA > bufferTXLenUCA){                                   //Done transmitting buffer?
            flagST7540 &= ~FLAG_ST7540_TX_ACTIVE;                               //Disable TX mode
            SSP1CON1bits.SSPEN = 0;                                             //Disable SPI module
            sspiInterruptFlag = 0;                                                //Clear int flag
            LAT_RXnTX = 1;                                                      //Change to read data
            return;
        }
        SSP1BUF = bufferTXST7540UCA[bufferTXNextUCA++];                         //Send next byte
    }else if(flagST7540 & FLAG_ST7540_RX_ACTIVE){                               //Are we in RX mode
        bufferRXST7540UCA[bufferRXNextUCA] = dataReadUC;                      //Read next byte
        if(++bufferRXNextUCA>=ST7540_MAX_PACKET_LEN) bufferRXNextUCA = ST7540_MAX_PACKET_LEN -1;
        if(bufferRXST7540UCA[0] == bufferRXNextUCA){                            //Entire packet read?
            flagST7540 |= FLAG_ST7540_DATA_READY;                               //Mark data as ready
            flagST7540 &= ~FLAG_ST7540_RX_ACTIVE;                               //Disable RX mode
            SSP1CON1bits.SSPEN = 0;                                             //Disable SPI module
            sspiInterruptFlag = 0;                                                //Clear int flag
            return;
        }
    }
    sspiInterruptFlag = 0;                                                        //Clear int flag
}

void RXReadyISRHandlerST7540(void){
    //finish this for pin change int?
    char ReadIOCC;
    bufferRXNextUCA = 0;
    if((flagST7540 & FLAG_ST7540_RX_ACTIVE) && !PORT_n_CD_PD)                   //If in RX mode and get SS pulse
        SSP1CON1bits.SSPEN = 1;                                                 //Enable SPI
    ReadIOCC = PORT_CLR_T_IOC;                                                  //Read the state of PORT_CLR_T_IOC to fascilitate clearing interrupt flag
}

unsigned char LineIdleST7540(void){
    if(PORT_BU_THERM)                                                           //Is the line active
        return 0;                                                               //Return not idle
    return 1;                                                                   //Return idle
}

void buildMessageST7540(void){
    unsigned char dataBufLocUC = 0;
    unsigned short packetCRCUS = 0;
    unsigned char index = 0;
    unsigned char dataOffset = 0;
    if(outgoingQueue.queue_pointer >= outgoingQueue.length)                     //determine which message should be sent
        index = outgoingQueue.queue_pointer - outgoingQueue.length;             //the queue resides within the bounds of the queue length
    else
        index = outgoingQueueLength + outgoingQueue.queue_pointer - outgoingQueue.length;        //the queue overflows the bounds of the queue length and therefore has started at 0 again

    bufferTXST7540UCA[0] = 0xAA;                                                //Preamble
    bufferTXST7540UCA[1] = 0xAA;
    bufferTXST7540UCA[2] = ST7540_HEADER >> 8;                                  //Packet header
    bufferTXST7540UCA[3] = ST7540_HEADER;
    bufferTXST7540UCA[4] = ST7540_MIN_PACKET_LEN + outgoingQueue.queue_store[index].data_length;//Packet length
    bufferTXST7540UCA[5] = ABB_1.serial >> 8;                                     //Packet source
    bufferTXST7540UCA[6] = ABB_1.serial;
    bufferTXST7540UCA[7] = outgoingQueue.queue_store[index].destination >> 8;   //Packet destination
    bufferTXST7540UCA[8] = outgoingQueue.queue_store[index].destination;
    bufferTXST7540UCA[9] = packetNumberUC++;                                    //Packet number
    bufferTXST7540UCA[10] = outgoingQueue.queue_store[index].command;           //Command

    if(bufferTXST7540UCA[10] == CMD_AB1_DATA){                                  //if we are sending up data
        unsigned short *statusPtr;                                              //send the status of the ABB1 as well
        statusPtr = &ABB_1.info.statusBits;                                     //with its CRC
        bufferTXST7540UCA[11] = ABB_1.dets_length;
        bufferTXST7540UCA[12] = ABB_1.info.CRC;
        bufferTXST7540UCA[13] = *statusPtr >> 8;
        bufferTXST7540UCA[14] = *statusPtr;
        bufferTXST7540UCA[4] += 4;                                              //indicate that an additional four bytes will be sent
        dataOffset = 4;                                                         //shift the data from the queue four bytes to make space for these 4
    }
    for(dataBufLocUC = dataOffset; dataBufLocUC < outgoingQueue.queue_store[index].data_length + dataOffset; dataBufLocUC++)//shift the contents of the data from the              //Packet payload
        bufferTXST7540UCA[dataBufLocUC + 11] = outgoingQueue.queue_store[index].data[dataBufLocUC - dataOffset];//outgoing queue into the register to be transmitted
    dataBufLocUC += 11;

    packetCRCUS = CRC16(bufferTXST7540UCA + 4, ST7540_MIN_PACKET_LEN + outgoingQueue.queue_store[index].data_length - 2 + dataOffset);//determine the CRC                                                    //Calculate CRC
    bufferTXST7540UCA[dataBufLocUC++] = packetCRCUS >> 8;                       //shift it into the outgoing buffer
    bufferTXST7540UCA[dataBufLocUC] = packetCRCUS;
    bufferTXLenUCA = dataBufLocUC;                                              //let the MSSP know how many bytes are intended to be sent
}

void StartTransmitST7540(void){
    unsigned char discardUC;

    flagST7540 |= FLAG_ST7540_TX_ACTIVE;                                        //Set TX state as active
    LAT_RXnTX = 0;                                                              //Start TX
    while(PORT_CLR_T);
    SSP1CON1bits.SSPEN = 1;                                                     //Enable SPI module
    discardUC = SSP1BUF;                                                        //Clear BF just in case
    SSP1BUF = bufferTXST7540UCA[0];                                             //Write first byte
    bufferTXNextUCA = 1;                                                        //Set index
}

unsigned char TransmitBusyST7540(void){
    return (flagST7540 & FLAG_ST7540_TX_ACTIVE);                                //Are we transmitting
}

void ReceiveNewDataST7540(void){
    unsigned char discardUC;

    flagST7540 |= FLAG_ST7540_RX_ACTIVE;                                        //Set RX state as active
    flagST7540 &= ~FLAG_ST7540_DATA_READY;                                      //Mark data as not ready
    discardUC = SSP1BUF;                                                        //Clear BF just in case
    SSP1BUF = 0;                                                                //Write null byte
    bufferRXNextUCA = 0;                                                        //Set index
}

char *PacketDataST7540(void){
    return (bufferRXST7540UCA + 7);                                             //Return address to start of data
}

unsigned short PacketReadParamST7540(unsigned char paramName){
    unsigned short retValueUS = 0;

    switch(paramName){
        case(ST7540_DATA_LEN):                                                  //Retrieve length of the packet
            return bufferRXST7540UCA[0] - ST7540_MIN_PACKET_LEN;
        case(ST7540_SOURCE):                                                    //Retrieve the source address
            retValueUS |= ((unsigned short) bufferRXST7540UCA[1] << 8);
            retValueUS |= bufferRXST7540UCA[2];
            return retValueUS;
        case(ST7540_DEST):                                                      //Retrieve the destination address
            retValueUS |= ((unsigned short) bufferRXST7540UCA[3] << 8);
            retValueUS |= bufferRXST7540UCA[4];
            return retValueUS;
        case(ST7540_NUMBER):                                                    //Retrieve the packet number
            return bufferRXST7540UCA[5];
        case(ST7540_CMD):                                                       //Retrieve the command
            return bufferRXST7540UCA[6];
        case(ST7540_CRC_VALID):                                                 //Does the CRC match
            return CheckCRCST7540();
    }

    return 0;
}

unsigned char DataReadyST7540(void){
    return (flagST7540 & FLAG_ST7540_DATA_READY);                               //Is the received data ready
}

unsigned char CheckCRCST7540(void){
    unsigned short expectedCRCUS;
    unsigned short receivedCRCUS = 0;
    unsigned char packetLenUC;

    packetLenUC = bufferRXST7540UCA[0]-2;
    expectedCRCUS = CRC16(bufferRXST7540UCA, packetLenUC);                      //Calculate the expected CRC
    receivedCRCUS |= ((unsigned short) bufferRXST7540UCA[packetLenUC] << 8);
    receivedCRCUS |= bufferRXST7540UCA[packetLenUC+1];

    return (expectedCRCUS == receivedCRCUS);                                    //Compare to the received CRC
}

void InitST7540Pins(void){
    //Define ST7540 comms pins
    initST7540PPS();
    
    TRIS_n_CD_PD = 1;
    ANSEL_n_CD_PD = 0;

    TRIS_REGnDATA = 0;
    LAT_REGnDATA = 0;

    TRIS_RXD = 1;

    TRIS_TXD = 0;
    LAT_TXD = 0;

    TRIS_RXnTX = 0;
    LAT_RXnTX = 1;

    TRIS_BU_THERM = 1;

    TRIS_CLR_T = 1;

    TRIS_CLR_T_IOC = 1;
    ANSEL_CLR_T_IOC = 0;

    TRIS_SPI_UART = 0;
    LAT_SPI_UART = 0;
    
}

void initST7540PPS(void){
    ppsUnlock();
    INT0PPS = 0b00000101;                                                       //INT0 on RA5
    SSP1DATPPS = 0b00010000;                                                    //SSP1 data in on RC0
    RC7PPS = 0x1A;                                                              //SSP1 data out on RC7
    ppsLock();
}

void initST7540Interrupts(void){
    INTCONbits.INT0EDG = 0;
    modemReceiveInterrupt = 1;
    modemReceiveInterruptFlag = 0;
    
    sspiInterrupt = 1;                                                          //Enable SPI ints
    sspiInterruptFlag = 0;
    IPR3bits.SSP1IP = 1;
}

void modemSleep(void){
    TRIS_RXnTX = 1;
    TRIS_n_CD_PD = 1;
    TRIS_REGnDATA = 1;
    TRIS_RXD = 1;
    TRIS_TXD = 1;
    TRIS_SPI_UART = 1;
}