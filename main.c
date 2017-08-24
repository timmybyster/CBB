/*
 * File:   main.c
 * Author: Tim Buckley
 *
 * Created on 04 April 2017, 11:11 AM
 */

#include "main.h"
#include "led.h"

void device(void);
void deviceStateHandler(void);
void checkStatusBits(void);

extern void currentStateHandler(void);

extern void us100DelayIsr(void);
extern void us500Isr(void);
extern void msIsr(void);
extern void us100Isr(void);
extern void sec4OverflowIsr(void);

extern void SPIISRHandlerST7540(void);
extern void RXReadyISRHandlerST7540(void);

extern void readTagRoutine(void);

extern void acOr36VISR(void);

extern void initialise(void);

extern void program(void);
extern void fire(void);
extern void firePic(void);

extern void sleep(void);
extern void mainsSleep(void);
extern void mainsWake(void);

extern void readEUSART(void);

extern void addPacketToOutgoingQueue(char *data, unsigned char command, unsigned char length, unsigned short destination);
extern void writeStructureToEeprom(unsigned char *structure, unsigned int size);

extern void activateBluetooth(void);
extern void sleepBluetooth(void);
extern void testMemory(void);

extern void initialiseTimer1(void);
void turnOffAcInterrupt(void);

extern void setOffLed(void);
extern void disengageRelay(void);

void interrupt isr(void){
    if(ABB_1.deviceState== fireDevice && !FLAGS.fireComplete){     
        if(us100DelayInterruptFlag && us100DelayInterrupt)                          //100us interrupt
            us100DelayIsr();
        if(us100InterruptFlag && us100Interrupt)                                    //100us interrupt for EDD reads
            us100Isr();
        if(us500InterruptFlag && us500Interrupt)                                    //500us interrupt
            us500Isr();
        if(acOr36VInterruptFlag && acOr36VInterrupt){                               //a falling edge has been detected on the ACor36V pin
            acOr36VISR();                                                           //process depending on circumstance
            acOr36VInterruptFlag = 0;
        }
        return;
    }
    if(us100InterruptFlag && us100Interrupt)                                    //100us interrupt for EDD reads
        us100Isr();
    if(us100DelayInterruptFlag && us100DelayInterrupt)                          //100us interrupt
        us100DelayIsr();
    if(us500InterruptFlag && us500Interrupt)                                    //500us interrupt
        us500Isr();
    if(msInterruptFlag && msInterrupt)                                          //1ms interrupt for the State handler
        msIsr();
    if(tagInterruptFlag && tagInterrupt){                                       //A tag has been placed on the tagReader
        readTagRoutine();                                                       //Call the Read Tag Routine
        tagInterruptFlag = 0;
    }
    if(acOr36VInterruptFlag && acOr36VInterrupt){                               //a falling edge has been detected on the ACor36V pin
        acOr36VISR();                                                           //process depending on circumstance
        acOr36VInterruptFlag = 0;
    }
    if(sspiInterruptFlag && sspiInterrupt)                                      //MSSP Modem Interrupt
        SPIISRHandlerST7540();                                                  //Either data to be received or transmitted
    if (modemReceiveInterruptFlag && modemReceiveInterrupt){
        RXReadyISRHandlerST7540();                                              //Falling edge on SS pin indicating data to be received
        modemReceiveInterruptFlag = 0;                                          //from Modem 
        INTCONbits.INT0EDG = !INTCONbits.INT0EDG;                               //Make sure that the edge rises again
    }
    if(uartReceiveInterruptFlag && uartReceiveInterrupt)                        //Bluetooth data to be received
        readEUSART();
    if(sec4InterruptFlag && sec4Interrupt)                                      //4 second timer overflow interrupt       
        sec4OverflowIsr();                                                      //show that there has been a loss of 36V
}

void main(void) {
    CLRWDT();
    ABB_1.deviceState = idleDevice;
    outgoingQueue.queue_pointer = 0;
    outgoingQueue.length = 0;
    COUNTERS.shaftTests = 0;
    ABB_1.info.statusBits.voltage = 1;
    ABB_1.info.statusBits.communication_status = 1;
    FLAGS.communication_status = 1;
    FLAGS.bluetoothTimer = 0;
    COUNTERS.min10 = mins10;
    COUNTERS.missingPulses = 0;
    ABB_1.destination = 0xFFFF;    
    initialise();                                                              //initialise the device
    testMemory();
    while(1){                                                                   //loop forever
        device();                                                               //execute based on the current State
        deviceStateHandler();                                                   //determine the next device State
    }
    return;      
  }
//The device function executes the overall behaviour of the device based on the 
//state it is in
void device(void){
    switch(ABB_1.deviceState){
        case sleepDevice :
            sleep();
            break;
            
        case shaftDevice :
            checkStatusBits();                                                   //check to see if anything has changed  
            currentStateHandler();                                               //execute any background processes
            FLAGS.shaftCheck = 1;
            break;
            
        case voltageDevice :
            checkStatusBits();                                                   //check to see if anything has changed  
            currentStateHandler();                                               //execute any background processes
            disengageRelay();
            break;

        case idleDevice :
           checkStatusBits();                                                   //check to see if anything has changed  
           currentStateHandler();                                               //execute any background processes
           FLAGS.shaftComplete = 0;
           FLAGS.min10 = 1;                                                     //set the 10 minute Flag so programming will commence as soon as the Key is armed
           FLAGS.sec3 = 0;                                                      //Clear the 2 second Flag
           COUNTERS.sec3 = secs3;                                               //reload the 2 second counter
           FLAGS.sec1 = 0;                                                      //Clear the 2 second Flag
           COUNTERS.sec1 = secs1;                                               //reload the 2 second counter
           ABB_1.ledDeviceState = ABB_1.deviceState;                            //Set the LED behaviour to indicate a shaft Fault
           ABB_1.info.statusBits.ready = 0;
           ABB_1.info.statusBits.detError = 0;
           break;
           
        case cableFaultDevice :
           checkStatusBits();                                                   //check to see if anything has changed
           currentStateHandler();                                               //execute any background processes
           ABB_1.ledDeviceState = ABB_1.deviceState;                            //Set the LED behaviour to indicate a shaft Fault
           break;
        
        case lowBatDevice :
           checkStatusBits();                                                   //check to see if anything has changed
           currentStateHandler();                                               //execute any background processes
           ABB_1.ledDeviceState = ABB_1.deviceState;                            //Set the LED behaviour to indicate a shaft Fault
           break;
           
        case lowBat2Device :
            mainsSleep();
            break;
            
        case offDevice :
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            break;
            
        case turnOnDevice :
            mainsWake();
            break;
            
        case readyDevice:
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            ABB_1.ledDeviceState = ABB_1.deviceState;                           //Set the LED behaviour to indicate the device is ready to fire
            ABB_1.info.statusBits.ready = 1;
            FLAGS.sec3 = 0;
            COUNTERS.sec3 = secs3;
            FLAGS.sec1 = 0;                                                     //Clear the 2 second Flag
            COUNTERS.sec1 = secs1;                                              //reload the 2 second counter
            FLAGS.min10 = 0;
            COUNTERS.min10 = 0;
            break;
            
        case detErrorDevice :
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            ABB_1.ledDeviceState = ABB_1.deviceState;                           //Set the LED behaviour to indicate the device is ready to fire
            ABB_1.info.statusBits.detError = 1;
            FLAGS.sec3 = 0;
            COUNTERS.sec3 = 0;
            FLAGS.sec1 = 0;                                                     //Clear the 2 second Flag
            COUNTERS.sec1 = secs1;                                              //reload the 2 second counter
            FLAGS.min10 = 0;
            COUNTERS.min10 = 0;
            break;
            
        case bluetoothDevice :
            activateBluetooth();                                                //initialise the Bluetooth
            COUNTERS.min10 = mins10;                                            //reload the 10 minute timeout counter
            FLAGS.min10 = 0;                                                    //clear the 10 minute timeout Flag
            break;
            
        case bluetoothOffDevice :
            break;
            
        case programDevice :
            FLAGS.fireFlag = 0;                                                 //clear the fire Flag
            FLAGS.progComplete = 0;                                             //clear the program Complete Flag
            COUNTERS.min10 = mins10;                                            //initialise the 10 minute timeout so that
            FLAGS.min10 = 0;                                                    //EDDs will be reprogrammed every 10 minutes
            program();                                                          //call the Program Routine
            break;

        case fireDevice :                                                       //call the Fire Routine
            fire();                                                             //Clear the Fire Flag
            FLAGS.fireFlag = 0;
            break;
            
        case fireCheckDevice :
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            break;
            
        case successDevice :
            ABB_1.ledDeviceState = ABB_1.deviceState;                           //Set the LED behaviour to indicate the device fired successfully 
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            break;
            
        case failDevice :
            ABB_1.ledDeviceState = ABB_1.deviceState;                           //Set the LED behaviour to indicate the device did not fire successfully 
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            break;
            
        case fireReturnDevice :
            ABB_1.dets_length = 0;
            writeStructureToEeprom(&ABB_1.dets_length,1);
            RESET();
            break;
            
        default :
            checkStatusBits();                                                  //check to see if anything has changed
            currentStateHandler();                                              //execute any background processes
            break;
    }
}
//Determines what the device Behaviour state should be based on the current 
//parameters and status of the device
void deviceStateHandler(void){
    switch(ABB_1.deviceState){
        case sleepDevice :
            ABB_1.deviceState = idleDevice;
            break;
        
        case shaftDevice :
            ABB_1.deviceState = keyIdleDevice;
            break;
            
        case voltageDevice :
            if(ABB_1.info.statusBits.voltage || PORT_ACor36V)
                ABB_1.deviceState = keyIdleDevice;
            else
                ABB_1.deviceState = sleepDevice;
            break;
            
        case idleDevice :
            ABB_1.deviceState = keyIdleDevice;                                  //Go to the key Idle check state
            break;
            
        case cableFaultDevice:                                                  //cable fault has been detected 
            ABB_1.deviceState = keyIdleDevice;                                  //check to see whether it has been cleared
            break;
                
        case keyIdleDevice :
            if(!ABB_1.info.statusBits.voltage && !FLAGS.shaftComplete && !PORT_ACor36V)
                ABB_1.deviceState = shaftDevice;
            else if(FLAGS.shaftComplete){
                if(ABB_1.info.statusBits.shaftFault)
                    ABB_1.deviceState = shaftDevice;
                else
                    ABB_1.deviceState = voltageDevice;
            }
            else if(ABB_1.info.statusBits.cable_fault)                          //if there is a cable fault
                    ABB_1.deviceState = cableFaultDevice;                       //go to cable Fault state
            else if(ABB_1.info.statusBits.key_switch_status ){                  // && ABB_1.info.statusBits.communication_status){//if the key switch is armed and we have communication
                if (FLAGS.min10)                                                //and the 10 min timeout flag is set
                    ABB_1.deviceState = mainsDevice;                            //check the power status before programming
                else                                                            //if the 10 minute flag is clear it means that programming has already been completed
                    ABB_1.deviceState = programSuccessDevice;                   //so check whether is was successful or not
            } 
            else if (FLAGS.bluetoothTimer && FLAGS.bluetooth)                          //if the bluetooth timeout has occurred
                ABB_1.deviceState = bluetoothOffDevice;                         //the turn off the bluetooth
            else                                                                //otherwise
                ABB_1.deviceState = idleDevice;                                 //just idle
            break;
        
        case mainsDevice :
            if(ABB_1.info.statusBits.mains)                                     //check to see whether mains has been detected
                ABB_1.deviceState = bluetoothKeyDevice;                         //if so then check to see whether the bluetooth should be activated
            else{
                if(ABB_1.info.statusBits.lowBat)                                //no mains so check if low battery should be indicated
                    ABB_1.deviceState = lowBatDevice;
                else
                   ABB_1.deviceState = bluetoothKeyDevice;                      //otherwise check to see whether bluetooth should be activated 
            }
            break;
                
        case lowBatDevice :                                                     
            if(FLAGS.sec3){
                if(ABB_1.info.statusBits.lowBat2)                               //if the battery is too low to program then go to the off state
                    ABB_1.deviceState = lowBat2Device;
                else                                                            //if 2 seconds have passed since it was armed
                    ABB_1.deviceState = bluetoothKeyDevice;                     //otherwise check if bluetooth should be activated
            }
            break;
            
        case lowBat2Device :                                                    
            ABB_1.deviceState = offDevice;                                      //then switch off
            break;
            
        case offDevice :                                                        
            if(ABB_1.info.statusBits.mains)                                     //only leave the off state if mains is returned
                ABB_1.deviceState = turnOnDevice;             
            break;
            
        case turnOnDevice :
            ABB_1.deviceState = idleDevice;
            break;
            
        case bluetoothKeyDevice :                                               
            if(FLAGS.sec1){                                                     //if 2 seconds has passed from the when the key was armed
                if(ABB_1.info.statusBits.key_switch_status)                     //if the key has been not been disarmed in this time
                    ABB_1.deviceState = programDevice;                          //then program the EDDs
                else
                    ABB_1.deviceState = bluetoothDevice;                        //otherwise activate the bluetooth
            }
            break;
            
        case bluetoothDevice :
            ABB_1.deviceState = idleDevice;                                     //once the bluetooth has been activated return to idle
            break;
            
        case bluetoothOffDevice :
            ABB_1.deviceState = idleDevice;                                     //once bluetooth has been deactivated return to idle
            break;
            
        case programDevice :                                                    
                ABB_1.deviceState = programSuccessDevice;                       //once programming has been completed see whether or not it was successful
            break;
            
        case programSuccessDevice :
            if(FLAGS.progSuccess)                                               //if programming was successful
                ABB_1.deviceState = readyDevice;                                //indiacte the device is ready to fire
            else
                ABB_1.deviceState = detErrorDevice;                             //otherwise indicate it was unsuccessful
            break;
            
        case readyDevice :
            if(FLAGS.fireFlag || COUNTERS.missingPulses > 3)                    //if the fire command has been received or more than 3 missing pulses have been detected
                ABB_1.deviceState = fireDevice;                                 //then call the fire routine
            else
                ABB_1.deviceState = keyIdleDevice;                              //otherwise check to see if an other paramters have changed
            
            break;

        case detErrorDevice :
            if(FLAGS.fireFlag || COUNTERS.missingPulses > 3)                    //if the fire command has been received or more than 3 missing pulses have been detected
                ABB_1.deviceState = fireDevice;                                 //then call the fire routine
            else
                ABB_1.deviceState = keyIdleDevice;                              //otherwise check to see if an other paramters have changed
            
            break;
            
        case fireDevice :
            ABB_1.deviceState = fireCheckDevice;                                //once firing is complete check to see if it was successful
            break;
            
        case fireCheckDevice :                                                  
            if(FLAGS.fireSuccessFlag)                                           //if firing was successful
                ABB_1.deviceState = successDevice;                              //indicate it
            else
                ABB_1.deviceState = failDevice;                                 //otherwise indicate is was unsuccessful
            break;
            
        case successDevice:
            ABB_1.deviceState = keyFireDevice;                                  //check to see if the device should return from firing
            break;
            
        case failDevice:
            ABB_1.deviceState = keyFireDevice;                                  //check to see if the device should return from firing
            break;
            
        case keyFireDevice :
            if(!ABB_1.info.statusBits.key_switch_status)//then return from firing                        //if the key switch has been disarmed
                ABB_1.deviceState = fireReturnDevice; 
            else if(FLAGS.fireFlag || COUNTERS.missingPulses > 3)                    //if the fire command has been received or more than 3 missing pulses have been detected
                ABB_1.deviceState = fireDevice;                                 //then call the fire routine
            else if(!ABB_1.info.statusBits.voltage && !FLAGS.shaftComplete && !PORT_ACor36V)
                ABB_1.deviceState = shaftDevice;
            else if(FLAGS.shaftComplete){
                if(ABB_1.info.statusBits.shaftFault)
                    ABB_1.deviceState = shaftDevice;
                else
                    ABB_1.deviceState = voltageDevice;
            }
            else
                ABB_1.deviceState = fireCheckDevice;                            //check whether firing was successful or not
            break;
            
        case fireReturnDevice:                                                  
            ABB_1.deviceState = idleDevice;                                     //return to the idle state
            break;
            
        default :
            break;
    }
}

//determines whether a change has been made to an of the status bits of the device
//if something has changed it adds a data message to the outgoing queue to be sent 
//to the CCB
void checkStatusBits(void){ 
    unsigned short *status;
    status = &ABB_1.info.statusBits;                                            //create a pointer to the current status bits
    if(*status != previousStatus){                                              //compare the value of the current pointer to the previous value
        addPacketToOutgoingQueue(0, CMD_AB1_DATA, 0, ABB_1.destination);        //if its changed add a data packet to the outgoing queue
        previousStatus = *status;                                               //assign the current pointer to the previousStatus value for the next comparison
    }
}
