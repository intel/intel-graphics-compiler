/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
//  Filename : Driver_Model.h
//  Purpose  : Global definitions for many #defines and Macros to be used
//             By all portions of the driver
//
//  Date     : 7/11/2005
//  Date     : 4/04/2012  -  Removed XP support. Win7/Win8 only.
//=============================================================================

#ifndef  _DRIVER_MODEL_H_
#define  _DRIVER_MODEL_H_

// Info to determine version of OS for build

#ifdef _WIN32

    // These symbols should be taken from sdkddkver.h. But if not defined
    // for any reason or driver_model.h is included before sdkddkver.h,
    // do it here.
    #ifndef _WIN32_WINNT_WIN7
        #define _WIN32_WINNT_WIN7    0x0601
    #endif

    #ifdef WINVER
        #undef WINVER
    #endif

    #ifdef _WIN32_WINNT
       #undef _WIN32_WINNT
    #endif

    #undef  _NT_TARGET_VERSION  
    #undef  NTDDI_VERSION

    #define  _WIN32_WINNT   _WIN32_WINNT_WIN7
    #define WINVER  _WIN32_WINNT_WIN7
    #define  _NT_TARGET_VERSION  _WIN32_WINNT_WIN7
    #define NTDDI_VERSION   0x06010000

    #define LHDM    1
    #define XPDM    0
     
#else // #ifdef _WIN32

// Not a Windows OS
  #define LHDM    0
  #define XPDM    0

#endif // #ifdef _WIN32

#endif // _DRIVER_MODEL_H_
