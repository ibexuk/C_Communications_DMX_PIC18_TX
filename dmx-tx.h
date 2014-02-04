
/*
Provided by IBEX UK LTD http://www.ibexuk.com
Electronic Product Design Specialists
RELEASED SOFTWARE

The MIT License (MIT)

Copyright (c) 2013, IBEX UK Ltd, http://ibexuk.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
//Visit http://www.embedded-code.com/source-code/communications/dmx512/pic18-dmx-transmit-driver for more information
//
//Project Name:	PIC18 DMX TX



/*

This code uses interrupts to TX as fast as possible.
A timer is also used to provide the timing of the DMX MAB and BREAK.
In order to generate the BREAK the UART is disabled and the TX pin driven low. If you want to keep RX active then add a pull down resistor to your TX pin and mod
this code so that instead of diabling the UART for the BREAK it instead disables TX and sets the pin to be an input (mays PIC18's can't set the pin to output with
the UART enabled).
Note that DMX RX is very fast for many PIC18 devices and care shoudl be taken to ensure that the interrupt handler will respond quickly enough as bytes arrive. You
should test using a DMX source transmitting at the fastest possible speed (many DMX sources do not).


//TO USE:

//##### INITIALISE #####

	//----- SETUP TIMER 1 FOR DMX TX -----
	IPR1bits.TMR1IP = 1;				//TMR1 interrupt at high priority
	T1CON = 0b10000001;					//Timer 1 on, internal, 1:1 prescale, R/W 16bits in 1 operation <<<<<CHECK THIS VALUE IS RIGHT FOR YOUR PIC18 DEVICE

	//----- SETUP UART1 FOR DMX OUT -----
	TXSTA1 = 0b01100100;				//TX enabled, 9bit,  BRGH on (on=b2 high)
	RCSTA1 = 0b11010000;				//Serial port enabled, rx enabled, 9 bit, addr detect off
	BAUDCON1 = 0b00000000;				//BRG16 disabled
	//SPBRGH1 = (9 >> 8);
	SPBRG1 = 9;							//Set baud rate to 250k

	//----- INITIALISE DMX TX -----
	dmx_tx_initalise();

	//----- ENABLE INTERRUPTS -----
	INTCONbits.PEIE = 1;					//Enable peripheral interrupts
	ENABLE_INT;



//##### INTERRUPT HANDER #####
	if(PIE1bits.TX1IE && PIR1bits.TX1IF)		//TX irq
		dmx_transmit();

	if(PIE1bits.TMR1IE && PIR1bits.TMR1IF)		//Timer irq
		dmx_transmit();

*/


#define DMX_NO_OF_CHANS_TO_TX				512								//512 is standard, but you can TX as few as 24 channels

//*****************************
//*****************************
//********** DEFINES **********
//*****************************
//*****************************
#ifndef DMX_TX_C_INIT		//(Do only once)
#define	DMX_TX_C_INIT


#define	DMX_TX_TIMER_IRQ_ENABLE_BIT(state)	PIE1bits.TMR1IE = state
#define	DMX_TX_TIMER_IRQ_FLAG_BIT(state)	PIR1bits.TMR1IF = state
#define	DMX_TX_TIMER_IRQ_FLAG_BIT_READ		PIR1bits.TMR1IF
#define DMX_TX_SETUP_TIMER_130US			TMR1H = 0xfa; TMR1L = 0xeb		//Set timer to trigger in 130uS based on your osc speed etc (0xffff - 130uS) <<<<< SET FOR YOUR OSC SPEED
#define	DMX_TX_SETUP_TIMER_52US				TMR1H = 0xfd; TMR1L = 0xf7		//Set timer to trigger in 52uS based on your osc speed etc (0xffff - 52uS) <<<<< SET FOR YOUR OSC SPEED
#define DMX_TX_SETUP_TIMER_8US				TMR1H = 0xff; TMR1L = 0xaf		//Set timer to trigger in 8uS based on your osc speed etc (0xffff - 8uS) <<<<< SET FOR YOUR OSC SPEED

#define	DMX_TX_UART_IRQ_ENABLE_BIT(state)	PIE1bits.TX1IE = state
#define	DMX_TX_UART_IRQ_FLAG_BIT(state)		PIR1bits.TX1IF = state
#define	DMX_TX_DATA_REGISTER				TXREG1
#define	DMX_TX_BIT9(state)					TXSTA1bits.TX9D = state
#define	DMX_TX_TRMT_BIT						TXSTA1bits.TRMT
#define	DMX_TX_SPEN_BIT(state)				RCSTA1bits.SPEN = state

#define DMX_TX_PIN_TRIS(state)				TRISCbits.TRISC6 = state
#define DMX_TX_PIN_LAT(state)				LATCbits.LATC6 = state



typedef enum _DMX_TX_SM
{
	DMX_TX_WAIT_MAB,
	DMX_TX_DATA,
	DMX_TX_WAIT_FOR_LAST,
	DMX_TX_BREAK,
	DMX_TX_MAB
} DMX_TX_SM;


#endif



//*******************************
//*******************************
//********** FUNCTIONS **********
//*******************************
//*******************************
#ifdef DMX_TX_C
//-----------------------------------
//----- INTERNAL ONLY FUNCTIONS -----
//-----------------------------------


//-----------------------------------------
//----- INTERNAL & EXTERNAL FUNCTIONS -----
//-----------------------------------------
//(Also defined below as extern)
void dmx_tx_initalise (void);
void dmx_transmit (void);


#else
//------------------------------
//----- EXTERNAL FUNCTIONS -----
//------------------------------
extern void dmx_tx_initalise (void);
extern void dmx_transmit (void);


#endif




//****************************
//****************************
//********** MEMORY **********
//****************************
//****************************
#ifdef DMX_TX_C
//--------------------------------------------
//----- INTERNAL ONLY MEMORY DEFINITIONS -----
//--------------------------------------------
BYTE dmx_tx_state;
WORD dmx_tx_count;


//--------------------------------------------------
//----- INTERNAL & EXTERNAL MEMORY DEFINITIONS -----
//--------------------------------------------------
//(Also defined below as extern)



#pragma udata dmx_512_byte_ram_section				//This is the PIC C18 compiler command to use the specially defined section in the linker script (uses a modified linker script for this)
BYTE dmx_tx_buffer[DMX_NO_OF_CHANS_TO_TX];			//(C18 large array requirement to use a special big section of ram defined in the linker script)
#pragma udata
//If you are transmitting < 256 channels then you won't need the large memory defintion in the linker script and the #pragma's can be removed
//Example of definition in the linker script DATABANK NAME=dmx_512_byte_ram_section START=0x700 END=0x8FF
//See here for how to create this: http://www.electronics-design.net/embedded-programming/microchip-pic/pic18/c18-compiler/memory-2

#else
//---------------------------------------
//----- EXTERNAL MEMORY DEFINITIONS -----
//---------------------------------------


#pragma udata dmx_512_byte_ram_section				//This is the PIC C18 compiler command to use the specially defined section in the linker script (uses a modified linker script for this)
extern BYTE dmx_tx_buffer[DMX_NO_OF_CHANS_TO_TX];			//(C18 large array requirement to use a special big section of ram defined in the linker script)
#pragma udata

#endif











