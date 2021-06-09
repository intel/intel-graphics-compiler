/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cstring>
#include "common/ged_compact_mapping_table.h"

using std::memcmp;
using std::memset;


const char* ged_compact_mapping_table_t_str = "ged_compact_mapping_table_t";


const char* ged_compaction_table_entry_t_str = "ged_compaction_table_entry_t";


const char* gedCompactTableEntryTypeStrings[GED_MAPPING_TABLE_ENTRY_TYPE_SIZE] =
{
    "GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE",
    "GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED",
    "GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE",
    "GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED",
    "GED_MAPPING_TABLE_ENTRY_TYPE_NO_MAPPING",
    "GED_MAPPING_TABLE_ENTRY_TYPE_FIXED",
    "GED_MAPPING_TABLE_ENTRY_TYPE_NEXT_TABLE",
    "GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED"
};


const char* gedCompactTableEntryTypePadding[GED_MAPPING_TABLE_ENTRY_TYPE_SIZE] =
{
    "",
    " ",
    "",
    " ",
    "               ",
    "                    ",
    "               ",
    "            "
};


const char* gedCompactMappingTypeStrings[GED_COMPACT_MAPPING_TYPE_SIZE] =
{
    "GED_COMPACT_MAPPING_TYPE_1x1",
    "GED_COMPACT_MAPPING_TYPE_REP",
    "GED_COMPACT_MAPPING_TYPE_NO_MAPPING",
    "GED_COMPACT_MAPPING_TYPE_FIXED"
};


const char* gedCompactMappingTypePadding[GED_COMPACT_MAPPING_TYPE_SIZE] =
{
    "       ",
    "       ",
    "",
    "     "
};


const char* ged_compact_mapping_fragment_t_str = "ged_compact_mapping_fragment_t";


bool ged_compact_mapping_fragment_t::operator==(const ged_compact_mapping_fragment_t& rhs) const
{
    if (_mappingType != rhs._mappingType) return false;
    if (_from != rhs._from) return false;
    return (_to == rhs._to);
}


bool ged_compact_mapping_entry_t::operator==(const ged_compact_mapping_entry_t& rhs) const
{
    return (0 == memcmp(this, &rhs, sizeof(ged_compact_mapping_entry_t)));
}


const char* ged_compact_mapping_entry_t_str = "ged_compact_mapping_entry_t";


void InitTableEntry(ged_compact_mapping_entry_t& entry, /* GED_INS_FIELD */ uint32_t field)
{
    entry._field = field;
    entry._entryType = GED_MAPPING_TABLE_ENTRY_TYPE_NOT_SUPPORTED;
    memset(&(entry._dummy), 0, sizeof(entry._dummy));
    entry._compactionTable = NULL;
}


uint32_t MappingSourceBitSize(const ged_compact_mapping_entry_t* entryPtr)
{
    return MappingSourceBitSize(*entryPtr);
}


uint32_t MappingSourceBitSize(const ged_compact_mapping_entry_t& entry)
{
    switch (entry._entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
    {
        const ged_compact_mapping_single_fragment_t& data = entry._consecutive;
        if (0 == data._fromMask) return 0;
        uint32_t bitSize = 0;
        for (uint32_t bitMask = data._fromMask; 0 != bitMask; bitMask >>= 1, ++bitSize) { /* empty loop */ }
        return bitSize;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
    {
        const ged_compact_mapping_multiple_fragments_t& data = entry._fragmented;
        unsigned int bitSize = data._fragments[0]._from._highBit - data._fragments[0]._from._lowBit + 1;
        for (unsigned int i = 1; i < data._numOfMappingFragments; ++i)
        {
            bitSize += (data._fragments[i]._from._highBit - data._fragments[i]._from._lowBit + 1);
        }
        return bitSize;
    }
    default:
        break;
    }

    // The compact field does not have an explicit mapping, so return 0.
    return 0;
}


uint32_t MappingTargetBitSize(const ged_compact_mapping_entry_t* entryPtr)
{
    return MappingTargetBitSize(*entryPtr);
}


uint32_t MappingTargetBitSize(const ged_compact_mapping_entry_t& entry)
{
    switch (entry._entryType)
    {
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_CONSECUTIVE:
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_CONSECUTIVE:
    {
        const ged_compact_mapping_single_fragment_t& data = entry._consecutive;
        unsigned int bitSize = data._to._highBit - data._to._lowBit + 1;
        return bitSize;
    }
    case GED_MAPPING_TABLE_ENTRY_TYPE_VALUE_MAPPING_FRAGMENTED:
    case GED_MAPPING_TABLE_ENTRY_TYPE_INDEX_MAPPING_FRAGMENTED:
    {
        const ged_compact_mapping_multiple_fragments_t& data = entry._fragmented;
        unsigned int bitSize = data._fragments[0]._to._highBit - data._fragments[0]._to._lowBit + 1;
        for (unsigned int i = 1; i < data._numOfMappingFragments; ++i)
        {
            bitSize += (data._fragments[i]._to._highBit - data._fragments[i]._to._lowBit + 1);
        }
        return bitSize;
    }
    default:
        break;
    }

    // The compact field does not have an explicit mapping, so return 0.
    return 0;
}
