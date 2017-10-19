/* 
 * File: tag.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef TAG_H
#define	TAG_H

#include <xc.h>

#define tagCheckInterval        100

#define scanTagPins             1
#define tagDetected             2
#define tagWait                 3
#define tagClear                4
#define tagRead                 5
#define scanTagPinsReverse      6
#define tagClearReverse         7
#define tagDetectedReverse      8

#define tagReadValue            0x100
#define tagClearValue           0x200

#define testRepetitions         5
#define ADCDelay                50
#define allowedVariance         100
    
#endif TAG_H