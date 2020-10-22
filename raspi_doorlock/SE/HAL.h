/*****************************************************************************
 * ISO7816 communication library Hardware abstraction layer for:
 * - Windows PC Serial (COM) port 
 * - Infineon XMC4500(ARM) family microcontroller
 * - TI MSP430 family microcontroller
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
    Created: Juri Cizas  02/01/2012
 *****************************************************************************/
#ifndef HAL_H
#define HAL_H
#include <wiringPi.h>

//###########################################################################
// Platform and ISO7816 interface hardware implementation choice
//###########################################################################
//#define DEBUGMODE
#define LOGLEVEL 2

#if 0
//-----------------------------------------------------------------
//----------- Windows ISO7816 emulation using COM port  -----------
//-----------------------------------------------------------------
#ifdef WIN32
  #define _CRT_SECURE_NO_DEPRECATE
  #define COM_PORT_CONVERTER     // USB-COM port converter with 3.5712MHz oscillator and RTS used as RST
//#define IFX_TRANSPARENT_READER // Infineon transparent reader using DTR as RST

//-----------------------------------------------------------------
//--------  Infineon XMC4xxx embedded family MCU (ARM 32 bit) -----
//-----------------------------------------------------------------
#elif defined DAVE_CE 
//#define IFX_XE166_MICROCONTROLLER	// Infineon XE164/166 16-bit MCU family - Uconnect board

//-----------------------------------------------------------------
//----------------  TI MSP430 Code Composer Studio ----------------
//-----------------------------------------------------------------
#else
  #define MSP430
#endif
#endif
#define ARDUINO

/////////////////////////////////////////////////////////////////////////////
//    Includes
/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if 0
/////////////////////////////////////////////////////////////////////////////
//   Platform and ISO7816 interface hardware dependent Includes and Types
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
  #include <windows.h>
  #include <sys/stat.h>
  #include <io.h>
  #include <conio.h>
  #define MAXIMUM_BIT_RATE // Defined for programmable UART bit rate (DF=372 changed to DF=31 after ATR)
#endif

#ifdef DAVE_CE
  #include <sys/stat.h>
  #include <string.h>
  #include <DAVE3.h>			//Declarations from DAVE3 Code Generation (includes SFR declaration)

  typedef uint8_t BYTE;
  typedef uint16_t WORD;
  typedef unsigned int BOOL;
  typedef unsigned int UINT;
  typedef unsigned int ULONG;
  typedef unsigned int LONGLONG;

  #define TRUE 1
  #define FALSE 0

  #define USB_HOST
  #define MAXIMUM_BIT_RATE // Defined for programmable UART bit rate (DF=372 changed to DF=31 after ATR)
#endif

#ifdef MSP430
  typedef unsigned char BYTE;
  typedef unsigned short WORD;
  typedef unsigned int BOOL;
  typedef unsigned int LONGLONG;

  #define TRUE 1
  #define FALSE 0

  #define __NOP() _nop()
#endif
#endif
#ifdef ARDUINO
  typedef unsigned char BYTE;
  typedef unsigned short WORD;
  typedef unsigned int BOOL;
  typedef unsigned int LONGLONG;

  #define TRUE 1
  #define FALSE 0

  #define __NOP() _nop()
//  #define MAXIMUM_BIT_RATE // Defined for programmable UART bit rate (DF=372 changed to DF=31 after ATR)
#endif

/////////////////////////////////////////////////////////////////////////////
//                  ISO7816 Interface parameters 
/////////////////////////////////////////////////////////////////////////////
#define APDU_BUFFER_SIZE 270
#define RESP_BUFFER_SIZE 270
#define MAX_ATR_SIZE 100
#define MAX_FRAME_SIZE 254 // ISO7816 T=1 protocol data size. Recommended: 254, UART RAM buffers saving: 32-4=28 or 16-4=12
#define MAX_DATA_BLOCK_SIZE 249

/////////////////////////////////////////////////////////////////////////////
//    Console output
/////////////////////////////////////////////////////////////////////////////
#define LogError Log 
#define LogScreen Log 

/////////////////////////////////////////////////////////////////////////////
//    Platform dependent System Functions
/////////////////////////////////////////////////////////////////////////////
void HAL_Initialize(int);

void HAL_LED(int num, int onoff);

void HAL_Sleep(int time);
LONGLONG HAL_GetTimerValue(void);

/////////////////////////////////////////////////////////////////////////////
//    ISO7816 Functions: RST signal manipulation via I/O port
/////////////////////////////////////////////////////////////////////////////
void HAL_RST_On(void);
void HAL_RST_Off(void);

/////////////////////////////////////////////////////////////////////////////
//    ISO7816 Functions: UART control 
/////////////////////////////////////////////////////////////////////////////
BOOL HAL_Open_UART(char *port);
void HAL_Close_UART(void);

BOOL HAL_SetTimeout_UART(int Timeout);

void HAL_Purge_UART(void);

BOOL HAL_WriteBlock_UART(BYTE *pbSendBuffer, int SendLength);

int  HAL_ReadBlock_UART(BYTE *pbBuffer, int NumberOfBytesToRead, int *pNumberOfBytesRead);

void HAL_SetBitRate(int division_factor);

/////////////////////////////////////////////////////////////////////////////
// Host USB-UART communication functions. Ex., XMC4xxx USB Virtual COM port
/////////////////////////////////////////////////////////////////////////////
#include "Host.h"

#endif // HAL_H
