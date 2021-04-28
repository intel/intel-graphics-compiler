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

#include <cstring>
#include "common/ged_base.h"
#include "common/ged_ins_encoding_masks.h"

using std::memcmp;


const char* ged_instruction_masks_table_t_str = "ged_instruction_masks_table_t";


const char* gedMaskTableEntryTypeStrings[GED_MASKS_TABLE_ENTRY_TYPE_SIZE] =
{
    "GED_MASKS_TABLE_ENTRY_TYPE_MASKS",
    "GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE",
    "GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS"
};


const char* gedMaskTableEntryTypePadding[GED_MASKS_TABLE_ENTRY_TYPE_SIZE] =
{
    "     ",
    "",
    "  "
};


bool ged_instruction_masks_t::operator==(const ged_instruction_masks_t& cmp) const
{
    if (0 != memcmp(_or, cmp._or, GED_NATIVE_INS_SIZE)) return false;
    return (0 == memcmp(_and, cmp._and, GED_NATIVE_INS_SIZE));
}


bool ged_instruction_masks_next_table_t::operator==(const ged_instruction_masks_next_table_t& cmp) const
{
    if (_tableKey != cmp._tableKey)
    {
        return false;
    }
    return (_tablePtr == cmp._tablePtr);
}


bool ged_instruction_masks_entry_t::operator==(const ged_instruction_masks_entry_t& cmp) const
{
    if (_entryType != cmp._entryType) return false;
    if (GED_MASKS_TABLE_ENTRY_TYPE_MASKS == _entryType) return _masks == cmp._masks;
    if (GED_MASKS_TABLE_ENTRY_TYPE_NEXT_TABLE == _entryType) return _nextTable == cmp._nextTable;
    GEDASSERT(GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS == _entryType);
    GEDASSERT(noMasks._masks == _masks);
    GEDASSERT(noMasks._masks == cmp._masks);
    return true;
}


const char* ged_instruction_masks_entry_t_str = "ged_instruction_masks_entry_t";

#if defined(TARGET_IA32)
const ged_instruction_masks_entry_t emptyMasks = { GED_MASKS_TABLE_ENTRY_TYPE_MASKS, {
    // or-mask
    (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000,
    // and-mask
    (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff }
};
const ged_instruction_masks_entry_t noMasks = { GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS, {
    // or-mask
    (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000, (void*)(g_uintptr_t)0x00000000,
    // and-mask
    (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff, (void*)(g_uintptr_t)0xffffffff }
};

#elif defined (TARGET_INTEL64)

const ged_instruction_masks_entry_t emptyMasks = { GED_MASKS_TABLE_ENTRY_TYPE_MASKS, {
    // or-mask
    (void*)(g_uintptr_t)0x0000000000000000, (void*)(g_uintptr_t)0x0000000000000000,
    // and-mask
    (void*)(g_uintptr_t)0xffffffffffffffff, (void*)(g_uintptr_t)0xffffffffffffffff }
};

const ged_instruction_masks_entry_t noMasks = { GED_MASKS_TABLE_ENTRY_TYPE_NO_MASKS, {
    // or-mask
    (void*)(g_uintptr_t)0x0000000000000000, (void*)(g_uintptr_t)0x0000000000000000,
    // and-mask
    (void*)(g_uintptr_t)0xffffffffffffffff, (void*)(g_uintptr_t)0xffffffffffffffff }
};
#endif // TARGET_INTEL64
