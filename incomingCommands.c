/*
 * File:   incomingCommands.c
 * Author: Tim Buckley
 *
 * Created on 10 May 2017, 8:51 AM
 */


#include "main.h"
#include "modem.h"

extern void ReceiveNewDataST7540(void);
extern void addPacketToIncomingQueue(void);
extern void addPacketToIncomingQueue(void);
extern void handleIncomingQueuePacket(void);
extern unsigned char DataReadyST7540(void);

//executes code for the incoming commands process based on the current state 
void incomingCommands(void){
    switch(state.incomingCommands.current){
        case clearBuffer :
            modemReceiveInterrupt = 1;                                          //ensure that the modem receive interrupt is enabled
            ReceiveNewDataST7540();                                             //reset the receive buffer for new data
            state.incomingCommands.counter = clearBufferPeriod;                 //load the counter to move to the next state
            break;
            
        case checkQueueNew :
            state.incomingCommands.counter = checkQueueNewPeriod;               //load the counter to move to the next state
            break;  
            
        case newToQueue :
            addPacketToIncomingQueue();                                         //add the received data to the incoming queue
            state.incomingCommands.counter = newToQueuePeriod;                  //load the counter to move to the next state
            break;
            
        case handleIncomingQueue :
            handleIncomingQueuePacket();                                        //handle packets in the incoming queue
            state.incomingCommands.counter = handleIncomingPeriod;              //load the counter to move to the next state
            break;
    }
    
}

//determine the next state for the incoming commands process
void incomingCommandsStateHandler(void){
    switch(state.incomingCommands.current){
        case clearBuffer :
            state.incomingCommands.next = checkQueueNew;                        //after the buffer has been cleared see if there is new data or something in the queue
            break;
        
        case checkQueueNew :
            if (DataReadyST7540())                                              //if new data has been received
               state.incomingCommands.next = newToQueue;                        //add it to the queue
            else if (incomingQueue.length > 0)                                  //otherwise if there is no new data but something in the queue
                state.incomingCommands.next = handleIncomingQueue;              //handle the queue
            break;
        
        case newToQueue :
            state.incomingCommands.next = clearBuffer;                          //after the new data has been added to the queue clear the buffer to receive new data
            break;
            
        case handleIncomingQueue :
            state.incomingCommands.next = checkQueueNew;                        //after a packet has been processed check if there is another packet or new data
            break;
            
        default :
            state.incomingCommands.next = clearBuffer;
    }
}
