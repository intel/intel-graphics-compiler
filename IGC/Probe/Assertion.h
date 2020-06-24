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

#ifndef IGC_PROBE_ASSERTION_H
#define IGC_PROBE_ASSERTION_H


#include <cassert>
#include <cstdlib>

#define IGC_ASSERT assert

#define IGC_ASSERT_MESSAGE(x, m, ...) IGC_ASSERT(x)

#define IGC_ASSERT_EXIT(x) \
    do \
    { \
        if(0 == (x)) \
        { \
            assert(0); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

#define IGC_ASSERT_EXIT_MESSAGE(x, m, ...) IGC_ASSERT_EXIT(x)


#endif // IGC_PROBE_ASSERTION_H
