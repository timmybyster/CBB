/*
 * File:   outgoingMessages.c
 * Author: Tim Buckley
 *
 * Created on 30 April 2017, 12:13 PM
 */


#include "main.h"
#include "modem.h"

extern unsigned char InitST7540(void);
extern void buildMessageST7540(void);
extern void StartTransmitST7540(void);
extern unsigned char TransmitBusyST7540(void);
extern unsigned char LineIdleST7540(void);
extern void initialiseState(states *specific, char number);

void initialiseQueues(void);

//initialises the modem as well as the message queues and 
void initialiseModem(void){
    unsigned char attempts = 0;                                                 //initialise the attempts to 0
    sspiInterrupt = 0;                                                          //disable the sppi Interrupt
    initialiseState(&state.outgoingMessages, outgoingMessagesState);            //initialise the outgoing messages state for the state handler
    initialiseState(&state.incomingCommands, incomingCommandsState);            //initialise the incoming commands state for the state handler    
    initialiseQueues();                                                         //initialise the incoming and outgoing message queues
    while(!InitST7540() && attempts < 5){
        attempts++;
    }
    TRISBbits.TRISB6 = 0;                                                       //ground the Header Pins
    LATBbits.LATB6 = 0;
    TRISBbits.TRISB7 = 0;
    LATBbits.LATB7 = 0;
}

//processes messages that need to be sent to the surface
void outgoingMessages(void){
    switch (state.outgoingMessages.current){
        case checkQueue :
            state.outgoingMessages.counter = checkOutgoingQueuePeriod;          //load the counter to move to the next state
            break;
            
        case buildMessage :
            state.outgoingMessages.counter = buildOutGoingMessagePeriod;        //load the counter to move to the next state
            FLAGS.acknowledgeCCB = 1;                                           //set the acknowledge Flag to wait for the data to be received by the CCB
            buildMessageST7540();                                               //build a message to be sent to the surface from the queue                                               
            COUNTERS.communicationStatus = noCommsPeriod;
            break;
            
        case lineClear :
            state.outgoingMessages.counter = lineClearPeriod;                   //load the counter to move to the next state
            break;
            
        case transmit :
            state.outgoingMessages.counter = transmitPeriod;                    //load the counter to move to the next state
            StartTransmitST7540();                                              //start transmitting the message
    }                                                                           
    
}

//determines the next state in the outgoing messages process based on the current
//state and the process variables
void outgoingMessagesStateHandler(void){
    switch(state.outgoingMessages.current){
        case checkQueue : 
            if(FLAGS.acknowledgeCCB)
                state.outgoingMessages.next = lineClear;                         //the acknowledge has not been received so send the message again
            else if (outgoingQueue.length > 0)                                  //if there is a message in the queue                                      
                state.outgoingMessages.next = buildMessage;                     //build the  next message
            else
                state.outgoingMessages.next = checkQueue;                       //otherwise wait for something in the queue
            break;
        
        case buildMessage :
            state.outgoingMessages.next = lineClear;                            //after the message has been built check if the line is clear
            break;
            
        case lineClear :
            if (!TransmitBusyST7540() && LineIdleST7540() && ABB_1.info.statusBits.communication_status)//Only allowed to transmit if the we are not already transmitting and not receiving and there have been valid pings from the surface
                state.outgoingMessages.next = transmit;                         //if conditions are met transmit the message
            else
                state.outgoingMessages.next = lineClear;                        //otherwise wait for the line to clear
            break;
            
        case transmit :
            state.outgoingMessages.next = checkQueue;                           //after the message has been sent check the queue for more messages
            break;
            
        default :
            state.outgoingMessages.next = checkQueue;                           //if context is lost default to checking the queue
    }
}

//initalises the incoming and outgoing message queues
void initialiseQueues(void){
    outgoingQueue.queue_pointer = 0;                                            //set the queue pointer to 0
    outgoingQueue.length = 0;                                                   //set the queue length to 0
    incomingQueue.queue_pointer = 0;                                            //set the queue pointer to 0
    incomingQueue.length = 0;                                                   //set the queue length to 0
}

void checkOutgoingMessages(void){
    if(FLAGS.acknowledgeCCB)
        FLAGS.acknowledgeCCB = 0;
}
