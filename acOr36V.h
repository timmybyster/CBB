#define sec4Enable                      T0CON0bits.T0EN
#define sec4Interrupt                   PIE0bits.TMR0IE
#define sec4InterruptFlag               PIR0bits.TMR0IF
#define sec4Timer                       TMR0
#define sec4TimerValue                  

#define ms13Enable                      T1CONbits.ON
#define ms13InterruptFlag               PIR5bits.TMR1IF
#define ms13Timer                       TMR1
#define ms13TimerValue                  0xA23F