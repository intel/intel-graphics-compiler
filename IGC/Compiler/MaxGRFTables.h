/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "../common/Types.hpp"
#include "CISACodeGen/Platform.hpp"

#include <stdint.h>
#include <vector>

using namespace IGC;
namespace IGC
{
    enum class HWLocalId : unsigned char
    {
        VALUE0 = 0,
        VALUE1,
        EITHER
    };

    // Define a struct to represent each row
    struct MaxGRFEntry
    {
        uint16_t GRF;
        SIMDMode SIMT;
        HWLocalId HWLID; // HW local-id generation
        uint16_t Workitems;
    };

    // Singleton class to create only one instance
    class MaxGRFTable
    {
    public:
        // Static method to access the singleton instance
        static MaxGRFTable & GetInstance(const CPlatform& platform)
        {
            static MaxGRFTable grfTable(platform);
            return grfTable;
        }

        // Delete copy constructor and assignment operator to prevent copying
        MaxGRFTable(MaxGRFTable&) = delete;
        MaxGRFTable & operator=(const MaxGRFTable&) = delete;
        MaxGRFTable(const CPlatform& platform) : m_platform(platform)
        {
            LoadTable(platform);
        }
        uint16_t GetMaxGRF(SIMDMode simt, HWLocalId hwlid, uint witems);

    private:
        const CPlatform& m_platform;
        llvm::ArrayRef<MaxGRFEntry> table;
        void LoadTable(const CPlatform& platform);
        bool MatchHWLocalID(HWLocalId fromTable, HWLocalId target);
    };
}
