/*
 * File:   tag.c
 * Author: Tim Buckley
 *
 * Created on 09 October 2017, 3:21 PM
 */


#include "tag.h"
#include "tagRead.h"
#include "main.h"

unsigned short readTagDetectADC(unsigned char direction);
void tagDetectADCSetup(unsigned char direction);
void triggerReadTagInterrupt(void);

extern void _delay_us(unsigned char us);
extern unsigned short variance(unsigned short *values, unsigned char sampleSize);

extern void readTagRoutine(void);

void tag(void) {
    switch(state.tag.current){                                                  //execute based on the current process state
        case scanTagPins :                                                      //set up the tag pins to be scanned in the forward direction
            TRIS_TAG_UIO_1 = 0;                                                 //set the pin as an output
            LAT_TAG_UIO_1 = 0;                                                  //set it low
            WPU_TAG_UIO = 1;                                                    //enable the other pin with a weak pull up
            TRIS_TAG_UIO = 1;                                                   //set as an input
            ANSEL_TAG_UIO = 1;                                                  //set it as analog
            state.tag.counter = 1;                                              //move to the next state immediately
            break;
            
        case scanTagPinsReverse :                                               //set up the pins to scan in the reverse direction
            TRIS_TAG_UIO = 0;                                                   //set the pin as an output
            LAT_TAG_UIO = 0;                                                    //set it low
            WPU_TAG_UIO_1 = 1;                                                  //enable the other pin as a weak pull up
            TRIS_TAG_UIO_1 = 1;                                                 //set it as an input
            ANSEL_TAG_UIO_1 = 1;                                                //set it as analog
            state.tag.counter = 1;                                              //move to the next state immediately
            break;
            
        case tagDetected :                                                      //tag was detected in the forward direction
            FLAGS.tagConnected = 1;                                             //set a flag
            triggerReadTagInterrupt();                                          //trigger the read routine as an interrupt
            state.tag.counter = 1;                                              //move to the next state immediately
            break;
            
        case tagDetectedReverse :                                               //tag was detected in the reverse direction
            FLAGS.tagConnected = 1;                                             //set a flag
            triggerReadTagInterrupt();                                          //trigger the read routine as an interrupt
            state.tag.counter = 1;                                              //move to the next state immediately
            break;
            
        case tagClear :                                                         //prepare the pins to determine whether the tag has been removed from a forward direction 
            tagInterruptFlag = 0;                                               //ensure that the interrupt flag is cleared
            TRIS_TAG_UIO_1 = 0;                                                 //set the pin as an output
            LAT_TAG_UIO_1 = 0;                                                  //set it as low
            WPU_TAG_UIO = 1;                                                    //enable the weak pull up on the other pin
            TRIS_TAG_UIO = 1;                                                   //set it as an input
            ANSEL_TAG_UIO = 1;                                                  //set it as analog
            state.tag.counter = tagCheckInterval;                               //wait to move to the next state
            break;
            
        case tagClearReverse :                                                  //prepare the pins to determine whether or not the tag has been removed in a reverse direction
            tagInterruptFlag = 0;                                               //ensure that the interrupt flag is cleared
            TRIS_TAG_UIO = 0;                                                   //set the pin as an output
            LAT_TAG_UIO = 0;                                                    //set it as low
            WPU_TAG_UIO_1 = 1;                                                  //enable the weak pull up on the other pin
            TRIS_TAG_UIO_1 = 1;                                                 //set it as an input
            ANSEL_TAG_UIO_1 = 1;                                                //set it as analog
            state.tag.counter = tagCheckInterval;                               //wait to move to the next state
            break;
            
        case tagWait :                                                          //wait period between tag reads
            FLAGS.tagConnected = 0;                                             //show that there is no tag connected
            state.tag.counter = tagCheckInterval;                               //wait period between scans
            break;
            
        default :
            state.tag.counter = tagCheckInterval;                               //standard wait period in case context is lost
            break; 
    }
    
}

void tagStateHandler(void){
    unsigned short tagADC = 0;                                                  //variable to store adc reads
    switch(state.tag.current){                                                  //determine the next state based on the current one
        case scanTagPins :                                                      
            tagADC = readTagDetectADC(0);                                       //read the adc in the forward direction
            if(tagADC < tagReadValue)                                           //if it is low
                state.tag.next = tagDetected;                                   //a tag has been connected
            else                                                                //otherwise if it is still high
                state.tag.next = scanTagPinsReverse;                            //check the other direction
            break;
            
        case scanTagPinsReverse :
            tagADC = readTagDetectADC(1);                                       //read the adc in the reverse direction
            if(tagADC < tagReadValue)                                           //if it is low
                state.tag.next = tagDetectedReverse;                            //a tag has been connected in the reverse direction
            else                                                                //otherwise
                state.tag.next = tagWait;                                       //wait to scan again
            break;
            
        case tagDetected :                                                      //a tag has been detected
            state.tag.next = tagClear;                                          //wait for it to be removed
            break;
            
        case tagDetectedReverse :                                               //a tag has been detected in the reverse direction
            state.tag.next = tagClearReverse;                                   //wait for it to be removed
            break;
            
        case tagClear :
            tagADC = readTagDetectADC(0);                                       //read tag in the forward direction
            if(tagADC > tagClearValue)                                          //if it is high
                state.tag.next = tagWait;                                       //it has been removed
            else                                                                //otherwise if it is still low
                state.tag.next = tagClear;                                      //the tag is still connected and another check can be done 
            break;
            
        case tagClearReverse :
            tagADC = readTagDetectADC(1);                                       //read the tag in the forward direction
            if(tagADC > tagClearValue)                                          //if it is high
                state.tag.next = tagWait;                                       //it has been removed
            else                                                                //otherwise if it is still low
                state.tag.next = tagClearReverse;                               //the tag is still connected and another check can be done 
            break;
            
        case tagWait :                                  
            state.tag.next = scanTagPins;                                       //go to scan the pins again
            break;
            
        default :
            state.tag.next = tagWait;                                           //default state in case context is lost
    }
}

void tagDetectADCSetup(unsigned char direction){
    ADCON1 = 0b01110000;                                                        //standard ADC
    ADPCH = tagADCChannel - direction;                                          //set the channel to the tag pin
    ADCLK = 0b00111111;                                                         //standard conversion time
    ADREF = 0b00000000;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        //Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
}

unsigned short readTagDetectADC(unsigned char direction){ 
    unsigned short ADCStore[testRepetitions];                                   //initialise the sample array
    unsigned short currentVariance = 1000;                                      //initialise the variance as very high
    int i;                                                                      //create a counter
    tagDetectADCSetup(direction);                                               //initialise the ADC                          
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

void triggerReadTagInterrupt(void){
    tagInterruptFlag = 0;                                                       //clear the interrupt flag
    tagInterrupt = 1;                                                           //enable the interrupt
    ANSEL_TAG_UIO_1 = 0;                                                        //set the pin as digital
    TRIS_TAG_UIO_1 = 0;                                                         //set the pin as an output
    LAT_TAG_UIO_1 = 1;                                                          //force it high
    _delay_us(20);                                                              //give the interrupt time to take effect
    LAT_TAG_UIO_1 = 0;                                                          //force it low to trigger the falling edge interrupt
}
