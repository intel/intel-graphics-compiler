/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifdef __cplusplus

/*****************************************************************************\
MACRO: ASSERT
\*****************************************************************************/
#ifndef ASSERT
#define ASSERT( expr )
#endif

/*****************************************************************************\
MACRO: DPF
PURPOSE: Debug Print function
\*****************************************************************************/
#ifndef DPF
#define DPF( debugLevel, message, ...)    
#endif

/*****************************************************************************\
MACRO: GFXDBG_STDLIB
PURPOSE: Special Debug Print flag for iSTD classes
\*****************************************************************************/
#define GFXDBG_STDLIB   (0x00001000)


/*****************************************************************************\
MACRO: NODEFAULT
PURPOSE: The use of __assume(0) tells the optimizer that the default case
cannot be reached. As a result, the compiler does not generate code to test
whether p has a value not represented in a case statement. Note that
__assume(0) must be the first statement in the body of the default case for
this to work.
\*****************************************************************************/
#ifndef NODEFAULT
#ifdef _MSC_VER
#define NODEFAULT   __assume(0)
#else
#define NODEFAULT 
#endif // _MSC_VER
#endif // NODEFAULT

#endif // __cplusplus
