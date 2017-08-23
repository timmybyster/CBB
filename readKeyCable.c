/*
 * File:   readKeyCable.c
 * Author: Tim Buckley
 *
 * Created on 17 April 2017, 6:04 PM
 */

#include "main.h"
#include "readKeyCable.h"

unsigned short readS1StateADC(void);
void readS1StateADCSetup(void);
unsigned short readEDDADC(void);
void readEDDADCSetup(void);

extern void initialiseState(states *specific, char number);
extern void _delay_us(unsigned char us);
extern unsigned short variance(unsigned short *values, unsigned char sampleSize);

//initialise the read key cable pins and the state process
void initialiseReadKeyCable(void){ 
    ANSEL_S1_State = 1;                                                         //set the pin to analog
    TRIS_S1_State = 1;                                                          //set the pin to digital
    
    ANSEL_EDDRead = 1;                                                          //set the pin to analog
    TRIS_EDDRead = 1;                                                           //set the pin to digital
    
    ANSEL_CableTest_Enable = 0;                                                 //set the pin to digital
    TRIS_CableTest_Enable = 0;                                                  //set the pin to analog
    
    initialiseState(&state.readKeyCable, readKeyCableState);                    //initialise the state
}

//executes the code from the read key cable process base on the process state
void readKeyCable(void){
    switch (state.readKeyCable.current){
        case cableEnable :
            state.readKeyCable.counter = cableEnableInterval;                   //load the process counter
            LAT_CableTest_Enable = 1;                                           //enable the cable test relay
            testCounter = 0;                                                    //reset the test counter
            break;
            
        case cableFaultFound :
            state.readKeyCable.counter = cableKeyInterval;                      //load the process counter
            ABB_1.info.statusBits.cable_fault = 1;                              //set the cable fault bit in memory
            testCounter = 0;                                                    //clear the test counter
            break;
            
        case noFaultFound :
            state.readKeyCable.counter = cableKeyInterval;                      //load the process counter
            ABB_1.info.statusBits.cable_fault = 0;                              //clear the cable fault bit in memory
            testCounter = 0;                                                    //clear the test counter
            break;
              
        case keyOn :
            state.readKeyCable.counter = keyCableInterval;                      //load the process counter
            ABB_1.info.statusBits.key_switch_status = 1;                        //set the key switch bit in memory
            LAT_CableTest_Enable = 0;                                           //disable the cable test
            break;
            
        case keyOff :
            state.readKeyCable.counter = keyCableInterval;                      //load the process counter
            ABB_1.info.statusBits.key_switch_status = 0;                        //clear the key switch bit in memory
            LAT_CableTest_Enable = 0;                                           //disable the cable test
            break;
            
        default :
            state.readKeyCable.counter = 1;                                     //test immediately afterwards
            testCounter++;                                                      //increment the test counter
            
    }
}
//Inputs are S1State and the testCounter
void readKeyCableStateHandler(void){
    switch(state.readKeyCable.current){
        case cableEnable :
            state.readKeyCable.next = cableRead;                                //the cable test has been enabled so read the cable next
            break;
            
        case cableRead :
            if(readEDDADC() > cableFaultReadValue){                             //if there is a cable fault value read
                if(testCounter > testRepetitions)                               //if the 5 tests have been performed
                    state.readKeyCable.next = cableFaultFound;                  //go to the cable fault state
                else
                    state.readKeyCable.next = cableRead;                        //otherwise read the cable again
            }
            else
                state.readKeyCable.next = noFaultFound;                         //as long as any test is negative show that there is no fault
            break;
            
        case cableFaultFound :
            state.readKeyCable.next = keyRead;                                  //read the key
            break;
            
        case noFaultFound :
            state.readKeyCable.next = keyRead;                                  //read the key
            break;
            
        case keyRead :
            if(readS1StateADC() > keyReadValue){                                //if the key is on
                if(testCounter > testRepetitions){                              //if 5 successful reads have been performed
                    state.readKeyCable.next = keyOn;                            //go to the key on state
                }
                else
                   state.readKeyCable.next = keyRead;                           //otherwise read it again
            }
            else
                state.readKeyCable.next = keyOff;                               //if it is off at any point then go to the key off state
            break;
            
        default :
            state.readKeyCable.next = cableEnable;                              //after all other states restart the process
    }
            
            
}

//performs an ADC read for the key switch
unsigned short readS1StateADC(void){ 
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    readS1StateADCSetup();                                                      //initialise the ADC                          
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

unsigned short readEDDADC(void){
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    readEDDADCSetup();                                                          //initialise the ADC  
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

void readEDDADCSetup(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    _delay_us(ADCDelay);                                                        //give the ADC time to enable
    
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = CHANNEL_EDDRead;                                                    //set the channel to the EDD pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

void readS1StateADCSetup(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    _delay_us(ADCDelay);                                                        //give the ADC time to enable
    
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = CHANNEL_S1_State;                                                   //set the channel to the S1 state pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

void turnOffCableTest(void){
    LAT_CableTest_Enable = 0;
}

/*CABLE FAULT READINGS
 *  Open Circuit    0x0255      597
 *  1K              0x02C8      712
 *  1 Det           0x0257      599
 *  1 Det 1K        0x02C9      713
 *  100 Dets        0x02B4      692
 *  1K 100 Dets     0x02D5      725 
 * 
 * VALUE of cable Fault can be anything above 700   
 */