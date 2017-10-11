/* 
 * File: tagRead.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef TAG_READ_H
#define	TAG_READ_H

#include <xc.h>

#define tagAttempts             5
#define halfBitPeriod           6  

//PIN DEFINITIONS
#define TRIS_TAG_UIO                    TRISEbits.TRISE3
#define ANSEL_TAG_UIO                   ANSELEbits.ANSELE3
#define LAT_TAG_UIO                     LATEbits.LATE3
#define PORT_TAG_UIO                    PORTEbits.RE3
#define WPU_TAG_UIO                     WPUEbits.WPUE3

#define TRIS_TAG_UIO_1                  TRISEbits.TRISE2
#define ANSEL_TAG_UIO_1                 ANSELEbits.ANSELE2
#define LAT_TAG_UIO_1                   LATEbits.LATE2
#define PORT_TAG_UIO_1                  PORTEbits.RE2
#define WPU_TAG_UIO_1                   WPUEbits.WPUE2
//END OF PIN DEFINITIONS

//ADC DEFINITIONS
#define tagADCChannel           0b00100011
#define tagADCChannel_1         0b00100010
#define tagEdgeDifference       20
//END OF ADC DEFINTIONS

//LED DEFINITIONS
#define tagGreen       14
#define tagRed         15
//END OF LED DEFINITIONS

//VARIABLE DEFINITIONS
unsigned char	TAG_Request[10];
unsigned char   TAG_Response[10];
unsigned int	UIO_data_out;
unsigned char	tag_state;
unsigned char	gDigit;
unsigned char 	j;
unsigned int	ii;
unsigned short  previousTagADC;
//END OF VARIABLE DEFINITIONS

#endif TAG_READ_H