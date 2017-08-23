/*
 * File:   readVoltage.c
 * Author: R&D
 *
 * Created on 27 April 2017, 9:43 PM
 */
#include "main.h"
#include "readVoltage.h"

extern void initialiseState(states *specific, char number);
extern void turnOffAcInterrupt(void);
extern void initialiseACinterrupt(void);
unsigned char readACor36V(void);

void initialiseReadVoltage(void) { 
    ANSEL_ACor36V = 0;
    TRIS_ACor36V = 1;
    
    initialiseState(&state.readVoltage, readVoltageState);
}

void readVoltage(void){
    switch (state.readVoltage.current){
        case wait :
            return;
            
        case read36V :
            state.readVoltage.counter = 1;
            break;
            
        case voltageDetected :
            state.readVoltage.counter = readVoltageInterval - 1;
            ABB_1.info.statusBits.voltage = 1;
            testCounter = 0;
            COUNTERS.missingPulses = 0;
            break;
            
        case noVoltage :
            testCounter++;
            state.readVoltage.counter = noVoltageInterval;
            break;
            
        case noVoltageConfirmed :
            state.readVoltage.counter = readVoltageInterval - 1;
            ABB_1.info.statusBits.voltage = 0;
            break;
            
        default :
            state.readVoltage.counter = 1;
            break;
            
    }
}
//Input is ACor36
void readVoltageStateHandler(void){
    switch(state.readVoltage.current){
        case wait :
            return;
        
        case read36V :
            if(readACor36V())
                state.readVoltage.next = voltageDetected;
            else
                state.readVoltage.next = noVoltage;
            break;
            
        case noVoltage :
            if(readACor36V())
                state.readVoltage.next = voltageDetected;
            else if(testCounter >= samples36V)
                state.readVoltage.next = noVoltageConfirmed;
            else
                state.readVoltage.next = noVoltage;
            break;
            
        case noVoltageConfirmed :
            state.readVoltage.next = read36V;
            break;
                
        default :
            state.readVoltage.next = read36V;                                   //after all other states restart the process
            break;
    }
            
            
}

unsigned char readACor36V(void){
    return PORT_ACor36V;
}