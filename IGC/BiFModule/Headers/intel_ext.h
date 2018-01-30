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

#ifndef __INTEL_EXT_H__
#define __INTEL_EXT_H__

// Intel precision enum
//   Precision: type of a value whose size <= 8 bits
typedef enum
{
    PRECISION_U1 = 0,      // unsigned 1 bit
    PRECISION_U2 = 1,      // unsigned 2 bits
    PRECISION_U4 = 2,      // unsigned 4 bits
    PRECISION_U8 = 3,      // unsigned 8 bits
    PRECISION_S1 = 4,      // signed 1 bit
    PRECISION_S2 = 5,      // signed 2 bits
    PRECISION_S4 = 6,      // signed 4 bits
    PRECISION_S8 = 7       // signed 8 bits
} Precision_t;

#endif // __INTEL_EXT_H__

