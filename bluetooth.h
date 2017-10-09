/* 
 * File: Bluetooth.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */
  
#ifndef BLUETOOTH_H
#define	BLUETOOTH_H

#include <xc.h>

//Definitions
#define     MAX_DELAY           200
#define     MIN_BT_LENGTH       5
#define     MAX_RESPONSE_SIZE   11

#define     ANSEL_BT_RX            ANSELEbits.ANSELE1
#define     TRIS_BT_RX             TRISEbits.TRISE1
#define     LAT_BT_RX              LATEbits.LATE1
#define     TRIS_BT_TX             TRISEbits.TRISE0
#define     LAT_BT_TX              LATEbits.LATE0

#define     TRIS_BT_RST            TRISGbits.TRISG0
#define     ANSEL_BT_RST           ANSELGbits.ANSELG0
#define     LAT_BT_RST             LATGbits.LATG0

#define     TRIS_BT_ENABLE         TRISGbits.TRISG1
#define     LAT_BT_ENABLE          LATGbits.LATG1

#define     uartEnable             RC3STAbits.SPEN
#define     uartRecieveEnable      RC3STAbits.CREN
#define     uartTransmitEnable     TX3STAbits.TXEN
#define     uartReceiveByte        RC3REG
#define     uartTransmitByte       TX3REG

//BLUETOOTH PACKET STUCTURES
typedef union bluetoothPackets{
    struct{
        unsigned char start;
        unsigned char size;
        unsigned char command;
        unsigned char data[6];
        unsigned short crc;
    };
}bluetoothPacket;

typedef struct btPackets{
    bluetoothPacket send;
    bluetoothPacket receive;
}btPacket_t;

btPacket_t btPacket;

typedef union{
    struct{
        unsigned startFound         : 1;
        unsigned bluetoothSetup     : 1;
        unsigned dataReceived       : 1;
        unsigned ACKReady           : 1;
        unsigned packetReceived     : 1;
        unsigned B5                 : 1;
        unsigned B6                 : 1;
        unsigned B7                 : 1;
    };
}bluetoothStatus_t;

bluetoothStatus_t bluetoothStatus;
unsigned char expectedDets;

unsigned char index, setupIndex;
unsigned char dataR[25];

//BLUETOOTH COMMANDS
#define CMD_BT_HEADER               1
#define CMD_BT_INFO                 2
#define CMD_BT_END                  3
#define CMD_BT_STATUS               4
#define CMD_BT_BOOT                 5
#define CMD_BT_EDD_LENGTH           6
#define CMD_BT_UID                  7
#define CMD_BT_PROGRAMMING          8
#define CMD_BT_CALIBRATING          9
#define CMD_BT_SELF_CHECKING        10

//BLUETOOTH STATES
#define     deviceIdle              5
#define     receiveData             2
#define     processData             3
#define     sendData                4
#define     setupCheck              1
#define     setup                   6
#define     processSetup            7
#define     receiveSetup            8
#define     resetCheck              9
#define     reset                   10
#define     sendPacketToSurface     11

#define     startByte               0xAA

#define     SPRG9600                0x0682
#define     SPRG115200              0x08A

//LED DEFINITIONS
#define solidRed       4
#define solidGreen     5
#define solidBlue      6
//END OF LED DEFINITIONS

//BLUETOOTH STATE TIMES
#define setupWaitPeriod     300
#define processPeriod       1

//PROGRAMMING STAGES
#define programming         1
#define calibrating         2
#define selfChecking        3

#endif BLUETOOTH_H