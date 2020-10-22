/*****************************************************************************
 * Secure Device API library
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
#include <wiringPi.h>
#include <wiringSerial.h>
#include <termios.h>
#include "ISO7816.h"
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>
#ifdef DEBUGMODE
	#include "Utils.h"
#endif
#include "ScriptBin.h"

BYTE C_APDU[MAX_C_APDU_SIZE]; // Array reserved for Command APDU
BYTE R_APDU[MAX_C_APDU_SIZE]; // Array reserved for response APDU

#define PRIVATE 0
#define PUBLIC  1

//------------------------ Local variables ------------------------------
#define CryptoAppletAID 0x12, 0x34, 0x56, 0x78, 0x9A, 0x01
/*--------------------------------------------------------------------
  Command APDU: Select CryptoApplet with AID="12 34 56 78 9A 01"
 --------------------------------------------------------------------*/
static BYTE cmd_SelectCryptoApplet[] = { 0, 0xA4, 0x04, 0x00, 6, CryptoAppletAID };
extern int Serial3;
extern ISO7816 iso7816;
extern WORD SW1SW2;

#define P_RESET 9
ISO7816 iso7816 = ISO7816(P_RESET);

/*------------------------------------------------------------------------------
   The following include contains the APDU script for JavaCard applet loading
   during initialization or firmware/applet version upgrade.
 -------------------------------------------------------------------------------*/
#if 1
    BYTE loadapplet_script[] = {
	#include "JTOP46_LoadCryptoApplet.h"
    };
#endif
/*-------------------------------------------------------------------------------
   Open channel to Secure Device. Returns TRUE if OK or FALSE in case of error.
 -------------------------------------------------------------------------------*/
int SecDev_Open(void)
{
/*------------------------------------------------------------------------------
   The following include contains the APDU script for JavaCard initialization 
   from NOT_READY state to OP_READY and then SECURED state.
 -------------------------------------------------------------------------------*/
	BYTE init_script[] = {
	 #include "JTOP46_Initialize.h"
	};
	//--------- Send SELECT APPLET APDU ---------
	if (iso7816.TransmitAPDU(cmd_SelectCryptoApplet, sizeof(cmd_SelectCryptoApplet), NULL, NULL) && SW1SW2 == 0x9000) 
        return TRUE; // Applet selected normally - return 1

    if (SW1SW2 == 0x6E00) { // Chip is in NOT_READY state - perform initialization
        if (!RunBinaryScript(init_script, sizeof(init_script))) {
          #ifdef DEBUGMODE
            LogError("ERROR: Initialization Script failed\n");
          #endif
            return FALSE; // Applet initialization failed - return 0
        }
        if (SecDev_LoadApplet()) return TRUE;  // Load applet after initialization
    }
#if 1
    else if (SW1SW2 == 0x6A82 || SW1SW2 == 0x6999) { // Applet not found - shouldn't happen, probably error. 
        if (SecDev_LoadApplet()) return TRUE; // Try to reload applet 
    }
#endif
    return FALSE;
}

#if 1
/*------------------------------------------------------------------------------
   Load (or reload) applet, deletes if applet exists. Returns FALSE in case of error.
 -------------------------------------------------------------------------------*/
int SecDev_LoadApplet(void)
{
    if (!RunBinaryScript(loadapplet_script, sizeof(loadapplet_script))) {
      #ifdef DEBUGMODE
        LogError("ERROR: Applet Loading script failed\n");
      #endif
        return FALSE; 
    }
	if (iso7816.TransmitAPDU(cmd_SelectCryptoApplet, sizeof(cmd_SelectCryptoApplet), NULL, NULL) && SW1SW2 == 0x9000) 
        return TRUE;
  #ifdef DEBUGMODE
    LogError("ERROR: Select Applet failed\n");
  #endif
    return FALSE; 
}

/*-------------------------------------------------------------------------------
 Put key value (AES,3DES,RSA) into Crypto Applet NVM storage on Secure Processor 
 -------------------------------------------------------------------------------*/
int SecDev_PutKey(BYTE   algid, // ALG:   09=AES_CBC_128  0D=AES_CBC_256
                  BYTE   keyid, // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                  BYTE*  key,   // Pointer to AES key array
                  int    keylen)// AES key length
{
    //keylen = keylen/8;
    C_APDU[APDU_CLA] = 0x80;            // CLA - Class
    C_APDU[APDU_INS] = 0x80;            // INS - Instruction: Set key
    C_APDU[APDU_P1] = algid;            // P1 - Parameter 1
    C_APDU[APDU_P2] = keyid;            // P2 - Parameter 2
    C_APDU[APDU_LC] = (BYTE)(keylen);   // Set Lc value = number of bytes in key
    memcpy(C_APDU + APDU_DATA, key, keylen);  // Copy key value to APDU

    if (iso7816.TransmitAPDU(C_APDU, keylen + 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
    return FALSE;
}
/*-------------------------------------------------------------------------------
    Encrypt data using AES or 3DES key stored on Secure Processor
 -------------------------------------------------------------------------------*/
int SecDev_Encrypt_Symm(BYTE   keyid,       // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *outdata,     // Pointer to output data array
                        int   *outdatalen)  // Length of output data
{
    C_APDU[APDU_CLA] = 0x80;            // CLA - Class
    C_APDU[APDU_INS] = 0x81;            // INS - Instruction: AES encryption
    C_APDU[APDU_P1]  = 0x00;            // P1 - Parameter 1
    C_APDU[APDU_P2]  = keyid;           // P2 - Parameter 2
    C_APDU[APDU_LC]  = (indatalen >= 256) ? 0xFF : (BYTE)indatalen; // Lc - FF for 256 bytes

    memcpy(C_APDU + APDU_DATA, indata, indatalen); // Copy data to APDU

    if (outdatalen) *outdatalen = 0;
    if (!iso7816.TransmitAPDU(C_APDU, indatalen + 5, outdata, outdatalen) || SW1SW2 != 9000) return FALSE;
    return TRUE;
}

/*-------------------------------------------------------------------------------
    Decrypt data using AES or 3DES key stored on Secure Processor
 -------------------------------------------------------------------------------*/
int SecDev_Decrypt_Symm(BYTE   keyid,       // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *outdata,     // Pointer to output data array
                        int   *outdatalen)  // Length of output data
{
    C_APDU[APDU_CLA] = 0x80;            // CLA - Class
    C_APDU[APDU_INS] = 0x82;            // INS - Instruction: AES encryption
    C_APDU[APDU_P1]  = 0x00;            // P1 - Parameter 1
    C_APDU[APDU_P2]  = keyid;           // P2 - Parameter 2
    C_APDU[APDU_LC]  = (indatalen >= 256) ? 0xFF : (BYTE)indatalen; // Lc - FF for 256 bytes
    memcpy(C_APDU + APDU_DATA, indata, indatalen); // Copy data to APDU

    if (outdatalen) *outdatalen = 0;
    if (!iso7816.TransmitAPDU(C_APDU, indatalen + 5, outdata, outdatalen) || SW1SW2 != 0x9000) return FALSE;
    return TRUE;
}
/*-------------------------------------------------------------------------------
    Secure Processor calculates hash of input data using SHA
 -------------------------------------------------------------------------------*/
int SHA_256         (    BYTE  *plain_data,      // Pointer to input data array
                        int    plain_len,   // Length of input data
                        BYTE  *digest,      // Pointer to digest data array
                        int   *digest_len)   // Length of digest
{
    int ofs = 0;
    int lenc;

    C_APDU[APDU_CLA] = 0x80;            // CLA - Class uses command chaining
    C_APDU[APDU_INS] = 0x50;            // INS - Instruction: SHA
    C_APDU[APDU_P1]  = 0x04;           // P1 - Parameter 1
    C_APDU[APDU_P2]  = 0x00;            // P2 - Parameter 2

    while (plain_len > 0) 
    {
        if (plain_len < (int) MAX_DATA_BLOCK_SIZE) {
            lenc = plain_len;
            C_APDU[APDU_CLA] = 0x80;    // CLA - Class: last APDU in chain
        }
        else lenc = MAX_DATA_BLOCK_SIZE;

        C_APDU[APDU_LC] = (BYTE)lenc;       // Add Lc = data block legth
        memcpy((void *)(C_APDU + APDU_DATA), (void *)(plain_data + ofs), lenc); // Copy data block to APDU

        if (!iso7816.TransmitAPDU(C_APDU, lenc + 5, digest, digest_len) || SW1SW2 != 0x9000) return FALSE;

        ofs += lenc;
        plain_len -= lenc;
    }
    return TRUE;
}
/*-------------------------------------------------------------------------------
    Secure Processor calculates hash of input data using SHA
 -------------------------------------------------------------------------------*/
int SecDev_SHA2         (BYTE   algid,       // 01=SHA1, 04=SHA_256, 05=SHA384, 06=SHA512
                        BYTE  indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *digest,      // Pointer to digest data array
                        int   *digestlen,
			BYTE check)   // Length of digest
{



    C_APDU[APDU_CLA] = 0x80;            // CLA - Class uses command chaining
    C_APDU[APDU_INS] = 0x50;            // INS - Instruction: SHA
    C_APDU[APDU_P1]  = algid;           // P1 - Parameter 1
    C_APDU[APDU_P2]  = check;            // P2 - Parameter 2
C_APDU[APDU_LC] = 0x01;

        C_APDU[APDU_DATA] = indata;       // Add Lc = data block legth
       
        if (!iso7816.TransmitAPDU(C_APDU, 6, digest, digestlen) || SW1SW2 != 0x9000) return FALSE;

    return TRUE;
}
/*-------------------------------------------------------------------------------
   Generate RSA Key Pair, store it on Secure Processor, return public key
 -------------------------------------------------------------------------------*/
int SecDev_RSA_GenKeyPair(BYTE   algid,    // ALG:   06=RSA1024/CRT 07=RSA2048/CRT
                          BYTE   keyid,    // KEYID: 08=RSA1024/CRT 09=RSA2048/CRT 0A=RSA1024/nonCRT 0B=RSA2048/nonCRT
                          BYTE  *pubkey,   // Pointer to RSA public key array
                          int   *pubkeylen)// RSA public key length
{
    C_APDU[APDU_CLA] = 0x80;            // CLA - Class
    C_APDU[APDU_INS] = 0x88;            // INS - Instruction: Generate RSA Key Pair
    C_APDU[APDU_P1] = algid;            // P1 - Parameter 1
    C_APDU[APDU_P2] = keyid;            // P2 - Parameter 2
    C_APDU[APDU_LC] = 0x00;             // Lc - no data for CRT key or 1 byte for non-CRT
    C_APDU[APDU_DATA]=0x01;             // Data absent - CRT, data present - non-CRT

    if (iso7816.TransmitAPDU(C_APDU, 6, pubkey, pubkeylen) && SW1SW2 == 0x9000) return TRUE;
    return FALSE;
}
/*-------------------------------------------------------------------------------
    Secure Processor calculates hash/signature of input data using RSA
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Sign    (BYTE   keyid,       // RSA key reference
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int   *signaturelen)// Length of signature
{
    int ofs = 0;
    int lenc;

    C_APDU[APDU_CLA] = 0x90;            // CLA - Class uses command chaining
    C_APDU[APDU_INS] = 0x8B;            // INS - Instruction: RSA SIGN
    C_APDU[APDU_P1]  = 0x00;            // P1 - Parameter 1
    C_APDU[APDU_P2]  = keyid;           // P2 - Parameter 2

    while (indatalen > 0) 
    {
        if (indatalen < (int) MAX_DATA_BLOCK_SIZE) {
            lenc = indatalen;
            C_APDU[APDU_CLA] = 0x80;    // CLA - Class: last APDU in chain
        }
        else lenc = MAX_DATA_BLOCK_SIZE;

        C_APDU[APDU_LC] = (BYTE)lenc;       // Add Lc = data block legth
        memcpy((void *)(C_APDU + APDU_DATA), (void *)(indata + ofs), lenc); // Copy data block to APDU

        if (!iso7816.TransmitAPDU(C_APDU, lenc + 5, signature, signaturelen) || SW1SW2 != 0x9000) return FALSE;

        ofs += lenc;
        indatalen -= lenc;
    }
    return TRUE;
}
/*-------------------------------------------------------------------------------
    Secure Processor verifies the signature of input data using RSA
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Verify  (BYTE   keyid,       // RSA key reference
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int    signaturelen)// Length of signature
{
    int ofs = 0;
    int lenc;

    C_APDU[APDU_CLA] = 0x80;            // CLA - Class
    C_APDU[APDU_INS] = 0xDA;            // INS - Instruction: Put Data - set signature for verification
    C_APDU[APDU_P1]  = 0x00;            // P1 - Parameter 1
    C_APDU[APDU_P2]  = 0x00;            // P2 - Parameter 2
    C_APDU[APDU_LC]  = (signaturelen >= 256) ? 0xFF : (BYTE)signaturelen; // Lc - FF for 256 bytes
    memcpy(C_APDU + APDU_DATA, signature, signaturelen);// Signature

    if (!iso7816.TransmitAPDU(C_APDU, signaturelen + 5, NULL, NULL) || SW1SW2 != 0x9000) return FALSE;

    C_APDU[APDU_CLA] = 0x90;            // CLA - Class uses command chaining
    C_APDU[APDU_INS] = 0x8C;            // INS - Instruction: RSA VERIFY
    C_APDU[APDU_P1]  = 0x00;            // P1 - Parameter 1
    C_APDU[APDU_P2]  = keyid;           // P2 - Parameter 2

    while (indatalen > 0) 
    {
        if (indatalen < (int) MAX_DATA_BLOCK_SIZE) {
            lenc = indatalen;
            C_APDU[APDU_CLA] = 0x80;    // CLA - Class: last APDU in chain
        }
        else lenc = MAX_DATA_BLOCK_SIZE;

        C_APDU[APDU_LC] = (BYTE)lenc;       // Add Lc = data block legth
        memcpy((void *)(C_APDU + APDU_DATA), (void *)(indata + ofs), lenc); // Copy data block to APDU

        if (!iso7816.TransmitAPDU(C_APDU, lenc + 5, NULL, NULL) || SW1SW2 != 0x9000) return FALSE;

        ofs += lenc;
        indatalen -= lenc;
    }
    return TRUE;
}
/*-------------------------------------------------------------------------------
   Create a file in Secure Processor NVM storage
 -------------------------------------------------------------------------------*/
int SecDev_FileSystem_CreateFile (WORD fileid,  // File Identifier
		                          int  filesize)// File size in bytes
{
    C_APDU[APDU_CLA] = 0x80;                    // CLA - Class
    C_APDU[APDU_INS] = 0xE0;            		// INS - Instruction: Create File
    C_APDU[APDU_P1] = (BYTE)(fileid >> 8);      // P1 - Parameter 1 - File ID high byte
    C_APDU[APDU_P2] = (BYTE)(fileid & 0xFF);    // P2 - Parameter 2 - File ID low byte
    C_APDU[APDU_LC] = 0x02;                     // Lc - no data
    C_APDU[APDU_DATA]=(BYTE)(filesize >> 8);    // File size in bytes (high byte)
    C_APDU[APDU_DATA+1]=(BYTE)(filesize & 0xFF);// File size in bytes (low byte)

    if (iso7816.TransmitAPDU(C_APDU, 7, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
    return FALSE;
}
/*-------------------------------------------------------------------------------
   Select file in Secure Processor NVM storage
 -------------------------------------------------------------------------------*/
int SecDev_FileSystem_SelectFile (WORD fileid)  // File Identifier
{
    C_APDU[APDU_CLA] = 0x80;                    // CLA - Class
    C_APDU[APDU_INS] = 0xA4;            		// INS - Instruction: Select File
    C_APDU[APDU_P1] = (BYTE)(fileid >> 8);      // P1 - Parameter 1 - File ID high byte
    C_APDU[APDU_P2] = (BYTE)(fileid & 0xFF);    // P2 - Parameter 2 - File ID low byte
    C_APDU[APDU_LC] = 0x00;                     // Lc - no data

    if (iso7816.TransmitAPDU(C_APDU, 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
    return FALSE;
}
/*-------------------------------------------------------------------------------
    Write/Update file in Secure Processor NVM storage
 -------------------------------------------------------------------------------*/
int SecDev_FileSystem_WriteFile(WORD   fileid,  // File Identifier
								int    offset,  // Offset in file
								BYTE  *data,    // Pointer to a data array to be written
								int    datalen) // Length of data data to be written
{
    int ofs = offset;
    int lenc;

    C_APDU[APDU_CLA] = 0x80;            		// CLA - Class
    C_APDU[APDU_INS] = 0xD6;            		// INS - Instruction: UpdateFile

    while (datalen > 0)
    {
        C_APDU[APDU_P1]  = (BYTE)(ofs >> 8); 	// P1 - Parameter 1 - offset in file (high byte)
        C_APDU[APDU_P2]  = (BYTE)(ofs & 0xFF);  // P2 - Parameter 2 - offset in file (low byte)

        if (datalen < (int) MAX_DATA_BLOCK_SIZE) lenc = datalen;
        else                                     lenc = MAX_DATA_BLOCK_SIZE;

        C_APDU[APDU_LC] = (BYTE)lenc;       	// Lc = data block length
        memcpy((void *)(C_APDU + APDU_DATA), (void *)(data + ofs), lenc); // Copy data block to APDU

        if (!iso7816.TransmitAPDU(C_APDU, lenc + 5, NULL, NULL) || SW1SW2 != 0x9000) return FALSE;

        ofs += lenc;
        datalen -= lenc;
    }
    return TRUE;
}
/*-------------------------------------------------------------------------------
    Write/Update file in Secure Processor NVM storage. Returns actual bytes number
 -------------------------------------------------------------------------------*/
int SecDev_FileSystem_ReadFile (WORD   fileid,  // File Identifier
								int    offset,  // Offset in file
								BYTE  *data,    // Pointer to a data array to store the result
								int    datalen) // Length of data data to be read
{
    int ofs = offset;
    int lenc, lenr;

    C_APDU[APDU_CLA] = 0x80;            		// CLA - Class
    C_APDU[APDU_INS] = 0x02;            		// INS - Instruction: ReadFile

    while (datalen > 0)
    {
        C_APDU[APDU_P1]  = (BYTE)(ofs >> 8); 	// P1 - Parameter 1 - offset in file (high byte)
        C_APDU[APDU_P2]  = (BYTE)(ofs & 0xFF);  // P2 - Parameter 2 - offset in file (low byte)

        if (datalen < 256) lenc = datalen;
        else               lenc = 256;

        C_APDU[APDU_LC] = (BYTE)lenc;       	    // Le = length of expected data block

        if (!iso7816.TransmitAPDU(C_APDU, 5, R_APDU, &lenr) || SW1SW2 != 0x9000) return 0;
        memcpy((void *)(data + ofs), (void *)R_APDU, lenr); // Copy response data to output array

        ofs += lenr;
        datalen -= lenr;
    }
    return ofs;
}
/*-------------------------------------------------------------------------------
    RSA 키를 IMPORT 하기 위해 먼저 초기화를 진행
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Init()
{
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x8D;
	C_APDU[APDU_P1] = 0x86;
	C_APDU[APDU_P2] = 0x10;
	C_APDU[APDU_LC] = 0x00;

	if(iso7816.TransmitAPDU(C_APDU, 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}
/*-------------------------------------------------------------------------------
    RSA 키를 IMPORT
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Import(BYTE P1, BYTE* data, int len)
{
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x8E;
	C_APDU[APDU_P1] = P1;
	C_APDU[APDU_P2] = 0x10;
	C_APDU[APDU_LC] = (len >= 256) ? 0xFF : (BYTE)len;
	
	memcpy(C_APDU + APDU_DATA, data, len);

	if(!iso7816.TransmitAPDU(C_APDU, len + 5, NULL, NULL) || SW1SW2 != 9000) return FALSE;
	return TRUE;
}
/*-------------------------------------------------------------------------------
    RSA 키를 통해 암호화(공개키 암호화)
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Encrypt(BYTE* data, int len, BYTE* data2, int* len2)
{

	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x89;
	C_APDU[APDU_P1] = 0x0A;
	C_APDU[APDU_P2] = 0x10;
	C_APDU[APDU_LC] = (len >= 256) ? 0xFF : (BYTE)len;
	
	memcpy(C_APDU + APDU_DATA, data, len);

	if(!iso7816.TransmitAPDU(C_APDU, len + 5, data2, len2) || SW1SW2 != 9000) return FALSE;

	return TRUE;
}
/*-------------------------------------------------------------------------------
    RSA 키를 통해 암호화(공개키 암호화)
 -------------------------------------------------------------------------------*/
int SecDev_RSA_Decrypt(BYTE* data, int len, BYTE* data2, int* len2)
{
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x8A;
	C_APDU[APDU_P1] = 0x0A;
	C_APDU[APDU_P2] = 0x10;
	C_APDU[APDU_LC] = (len >= 256) ? 0xFF : (BYTE)len;
	
	memcpy(C_APDU + APDU_DATA, data, len);

	if(!iso7816.TransmitAPDU(C_APDU, len + 5, data2, len2) || SW1SW2 != 9000) return FALSE;
	return TRUE;
}
/*-------------------------------------------------------------------------------
   Create a file in Secure Processor NVM storage and update file
 -------------------------------------------------------------------------------*/

/*
int SecDev_Req_VP_Verify(BYTE* pbInBuf, int InBufLen)
{
	int result;
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x02;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = 0x00;
	C_APDU[APDU_LC] = (BYTE)InBufLen;

	memcpy(C_APDU+APDU_DATA, pbInBuf, InBufLen);
	result = iso7816.TransmitAPDU(C_APDU, 5 + InBufLen, NULL, NULL);
	if (result && SW1SW2 == 0x9000) return 0x01;
	else if(result && SW1SW2 == 0x6A10) return 0x02; 
	else if(result && SW1SW2 == 0x6A20) return 0x03;
	else if(result && SW1SW2 == 0x6A30) return 0x04;
	else if (result && SW1SW2 == 0x6A60) return 0x07;
	else if (result && SW1SW2 == 0x6A70) return 0x08;
	else return 0x05;
}

int SecDev_Req_FW_Verify(BYTE* pbInBuf, int InBufLen)
{
	int result;
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x04;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = 0x00;
	C_APDU[APDU_LC] = (BYTE)InBufLen;

	memcpy(C_APDU+APDU_DATA, pbInBuf, InBufLen);
	result = iso7816.TransmitAPDU(C_APDU, 5 + InBufLen, NULL, NULL);
	if (result && SW1SW2 == 0x9000) return 0x01;
	else if(result && SW1SW2 == 0x6A20) return 0x03;
	else if(result && SW1SW2 == 0x6A50) return 0x06;
	else if (result && SW1SW2 == 0x6A60) return 0x07;
	else if (result && SW1SW2 == 0x6A70) return 0x08;
	else return 0x07;
}

int SecDev_Req_Gen_ECC(BYTE* PbOutBuf, int OutBufLen) {
	int result;
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0xA0;
	C_APDU[APDU_P1] = 0x0E;
	C_APDU[APDU_P2] = 0x02;
	C_APDU[APDU_LC] = 0x00;

	result = iso7816.TransmitAPDU(C_APDU, 5, PbOutBuf, OutBufLen);
	if (result && SW1SW2 == 0x9000) return 0x01;
	else return 0x00;
}

int SecDev_Req_Gen_secKey(BYTE* pbInBuf, int InBufLen, BYTE* pbOutBuf, int outBufLen){
	int result;
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0xA3;
	C_APDU[APDU_P1] = 0x0E;
	C_APDU[APDU_P2] = 0x02;
	C_APDU[APDU_LC] = (BYTE)InBufLen+1;
	C_APDU[APDU_DATA] = 0x04;

	memcpy(C_APDU + APDU_DATA+1, pbInBuf, InBufLen);
	result = iso7816.TransmitAPDU(C_APDU, 6 + InBufLen, pbOutBuf, outBufLen);
	if (result && SW1SW2 == 0x9000) return 0x01;
	else return 0x00;
}

int SecDev_Req_Nonce(BYTE* PbOutBuf, int OutBufLen) {
	int result;
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x84;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = 0x00;
	C_APDU[APDU_LC] = 0x00;

	result = iso7816.TransmitAPDU(C_APDU, 5, PbOutBuf, OutBufLen);
	if (result && SW1SW2 == 0x9000) return 0x01;
	else return 0x00;
}
*/

//IoT SW API

/*=================================================
 int Connect_Applet()
 : Crypto Applet에 접근하기 위한 Applet ID 입력
=================================================*/
int Connect_Applet() {

	//Applet ID
	BYTE cmd_SelectCryptoApplet[6] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0x01 };

	C_APDU[APDU_CLA] = 0x00;
	C_APDU[APDU_INS] = 0xA4;
	C_APDU[APDU_P1] = 0x04;
	C_APDU[APDU_P2] = 0x00;
	C_APDU[APDU_LC] = 0x06;

	memcpy(C_APDU + APDU_DATA, cmd_SelectCryptoApplet, 6);

	if (iso7816.TransmitAPDU(C_APDU, 11, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}

/*=================================================
 int Generate_AES128Key
 : SE 내의 AES128 Key 생성 및 NVM 내 저장
 : key_num = Key Reference (Maximum: 0x20)
=================================================*/
int Generate_AES128Key(int key_num) {
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x84;
	C_APDU[APDU_P1] = 0x09;
	C_APDU[APDU_P2] = (BYTE)key_num;
	C_APDU[APDU_LC] = 0x00;

	if (iso7816.TransmitAPDU(C_APDU, 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}

/*=================================================
 int Encrypt_AES128
 : SE 내 저장된 AES128 Key로 plain_data 암호화
 : key_num = Key Reference (Maximum: 0x20)
 : enc_data : SE에서 암호화된 데이터 저장 공간
=================================================*/
int Encrypt_AES128(int key_num, BYTE* plain_data, int plain_len, BYTE* enc_data, int* enc_len) {
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x81;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = (BYTE)key_num;
	C_APDU[APDU_LC] = (BYTE)plain_len;

	memcpy(C_APDU + APDU_DATA, plain_data, plain_len);

	if (iso7816.TransmitAPDU(C_APDU, plain_len + 5, enc_data, enc_len) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}

/*=================================================
 int Decrypt_AES128
 : SE 내 저장된 AES128 Key로 enc_data 복호화
 : key_num = Key Reference (Maximum: 0x20)
 : plain_data : SE에서 복호화된 데이터 저장 공간
=================================================*/
int Decrypt_AES128(int key_num, BYTE* enc_data, int enc_len, BYTE* plain_data, int* plain_len) {
	C_APDU[APDU_CLA] = 0x80;
	C_APDU[APDU_INS] = 0x82;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = (BYTE)key_num;
	C_APDU[APDU_LC] = (BYTE)enc_len;

	memcpy(C_APDU + APDU_DATA, enc_data, enc_len);

	if (iso7816.TransmitAPDU(C_APDU, enc_len + 5, plain_data, plain_len) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}

/*=================================================
int Generate_RSA1024Key
: SE 내의 RSA1024 Key Pair 생성 및 NVM 내 저장
: key_num = Key Reference (Maximum: 0x20)
=================================================*/
int Generate_RSA1024Key(int key_num) {
	C_APDU[APDU_CLA] = 0x80;            // CLA - Class
	C_APDU[APDU_INS] = 0x88;            // INS - Instruction: Generate RSA Key Pair
	C_APDU[APDU_P1] = 0x06;            // P1 - Parameter 1
	C_APDU[APDU_P2] = (BYTE)key_num;    // P2 - Parameter 2
	C_APDU[APDU_LC] = 0x00;             // Lc - no data for CRT key or 1 byte for non-CRT
	//C_APDU[APDU_DATA] = 0x01;           // Data absent - CRT, data present - non-CRT

	if (iso7816.TransmitAPDU(C_APDU, 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}

/*=================================================
int Encrypt_RSA1024
: SE 내의 RSA1024 public_key로 plain_data 암호화
: key_num = Key Reference (Maximum: 0x20)
: 암호화된 데이터는 enc_data에 저장
=================================================*/
int Encrypt_RSA1024(int key_num, int key_type, BYTE* plain_data, int plain_len, BYTE* enc_data, int* enc_len) {
	
	C_APDU[APDU_CLA] = 0x80;
	if (key_type == PUBLIC) {
		C_APDU[APDU_INS] = 0x89;
		C_APDU[APDU_P1] = 0x0A;
	}
	else { // key_type == PRIVATE
		C_APDU[APDU_INS] = 0x8B;
		C_APDU[APDU_P1] = 0x00;
	}
	C_APDU[APDU_P2] = (BYTE)key_num;
	C_APDU[APDU_LC] = (plain_len >= 256) ? 0xFF : (BYTE)plain_len;

	memcpy(C_APDU + APDU_DATA, plain_data, plain_len);

	if (iso7816.TransmitAPDU(C_APDU, plain_len + 5, enc_data, enc_len) && SW1SW2 == 0x9000) return TRUE;

	return FALSE;
}

/*=================================================
int Generate_RSA1024Key
: SE 내의 RSA1024 public_key로 enc_data 검증
: key_num = Key Reference (Maximum: 0x20)
: 복호화된 데이터는 plain_data에 저장
=================================================*/
int Decrypt_RSA1024(int key_num, int key_type, BYTE* enc_data, int enc_len, BYTE* plain_data, int* plain_len) {
	
	C_APDU[APDU_CLA] = 0x90;
	if (key_type == PRIVATE) {
		C_APDU[APDU_INS] = 0x8A;
		C_APDU[APDU_P1] = 0x0A;
	}
	else { // key_type == PUBLIC
		C_APDU[APDU_INS] = 0x8C;
		C_APDU[APDU_P1] = 0x00;
	}
	C_APDU[APDU_P2] = (BYTE)key_num;
	C_APDU[APDU_LC] = 64;

	memcpy(C_APDU + APDU_DATA, enc_data, 64);

	if (iso7816.TransmitAPDU(C_APDU, 64 + 5, NULL, NULL) && SW1SW2 == 0x9000) {}
	else return FALSE;

	BYTE buffer[64];
	for (int i = 0; i < 64; i++) buffer[i] = enc_data[i + 64];

	memcpy(C_APDU + APDU_DATA, buffer, 64);

	if (iso7816.TransmitAPDU(C_APDU, 64 + 5, plain_data, plain_len) && SW1SW2 == 0x9000) return TRUE;
	else return FALSE;
}



bool Init_SE() {
	// Initialize Serial 
	int ret;
	struct termios options;
	if ((Serial3 = serialOpen("/dev/serial0", 9600)) < 0)
		return FALSE;

	tcgetattr(Serial3, &options);
	options.c_cflag &= ~CSIZE;	// Mask Character Size
	options.c_cflag |= CS8;		// Character Size 8
	options.c_cflag |= PARENB;	// Even Parity
	options.c_cflag |= CSTOPB;	// Stop Bit 2
	
	ret = tcsetattr(Serial3, TCSANOW, &options);
	if (ret != 0)
		printf("error %d from tcsetattr", errno);
	iso7816.Initialize(NULL);


 	if (iso7816.OpenCard() && Connect_Applet()) {
		printf("SE Connection Success\n");
		return TRUE;
	}
	else return FALSE;
}


// 데이터가 SE로 제대로 전송했는지 확인하는 API
int APDU_Test(BYTE* data, int data_len) {

	C_APDU[APDU_CLA] = 0x90;
	C_APDU[APDU_INS] = 0x0A;
	C_APDU[APDU_P1] = 0x00;
	C_APDU[APDU_P2] = 0x00;
	C_APDU[APDU_LC] = (BYTE)data_len;

	memcpy(C_APDU + APDU_DATA, data, data_len);

	if (iso7816.TransmitAPDU(C_APDU, data_len + 5, NULL, NULL) && SW1SW2 == 0x9000) return TRUE;
	return FALSE;
}
#endif
