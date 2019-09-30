/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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

#endif   // End of file Driver_Model.h
