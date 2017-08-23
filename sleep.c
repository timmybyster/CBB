/*
 * File:   sleep.c
 * Author: Tim Buckley
 *
 * Created on 15 May 2017, 1:15 PM
 */

#include "main.h"

extern void setOffLed(void);
extern void sleepBluetooth(void);
extern void modemSleep(void);
extern void tagSleep(void);
extern void ledSleep(void);
extern void shaftSleep(void);
extern void Set_Line_Low(void);
extern void turnOffCableTest(void);
void sleep12V(void);
extern void initialiseStates(void);

extern void initialiseModem(void);
extern void _delay_ms(unsigned int delay);
extern void reset4SecTimer(void);
extern void tagForInterrupt(void);
extern void writeStructureToEeprom(unsigned char *structure, unsigned int size);
extern void testMemory(void);

//puts the unit into a low power state when there is no 36V
void sleep(void){
    setOffLed();                                                               //turn the led OFF
    sleepBluetooth();                                                           //turn the Bluetooth OFF
    sleep12V();                                                                 //turn the 12V supply off
    _delay_ms(1000);
    Set_Line_Low();                                                             //ensure that the 24 line is low
    turnOffCableTest();                                                         //turn off the cable test
    tagSleep();
    us100DelayEnable = 0;                                                       //turn off the 100us Delay Timer
    us500Enable = 0;                                                            //turn off the 500us Timer
    msEnable = 0;                                                               //turn off the ms Timer
    us100Enable = 0;                                                            //turn off the 100us Timer
    sec4Interrupt = 0;
    modemReceiveInterrupt = 0;
    writeStructureToEeprom(ABB_1.det_arrays.UIDs,sizeof(detonator_UID)*(ABB_1.dets_length + 1));
    writeStructureToEeprom(ABB_1.det_arrays.info,sizeof(detonator_data)*(ABB_1.dets_length + 1));
    writeStructureToEeprom(&ABB_1.dets_length,1);
    writeStructureToEeprom(&ABB_1.destination,2);
    modemSleep();
    ADCON0bits.ADON = 0;                                                        //turn off the ADC
    FVRCONbits.FVREN = 0;                                                       //turn off the FVR
    acOr36VInterruptEdge = 1;                                                   //Enable the interrupt on a positive edge now in anticipation of the return of 36V
    CPUDOZEbits.IDLEN = 0;                                                      //ensure the device goes to sleep
    ABB_1.deviceState = sleepDevice;                                            //set the device state to sleep
    WDTCON0bits.SEN = 0;
    SLEEP();                                                                    //SLEEP instruction
}

//Re-intialises the pic after it has been woken up from sleep
void wake(void){
   RESET();
   INTCONbits.GIE = 0;                                                          //change the interrupt edge to falling
   ABB_1.deviceState = idleDevice;                                              //set the device state to idle
   acOr36VInterruptEdge = 0;  
   reset4SecTimer();                                                            //reset the 4 second timer
   LAT_12VCntrl = 1;                                                            //Enable the 12V Control
   _delay_ms(100);                                                              //give the 12V time to power up
   initialiseModem();                                                           //re-initialise the modem
   tagForInterrupt();
   initialiseStates();                                                          //reset all the background processes contexts
   INTCONbits.GIE = 1;
   msEnable = 1;                                                                //enable the ms timer                                                          //
   INTCONbits.PEIE = 1;                                                         //enable all peripheral interrupts
   ABB_1.info.statusBits.voltage = 1;                                           //set the 36V bit in memory
   FLAGS.shaftCheck = 0;                                                        //ensures that a shaft Check is not preformed
   FLAGS.shaftComplete = 0;                                                     //ensures that if a shaft check is performed later it will execute
   WDTCON0bits.SEN = 1;
}

//turns off the 12V supply
void sleep12V(void){
    LAT_12VCntrl = 0;                                                           //clear the 12V enable pin
}

void mainsSleep(void){
    setOffLed();                                                               //turn the led OFF
    sleepBluetooth();                                                           //turn the Bluetooth OFF
    sleep12V();                                                                 //turn the 12V supply off
    _delay_ms(1000);
    Set_Line_Low();                                                             //ensure that the 24 line is low
    turnOffCableTest();                                                         //turn off the cable test
    tagSleep();
    us100DelayEnable = 0;                                                       //turn off the 100us Delay Timer
    us500Enable = 0;                                                            //turn off the 500us Timer
    us100Enable = 0;                                                            //turn off the 100us Timer
    sec4Interrupt = 0;
    modemReceiveInterrupt = 0;
    modemSleep();
    ADCON0bits.ADON = 0;                                                        //turn off the ADC
    FVRCONbits.FVREN = 0;                                                       //turn off the FVR
    acOr36VInterruptEdge = 1;                                                   //Enable the interrupt on a positive edge now in anticipation of the return of 36V
    CPUDOZEbits.IDLEN = 0;                                                      //ensure the device goes to sleep
    ABB_1.deviceState = sleepDevice;                                            //set the device state to sleep
    WDTCON0bits.SEN = 0;
}

void mainsWake(void){
   INTCONbits.GIE = 0;                                                          //change the interrupt edge to falling
   ABB_1.deviceState = idleDevice;                                              //set the device state to idle
   reset4SecTimer();                                                            //reset the 4 second timer
   LAT_12VCntrl = 1;                                                            //Enable the 12V Control
   _delay_ms(100);                                                              //give the 12V time to power up
   initialiseModem();                                                           //re-initialise the modem
   initialiseStates();                                                          //reset all the background processes contexts
   tagForInterrupt();
   INTCONbits.GIE = 1;
   INTCONbits.PEIE = 1;                                                         //enable all peripheral interrupts
   ABB_1.info.statusBits.voltage = 1;                                           //set the 36V bit in memory
   FLAGS.shaftCheck = 0;                                                        //ensures that a shaft Check is not preformed
   FLAGS.shaftComplete = 0;                                                     //ensures that if a shaft check is performed later it will execute
   WDTCON0bits.SEN = 1;
   ABB_1.info.statusBits.key_switch_status = 0;
}