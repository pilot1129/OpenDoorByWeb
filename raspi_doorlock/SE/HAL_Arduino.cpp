/*****************************************************************************
 * Serial port communication library for Arduino
 * --------------------------------------------------------------------
 * THE SOURCE CODE AND ITS RELATED DOCUMENTATION IS PROVIDED "AS IS". INFINEON
 * TECHNOLOGIES MAKES NO OTHER WARRANTY OF ANY KIND,WHETHER EXPRESS,IMPLIED OR,
 * STATUTORY AND DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * SATISFACTORY QUALITY, NON INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * THE SOURCE CODE AND DOCUMENTATION MAY INCLUDE ERRORS. INFINEON TECHNOLOGIES
 * RESERVES THE RIGHT TO INCORPORATE MODIFICATIONS TO THE SOURCE CODE IN LATER
 * REVISIONS OF IT, AND TO MAKE IMPROVEMENTS OR CHANGES IN THE DOCUMENTATION OR
 * THE PRODUCTS OR TECHNOLOGIES DESCRIBED THEREIN AT ANY TIME.
 *
 * INFINEON TECHNOLOGIES SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGE OR LIABILITY ARISING FROM YOUR USE OF THE SOURCE CODE OR
 * ANY DOCUMENTATION, INCLUDING BUT NOT LIMITED TO, LOST REVENUES, DATA OR
 * PROFITS, DAMAGES OF ANY SPECIAL, INCIDENTAL OR CONSEQUENTIAL NATURE, PUNITIVE
 * DAMAGES, LOSS OF PROPERTY OR LOSS OF PROFITS ARISING OUT OF OR IN CONNECTION
 * WITH THIS AGREEMENT, OR BEING UNUSABLE, EVEN IF ADVISED OF THE POSSIBILITY OR
 * PROBABILITY OF SUCH DAMAGES AND WHETHER A CLAIM FOR SUCH DAMAGE IS BASED UPON
 * WARRANTY, CONTRACT, TORT, NEGLIGENCE OR OTHERWISE.
 *
 * (C)Copyright INFINEON TECHNOLOGIES All rights reserved
 ******************************************************************************
   Redistribution and use of this file in source and binary forms, with
   or without modification, are permitted.
   --------------------------------------------------------------------
  Revision History:
    Created: Juri Cizas  11/14/2003
 *****************************************************************************/
//#include "Arduino.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdio.h>
#include "ISO7816.h"
#if 0
#include "Utils.h"
#endif
#include <unistd.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

extern int Serial3;
static int _resetpin = 9;
int testcount = 0;
//###########################################################################
// System Timer functions (used for delays and timeouts)
//###########################################################################
/************************************************
	 Wait for a specified time in milliseconds
 ************************************************/
void HAL_Sleep(int time)
{
    delay(time);
}
/*-------------------------------------------------------------------------------
             Get high resolution counter/timer in miliseconds
    Uses high-resolution performance counter if it is available on the system
  -------------------------------------------------------------------------------*/
LONGLONG HAL_GetTimerValue(void)
{
    return millis();
}
//###########################################################################
// Console Input/Output functions (Control and debug)
//###########################################################################
/************************************************
  Console output to Windows console
 ************************************************/
void HAL_Host_Write(BYTE *bt, int len)
{
    int i;
    for (i = 0; len > 0; len--) printf("%c", (char)bt[i++]);
}

//#define _getch Serial.read
//#define _kbhit Serial.available
/************************************************
  Input from XMC4xxx USB-UART - wait for a byte
  Return the value of byte received
 ************************************************/
/*
short HAL_Host_Wait_Input()
{
    char c = _getch();
    if ((c & 0xFF) != 0xE0) return toupper(c);     // Check if it is an extended key
    return (0xE0 << 8 | toupper(_getch()));
}*/
/************************************************
 Check if any key is pressed on PC keyboard. 
 Returns 0 if key not pressed or upper case of pressed key otherwise.
 ************************************************/
/*
short HAL_Host_Get_Input()
{
    if (_kbhit()) return HAL_Host_Wait_Input();
    else return 0;
}*/
/************************************************
  HAL: Activate/deactivate LED.
  Input: num: I/O pin (LED) number (1, 2)
         onoff: 1 - LED on, 0 - LED off
 ************************************************/
void HAL_LED(int num, int onoff)
{
// Not available for this configuration
}
//###########################################################################
// External Clock Output functions (ISO7816 clock generator - from 2 to 7 MHz)
//###########################################################################

/************************************************
  Set ISO7816 CLK frequency - External clock's division factor
 ************************************************/
static void ISO7816_SetCLK(int clkfreq)
{
// Not available for this configuration
}
/************************************************
  Set UART's baud rate generator's division factor
 ************************************************/
static void ISO7816_SetBitRate(int bitrate)
{
// Not stable for this configuration
   //Serial.begin(bitrate, SERIAL_8E2);
}
/**************************************************************************
	Set ISO7816 CLK frequency and UART Bit rate
 **************************************************************************/
void HAL_SetFreqAndBitRate(int clkfreq, int bitrate)
{
	ISO7816_SetCLK(clkfreq);
	ISO7816_SetBitRate(bitrate);
}
/**************************************************************************
	Set ISO7816 UART Bit rate using division factor 372 or 31
 **************************************************************************/
void HAL_SetBitRate(int division_factor)
{
	if (division_factor == ISO7816_FAST_DIVISION_FACTOR) {	// DF=31
//		ISO7816_SetCLK(...);     	// 3.579 MHz
		ISO7816_SetBitRate(115200); 
	}
	else {							// DF=372
//		ISO7816_SetCLK(...);     	// 3.579 MHz
		ISO7816_SetBitRate(9600);
	}
	HAL_Purge_UART();               // purge any information in the buffer
}
//###########################################################################
// ISO7816 RST signal control functions (RESET)
//###########################################################################

/////////////////////////////////////////////////////////////////////////////
//    HAL: Activate (low level) the RST line of ISO7816 interface
/////////////////////////////////////////////////////////////////////////////
void HAL_RST_On(void)
{
#if 0
    #ifdef COM_PORT_CONVERTER		           // Direct COM port connection using RTS as RST
	EscapeCommFunction(COMHandle, SETRTS); // Set RTS=1 : ISO7816-RST = 0
    #endif
    #ifdef IFX_TRANSPARENT_READER	           // IFX transparent reader using modem signal DTR as RST
	EscapeCommFunction(COMHandle, CLRDTR); // Set DTR=0 : ISO7816-RST = 0
    #endif
#endif
    digitalWrite(_resetpin, LOW);
}
/////////////////////////////////////////////////////////////////////////////
//    HAL: Dectivate (high level) the RST line of ISO7816 interface
/////////////////////////////////////////////////////////////////////////////
void HAL_RST_Off(void)
{
#if 0
    #ifdef COM_PORT_CONVERTER		           // Direct COM port connection using RTS as RST
	EscapeCommFunction(COMHandle, CLRRTS); // Set RTS=0 : ISO7816-RST = 1
    #endif
    #ifdef IFX_TRANSPARENT_READER	           // IFX transparent reader using modem signal DTR as RST
	EscapeCommFunction(COMHandle, SETDTR); // Set DTR=1 : ISO7816-RST = 1
    #endif
#endif
    digitalWrite(_resetpin, HIGH);
}

//###########################################################################
// ISO7816 UART functions (send, receive packet)
//###########################################################################

/************************************************
     Initialize hardware at power up
 ************************************************/
void HAL_Initialize(int resetpin)
{
    _resetpin = resetpin;
    pinMode(_resetpin, OUTPUT);
}
/////////////////////////////////////////////////////////////////////////////
static char *getSysErrorMessage(void)
{
    return "ERROR";
}
/////////////////////////////////////////////////////////////////////////////
//                       HAL: Open UART channel
//    Opens the serial port for reading and writing and sets, the 
//    parameters for the serial communication. Before opening the device
//    an existing open device is closed. If an error is reported, the
//    device will be closed.
//    
// Parameters:
//    port        [IN]  requested com port as a string (e.g. "com1")
//
// Return values:
//    TRUE              success
//    FALSE           error
/////////////////////////////////////////////////////////////////////////////
BOOL HAL_Open_UART(char *port)
{
//	return Open_COM(port, 9600, 8, EVENPARITY, TWOSTOPBITS, DTR_CONTROL_DISABLE, RTS_CONTROL_DISABLE);
    return true;
}
/////////////////////////////////////////////////////////////////////////////
//                      Closes an open UART channel
/////////////////////////////////////////////////////////////////////////////
void HAL_Close_UART(void)
{
}
/////////////////////////////////////////////////////////////////////////////
// Function: Set_COM_Timeouts
//    Sets the COM port reading timeout parameters 
//
// Parameters:
//
// Return values:
//    TRUE                  on success
//    FALSE    the device settings are illegal
/////////////////////////////////////////////////////////////////////////////
BOOL HAL_SetTimeout_UART(int ReadTotalTimeoutConstant)
{
    //Serial.setTimeout(ReadTotalTimeoutConstant);
    return true;
}
/////////////////////////////////////////////////////////////////////////////
//              Clear all received data in UART
/////////////////////////////////////////////////////////////////////////////
void HAL_Purge_UART(void)
{
	serialFlush(Serial3);
}

int timedRead()
{
	int c = -1;
	int len;
	int good;
	int _timeout = 1000;
	int _startMillis = millis();
	do {
		if ((good = serialDataAvail(Serial3)) > 0) {
			c = serialGetchar(Serial3);
			if (c >= 0) return c;
		}
	} while (millis() - _startMillis < _timeout);
	return -1;     // -1 indicates timeout
}

int readBytes(BYTE* buffer, int length) {
	int count = 0;
	BYTE ff = 0xff;
	while (count < length) {
		int c = timedRead();
		if (c < 0) break;
		
		*buffer++ = (BYTE)c;
		count++;
	}
	return count;
}

/////////////////////////////////////////////////////////////////////////////
//	                    HAL_WriteBlock_UART
//
//	Description:
//		This function transmits command and receives echo from UART.
//
//	Arguments:
//    pbSendBuffer  - buffer containing the data to be written the UART
//    SendLength  - number of bytes to write to the file
//
//	Return Value:
//		This function returns TRUE or FALSE
/////////////////////////////////////////////////////////////////////////////
BOOL HAL_WriteBlock_UART(BYTE *pbSendBuffer, int SendLength)
{
    BYTE  buffer[APDU_BUFFER_SIZE];
    int   i;
    int NumberOfBytesRead = 0;
	testcount++;
    HAL_Purge_UART();
	
	for (i = 0; i < SendLength; i++)
	{
		serialPutchar(Serial3, pbSendBuffer[i]);
	}
	NumberOfBytesRead = readBytes( buffer, SendLength);
	
    if (NumberOfBytesRead > 0 ) {
		i = 0;
		for (i = 0; i < SendLength; i++)
			if (buffer[i] != pbSendBuffer[i]) {
				return FALSE;
			}
    }
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
//	                    HAL_ReadBlock_UART

//
//	Description:
//		This function reads maxlen bytes from UART
//
//	Arguments:
//    pbRecvBuffer  - buffer that receives the data read from the UART
//
//	Return Value:
//		This function returns 0 or number of bytes received
/////////////////////////////////////////////////////////////////////////////
int HAL_ReadBlock_UART(BYTE *pbRecvBuffer, int maxlen, int *pRecvLength)
{
    int NumberOfBytesRead = 0;
    if (pRecvLength) *pRecvLength = 0;
	
    NumberOfBytesRead = 0;                                                // Try to read the first byte
    NumberOfBytesRead = readBytes(pbRecvBuffer, 1);

    if (NumberOfBytesRead == 0) return 0;                                 // Read timeout
    
	if (maxlen > 1) 
    {
        NumberOfBytesRead = 0;                                            // Try to read more bytes
        NumberOfBytesRead = readBytes(pbRecvBuffer + 1, maxlen - 1); // Ignore potential error
        NumberOfBytesRead++;
    }
    if (pRecvLength) *pRecvLength = NumberOfBytesRead;

    return NumberOfBytesRead;
}
