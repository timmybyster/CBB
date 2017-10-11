/*
 * File:   readTag.c
 * Author: Tim Buckley
 *
 * Created on 04 April 2017, 11:52 AM
 */


#include "main.h"
#include "tagRead.h"

unsigned int SCAN_TAG(void);
unsigned int SCAN_TAG_1(void);
void UIO_WRITE(unsigned char cmd);
unsigned char UIO_SAK(void);
void UIO_MAK(void);
void UIO_noMAK(void);
void UIO_WRITE_1(unsigned char cmd);
unsigned char UIO_SAK_1(void);
void UIO_MAK_1(void);
void UIO_noMAK_1(void);
unsigned char READ_TAG(void);
unsigned char READ_TAG_REVERSE(void);
unsigned char CHECK_CRC16(void);
unsigned int calcrc(unsigned char *ptr, int count);
void addNewTag(void);
void tagForInterrupt(void);
void tagForRead(void);
unsigned short readTagADC(void);
void initialiseTagADC(void);
void initialiseTagADC_1(void);
unsigned char detectFallingEdge(void);
unsigned char detectRisingEdge(void);
void tagFor0(void);
void tagFor1(void);

unsigned char rxWord(void);

extern void _delay_us(unsigned char delay);                                      
extern void _delay_ms(unsigned int delay);                                      
extern void WaitNewTick(void);
extern void ppsUnlock(void);
extern void ppsLock(void);

extern void addPacketToOutgoingQueue(char *data, unsigned char command, unsigned char length, unsigned short destination);
extern unsigned char checkForExistingUID(unsigned long *receivedUID);
unsigned char getTaggedUids(void);

extern void setBlueLed(void);
extern void initialiseStates(void);

//initialises the tag read pins and interrupt to trigger the tag read routine
void initialiseTag(void){
    tagForInterrupt();                                                          //initialise the tag pins to trigger the interrupt
    
    ppsUnlock();                                                                //unlock the peripheral port select
    INT3PPS = 0b00100010;                                                       //put INT3 on the RE2
    ppsLock();                                                                  //lock the PPS  
    
    INTCONbits.GIE = 0;
    INTCONbits.INT3EDG = 0;                                                     //INT3 on falling EDGE
    tagInterruptFlag = 0;                                                       //clear INT3 flag
    tagInterrupt = 1;                                                           //enable the tag interrupt
    
}

void tagSleep(void){
    tagInterrupt = 0;
    ANSEL_TAG_UIO = 0;                                                          //set the pin as digital
    TRIS_TAG_UIO = 1;                                                           //set the pin as an input
    WPU_TAG_UIO = 0;                                                            //enable a weak pull up
    LAT_TAG_UIO = 0;
    
    ANSEL_TAG_UIO_1 = 0;                                                        //set the pin as digital
    TRIS_TAG_UIO_1 = 1;                                                         //set the pin as an output
    LAT_TAG_UIO_1 = 0;                    
}

//prepares the pins to trigger an interrupt when the tag is placed in the cradle
void tagForInterrupt(void){
    ANSEL_TAG_UIO = 0;                                                          //set the pin as digital
    TRIS_TAG_UIO = 1;                                                           //set the pin as an input
    WPU_TAG_UIO = 1;                                                            //enable a weak pull up
    
    ANSEL_TAG_UIO_1 = 0;                                                        //set the pin as digital
    TRIS_TAG_UIO_1 = 0;                                                         //set the pin as an output
    LAT_TAG_UIO_1 = 0;                                                          //set the pin low
    
    ADCON0bits.ADON = 0;                                                        //turn off the ADC
    FVRCONbits.FVREN = 0;                                                       //turn off the FVR
    
    sec4Enable = 1;
}

//adds a new tag to memory after a successful read but only if it is not stored
//in memory currently
void addNewTag(void){
    if(checkForExistingUID(TAG_Response + 2)){
        state.led.next = tagGreen;                                              //show that the tag was successful
        return;                                                                 //if it exists do not add it
    }
    if(ABB_1.dets_length >= 100 || getTaggedUids() >= 10){//check the read UID from the tag against existing UIDs in memory
        state.led.next = tagRed;                                                //show that the tag was successful
        return;                                                                 //if it exists do not add it
    }
    state.led.next = tagGreen;                                                  //show that the tag was successful
    ABB_1.dets_length++;                                                        //otherwise increment the number of EDDs stored
    ABB_1.det_arrays.info[ABB_1.dets_length].delay = STANDARD_EDD_DELAY;        //assign it with the standard delay
    for (int i = 0; i < 4; i ++){                                               //for each index of the UID
        ABB_1.det_arrays.UIDs[ABB_1.dets_length].UID[i] = TAG_Response[i + 2];  //store it in memory
    }
    ABB_1.det_arrays.info[ABB_1.dets_length].data.tagged = 1;                   //show that the EDD was tagged in memory
    ABB_1.det_arrays.info[ABB_1.dets_length].data.logged = 0;                   //show that the EDD was not logged in memory
    addPacketToOutgoingQueue(&ABB_1.det_arrays.UIDs[ABB_1.dets_length], CMD_AB1_UID, sizeof(detonator_UID), ABB_1.destination);//send the UID to the surface
    addPacketToOutgoingQueue(&ABB_1.det_arrays.info[ABB_1.dets_length], CMD_AB1_DATA, sizeof(detonator_data), ABB_1.destination);//send the EDD data to the surface
}

unsigned int SCAN_TAG(void)
{
	// HEADER  / ID1 / ID2 / ID3 / ID4 / CRC1 / CRC2
	// AA / 55 / ID1 / ID2 / ID3 / ID4 / CRC1 / CRC2

	UIO_data_out = 0;
	
	TRIS_TAG_UIO = 0;

	LAT_TAG_UIO = 1;                                                            // standby pulse
	_delay_ms(2);                                                               // 2 milli seconds
	LAT_TAG_UIO = 0;                                                            // POR pulse
	
	_delay_us(100);                                                             // 100 micro seconds
	
                                                                                // Standby pulse
	
                                                                                // Standby pulse

	UIO_WRITE(0x55);                                                            // start Header
	UIO_MAK();                                                                  // master acknowledge - MAK
	tag_state = UIO_SAK();                                                      // slave acknowledge
    TRIS_TAG_UIO = 0;
	UIO_WRITE(0xA0);                                                            // slave device address
	UIO_MAK();                                                                  // master acknowledge - MAK
	tag_state = UIO_SAK();                                                      // slave acknowledge

    TRIS_TAG_UIO = 0;
	UIO_WRITE(TAG_Request[0]);                                                  // command
    
	if (TAG_Request[1] == 0x00) 
	{ 
		UIO_noMAK();                                                            // master acknowledge - noMAK
		tag_state = UIO_SAK();                                                  // slave acknowledge
		LAT_TAG_UIO = 0;                                                        // clear line
		return 0;
	}
	else
	{
		UIO_MAK();                                                              // master acknowledge - MAK
		tag_state = UIO_SAK();                                                  // slave acknowledge
        TRIS_TAG_UIO = 0;
	}

	if (TAG_Request[3] == 0x01)
	{
		UIO_WRITE(TAG_Request[4]);                                              // Word Address MSB
		UIO_MAK();                                                              // Master Acknowledge - MAK
		tag_state = UIO_SAK();                                                  // Slave Acknowledge
        TRIS_TAG_UIO = 0;

		UIO_WRITE(TAG_Request[5]);                                              // Word Address LSB
		UIO_MAK();                                                              // Master Acknowledge - MAK
		tag_state = UIO_SAK();                                                  // Slave Acknowledge

	}


       
    
	if (TAG_Request[6] == 0x00)                                                 // READ
	{
                                                                                // change PORT Direction - INPUT
		TRIS_TAG_UIO = 1;
        ANSEL_TAG_UIO = 1;
		UIO_data_out = rxWord();
		 
		TRIS_TAG_UIO = 0;                                                       // Change PORT Direction - OUTPUT
	}
	else                                                                        // WRITE
	{
		UIO_WRITE(TAG_Request[2]);
	}
    TRIS_TAG_UIO = 0;
	UIO_noMAK();                                                                // master acknowledge - noMAK
	tag_state = UIO_SAK();                                                      // slave acknowledge
	_delay_ms(1);                                                               // 1 milli seconds - wait write cycle time delay

	return UIO_data_out;
}


unsigned int SCAN_TAG_1(void)
{
	// HEADER  / ID1 / ID2 / ID3 / ID4 / CRC1 / CRC2
	// AA / 55 / ID1 / ID2 / ID3 / ID4 / CRC1 / CRC2

	UIO_data_out = 0;
	
	TRIS_TAG_UIO_1 = 0;

	LAT_TAG_UIO_1 = 1;                                                          // standby pulse
	_delay_ms(2);                                                               // 2 milli seconds
	LAT_TAG_UIO_1 = 0;                                                          // POR pulse
	
	_delay_us(100);                                                             // 100 micro seconds
	
	// Standby pulse
	
	// Standby pulse

	UIO_WRITE_1(0x55);                                                          // start Header
	UIO_MAK_1();                                                                // master acknowledge - MAK
	tag_state = UIO_SAK_1();                                                    // slave acknowledge
    TRIS_TAG_UIO_1 = 0;
	UIO_WRITE_1(0xA0);                                                          // slave device address
	UIO_MAK_1();                                                                // master acknowledge - MAK
	tag_state = UIO_SAK_1();                                                    // slave acknowledge

    TRIS_TAG_UIO_1 = 0;
	UIO_WRITE_1(TAG_Request[0]);                                                // command
    
	if (TAG_Request[1] == 0x00) 
	{ 
		UIO_noMAK_1();                                                          // master acknowledge - noMAK
		tag_state = UIO_SAK_1();                                                // slave acknowledge
		LAT_TAG_UIO_1 = 0;                                                      // clear line
		return 0;
	}
	else
	{
		UIO_MAK_1();                                                            // master acknowledge - MAK
		tag_state = UIO_SAK_1();                                                // slave acknowledge
        TRIS_TAG_UIO_1 = 0;
	}

	if (TAG_Request[3] == 0x01)
	{
		UIO_WRITE_1(TAG_Request[4]);                                            // Word Address MSB
		UIO_MAK_1();                                                            // Master Acknowledge - MAK
		tag_state = UIO_SAK_1();                                                // Slave Acknowledge
        TRIS_TAG_UIO_1 = 0;

		UIO_WRITE_1(TAG_Request[5]);                                            // Word Address LSB
		UIO_MAK_1();                                                            // Master Acknowledge - MAK
		tag_state = UIO_SAK_1();                                                // Slave Acknowledge

	}


       
    
	if (TAG_Request[6] == 0x00)                                                 // READ
	{
                                                                                // change PORT Direction - INPUT
		TRIS_TAG_UIO_1 = 1;
        ANSEL_TAG_UIO_1 = 1;
        UIO_data_out = rxWord();
        TRIS_TAG_UIO_1 = 0;                                                     // Change PORT Direction - OUTPUT
	}
	else                                                                        // WRITE
	{
		UIO_WRITE_1(TAG_Request[2]);
	}
    TRIS_TAG_UIO_1 = 0;
	UIO_noMAK_1();                                                              // master acknowledge - noMAK
	tag_state = UIO_SAK_1();                                                    // slave acknowledge
	_delay_ms(1);                                                               // 1 milli seconds - wait write cycle time delay

	return UIO_data_out;
}
//---------------------------------------------------------------------------------------------------------     
// This routine write 8-bit of data to the UIO bus
//---------------------------------------------------------------------------------------------------------    
 
void UIO_WRITE(unsigned char cmd)
{
	unsigned char uio_data;
	unsigned char nDigit;
	uio_data = cmd;
	
	for	( nDigit=8; nDigit>0; nDigit--)                                         //01010101
	{                                                                           //10000000
		switch ( (uio_data & 0x80) )
		{
			case 0:
			{
				LAT_TAG_UIO = 1;
				_delay_us(50);                                                  // 50 us delay
				LAT_TAG_UIO = 0;
				_delay_us(50);                                                  // 50 us delay
				break;
			} 
			default:
			{
				LAT_TAG_UIO = 0;
				_delay_us(50);                                                  // 50 us delay				
				LAT_TAG_UIO = 1 ;
				_delay_us(50);                                                  // 50 us delay				
				break;
			}
		}
		uio_data = (uio_data << 1);
	}
}

void UIO_WRITE_1(unsigned char cmd)
{
	unsigned char uio_data;
	unsigned char nDigit;
	uio_data = cmd;
	
	for	( nDigit=8; nDigit>0; nDigit--)                                         //01010101
	{                                                                           //10000000
		switch ( (uio_data & 0x80) )
		{
			case 0:
			{
				LAT_TAG_UIO_1 = 1;
				_delay_us(50);                                                  // 50 us delay
				LAT_TAG_UIO_1 = 0;
				_delay_us(50);                                                  // 50 us delay
				break;
			} 
			default:
			{
				LAT_TAG_UIO_1 = 0;
				_delay_us(50);                                                  // 50 us delay				
				LAT_TAG_UIO_1 = 1 ;
				_delay_us(50);                                                  // 50 us delay				
				break;
			}
		}
		uio_data = (uio_data << 1);
	}
}

//-------------------------------------------------------------------------------------------------     
// This routine reads the Slave Acknowledge
//-------------------------------------------------------------------------------------------------
   
unsigned char UIO_SAK(void)                                                     // slave Acknowledge 
{
	unsigned char SAK = 0;

	TRIS_TAG_UIO = 1;                                                           // Change PORT Direction - INPUT
	_delay_us(25);                                                              // 25 micro seconds	- wait halve bit
	
	if (PORT_TAG_UIO)
        SAK |= 0x02;
	else
        SAK &= ~0x02;
		
	_delay_us(50);                                                              // 100 us delay	- wait full bit
	
	if (PORT_TAG_UIO)
        SAK |= 0x01;
	else							
        SAK &= ~0x01;
		
	_delay_us(25);                                                              // 25 micro seconds	- wait halve bit
	return SAK;
}

unsigned char UIO_SAK_1(void)                                                   // slave Acknowledge 
{
	unsigned char SAK = 0;

	TRIS_TAG_UIO_1 = 1;                                                         // Change PORT Direction - INPUT
	_delay_us(25);                                                              // 25 micro seconds	- wait halve bit
	
	if (PORT_TAG_UIO_1) { SAK |= 0x02; }
	else							{ SAK &= ~0x02;}
		
	_delay_us(50);                                                              // 100 us delay	- wait full bit
	
	if (PORT_TAG_UIO_1)	{ SAK |= 0x01; }
	else							{ SAK &= ~0x01;}
		
	_delay_us(25);                                                              // 25 micro seconds	- wait halve bit
	return SAK;
}

//---------------------------------------------------------------------------------------------------     
// This routine writes the Master Acknowledgment
//---------------------------------------------------------------------------------------------------     

void UIO_MAK(void)                                                              // Master Acknowledge - MAK
{
	LAT_TAG_UIO = 0;
	_delay_us(50);                                                              // 50 us delay
	LAT_TAG_UIO = 1;
	_delay_us(50);                                                              // 50 us delay
}

void UIO_MAK_1(void)                                                            // Master Acknowledge - MAK
{
	LAT_TAG_UIO_1 = 0;
	_delay_us(50);                                                              // 50 us delay
	LAT_TAG_UIO_1 = 1;
	_delay_us(50);                                                              // 50 us delay
}

//-----------------------------------------------------------------------------------------------------      
// This routine writes the No Master Acknowledgment
//-----------------------------------------------------------------------------------------------------  
    
void UIO_noMAK(void)                                                            // Master Acknowledge - noMAK
{
	LAT_TAG_UIO = 1;
	_delay_us(50);                                                              // 50 us delay
	LAT_TAG_UIO = 0;
	_delay_us(50);                                                              // 50 us delay
}

void UIO_noMAK_1(void)                                                          // Master Acknowledge - noMAK
{
	LAT_TAG_UIO_1 = 1;
	_delay_us(50);                                                              // 50 us delay
	LAT_TAG_UIO_1 = 0;
	_delay_us(50);                                                              // 50 us delay
}

unsigned char CHECK_CRC16(void)
{
    unsigned short crc;
    TAG_Response[0] = 0XAA;
    crc = calcrc(TAG_Response, 6);                                              // compute CRC and validate it
    if ((TAG_Response[6] == (unsigned char)(crc >> 8)) && (TAG_Response[7] == (unsigned char)(crc)))
    {	return 1; }                                                             // CRC is valid for the frame
	return 0;
}

//calculates the CRC from a pointer to an array of data based on the length of 
//data argument "count". returns a integer crc
unsigned int calcrc(unsigned char *ptr, int count){
    unsigned int crc;
    unsigned char i;

    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}

//receives a word response from the Tag and and returns it as unsigned char
unsigned char rxWord(void){
    unsigned char watch, eddReadState, halfBits, word;                          //initialise the context variables
    watch = 0;                                                                  //set the watch to 0
    halfBits = 0;                                                               //clear the number of half bits
    word = 0;                                                                   //clear the word
    eddReadState = 1;                                                           //initially set the line to high 
    while(halfBits < 16){                                                       //while there have been less than 16 half bits
        watch = 0;                                                              //clear the watch
        if(eddReadState){                                                       //if the line is high
            while(!detectFallingEdge() && watch < halfBitPeriod){               //wait for a falling edge or for a half bit
                watch++;                                                        //increment the watch
            }
            if(watch >= halfBitPeriod){                                         //if the watch is greater than a half bit
                eddReadState = 1;                                               //then the edd state remains high as there has not been an edge
                _delay_us(5);                                                   //small delay waiting for the next edge
            }
                
            else{
                eddReadState = 0;                                               //otherwise there has been a falling edge and the line is now low
            }
        }
        else{                                                                   //the line is low
            while(!detectRisingEdge() && watch < halfBitPeriod){                //wait for a rising edge or for a half bit timeout
                watch++;                                                        //increment the watch counter
            }
            if(watch >= halfBitPeriod){                                         //if the watch is greater than a half bit
                eddReadState = 0;                                               //there hasn't been an edge so the line remains low
                _delay_us(5);                                                   //small delay waiting for the next edge
            }
            else{
                eddReadState = 1;                                               //otherwise and edge has been detected and the line is now high
            }
        }
        if(halfBits % 2 == 0){                                                  //every bit
                word = word << 1;                                               //shift the word 1  bit left
                word += !eddReadState;                                          //add the inverse of the state
        }
        halfBits++;                                                             //half bit has occurred
    }
    return word;                                                                //return the result
}

//returns a 1 if a there has been low value
unsigned char detectFallingEdge(void){
    unsigned short currentTagADC = readTagADC();                                //read the value and store it
    if(currentTagADC < 200)                                                     //if it is less than 200
        return 1;                                                               //return true
     return 0;                                                                  //otherwise return false
}

//returns a 1 if there has been a high value
unsigned char detectRisingEdge(void){
    unsigned short currentTagADC = readTagADC();                                //read the value and store                                
    if(currentTagADC > 700)                                                     //if it is greater than 700
        return 1;                                                               //return true
     return 0;                                                                  //return false
}

//reads a value from the ADC
unsigned short readTagADC(void){ 
    ADRES = 0;                                                                  //clear ADRES
    ADCON0bits.ADGO = 1;                                                        // start ADC conversion
    while (ADCON0bits.ADGO);                                                    // wait for conversion to finish
    return ADRES;                                                               //return the result
}

//initialise the tag ADC for standard reads
void initialiseTagADC(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    while(!FVRCONbits.FVRRDY);                                                  //wait for the FVR to be ready
    
    ADCON1 = 0b11110000;                                                        //standard ADC 
    ADPCH = tagADCChannel;                                                      //ADC channel
    ADCLK = 0b00010000;                                                         //slow conversion to simulate 10us delay
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        // Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
    _delay_us(20);                                                              //give it time to initialise
}

//initialise the tag ADC for reverse reads
void initialiseTagADC_1(void){
    FVRCON = 0b00000010;                                                        //set FVR comparator off and FVR ADC to 2.048V            
    FVRCONbits.FVREN = 1;                                                       //enable the FVR
    while(!FVRCONbits.FVRRDY);                                                  //wait for the FVR to be ready
    
    ADCON1 = 0b11110000;                                                        //standard ADC 
    ADPCH = tagADCChannel_1;                                                      //ADC channel
    ADCLK = 0b00010000;                                                         //slow conversion to simulate 10us delay
    ADREF = 0b00000011;                                                         //standard reference
    ADCON0bits.ADFM = 1;                                                        // Right Justified
    ADCON0bits.ADON = 1;                                                        //turn the ADC on
    _delay_us(20);                                                              //give it time to initialise
}

unsigned char READ_TAG(void){
    TAG_Request[0] = 0x03;			// command
	TAG_Request[1] = 0x01;			// position
	TAG_Request[2] = 0x00;			// data
	TAG_Request[3] = 0x01;			// memory
	TAG_Request[4] = 0x00;			// MSB
	TAG_Request[5] = 0x03;			// LSB
	TAG_Request[6] = 0x00;			// RW
	initialiseTagADC();
    ANSEL_TAG_UIO = 1;
    TAG_Response[0] = SCAN_TAG();
    for(int i = 0; i < 8; i ++){
        TAG_Request[5] = i + 1;
        TAG_Response[i] = SCAN_TAG();                                           // Read DATA from MEMORY
    }
    if (CHECK_CRC16()){
            return 1;
    }
    else
        return 0;
}

unsigned char READ_TAG_REVERSE(void){
    TAG_Request[0] = 0x03;			// command
	TAG_Request[1] = 0x01;			// position
	TAG_Request[2] = 0x00;			// data
	TAG_Request[3] = 0x01;			// memory
	TAG_Request[4] = 0x00;			// MSB
	TAG_Request[5] = 0x01;			// LSB
	TAG_Request[6] = 0x00;			// RW
	initialiseTagADC_1();
    ANSEL_TAG_UIO_1 = 1;
    TAG_Response[0] = SCAN_TAG_1();
    for(int i = 0; i < 8; i ++){
        TAG_Request[5] = i + 1;
        TAG_Response[i] = SCAN_TAG_1();                                         // Read DATA from MEMORY
    }
    if (CHECK_CRC16()){
            return 1;
    }
    else
        return 0;
}

void readTagRoutine(void){
    tagInterrupt = 0;
    sec4Enable = 0;
    if(ABB_1.info.statusBits.key_switch_status)                                 //if the key switch is armed then
        return;                                                                 //return and do not allow tagging
    unsigned char attempts = 0;                                                 //otherwise initialise the attempts to 0
    FLAGS.sec2 = 0;                                                             //clear the 3 second Flag
    COUNTERS.sec2 = secs2;                                                      //load the 3 second counter
    setBlueLed();                                                               //set the blue LED to show that a tag is in process
    tagFor0();                                                                  //prepare the pins for forward scanning
    while(!READ_TAG() && attempts < tagAttempts)                                //while there have been less than four unsuccessful attempts 
        attempts ++;                                                            //increment the number of attempts
    if(attempts < tagAttempts){                                                 //if there was a successful tag
        addNewTag();                                                            //add it to memory
        tagForInterrupt();                                                      //prepare the pins for another tag
        return;                                                                 //leave the routine
    }
    attempts = 0;                                                               //reset the number of attempts
    tagFor1();                                                                  //prepare the pins for reverse scanning
    while(!READ_TAG_REVERSE() && attempts < tagAttempts)                        //while there have been less than four unsuccessful attempts
        attempts ++;                                                            //increment the number of attempts
    if(attempts < tagAttempts){                                                 //if there was a successful tag
        addNewTag();                                                            //add it to memory
        tagForInterrupt();                                                      //prepare the pins for another tag
        return;                                                                 //leave the routine
    }
    tagForInterrupt();                                                          //prepare the pins for another tag
    state.led.next = tagRed;                                                    //show that tagging was unsuccessful
    
}

//prepares the pins for forward scanning
void tagFor0(void){
    TRIS_TAG_UIO = 0;                                                           //set the pin as an output
    TRIS_TAG_UIO_1 = 0;                                                         //set the pin as an output
    WPU_TAG_UIO = 0;                                                            //disable the weak pull up
    WPU_TAG_UIO_1 = 0;                                                          //disable the weak pull up
    
    LAT_TAG_UIO_1 = 0;                                                          //set the pin low
    LAT_TAG_UIO = 1;                                                            //set the pin high
    _delay_ms(2);                                                               //wait a few ms for the tag to charge
}

void tagFor1(void){
    TRIS_TAG_UIO = 0;                                                           //set the pin as an output
    TRIS_TAG_UIO_1 = 0;                                                         //set the pin as an output
    WPU_TAG_UIO = 0;                                                            //disable the weak pull up
    WPU_TAG_UIO_1 = 0;                                                          //disable the weak pull up
    
    LAT_TAG_UIO_1 = 1;                                                          //set the pin high
    LAT_TAG_UIO = 0;                                                            //set the pin low
    _delay_ms(2);                                                               //wait a few ms for the tag to charge
}

//checks to see if the tag has been removed by reading the port after the weak
// pull up is enabled
unsigned char checkIfTagIsRemoved(void){                                        
    return PORT_TAG_UIO;                                                        //if the port is high then the tag has been removed
}