/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MaxGRFTables.h"
#include "Compiler/CISACodeGen/Platform.hpp"

namespace IGC
{
    bool MaxGRFTable::MatchHWLocalID(HWLocalId fromTable, HWLocalId target)
    {
        // If target = '0', table entries with '0 or 1' and '0' are matched
        // If target = '1', table entries with '0 or 1' and '1' are matched
        if (fromTable == HWLocalId::EITHER ||
            fromTable == target)
            return true;
        else
            return false;
    }

    uint16_t MaxGRFTable::LookupMaxGRF(
        SIMDMode simt, HWLocalId hwlid, unsigned int witems,
        const vector<MaxGRFEntry>& table)
    {
        MaxGRFEntry entry;
        // Reverse order intentionally
        // GRF column is accessed in descening order
        for (auto it = table.rbegin(); it != table.rend(); ++it)
        {
            entry = (*it);
            if (simt != entry.SIMT)
                continue;

            if (MatchHWLocalID(entry.HWLID, hwlid) && witems <= entry.Workitems)
            {
                return entry.GRF;
            }
        }

        return 0;
    }

    void MaxGRFTable::LoadTable(const CPlatform& platform)
    {
        {
#define GRF_SIMT_LID_WITEMS(a, b, c, d) table.push_back({a, b, c, d});
            XE3_GRF
#undef GRF_SIMT_LID_WITEMS
        }
    }

    uint16_t MaxGRFTable::GetMaxGRF(SIMDMode simt, HWLocalId hwlid,
        uint16_t witems, const CPlatform& platform)
    {
        if (table.empty())
            LoadTable(platform);

        return LookupMaxGRF(simt, hwlid, witems, table);
    }

} //namespace IGC
