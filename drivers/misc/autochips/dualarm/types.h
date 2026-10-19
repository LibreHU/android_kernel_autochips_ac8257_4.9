/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * AutoChips Inc. (C) 2016. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */
/*-----------------------------------------------------------------------------
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#ifndef _TYPES_H
#define _TYPES_H

/*-----------------------------------------------------------------------------
                    macros, defines, typedefs, enums
 ----------------------------------------------------------------------------*/
//binyang 

#if 0
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef _Bool			bool;

typedef unsigned int size_t;

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef u32 phys_addr_t;

#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef		__u8		u_int8_t;
typedef		__s8		int8_t;
typedef		__u16		u_int16_t;
typedef		__s16		int16_t;
typedef		__u32		u_int32_t;
typedef		__s32		int32_t;

#endif /* !(__BIT_TYPES_DEFINED__) */

typedef		__u8		uint8_t;
typedef		__u16		uint16_t;
typedef		__u32		uint32_t;

#if defined(__GNUC__)
typedef		__u64		uint64_t;
typedef		__u64		u_int64_t;
typedef		__s64		int64_t;
#endif

#endif
/////////////////////////////////////////

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
//#typedef unsigned long  UINT32;
typedef unsigned int UINT32;
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
//typedef signed long  INT32;
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
typedef void * HANDLE;
typedef UINT16 HANDLE_TYPE_T;
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
typedef UINT32  HANDLE_T;

typedef unsigned int SIZE_T;
typedef unsigned int HRESULT;

#if 0
typedef unsigned long		uintptr_t;
#endif

#ifdef __linux__
typedef struct _DBGPARAM {
   CHAR   lpszName[32];
   CHAR   rglpszZones[16][32];
   ULONG   ulZoneMask;
} DBGPARAM, *LPDBGPARAM;
#else
typedef struct _DBGPARAM {
   WCHAR   lpszName[32];
   WCHAR   rglpszZones[16][32];
   ULONG   ulZoneMask;
} DBGPARAM, *LPDBGPARAM;
#endif 

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
