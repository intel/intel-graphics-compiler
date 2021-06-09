/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
