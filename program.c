/*
 * File:   program.c
 * Author: Tim Buckley
 *
 * Created on 07 April 2017, 10:11 AM
 */

#include "main.h"

void programInitialise(void);
void programUIDs(void);
void prepareForProgramming(void);
void returnFromProgramming(void);

extern void EDD_Init_Comms(void);
extern unsigned char readUID(unsigned char window);
extern unsigned char programUID(unsigned char window);
extern unsigned char selfCheckUID(unsigned short window);
extern unsigned char checkLineForConnectedUIDs(void);
extern unsigned short readS1StateADC(void);
extern void EDD_Calibrate(void);
extern void EDD_Energy_Store(void);
extern void EDD_Blast(void);
extern void _delay_ms(unsigned int delay);
extern void Set_Line_Low(void);
extern void Set_Line_High(void);
extern unsigned char programRetryChecks(unsigned char window);
extern void turnOn24V(void);

extern void initialiseState(states *specific, char number);

extern void setBlueLed(void);
extern void setOffLed(void);
extern void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size );
extern void addPacketToOutgoingQueue(char *data, unsigned char command, unsigned char length, unsigned short destination);

extern void initialiseStates(void);
extern void clearEDDStatusbits(void);
extern void sleepBluetooth(void);

extern void activateBluetooth(void);
void transmitBluetoothProgressPacket(unsigned char index, unsigned char stage);


void program(void){
    CLRWDT();
    setOffLed();
    prepareForProgramming();                                                    //prepare the device for programming
    programInitialise();                                                        //initialise the data memory
    programUIDs();                                                              //run through the Programming routine
    //uncomment to fire in program routine
//    _delay_ms(1000);
//    EDD_Energy_Store();
//    _delay_ms(1000);
//    EDD_Blast();
    addDataToOutgoingQueue(ABB_1.det_arrays.info, CMD_AB1_DATA, sizeof(detonator_data));//add the entire data memory to the outgoing queue to update changes on surface    
    initialiseStates();                                                         //re-initalise the background processes to restart the state Handler
    returnFromProgramming();                                                    //prepare for normal device operation
    //activateBluetooth();                                                        //initialise the Bluetooth
}

void programInitialise(void){
    clearEDDStatusbits();
    EDD_Init_Comms();                                                           //initialise pins and ADC for programming
    FLAGS.progSuccess = 1;                                                      //assume that programming will be successful and only
    FLAGS.programStop = 0;
}                                                                               //clear the flag if we confirm that it is not

void prepareForProgramming(void){
    //sleepBluetooth();
    sec4Interrupt = 0;                                                          //disable the sleep interrupt
    sec4Enable = 0;                                                             //disable the sleep timer                                                   //turn on the 100us Delay Timer
    msInterrupt = 0;                                                            //Disable the State Handler 
    tagInterrupt = 0;                                                           //Disable the tagInterrupt
    uartReceiveInterrupt = 0;                                                   //Disable any Bluetooth Reception
    modemReceiveInterrupt = 0;                                                  //disable any incoming message Reception form the surface
    LAT_CableTest_Enable = 0;                                                   //disable 12V on the line
    LAT_24VCntrl = 1;                                                           //enable the 24V supply
//    turnOn24V();                                                                //uncomment with new 24V supply mod
    us100Interrupt = 1;                                                         //enable the 100us interrupt
    us500Interrupt = 1;                                                         //enable the 500us interrupt
    us100DelayInterrupt = 1;                                                    //turn on the 100us Delay interrupt
    us100DelayEnable = 1;    
    _delay_ms(500);                                                             //wait for the EDDs to charge up
    Set_Line_High();                                                            //and set the line high
    _delay_ms(2000);
}

void returnFromProgramming(void){
    sec4Timer = 0;                                                              //Clear the Sleep Timer
    sec4Interrupt = 1;                                                          //Enable the sleep Interrupt
    sec4Enable = 1;                                                             //Enable the sleep Timer
    us100DelayInterrupt = 0;                                                    //Disable the 100us Delay Interrupt
    us100DelayEnable = 0;                                                       //Disable the 100us Delay Timer
    us100Interrupt = 0;                                                         //Disable the 100us Interrupt
    us500Interrupt = 0;                                                         //Disable the 500us Interrupt
    msInterrupt = 1;                                                            //enable the state Handler
    tagInterrupt = 1;                                                           //enable the Tag Interrupt
    uartReceiveInterrupt = 1;                                                   //enable Bluetooth Reception
    modemReceiveInterrupt = 1;                                                  //enable message reception From the surface
    Set_Line_Low();                                                             //Clear the Line
    LAT_CableTest_Enable = 1;                                                   //Enable cable testing
    LAT_24VCntrl = 0;                                                           //turn off the 24V supply 
    _delay_ms(1000);
    CLRWDT();
}

void programUIDs(void){
    maxProgramAttempts = 5;
    if(!checkLineForConnectedUIDs()){                                           //check to see if there is anything connected to the blast terminals
        if(ABB_1.dets_length > 0)                                               //if there isn't and there should be 
            FLAGS.progSuccess = 0;                                              //programming was unsuccessful
        FLAGS.progComplete = 1;                                                 //programming has been aborted
        return;                                                                 //there isn't anything connected so abort programming immediately
    }
    else{
        if(ABB_1.dets_length == 0)
           FLAGS.progSuccess = 0; 
    }
    CLRWDT();
    setBlueLed();                                                               //turn on the Blue LED to show that programming is underway
    if(ABB_1.dets_length > 0){                                                  //if there is something on the line and there are logged UIDs
        for(int i = 1; i <= ABB_1.dets_length; i++){                            //loop through them to program them
              programRetries = 3;
              while(!programRetryChecks(i));
              if(programRetries == 0){
                FLAGS.progSuccess = 0;                                          //clear the success Flag
                ABB_1.det_arrays.info[i].data.connection_status = 0;            //indicate that this EDD is not connected
            }
            else                                                                //if programming was successful
                ABB_1.det_arrays.info[i].data.connection_status = 1;            //indicate that the EDD is connected
            CLRWDT();
            if(FLAGS.programStop){
                FLAGS.progSuccess = 0;
                FLAGS.progComplete = 1;
                return;
            }
            transmitBluetoothProgressPacket(i, 1);
        }
    }
    _delay_ms(1000);
    if(readUID(maxDets)){                                                       //once all tagged EDDs have been attempted to be programmed
        FLAGS.progSuccess = 0;                                                  //if an EDD is still in window 0
        if(programUID(ABB_1.dets_length)){                                      //attempt to program it
            ABB_1.det_arrays.info[ABB_1.dets_length].data.connection_status = 1;//if programming was succesful indicate that this det is connected 
            addPacketToOutgoingQueue(&ABB_1.det_arrays.UIDs[ABB_1.dets_length], CMD_AB1_UID, sizeof(detonator_UID), ABB_1.destination);//add a packet to the outgoing queue to relay the new UID to the surface
        }
        else{                                                                   //if programming was unsuccessful
            ABB_1.dets_length--;                                                //remove the newly read detonator from memory as it is assumed that
        }                                                                       //it is not a valid read
    }                                                                                                                                                                                                                                     
    unsigned char attempts;
    if(ABB_1.dets_length > 0){                                                  //if at this point there are EDDs connected
        transmitBluetoothProgressPacket(0, 2);
        EDD_Calibrate(); 
        if(FLAGS.programStop){
                FLAGS.progSuccess = 0;
                FLAGS.progComplete = 1;
                return;
        }//calibrate them
        for(int i = 1; i <= ABB_1.dets_length; i++){                            //loop through all of them again
            if(ABB_1.det_arrays.info[i].data.connection_status){                //Only bother checking UIDs that have been programmed successfully  
                attempts = 0;
                while(!selfCheckUID(i) && attempts++ < 2);                      //If the EDD is not in the correct state
                if(attempts >= 2)
                    //FLAGS.progSuccess = 0;                                    //Clear the program success Flag
                    NOP();
            }
            if(FLAGS.programStop){
                FLAGS.progSuccess = 0;
                FLAGS.progComplete = 1;
                return;
            }
            transmitBluetoothProgressPacket(i, 3);
            CLRWDT();
        }
    }
    FLAGS.progComplete = 1;                                                     //indicate that Programming has been completed
}