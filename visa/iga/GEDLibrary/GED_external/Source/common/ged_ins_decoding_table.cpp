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

#include <cstring>
#include "common/ged_base.h"
#include "common/ged_ins_decoding_table.h"

using std::memcmp;
using std::memset;


const char* ged_ins_decoding_table_t_str = "ged_ins_decoding_table_t";


const char* gedTableEntryTypeStrings[GED_TABLE_ENTRY_TYPE_SIZE] =
{
    "GED_TABLE_ENTRY_TYPE_CONSECUTIVE",
    "GED_TABLE_ENTRY_TYPE_FRAGMENTED",
    "GED_TABLE_ENTRY_TYPE_FIXED_VALUE",
    "GED_TABLE_ENTRY_TYPE_NEXT_TABLE",
    "GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED"
};


const char* gedTableEntryTypePadding[GED_TABLE_ENTRY_TYPE_SIZE] =
{
    "  ",
    "   ",
    "  ",
    "   ",
    ""
};


const char* ged_ins_field_entry_t_str = "ged_ins_field_entry_t";


bool ged_ins_field_entry_t::operator==(const ged_ins_field_entry_t& rhs) const
{
    if (_field != rhs._field) return false;
    if (_entryType != rhs._entryType) return false;
    return (0 == memcmp(this, &rhs, sizeof(ged_ins_field_entry_t)));
}


bool ged_ins_field_entry_t::operator<(const ged_ins_field_entry_t& rhs) const
{
    if (_field != rhs._field) return (_field < rhs._field);
    if (_entryType != rhs._entryType) return (_entryType < rhs._entryType);
    return (memcmp(this, &rhs, sizeof(ged_ins_field_entry_t)) < 0);
}


void InitTableEntry(ged_ins_field_entry_t& entry, const /* GED_INS_FIELD */ uint32_t field)
{
    GEDASSERT(MAX_UINT16_T >= field);
    entry._field = field;
    entry._entryType = GED_TABLE_ENTRY_TYPE_NOT_SUPPORTED;
    entry._bitSize = 0;
    memset(&(entry._dummy), 0, sizeof(entry._dummy));
    entry._restrictions = NULL;
}


uint64_t MaxValue(const ged_ins_field_entry_t& entry)
{
    // TODO: This is a workaround for restricted fields with more than 64 bits. Remove this when we support fields wider than 64.
    uint8_t bitSize = entry._bitSize;
    if (GED_QWORD_BITS < bitSize)
    {
        bitSize = GED_QWORD_BITS;
    }
    return BitsToMaxValue(bitSize);

    // TODO: Original function implementation.
//    return BitsToMaxValue(entry._bitSize);
}


uint64_t MaxValue(const ged_ins_field_entry_t* entry)
{
    GEDASSERT(NULL != entry);
    return MaxValue(*entry);
}
