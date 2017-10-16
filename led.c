/*
 * File:   led.c
 * Author: Tim Buckley
 *
 * Created on 17 April 2017, 6:04 PM
 */

#include "main.h"
#include "led.h"

extern void initialiseState(states *specific, char number);

void setRedLed(void);
void setGreenLed(void);
void setBlueLed(void);
void setOffLed(void);

extern unsigned char checkIfTagIsRemoved(void);

void initialiseLed(void) {
    ANSEL_LED_BLUE = 0;                                                         //make the pin digital
    
    TRIS_LED_RED = 0;                                                           //set the pin as an output
    TRIS_LED_BLUE = 0;                                                          //set the pin as an output
    TRIS_LED_GREEN = 0;                                                         //set the pin as an output
    
    setOffLed();                                                                //start with the led off
    
    initialiseState(&state.led, ledState);                                      //initialise the  led state
}

//executes the behaviour of the LED 
void led(void){
    switch (state.led.current){
        case ledOff :
            setOffLed();                                                        //turn the led off
            state.led.counter = OFF_TIME;                                       //load the counter with them time it should be off
            break;
            
        case ledOff1 :
            setOffLed();                                                        //turn the led off
            state.led.counter = OFF_TIME;                                       //load the counter with the time it should be off
            break;
            
        case offFlash :
            setOffLed();                                                        //turn the led off
            state.led.counter = FLASH_TIME;                                     //load the counter with the time it should be off
            break;
            
        case offFlash2 :
             setOffLed();                                                       //turn the led off
             state.led.counter = FLASH_TIME;                                    //load the counter with the time it should be off
             break;
            
        case flashRed:
            setRedLed();                                                        //make the LED red
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case flashGreen:
            setGreenLed();                                                      //make the led green
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case flashBlue:
            setBlueLed();                                                       //make the led blue
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case flashRed2:
            setRedLed();                                                        //make the led red
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case flashGreen2:
            setGreenLed();                                                      //make the led green
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case flashBlue2:
            setBlueLed();                                                       //make the led blue
            state.led.counter = FLASH_TIME;                                     //load the counter with the duration of the flash
            break;
            
        case solidRed:
            setRedLed();                                                        //make the led red
            state.led.counter = SOLID_ON_TIME;                                  //load the counter with the duration of the solid led
            break;
            
        case solidBlue:
            setBlueLed();                                                       //make the led blue
            state.led.counter = SOLID_ON_TIME;                                  //load the counter with the duration of the solid led
            break;
            
        case solidGreen:
            setGreenLed();                                                      //make the led green
            state.led.counter = SOLID_ON_TIME;                                  //load the counter with the duration of the solid led
            break;
            
        case tagGreen:
            setGreenLed();                                                      //make the led green
            state.led.counter = TAG_TEST_TIME;                                  //load the counter with the duration of the tag led
            break;
            
        case tagRed:
            setRedLed();                                                        //make the led red
            state.led.counter = TAG_TEST_TIME;                                  //load the counter with the duration of the solid led
            break;
    }
    
}

//determines the behaviour of the led based on the current device state
void ledStateHandler(void){
    if(state.led.current == tagRed || state.led.current == tagGreen){           //special case if an EDD has been tagged
        if(COUNTERS.sec2 < secs2){                                              //if it has been at least 1 second
            if(!FLAGS.tagConnected || FLAGS.sec2)                                //wait until the tag has been removed or 3 seconds has passed
                state.led.next = ledOff;                                        //if so then turn off the LED
            else
                state.led.next = state.led.current;                             //otherwise maintain the current status     
        }
        return;                                                                 //do not process any other conditions
    }
    switch (ABB_1.ledDeviceState){                                              //switch based on the current state of the device
        case idleDevice :                                                       //green flash 1 second intervals
            switch (state.led.current){         
                case ledOff :
                    state.led.next = flashGreen;                                //the led was off so now flash green                                
                    break;
                case flashGreen :                                               //the led was green so now turn off
                    state.led.next = ledOff;
                    break;
                default :
                   state.led.next = flashGreen;                                 //coming from another device state so start with a green flash
                
            }
            break;
            
        case readyDevice :                                                      //green flash 1 second intervals
            switch (state.led.current){
                case ledOff :
                    state.led.next = flashGreen;                                //the led was off so now flash green
                    break;
                case flashGreen :
                    state.led.next = ledOff;                                    //the led was green so now turn off
                    break;
                default :
                   state.led.next = flashGreen;                                 //coming from another device state so start with a green flash    
            }
            break;   
        case cableFaultDevice :                                                 //red flash 1 second intervals
            switch (state.led.current){
                case flashRed :                                     
                    state.led.next = ledOff;                                    //the led was red so turn off
                    break;
                case ledOff :
                    state.led.next = flashRed;                                  //the led was off so now flash red
                    break;
                default :
                   state.led.next = flashRed;                                   //coming from another state so start with red flash
            }
            break;
            
        case shaftFaultDevice :                                                 //red flash 1 second intervals
            switch (state.led.current){
                case flashRed :
                    state.led.next = ledOff;                                    //the led was red so now turn off
                    break;
                case ledOff :
                    state.led.next = flashRed;                                  //the led was off so now flash red
                    break;
                default :
                   state.led.next = flashRed;                                   //coming from another device state so start with a red flash
            }
            break;
            
        case lowBatDevice :                                                     //continuous red flash
            switch (state.led.current){
                case flashRed :
                    state.led.next = offFlash;                                  //led flashed red so flash off
                    break;
                case offFlash :
                    state.led.next = flashRed;                                  //led was off so flash red
                    break;
                default :
                   state.led.next = flashRed;                                   //coming from another device state so flash red to start
            }   
            break;
            
        case lowBat2Device :                                                    //led off
            switch (state.led.current){
                default :
                    state.led.next = ledOff;                                    //no matter what turn the led off     
            }
            
        case detErrorDevice :                                                   //red flash 1 second interval
            switch (state.led.current){
                case ledOff :
                    state.led.next = flashRed;                                  //the led was off so now flash red
                    break;
                case flashRed :
                    state.led.next = ledOff;                                    //the led was red so now turn off
                    break;
                default :
                   state.led.next = flashRed;                                   //coming from another device state so start with a red flash                                     
            }
            break;
            
        case successDevice :                                                    //blue flash 1 second intervals
            switch (state.led.current){
                case flashBlue :
                    state.led.next = offFlash;                                 //the led was off so now flash blue        
                    break;
                case offFlash :
                    state.led.next = flashBlue2;                                 //the device was in an off flash state so now flash red the second time
                    break;
                case flashBlue2 :
                    state.led.next = ledOff;                                    //the led was blue so now turn off
                    break;
                case ledOff :
                    state.led.next = flashBlue;                                 //the led was off so flash red for the first time
                    break;   
                default :
                   state.led.next = flashBlue;                                  //coming from another state so flash blue for the first time
                    
            }   
            break;
            
        case failDevice :                                                       //double flash red
            switch (state.led.current){
                case flashRed :
                    state.led.next = offFlash;                                  //the device flashed red for the first time so flash off
                    break;
                case offFlash :
                    state.led.next = flashRed2;                                 //the device was in an off flash state so now flash red the second time
                    break;
                case flashRed2 :
                    state.led.next = ledOff;                                    //the led flashed red for the second time so now turn it off
                    break;
                case ledOff :
                    state.led.next = flashRed;                                  //the led was off so flash red for the first time
                    break;
                default :
                   state.led.next = flashRed;                                   //coming from another device state so flash red for the first time
                    
            }   
            break;
            
        case noCommsDevice :                                                    //led is off permanently
            switch (state.led.current){
                default :
                   state.led.next = ledOff;                                     //no matter what turn the led off
            }
            break;
            
            
            
            
    }
}

//makes the LED red
void setRedLed(void){
    LAT_LED_RED = 1;                                                            //set the Red led
    LAT_LED_BLUE = 0;                                                           //clear the Blue led
    LAT_LED_GREEN = 0;                                                          //clear the Green led
}

//makes the LED green
void setGreenLed(void){
    LAT_LED_RED = 0;                                                            //clear the Red led
    LAT_LED_BLUE = 0;                                                           //clear the Blue led
    LAT_LED_GREEN = 1;                                                          //set the Green led
}

//makes the led Blue
void setBlueLed(void){
    LAT_LED_RED = 0;                                                            //clear the Red led
    LAT_LED_BLUE = 1;                                                           //set the Blue led
    LAT_LED_GREEN = 0;                                                          //clear the green led
}

//turns the led off
void setOffLed(void){
    LAT_LED_RED = 0;                                                            //clear the Red led
    LAT_LED_BLUE = 0;                                                           //clear the Blue led
    LAT_LED_GREEN = 0;                                                          //clear the Green led
}

void determineLedStatusBits(void){
    unsigned short *statusBitsPtr;
    statusBitsPtr = &ABB_1.info.statusBits;
    *statusBitsPtr &= 0x0FFF;
    if(FLAGS.fireComplete){
        if(FLAGS.fireSuccessFlag)
            *statusBitsPtr |= blueSingleFlash << 12;
        else
            *statusBitsPtr |= redDoubleFlash << 12;
        return;
    }
    if(ABB_1.info.statusBits.cable_fault || ABB_1.info.statusBits.detError){
        *statusBitsPtr |= redSingleFlash << 12;
        return;
    }
    if(ABB_1.info.statusBits.key_switch_status && !(ABB_1.info.statusBits.cable_fault || ABB_1.info.statusBits.ready)){
        *statusBitsPtr |= blueSolid << 12;
        return;
    }
    *statusBitsPtr |= greenSingleFlash << 12;
    return;
    
                
        
}