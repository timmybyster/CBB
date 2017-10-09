/*
 * File:   bluetooth.c
 * Author: Tim Buckley
 *
 * Created on 04 April 2017, 2:15 PM
 */
#include "bluetooth.h"
#include "main.h"

void initialiseUART(void);
void writeEUSART(unsigned char *Data, unsigned char size);
void readEUSART(void);
void receiveBluetoothSetup(void);
void receiveBluetooth(void);
void bluetoothSetupSwitch(void);
void sendAtCommand(void);
void setDeviceName(void);
void setSlaveMode(void);
void setBaudRate(void);
void exitCMDMode(void);
void setSniffMode(void);
void setConfigTimer(void);
void resetBTModule(void);
void processBluetooth(void);
void sendACK(void);
void processBTPacket(void);
void initialiseBluetoothPins(void);
void resetBluetooth(void);
unsigned char checkBluetoothCRC(void);
unsigned short bluetoothCRC(unsigned char *data, unsigned char length);
unsigned char power(int base, int exponent);

extern void _delay_ms(unsigned int delay);
extern void initialiseState(states *specific, char number);
extern void ppsLock(void);
extern void ppsUnlock(void);
extern void _delay_us(unsigned char us);
extern void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size );
extern unsigned char checkForExistingUID(unsigned long *receivedUID);


//activates the Bluetooth by intialisation and enabling it as a background process
//as well as configuring the EUSART
void initialiseBluetooth(void) {
    initialiseBluetoothPins();                                                  //initialise the Pins associated with the Bluetoot
}

void activateBluetooth(void){
    initialiseState(&state.bluetooth, bluetoothState);                          //initialise the Bluetooth state for the state Handler
    initialiseUART();                                                           //Intialise the UART
    LAT_BT_ENABLE = 1;
    FLAGS.bluetooth = 1;                                                        //Set the Bluetooth flag so the state Handler knows to process it
    FLAGS.bluetoothTimer = 0;
    COUNTERS.bluetoothTimer = mins10;
    bluetoothStatus.bluetoothSetup = 1;                                         //Set the Setup Flag to send commands to the Bluetooth Module to configure it
    setupIndex = 0;                                                             //Ensure that the setup begins with the correct command
}

//Uses PPS to enable the EUSART3 module onto the appropriate pins
void initialiseBluetoothPins(void){
    ppsUnlock();                                                                //Unlock the PPS module
    RX3PPS = 0b00100001;                                                        //RX3 on RE1
    RE0PPS = 0x10;                                                              //TX3 on RE0
    ppsLock();                                                                  //Lock the PPS module
    
    TRIS_BT_RX = 1;                                                             //set the Rx pin as a digital input
    ANSEL_BT_RX = 0;
    TRIS_BT_TX = 1;                                                             //set the Tx pin as an input
    
    TRIS_BT_RST = 0;                                                            //set the Reset pin as an output and set it to 1
    LAT_BT_RST = 1;
    
    TRIS_BT_ENABLE = 0;
    LAT_BT_ENABLE = 0;
}

//Initialises the EUSART3 module and its associated interrupts
void initialiseUART(void) {
    //EUSART setup
    PMD5bits.UART3MD = 0;
    TX3STAbits.BRGH = 1;                                                        // High speed Baud Rate
    BAUD3CONbits.BRG16 = 1;                                                     // 16-bit Baud Rate Generator
    SP3BRGH = 0x06;                                                             // 0x0682 = 1666d
    SP3BRGL = 0x82;                                                             // 64000000/(4*(1666+1)) = 9598 = 0.02% error
    BAUD3CONbits.ABDEN = 1;
    TX3STAbits.SYNC = 0;                                                        // Asynchronous mode
    RC3STAbits.SPEN = 0;                                                        // Serial port disabled
    TX3STAbits.TXEN = 0;                                                        // Transmit disabled
    RC3STAbits.CREN = 0;                                                        // Receiver disabled 
}

//Writes a passed array to the EUSART module
void writeEUSART(unsigned char *data, unsigned char size) {
    uartTransmitEnable = 1;
    for(int i = 0; i < size; i++) {         
        while(!TX3STAbits.TRMT);                                                // Ensure that the transmit shift register is empty
        uartTransmitByte = *data;                                               //shift the next byte to be transmitted
        data++;                                                                 //move the index of the array along
    }
    while(!TX3STAbits.TRMT);                                                    // Wait for transmission to complete
    uartTransmitEnable = 0;
}

//Builds a serial array from a parsed packet to be sent
void sendPacket(void){
    unsigned char txReg[MAX_RESPONSE_SIZE];
    int i;
    txReg[0] = startByte;                                                       //start Byte
    txReg[1] = btPacket.send.size;                                              //send size
    txReg[2] = btPacket.send.command;                                           //command
    
    for (i = 0; i < btPacket.send.size - MIN_BT_LENGTH; i++){                   //loop depending on the size of the data
        txReg[i + 3] = btPacket.send.data[i];                                   //assign the data to transmit array
    }
    btPacket.send.crc = bluetoothCRC(txReg, btPacket.send.size - 2);            //calculate the CRC of the current array
    txReg[btPacket.send.size - 2] = btPacket.send.crc >> 8;                     //assign the first byte of the crc to the second last byte of the array
    txReg[btPacket.send.size - 1] = (unsigned char)btPacket.send.crc;           //assign the second byte of the crc to the last byte of the array
    
    writeEUSART(txReg, btPacket.send.size);                                     //write the array to the EUSAT module
}

//Determines whether we are processing AT command data from a the Bluetooth module 
//or log data from a connected device
void readEUSART(void){
    COUNTERS.bluetoothTimer = mins10;
    FLAGS.bluetoothTimer = 0;
    if(bluetoothStatus.bluetoothSetup)                                          //if in setup mode
        receiveBluetoothSetup();                                                //process an AT Response
    else
       receiveBluetooth();                                                      //otherwise process Log Data
}

//processes a received Bluetooth data packet and builds the appropriate response
//based on the status of the device and the command received
void processBTPacket(void){
    if(!checkBluetoothCRC()){                                                   //check the CRC of the received Packet
        bluetoothStatus.ACKReady = 0;                                           //if its wrong clear the acknowledge flag to indicate a response
        return;                                                                 //will not be sent and return
    }
    unsigned char *tempDetInfo;
    bluetoothStatus.packetReceived = 0;
    btPacket.send.start = startByte;                                            //Build the response packet beginning with the start byte
    btPacket.send.command = btPacket.receive.command;                           //The response command is always the received command
    btPacket.send.size = MIN_BT_LENGTH;                                         //set the response size to the minimum 
    switch(btPacket.receive.command){                                           //Process based on the current command
        case CMD_BT_HEADER :                                                    //Header Packet to show how many EDDs are about to be sent
            ABB_1.dets_length = 0;
            btPacket.send.size += 2;                                            //Increase the packet size by 2
            expectedDets = btPacket.receive.data[0];                            //set the number of dets we expect to have after receiving the log
            if(btPacket.receive.data[0] < maxDets -1)
                btPacket.send.data[0] = expectedDets;
            else
                btPacket.send.data[0] = maxDets - 1; 
            btPacket.send.data[1] = 10;                                         //assume we can receive any size of delay
            break;
            
        case CMD_BT_INFO :                                                      //EDD Packet that will contain the UID and delay of and EDD
            btPacket.send.size += 6;                                            //increase the packet size accordingly
            if(checkForExistingUID(btPacket.receive.data)){
                NOP();
                bluetoothStatus.ACKReady = 0;
                return;
            }
            ABB_1.dets_length ++;                                           //increment the number of dets stored on the device
            for(int i = 0; i < 4; i++){
                ABB_1.det_arrays.UIDs[ABB_1.dets_length].UID[i] = btPacket.receive.data[i];//assign the uid to data memory
            }
            ABB_1.det_arrays.info[ABB_1.dets_length].delay = (unsigned short)btPacket.receive.data[4] << 8;//assign the first byte of the delay to memory
            ABB_1.det_arrays.info[ABB_1.dets_length].delay |= (unsigned short)btPacket.receive.data[5];//assign the second byte of the delay to memory
            ABB_1.det_arrays.info[ABB_1.dets_length].data.logged = 1;           //show that this EDD has been logged
            ABB_1.det_arrays.info[ABB_1.dets_length].data.tagged = 0;           //show that this EDD has not been tagged
            for(int i = 0; i < 4; i++){
                btPacket.send.data[i] = btPacket.receive.data[i];               //build the response
            }
            btPacket.send.data[4] = btPacket.receive.data[4];                   //add this to the response    
            btPacket.send.data[5] = btPacket.receive.data[5];                   //also add this to the response
            break;
            
        case CMD_BT_END :                                                       //End Packet to show whether Log has been received successfully
            if(ABB_1.dets_length == expectedDets)                                      //if the number of dets stored in data memory is what was expected
                btPacket.send.data[0] = 1;                                      //respond with a true value
            else
                btPacket.send.data[0] = 0;                                      //respond with a false value
            btPacket.send.size += 1;                                            //increase the send size appropriately
            bluetoothStatus.packetReceived = 1;
            break;
            
        case CMD_BT_STATUS :
            tempDetInfo = &ABB_1.det_arrays.info[btPacket.receive.data[0]].data;
            btPacket.send.size += 2;
            btPacket.send.data[0] = btPacket.receive.data[0];
            btPacket.send.data[1] = (*tempDetInfo)&0xF0;
            break;
            
        case CMD_BT_BOOT :
            RESET();
            break;
            
        case CMD_BT_EDD_LENGTH :
            btPacket.send.size += 1;
            btPacket.send.data[0] = ABB_1.dets_length;
            break;
            
        case CMD_BT_UID :
            if(btPacket.receive.data[0] > ABB_1.dets_length){                   //if the request is for a window that does not have an EDD
                bluetoothStatus.ACKReady = 0;
                return;
            }    
            btPacket.send.size += 6;                                            
            for (int i = 0; i < 4; i++){
                btPacket.send.data[i] = ABB_1.det_arrays.UIDs[btPacket.receive.data[0]].UID[i];
            }
            btPacket.send.data[4] = ABB_1.det_arrays.info[btPacket.receive.data[0]].delay >> 8;
            btPacket.send.data[5] = ABB_1.det_arrays.info[btPacket.receive.data[0]].delay;
            break;
            
        default :
            return;
            
    }
    bluetoothStatus.ACKReady = 1;                                               //set the acknowledge bit so a response can be sent
}

void sendAtCommand(void){
    unsigned char txreg[] = "AT\r\n";
    bluetoothStatus.dataReceived = 0;                                           //clear the data Received Flag to allow for the module to respond
    writeEUSART(txreg, sizeof(txreg)/sizeof(txreg[0]));                         //write the command to the EUSART
}

//A setup command that sets the Bluetooth to slave mode
void setSlaveMode(void){
    unsigned char txreg[] = "AT+ROLE0\r\n";
    bluetoothStatus.dataReceived = 0;                                           //clear the data Received Flag to allow for the module to respond
    writeEUSART(txreg, sizeof(txreg)/sizeof(txreg[0]));                         //write the command to the EUSART
}

//A Setup command that sets the device name to be displayed on bluetooth scans
void setDeviceName(void){
    unsigned char txreg[] = "AT+NAMECBB SN: 0000\r\n";                          //initialise the the command to be sent to the module
    
    unsigned short tempSerial = ABB_1.serial;                                     //assign the serial number of the device to a temporary variable
    txreg[15] = tempSerial/1000 + 48;                                           //determine the serial number as 4 digit decimal value
	tempSerial -= (txreg[15] - 48)*1000;                                        //remove the 4th tens digit
	txreg[16] = tempSerial/100 + 48;                                            //determine the 3rd decimal digit
	tempSerial -= (txreg[16] - 48)*100;                                         //remove the 3rd decimal digit
	txreg[17] = tempSerial/10 + 48;                                             //determine the second decimal digit
	tempSerial -= (txreg[17] - 48)*10;                                          //remove the 2nd decimal digit
	txreg[18] = tempSerial + 48;                                                //assign the first decimal digit
    bluetoothStatus.dataReceived = 0;
    writeEUSART(txreg, sizeof(txreg)/sizeof(txreg[0]));                         //write the command to the EUSART
}

//Sets the baud Rate of the Serial Port on the Bluetooth Module
void setBaudRate(void){
    unsigned char txreg[] = "AT+BAUD8\r\n";                                     //initialise the Baud Rate
    bluetoothStatus.dataReceived = 0;                                           //clear the data Received Flag to allow for the module to respond
    writeEUSART(txreg, sizeof(txreg)/sizeof(txreg[0]));                         //write the command to the EUSART
}

//Executes behaviour based on the current state of the the bluetooth state handler
void bluetooth(void) {
    switch(state.bluetooth.current) {
        case setupCheck :
            state.bluetooth.counter = 0;                                        //move to the next state after a ms
            break;
            
        case resetCheck :
            state.bluetooth.counter = 1;                                        //move to the next state after a ms
            break;
            
        case reset :
            setupIndex = 0;
            state.bluetooth.counter = 1;                                        //move to the next state after a ms
            break;
            
        case setup :
            uartEnable = 1;                                                //enable EUSART reception
            uartRecieveEnable = 1;
            uartReceiveInterrupt = 1;                                           //enable the EUSART reception interrupt
            bluetoothSetupSwitch();                                             //execute a command 
        state.bluetooth.counter = setupWaitPeriod;                              //load a timeout of a 1 Second so that if a response
            break;                                                              //is not received the command is sent again
 
        case receiveSetup :
            state.bluetooth.counter = 1;                                        //move straight to the next state
            break;
        
        case processSetup :                                                     //a response has been received 
            setupIndex ++;                                                      //move to the next command to be sent
            uartEnable = 0;                                                     //disable EUSART reception
            uartRecieveEnable = 0;
            uartReceiveInterrupt = 0;       
            bluetoothStatus.dataReceived = 0;                                   //clear the data received flag so new data can be processed
            state.bluetooth.counter = 1;
            break;
        
        case deviceIdle:
            uartEnable = 1;                                                     //enable the uart
            uartRecieveEnable = 1;                                              //enable data reception
            uartReceiveInterrupt = 1;                                           //enable data reception
            state.bluetooth.counter = 1;                                        //move to the next state after a ms
            break;
            
        case receiveData :
            uartEnable = 1;                                                     //enable the uart
            uartRecieveEnable = 1;                                              //enable data reception
            uartReceiveInterrupt = 1;                                           //enable data reception
            state.bluetooth.counter = 1;                                        //move to the next state after a ms
            break;
            
        case processData:
            uartReceiveInterrupt = 0;                                           //disable data reception
            processBTPacket();                                                  //process the received data
            bluetoothStatus.dataReceived = 0;                                   //clear the data received flag in expectation of a new data packet
            state.bluetooth.counter = processPeriod;                                        //move to the next state after a ms
            break;
            
        case sendData:
            sendPacket();                                                       //build the appropriate response and send it
            uartEnable = 0;                                                     //disable the uart
            uartRecieveEnable = 0;                                              //disable data reception
            state.bluetooth.counter = 0;                                        ////move to the next state after 10 ms
            break;
            
        case sendPacketToSurface :
            addDataToOutgoingQueue(ABB_1.det_arrays.UIDs, CMD_AB1_UID, sizeof(detonator_UID));
            //addDataToOutgoingQueue(ABB_1.det_arrays.info, CMD_AB1_DATA, sizeof(detonator_data));
            state.bluetooth.counter = 1;
            bluetoothStatus.packetReceived = 0;
            bluetoothStatus.ACKReady = 0;
            break;
    }
}

//determines the next state based on the current state and the outcome of process
//variables
void bluetoothStateHandler(void) {
    // Determines next state
    switch(state.bluetooth.current){                                            
        case setupCheck :
            if(bluetoothStatus.bluetoothSetup)                                  //start by seeing if setting up the device
                state.bluetooth.next = setup;                              //if so then see if if it needs to be reset
            else
                state.bluetooth.next = receiveData;                              //otherwise wait to receive data
            break;
            
        case resetCheck :                                                       
            if(setupIndex == 0)                                                 //if in the first stage of the setup
                state.bluetooth.next = reset;                                   //reset the device
            else
                state.bluetooth.next = setup;                                   //otherwise process the current state of the setup
            break;
            
        case reset :
            state.bluetooth.next = setup;                                       //after resetting the device proceed to the next stage of the setup
            break;
                
        case setup :
            state.bluetooth.next = receiveSetup;                                //after sending out a setup command wait for a response
            break;
            
        case receiveSetup :
            if(bluetoothStatus.dataReceived)                                    //if a response has been received
                state.bluetooth.next = processSetup;                            //process it accordingly
            else
                state.bluetooth.next = reset;                                   //otherwise start again
            break;
            
        case processSetup :
            state.bluetooth.next = setupCheck;                                  //after processing the setup check if the setup has concluded
            break;
            
        case deviceIdle :
            state.bluetooth.next = receiveData;                                 //keep checking for data
            break;
        
        case receiveData :
            if(bluetoothStatus.dataReceived)                                    //if data has been received from the logger
                state.bluetooth.next = processData;                             //process it
            break;
        
        case processData :                              
            if(bluetoothStatus.ACKReady)                                        //if data was processed successfully
                state.bluetooth.next = sendData;                                //send a response
            else
               state.bluetooth.next = setupCheck;                               //otherwise determine the current state
            break;
            
        case sendData :
            if(bluetoothStatus.packetReceived)
                 state.bluetooth.next = sendPacketToSurface;
            else
                state.bluetooth.next = setupCheck;
            break;
            
        default :
            state.bluetooth.next = setupCheck;                                  //after all other states restart the process
    }
}
//builds a data string from a packet and returns checks the received Crc against
//the expected result. Returns a 1 if they match otherwise returns a 0
unsigned char checkBluetoothCRC(void){
    unsigned char data[11];                                                     //initialise the string to be checked
    unsigned char indexCrc = 0;                                                 //reset the index to 0
    data[indexCrc ++ ] = startByte;                                             //assign the start byte
    data[indexCrc ++ ] = btPacket.receive.size;                                 //assign the packet receice size
    data[indexCrc ++ ] = btPacket.receive.command;                              //assign the command
    for(int i = 0; i < btPacket.receive.size - MIN_BT_LENGTH; i++){             //assign the data
        data[indexCrc ++ ] = btPacket.receive.data[i];                          //the index of the data is determined by indexCrc
    }
    if(btPacket.receive.crc == bluetoothCRC(data, indexCrc))                    //if the received CRC is the same as the calculated one
        return 1;                                                               //return true
    else
        return 0;                                                               //otherwise return false
    
}

//Calculates the Crc of a Packet
unsigned short bluetoothCRC(unsigned char *data, unsigned char length){
         int intCrc = 0;                                                        //define an integer
         int loopBytes,loopBits;                                                //define two references
         unsigned short crc;                                                    //define the return result
         unsigned testBit;                                                      //define an unsigned 
         
         for(loopBytes = 0; loopBytes < length; loopBytes++){                   //loop through the entire data string
                 for(loopBits = 7; loopBits >= 0; loopBits--){                  //loop through each bit of the string
                              testBit = ((intCrc & 0x8000) == 0x8000) ^ ((data[loopBytes] & power(2,loopBits)) == power(2, loopBits));// CRC main condition
                 if(testBit)
                             intCrc = (intCrc & 0x7FFF) * 2 ^ 0x1021;           //if the condition is true XOR with 0x1021
                 else
                             intCrc = (intCrc & 0x7FFF) * 2;                    //if it was false otherwise just AND with 0x7FFF
                 }
         }
         crc = (unsigned short)intCrc;                                          //covert the int to an unsigned short 
         return crc;                                                            //return the result
}

//a function to determine a base to exponent operation
unsigned char power(int base, int exponent){
         unsigned short result;                                                 //initialise the result
         int i;                                                                 //initialise a reference
         if(!exponent)                                                          //if the exponent is 0
           result = 1;                                                          //return 1 regardless of the base
         else
             result = base;                                                     //otherwise the result is initially the base
         for (i = 1; i < exponent; i ++){                                       //for every time the exponent is greater than 1
            result *= base;                                                     //multiply the result by the base
         }
         return result;                                                         //return the result
}

//puts the BT module in a sleep state
void sleepBluetooth(void){
    LAT_BT_ENABLE = 0;                                                             //clear the Reset pin and hold it there                                           //
    LAT_BT_RST = 0;
}

//data reception when sending setup commands to the module
void receiveBluetoothSetup(void){
    index ++;                                                                   //every time new data is received increment the index
    if(uartReceiveByte == '\n'){                                                //until the line end character has been received
        bluetoothStatus.dataReceived = 1;                                       //if it's the end of the line show that data has been received
        if(setupIndex >= 3){                                                    //if the setup index is 1 then all commands responses have been received
            bluetoothStatus.bluetoothSetup = 0;                                 //conclude the setup
            if(!ABB_1.info.statusBits.key_switch_status)
                state.led.next = solidBlue;                                     //Ensure the LED goes solid blue to indicate that bluetooth has been activated
        }
        index = 0;                                                              //reset the index to receive new data
    }
}

//data reception when communicating with the Logger
void receiveBluetooth(void){
    if(index > 15){
        bluetoothStatus.startFound = 0;
        index = 0;
    }
    if(!bluetoothStatus.startFound){                                            //if it has received any data
        if(uartReceiveByte == startByte)                                        //if the data is a start byte
           bluetoothStatus.startFound = 1;                                      //indicate we have received a start byte
           index = 0;                                                           //set the index to 0
    }
    else{
        switch(index){                                                          //start byte already received so assign data based on the index
            case 1 :
                btPacket.receive.size = uartReceiveByte;                        //the first byte after the start byte is the packet size
                break;

            case 2:
                btPacket.receive.command = uartReceiveByte;                     //the next byte is the command
                break;

            default:
                switch(btPacket.receive.size - index - 1){                      //based on the index and the size of the data packet
                    case 0 :
                        btPacket.receive.crc |= uartReceiveByte;                //if its the last byte its the LSB of the CRC
                        bluetoothStatus.startFound = 0;
                        bluetoothStatus.dataReceived = 1;
                        break;

                    case 1:
                        btPacket.receive.crc = (unsigned short)uartReceiveByte << 8;//the second last byte is the MSB of the CRC
                        break;

                    default :
                        btPacket.receive.data[index - 3] = uartReceiveByte;     //every other byte is data 
                        break;
                }

        }
   }
   index ++;
}

//sends a command to the BT module each time incrementing the index so the next
//command can be sent
void bluetoothSetupSwitch(void){
    switch(setupIndex){                                                         //send a command based on the current index of the setup         
        case 0 :    
            resetBluetooth();                                                   //first check if the device is on
            break;
            
        case 1 :
            setDeviceName();                                                     //then set the device name
            break;
        
        case 2 :
            resetBluetooth();
            break;
            
        case 3 :
            setSlaveMode();                                                    //then set it to slave mode
            break;            
    }
}

void resetBluetooth(void){
    LAT_BT_ENABLE = 0;
    LAT_BT_RST = 0;                                                             //reset the bluetooth module
    _delay_us(100);                                                             //by holding the rest pin low for 100us
    LAT_BT_RST = 1;                                                             //and then returning it to a high
    LAT_BT_ENABLE = 1;
    _delay_us(100);
    bluetoothStatus.dataReceived = 0;
}

void transmitBluetoothProgressPacket(unsigned char index, unsigned char stage){
    btPacket.send.start = startByte;                                            //Build the response packet beginning with the start byte
    btPacket.send.size = MIN_BT_LENGTH;                                         //set the response size to the minimum 
    switch(stage){
        case programming :
            btPacket.send.size += 1;
            btPacket.send.command = CMD_BT_PROGRAMMING;
            btPacket.send.data[0] = index;
            break;
            
        case calibrating :
            btPacket.send.size += 1;
            btPacket.send.command = CMD_BT_CALIBRATING;
            btPacket.send.data[0] = index;
            break;
            
        case selfChecking :
            btPacket.send.size += 1;
            btPacket.send.command = CMD_BT_SELF_CHECKING;
            btPacket.send.data[0] = index;
            break;
    }
    uartEnable = 1;                                                             //enable the uart      
    sendPacket();
}