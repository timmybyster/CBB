/* 
 * File: shaftTest.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef SHAFT_TEST_H
#define	SHAFT_TEST_H

#include <xc.h>

//SHAFT TEST INTERVALS
#define relayInterval          100
#define normalWaitInterval     4000
#define shaftTestInterval      10000
//END OF SHAFT TEST INTERVALS

//SHAFT TEST PINS
#define ANSEL_Relay_Drive      ANSELAbits.ANSELA1
#define TRIS_Relay_Drive       TRISAbits.TRISA1
#define LAT_Relay_Drive        LATAbits.LATA1
#define PORT_Relay_Drive       PORTAbits.RA1

#define ANSEL_EL_Enable        ANSELGbits.ANSELG2
#define TRIS_EL_Enable         TRISGbits.TRISG2
#define LAT_EL_Enable          LATGbits.LATG2
#define PORT_EL_Enable         PORTGbits.RG2

#define TRIS_CableTestEnable   TRISHbits.TRISH3
#define LAT_CableTestEnable    LATHbits.LATH3
#define PORT_CableTestEnable   PORTHbits.RH3

#define ANSEL_CableTestRead    ANSELGbits.ANSELG3
#define TRIS_CableTestRead     TRISGbits.TRISG3
#define LAT_CableTestRead      LATGbits.LATG3
#define PORT_CableTestRead     PORTGbits.RG3

#define ANSEL_EarthLeakage     ANSELGbits.ANSELG7
#define TRIS_EarthLeakage      TRISGbits.TRISG7
#define LAT_EarthLeakage       LATGbits.LATG7
#define PORT_EarthLeakage      PORTGbits.RG7
//END OF SHAFT TEST PINS

//SHAFT TEST ADC CONFIG
#define CHANNEL_CableTestRead  0b00110011
#define CHANNEL_EarthLeakage   0b00101001
#define ADCDelay               20
#define testRepetitions        5  
//SHAFT TEST ADC CONFIG

//SHAFT TEST RANGES
//ShaftFault _____ on ADC
#define shaftFaultRead      0x01F0
//earthFault _____ on ADC
#define earthFaultRead      742

#define allowedVariance     100
//END OF SHAFT TEST RANGES

//SHAFT TEST STATES
#define relayDrive          1
#define cableEnable         14
#define cableRead           2
#define cableCheck          3
#define faultDetected       4
#define earthLeakage        5
#define earthRead           6
#define earthCheck          7
#define earthFault          8
#define shaftTestCheck      9
#define noFault             10
#define shaftTestComplete   11
#define earthEnable         12
#define shaftTestWait       13

//END OF SHAFT TEST STATES

#define MaxShaftTests       10

#endif SHAFT_TEST_H