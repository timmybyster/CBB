/* 
 * File: readSupply.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef READ_SUPPLY_H
#define	READ_SUPPLY_H

#include <xc.h>

//READ SUPPLY INTERVALS
#define readSupplyInterval     50
//END OF READ SUPPLY INTERVALS

//READ SUPPLY PINS
#define ANSEL_BatSense      ANSELFbits.ANSELF2
#define TRIS_BatSense       TRISFbits.TRISF2
#define LAT_BatSense        LATFbits.LATF2
#define PORT_BatSense       PORTFbits.RF2

#define TRIS_MainsDetect    TRISHbits.TRISH2
#define LAT_MainsDetect     LATHbits.LATH2
#define PORT_MainsDetect    PORTHbits.RH2
//END OF READ SUPPLY PINS

//READ SUPPLY ADC CONFIG
#define CHANNEL_BatSense        0b00101010
#define ADCDelay                20
#define testRepetitions         5
#define allowedVariance         100
//READ SUPPLY ADC CONFIG

//READ SUPPLY RANGES
//Low Battery at 3,9V >> 1,60875V on ADC 804
#define lowBatteryRead      804
//Too Low Battery at 3,6V >> 1,485V on ADC 742
#define lowBattery2Read     0         
//END OF READ SUPPLY RANGES

//READ SUPPLY STATES
#define readMains           1
#define mainsDetected       2
#define readBattery         3
#define lowBattery          4
#define lowBattery2         5
#define noMains             6

unsigned char testCounter;

#endif READ_SUPPLY_H