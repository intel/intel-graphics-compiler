/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/ged_base.h"
#include "common/ged_ins_position_fragment.h"


const uint32_t rightShiftedMasks[] =
{
    0x1,        // 1 bit:   high-low = 0
    0x3,        // 2 bits:  high-low = 1
    0x7,        // 3 bits:  ...
    0xf,        // 4 bits
    0x1f,       // 5 bits
    0x3f,       // 6 bits
    0x7f,       // 7 bits
    0xff,       // 8 bits
    0x1ff,      // 9 bits
    0x3ff,      // 10 bits
    0x7ff,      // 11 bits
    0xfff,      // 12 bits
    0x1fff,     // 13 bits
    0x3fff,     // 14 bits
    0x7fff,     // 15 bits
    0xffff,     // 16 bits
    0x1ffff,    // 17 bits
    0x3ffff,    // 18 bits
    0x7ffff,    // 19 bits
    0xfffff,    // 20 bits
    0x1fffff,   // 21 bits
    0x3fffff,   // 22 bits
    0x7fffff,   // 23 bits
    0xffffff,   // 24 bits
    0x1ffffff,  // 25 bits
    0x3ffffff,  // 26 bits
    0x7ffffff,  // 27 bits
    0xfffffff,  // 28 bits
    0x1fffffff, // 29 bits
    0x3fffffff, // 30 bits: ...
    0x7fffffff, // 31 bits: high-low=30
    0xffffffff  // 32 bits: high-low=31
};


const char* ged_ins_field_position_fragment_t_str = "ged_ins_field_position_fragment_t";


bool ged_ins_field_position_fragment_t::operator==(const ged_ins_field_position_fragment_t& rhs) const
{
    GEDASSERT(sizeof(ged_ins_field_position_fragment_t) == sizeof(uint64_t));
    return (*reinterpret_cast<const uint64_t*>(this) == *reinterpret_cast<const uint64_t*>(&rhs));
}



void FillPositionFragment(ged_ins_field_position_fragment_t* fragment, const uint8_t lowBit, const uint8_t highBit)
{
    GEDASSERT(highBit >= lowBit);
    fragment->_lowBit = lowBit;
    fragment->_highBit = highBit;
    fragment->_dwordIndex = lowBit / GED_DWORD_BITS;
    fragment->_shift = lowBit % GED_DWORD_BITS;
    fragment->_bitMask = rightShiftedMasks[(highBit - lowBit)] << fragment->_shift;
}


uint8_t FragmentSize(const ged_ins_field_position_fragment_t* fragmentPtr)
{
    return (fragmentPtr->_highBit - fragmentPtr->_lowBit + 1);
}


uint8_t FragmentSize(const ged_ins_field_position_fragment_t& fragment)
{
    return (fragment._highBit - fragment._lowBit + 1);
}


uint32_t MaxFragmentValue(const ged_ins_field_position_fragment_t& fragment)
{
    GEDASSERT(FragmentSize(fragment) <= GED_DWORD_BITS);
    GEDASSERT((uint64_t)MAX_UINT32_T >= BitsToMaxValue(FragmentSize(fragment)));
    return (uint32_t)BitsToMaxValue(FragmentSize(fragment));
}


uint32_t MaxFragmentValue(const ged_ins_field_position_fragment_t* fragment)
{
    return MaxFragmentValue(*fragment);
}
