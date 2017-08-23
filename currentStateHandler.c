/*
 * File:   currentStateHandler.c
 * Author: Tim Buckley
 *
 * Created on 05 April 2017, 10:26 AM
 */

#include "main.h"

void checkCounter(states *specific );
void updateStateIds(void);

//These functions only execute processes and store the results in the relevant structures.
extern void ledStateHandler(void);
extern void led(void);
extern void readSupplyStateHandler(void);
extern void readSupply(void);
extern void readKeyCableStateHandler(void);
extern void readKeyCable(void);
extern void outgoingMessagesStateHandler(void);
extern void outgoingMessages(void);
extern void incomingCommandsStateHandler(void);
extern void incomingCommands(void);
extern void shaftTestStateHandler(void);
extern void shaftTest(void);
extern void bluetoothStateHandler(void);
extern void bluetooth(void);

/*The current State Handler manages all background processes and acts as a form 
 * of multi-tasking allowing processes to run independently and asynchronously 
 * from one another.
 * The context of each process is stored in its specific memory structure. The 
 * memory structure stores the current state, the next state and the process 
 * counter that acts as the process clock.
 * Every ms the counter of each state is decremented until zero at which point 
 * the next state becomes the current state. This state then gets added to the 
 * state queue. The state Handler executes any process in the context of the
 * process that has been added to the state queue until the queue is empty. 
 * This ensures that multiple processes can happen in very quick succession of 
 * each other giving the impression of multi tasking. 
 * 
 * Importantly each process is divided up into small segments of code that can be
 * be called and executed quickly.
 */
void currentStateHandler(void){
    
    switch(state.device){                                                       //switch statement based on the the state that should now
        case wait:                                                              //be executed.
            break;
            
        case readKeyCableState :                                                
            readKeyCable();                                                     //execute code based on the context of the read Key Cable process
            readKeyCableStateHandler();                                         //determine the next state of the read Key Cable process 
            state.readKeyCable.flag = 0;                                        //clear the read Key Cable process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case readSupplyState :
            readSupply();                                                       //execute code based on the context of the read Supply process
            readSupplyStateHandler();                                           //determine the next state of the read supply process 
            state.readSupply.flag = 0;                                          //clear the read Key supply Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case outgoingMessagesState :
            outgoingMessages();                                                 //execute code based on the context of the Outgoing Messages process
            outgoingMessagesStateHandler();                                     //determine the next state of the Outgoing Messages process 
            state.outgoingMessages.flag = 0;                                    //clear the outgoing messages process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case ledState :
            led();                                                              //execute code based on the context of the led process
            ledStateHandler();                                                  //determine the next state of the led process 
            state.led.flag = 0;                                                 //clear the led process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case incomingCommandsState :
            incomingCommands();                                                 //execute code based on the context of the incoming commands process
            incomingCommandsStateHandler();                                     //determine the next state of the incoming commands process 
            state.incomingCommands.flag = 0;                                    //clear the incoming commands process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case shaftTestState :
            shaftTest();                                                        //execute code based on the context of the shaft test process
            shaftTestStateHandler();                                            //determine the next state of the shaft test process 
            state.shaftTest.flag = 0;                                           //clear the shaft test process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        case bluetoothState :
            bluetooth();                                                        //execute code based on the context of the bluetooth process
            bluetoothStateHandler();                                            //determine the next state of the bluetooth process 
            state.bluetooth.flag = 0;                                           //clear the bluetooth process Flag to indicate to the state 
            break;                                                              //handler that this context has been executed.
            
        default :
            break;
    }
    if(state.queuePointer > 0){                                                 //If there is something in the state Queue
        CLRWDT();
        state.queuePointer--;                                                   //decrement the state Queue pointer
        state.device = state.stateQueue[0];                                     //make the next state to be excuted the first state added to the queue
        for(int i = 0; i < state.queuePointer; i++){                            //shift all the states along 1 becomes 0, 2 becomes 1 etc.
            state.stateQueue[i] = state.stateQueue[i + 1];
        }
        state.stateQueue[state.queuePointer] = wait;                            //ensure that the end of the queue is denoted by wait
    }                                                                           //so when all processes have been executed the handler waits
    else{                                                       
        state.device = wait;                                                    //otherwise if there's nothing in the queue
        state.outgoingMessages.flag = 0;                                        //clear all the flags to ensure that they can be added appropriately
        state.bluetooth.flag = 0;
        state.incomingCommands.flag = 0;
        state.readKeyCable.flag = 0;
        state.readSupply.flag = 0;
        state.shaftTest.flag = 0;
    }
}


//The state Counter Handler serves as the process clock for each of the 
//background processes, determining when they should be added to the state queue
//and therfore executed.
//This gives it the ability to determine which processes are running at any
//point based on the behaviour of the device
void stateCounterHandler(void){
    switch(ABB_1.deviceState){
        
        case sleepDevice :
            break;
            
        case offDevice :
            checkCounter(&state.readSupply);                                    //the device is off so only check the supply to see whether or not 
            break;                                                              //to switch on again
            
        case fireDevice :
            break;
            
        case programDevice :
            break;
            
        default :
            checkCounter(&state.outgoingMessages);                              //normal operation
            checkCounter(&state.incomingCommands);                              //send and receive messages to and from the surface
            checkCounter(&state.readSupply);                                    //check for mains and the battery voltage level
            checkCounter(&state.led);                                           //Flash the LED accordingly
            checkCounter(&state.readKeyCable);                                  //check for cable Faults and the key switch
            if(FLAGS.bluetooth)                                                 //if bluetooth has been activated
                checkCounter(&state.bluetooth);                                 //process bluetooth data
            if(FLAGS.shaftCheck)                                                //if we should be performing a shaft check
                checkCounter(&state.shaftTest);                                 //process the shaft check counter
            
            break;
            
    }
}

//the checkCounter function decrements a the state counter of a process to 0 if
//it is not in the state queue.
//when the counter reaches 0 the process is added to the state queue
void checkCounter(states *specific ){
    if(!specific->flag){                                                        //if this specific state is not in the state queue
        if(!specific->counter){                                                 //if the counter is zero
            updateStateIds();
            specific->current = specific->next;                                 //make the next state the current state to be executed
            specific->flag = 1;                                                 //set the flag of this process to indicate it is in the queue
            state.stateQueue[state.queuePointer] = specific->id;                //add it to the queue
            state.queuePointer++;                                               //increment the queue pointer
        }
        else
            specific->counter--;                                                //otherwise just decrement the  process counter
    }
}

void updateStateIds(void){
    state.bluetooth.id = bluetoothState;
    state.incomingCommands.id = incomingCommandsState;
    state.led.id = ledState;
    state.outgoingMessages.id = outgoingMessagesState;
    state.readKeyCable.id = readKeyCableState;
    state.readSupply.id = readSupplyState;
    state.shaftTest.id = shaftTestState;
}