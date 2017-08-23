/*
 * File:   shaftTest.c
 * Author: Tim Buckley
 *
 * Created on 11 May 2017, 8:15 AM
 */

#include "main.h"
#include "shaftTest.h"

unsigned short readCableADC(void);
void readCableADCSetup(void);
unsigned short readEarthADC(void);
void reaEarthADCSetup(void);

extern void initialiseState(states *specific, char number);
extern void _delay_us(unsigned char us);
extern unsigned short variance(unsigned short *values, unsigned char sampleSize);


//initialises the pins and states for the shaft test process
void initialiseShaftTest(void) { 
    ANSEL_Relay_Drive = 0;                                                      //set the pin as digital
    TRIS_Relay_Drive = 0;                                                       //set the pin as an output
    LAT_Relay_Drive = 0;                                                        //set the pin low
    
    ANSEL_EL_Enable = 0;                                                        //set the pin as digital
    TRIS_EL_Enable = 0;                                                         //set the pin as an output
    LAT_EL_Enable = 0;                                                          //set the pin low
    
    TRIS_CableTestEnable = 0;                                                   //set the pin as an output
    LAT_CableTestEnable = 0;                                                    //set the pin low
    
    ANSEL_CableTestRead = 1;                                                    //set the pin as analog
    TRIS_CableTestRead = 1;                                                     //set the pin as an input
    
    ANSEL_EarthLeakage = 1;                                                     //set the pin as analog
    TRIS_EarthLeakage = 1;                                                      //set the pin as an input
    
    initialiseState(&state.shaftTest, shaftTestState);                          //initialise the state
}

//executes code from the shaft test process based on the current state
void shaftTest(void){
    switch (state.shaftTest.current){
        case relayDrive :
            LAT_Relay_Drive = 1;                                                //turn on the relay
            state.shaftTest.counter = relayInterval;                            //load the counter to move to the next state
            break;
          
        case shaftTestWait :
            state.shaftTest.counter = shaftTestInterval;                        //load the counter to move to the next state
            break;
            
        case cableEnable :
            LAT_CableTestEnable = 1;                                            //enable the cable test
            state.shaftTest.counter = relayInterval;                            //load the counter to move to the next state
            break;
               
        case faultDetected :
            LAT_Relay_Drive = 1;                                                //if a fault is detected ensure that relay isolates it
            LAT_CableTestEnable = 0;
            state.shaftTest.counter = normalWaitInterval;                       //load the counter to move to the next state
            ABB_1.info.statusBits.shaftFault = 1;                               //set the shaft fault bit in memory
            ABB_1.info.statusBits.earth_leakage = 0;                            //cleat the earth fault bit in memory
            FLAGS.shaftComplete = 1;                                            //show that the test has concluded
            break;
            
        case earthEnable :
            state.shaftTest.counter = relayInterval;                            //load the counter to move to the next state
            LAT_CableTestEnable = 0;                                            //disable the cable test
            LAT_EL_Enable = 1;                                                  //enable the cable test
            break;
            
        case earthFault :
            LAT_EL_Enable = 0;
            state.shaftTest.counter = normalWaitInterval;                       //load the counter to move to the next state
            ABB_1.info.statusBits.shaftFault = 0;                               //clear the shaft fault bit in memory
            ABB_1.info.statusBits.earth_leakage = 1;                            //set the earth fault bit in memory
            FLAGS.shaftComplete = 1;                                            //show that the test has concluded
            break;
            
        case noFault :
            state.shaftTest.counter = normalWaitInterval;                       //load the counter to move to the next state
            ABB_1.info.statusBits.shaftFault = 0;                               //clear the shaft fault bit in memory
            ABB_1.info.statusBits.earth_leakage = 0;                            //clear the earth fault bit in memory
            LAT_EL_Enable = 0;                                                  //disable the EL enable 
            LAT_Relay_Drive = 0;                                                //disable the cable test
            LAT_CableTestEnable = 0;                                            //close the relay
            FLAGS.shaftComplete = 1;                                            //indicate the test has been completed
            FLAGS.shaftCheck = 0;                                               //clear the flag so the test is not performed again
            break;
            
        default :
            state.shaftTest.counter = 1;                                        //default should be to move to next state immediately
            
    }
}
//Inputs are CableTestRead and Earth_Leakage
void shaftTestStateHandler(void){
    switch(state.shaftTest.current){
        case relayDrive :
            state.shaftTest.next = shaftTestCheck;                              //move straight to checking the shaft
           break;
              
        case shaftTestCheck :
            if(!ABB_1.info.statusBits.voltage)              //if there is no 36V or a shaft check should be performed
                state.shaftTest.next = shaftTestWait;                           //wait for the test to take affect on the entire shaft
            else
                state.shaftTest.next = cableEnable;                             //otherwise begin the test
            break;
            
        case shaftTestWait :
            state.shaftTest.next = cableEnable;                                 //once waiting is complete perform the read on the cable
            break;
            
        case cableEnable :
            state.shaftTest.next = cableRead;                                   //once waiting is complete perform the read on the cable
            break;
              
        case cableRead :
            if(readCableADC() > shaftFaultRead)                                 //if there is a fault
                state.shaftTest.next = faultDetected;                           //indicate it
            else
                state.shaftTest.next = earthEnable;                             //otherwise perform an earth test
            break;
            
        case earthEnable :
            state.shaftTest.next = earthLeakage;                                //after the earth test is ready
            break;
            
        case earthLeakage :
            if(readEarthADC() > earthFaultRead)                                 //if there is an earth fault
                state.shaftTest.next = earthFault;                              //indicate it
            else
               state.shaftTest.next = noFault;                                  //otherwise show that there is no fault
            break;
         
        default :
            state.shaftTest.next = relayDrive;                                  //after all other states restart the process
            break;
    }
            
            
}

unsigned short readCableADC(void){
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    readCableADCSetup();                                                        //initialise the ADC  
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

void readEarthADCSetup(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    _delay_us(ADCDelay);                                                        //give the ADC time to enable
    
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = CHANNEL_EarthLeakage;                                               //set the channel to the EDD pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

unsigned short readEarthADC(void){
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    readEarthADCSetup();                                                        //initialise the ADC  
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

void readCableADCSetup(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    _delay_us(ADCDelay);                                                        //give the ADC time to enable
    
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = CHANNEL_CableTestRead;                                              //set the channel to the EDD pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

void disengageRelay(void){
    LAT_Relay_Drive = 0; 
}

void shaftSleep(void){
    TRIS_Relay_Drive = 0;
    LAT_EL_Enable = 0;
    TRIS_CableTestEnable = 1;
}