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



#include "main.h"					//Global data type definitions (see https://github.com/ibexuk/C_Generic_Header_File )
#define	DMX_TX_C					//(Our header file define)
#include "dmx-tx.h"





//************************************
//************************************
//********** INITIALISE DMX **********
//************************************
//************************************
void dmx_tx_initalise (void)
{
	WORD count;
	
	//----- CLEAR BUFFER -----
	for (count = 0; count < DMX_NO_OF_CHANS_TO_TX; count++)
	{
		dmx_tx_buffer[count] = 0;
	}

	//----- SETUP TX -----
	dmx_tx_state = DMX_TX_BREAK;
	dmx_transmit();			//Call the irq function

}


//************************************
//************************************
//********** PROCESS DMX TX **********
//************************************
//************************************
void dmx_transmit (void)
{
	static BYTE do_again;			//static as it can help to avoid the irq use the general ram stack

	//--------------------------
	//--------------------------
	//----- CHECK DMX 1 TX -----
	//--------------------------
	//--------------------------

	do_again = 1;
	while (do_again)
	{
		do_again = 0;

		switch (dmx_tx_state)
		{
		case DMX_TX_WAIT_MAB:
			//------------------------
			//----- WAIT FOR MAB -----
			//------------------------

			//Wait for MAB to complete (stall execution for 8uS as its faster then exiting IRQ and coming back into it)
			while (DMX_TX_TIMER_IRQ_FLAG_BIT_READ == 0)
				;

			//Disable timer irq
			DMX_TX_TIMER_IRQ_ENABLE_BIT(0);

			//----- SEND DMX HEADER -----
			DMX_TX_UART_IRQ_FLAG_BIT(0);
			DMX_TX_DATA_REGISTER = 0x00;		//TX header start code 0x00
			DMX_TX_UART_IRQ_ENABLE_BIT(1);

			dmx_tx_count = 0;
			DMX_TX_BIT9(1);				//Set bit 9 high (2 stop bits)
			dmx_tx_state = DMX_TX_DATA;
			break;

		case DMX_TX_DATA:
			//-------------------------
			//----- SEND DMX DATA -----
			//-------------------------

			//Send next byte
			DMX_TX_BIT9(1);				//Set bit 9 high (2 stop bits)
			DMX_TX_DATA_REGISTER = dmx_tx_buffer[dmx_tx_count];
			dmx_tx_count++;

			//All done?
			if (dmx_tx_count >= DMX_NO_OF_CHANS_TO_TX)
				dmx_tx_state = DMX_TX_WAIT_FOR_LAST;

			break;


		case DMX_TX_WAIT_FOR_LAST:
			//------------------------------------
			//----- WAIT FOR LAST BYTE TO TX -----
			//------------------------------------
			//Last byte will now be transmitting assuming a single byte UART buffer.
			//Wait to allow it to have been sent and the stop bit of the byte before (IRQ will have triggered as previous byte got to stop bit being sent)

			//Disable TX irq
			DMX_TX_UART_IRQ_ENABLE_BIT(0);

			//Setup timer
			DMX_TX_SETUP_TIMER_52US;
			DMX_TX_TIMER_IRQ_FLAG_BIT(0);
			DMX_TX_TIMER_IRQ_ENABLE_BIT(1);

			dmx_tx_state = DMX_TX_BREAK;
			break;


		case DMX_TX_BREAK:
			//----------------------
			//----- SEND BREAK -----
			//----------------------
			//Continuous low signal for at least 88uS (two complete frames).  Recomended > 130uS to allow for equipment not properly compliant.

			//Ensure last byte has gone
			while (DMX_TX_TRMT_BIT == 0)
				;


			//START BREAK (min 130uS)
			DMX_TX_SPEN_BIT(0);				//Disable the UART to allow pins to be controlled
			DMX_TX_PIN_TRIS(0);				//Low for break
			DMX_TX_PIN_LAT(0);

			//Setup timer
			DMX_TX_SETUP_TIMER_130US;
			DMX_TX_TIMER_IRQ_FLAG_BIT(0);
			DMX_TX_TIMER_IRQ_ENABLE_BIT(1);

			dmx_tx_state = DMX_TX_MAB;
			break;

		case DMX_TX_MAB:
			//--------------------
			//----- SEND MAB -----
			//--------------------
			//Minimum 8uS, maximum 100 to 200uS - some devices are sensitive to too long break time.  If your PIC18 is slow running from C you might want to jsut stall execution here instead and jump to DMX_TX_WAIT_MAB

			DMX_TX_SPEN_BIT(1);			//Enable the UART

			//Setup timer
			DMX_TX_SETUP_TIMER_8US;
			DMX_TX_TIMER_IRQ_FLAG_BIT(0);
			DMX_TX_TIMER_IRQ_ENABLE_BIT(0);  //Not using IRQ as usually for a PIC18 it will take > 8uS to exit the irq and come back into it with the C compiler overhead. Better to just stall

			dmx_tx_state = DMX_TX_WAIT_MAB;
			do_again = 1;			//Loop back
			break;



		} //switch (dmx_tx_state)

	} //while (do_again)







}











