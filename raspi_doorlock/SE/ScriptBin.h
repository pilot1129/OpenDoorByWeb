/*****************************************************************************
 * Binary APDU script engine
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
    Created: Juri Cizas  06/17/2012
 *****************************************************************************/
#ifndef INCLUDED_SCRIPT_H
#define INCLUDED_SCRIPT_H

#define OPCODE_APDU_PLAIN     0x81 // Send plaintext APDU(param1)
#define OPCODE_APDU_SM_MAC    0x82 // Send secure APDU(param1) with MAC-only SM 
#define OPCODE_APDU_SM_ENC    0x83 // Send secure APDU(param1) with ENC+MAC SM 

#define OPCODE_RESET          0x90 // Reset ISO7816 interface
#define OPCODE_SET_TIMEOUT    0x91 // Set maximum response timeout value

int RunBinaryScript(const BYTE *pscript, int script_length);

#endif // INCLUDED_SCRIPT_H
