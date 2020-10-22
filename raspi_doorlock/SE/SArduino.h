/*****************************************************************************
 * Secure Device API
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
#ifndef INCLUDED_SECDEVAPI_H
#define INCLUDED_SECDEVAPI_H

#include "HAL.h"
#include "ISO7816.h"
#include <wiringPi.h>

#define ALG_ECC_P_192    0x00
#define ALG_DES_ECB_128  0x01		
#define ALG_DES_CBC_128  0x02		
#define ALG_DES_ECB_192  0x03		
#define ALG_DES_CBC_192  0x04		
#define ALG_RSA3072      0x05		
#define ALG_RSA1024      0x06		
#define ALG_RSA2048      0x07		
#define ALG_AES_ECB_128  0x08		
#define ALG_AES_CBC_128  0x09		
#define ALG_AES_ECB_192  0x0A		
#define ALG_AES_CBC_192  0x0B		
#define ALG_AES_ECB_256  0x0C		
#define ALG_AES_CBC_256  0x0D		
#define ALG_ECC_P_224    0x0E		
#define ALG_ECC_K_233    0x0F		
#define ALG_ECC_B_233    0x10		
#define ALG_ECC_P_256    0x11		
#define ALG_ECC_K_283    0x12		
#define ALG_ECC_B_283    0x13		
#define ALG_ECC_P_384    0x14		
#define ALG_ECC_P_521    0x15		

#define SHA1         0x01		
#define SHA256       0x04
#define SHA384       0x05
#define SHA512       0x06

#define KEY_ID_AES128  0x00
#define KEY_ID_AES256  0x01
#define KEY_ID_ECC192  0x05
#define KEY_ID_ECC224  0x06
#define KEY_ID_ECC256  0x07
#define KEY_ID_RSA1024 0x0A
#define KEY_ID_RSA2048 0x0B

int SecDev_Open(void);
int SecDev_LoadApplet(void);

#if 1
int SecDev_PutKey(BYTE   algid,  // ALG:   09=AES_CBC_128  0D=AES_CBC_256
                  BYTE   keyid,  // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                  BYTE*  key,    // Pointer to AES key array
                  int    keylen);// AES key length

int SecDev_Encrypt_Symm(BYTE   keyid,       // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *outdata,     // Pointer to input data array
                        int   *outdatalen); // Length of input data

int SecDev_Decrypt_Symm(BYTE   keyid,       // KEYID: 00=AES_CBC_128  01=AES_CBC_256
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *outdata,     // Pointer to input data array
                        int   *outdatalen); // Length of input data

int SHA_256             (BYTE  *plain_data,      // Pointer to input data array
                        int    plain_len,   // Length of input data
                        BYTE  *digest,      // Pointer to digest data array
                        int   *digest_len);  // Length of digest

int SecDev_RSA_GenKeyPair(BYTE   algid,    // ALG:   07=RSA2048/CRT  87=RSA2048/nonCRT
                          BYTE   keyid,    // KEYID: 09=RSA2K/CRT    0B=RSA2K/nonCRT
                          BYTE  *pubkey,   // Pointer to RSA public key array
                          int   *pubkeylen);// RSA public key length

int SecDev_RSA_Sign    (BYTE   keyid,       // KEYID: 16=RSA2K/CRT  18=RSA2K/nonCRT
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int   *signaturelen);// Length of signature

int SecDev_RSA_Verify  (BYTE   keyid,       // RSA key reference
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int    signaturelen);// Length of signature

int SecDev_ECC_GenKeyPair(BYTE   algid,    // ALG:   00=ECC192  0E=ECC224  11=ECC256
                          BYTE   keyid,    // KEYID: 01=ECC192  02=ECC224  03=ECC256 (example)
                          BYTE  *pubkey,   // Pointer to RSA public key array
                          int   *pubkeylen);// RSA public key length

int SecDev_ECDSA_Sign  (BYTE   keyid,       // ECC key reference
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int   *signaturelen);// Length of signature

int SecDev_ECDSA_Verify(BYTE   keyid,       // ECC key reference
                        BYTE  *indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *signature,   // Pointer to signature data array
                        int    signaturelen);// Length of signature

int SecDev_FileSystem_CreateFile (WORD fileid,  // File Identifier
		                          int  filesize);// File size in bytes

int SecDev_FileSystem_SelectFile (WORD fileid); // File Identifier

int SecDev_FileSystem_WriteFile(WORD   fileid,  // File Identifier
								int    offset,  // Offset in file
								BYTE  *data,    // Pointer to a data array to be written
								int    datalen);// Length of data data to be written

int SecDev_FileSystem_ReadFile (WORD   fileid,  // File Identifier
								int    offset,  // Offset in file
								BYTE  *data,    // Pointer to a data array to store the result
								int    datalen);// Length of data data to be read

int SecDev_RSA_Init();

int SecDev_RSA_Import(BYTE P1, BYTE* data, int len);

int SecDev_RSA_Encrypt(BYTE* data, int len, BYTE* data2, int* len2);

int SecDev_RSA_Decrypt(BYTE* data, int len, BYTE* data2, int* len2);

int SecDev_SHA2         (BYTE   algid,       // 01=SHA1, 04=SHA_256, 05=SHA384, 06=SHA512
                        BYTE  indata,      // Pointer to input data array
                        int    indatalen,   // Length of input data
                        BYTE  *digest,      // Pointer to digest data array
                        int   *digestlen,
			BYTE check);

//SArduino API
int SecDev_Req_VP_Verify(BYTE* pbInBuf, int InBufLen);
int SecDev_Req_FW_Verify(BYTE* pbInBuf, int InBufLen);
int SecDev_Req_Gen_ECC(BYTE* PbOutBuf, int OutBufLen);
int SecDev_Req_Gen_secKey(BYTE* pbInBuf, int InBufLen, BYTE* pbOutBuf, int outBufLen);
int SecDev_Req_Nonce(BYTE* PbOutBuf, int OutBufLen);

//IotSW API
bool Init_SE();
int Connect_Applet();

int Generate_AES128Key(int key_num);
int Encrypt_AES128(int key_num, BYTE* plain_data, int plain_len, BYTE* enc_data, int* enc_len);
int Decrypt_AES128(int key_num, BYTE* enc_data, int enc_len, BYTE* plain_data, int* plain_len);

int Generate_RSA1024Key(int key_num);
int Encrypt_RSA1024(int key_num, int key_type, BYTE* plain_data, int plain_len, BYTE* enc_data, int* enc_len);
int Decrypt_RSA1024(int key_num, int key_type, BYTE* enc_data, int enc_len, BYTE* plain_data, int* plain_len);

int APDU_Test(BYTE* data, int data_len);

#endif
#endif // INCLUDED_SECDEVAPI_H

