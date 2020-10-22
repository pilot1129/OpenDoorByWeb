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
    Modified: Juri Cizas  02/01/2012
 *****************************************************************************/
//#include "Arduino.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include "HAL.h"
#include "ISO7816.h"
#ifdef DEBUGMODE
  #include "Utils.h"
  LONGLONG Time_start;
#endif
  LONGLONG Time_start;

/*------------------------ Global variables -----------------------------*/
BYTE ResponseBuffer[RESP_BUFFER_SIZE];
int NumberOfResponseBytes = 0;

int DataChunkSize = 0;

int Serial3;
WORD SW1SW2;
WORD expectedSW1SW2 = 0x9000;

/*------------------------- Local variables -----------------------------*/
static BYTE Current_PCB = 0;
static BYTE Receive_PCB = 0;
static BYTE T1_IFSC = MAX_FRAME_SIZE;    // 254 or 32-4 = frame size acceptable by the card (including NAD+PCB+LEN+LRC)
static BYTE T1_IFSD = MAX_FRAME_SIZE;    // frame size acceptable by the host
static int T1_NumRetries = 2;


void Log(char *msg) {
	//printf("%s", msg);
}
static int LogLevel=2;
//#include <SoftwareSerial.h>
//extern SoftwareSerial swserial;
void HexDump(char *title, BYTE * buf, int count)
{
	int i;
	//printf("%s", title);
		//Serial.println(title);
		for (i=0; i<count; ++i) {
			if (buf[i] < 16) //Serial.print(0);
				//printf("%#x ", buf[i]);
			//Serial.print(buf[i], HEX); Serial.print(" ");
			if (i>0) {
				if (i%16 == 15) {
					//printf("\n");
					//Serial.println();
				} else if (i%8 == 7) {
					//printf(" ");
					//Serial.print(' ');
				}
			}
		}
		//printf(" ");
		//Serial.println();
		//printf("\n");
}

ISO7816::ISO7816()
{
    _resetpin = 9;
}

ISO7816::ISO7816(int p_reset)
{
    _resetpin = p_reset;
}

/**************************************************************************
					Initialize UART

    This function opens and initializes UART port if needed. 
    Usually called at program start.

Input Parameters:
	Port   - UART name or number

Return:
    This function returns TRUE in case of success or FALSE otherwise
 **************************************************************************/
BOOL ISO7816::Initialize(char *port)
{
    HAL_Initialize(_resetpin);
    if (HAL_Open_UART(port) != TRUE) return FALSE;
	HAL_RST_Off();	// Deactivate (high level) the RST line of ISO7816 interface
    return TRUE;
}
/**************************************************************************
					Shutdown

    This function closes UART port, usually called when program exits.

Input Parameters:
	none

Return:
	none
 **************************************************************************/
void ISO7816::Shutdown(void)
{
	HAL_Close_UART();
}
/**************************************************************************
			Reset ISO7816 interface

	This function performs RESET sequence using ISO7816 RST line

Input Parameters:
	none

Return:
	none
 **************************************************************************/
void ISO7816::Reset()
{
#ifdef DEBUGMODE
    if (LogLevel > 2) Log("Reset\n");
#endif
#ifdef MAXIMUM_BIT_RATE
	HAL_SetBitRate(ISO7816_DEF_DIVISION_FACTOR);
#endif
	HAL_Sleep(ISO7816_RESET_TIME);   

	HAL_SetTimeout_UART(ISO7816_AWT);	/* Set default Block Wait Timeout = 618 ms */

    HAL_Purge_UART();			    /* purge any information in the UART's buffer */

	HAL_RST_On();				    /* Activate (low level) RST line of ISO7816 interface */
	HAL_Sleep(ISO7816_RESET_TIME);  /* Wait 100 ms */
	HAL_RST_Off();				    /* Deactivate (low level) RST line of ISO7816 interface */

    Current_PCB = 0x00;
    Receive_PCB = 0x00;
}
/**************************************************************************
    Send control command (ex., PPS) to ISO7816 and receive response

	Arguments:
	    pbSendBuffer  - buffer containing the data to be written the UART
    	SendLength  - number of bytes to write to the file
	    pbRecvBuffer  - pointer to buffer for response
    	pRecvLength  - pointer to int for the length of the response received

	Return Value:
		This function returns TRUE or FALSE
 **************************************************************************/
static BOOL Transmit_UART(BYTE *pbSendBuffer, int SendLength, BYTE *pbRecvBuffer, int *pRecvLength)
{
int a = 3;
#ifdef DEBUGMODE
    if (LogLevel > 2)
        HexDump("Send ---> ", pbSendBuffer, SendLength);
#endif
    HAL_Purge_UART();
	if (HAL_WriteBlock_UART(pbSendBuffer, SendLength) != TRUE) {
		HexDump("Error: Write failed APDU: ", pbSendBuffer, SendLength);
		return FALSE;
	}
    if (HAL_ReadBlock_UART(pbRecvBuffer, MAX_R_APDU_SIZE, pRecvLength) == 0) {
#if 1//def DEBUGMODE
        HexDump("Error: response timeout. APDU: ", pbSendBuffer, SendLength);
#endif
        return FALSE;
    }
#ifdef DEBUGMODE
    if (LogLevel > 2)
        HexDump("Recv <--- ", pbRecvBuffer, *pRecvLength);
#endif
    return TRUE;
}
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
BOOL ISO7816::OpenCard(void)
{
#define SIZEOF_PPS 4
#define SIZEOF_IFS 5

#ifdef MAXIMUM_BIT_RATE
    BYTE cmdPPS_T1[]    = { 0xFF, 0x11, 0x18, 0xF6 };	/* Max. bit rate (DF=31) */
#else
	BYTE cmdPPS_T1[]    = { 0xFF, 0x11, 0x11, 0xFF };   /* Default bit rate (DF=372) */
#endif
	BYTE cmd_S_IFS_T1[] = { 0x00, 0xC1, 0x01, 0x00, 0x00 }; // Frame size IFS: 0xFE = 254 or 0x1C = 32 - 4
	BYTE COMbuffer[10];
	int i, NumberOfComBytes;

	cmd_S_IFS_T1[3] = T1_IFSD;
    cmd_S_IFS_T1[4] = cmd_S_IFS_T1[0] ^ cmd_S_IFS_T1[1] ^ cmd_S_IFS_T1[2] ^ cmd_S_IFS_T1[3];

    Reset();

    for (i = 0; i < 5; i++) 
    {
        if (HAL_ReadBlock_UART(ResponseBuffer, 1, NULL) <= 0) {   /* Read first ATR byte (3B) */
          #ifdef DEBUGMODE
            LogError("Read ATR: Timeout.\n");
          #endif
//Serial.println("1");
            return FALSE;
        }
        if (ResponseBuffer[0] == 0x3B) {
            HAL_ReadBlock_UART(ResponseBuffer + 1, MAX_ATR_SIZE, &NumberOfResponseBytes); /* Read other ATR bytes */
            NumberOfResponseBytes++;
            break;
        }
    }

#if 1 //def DEBUGMODE
	if (LogLevel > 0) HexDump("ATR: ", ResponseBuffer, NumberOfResponseBytes);
#endif
	HAL_Sleep(20);
    HAL_Purge_UART();                /* purge any information in the buffer */

	if (Transmit_UART(cmdPPS_T1, SIZEOF_PPS, COMbuffer, &NumberOfComBytes) != TRUE) {
      #if 1 //def DEBUGMODE
        LogError("Error: PPS failed\n");
      #endif
//Serial.println("2");
		return FALSE;
	}
	if (NumberOfComBytes != SIZEOF_PPS) {
      #if 1 //def DEBUGMODE
        //Serial.print("Error: PPS failed. Response length=");
		//Serial.println(NumberOfComBytes);
       // LogError("Error: PPS failed. Response length=%d\n", NumberOfComBytes);
       // if (NumberOfComBytes > 0) HexDump("", COMbuffer, NumberOfComBytes);
      #endif
//Serial.println("3");
		return FALSE;
    }
#ifdef MAXIMUM_BIT_RATE
	HAL_SetBitRate(ISO7816_FAST_DIVISION_FACTOR);
#endif

	if (Transmit_UART(cmd_S_IFS_T1, SIZEOF_IFS, COMbuffer, &NumberOfComBytes) != TRUE) {
      #if 1 //def DEBUGMODE
        LogError("Error: IFS failed\n");
      #endif
//Serial.println("4");
		return FALSE;
	}
	if (NumberOfComBytes != SIZEOF_IFS) {
      #if 1 //def DEBUGMODE
        //Serial.print("Error: IFS failed. Response length=");
		//Serial.println(NumberOfComBytes);
        //LogError("Error: IFS failed. Response length=%d\n", NumberOfComBytes);
        //if (NumberOfComBytes > 0) HexDump("", COMbuffer, NumberOfComBytes);
      #endif
//Serial.println("5");
		return FALSE;
    }
    T1_IFSC = COMbuffer[3];
    DataChunkSize = T1_IFSC - 6; // Max. data length in APDU: T1_IFSC-6: short Lc/Le or T1_IFSC-9: extended Lc/Le 
    if (DataChunkSize <= 0) {
      #if 1 //def DEBUGMODE
        //LogError("Error: IFSC too small, should be greater than 6: %d\n", T1_IFSC);
        LogError("Error: IFSC too small, should be greater than 6:");
      #endif
//Serial.println("6");
        return FALSE;
    }
    return TRUE;
}
/**************************************************************************
			Close connection to the chip
Input Parameters:
	none

Return:
	none
 **************************************************************************/
void ISO7816::CloseCard(void)
{
  //HAL_RST_Off();	// Deactivate (high level) the RST line of ISO7816 interface
}
/**************************************************************************
                         Set UART Timeouts

  Sets the UART reading timeout - max. time to wait for a response packet

 Parameters:
    timeout - in milliseconds
 Return values:
    TRUE                  on success
    FALSE    the device settings are illegal
 **************************************************************************/
BOOL ISO7816::SetTimeout(int timeout)
{
    return HAL_SetTimeout_UART(timeout);
}
/**************************************************************************
  Transmit a single T=1 packet, receive response
  Re-transmit in case of error
	Description:
 		This function adds T=1 frame, transmits command, 
      	receives response, removes T=1 frame.

	Arguments:
	    NAD
    	PCB
	    pbData  - buffer containing the APDU
    	SendLength  - number of bytes to send
	    pbRecvBuffer  - pointer to buffer for response
    	pRecvLength  - pointer to int for the length of the response received

	Return Value:
		This function returns TRUE or FALSE
 **************************************************************************/
BOOL ISO7816::Transmit_T1_packet(BYTE NAD, 
                              BYTE PCB,
                              BYTE *pbData, 
                              int iDataLength, 
                              BYTE *pbRecvBuffer, 
                              int *pRecvLength)
{
BYTE tempBuffer[100];
int tempLength;
int a = 3;

    BYTE sendbuffer[4];
    BYTE respbuffer[4];
    BYTE recvPCB, EDC;
    int  i, headerlen = 3, recvlen, datalen, cnt = 0;

    EDC = NAD ^ PCB ^ (BYTE)iDataLength;

    sendbuffer[0] = NAD;
    sendbuffer[1] = PCB;
    sendbuffer[2] = (BYTE)iDataLength;
    for (i=0; i < iDataLength; i++) 
        EDC ^= pbData[i];

	if ((PCB & 0x80) == 0x00) // Toggle PCB transmit sequence counter only if sending I-Block
        Current_PCB ^= 0x40;

    do {
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      #ifdef DEBUGMODE
        if (LogLevel > 1) {
            //Log("Send --> {%02X %02X %02X} ", sendbuffer[0], sendbuffer[1], sendbuffer[2]);
			Serial.print("Send --> {");
			Serial.print(sendbuffer[0], HEX); Serial.print(" ");
			Serial.print(sendbuffer[1], HEX); Serial.print(" ");
			Serial.print(sendbuffer[2], HEX);
//            if (headerlen > 3) Log("%02X", sendbuffer[3]);
            if (headerlen > 3) {
				Serial.print(" ");
				Serial.print(sendbuffer[3], HEX);
			}
				Serial.print("} ");
//            if (iDataLength) HexDumpNoCR("", pbData, iDataLength);
            if (iDataLength) HexDump("", pbData, iDataLength);
//            Log(" [%02X]\n", EDC);
            Serial.print(" ["); Serial.print(EDC, HEX);
			Serial.println("]");
        }
      #endif
        HAL_Purge_UART();
		if (HAL_WriteBlock_UART(sendbuffer, headerlen) != TRUE) return FALSE;   // Send T=1 header
        if (iDataLength)
    		if (HAL_WriteBlock_UART(pbData, iDataLength) != TRUE) return FALSE; // Send APDU
		if (HAL_WriteBlock_UART(&EDC, 1) != TRUE) return FALSE;                 // Send EDC

		if (HAL_ReadBlock_UART(respbuffer, 4, &recvlen) == 0)   // Receive T=1 minimum packet (header)
        {
		  #if 1 //def DEBUGMODE
//            LogScreen("Error: T=1 timeout in %d ms\n", HAL_GetTimerValue() - Time_start);
			printf("Error T1 Time out");
            //Serial.print("Error: T=1 timeout in ");
			//Serial.print(HAL_GetTimerValue() - Time_start, DEC);
			//Serial.println(" ms");
			HexDump("APDU: ", pbData, iDataLength);
		  #endif
			return FALSE;
		}
		if (recvlen >= 4) {
          #ifdef DEBUGMODE
            if (LogLevel > 1) {
//                Log("Recv <-- {%02X %02X %02X} ", respbuffer[0], respbuffer[1], respbuffer[2]);
                Serial.print("Recv <-- {");
				Serial.print(respbuffer[0], HEX); Serial.print(" ");
				Serial.print(respbuffer[1], HEX); Serial.print(" ");
				Serial.print(respbuffer[2], HEX); Serial.print("} ");
			}
          #endif
			datalen = (int)respbuffer[2];
            recvlen = 0;
			if (datalen) {
                pbRecvBuffer[0] = respbuffer[3];
				if ((a=HAL_ReadBlock_UART(pbRecvBuffer+1, datalen, &recvlen) == 0) || datalen != recvlen) {
	//Serial.print("a: "); Serial.println(a);
	//Serial.print("datalen: "); Serial.println(datalen);
	//Serial.print("recvlen: "); Serial.println(recvlen);
				  #if 1 //def DEBUGMODE
					//Serial.print("Error: T=1 data timeout\n");
					HexDump("APDU: ", pbData, iDataLength);
				  #endif
					return FALSE;
				}
              #if 1 //def DEBUGMODE
                if (LogLevel > 2) {
                    HexDump("", pbRecvBuffer, recvlen);
                    //Serial.print(" [");
                    //Serial.print(pbRecvBuffer[recvlen], HEX);
                    //Serial.print("]\n");
                }
              #endif
			}
            else {
              #if 1 //def DEBUGMODE
                if (LogLevel > 2) Log("\n");
              #endif
            }
		}
		else {
		  #if 1 //def DEBUGMODE
//			LogScreen("Error: Wrong T=1 response. (Len = %d)\n", recvlen);
			//Serial.print("Error: Wrong T=1 response. (Len = ");
			//Serial.print(recvlen);
			//Serial.println();
			if (recvlen)
				HexDump("Response: ", respbuffer, recvlen);
		  #endif
			return FALSE;
		}

        recvPCB = respbuffer[1] & 0xEF;                 // remove Send Sequence Number
        if (recvPCB == 0x81 || recvPCB == 0x82) {       // Error ? 
            if (cnt++ < T1_NumRetries) continue;        // Resend T=1 frame
            else return FALSE;
        }
        else if (recvPCB == 0xC3) { // WTX request ?
            sendbuffer[1] = 0xE3;   // WTX response
            sendbuffer[2] = 1;      // Len:  1
            sendbuffer[3] = 1;      // Data: 1
            EDC           = 0xE3;   // EDC
            headerlen = 4;
            iDataLength = 0; 
            HAL_Sleep(ISO7816_WTX_DELAY);
            if (cnt++ < ISO7816_MAX_WTX_RESPONSES) continue;
            else return FALSE;
        }
        if (datalen < recvlen)
            recvlen = datalen;	    // Cut-off the noise 
        break;
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
    } while (1);

    if (pRecvLength) *pRecvLength = recvlen;

	if ((recvPCB & 0x80) == 0x00)
        Receive_PCB ^= 0x40;   // Toggle PCB receive sequence counter only if sending I-Block

    if ((recvPCB & 0x20) != 0) return 0x20;         // R(x) chaining received
    return TRUE;
}
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
BOOL ISO7816::Transmit_T1       (BYTE *pbSendBuffer,
                                int   numBytesToSend, 
                                BYTE *pbRecvBuffer, 
                                int  *pRecvLength)
{
    int numBytesSent = 0, datablocklen;
    int NumberOfBytesRead, NumberOfBytesRead1 = 0;
    BYTE PCB;
    int  result;
/*--------------- First block exchange -------------------*/
    do {
        datablocklen = numBytesToSend;
        if (numBytesToSend > T1_IFSC){PCB = Current_PCB | 0x20; /* Use chaining when sending data bigger than IFSC */
                                      datablocklen = T1_IFSC; }
        else                          PCB = Current_PCB  & 0xDF; // Turn off more data bit;
        
        if ((result = Transmit_T1_packet(0, PCB, 
                                             pbSendBuffer + numBytesSent, datablocklen, 
                                             pbRecvBuffer, &NumberOfBytesRead1)) == FALSE) return FALSE;
        numBytesToSend -= T1_IFSC;
        numBytesSent += T1_IFSC;
    } while (numBytesToSend > 0);

/*----------- Optional received blocks chaining ---------*/
    while (result == 0x20)                  /* R(x) chaining received */
    {
        PCB = 0x80 | ((Receive_PCB != 0) ? 0x10 : 0);
        if ((result = Transmit_T1_packet(0, PCB, NULL, 0,
                                             pbRecvBuffer + NumberOfBytesRead1, 
                                             &NumberOfBytesRead)) == FALSE) return FALSE;
        NumberOfBytesRead1 += NumberOfBytesRead;
    }
    if (pRecvLength) *pRecvLength = NumberOfBytesRead1;

    return TRUE;
}
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
BOOL ISO7816::TransmitAPDU(BYTE   *pbInBuf,
                          int     lInBufLen,
                          BYTE   *pbOutBuf,
                          int    *plOutBufLen)
{
static BYTE cmd_GetResponse[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 };
  int   i, lResult = FALSE;
  int   offset = 0;
  int   saveLeflag = 0;
  BYTE  saveLe;
  int   lenr;

  if (pbOutBuf == NULL) pbOutBuf = ResponseBuffer; // Output buffer not supplied - use local buffer for response processing
  if (plOutBufLen) *plOutBufLen = 0;

#if 1 //def DEBUGMODE
  if (LogLevel > 1) {
      HexDump("APDU:     ", pbInBuf, lInBufLen);
  }
  Time_start = HAL_GetTimerValue();
#endif

  SW1SW2 = 0;
  for (i = 0; i<256; i++) { // Up to 256 chained responses
      lResult = Transmit_T1(pbInBuf, lInBufLen, pbOutBuf+offset, &lenr);

      if (saveLeflag) pbInBuf[lInBufLen-1] = saveLe;            // Restore Le if changed by SW 6Cxx processing
      saveLeflag = 0;
      if (lResult != TRUE && expectedSW1SW2 != 0x0000) return FALSE;
      if (lenr < 2) return FALSE;  // SW1SW2 not received - error

      offset += lenr;
      SW1SW2 = pbOutBuf[offset-2] * 256 + pbOutBuf[offset-1];

      if (pbOutBuf[offset-2] == 0x6C) {             // ISO7816-4: 6Cxx: Wrong Le field; 
          saveLe = pbInBuf[lInBufLen-1];
          saveLeflag = 1;
          pbInBuf[lInBufLen-1] = pbOutBuf[offset-1];// SW2 encodes the exact number of available data bytes
          continue;
      }
      else if (pbOutBuf[offset-2] == 0x61) {        // ISO7816-4: 61xx: more data available
          pbInBuf = cmd_GetResponse;                           // SW2 encodes the number of data bytes still available
          cmd_GetResponse[APDU_LC] = pbOutBuf[offset-1];
          offset += lenr - 2;
          lInBufLen = sizeof(cmd_GetResponse);
          continue;
      }
      break;
  }
  lenr -= 2;
  if (plOutBufLen) *plOutBufLen = offset - 2;

#if 1 //def DEBUGMODE
  if (LogLevel > 1) {
     // Log("Time: %d ms\n", HAL_GetTimerValue() - Time_start);
      HexDump("Resp:     ", pbOutBuf, offset);
  }
#endif

  return lResult;
}
