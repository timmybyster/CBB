/*
 * File:   readSupply.c
 * Author: Tim Buckley
 *
 * Created on 17 April 2017, 6:04 PM
 */

#include "main.h"
#include "readSupply.h"

unsigned short readBatteryADC(void);
void readBatteryADCSetup(void);
unsigned char readMainsInterval(void);

extern void initialiseState(states *specific, char number);
extern void _delay_us(unsigned char us);
extern unsigned short variance(unsigned short *values, unsigned char sampleSize);

//initialises pins and states for the read supply process
void initialiseReadSupply(void) { 
    ANSEL_BatSense = 1;                                                         //set the pin to analog
    TRIS_BatSense = 1;                                                          //set the pin as an input
    
    TRIS_MainsDetect = 1;                                                       //set the pin as an input
    
    initialiseState(&state.readSupply, readSupplyState);                        //initialise the read supply state 
}

//executes code from the read supply process based on the current process state
void readSupply(void){
    switch (state.readSupply.current){
        case mainsDetected :
            state.readSupply.counter = readSupplyInterval - 1;                  //load the state counter to move to the next state
            ABB_1.info.statusBits.mains = 1;                                    //set the mains detected bit in memory
            ABB_1.info.statusBits.lowBat = 0;                                   //clear the low battery bit in memory
            ABB_1.info.statusBits.lowBat2 = 0;                                  //clear the too low battery in memory
            testCounter = 0;                                                    //clear the test counter
            break;
            
        case readBattery :
            state.readSupply.counter = 1;                                       //load the counter to move to the next state
            testCounter ++;                                                     //increment the test counter
            break;
            
        case lowBattery :
            state.readSupply.counter = readSupplyInterval - 2;                  //load the state counter to move to the next state
            ABB_1.info.statusBits.mains = 0;                                    //clear the mains detected bit in memory
            ABB_1.info.statusBits.lowBat = 1;                                   //set the low battery bit in memory
            ABB_1.info.statusBits.lowBat2 = 0;                                  //clear the too low battery bit in memory
            testCounter = 0;                                                    //clear the test counter
            break;
            
        case lowBattery2 :
            state.readSupply.counter = readSupplyInterval - 2;                  //load the state counter to move to the next state
            ABB_1.info.statusBits.mains = 0;                                    //clear the mains detected bit in memory
            ABB_1.info.statusBits.lowBat = 1;                                   //set the low battery bit in memory
            ABB_1.info.statusBits.lowBat2 = 1;                                  //set the too low battery bit in memory
            testCounter = 0;                                                    //clear the test counter 
            break;
            
        case noMains :
            state.readSupply.counter = readSupplyInterval - 2;                  //load the state counter to move to the next state
            ABB_1.info.statusBits.mains = 0;                                    //clear the mains detected bit in memory
            ABB_1.info.statusBits.lowBat = 0;                                   //clear the low battery bit in memory
            ABB_1.info.statusBits.lowBat2 = 0;                                  //clear the too low battery bit in memory
            testCounter = 0;                                                    //clear the test counter 
            break;
            
        default :
            state.readSupply.counter = 1;                                       //default should be to move to the next state immediately
            break;
            
    }
}

//Determines the next state based on the current state for the read supply process
//Inputs are BatSense and Mains Detect
void readSupplyStateHandler(void){
    unsigned short batteryVoltage;                                              //temporary variable for comparison
    switch(state.readSupply.current){
        case readMains :
            if(readMainsInterval())                                             //if there is mains connected
                state.readSupply.next = mainsDetected;                          //show that there is mains
            else
                state.readSupply.next = readBattery;                            //otherwise check the battery
            break;
            
        case readBattery :
            batteryVoltage = readBatteryADC();                                  //read the battery voltage
            if(ABB_1.info.statusBits.lowBat2){                                  //if the device is in a too low battery state
                if(batteryVoltage <= lowBattery2Read + 30)                      //only if the battery voltage has gone well above the threshold
                    state.readSupply.next = lowBattery2;                        //otherwise stay in the too low battery state
                else if(batteryVoltage <= lowBatteryRead)                       //if its above the low battery threshold
                    state.readSupply.next = lowBattery;                         //go to the low battery state
                else
                    state.readSupply.next = noMains;                            //otherwise just indicate there is no mains
            }
            else if(ABB_1.info.statusBits.lowBat){                              //if in the low battery state
                if(batteryVoltage <= lowBattery2Read)                           //if the voltage is less than the too low battery threshold 
                    state.readSupply.next = lowBattery2;                        //go to the too low battery state
                else if(batteryVoltage <= lowBatteryRead + 30)                  //only if the voltage is well above the threshold
                    state.readSupply.next = lowBattery;                         //stay in low battery
                else                                                            //otherwise
                    state.readSupply.next = noMains;                            //otherwise just indicate there is no mains
            }
            else{                                                               //otherwise previously mains or just no mains
                if(batteryVoltage <= lowBattery2Read)                           //as long as the voltage is below the too low battery threshold
                    state.readSupply.next = lowBattery2;                        //go to the too low battery state 
                else if(batteryVoltage <= lowBatteryRead)                       //voltage below the low battery threshold
                    state.readSupply.next = lowBattery;                         //go to the low battery state
                else
                    state.readSupply.next = noMains;                            //otherwise just go to the no mains state
            }
            break;
            
        default :
            state.readSupply.next = readMains;                                  //after all other states restart the process
            break;
    }
            
            
}

unsigned short readBatteryADC(void){
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    readBatteryADCSetup();                                                      //initialise the ADC  
    _delay_us(ADCDelay);                                                        //give the ADC time to setup
    ADRES = 0;                                                                  //cleat the ADRES value
    while(currentVariance > allowedVariance){                                   //read the values until there is a stable set of readings
        ADCON0bits.ADGO = 1;                                                    // start ADC conversion
        while (ADCON0bits.ADGO);                                                // wait for conversion to finish
        for(i = 0; i < testRepetitions - 1; i++){                               //shift the previous values along
            ADCStore[i + 1] = ADCStore[i];                                      //1 >> 2 etc
        }
         ADCStore[0] = ADRES;                                                   //load the newly read value into the sample array
         currentVariance = variance(ADCStore, testRepetitions);                 //determine the variance of the samples
    }
    ADCON0bits.ADON = 0;                                                        //turn off the ADC
    FVRCONbits.FVREN = 0;                                                       //turn off the FVR
        
    return ADRES;                                                               //return the last value read
}

void readBatteryADCSetup(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    _delay_us(ADCDelay);                                                        //give the ADC time to enable
    
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = CHANNEL_BatSense;                                                   //set the channel to the EDD pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

unsigned char readMainsInterval(void){
    int i = 0;
    while(i < 1000){
        if(PORT_MainsDetect)
            return 1;
        i++;
    }
    return 0;
}

unsigned char readMainSleep(void){
    return PORT_MainsDetect;
}