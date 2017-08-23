/*
 * File:   acOr36V.c
 * Author: Tim Buckley
 *
 * Created on 13 June 2017, 8:04 AM
 */
#include "main.h"

void reset4SecTimer(void);
void sec4OverflowIsr(void);
void reset13msTimer(void);
void checkPulse(void);
void initialiseACinterrupt(void);

extern void wake(void);

//Initialises the pins and the interrupts associated with the ACor36V detection
//This includes going to sleep and missing pulse detection for firing
void intialiseAcOr36V(void){
    TRIS_ACor36V = 1;                                                           //Set the pin as an input
    ANSEL_ACor36V = 0;                                                          //Set the pin as digital
    initialiseACinterrupt();                                                    //initialise the interrupt
}

//AC or 36V interrupt service routine
//Determines whether the device should be waking up or if its a normal Edge from
//the earth leakage test or AC detection
void acOr36VISR(void){
    switch(ABB_1.deviceState){
        case sleepDevice :                                                      //if the device is asleep
            wake();                                                             //then wake up
            break;
        default :
            checkPulse();                                                       //otherwise process it as a pulse
            break;
    }
}

//determines the condition that a pulse has occurred under
//Either AC, missing pulse or 
void checkPulse(void){
    if(ms13InterruptFlag){                                                      //if more than 13ms has passed since the last falling edge
        if(COUNTERS.acPulses >= acMissingPulseCount){                           //if there have been 20 or more ac Pulses
            COUNTERS.missingPulses++;                                           //increment the missing oulse count
        }
        else if(ABB_1.deviceState != fireDevice)                                //otherwise
        COUNTERS.missingPulses = 0;                                             //clear the missing pulse counter
        COUNTERS.acPulses = 0;                                                  //clear the ac pulse counter
    }
    else{                                                                       //time between pulses is less than 13ms
       COUNTERS.acPulses++;                                                     //must be ac so increment the counter
    }
    if(ABB_1.deviceState != fireDevice)
        reset4SecTimer();                                                           //clear the 4 second timer
    reset13msTimer();                                                           //clear the 13ms second timer
    ABB_1.info.statusBits.voltage = 1;                                          //under any circumstance that this interrupt occurs there must be voltage on the line
        
}

//Shows there has been no 36V detected for 4 seconds
void sec4OverflowIsr(void){
    sec4InterruptFlag = 0;                                                      //clear the interrupt Flag
    if(!PORT_ACor36V)
        ABB_1.info.statusBits.voltage = 0;                                      //set the voltage status bit to 0
    else
        ABB_1.info.statusBits.voltage = 1;                                      //there is actually 36V  
}

//clears the 4 second overflow timer
void reset4SecTimer(void){
    sec4Interrupt = 0;                                                          //disable the interrupt
    sec4Enable = 0;                                                             //disbale the timer
    sec4Timer = 0;                                                              //cleat the timer
    sec4Enable = 1;                                                             //enable the timer
    sec4Interrupt = 1;                                                          //and the the interrupt
}

//clears the 13ms Timer
void reset13msTimer(void){                                                      
    ms13Enable = 0;                                                             //disable the timer
    ms13InterruptFlag = 0;                                                      //clear the interrupt flag
    ms13Timer = ms13TimerValue;                                                 //reload the timer value
    ms13Enable = 1;                                                             //enable the timer
}

//enables the ac interrupt 
void initialiseACinterrupt(void){
    acOr36VInterruptFlag = 0;                                                   //clear the interrupt flag
    acOr36VInterruptEdge = 0;                                                   //set the interrupt to trigger on a falling edge
    acOr36VInterrupt = 1;                                                       //enable the interrupt
}
