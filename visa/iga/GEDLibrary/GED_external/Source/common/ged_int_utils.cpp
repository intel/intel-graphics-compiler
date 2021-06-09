/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/ged_int_utils.h"
#include "common/ged_base.h"

static const uint64_t maxValues[] =
{
    0x0ULL,                 // 0 bits
    0x1ULL,                 // 1 bit
    0x3ULL,                 // 2 bits
    0x7ULL,                 // 3 bits
    0xfULL,                 // 4 bits
    0x1fULL,                // 5 bits
    0x3fULL,                // 6 bits
    0x7fULL,                // 7 bits
    0xffULL,                // 8 bits
    0x1ffULL,               // 9 bits
    0x3ffULL,               // 10 bits
    0x7ffULL,               // 11 bits
    0xfffULL,               // 12 bits
    0x1fffULL,              // 13 bits
    0x3fffULL,              // 14 bits
    0x7fffULL,              // 15 bits
    0xffffULL,              // 16 bits
    0x1ffffULL,             // 17 bits
    0x3ffffULL,             // 18 bits
    0x7ffffULL,             // 19 bits
    0xfffffULL,             // 20 bits
    0x1fffffULL,            // 21 bits
    0x3fffffULL,            // 22 bits
    0x7fffffULL,            // 23 bits
    0xffffffULL,            // 24 bits
    0x1ffffffULL,           // 25 bits
    0x3ffffffULL,           // 26 bits
    0x7ffffffULL,           // 27 bits
    0xfffffffULL,           // 28 bits
    0x1fffffffULL,          // 29 bits
    0x3fffffffULL,          // 30 bits
    0x7fffffffULL,          // 31 bits
    0xffffffffULL,          // 32 bits
    0x1ffffffffULL,         // 33 bits
    0x3ffffffffULL,         // 34 bits
    0x7ffffffffULL,         // 35 bits
    0xfffffffffULL,         // 36 bits
    0x1fffffffffULL,        // 37 bits
    0x3fffffffffULL,        // 38 bits
    0x7fffffffffULL,        // 39 bits
    0xffffffffffULL,        // 40 bits
    0x1ffffffffffULL,       // 41 bits
    0x3ffffffffffULL,       // 42 bits
    0x7ffffffffffULL,       // 43 bits
    0xfffffffffffULL,       // 44 bits
    0x1fffffffffffULL,      // 45 bits
    0x3fffffffffffULL,      // 46 bits
    0x7fffffffffffULL,      // 47 bits
    0xffffffffffffULL,      // 48 bits
    0x1ffffffffffffULL,     // 49 bits
    0x3ffffffffffffULL,     // 50 bits
    0x7ffffffffffffULL,     // 51 bits
    0xfffffffffffffULL,     // 52 bits
    0x1fffffffffffffULL,    // 53 bits
    0x3fffffffffffffULL,    // 54 bits
    0x7fffffffffffffULL,    // 55 bits
    0xffffffffffffffULL,    // 56 bits
    0x1ffffffffffffffULL,   // 57 bits
    0x3ffffffffffffffULL,   // 58 bits
    0x7ffffffffffffffULL,   // 59 bits
    0xfffffffffffffffULL,   // 60 bits
    0x1fffffffffffffffULL,  // 61 bits
    0x3fffffffffffffffULL,  // 62 bits
    0x7fffffffffffffffULL,  // 63 bits
    0xffffffffffffffffULL   // 64 bits
};


static const uint32_t numOfValues[] =
{
    0x0,        // 0 bits
    0x2,        // 1 bit
    0x4,        // 2 bits
    0x8,        // 3 bits
    0x10,       // 4 bits
    0x20,       // 5 bits
    0x40,       // 6 bits
    0x80,       // 7 bits
    0x100,      // 8 bits
    0x200,      // 9 bits
    0x400,      // 10 bits
    0x800,      // 11 bits
    0x1000,     // 12 bits
    0x2000,     // 13 bits
    0x4000,     // 14 bits
    0x8000,     // 15 bits
    0x10000,    // 16 bits
    0x20000,    // 17 bits
    0x40000,    // 18 bits
    0x80000,    // 19 bits
    0x10000,    // 20 bits
    0x200000,   // 21 bits
    0x400000,   // 22 bits
    0x800000,   // 23 bits
    0x1000000,  // 24 bits
    0x2000000,  // 25 bits
    0x4000000,  // 26 bits
    0x8000000,  // 27 bits
    0x10000000, // 28 bits
    0x20000000, // 29 bits
    0x40000000, // 30 bits
    0x80000000  // 31 bits
};


const uint64_t& BitsToMaxValue(const uint8_t size)
{
    GEDASSERT(GED_QWORD_BITS >= size);
    return maxValues[size];
}


const uint32_t& BitsToNumOfValues(const uint8_t size)
{
    GEDASSERT(GED_DWORD_BITS > size);
    return numOfValues[size];
}
