#include <xc.h>

//READ VOLTAGE INTERVALS
#define readVoltageInterval     100
#define noVoltageInterval       100
//END OF READ VOLTAGE INTERVALS

#define samples36V              40

//READ VOLTAGE PINS
#define ANSEL_ACor36V       ANSELBbits.ANSELB2
#define TRIS_ACor36V        TRISBbits.TRISB2
#define LAT_ACor36V         LATBBits.LATB2
#define PORT_ACor36V        PORTBbits.RB2
//END OF READ VOLTAGE PINS

//READ VOLTAGE STATES
#define read36V             1
#define voltageDetected     2
#define noVoltage           3
#define noVoltageConfirmed  4

unsigned char testCounter;