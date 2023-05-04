// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_TYPE_H_
#define _AVIO_TYPE_H_

typedef unsigned char UCHAR;
typedef char CHAR;
typedef unsigned int UINT;
typedef int INT;
typedef UCHAR UINT8;
typedef CHAR INT8;
typedef unsigned short UINT16;
typedef short INT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef void VOID;
typedef void *PVOID;
typedef signed int HRESULT;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long INT64;
typedef unsigned long long UINT64;

#ifndef E_SUC
#define E_SUC			(0x00000000)
#define E_ERR			(0x80000000)

// generic error code macro
#define E_GEN_SUC(code) (E_SUC | E_GENERIC_BASE | (code & 0x0000FFFF))
#define E_GEN_ERR(code) (E_ERR | E_GENERIC_BASE | (code & 0x0000FFFF))

#define S_OK			E_GEN_SUC(0x0000)	// Success
#define S_FALSE			E_GEN_SUC(0x0001)	// Success but return false status
#define E_FAIL			E_GEN_ERR(0x4005)	// Unspecified failure
#define E_OUTOFMEMORY	E_GEN_ERR(0x700E)	// Failed to allocate necessary memory

// error code base list
#define ERRORCODE_BASE	(0)
#define E_GENERIC_BASE	(0x0000 << 16)
#define E_SYSTEM_BASE	(0x0001 << 16)
#define E_DEBUG_BASE	(0x0002 << 16)
#endif

#endif //_AVIO_TYPE_H_
