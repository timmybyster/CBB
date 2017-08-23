/* 
 * File: main.h  
 * Author: Tim Buckley
 * Comments: 
 * Revision history: 
 */

#ifndef MAIN_H
#define	MAIN_H

// PIC18LF66K40 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config FEXTOSC = HS     // External Oscillator mode Selection bits (HS (crystal oscillator) above 8 MHz; PFM set to high power)
#pragma config RSTOSC = EXTOSC_4PLL// Power-up default value for COSC bits (EXTOSC with 4x PLL, with EXTOSC operating per FEXTOSC bits)

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (Power up timer disabled)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_190  // Brown Out Reset Voltage selection bits (Brown-out Reset Voltage (VBOR) set to 1.90V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = SWDTEN     // WDT operating mode (WDT enabled/disabled by SWDTEN bit)

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = LFINTOSC// WDT input clock selector (WDT reference clock is the 31.0 kHz LFINTOSC)

// CONFIG4L
#pragma config WRT0 = OFF       // Write Protection Block 0 (Block 0 (000800-003FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection Block 1 (Block 1 (004000-007FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection Block 2 (Block 2 (008000-00BFFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection Block 3 (Block 3 (00C000-00FFFFh) not write-protected)

// CONFIG4H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-30000Bh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config SCANE = ON       // Scanner Enable bit (Scanner module is available for use, SCANMD bit can control the module)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)

// CONFIG5L
#pragma config CP = OFF         // UserNVM Program Memory Code Protection bit (UserNVM code protection disabled)
#pragma config CPD = OFF        // DataNVM Memory Code Protection bit (DataNVM code protection disabled)

// CONFIG5H

// CONFIG6L
#pragma config EBTR0 = OFF      // Table Read Protection Block 0 (Block 0 (000800-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection Block 1 (Block 1 (004000-007FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection Block 2 (Block 2 (008000-00BFFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection Block 3 (Block 3 (00C000-00FFFFh) not protected from table reads executed in other blocks)

// CONFIG6H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0007FFh) not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
//============================================================================//
#define _XTAL_FREQ      16000000

#define AB1_SERIAL      0x0008

//PIN_DEFINITIONS
#define TRIS_24VCntrl   TRISFbits.TRISF3
#define LAT_24VCntrl    LATFbits.LATF3
#define ANSEL_24VCntrl  ANSELFbits.ANSELF3

#define TRIS_12VCntrl   TRISGbits.TRISG1
#define LAT_12VCntrl    LATGbits.LATG1
#define ANSEL_12VCntrl  ANSELGbits.ANSELG1
#define PORT_12VCntrl   PORTGbits.RG1

#define LAT_CableTest_Enable            LATEbits.LATE5

#define ANSEL_ACor36V           ANSELBbits.ANSELB2
#define TRIS_ACor36V            TRISBbits.TRISB2
#define LAT_ACor36V             LATBBits.LATB2
#define PORT_ACor36V            PORTBbits.RB2

#define ANSEL_FireSignal_Read   ANSELEbits.ANSELE4
#define TRIS_FireSignal_Read    TRISEbits.TRISE4
#define PORT_FireSignal_Read    PORTEbits.RE4
//END OF PIN DEFINITIONS

//TIMER DEFINITIONS
#define us100DelayEnable                T2CONbits.T2ON
#define us100DelayInterrupt             PIE5bits.TMR2IE
#define us100DelayInterruptFlag         PIR5bits.TMR2IF

#define msEnable                        T6CONbits.T6ON
#define msInterrupt                     PIE5bits.TMR6IE
#define msInterruptFlag                 PIR5bits.TMR6IF

#define us100Enable                     T8CONbits.T8ON
#define us100Interrupt                  PIE5bits.TMR8IE
#define us100InterruptFlag              PIR5bits.TMR8IF

#define us500Enable                     T4CONbits.T4ON
#define us500Interrupt                  PIE5bits.TMR4IE
#define us500InterruptFlag              PIR5bits.TMR4IF

#define sec4Enable                      T0CON0bits.T0EN
#define sec4Interrupt                   PIE0bits.TMR0IE
#define sec4InterruptFlag               PIR0bits.TMR0IF
#define sec4Timer                       TMR0

#define ms13Enable                      T1CONbits.ON
#define ms13InterruptFlag               PIR5bits.TMR1IF
#define ms13Timer                       TMR1
#define ms13TimerValue                  0xA23F                                  
//END OF TIMER DEFINITIONS

//INTERRUPT DEFINITIONS
#define acOr36VInterrupt                PIE0bits.INT2IE
#define acOr36VInterruptFlag            PIR0bits.INT2IF
#define acOr36VInterruptEdge            INTCONbits.INT2EDG

#define sspiInterrupt                   PIE3bits.SSP1IE
#define sspiInterruptFlag               PIR3bits.SSP1IF

#define modemReceiveInterrupt           PIE0bits.INT0IE
#define modemReceiveInterruptFlag       PIR0bits.INT0IF

#define uartReceiveInterrupt            PIE4bits.RC3IE
#define uartReceiveInterruptFlag        PIR4bits.RC3IF

#define tagInterrupt                    PIE0bits.INT3IE
#define tagInterruptFlag                PIR0bits.INT3IF
//STRUCTURE DEFINITIONS
#define maxDets             101
#define maxData             12
#define maxOutgoingData     60
#define queueLength         10
#define outgoingQueueLength 20

#define mins1                   60000
#define mins10                  600000
#define secs3                   3000
#define secs2                   2000
#define secs1                   1000
#define noCommsPeriod           4000
#define acMissingPulseCount     20

#define STANDARD_EDD_DELAY      4000

typedef union {
        struct{
               unsigned key_switch_status           :1;
               unsigned cable_fault                 :1;
               unsigned earth_leakage               :1;
               unsigned mains                       :1;
               unsigned shaftFault                  :1;
               unsigned voltage                     :1;
               unsigned lowBat                      :1;
               unsigned lowBat2                     :1;
               unsigned ready                       :1;
               unsigned detError                    :1;
               unsigned isolation_relay             :1;
               unsigned communication_status        :1;
               unsigned B12                         :1;
               unsigned B13                         :1;
               unsigned B14                         :1;
               unsigned B15                         :1;
        };
}ABB_status;

typedef union{
    struct{
        ABB_status statusBits;
        unsigned char CRC;
    };
}ABB_data;

typedef union {
        struct{
               unsigned B7                          :1;
               unsigned connection_status           :1;
               unsigned tagged                      :1;
               unsigned fired                       :1;
               unsigned program_status              :1;
               unsigned calibration_status          :1;
               unsigned bridge_wire_resitance       :1;
               unsigned energy_storing              :1;
        };
}detonator_status;

typedef union{
        struct{
        unsigned char UID[4];
        unsigned char window;
    };
}detonator_UID;

typedef union {
    struct{
        unsigned char window;
        detonator_status data;
        unsigned short delay;
    };
}detonator_data;

typedef union{
       struct{
            detonator_UID UIDs[maxDets];
            detonator_data info[maxDets];
       };
}detonator_array;

typedef union{
       struct{
            unsigned char dets_length;
            ABB_data info;
            detonator_array det_arrays;
            unsigned short destination;                                         //The current IBC for changes
            unsigned char ledDeviceState;
            unsigned char deviceState;
       };
}ABB;

typedef struct parsedPackets {
       unsigned short start;
       unsigned char data_length;
       unsigned short source;
       unsigned short destination;
       unsigned char packet_number;
       unsigned char command;
       unsigned char FF;
       unsigned char data[maxData];
       unsigned short CRC;
}parsedPacket;

typedef union{
    struct{
       unsigned char data_length;
       unsigned short destination;
       unsigned char packet_number;
       unsigned char command;
       unsigned char *data;
    };
}outgoingPacket;

typedef union{
    struct{
       unsigned char data_length;
       unsigned short source;
       unsigned short destination;
       unsigned char packet_number;
       unsigned char command;
       unsigned char data[maxData];
    };
}incomingPacket;

typedef union{
    struct{
        unsigned char length;
        unsigned char queue_pointer;
        outgoingPacket queue_store[outgoingQueueLength];
    };
}outgoing_q;

typedef union{
    struct{
        unsigned char length;
        unsigned char queue_pointer;
        incomingPacket queue_store[queueLength];
    };
}incoming_q;

ABB ABB_1;
unsigned short previousStatus;
unsigned char maxProgramAttempts;
outgoing_q outgoingQueue;
incoming_q incomingQueue;
unsigned char programRetries;
//END OF SRUCTURES

//FLAGS
typedef union {
        struct{
               unsigned us100                   :1;
               unsigned us200                   :1;
               unsigned us500                   :1;
               unsigned ms                      :1;
               unsigned sec1                    :1;
               unsigned sec2                    :1;
               unsigned sec3                    :1;
               unsigned min                     :1;
               unsigned min10                   :1;
               unsigned progComplete            :1;
               unsigned progSuccess             :1;
               unsigned fireFlag                :1;
               unsigned fireComplete            :1;
               unsigned shaftCheck              :1;
               unsigned shaftComplete           :1;
               unsigned onState                 :1;
               unsigned fireSuccessFlag         :1;
               unsigned communication_status    :1;
               unsigned bluetooth               :1;
               unsigned bluetoothTimer          :1;
               unsigned acknowledgeCCB          :1;
        };
}FLAGS_t;

FLAGS_t FLAGS;
//END OF FLAGS
typedef union {
        struct{
               unsigned short min;
               unsigned long min10;
               unsigned short sec1;
               unsigned short sec2;
               unsigned short sec3;
               unsigned short communicationStatus;
               unsigned char shaftTests;
               unsigned char missingPulses;
               unsigned char acPulses;
               unsigned int bluetoothTimer;
        };
}COUNTERS_t;

COUNTERS_t COUNTERS;
//COUNTERS


//STATES
typedef struct{
    unsigned char current;
    unsigned char next;
    unsigned int counter;
    unsigned flag;
    unsigned char id;
}states;

typedef struct{
    unsigned char stateQueue[queueLength];
    unsigned char queuePointer;
    unsigned char device;
    states led;
    states readKeyCable;
    states readSupply;
    states outgoingMessages;
    states incomingCommands;
    states shaftTest;
    states bluetooth;
}state_struct;

state_struct state;

//DEVICE STATES
#define sleepDevice           0
#define idleDevice            1
#define shaftDevice           2
#define cableFaultDevice      3
#define lowBatDevice          4
#define offDevice             5
#define readyDevice           6
#define detErrorDevice        7
#define programDevice         8
#define fireDevice            9
#define failDevice            10
#define successDevice         11
#define keyIdleDevice         12
#define voltageDevice         13
#define mainsDevice           14
#define programSuccessDevice  15
#define shaftFaultDevice      16
#define keyFireDevice         17
#define fireReturnDevice      18
#define fireCheckDevice       19
#define noCommsDevice         20
#define bluetoothKeyDevice    21
#define bluetoothDevice       22
#define bluetoothOffDevice    23
#define lowBat2Device         24
#define shaftTestDevice       25
#define turnOffDevice         26
#define turnOnDevice          27

#define wait                    0

//GLOBAL STATES
#define readKeyCableState        1
#define fireRoutineState         2
#define incomingCommandsState    3
#define ledState                 4
#define outgoingMessagesState    5
#define programUIDsState         6
#define readBlastLoopState       7
#define readKeyCable24State      8
#define readSupplyState          9
#define readVoltageState         10
#define shaftTestState           11
#define eddGlobalState           12
#define bluetoothState           13

//END OF STATES

//COMMAND DEFINITIONS
#define cmdPing                 0x29
#define CMD_FORCE_DEFAULT       0b01000000
#define CMD_SEND_TEMPERATURE    0b00000100
#define CMD_OPEN_RELAY          0b00000110
#define CMD_CLOSE_RELAY         0b00000101
#define CMD_GET_SN              0b00000111
#define CMD_SN_LIST_CHANGED     0b00001000
#define CMD_BLAST_COMMAND       0b00100101
#define PING_ISC                0b00101001
#define ARM_ISC                 0b00110001
#define DISARM_ISC              0b00110000
#define CMD_CABLE_FAULT         0b00100110
#define CMD_ISC_NEW_SN          0b00001001
#define CMD_NULL                0b11111111
#define CMD_AB1_UID             0b00001010
#define CMD_AB1_DATA            0b00001011

#define CMD_SEND_DEFAULT_B        0b10000000
#define CMD_FORCE_DEFAULT_B       0b11000000
#define CMD_SEND_DC_B             0b10000001
#define CMD_FORCE_DC_B            0b11000001
#define CMD_SEND_AC_B             0b10000010
#define CMD_FORCE_AC_B            0b11000010
#define CMD_SEND_BLAST_VALUE_B    0b10000011
#define CMD_FORCE_BLAST_VALUE_B   0b11000011
#define CMD_SEND_TEMPERATURE_B    0b10000100
#define CMD_OPEN_RELAY_B          0b10000110
#define CMD_CLOSE_RELAY_B         0b10000101
#define CMD_GET_SN_B              0b10000111
#define CMD_SN_LIST_CHANGED_B     0b10001000
#define CMD_BLAST_COMMAND_B       0b10100101
#define PING_ISC_B                0b10101001
#define ARM_ISC_B                 0b10110001
#define DISARM_ISC_B              0b10110000
#define CMD_CABLE_FAULT_B         0b10100110
#define CMD_ISC_NEW_SN_B          0b10001001
//END OF COMMAND DEFINITIONS

#endif MAIN_H