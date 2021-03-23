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

namespace USC
{

/*****************************************************************************\
STRUCT: ErrorCode
\*****************************************************************************/
struct ErrorCode
{
    // External values
    unsigned int       Success             : 1;    // Call was successful
    unsigned int       Error               : 1;    // Invalid call
    unsigned int       OutOfSystemMemory   : 1;    // System memory allocation failed
    unsigned int       Busy                : 1;    // Compilation not done yet
    unsigned int       Reserved            : 28;   // Reserved

};

/*****************************************************************************\
CONST: g_cInitRetVal
\*****************************************************************************/
const ErrorCode g_cInitErrorCode =
{
    true,   // Success
    false,  // Error
    false,  // OutOfSystemMemory
    false,  // Busy
    0       // Reserved
};

}