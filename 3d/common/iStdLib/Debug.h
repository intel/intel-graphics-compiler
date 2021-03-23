/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
