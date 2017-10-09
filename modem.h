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
#define confirmAck      5
#define removeFromQueue 6

//INCOMING COMMANDS STATES
#define clearBuffer         1
#define checkQueueNew       2
#define newToQueue          3
#define handleIncomingQueue 4

//OUTGOING QUEUE TIMES
#define checkOutgoingQueuePeriod    10
#define buildOutGoingMessagePeriod  10
#define lineClearPeriod             10
#define transmitTimeoutPeriod       1000

//INCOMING QUEUE TIMES
#define clearBufferPeriod            10
#define checkQueueNewPeriod          10
#define newToQueuePeriod             10
#define handleIncomingPeriod         10

#endif MODEM_H