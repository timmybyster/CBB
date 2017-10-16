/*
 * File:   packetQueueHandler.c
 * Author: Tim Buckley
 *
 * Created on 07 April 2017, 8:37 AM
 */
#include "main.h"
#include "modem.h"
#include "ST7540.h"

void addPacketToOutgoingQueue(char *data, unsigned char command, unsigned char length, unsigned short destination);
void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size );
unsigned char getIndexByUID(unsigned char *UID);
extern unsigned short PacketReadParamST7540(unsigned char paramName);
extern char *PacketDataST7540(void);

extern unsigned short CRC16(char *, unsigned short);

//an asynchronous function that can add messages to the queue be sent to the surface
void addPacketToOutgoingQueue(char *data, unsigned char command, unsigned char length, unsigned short destination){
    if(ABB_1.destination == 0xFFFF)
         return;
    
    outgoingQueue.queue_store[outgoingQueue.queue_pointer].command = command;   //store the command in the queue
    outgoingQueue.queue_store[outgoingQueue.queue_pointer].data = data;         //create a pointer to the data that needs to be sent
    outgoingQueue.queue_store[outgoingQueue.queue_pointer].data_length = length;//store the length of the data to be sent
    outgoingQueue.queue_store[outgoingQueue.queue_pointer].destination = destination;//store the destination the message should be sent to
    if(outgoingQueue.length < outgoingQueueLength)                                                     //increment the queue length
        outgoingQueue.length++;
    outgoingQueue.queue_pointer++;                                              //increment the queue pointer
    if (outgoingQueue.queue_pointer == outgoingQueueLength){                    //if the queue pointer reaches the end of the queue
        outgoingQueue.queue_pointer = 0;                                        //start it at 0 again
    }
}

//adds a full length of data to the queue that can consist of multiple entries.
//used for sending whole lists of data to the surface and is based on the current
//number of EDDs connected
void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size ){
     unsigned char length_tracker = 0;                                          //initialise the length of the data to 0
     unsigned char length = 0;                                                  //initialise the length of the queue to 0
     
     if(ABB_1.destination == 0xFFFF)
         return;
     
     outgoingQueue.queue_store[outgoingQueue.queue_pointer].data = data + size; //set the data pointer for the queue to the index[1] of the data to be sent
     outgoingQueue.queue_store[outgoingQueue.queue_pointer].command = command;  //set the command
     outgoingQueue.queue_store[outgoingQueue.queue_pointer].destination = ABB_1.destination;//set the destination
     
     while (length_tracker + maxOutgoingData*length < size*ABB_1.dets_length) { //while the total length of data being sent is less than the full list of data
         length_tracker ++;                                                     //increment the length tracker
         
         if ( length_tracker == maxOutgoingData ){                              //when the data exceeds the maximum length of a packet
              outgoingQueue.queue_store[outgoingQueue.queue_pointer].data_length = length_tracker;//set the length of the packet to the max length
              
              length_tracker = 0;                                               //clear the length tracker
              length ++;                                                        //increment the length of the packets to be sent
              outgoingQueue.queue_pointer++;                                    //increment the queue pointer 
              if(outgoingQueue.length < outgoingQueueLength)                                                     //increment the queue length
                outgoingQueue.length++;                                           //increment the queue length
              if (outgoingQueue.queue_pointer == outgoingQueueLength){                  //ensure that of the queue is full it starts from the the first index
                  outgoingQueue.queue_pointer = 0;                              //set the queue pointer to 0
              }
              
              outgoingQueue.queue_store[outgoingQueue.queue_pointer].data = data + maxOutgoingData*length + size;//add the pointer of the next packet of data to the queue based on how many packets preceed it
              outgoingQueue.queue_store[outgoingQueue.queue_pointer].command = command;//assign the same command to the packet
              outgoingQueue.queue_store[outgoingQueue.queue_pointer].destination = ABB_1.destination;//assign the same destination
         }
     }
     if(length_tracker != 0){                                                   //this just excludes the very rare case that the packet lengths match the data
        outgoingQueue.queue_store[outgoingQueue.queue_pointer].data_length = length_tracker;//the last packet will have the length of the tracker

        outgoingQueue.queue_pointer++;                                          //increment the queue pointer 
        if(outgoingQueue.length < outgoingQueueLength)                                                     //increment the queue length
            outgoingQueue.length++;                                                 //increment the queue length
        if (outgoingQueue.queue_pointer == outgoingQueueLength){                        //ensure that of the queue is full it starts from the the first index
           outgoingQueue.queue_pointer = 0;                                     //set the queue pointer to 0
        }
     }
     return;
     
}

void handleIncomingQueuePacket(void){
    unsigned char index = 0; 
    if(incomingQueue.queue_pointer >= incomingQueue.length)                     //determine which message should be sent
        index = incomingQueue.queue_pointer - incomingQueue.length;             //the queue resides within the bounds of the queue length
    else
        index = queueLength + incomingQueue.queue_pointer - incomingQueue.length;//the queue overflows the bounds of the queue length and therefore has started at 0 again

    ABB_1.destination = incomingQueue.queue_store[index].source;//make the destination that messages will be sent to that of the received source
    FLAGS.communication_status = 1;                                             //set the communication flag
    ABB_1.info.statusBits.communication_status = 1;                             //set the communication status
    switch(incomingQueue.queue_store[index].command){     //process the packet based on the command
        case cmdPing :
            FLAGS.communication_status = 1;                                     //set the communication status
            break;
            
        case CMD_BLAST_COMMAND :
            FLAGS.fireFlag = 1;                                                 //set the fire Flag
            break;
            
        case CMD_AB1_DATA :                                                     //the CCB has acknowledged
            if(incomingQueue.queue_store[index].destination == ABB_1.serial){   //if this is the intended serial that has been acknowledged
                state.outgoingMessages.counter = 1;                             //clear the process counter so the CBB moves on immediately
                FLAGS.acknowledgeCCB = 0;                                       //clear the Flag to show there is no need to wait for an acknowledge    
            }
            break;
            
        case CMD_AB1_UID :                                                      //the CCB has acknowledged
            if(incomingQueue.queue_store[index].destination == ABB_1.serial){   //if this is the intended serial that has been acknowledged
                state.outgoingMessages.counter = 1;                             //clear the process counter so the CBB moves on immediately
                FLAGS.acknowledgeCCB = 0;                                       //clear the Flag to show there is no need to wait for an acknowledge    
            }
            break;
            
        case CMD_SEND_DEFAULT :
            addPacketToOutgoingQueue(0, CMD_AB1_DATA, 0, ABB_1.destination);        //if its changed add a data packet to the outgoing queue
            break;
            
        case CMD_FORCE_DEFAULT :
            addDataToOutgoingQueue(ABB_1.det_arrays.info, CMD_AB1_DATA, sizeof(detonator_data));
            break;
            
        case CMD_GET_SN :
            addDataToOutgoingQueue(ABB_1.det_arrays.UIDs, CMD_AB1_UID, sizeof(detonator_UID));
            break;
            
        default :
            break;
                
    }
    incomingQueue.length--;                                                     //remove the processed packet from the queue
}

void addPacketToIncomingQueue(void){
    unsigned short destination = PacketReadParamST7540(ST7540_DEST);
    if(PacketReadParamST7540(ST7540_CRC_VALID) && destination == ABB_1.serial || destination == 0x3FFF){
        incomingQueue.queue_store[incomingQueue.queue_pointer].command = PacketReadParamST7540(ST7540_CMD);//store the command
        incomingQueue.queue_store[incomingQueue.queue_pointer].data_length = PacketReadParamST7540(ST7540_DATA_LEN);//store the data length
        if(incomingQueue.queue_store[incomingQueue.queue_pointer].data_length >= maxData) incomingQueue.queue_store[incomingQueue.queue_pointer].data_length = maxData;//limit the number of bytes that can be stored to the maxData length
        incomingQueue.queue_store[incomingQueue.queue_pointer].packet_number = PacketReadParamST7540(ST7540_NUMBER);//store the packet number
        incomingQueue.queue_store[incomingQueue.queue_pointer].source = PacketReadParamST7540(ST7540_SOURCE);//store the source of the packet
        incomingQueue.queue_store[incomingQueue.queue_pointer].destination = destination;//store the destination
        for (int i = 0; i < incomingQueue.queue_store[incomingQueue.queue_pointer].data_length; i++){//for the length of the data
                incomingQueue.queue_store[incomingQueue.queue_pointer].data[i] = *(PacketDataST7540() + i);//store each byte in the queue
        }
        incomingQueue.length++;
        incomingQueue.queue_pointer++;
        if (incomingQueue.queue_pointer == queueLength){                        //if the queue pointer exceeds the queue length
            incomingQueue.queue_pointer = 0;                                    //set it to 0
        }
    }
}

