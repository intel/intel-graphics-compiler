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

#ifndef UFO_FAKE_SEH_H
#define UFO_FAKE_SEH_H

/**
 *  This file provides trivial mapping of Microsoft specific structured exception handling mechanism:
 *      __try
 *      __leave
 *      __finally
 *      __except(e)
 *  NOTE: This mapping will just enable compilation of your code. SEH will *NOT* work as on MSVC!
 */

#if !defined(_MSC_VER)

    // __try may be defined as internal macro in GNU C++ library
    #if defined(__cplusplus)
        // NOTE: Any C++ standard header must be included _before_ any attempt of keyword
        // redefinition (i.e. try/catch) in other case a fatal error is generated according
        // to C++11 standard.
        #include <exception>
    #endif

    #ifndef __try
        #define __try           { if(1)
        #define __leave         goto _end_of_guarded_block
        #define __finally       __leave; } _end_of_guarded_block: if(1)
        #define __except(e)     __leave; } _end_of_guarded_block: if(0)
    #elif defined(__catch)
        // __try is likely defined as "try" or "if(true)"
        // and __catch is likely defined as "catch" or "if(false)"
        #define __leave         goto _end_of_guarded_block
        #define __finally       __catch(...) { __leave; } _end_of_guarded_block: if(true)
        #define __except(e)     __catch(...) { __leave; } _end_of_guarded_block: if(false)
    #else
        #pragma message "FIXME unknown config (found __try without __catch)"
        #error "FIXME unknown config (found __try without __catch)"
    #endif

#endif

#endif  // UFO_FAKE_SEH_H
