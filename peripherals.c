/*
 * File:   peripherals.c
 * Author: Tim Buckley
 *
 * Created on 04 April 2017, 11:22 AM
 */

#include "main.h"

extern void stateCounterHandler(void);
void Delay_100us(void);
void _delay_us(unsigned char us);
void turnOn24V(void);

extern void eddCommsIsr4(void);
extern void eddCommsIsr8(void);

inline void WaitNewTick(void);

extern void checkOutgoingMessages(void);


//handles the 500us interrupt
void us500Isr(void){
    us500InterruptFlag = 0;                                                     //clear the interrupt Flag                             
    FLAGS.us500 = 1;                                                            //set the 500us flag
    eddCommsIsr4();                                                             //process the EDD communications
}

//handles the ms interrupt
void msIsr(void){
    msInterruptFlag = 0;                                                        //clear the ms Interrupt flag
    FLAGS.ms = 1;                                                               //set the ms Flag
    stateCounterHandler();                                                      //process the states for Background Processes
    
    if(!COUNTERS.sec3)                                                          //if the 3 second counter has reached zero
        FLAGS.sec3 = 1;                                                         //set the flag 
    else
        COUNTERS.sec3--;                                                        //otherwise decrement it
    
    if(!COUNTERS.sec2)                                                          //if the 2 second counter has reached zero
        FLAGS.sec2 = 1;                                                         //set the 2 second flag
    else
        COUNTERS.sec2 --;                                                       //otherwise decrement it
    
    if(!COUNTERS.sec1)                                                          //if the 2 second counter has reached zero
        FLAGS.sec1 = 1;                                                         //set the 2 second flag
    else
        COUNTERS.sec1 --;                                                       //otherwise decrement it
    
    if(!COUNTERS.min)                                                           //if the minute counter has reached 0
        FLAGS.min = 1;                                                          //set the minute flag
    else
        COUNTERS.min --;                                                        //otherwise decrement it
    
    if(!COUNTERS.min10){                                                        //if the 10 minute counter has reached 0
        FLAGS.min10 = 1;                                                        //set the 10 minute flag
    }
    else
        COUNTERS.min10 --;                                                      //otherwise decrement it
    
    if(!COUNTERS.bluetoothTimer){                                                        //if the 10 minute counter has reached 0
        FLAGS.bluetoothTimer = 1;                                                        //set the 10 minute flag
    }
    else
        COUNTERS.bluetoothTimer --;                                                      //otherwise decrement it
    
    if(!COUNTERS.communicationStatus){                                          //if the communication status counter has reached 0
        if(!FLAGS.communication_status)                                         //if the communication flag has not been set in this time
            ABB_1.info.statusBits.communication_status = 0;                     //set the communication status of the device to 0
        
        else
            checkOutgoingMessages();
        FLAGS.communication_status = 0;                                         //clear the communication status flag
        COUNTERS.communicationStatus = noCommsPeriod;                           //load the counter again
    }
    else                                                                        //otherwise
        COUNTERS.communicationStatus--;                                         //decrement the counter
}

//processes the 100us delay interrupt
void us100DelayIsr(void){
    FLAGS.us100 = 1;                                                            //set the us100 delay Flag                                             
    us100DelayInterruptFlag = 0;                                                //clear the interrupt flag
}

//processes the 100us delay for EDD reads
void us100Isr(void){
    us100InterruptFlag = 0;                                                     //clear the us100 Interrupt Flag
    eddCommsIsr8();                                                             //process the EDD response
}

//ms delay for EDD comms
void Delay_ms(int ms)
{
    for (int i = 0; i < ms; i++){                                               //for every ms   
        for(int j = 0; j < 10; j++){                                            //10 times
            Delay_100us();                                                      //delay 100us
        }
    }
}

//500us delay for EDD comms
void Delay_500us(void){
    PR4=230;
    while (!FLAGS.us500);                                                       //wait for the 500us flag
    FLAGS.us500 = 0;                                                            //clear the flag
}

void Delay_500us_X(void){
    PR4=169;
    while (!FLAGS.us500);                                                       //wait for the 500us flag
    FLAGS.us500 = 0;                                                            //clear the flag
}

//100us delay for EDD comms
void Delay_100us(void){
    us100DelayEnable = 1;                                                       //ensure that the timer is enabled
    us100Interrupt = 1;                                                         //ensure that the interrupt is enabled
    while(!FLAGS.us100);                                                        //wait for the 100us Flag
    FLAGS.us100 = 0;                                                            //cleat the flag
}

//us delay without interrupts
void _delay_us(unsigned char us){
    while(us-- > 0){                                                            //wait the desired number of us
        NOP();                                                                  //NOPs to create wait period
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
    }
}

//ms delay without interrupt
void _delay_ms(unsigned int delay){                                              
    while(delay-- > 0){                                                         //wait for the delay to reach 0 and decrement
        for (int i = 0; i < 10; i ++){                                          //loop 10 times
           _delay_us(100);                                                      //delay 100us
        }
    }
}

//peripheral port select lock sequence
void ppsLock(void){
    INTCONbits.GIE = 0;
    PPSLOCK = 0x55;
    NOP();
    NOP();
    PPSLOCK = 0xAA;
    NOP();
    NOP();
    PPSLOCKbits.PPSLOCKED = 1;
}

//peripheral port select unlock sequence
void ppsUnlock(void){
    INTCONbits.GIE = 0;
    PPSLOCK = 0x55;
    NOP();
    NOP();
    PPSLOCK = 0xAA;
    NOP();
    NOP();
    PPSLOCKbits.PPSLOCKED = 0;
}

//returns the variance of a set of data.
//used to ensure consistent ADC readings
unsigned short variance(unsigned short *values, unsigned char sampleSize){
    unsigned long sum = 0;                                                      //initialise the sum to 0
    unsigned short mean = 0;                                                    //initialise the mean to 0
    
    for (int i = 0; i < sampleSize; i++){                                       //for every sample
        sum += values[i];                                                       //compute the sum
    }
    mean = sum/sampleSize;                                                      //calculate the mean
    sum = 0;                                                                    //set the sum to 0
    
    for (int i = 0; i < sampleSize; i++){                                       //for every sample
        sum += (values[i] - mean)*(values[i] - mean);                           //calculate the variance
    }
    return sum /(sampleSize -1);                                                //return the variance
}

//checks the memory of EDDs for an exsiting UID
unsigned char checkForExistingUID(unsigned long *receivedUID){
    unsigned long *currentUID;                                                  //create a pointer to for comparison
    for(int i = 0; i < ABB_1.dets_length; i ++){                                //for every EDD
        currentUID = ABB_1.det_arrays.UIDs[i].UID;                              //assign the value of the UID to the pointer
        if(*currentUID == *receivedUID || !*receivedUID)                        //compare the pointer with the received UID
            return 1;                                                           //if there is a match or the UID is completely 0 return success
    }
    return 0;                                                                   //otherwise return fail
}

void writeByteToEeprom(unsigned char *byte){
    unsigned int address = byte - &ABB_1;
    if(address >= 240)
        NOP();
    NVMCON1bits.NVMREG0 = 0;
    NVMCON1bits.NVMREG1 = 0;
    NVMADRL = address & 0xFF;
    NVMADRH = (address >> 8) & 0xFF;
    NVMDAT = *byte;
    NVMCON1bits.WREN = 1;
    INTCONbits.GIE = 0;
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    while(NVMCON1bits.WR);
    INTCONbits.GIE = 1;
    NVMCON1bits.WREN = 0;
}

void readByteFromEeprom(unsigned char *byte){
    unsigned int address = byte - &ABB_1;
    NVMCON1bits.NVMREG0 = 0;
    NVMCON1bits.NVMREG1 = 0;
    NVMADRL = address & 0xFF;
    NVMADRH = (address >> 8) & 0xFF;
    NVMCON1bits.RD = 1;
    while(NVMCON1bits.RD);
    *byte = NVMDAT;
}

void writeStructureToEeprom(unsigned char *structure, unsigned int size){
    for (int i = 0; i < size; i++){
        writeByteToEeprom(structure + i);
    }
}

void readStructureFromEeprom(unsigned char *data, int size){
    for(int i = 0; i < size; i++){
        readByteFromEeprom(data + i);
    }
}

void testMemory(void){
//    ABB_1.dets_length = 16;
//    
//    ABB_1.det_arrays.UIDs[1].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[1].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[1].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[1].UID[3] = 242;
//    
//    ABB_1.det_arrays.UIDs[2].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[2].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[2].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[2].UID[3] = 249;
//    
//    ABB_1.det_arrays.UIDs[3].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[3].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[3].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[3].UID[3] = 201;
//    
//    ABB_1.det_arrays.UIDs[4].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[4].UID[1] = 72;
//    ABB_1.det_arrays.UIDs[4].UID[2] = 230;
//    ABB_1.det_arrays.UIDs[4].UID[3] = 45;
//    
//    ABB_1.det_arrays.UIDs[5].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[5].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[5].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[5].UID[3] = 238;
//    
//    ABB_1.det_arrays.UIDs[6].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[6].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[6].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[6].UID[3] = 197;
//    
//    ABB_1.det_arrays.UIDs[7].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[7].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[7].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[7].UID[3] = 250;
//    
////    ABB_1.det_arrays.UIDs[8].UID[0] = 27;
////    ABB_1.det_arrays.UIDs[8].UID[1] = 72;
////    ABB_1.det_arrays.UIDs[8].UID[2] = 230;
////    ABB_1.det_arrays.UIDs[8].UID[3] = 36;
//    
//    ABB_1.det_arrays.UIDs[8].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[8].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[8].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[8].UID[3] = 190;
//    
////    ABB_1.det_arrays.UIDs[10].UID[0] = 27;
////    ABB_1.det_arrays.UIDs[10].UID[1] = 60;
////    ABB_1.det_arrays.UIDs[10].UID[2] = 86;
////    ABB_1.det_arrays.UIDs[10].UID[3] = 203;
////    
////    ABB_1.det_arrays.UIDs[11].UID[0] = 27;
////    ABB_1.det_arrays.UIDs[11].UID[1] = 72;
////    ABB_1.det_arrays.UIDs[11].UID[2] = 230;
////    ABB_1.det_arrays.UIDs[11].UID[3] = 31;
//    
//    ABB_1.det_arrays.UIDs[9].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[9].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[9].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[9].UID[3] = 191;
//    
//    ABB_1.det_arrays.UIDs[10].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[10].UID[1] = 72;
//    ABB_1.det_arrays.UIDs[10].UID[2] = 230;
//    ABB_1.det_arrays.UIDs[10].UID[3] = 34;
//    
////    ABB_1.det_arrays.UIDs[14].UID[0] = 27;
////    ABB_1.det_arrays.UIDs[14].UID[1] = 76;
////    ABB_1.det_arrays.UIDs[14].UID[2] = 84;
////    ABB_1.det_arrays.UIDs[14].UID[3] = 238;
//    
//    ABB_1.det_arrays.UIDs[16].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[16].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[16].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[16].UID[3] = 237;
//    
//    ABB_1.det_arrays.UIDs[11].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[11].UID[1] = 75;
//    ABB_1.det_arrays.UIDs[11].UID[2] = 195;
//    ABB_1.det_arrays.UIDs[11].UID[3] = 149;
//    
//    ABB_1.det_arrays.UIDs[12].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[12].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[12].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[12].UID[3] = 194;
//    
//    ABB_1.det_arrays.UIDs[13].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[13].UID[1] = 72;
//    ABB_1.det_arrays.UIDs[13].UID[2] = 230;
//    ABB_1.det_arrays.UIDs[13].UID[3] = 42;
//    
//    ABB_1.det_arrays.UIDs[14].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[14].UID[1] = 60;
//    ABB_1.det_arrays.UIDs[14].UID[2] = 86;
//    ABB_1.det_arrays.UIDs[14].UID[3] = 188;
//    
//    ABB_1.det_arrays.UIDs[15].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[15].UID[1] = 76;
//    ABB_1.det_arrays.UIDs[15].UID[2] = 84;
//    ABB_1.det_arrays.UIDs[15].UID[3] = 241;
//    
//    for (int i = 1; i <= 16; i++){
//        ABB_1.det_arrays.info[i].delay = i*500; 
//    }
    
    //Test Memory for Old 100 Det Box
    ABB_1.dets_length = 100;
    for (int i = 1; i <= 10; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
        ABB_1.det_arrays.UIDs[i].UID[1] = 17;
        ABB_1.det_arrays.UIDs[i].UID[2] = 3;
        ABB_1.det_arrays.UIDs[i].UID[3] = 130 - i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
    for (int i = 11; i <= 20; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
        ABB_1.det_arrays.UIDs[i].UID[1] = 17;
        ABB_1.det_arrays.UIDs[i].UID[2] = 3;
        ABB_1.det_arrays.UIDs[i].UID[3] = 129 - i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
    for (int i = 21; i <= 40; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
        ABB_1.det_arrays.UIDs[i].UID[1] = 16;
        ABB_1.det_arrays.UIDs[i].UID[2] = 236;
        ABB_1.det_arrays.UIDs[i].UID[3] = 8 + i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
    for (int i = 41; i <= 60; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
        ABB_1.det_arrays.UIDs[i].UID[1] = 16;
        ABB_1.det_arrays.UIDs[i].UID[2] = 235;
        ABB_1.det_arrays.UIDs[i].UID[3] = 74 + i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
    for (int i = 61; i <= 80; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 28;
        ABB_1.det_arrays.UIDs[i].UID[1] = 17;
        ABB_1.det_arrays.UIDs[i].UID[2] = 95;
        ABB_1.det_arrays.UIDs[i].UID[3] = 175 + i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
    for (int i = 81; i <= 100; i++){
        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
        ABB_1.det_arrays.UIDs[i].UID[1] = 17;
        ABB_1.det_arrays.UIDs[i].UID[2] = 8;
        ABB_1.det_arrays.UIDs[i].UID[3] = 71 + i;
        ABB_1.det_arrays.info[i].delay = i*100;
    }
     
//    //Test Memory for New Black 100 Det Box
//    ABB_1.dets_length = 100;
//    for (int i = 1; i <= 8; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 67;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 236;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 162 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }    
//    for (int i = 9; i <= 16; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 67;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 236;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 163 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    ABB_1.det_arrays.UIDs[17].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[17].UID[1] = 67;
//    ABB_1.det_arrays.UIDs[17].UID[2] = 233;
//    ABB_1.det_arrays.UIDs[17].UID[3] = 64;
//    ABB_1.det_arrays.info[17].delay = 1700;
//    
//    ABB_1.det_arrays.UIDs[18].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[18].UID[1] = 67;
//    ABB_1.det_arrays.UIDs[18].UID[2] = 236;
//    ABB_1.det_arrays.UIDs[18].UID[3] = 182;
//    ABB_1.det_arrays.info[18].delay = 1800;
//    for (int i = 19; i <= 28; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 76;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 154;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 110 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    for (int i = 29; i <= 37; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 76;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 154;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 111 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    for (int i = 38; i <= 56; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 67;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 233;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 2 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    for (int i = 57; i <= 74; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 75;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 11;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 169 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    ABB_1.det_arrays.UIDs[75].UID[0] = 27;
//    ABB_1.det_arrays.UIDs[75].UID[1] = 75;
//    ABB_1.det_arrays.UIDs[75].UID[2] = 23;
//    ABB_1.det_arrays.UIDs[75].UID[3] = 58;
//    ABB_1.det_arrays.info[75].delay = 7500;
//    for (int i = 76; i <= 92; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 76;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 154;
//        ABB_1.det_arrays.UIDs[i].UID[3] = 111 + i;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    for (int i = 93; i <= 95; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 76;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 155;
//        ABB_1.det_arrays.UIDs[i].UID[3] = i-45;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
//    for (int i = 96; i <= 100; i++){
//        ABB_1.det_arrays.UIDs[i].UID[0] = 27;
//        ABB_1.det_arrays.UIDs[i].UID[1] = 67;
//        ABB_1.det_arrays.UIDs[i].UID[2] = 233;
//        ABB_1.det_arrays.UIDs[i].UID[3] = i-37;
//        ABB_1.det_arrays.info[i].delay = i*100;
//    }
}

void clearEDDStatusbits(void){
    for(int i = 0; i < 101; i++){                                               //clear the entire UID memory bank
            ABB_1.det_arrays.info[i].data.connection_status = 0;
            ABB_1.det_arrays.info[i].data.calibration_status = 0;
            ABB_1.det_arrays.info[i].data.energy_storing = 0;
            ABB_1.det_arrays.info[i].data.bridge_wire_resitance = 0;
            ABB_1.det_arrays.info[i].data.energy_storing = 0;
            ABB_1.det_arrays.info[i].data.program_status = 0;
        }
}

void turnOn24V(void){
    for(int i = 0; i < 8; i++){
        LAT_24VCntrl = 1;
        _delay_us(100);
        LAT_24VCntrl = 0;
        _delay_us(100);
        _delay_us(100);
        _delay_us(100);
        _delay_us(100);
        _delay_us(100);
    }
    LAT_24VCntrl = 1;
}