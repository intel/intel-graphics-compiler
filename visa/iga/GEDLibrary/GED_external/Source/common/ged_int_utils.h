/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

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

#ifndef GED_INT_UTILS_H
#define GED_INT_UTILS_H

#include "common/ged_types_internal.h"


/*!
 * Get the maximum possible value for an unsigned integer consisting of the given number of bits. This function only supports integers
 * up to qwords i.e. 64-bit wide integers.
 *
 * @param[in]   size    The number of bits.
 *
 * @return      The maximal value an integer of "size" bits.
 */
extern const uint64_t& BitsToMaxValue(const uint8_t size);


/*!
 * Get the number of values supported for an unsigned integer consisting of the given number of bits. Due to overflow, this function
 * only supports integers up to 31 bits wide. Due to overflow,
 *
 * @param[in]   size    The number of bits.
 *
 * @return      The number of values for an integer of "size" bits.
 */
extern const uint32_t& BitsToNumOfValues(const uint8_t size);

#endif // GED_INT_UTILS_H
