/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef UFO_PORTABLE_WINDEF_H
#define UFO_PORTABLE_WINDEF_H

/*
 * This file provides definitions of Windows data types for non-Windows platforms.
 */

#ifdef _WIN32
#error "This file is shall not to be included in a windows build!"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> // needed for ssize_t

/*
 *  Comparison of C type sizes on different platforms
 *
 *  Type        win32    win64    ubuntu32    ubuntu64    android32     android64
 *  -----------------------------------------------------------------------------
 *  bool          1        1         1            1            1            1
 *  char          1        1         1            1            1            1
 *  short         2        2         2            2            2            2
 *  short int     2        2         2            2            2            2
 *  int           4        4         4            4            4            4
 *  long          4        4         4            8            4            8
 *  long int      4        4         4            8            4            8
 *  long long     8        8         8            8            8            8
 *  float         4        4         4            4            4            4
 *  double        8        8         8            8            8            8
 *  long double   8        8        12           16           12           16
 *  void          -        -         1            1            1            1
 *  wchar_t       2        2         4            4            4            4
 *  void*         4        8         4            8            4            8
 */

typedef int32_t     BOOL, *PBOOL, *LPBOOL;      //!< Boolean value, should be TRUE or FALSE
typedef uint8_t     BOOLEAN, *PBOOLEAN;         //!< Boolean value, should be TRUE or FALSE

#define TRUE        1
#define FALSE       0

typedef char        CHAR, *PCHAR;               //!< 8 bit signed char
typedef char        *LPSTR, TCHAR, *LPTSTR;     //!< Pointer to 8 bit signed chars
typedef const char  *PCSTR, *LPCSTR, *LPCTSTR;

typedef int8_t      INT8, *PINT8;               //!< 8 bit signed value

#if !defined TYPEDEF_DEFINED_UCHAR
#define TYPEDEF_DEFINED_UCHAR 1
typedef uint8_t     UCHAR;                      //!< 8 bit unsigned value
#endif

typedef uint8_t     *PUCHAR;                    //!< 8 bit unsigned value
typedef uint8_t     UINT8, *PUINT8;             //!< 8 bit unsigned value
typedef uint8_t     BYTE, *PBYTE, *LPBYTE;      //!< 8 bit unsigned value

typedef int16_t     INT16, *PINT16;             //!< 16 bit signed value

#if !defined TYPEDEF_DEFINED_SHORT
#define TYPEDEF_DEFINED_SHORT 1
typedef int16_t     SHORT;                      //!< 16 bit signed value
#endif

typedef int16_t     *PSHORT;                    //!< 16 bit signed value

typedef uint16_t    UINT16, *PUINT16;           //!< 16 bit unsigned value
typedef uint16_t    USHORT, *PUSHORT;           //!< 16 bit unsigned value
typedef uint16_t    WORD, *PWORD;               //!< 16 bit unsigned value

typedef int32_t     INT, *PINT;                 //!< 32 bit signed value
typedef int32_t     INT32, *PINT32;             //!< 32 bit signed value
typedef int32_t     LONG, *PLONG;               //!< 32 bit unsigned value

typedef uint32_t    UINT, *PUINT;               //!< 32 bit unsigned value
typedef uint32_t    UINT32, *PUINT32;           //!< 32 bit unsigned value
typedef uint32_t    ULONG, *PULONG;             //!< 32 bit unsigned value
typedef uint32_t    DWORD, *PDWORD, *LPDWORD;   //!< 32 bit unsigned value

typedef int64_t     INT64, *PINT64;             //!< 64 bit signed value
typedef int64_t     LONGLONG, *PLONGLONG;       //!< 64 bit signed value
typedef int64_t     LONG64, *PLONG64;           //!< 64 bit signed value

typedef uint64_t    UINT64, *PUINT64;           //!< 64 bit unsigned value
typedef uint64_t    ULONGLONG;                  //!< 64 bit unsigned value
typedef uint64_t    ULONG64;                    //!< 64 bit unsigned value
typedef uint64_t    QWORD, *PQWORD;             //!< 64 bit unsigned value

typedef float       FLOAT, *PFLOAT;             //!< Floating point value
typedef double      DOUBLE;

typedef size_t      SIZE_T;                     //!< unsigned size value
typedef ssize_t     SSIZE_T;                    //!< signed size value

typedef uintptr_t   ULONG_PTR;                  //!< unsigned long type used for pointer precision
typedef uintptr_t   UINT_PTR;
typedef intptr_t    INT_PTR;

/**/

typedef void        VOID, *PVOID, *LPVOID;      //!< Void
typedef const void  *LPCVOID;                   //!< Const pointer to void

#if 1
typedef void*       HANDLE;                     //!< Opaque handle comprehended only by the OS
typedef void**      PHANDLE;                    //!< Opaque handle comprehended only by the OS
#else // some modules require uintptr_t as handle
typedef uintptr_t   HANDLE, *PHANDLE;           //!< Opaque handle comprehended only by the OS
#endif

typedef void*       HINSTANCE;                  //!< Opaque handle comprehended only by the OS
typedef void*       HMODULE;                    //!< Opaque handle comprehended only by the OS
typedef void*       HHOOK;                      //!< Opaque handle comprehended only by the OS
typedef void*       HDC;                        //!< Opaque handle comprehended only by the OS

#if 0 // we can't define it here yet as some modules have conflicting requirements
typedef void*       HWND;                       //!< Opaque handle comprehended only by the OS
#endif

typedef uint32_t    HKEY;

typedef INT_PTR (*PROC)();
typedef INT_PTR (*FARPROC)();

#define IN
#define OUT
#define STDCALLTYPE

#define WINAPI

#ifndef APIENTRY
    #define APIENTRY
#endif

#define CONST const

#define INFINITE         0xFFFFFFFF
#define WAIT_OBJECT_0    0
#define WAIT_FAILED      0xFFFFFFFF
#define WAIT_TIMEOUT     0x00000102


/**/

    #define DUMMYSTRUCTNAME

#ifndef __MEDIA_PORTABLE_DATAYPE_DEFINED__
#ifndef __LARGE_INTEGER_STRUCT_DEFINED__
typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
#define __LARGE_INTEGER_STRUCT_DEFINED__
#endif // __LARGE_INTEGER_STRUCT_DEFINED__
#endif // __MEDIA_PORTABLE_DATAYPE_DEFINED__

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;


typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

#ifndef __MEDIA_PORTABLE_DATAYPE_DEFINED__
typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, *PRECT, *LPRECT;
#endif

typedef LONGLONG REFERENCE_TIME;


/**/
#ifndef __MEDIA_PORTABLE_DATAYPE_DEFINED__
typedef LONG HRESULT;

#define S_OK                    0
#define S_FALSE                 1
#define E_ABORT                 (0x80004004)
#define E_FAIL                  (0x80004005)
#define E_OUTOFMEMORY           (0x8007000E)
#define E_INVALIDARG            (0x80070057)
#endif

/**/

typedef LONG LSTATUS;
typedef LONG NTSTATUS, *PNTSTATUS;

#ifndef STATUS_SUCCESS
    #define STATUS_SUCCESS                  (0x0L)
    #define STATUS_UNSUCCESSFUL             (0xC0000001L)
    #define STATUS_NOT_SUPPORTED            (0xC00000BBL)
    #define STATUS_INVALID_PARAMETER        (0xC000000DL)
    #define STATUS_NOT_IMPLEMENTED          (0xC0000002L)
    #define STATUS_NO_SUCH_DEVICE           (0xC000000EL)
    #define STATUS_INVALID_DEVICE_REQUEST   (0xC0000010L)
    #define STATUS_BUFFER_TOO_SMALL         (0xC0000023L)
    #define STATUS_DEVICE_NOT_READY         (0xC00000A3L)
    #define STATUS_INVALID_OWNER            (0xC000005AL)
    #define STATUS_DATA_ERROR               (0xC000003EL)
#endif


/**/

#define VER_NT_WORKSTATION              0x0000001
#define VER_NT_DOMAIN_CONTROLLER        0x0000002
#define VER_NT_SERVER                   0x0000003


/**/
#ifndef __MEDIA_PORTABLE_DATAYPE_DEFINED__
typedef DWORD TP_WAIT_RESULT;
typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;
typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;
#endif

#define __UFO_PORTABLE_DATATYPE_DEFINED__

#endif  // #ifndef UFO_PORTABLE_WINDEF_H

