/* 
 * File:   eddComms.h
 * Author: Tim Buckley
 * Comments:
 * Revision history: 
 */
 
#ifndef EED_COMMS_H
#define	EED_COMMS_H

#include <xc.h>

/* EED COMMANDS */       
#define EDD_READ_UID_COMMAND                0b00000011
#define EDD_PROGRAM_UID_COMMAND             0b00000010
#define EDD_CALIBRATE_COMMAND               0b00001000
#define EDD_ENERGY_STORE_COMMAND            0b00001110
#define EDD_SELF_CHECK_COMMAND              0b00010110
#define EDD_BLAST_COMMAND                   0b00001111
#define EDD_DISCHARGE_ENERGY_COMMAND        0b00001101
        
#define readUIDTxLength             5
#define readUIDRxLength             4
#define programUIDTxLength          9
#define programUIDRxLength          4
#define selfCheckUIDTxLength        3
#define selfCheckUIDRxLength        1        
                
#define TRIS_TXisHIGH               TRISEbits.TRISE6
#define LAT_TXisHIGH                LATEbits.LATE6
#define ANSEL_TXisHIGH              ANSELEbits.ANSELE6
        
#define TRIS_EDDRead                TRISEbits.TRISE7
#define PORT_EDDRead                PORTEbits.RE7
#define ANSEL_EDDRead               ANSELEbits.ANSELE7
        
#define CHANNEL_EDDRead             0b00100111
              
#define WORD_DELAY              13
#define ENERGY_STORE_DELAY      10000
#define PRE_PULSE_DUTY      9
#define PULSE_DUTY          5                                                   //Calibration pulse duty cycle as a factor of 10
#define maxAttempts         4
//ADC Constants        
#define RX_BIT_HIGH         2

volatile unsigned int tx;
volatile unsigned short previous_ADC;
volatile char period;
unsigned int watchCounter;
unsigned short previousADC;
unsigned char halfBits;
unsigned char response[4];
                        
volatile unsigned eddState,readFlag;                                                
unsigned char byteRead = 0;
volatile unsigned char bitCounter = 0;

#define programDisarmValue          0x02
#define programCableFaultValue      0x316


#endif EED_COMMS_H

