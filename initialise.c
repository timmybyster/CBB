/*
 * File:   initialise.c
 * Author: Tim Buckley
 *
 * Created on 17 April 2017, 5:57 PM
 */


#include "main.h"

void initialisePic(void);
void initialiseTimer2(void);
void initialiseTimer4(void);
void initialiseTimer6(void);
void initialiseTimer8(void);
void initialiseTimer0(void);
void initialiseTimer1(void);
void initialiseACinterrupt(void);
void initialise12V(void);
void initialise24V(void);
void initialiseWatchDogTimer(void);
void initialiseDetMemory(void);

void initialiseDetWindows(void);

extern void _delay_ms(unsigned int ms);

extern void initialiseModem(void);
extern void initialiseLed(void);
extern void initialiseReadSupply(void);
extern void initialiseTag(void);
extern void initialiseModem(void);
extern void initialiseReadKeyCable(void);
extern void initialiseShaftTest(void);
extern void initialiseEDDPins(void);
extern void initialiseBluetooth(void);

extern void readStructureFromEeprom(unsigned char *data, int size);
extern void addDataToOutgoingQueue (unsigned char *data, unsigned char command, int size );
extern void clearEDDStatusbits(void);
extern void intialiseAcOr36V(void);

void initialise(void){ 
    initialisePic();
    initialise12V();
    initialiseModem();
    initialise24V();
    initialiseTimer2();
    initialiseTimer4();
    initialiseTimer6();
    initialiseTimer8();
    initialiseTimer0();
    initialiseTimer1();
    initialiseEDDPins();
    initialiseLed();
    initialiseReadSupply();
    initialiseTag();
    initialiseReadKeyCable();
    initialiseShaftTest();
    initialiseBluetooth();
    intialiseAcOr36V();
    initialiseWatchDogTimer();
    initialiseDetMemory();
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;                                                        //Enable all unmasked peripheral interrupts
}

void initialisePic(void){
    //OSCILLATOR 64MHz
    OSCCON1bits.NDIV = 0b0000;
    OSCCON1bits.NOSC = 0b010;
    OSCCON2bits.CDIV = 0b0000;
    OSCCON2bits.COSC = 0b010;
}

void initialiseTimer2(void){
    //Timer 2 100us interrupt
    T2CLKCONbits.CS = 0b0001;
    PMD1bits.TMR2MD = 0;                                                        //Enable Timer2
    T2CONbits.T2OUTPS = 0b0000;                                                 //Timer2 postscalar = 1:1 (1ticks every Tcy)
    T2CONbits.TMR2ON = 0;                                                       //Timer2 OFF, switch on just before main loop
    T2CONbits.T2CKPS = 0b11;                                                    //Timer2 prescalar = 1:16
    TMR2 = 0;                                                                   //Preload value of 0
    INTCONbits.GIEH = 1;                                                        //Enable all unmasked interrupts
    INTCONbits.PEIE = 1;                                                        //Enable all unmasked peripheral interrupts
    PIR5bits.TMR2IF = 0;                                                        //Clear interrupt flag
    PIE5bits.TMR2IE = 1;                                                        //Enable Timer2 interrupt
    IPR5bits.TMR2IP = 1;                                                        //Timer2 interrupt = high priority
    PR2 = 199;                                                                  //Register to match Timer2 value to
}

void initialiseTimer4(void){
    //Timer 4 500us interrupt
    T4CLKCONbits.CS = 0b0001;
    PMD1bits.TMR4MD = 0;                                                        //Enable Timer2
    T4CONbits.T4OUTPS = 0b0100;                                                 //Timer4 postscalar = 1:% (1 tick every 5 Tcy)
    T4CONbits.TMR4ON = 0;                                                       //Timer4 OFF, switch on just before main loop
    T4CONbits.T4CKPS = 0b11;                                                    //Timer4 prescalar = 1:16
    TMR4 = 0;                                                                   //Preload value of 0
    INTCONbits.GIEH = 1;                                                        //Enable all unmasked interrupts
    PIR5bits.TMR4IF = 0;                                                        //Clear interrupt flag
    IPR5bits.TMR4IP = 1;                                                        //Timer4 interrupt = high priority
    PR4 = 240;                                                                  //Register to match Timer2 value to
}

void initialiseTimer6(void){
    //Timer 6 1ms interrupt
    T6CLKCONbits.CS = 0b0001;
    PMD1bits.TMR6MD = 0;                                                        //Enable Timer6
    T6CONbits.T6OUTPS = 0b1001;                                                 //Timer6 postscalar = 1:10 (1 tick every 10 Tcy)
    T6CONbits.TMR6ON = 0;                                                       //Timer6 OFF, switch on just before main loop
    T6CONbits.T6CKPS = 0b11;                                                    //Timer6 prescalar = 1:16
    TMR6 = 0;                                                                   //Preload value of 0
    INTCONbits.GIEH = 1;                                                        //Enable all unmasked interrupts
    PIR5bits.TMR6IF = 0;                                                        //Clear interrupt flag
    PIE5bits.TMR6IE = 1;                                                        //Enable Timer6 interrupt
    IPR5bits.TMR6IP = 1;                                                        //Timer6 interrupt = high priority
    PR6 = 199;                                                                  //Register to match Timer6 value to
    T6CONbits.TMR6ON = 1;                                                       //Turn on Timer6
}

void initialiseTimer8(void){
    //Timer 8 50us interrupt
    T8CLKCONbits.CS = 0b0001;
    PMD2bits.TMR8MD = 0;                                                        //Enable Timer8
    T8CONbits.T8OUTPS = 0b0000;                                                 //Timer8 postscalar = 1:1 (1ticks every Tcy)
    T8CONbits.TMR8ON = 0;                                                       //Timer8 OFF, switch on just before main loop
    T8CONbits.T8CKPS = 0b11;                                                    //Timer8 prescalar = 1:16
    TMR8 = 0;                                                                   //Preload value of 0
    PIR5bits.TMR8IF = 0;                                                        //Clear interrupt flag
    IPR5bits.TMR8IP = 1;                                                        //Timer8 interrupt = high priority
    PR8 = 199;                                                                  //Register to match Timer8 value to
}

void initialiseTimer0(void){
    //triggers sec4Interrupt on 4s overflow
    T0CON1bits.T0CKPS = 0b1111;
    T0CON1bits.T0ASYNC = 0;
    T0CON1bits.T0CS = 0b010;
    T0CON0bits.T0EN = 1;
    T0CON0bits.T016BIT = 0;
    T0CON0bits.T0OUTPS = 0b1111;
    PIR0bits.TMR0IF = 0;
    PIE0bits.TMR0IE = 1;
    TMR0H = 122;
    TMR0L = 0;
}

void initialiseTimer1(void){
    T1CONbits.RD16 = 1;
    PIR5bits.TMR1IF = 0;
    TMR1CLKbits.CS = 0b0001;
    TMR1 = 0xA23F;
    T1CONbits.CKPS = 0b11;
}

void initialiseState(states *specific, char number){
    specific->counter = 0;
    specific->current = 1;
    specific->next = 1;
    specific->flag = 0;
    
    specific->id = number;
}

void initialiseDetWindows(void){
    for (int i = 0; i < maxDets; i++ ){
        ABB_1.det_arrays.UIDs[i].window = i;
        ABB_1.det_arrays.info[i].window = i;
    }
}

void initialise12V(void){
    ANSEL_12VCntrl = 0;
    TRIS_12VCntrl = 0;
    LAT_12VCntrl = 1;
}

void initialise24V(void){
    ANSEL_24VCntrl = 0;
    TRIS_24VCntrl = 0;
    LAT_24VCntrl = 0;
}

void initialiseStates(void){
    initialiseState(&state.incomingCommands, incomingCommandsState);
    initialiseState(&state.outgoingMessages, outgoingMessagesState);
    initialiseState(&state.led, ledState);
    initialiseState(&state.shaftTest, shaftTestState);
    initialiseState(&state.bluetooth, bluetoothState);
    initialiseState(&state.readSupply, readSupplyState);
    state.queuePointer = 0;
    for (int i = 0; i < queueLength; i ++){
        state.stateQueue[i] = 0;
    }
    
}

void initialiseWatchDogTimer(void){
    WDTCON0bits.WDTPS = 0b01110;
    CLRWDT();
    WDTCON0bits.SEN = 1;
}

void initialiseDetMemory(void){
    readStructureFromEeprom(&ABB_1.serial, 2);
    readStructureFromEeprom(&ABB_1.dets_length,1);
    if(ABB_1.dets_length == 0xFF)
        ABB_1.dets_length = 0;
    readStructureFromEeprom(&ABB_1.destination,2);
    readStructureFromEeprom(ABB_1.det_arrays.UIDs, sizeof(detonator_UID)*(ABB_1.dets_length + 1));
    readStructureFromEeprom(ABB_1.det_arrays.info, sizeof(detonator_data)*(ABB_1.dets_length + 1));
    clearEDDStatusbits();
    initialiseDetWindows();
}

