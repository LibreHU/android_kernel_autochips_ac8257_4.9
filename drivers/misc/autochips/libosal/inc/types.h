/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
/*-----------------------------------------------------------------------------
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#ifndef _TYPES_H
#define _TYPES_H
//#include  "windev.h"

/*-----------------------------------------------------------------------------
                    macros, defines, typedefs, enums
 ----------------------------------------------------------------------------*/
#ifndef VOID
#define VOID void
#endif

#if !defined (_NO_TYPEDEF_BYTE_) && !defined (_TYPEDEF_BYTE_)
typedef unsigned char  BYTE;
#define _TYPEDEF_BYTE_
#endif

#if !defined (_NO_TYPEDEF_UCHAR_) && !defined (_TYPEDEF_UCHAR_)
typedef unsigned char  UCHAR;
#define _TYPEDEF_UCHAR_
#endif

#if !defined (_NO_TYPEDEF_UINT8_) && !defined (_TYPEDEF_UINT8_)
typedef unsigned char  UINT8;
#define _TYPEDEF_UINT8_
#endif

#if !defined (_NO_TYPEDEF_UINT16_) && !defined (_TYPEDEF_UINT16_)
typedef unsigned short  UINT16;
#define _TYPEDEF_UINT16_
#endif

#if !defined (_NO_TYPEDEF_UINT32_) && !defined (_TYPEDEF_UINT32_)
typedef unsigned long  UINT32;
#define _TYPEDEF_UINT32_
#endif

#if !defined (_NO_TYPEDEF_UINT64_) && !defined (_TYPEDEF_UINT64_)
typedef unsigned long long  UINT64;
#define _TYPEDEF_UINT64_
#endif

#if !defined (_NO_TYPEDEF_CHAR_) && !defined (_TYPEDEF_CHAR_)
typedef char  CHAR;     // Debug, should be 'signed char'
#define _TYPEDEF_CHAR_
#endif

#if !defined (_NO_TYPEDEF_INT_) && !defined (_TYPEDEF_INT_)
typedef int INT;
#define _TYPEDEF_INT_
#endif

#if !defined (_NO_TYPEDEF_INT8_) && !defined (_TYPEDEF_INT8_)
typedef signed char  INT8;
#define _TYPEDEF_INT8_
#endif

#if !defined (_NO_TYPEDEF_INT16_) && !defined (_TYPEDEF_INT16_)
typedef signed short  INT16;
#define _TYPEDEF_INT16_
#endif

#if !defined (_NO_TYPEDEF_INT32_) && !defined (_TYPEDEF_INT32_)
typedef signed long  INT32;
#define _TYPEDEF_INT32_
#endif

#if !defined (_NO_TYPEDEF_INT64_) && !defined (_TYPEDEF_INT64_)
typedef signed long long  INT64;
#define _TYPEDEF_INT64_
#endif

/* Define a boolean as 8 bits. */
#if !defined (_NO_TYPEDEF_BOOL_) && !defined (_TYPEDEF_BOOL_)
typedef UINT32  BOOL;
#define _TYPEDEF_BOOL_
#endif

#if !defined (_NO_TYPEDEF_FLOAT_) && !defined (_TYPEDEF_FLOAT_)
typedef float  FLOAT;
#define _TYPEDEF_FLOAT_
#endif

#if !defined (_NO_TYPEDEF_DOUBLE_)  && !defined (_TYPEDEF_DOUBLE_)
typedef double  DOUBLE;
#define _TYPEDEF_DOUBLE_
#endif

#if !defined (_NO_TYPEDEF_DWRD_)  && !defined (_TYPEDEF_DWRD_)
typedef unsigned long DWRD;
#define _TYPEDEF_DWRD_
#endif

#if !defined (_NO_TYPEDEF_WORD_)  && !defined (_TYPEDEF_WORD_)
typedef unsigned short    WORD;
#define _TYPEDEF_WORD_
#endif

#ifndef UNUSED
#define UNUSED(x)               (void)x
#endif

#ifndef MIN
#define MIN(x, y)               (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y)               (((x) > (y)) ? (x) : (y))
#endif

#ifndef ABS
#define ABS(x)                  (((x) >= 0) ? (x) : -(x))
#endif

#ifndef DIFF
#define DIFF(x, y)              (((x) > (y)) ? ((x) - (y)) : ((y) - (x)))
#endif

#ifndef NULL
    #define NULL                0
#endif  // NULL

#ifndef TRUE
    #define TRUE                (0 == 0)
#endif  // TRUE

#ifndef FALSE
    #define FALSE               (0 != 0)
#endif  // FALSE


#ifdef __arm
    #define INLINE              __inline
    #define IRQ                 __irq
    #define FIQ                 __irq
#else
    #define INLINE
    #define IRQ
    #define FIQ
#endif // __arm



#ifndef EXTERN
    #ifdef __cplusplus
        #define EXTERN          extern "C"
    #else
        #define EXTERN          extern
    #endif
#endif  // EXTERN

#define __align(x)			__attribute__((aligned(x)))

typedef unsigned short USHORT;
typedef unsigned int DWORD;
typedef unsigned int  UINT  ;
typedef unsigned long ULONG;
typedef char TCHAR;

typedef INT8    *PINT8;
typedef UINT8   *PUINT8;
typedef INT16   *PINT16;
typedef UINT16  *PUINT16;
typedef INT32   *PINT32;
typedef UINT32  *PUINT32;
typedef INT64   *PINT64;
typedef UINT64  *PUINT64;


typedef long *PLONG;
typedef const void * PCVOID;
typedef void ** PPVOID;

typedef unsigned short WCHAR;
typedef long LONG;
//typedef unsigned long ULONG_PTR;
typedef unsigned long *PULONG;
typedef ULONG *PSIZE_T;

#ifdef KERNEL_STANDARD_API
#include <linux/types.h>
typedef u16 HANDLE_TYPE_T;
#else // old typdef
typedef UINT16 HANDLE_TYPE_T;
#endif

typedef void * HANDLE;
typedef HANDLE HPROCESS;
typedef HANDLE HTHREAD;
typedef CHAR * LPCHAR, *LPSTR;

typedef void * LPVOID;
typedef void *LPOVERLAPPED;
typedef void * PVOID;

//we don't use unicode in android c code
typedef char *LPTSTR;
typedef const char  *LPCTSTR ;
typedef CHAR *PBYTE;
typedef void *HKEY;
typedef void* LPSECURITY_ATTRIBUTES;
typedef const void * LPCVOID;

typedef const char* LPCSTR;
typedef char *LPWSTR;
typedef const char *LPCWSTR;
typedef BOOL* LPBOOL;
typedef long *LPLONG;
typedef unsigned int * LPDWORD;
typedef unsigned int * PDWORD;
//typedef UINT32  HANDLE_T;

#if defined(__arm__)
#pragma message "For armv7a arch"
typedef unsigned int SIZE_T;
#elif defined(__LP64__) && defined(__aarch64__)
//#pragma message "For armv8 arch"
#ifndef __KERNEL__
#include <stddef.h>
#else
#include <linux/types.h>
#endif
typedef size_t SIZE_T;
#endif

typedef unsigned int HRESULT;
#if 0

typedef struct tagBITMAPINFOHEADER{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
}RECT, *PRECT;
#endif
#endif
