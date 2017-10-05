/* 
 * File:   led.h
 * Author: Tim Buckley
 * Comments:
 * Revision history: 
 */

#ifndef LED_H
#define	LED_H

#include <xc.h>

//LED PIN DEFINITONS
#define TRIS_LED_RED                    TRISHbits.TRISH0
#define LAT_LED_RED                     LATHbits.LATH0
#define PORT_LED_RED                    PORTHbits.RH0

#define TRIS_LED_BLUE                   TRISAbits.TRISA0
#define ANSEL_LED_BLUE                  ANSELAbits.ANSELA0
#define LAT_LED_BLUE                    LATAbits.LATA0
#define PORT_LED_BLUE                   PORTAbits.RA0

#define TRIS_LED_GREEN                  TRISHbits.TRISH1
#define LAT_LED_GREEN                   LATHbits.LATH1
#define PORT_LED_GREEN                  PORTHbits.RH1
//END OF LED DEFINTIONS

//LED STATES
#define ledOff         1
#define ledOff1        2
#define offFlash       3
#define solidRed       4
#define solidGreen     5
#define solidBlue      6
#define flashRed       7
#define flashGreen     8
#define flashBlue      9
#define flashRed2      10
#define flashGreen2    11
#define flashBlue2     12
#define offFlash2      13
#define tagGreen       14
#define tagRed         15
//END OF LED STATES

//LED INFO BITS
#define redSingleFlash   0
#define greenSingleFlash 1
#define blueSingleFlash  2
#define redDoubleFlash   3
#define greenDoubleFlash 4
#define blueDoubleFlash  5
#define redSolid         6
#define greenSolid       7
#define blueSolid        8
#define redFastFlash     9
#define greenFastFlash   10
#define blueFastFlash    11


//LED TIMES
#define TAG_TEST_TIME      10
#define SOLID_ON_TIME      1000
#define OFF_TIME           900
#define FLASH_TIME         100
//END OF LED TIMES

#endif LED_H