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
#include "SArduino.h"
#ifdef DEBUGMODE
  #include "Utils.h"
#endif
#include "ScriptBin.h"

/*---------------------------------------------------------------
  APDU Script structure:
      Byte 1: Opcode(8 bits): 
              Opcode=81 - Plaintext APDU
        (RFU) Opcode=82 - Secure APDU(param1) with MAC-only SM 
        (RFU) Opcode=83 - Secure APDU(param1) with ENC+MAC SM 
              Opcode=90 - Reset ISO7816 interface, receive ATR
              Opcode=91 - Set timeout

      Byte 2: Mode(4 bits)/Number of params(4 bits)
              Mode=0: 1 param., Param1=C-APDU, ignore response data, SW1SW2 must be 9000
              Mode=1: 2 params, Param1=C-APDU, fail if SW1SW2 not as expected(param2), ignore response data
              Mode=2: 2 params, Param1=C-APDU, fail if response data(param2) not as expected, SW1SW2 must be 9000
              Mode=3: 3 params, Param1=C-APDU, fail if response data(param2) or SW1SW2(param3) not as expected

      Byte 3: Param1Len(1 byte = N1) or 'FF' + 2 bytes length or 'FE' + Script Variable index

      Byte 4...: Param1 (N1 bytes) 

      Byte x: Param2Len (1 byte = N2) or 'FF' + 2 bytes length or 'FE' + Script Variable index
      Byte x: Param2 (N2 bytes) ...
  ---------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Scripting engine: execute binary APDU script 
 -------------------------------------------------------------------------------*/
extern ISO7816 iso7816;
extern BYTE * ResponseBuffer;
extern WORD SW1SW2;
int RunBinaryScript(const BYTE *pscript, int script_length)
{
    int opcode, opmode, paramlen;
    int script_pos;
    WORD expSW;

    if (script_length == 0) {
		#ifdef DEBUGMODE
    	LogError("ERROR: Script not loaded\n");
		#endif
    	return FALSE;
    }
    for (script_pos=0; script_pos < script_length;)
    {//--------------- Opcode + parameters ------------------
        opcode = (int)(pscript[script_pos++]);             // Operation code - first byte
        opmode = (int)((pscript[script_pos] >> 4) & 0x0F); // Operation mode - higher nibble of second byte
//      nparam = (int)(pscript[script_pos++] & 0x0F);      // Parameters number- lower nibble of second byte
        script_pos++;
        switch (opcode) {
            //-------------------- Send Plaintext APDU ----------------------
            case OPCODE_APDU_PLAIN:
                paramlen = pscript[script_pos++]; // 1 parameter: C-APDU
                if (paramlen == 0xFF) { paramlen = pscript[script_pos++] * 256; paramlen += pscript[script_pos++]; }

	            if (!iso7816.TransmitAPDU((BYTE*)(pscript+script_pos), paramlen, NULL, NULL)) return 0;
//                LED_toggle(1);
                script_pos += paramlen;

                if ((opmode & 2) != 0) {          // Modes 2,3: 2 parameter: expected data
                    paramlen = pscript[script_pos++]; // 2 parameter: expected response or expected SW1SW2
                    if (paramlen == 0xFF) { paramlen = pscript[script_pos++] * 256; paramlen += pscript[script_pos++]; }

                    if (memcmp(ResponseBuffer, pscript+script_pos, paramlen) != 0) {
						#ifdef DEBUGMODE
                    	LogError("ERROR: Data not as expected\n");
						#endif
                    	return 0;
                    }
                    script_pos += paramlen;
                }
                if ((opmode & 1) != 0) {          // Modes 1,3: 2/3 parameter: expected SW1SW2
                    paramlen = pscript[script_pos++];
                    expSW = pscript[script_pos]*256 + pscript[script_pos+1];
                    script_pos += paramlen;
                }
                else expSW = 0x9000;

                if (expSW != 0 && SW1SW2 != expSW) {
					#ifdef DEBUGMODE
                	LogError("ERROR: SW1SW2=%04X. Expected: %04X.\n", SW1SW2, expSW);
					#endif
                	return 0;
                }
                break;
            //-------------------- Reset ISO7816 interface ----------------------
            case OPCODE_RESET: // No parameters
                iso7816.CloseCard();
                iso7816.OpenCard();
                break;
            //-------------------- Set maximum response timeout value ----------------------
            case OPCODE_SET_TIMEOUT:
                paramlen = pscript[script_pos++];
                iso7816.SetTimeout((WORD)(pscript[script_pos]*256 + pscript[script_pos+1]));
                script_pos += paramlen;
                break;
            default:
				#ifdef DEBUGMODE
                LogError("ERROR: Unsupported opcode in script\n");
				#endif
                return 0; 
        }
    }
    return 1; 
}
