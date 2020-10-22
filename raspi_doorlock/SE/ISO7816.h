/*****************************************************************************
 * ISO7816-3 library
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
#ifndef ISO7816_H
#define ISO7816_H

#include <wiringPi.h>
#include "HAL.h"
//--------------------------- Constants ----------------------------
#define MAX_C_APDU_SIZE 256
#define MAX_R_APDU_SIZE 256

#define ISO7816_DEF_CLK_FREQUENCY 3143000 // 4125000 //  3110000 // 2590000
#define ISO7816_DEF_DIVISION_FACTOR 372
#define ISO7816_FAST_DIVISION_FACTOR 31

#define ISO7816_RESET_TIME 1000
#define ISO7816_WTX_DELAY 300
#define ISO7816_MAX_WTX_RESPONSES 300

#define ISO7816_AWT 1000
#define ISO7816_BWT 618
#define ISO7816_CWT 309   // 100
#define ISO7816_ATRWT 100 // ATR characters waiting time

#define APDU_CLA   0
#define APDU_INS   1
#define APDU_P1    2
#define APDU_P2    3
#define APDU_LC    4
#define APDU_DATA  5

/*
//--------------------------- Externals ----------------------------
extern BYTE ResponseBuffer[RESP_BUFFER_SIZE];
extern int NumberOfResponseBytes;

extern WORD SW1SW2;
extern WORD expectedSW1SW2;
*/

//--------------------------- Functions ----------------------------

class ISO7816 {

    public:

ISO7816();
ISO7816(int p_reset);
/**************************************************************************
			Initialize UART

    This function opens and initializes UART port if needed. 
    Usually called at program start.

Input Parameters:
    Port   - UART name or number

Return:
    This function returns TRUE in case of success or FALSE otherwise
 **************************************************************************/
BOOL Initialize(char *port);


/**************************************************************************
				Shutdown

    This function closes UART port, usually called when program exits.

Input Parameters:
	none

Return:
	none
 **************************************************************************/
void Shutdown(void);

/**************************************************************************
			Open connection to the chip 
    This function:
	1. performs RESET sequence using ISO7816 RST line
	2. receives Answer To Reset (ATR) from the chip
	3. Performs PPS exchange to set T=1 protocol
	4. Performs IFS exchange to set bit rate and block size

Input Parameters:
	none
Return:
	This function returns TRUE in case of success or FALSE otherwise
 **************************************************************************/
BOOL OpenCard(void);

/**************************************************************************
			Close connection to the chip
Input Parameters:
	none
Return:
	none
 **************************************************************************/
void CloseCard(void);

/**************************************************************************
                         Set UART Timeouts

  Sets the UART reading timeout - max. time to wait for a response packet

 Parameters:
    timeout - in milliseconds
 Return values:
    TRUE     on success
    FALSE    the device settings are illegal
 **************************************************************************/
BOOL SetTimeout(int timeout);

/**************************************************************************
			Reset ISO7816 interface
	This function performs RESET sequence using ISO7816 RST line

Input Parameters:
	none
Return:
	none
 **************************************************************************/
void Reset(void);

/**************************************************************************
	         Transmit APDU, receive response, set SW1SW2 variable
    This function sends/receives one or more APDUs using chaining if needed.

	Arguments:
	    pbInBuf     - buffer containing the APDU
    	lInBufLen   - number of bytes to send
	    pbOutBuf    - pointer to buffer for response
    	plOutBufLen - pointer to int to return response length (must contain response buffer size)

	Return Value:
    This function returns TRUE in case of success or FALSE otherwise
 **************************************************************************/
BOOL TransmitAPDU(BYTE   *pbInBuf,
                          int     lInBufLen,
                          BYTE   *pbOutBuf,
                          int    *plOutBufLen);

/**************************************************************************
	                    Transmit T=1
	Description:
    This function sends/receives one or more T=1 frames using chaining if needed.

	Arguments:
	    pbSendBuffer  - buffer containing the APDU
    	SendLength  - number of bytes to send
	    pbRecvBuffer  - pointer to buffer for response
    	pRecvLength - pointer to int

	Return Value:
    This function returns TRUE in case of success or FALSE otherwise
 **************************************************************************/
BOOL Transmit_T1       (BYTE *pbSendBuffer,
                                int   numBytesToSend,
                                BYTE *pbRecvBuffer,
                                int  *pRecvLength);
    private:

    int _resetpin;
BOOL Transmit_T1_packet(BYTE NAD, 
                              BYTE PCB,
                              BYTE *pbData, 
                              int iDataLength, 
                              BYTE *pbRecvBuffer, 
                              int *pRecvLength);
};
#endif
