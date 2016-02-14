/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2014/12/04
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014 Christopher Gurnee.  All rights reserved.
 *
 * This is a wrapper for the MD4, MD5, SHA1, and CRC32 algorithms supported
 * natively by the Windows API, plus the included SHA-256 library.
 **/

#ifndef __WINHASH_H__
#define __WINHASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "SwapIntrinsics.h"

typedef CONST BYTE *PCBYTE;

/**
 * Structures used by the system libraries
 **/

typedef struct {
	UINT32 state[4];
	UINT64 count;
	BYTE buffer[64];
	BYTE result[16];
} MD4_CTX, *PMD4_CTX, MD5_CTX, *PMD5_CTX;

typedef struct {
	UINT32 state[5];
	UINT64 count;
	BYTE buffer[64];
	BYTE result[20];
} SHA1_CTX, *PSHA1_CTX;

typedef struct _SHA2_CTX {
	union {
		UINT32	st32[8];
		UINT64	st64[8];
	} state;
	UINT64 bitcount[2];
	BYTE buffer[128];
	BYTE result[64];
} SHA2_CTX, *PSHA2_CTX;


UINT32 crc32( UINT32 uInitial, PCBYTE pbIn, UINT cbIn );

void MD4Init( PMD4_CTX pContext );
void MD4Update( PMD4_CTX pContext, PCBYTE pbIn, UINT cbIn );
void MD4Final( PMD4_CTX pContext );

void MD5Init( PMD5_CTX pContext );
void MD5Update( PMD5_CTX pContext, PCBYTE pbIn, UINT cbIn );
void MD5Final( PMD5_CTX pContext );

void SHA1Init( PSHA1_CTX pContext );
void SHA1Update( PSHA1_CTX pContext, PCBYTE pbIn, UINT cbIn );
void SHA1Final( PSHA1_CTX pContext );

void SHA256Init( PSHA2_CTX pContext );
void SHA256Update( PSHA2_CTX pContext, PCBYTE pbIn, UINT cbIn );
void SHA256Final( PSHA2_CTX pContext );

/**
 * Structures used by our consistency wrapper layer
 **/

typedef union {
	UINT32 state;
	BYTE result[4];
} WHCTXCRC32, *PWHCTXCRC32;

#define  WHCTXMD4  MD4_CTX
#define PWHCTXMD4 PMD4_CTX
#define  WHCTXMD5  MD5_CTX
#define PWHCTXMD5 PMD5_CTX

#define  WHCTXSHA1  SHA1_CTX
#define PWHCTXSHA1 PSHA1_CTX

#define  WHCTXSHA256  SHA2_CTX
#define PWHCTXSHA256 PSHA2_CTX

typedef struct {
	MD4_CTX ctxList;
	MD4_CTX ctxChunk;
	PBYTE result;
	UINT cbChunkRemaining;
} WHCTXED2K, *PWHCTXED2K;

/**
 * Wrapper layer functions to ensure a more consistent interface
 **/

__forceinline VOID WHInitCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = 0;
}

__forceinline VOID WHUpdateCRC32( PWHCTXCRC32 pContext, PCBYTE pbIn, UINT cbIn )
{
	pContext->state = crc32(pContext->state, pbIn, cbIn);
}

__forceinline VOID WHFinishCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = SwapV32(pContext->state);
}

#define WHInitMD4 MD4Init
#define WHUpdateMD4 MD4Update
#define WHFinishMD4 MD4Final

#define WHInitMD5 MD5Init
#define WHUpdateMD5 MD5Update
#define WHFinishMD5 MD5Final

#define WHInitSHA1 SHA1Init
#define WHUpdateSHA1 SHA1Update
#define WHFinishSHA1 SHA1Final

#define WHInitSHA256 SHA256Init
#define WHUpdateSHA256 SHA256Update
#define WHFinishSHA256 SHA256Final

__forceinline VOID WHInitED2K( PWHCTXED2K pContext )
{
	MD4Init(&pContext->ctxList);
	MD4Init(&pContext->ctxChunk);
	pContext->cbChunkRemaining = 9500 << 10;
	pContext->result = pContext->ctxChunk.result;
}

__forceinline VOID WHUpdateED2K( PWHCTXED2K pContext, PCBYTE pbIn, UINT cbIn )
{
	if (cbIn >= pContext->cbChunkRemaining)
	{
		// Finish off the current chunk and add it to the list hash
		MD4Update(&pContext->ctxChunk, pbIn, pContext->cbChunkRemaining);
		MD4Final(&pContext->ctxChunk);
		MD4Update(&pContext->ctxList, pContext->ctxChunk.result, sizeof(pContext->ctxChunk.result));
		pbIn += pContext->cbChunkRemaining;
		cbIn -= pContext->cbChunkRemaining;

		// Reset the chunk context
		MD4Init(&pContext->ctxChunk);
		pContext->cbChunkRemaining = 9500 << 10;

		// The final result will now be the list hash, not the chunk hash
		pContext->result = pContext->ctxList.result;
	}

	MD4Update(&pContext->ctxChunk, pbIn, cbIn);
	pContext->cbChunkRemaining -= cbIn;
}

__forceinline VOID WHFinishED2K( PWHCTXED2K pContext )
{
	MD4Final(&pContext->ctxChunk);
	MD4Update(&pContext->ctxList, pContext->ctxChunk.result, sizeof(pContext->ctxChunk.result));
	MD4Final(&pContext->ctxList);
}

/**
 * WH*To* hex string conversion functions: These require WinHash.c
 **/

#define WHAPI __fastcall

#define WHFMT_UPPERCASE 0x00
#define WHFMT_LOWERCASE 0x20

BOOL WHAPI WHHexToByte( PTSTR pszSrc, PBYTE pbDest, UINT cchHex );
PTSTR WHAPI WHByteToHex( PBYTE pbSrc, PTSTR pszDest, UINT cchHex, UINT8 uCaseMode );

/**
 * WH*Ex functions: These require WinHash.c
 **/

typedef struct {
	TCHAR szHexCRC32[9];
	TCHAR szHexMD4[33];
	TCHAR szHexMD5[33];
	TCHAR szHexSHA1[41];
	TCHAR szHexSHA256[65];
} WHRESULTEX, *PWHRESULTEX;

typedef struct {
	UINT8      flags;
	UINT8      uCaseMode;
	WHCTXCRC32 ctxCRC32;
	WHCTXMD4   ctxMD4;
	WHCTXMD5   ctxMD5;
	WHCTXSHA1  ctxSHA1;
	WHCTXSHA256 ctxSHA256;
	WHRESULTEX results;
} WHCTXEX, *PWHCTXEX;

#define WHEX_CHECKCRC32 0x01
#define WHEX_CHECKMD4   0x02
#define WHEX_CHECKMD5   0x04
#define WHEX_CHECKSHA1  0x08
#define WHEX_CHECKSHA256 0x10
#define WHEX_ALL        0x1F
#define WHEX_ALL32      0x01
#define WHEX_ALL128     0x06
#define WHEX_ALL160     0x08
#define WHEX_ALL256     0x10

VOID WHAPI WHInitEx( PWHCTXEX pContext );
VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn );
VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults );

#ifdef __cplusplus
}
#endif

#endif
