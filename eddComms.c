/*
 * File:   eddComms.c
 * Author: Tim Buckley
 *
 * Created on 05 April 2017, 14:32 AM
 */

#include "eddComms.h"
#include "main.h"
void Set_Line_High(void);
void Set_Line_Low(void);
void Tx_1(void);
void Tx_0(void);
void Tx_Word(unsigned char byte);
void Tx_Start_Bit(void);
unsigned char detectRisingEdgeEDD(unsigned char change);
unsigned char detectFallingEdgeEDD(unsigned char change);
void Tx_Calibration_Pulses(void);
void Tx_Calibration_Pulses_90(void);
void initialiseEddADC(void);

unsigned short readEddADC(void);
unsigned char rxWordEDD(void);
unsigned char EDDCommandResponse (unsigned char command, unsigned char TxLength, unsigned char RxLength, char *data,char *data2);
unsigned char commandResponse(unsigned char TxLength, unsigned char RxLength, unsigned char *data);

unsigned char programUID(unsigned char window);

extern void Delay_ms(int ms);
extern void Delay_500us(void);
extern void Delay_100us(void);
extern unsigned char checkForExistingUID(unsigned long *receivedUID);

//handles the outgoing communication to the EDDs
void eddCommsIsr4(void){                                                        //500us interrupt    
    if (tx)                                                                     //should the line be high or low
        Set_Line_High();                                                        //set the line high
    else
        Set_Line_Low();                                                         //otherwise set it low
}

//handles the reception of data from the EDDs
void eddCommsIsr8(void){                                                        //50us interrupt
    watchCounter ++;                                                            //increment the watch Counter
    if(readFlag){                                                               //if there is valid data expected
        if(!period){                                                            //edge has been detected
            halfBits ++;                                                        //half bit has occurred
        }
        else if(period > 6){                                                    //period greater than half bit
            halfBits ++;                                                        //increment half bit
            period = 0;                                                         //zero the period
            eddState = !eddState;                                               //change the the reception state
        }
        if(!period && halfBits % 2){                                            //if a half bit has just occurred and it is an even half bit 
            byteRead = byteRead << 1;                                           //shift the received byte by 1
            byteRead += eddState;                                               //if the state is one add one to the byte otherwise add 0
        }
        if(halfBits == 16){                                                     //if 16 half bits have occurred the word has been recieved
            readFlag = 0;                                                       //clear the read flag
            byteRead = byteRead << 1;                                           //shift the last bit received into the byte
            byteRead += eddState;                                               //if its one or 0 add it to the received byte
        }
        period++;                                                               //otherwise just increment the period
    }
}

//configures pins to do with EDD communication
void initialiseEDDPins(void){
    TRIS_TXisHIGH = 0;                                                          //set the TXisHIGH pin as an output
    ANSEL_TXisHIGH = 0;                                                         //set it as digital
    LAT_TXisHIGH = 0;                                                           //set the line low

    TRIS_EDDRead = 1;                                                           //set the data reception as an input
    ANSEL_EDDRead = 1;                                                          //set it as analog for the ADC
}

//allows the fire pic to take control of the line
void triStateTxIsHigh(void){
    TRIS_TXisHIGH = 1;                                                          //tri state the pin
}

//takes control of the line
void controlTxIsHigh(void){
    TRIS_TXisHIGH = 0;                                                          //set the pin as an output
}

void EDD_Init_Comms(void){
    initialiseEddADC();                                                         //initialise the ADC
    
    FLAGS.us100 = 0;                                                            //clear the 100us flag
    FLAGS.us500 = 0;                                                            //clear the 500us flag
    tx = 0;                                                                     //clear tx
    
    Set_Line_High();                                                            //set the line high
}

void initialiseEddADC(void){
    FVRCON = 0b00000010;                                                        //set FVR comparitor off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    
    ADCON1 = 0b11110000;                                                        //standard ADC operation
    ADPCH = CHANNEL_EDDRead;                                                    //select the ADC channel
    ADCLK = 0b00111111;                                                         //default clock
    ADREF = 0b00000011;                                                         //standard ADC reference
    ADCON0bits.ADFM = 1;                                                        // Right Justified
    ADCON0bits.ADON = 1;                                                        //turn on the ADC
}

void Set_Line_High(void){  
    LAT_TXisHIGH = 1;                                                           //set TXisHIGH
}

void Set_Line_Low(void){
    LAT_TXisHIGH = 0;                                                           //clear TXisHIGH
}

//transmits a Manchester encoded 1
void Tx_1(void){
    tx = 1;                                                                     //first set the line high
    Delay_500us();                                                              //wait 500us
    tx = 0;                                                                     //then set the line low
    Delay_500us();                                                              //wait 500us for proceeding to next bit
}

//transmits a Manchester encoded 0
void Tx_0(void){
    tx = 0;                                                                     //first set the line low                                              
    Delay_500us();                                                              //wait 500us
    tx = 1;                                                                     //then set the line high
    Delay_500us();                                                              //wait 500us before proceeding to the next bit
}

//transmits a word to the EDD. The word consists of a start bit 0 and then 8 bits
// which are determined from the function argument byte.
void Tx_Word(unsigned char byte){
    int bit_pos = 0;                                                            //initialise the bit position as 0
    Delay_ms(WORD_DELAY);                                                       //wait a word Delay 
    us500Enable = 1;                                                            //enable the 500us timer
    Tx_0(); // start bit                                                        //transmit the start bit
    while (bit_pos < 8){                                                        //while not all 8 bits have been transmitted
        if (byte & 0b10000000)                                                  //if the MSB is a 1
            Tx_1();                                                             //transmit a 1
        else                    
            Tx_0();                                                             //otherwise transmit a 0
        byte = byte << 1;                                                       //shift the byte by one bit
        bit_pos++;                                                              //increment the reference
    }
    tx = 1;                                                                     //set the line high
    Delay_500us();                                                              //wait 500us
    Set_Line_High();                                                            //force the line high
    us500Enable = 0;                                                            //turn the 500 us timer off   
}

//receives a response word from an EDD. It will immediately return if it does not
//pick up the start bit before the timeout period
unsigned char rxWordEDD(void){
    watchCounter = 0;                                                           //clear the watch counter
    us100Enable = 1;                                                            //enable the 100us reception timer
    Delay_ms(3);
    while(!detectRisingEdgeEDD(RX_BIT_HIGH) && watchCounter < 200);             //wait for the start bit
    if(watchCounter >= 200){                                                    //if the timeout period has been exceeded
        us100Enable = 0;                                                        //no start bit then return
        return 0;                                                               //return false
    }
    watchCounter = 0;                                                           //clear the watch counter
    while(!detectFallingEdgeEDD(RX_BIT_HIGH) && watchCounter < 100);                       //wait for the falling edge of the start bit
    if(watchCounter >= 100){                                                    //no start bit then return
        us100Enable = 0;                                                        //no start bit then return
        return 0;
    }
    watchCounter = 0;                                                           //clear the watch counter
    period = 0;                                                                 //clear the period counter
    eddState = 0;                                                               //initially the edd bit state is 0
    readFlag = 1;                                                               //set the read flag to show a byte is expected
    halfBits = 0;                                                               //clear the half bits counter                                                
    bitCounter = 0;                                                             //clear the bit counter
    while(readFlag){                                                            //while bits are still expected
        while(!detectRisingEdgeEDD(RX_BIT_HIGH) && watchCounter < 50);          //wait for a rising edge or for a bit timeout to occur
        period = 0;                                                             //clear the period counter
        if(!readFlag)                                                           //if the byte is completely read exit the loop
            break;  
        while(!detectFallingEdgeEDD(RX_BIT_HIGH)&& watchCounter < 50);          //wait for a falling edge or for a bit timeout to occur
        watchCounter = 0;                                                       //clear the watchCounter
        period = 0;                                                             //clear the period Counter
    }
    readFlag = 0;                                                               //clear the read Flag 
    us100Enable = 0;                                                            //disable the timer
    Delay_ms(3);                                                                //wait an initial 3ms before the next word
    return 1;                                                                   //return 1 if a full byte was received
}

//transmits and receives a set number of words prescribed as arguments.
//Received words are stored in response in order of reception
unsigned char commandResponse(unsigned char TxLength, unsigned char RxLength, unsigned char *data){
    Delay_ms(50);                                                               //wait 50ms before transmitting
    if(previousADC > 0x316)
        FLAGS.programCableFault = 1;
    for(int i = 0; i < TxLength; i++){                                          //for each word
        Tx_Word(*(data + i));                                                   //transmit the word
    }
    for(int i = 0; i < RxLength; i++){                                          //for each received word
        if(rxWordEDD())                                                         //if there was a valid response
            response[i] = byteRead;                                             //assign the char to response
        else 
            return 0;                                                           //otherwise exit and return false
    }
    return 1;                                                                   //all words successfully received so return true
}

//reads a UID in a specified window. If the window is 0 the UID is added to memory
//otherwise if in another window it is compared to the UID already in memory. A
//1 is returned if a UID is successfully read or if the current UID matches the 
//response read.
unsigned char readUID(unsigned char window){
    unsigned char attempts = 0;                                                 //initialise the attempts to 0
    unsigned char txPacket[readUIDTxLength];                                    //initialise the instruction to be sent
    txPacket[0] = EDD_READ_UID_COMMAND;                                         //assign the command
    txPacket[1] = 0;                                                            //first three bytes will always be 0 
    txPacket[2] = 0;                                                            //as max window is only maxDets
    txPacket[3] = 0;                                                            //
    txPacket[4] = window;                                                       //last byte is the window   
    if(txPacket[4] == maxDets)                                                  //if the window is maxDets this is a special case
        txPacket[4] = 0;                                                        //for reading window 0
    while(!commandResponse(readUIDTxLength, readUIDRxLength, txPacket) && attempts++ < maxAttempts );//while there has not been a valid response and there have been less than 4 attempts
    if(attempts < maxAttempts){                                                 //if there was a valid response within 4 attempts
        if(!txPacket[1]){                                                       //if it was in window 0
            if(checkForExistingUID(response) || ABB_1.dets_length >= 100)       //check to see if the det already exists
                return 1;                                                       //if so return 1
            ABB_1.dets_length++;                                                //otherwise add it to memory
            ABB_1.det_arrays.info[ABB_1.dets_length].delay = STANDARD_EDD_DELAY;//assign the standard delay
            ABB_1.det_arrays.info[ABB_1.dets_length].data.connection_status = 0;//set its connection status to 0 until it has been confirmed to be programmable       
            for(int i = 0; i < 4; i++){                                         //for each UID byte
                ABB_1.det_arrays.UIDs[ABB_1.dets_length].UID[i] = response[i];  //assign it to memory
            }
        }
        else{
            for(int i = 0; i < 4; i++){                                         //otherwise for each byte 
                if(ABB_1.det_arrays.UIDs[ABB_1.dets_length].UID[i] != response[i])//compare it to memory
                    return 0;                                                   //if any bytes do not match return a 0
            }
            
        }
        return 1;                                                               //return success
    }
    return 0;                                                                   //no response so return 0
}

//programs a UID with a specified window and delay from data memory. Returns a 1 
//if the detonator responds with the correct window and delay.
unsigned char programUID(unsigned char window){
    unsigned char levelOfSuccess = 0;
    unsigned char attempts = 0;                                                 //initialise the attempts to 0
    unsigned char txPacket[9];                                                  //prepare the Instruction
    txPacket[0] = EDD_PROGRAM_UID_COMMAND;                                      //set the command
    txPacket[1] = ABB_1.det_arrays.UIDs[window].UID[0];                         //assign the first byte of the UID
    txPacket[2] = ABB_1.det_arrays.UIDs[window].UID[1];                         //assign the second byte of the UID                         
    txPacket[3] = ABB_1.det_arrays.UIDs[window].UID[2];                         //assign the third byte of the UID
    txPacket[4] = ABB_1.det_arrays.UIDs[window].UID[3];                         //assign the fourth byte of the UID
    txPacket[5] = 0;                                                            //the MSB of the window is always 0
    txPacket[6] = window;                                                       //assign the window
    txPacket[7] = ABB_1.det_arrays.info[window].delay >> 8;                     //assign the MSB of the delay
    txPacket[8] = (unsigned char) ABB_1.det_arrays.info[window].delay;          //assign the LSB of the delay
    while(!commandResponse(programUIDTxLength, programUIDRxLength, txPacket) && attempts++ < maxProgramAttempts);//while there has not been a valid response and there have been less than 4 attempts
    if(attempts < maxProgramAttempts){                                             //if there was a valid response before maximum attempts
        levelOfSuccess++;
        unsigned short delayCompare = (unsigned short) response[2] << 8;        //assign the MSB of the received delay
        delayCompare |= (unsigned short) response[3];                           //assign the LSB of the received delay
        unsigned short windowCompare = (unsigned short) response[0] << 8;       //assign the MSB of the Window
        windowCompare |= (unsigned short) response[1];                          //assign the LSB of the window
        if (windowCompare){
            if(ABB_1.det_arrays.info[window].delay == delayCompare)
                levelOfSuccess += 2;
            if(window == windowCompare)                                         //if both the window and delay are correct
                levelOfSuccess++;                                               //return success
        }                                                           
    }
    return levelOfSuccess;                                                               //return a fail if there was not a valid response
}

//returns a 1 if another attempt should not be made
unsigned char programRetryChecks(unsigned char window){
    switch(programUID(window)){
        case 0 :
            programRetries = 0;
            return 1;
            
        case 1:
            programRetries --;
            if(programRetries > 0)
                return 0;
            else
                return 1;
            
        case 2 :
            programRetries--;
            if(programRetries > 0)
                return 0;
            else
                return 1;
            
        case 3 :
            return 1;
            
        case 4:
            return 1;
    }
}

//checks the status of a detonator and should only be called after calibration in 
//the programming routine. Also assigns the returned data of an EDD to memory.
unsigned char selfCheckUID(unsigned short window){                              
    unsigned char attempts = 0;                                                 //initialise the attempts to 0
    unsigned char txPacket[3];                                                  //prepare the Instruction
    txPacket[0] = EDD_SELF_CHECK_COMMAND;                                       //assign the command
    txPacket[1] = 0;                                                            //MSB of the window is always 0
    txPacket[2] = window;                                                       //assign the LSB of the window
    while(!commandResponse (selfCheckUIDTxLength, selfCheckUIDRxLength, txPacket) && attempts++ < maxAttempts);//while there has not been a valid response and less than 4 attempts have been made
    if(attempts < maxAttempts){                                                 //if there was a valid response before max attempts tried
        if(response[0] & 0b10000000)                                            //assign the status of energy storing
            ABB_1.det_arrays.info[window].data.energy_storing = 1;              //if 1 then set it in memory
        else
            ABB_1.det_arrays.info[window].data.energy_storing = 0;              //if zero then clear it in memory

        if(response[0] & 0b01000000)                                            //assign the status of the bridge wire
            ABB_1.det_arrays.info[window].data.bridge_wire_resitance = 1;       //if 1 then set it in memory
        else
            ABB_1.det_arrays.info[window].data.bridge_wire_resitance = 0;       //if 0 clear it in memory

        if(response[0] & 0b00100000)                                            //assign the status of EDD calibration
            ABB_1.det_arrays.info[window].data.calibration_status = 1;          //if calibrated set it in memory
        else
            ABB_1.det_arrays.info[window].data.calibration_status = 0;          //clear it in memory

        if(response[0] & 0b00001000)                                            //assign the status of programming
            ABB_1.det_arrays.info[window].data.program_status = 1;              //if programmed set it in memory
        else
            ABB_1.det_arrays.info[window].data.program_status = 0;              //otherwise clear it in memory

        if(response[0] == 0x68)                                                 //if the response indicates the EDD is programmed calibrated and the bridge wire resistance is fine
            return 1;                                                           //return success
        else 
            return 0;                                                           //otherwise return fail
    }
    else
        return 0;                                                               //no response so return 0
}

//reads window 0 for UIDs and returns 1 if any form of response is received
unsigned char checkLineForConnectedUIDs(void){
    unsigned char attempts = 0;                                                 //initialise the attempts to 0
    unsigned char txPacket[readUIDTxLength];                                    //prepare the instruction
    txPacket[0] = EDD_READ_UID_COMMAND;                                         //assign the command
    txPacket[1] = 0;                                                            //set the window to 0
    txPacket[2] = 0;                                                            //set the window to 0
    txPacket[3] = 0;                                                            //set the window to 0
    txPacket[4] = 0;                                                            //set the window to 0
    while(!commandResponse(readUIDTxLength, readUIDRxLength, txPacket) && attempts++ < maxAttempts );//while there has not been a valid response and fewer than 4 attempts have been made
    if(attempts < maxAttempts){                                                 //if a response was received before max attempts made
        return 1;                                                               //return success
    }   
    return 0;                                                                   //otherwise return fail
}

//Sends the calibration command and the calibration pulses
void EDD_Calibrate(void){                                                       
    unsigned char EDD_calibration_command;                      
    EDD_calibration_command = EDD_CALIBRATE_COMMAND;                            //prepare the instruction
    
    //Tx_Calibration_Pulses_90();
    
    Tx_Word(EDD_calibration_command);                                           //transmit the command
    Delay_ms(WORD_DELAY);                                                       //delay 13ms
    
    Tx_Calibration_Pulses();                                                    //send the calibration pulses
    Set_Line_High();                                                            //set the line high
    Delay_ms(3000);                                                             //wait 3 seconds
}

void Tx_Calibration_Pulses(void)
{
    for (int i = 0; i < CALIBRATION_PULSES; i++){                               //loop for the number of calibration pulses
        Set_Line_Low();                                                         //set the line low
        for (int j = 0; j < 10 - PULSE_DUTY; j++){                              //for 10 - the pulse duty
            Delay_100us();                                                      //keep the line low for 100us
        }     
        Set_Line_High();                                                        //set the line high
        for (int j = 0; j < PULSE_DUTY; j++){                                   //for the duty cycle
            Delay_100us();                                                      //keep the line high for 100us
        }
        CLRWDT();
    }
}

void Tx_Calibration_Pulses_90(void)
{
    for (int i = 0; i < CALIBRATION_PULSES; i++){                               //loop for the number of calibration pulses
        Set_Line_Low();                                                         //set the line low
        for (int j = 0; j < 10 - 9; j++){                                       //for 10 - the pulse duty
            Delay_100us();                                                      //keep the line low for 100us
        }     
        Set_Line_High();                                                        //set the line high
        for (int j = 0; j < 9; j++){                                            //for the duty cycle
            Delay_100us();                                                      //keep the line high for 100us
        }
        CLRWDT();
    }
}

//sends the commands to tell the EDDs to store Energy before firing
void EDD_Energy_Store(void){                                                
    unsigned char EDD_energy_store_command;
    EDD_energy_store_command = EDD_ENERGY_STORE_COMMAND;                        //prepare the instruction
    
    Tx_Word(EDD_energy_store_command);                                          //transmit the command
    Delay_ms(WORD_DELAY);                                                       //wait a word delay
    Tx_Word(EDD_energy_store_command);                                          //transmit the command
    Delay_ms(WORD_DELAY);                                                       //wait a word delay
    Delay_ms(ENERGY_STORE_DELAY);                                               //wait the energy storing time
}

void EDD_Blast(void){
    unsigned char EDD_blast_command;
    EDD_blast_command = EDD_BLAST_COMMAND;
    
    Tx_Word(EDD_blast_command);
    Delay_ms(WORD_DELAY);
    Tx_Word(EDD_blast_command);
    Delay_ms(WORD_DELAY);
    Tx_Word(EDD_blast_command);
    Delay_ms(WORD_DELAY);
    Tx_Word(EDD_blast_command);
    Delay_ms(WORD_DELAY);
    Tx_Word(EDD_blast_command);
}

//sends the discharge command
void EDD_Discharge(void){                                                                                                          
    unsigned char EDD_discharge_command;
    EDD_discharge_command = EDD_DISCHARGE_ENERGY_COMMAND;                       //prepare the instruction
    
    Tx_Word(EDD_discharge_command);                                             //sends the command
    Delay_ms(WORD_DELAY);                                                       //wait a word delay
}

//differential ADC detection that waits for an increase in values by more than 
//RX_BIT_HIGH indicating an edge detected as a response from and EDD
unsigned char detectRisingEdgeEDD(unsigned char change){
    volatile unsigned short currentADC;                                         //create a variable for comparison     
    currentADC =  readEddADC();                                                 //read the ADC and assign it to the variable
    if(previousADC + change < currentADC){                                      //if the current value is 10 greater than the previous value
        previousADC = currentADC;                                               //update the previous value for the next comparison
        return 1;                                                               //return that a rising edge has been detected
    }
    else{
        previousADC = currentADC;                                               //otherwise just update the previous value
        return 0;                                                               //return no edge detected
    }
}

unsigned char detectFallingEdgeEDD(unsigned char change){
    volatile unsigned short currentADC;                                         //create a variable for comparison
    currentADC =  readEddADC();                                                 //read the ADC and assign it to the variable
    if(previousADC - change > currentADC){                                 //if the previous value is 10 greater than the current
        previousADC = currentADC;                                               //update the previous value for the next comparison
        return 1;                                                               //return 1
    }
    else{
        previousADC = currentADC;                                               
        return 0;
    }
}

//performs a read on the EDD pin for responses from EDD after Commands
unsigned short readEddADC(void){ 
    ADRES = 0;                                                                  //clear the ADRES register
    ADCON0bits.ADGO = 1;                                                        //start ADC conversion
    NOP();                                                                      //introduce a small delay
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    while (ADCON0bits.ADGO);                                                    //wait for conversion to finish
    return ADRES;                                                               //return the ADC in ADRES
}