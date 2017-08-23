
/*
 * File:   fire.c
 * Author: Tim Buckley
 *
 * Created on 10 May 2017, 10:28 AM
 */
#include "main.h"

extern void EDD_Init_Comms(void);
extern unsigned char programUID(unsigned char window);
extern void EDD_Calibrate(void);
extern void EDD_Energy_Store(void);
extern void EDD_Blast(void);
extern void _delay_ms(unsigned int ms);
extern void Set_Line_Low(void);
extern void Set_Line_High(void);
extern void triStateTxIsHigh(void);
extern void controlTxIsHigh(void);
extern void turnOffAcInterrupt(void);
extern void prepareForProgramming(void);
extern void returnFromProgramming(void);
extern unsigned short readS1StateADC(void);
extern void EDD_Discharge(void);
extern unsigned char checkLineForConnectedUIDs(void);
extern unsigned char programRetryChecks(unsigned char window);


void fireUIDs(void);
void checkForUnfiredEdds(void);

extern void setRedLed(void);
extern void initialiseTimer0(void);
extern void initialiseTimer1(void);
extern void programInitialise(void);
extern void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size );
extern void initialiseStates(void);

void fire(void){
    unsigned char missingPulseCheck = 0;
    setRedLed();
    FLAGS.fireComplete = 1;
    prepareForProgramming();
    FLAGS.fireSuccessFlag = 1;
    CLRWDT();
    _delay_ms(3000);
    CLRWDT();
    EDD_Init_Comms();
    CLRWDT();
    FLAGS.fireComplete = 0;
    fireUIDs();
    while(missingPulseCheck < COUNTERS.missingPulses && COUNTERS.missingPulses < 20){
        missingPulseCheck = COUNTERS.missingPulses;
        _delay_ms(1000);
        CLRWDT();
    }
    WDTCON0bits.SEN = 0;
    if(COUNTERS.missingPulses >= 20){
        EDD_Energy_Store();
        EDD_Blast();
        _delay_ms(10000);
    }
    else{
        FLAGS.fireSuccessFlag = 0;
    }
    CLRWDT();
    while(missingPulseCheck < COUNTERS.missingPulses){
        missingPulseCheck = COUNTERS.missingPulses;
        _delay_ms(1000);
        CLRWDT();
    }
    WDTCON0bits.SEN = 1;
    CLRWDT();
    checkForUnfiredEdds();
    LAT_24VCntrl = 0;
    addDataToOutgoingQueue(ABB_1.det_arrays.info, CMD_AB1_DATA, sizeof(detonator_data));
    initialiseStates();
    FLAGS.fireComplete = 1;
    COUNTERS.missingPulses = 0;
    FLAGS.fireFlag = 0;
    returnFromProgramming();
}


//checks the line for unfired EDDs by resetting any remaining EDDs and reading
//window 0. 
void checkForUnfiredEdds(void){
    CLRWDT();
    Set_Line_Low();                                                             //set the line low
    WDTCON0bits.SEN = 0;
    _delay_ms(10000);                                                           //wait for the EDDs to reset
    WDTCON0bits.SEN = 1;
    Set_Line_High();                                                            //set the line high
    _delay_ms(100);                                                             //small delay before reading
    CLRWDT();
    if(checkLineForConnectedUIDs())                                             //if there is a response from window 0
        FLAGS.fireSuccessFlag = 0;                                              //show that firing was unsuccessful
    else{                                                                       //otherwise success
        for(int i = 1; i <= ABB_1.dets_length; i++){                            //for every EDD
            if(ABB_1.det_arrays.info[i].data.connection_status){// && ABB_1.det_arrays.info[i].data.calibration_status){
                ABB_1.det_arrays.info[i].data.fired = 1;                        //mark it as fired
                ABB_1.det_arrays.info[i].data.connection_status = 0;            //and not connected
            }
            else{
                ABB_1.det_arrays.info[i].data.fired = 0;
                FLAGS.fireSuccessFlag = 0;
            }
        }
    }   
}

//Routine for programming EDDs with intent to fire
void fireUIDs(void){
    maxProgramAttempts = 2;
    if(ABB_1.dets_length > 0){                                                  //if there is something on the line and there are logged UIDs
        for(int i = 1; i <= ABB_1.dets_length; i++){                            //loop through them to program them
              programRetries = 2;
              while(!programRetryChecks(i));
              if(programRetries == 0){
                FLAGS.progSuccess = 0;                                          //clear the success Flag
                ABB_1.det_arrays.info[i].data.connection_status = 0;            //indicate that this EDD is not connected
            }
            else                                                                //if programming was successful
                ABB_1.det_arrays.info[i].data.connection_status = 1;            //indicate that the EDD is connected
            CLRWDT();
        }
    }
    if(ABB_1.dets_length > 0){                                                  //if there are supposed to be dets and nothing in window 0
        EDD_Calibrate();                                                        //calibrate them
    }    
}

//firing Routine that allows the firing PIC to control the line and send the 
//blast command once the EDDs have gone through all other states
void firePic(void){
    unsigned char missingPulseCheck = 0;                                        //reset the missing pulse comparison variable
    setRedLed();                                                                //set the red LED to indicate firing
    prepareForProgramming();
    _delay_ms(3000);                                                            //delay for 3 seconds to wait for missing pulses after blast command                                                     //prepare timers,interrupts and supplies for programming
    programInitialise();                                                        //initialise the pins, ADC and variables
    fireUIDs();                                                                 //program the UIDs for firing
    while(missingPulseCheck < COUNTERS.missingPulses && COUNTERS.missingPulses < 100){//while missing pulses are still occurring and there have been less than 75
        missingPulseCheck = COUNTERS.missingPulses;                             //assign the current number of missing pulses to the comparison variable
        _delay_ms(1000);                                                        //wait 1 second so that at least 1 missing pulse should occur in this time
    }
    
    if(COUNTERS.missingPulses >= 100){                                           //if at least 75 missing pulses have occurred there is intent to fir
        EDD_Energy_Store();                                                     //so charge the EDDs
        triStateTxIsHigh();                                                     //give control of the line to firing PIC
        _delay_ms(30000);                                                       //give the firing pic time to fire
    }
    else{
        FLAGS.fireSuccessFlag = 0;                                              //there were not enough missing pulses so firing was unsuccessful
    }   
//    if(COUNTERS.missingPulses >= 75){                                           //if at least 75 missing pulses have occurred there is intent to fire
//        EDD_Energy_Store();                                                     //so charge the EDDs
//        _delay_ms();                                                       //give the firing pic time to fire
//        triStateTxIsHigh();                                                     //give control of the line to firing PIC
//        _delay_ms(20000);                                                       //give the firing pic time to fire
//    }
//    else{
//        FLAGS.fireSuccessFlag = 0;                                              //there were not enough missing pulses so firing was unsuccessful
//    }
    controlTxIsHigh();                                                          //take control of the line
    EDD_Discharge();                                                            //discharge the EDDs for safety in case the firing signal was not sent by the other command
    checkForUnfiredEdds();                                                      //check the line for any EDDs that did not fire
    addDataToOutgoingQueue(ABB_1.det_arrays.info, CMD_AB1_DATA, sizeof(detonator_data));//add the resulting data from the routine to queue to be sent to the surface
    initialiseStates();                                                         //re-initialise the states when returning from programming
    returnFromProgramming();                                                    //prepare interrupts, timers, and supplies for normal operation
    FLAGS.fireComplete = 1;                                                     //show that firing is complete
}
