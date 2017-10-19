/* 
 * File: readKeyCable.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef READ_KEY_CABLE_H
#define	READ_KEY_CABLE_H

#include <xc.h>

//READ KEY CABLE INTERVALS
#define cableEnableInterval  25
#define cableKeyInterval     50
#define keyCableInterval     200
//END OF READ KEY CABLE INTERVALS

//READ KEY CABLE PINS
#define ANSEL_CableTest_Enable      ANSELEbits.ANSELE5
#define TRIS_CableTest_Enable       TRISEbits.TRISE5
#define LAT_CableTest_Enable        LATEbits.LATE5
#define PORT_CableTest_Enable       PORTEbits.RE5

#define ANSEL_S1_State              ANSELAbits.ANSELA3
#define TRIS_S1_State               TRISAbits.TRISA3
#define LAT_S1_State                LATAbits.LATA3
#define PORT_S1_State               PORTAbits.RA3

#define ANSEL_EDDRead               ANSELEbits.ANSELE7
#define TRIS_EDDRead                TRISEbits.TRISE7
#define LAT_EDDRead                 LATEbits.LATE7
#define PORT_EDDRead                PORTEbits.RE7
//END OF READ KEY CABLE PINS

//READ KEY CABLE ADC CONFIG
#define CHANNEL_S1_State            0b00000011
#define CHANNEL_EDDRead             0b00100111
#define ADCDelay                    40
#define allowedVariance             100
//READ KEY CABLE ADC CONFIG

//READ KEY CABLE RANGES
#define cableFaultReadValue         0x046
#define keyReadValue                0x010 
#define testRepetitions             5     
//END OF READ KEY CABLE RANGES

//READ KEY CABLE STATES
#define cableEnable         1
#define cableRead           2
#define cableFaultFound     3
#define noFaultFound        4
#define keyRead             5
#define keyOn               6
#define keyOff              7

unsigned char testCounter;

#endif READ_KEY_CABLE_H