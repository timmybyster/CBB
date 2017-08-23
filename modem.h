/* 
 * File: modem.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */

#ifndef MODEM_H
#define	MODEM_H

//OUTGOING MESSAGES STATES
#define checkQueue      1
#define buildMessage    2
#define lineClear       3
#define transmit        4    

//INCOMING COMMANDS STATES
#define clearBuffer         1
#define checkQueueNew       2
#define newToQueue          3
#define handleIncomingQueue 4

//OUTGOING QUEUE TIMES
#define checkOutgoingQueuePeriod    200
#define buildOutGoingMessagePeriod  200
#define lineClearPeriod             200
#define transmitPeriod              1000

//INCOMING QUEUE TIMES
#define clearBufferPeriod            1
#define checkQueueNewPeriod          100
#define newToQueuePeriod             100
#define handleIncomingPeriod         100

#endif MODEM_H