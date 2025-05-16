/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "../common/Types.hpp"
#include <stdint.h>
#include <vector>

using namespace std;
using namespace IGC;

namespace IGC
{
    class CPlatform;

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
        HWLocalId HWLID; // HW loal-id generation
        uint16_t Workitems;
    };

#define XE3_GRF \
    GRF_SIMT_LID_WITEMS(128, SIMDMode::SIMD16, HWLocalId::EITHER, 1024) \
    GRF_SIMT_LID_WITEMS(128, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024) \
    GRF_SIMT_LID_WITEMS(128, SIMDMode::SIMD32, HWLocalId::VALUE0, 2048) \
    GRF_SIMT_LID_WITEMS(160, SIMDMode::SIMD16, HWLocalId::EITHER,  768) \
    GRF_SIMT_LID_WITEMS(160, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024) \
    GRF_SIMT_LID_WITEMS(160, SIMDMode::SIMD32, HWLocalId::VALUE0, 1536) \
    GRF_SIMT_LID_WITEMS(192, SIMDMode::SIMD16, HWLocalId::EITHER,  640) \
    GRF_SIMT_LID_WITEMS(192, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024) \
    GRF_SIMT_LID_WITEMS(192, SIMDMode::SIMD32, HWLocalId::VALUE0, 1280) \
    GRF_SIMT_LID_WITEMS(256, SIMDMode::SIMD16, HWLocalId::EITHER,  512) \
    GRF_SIMT_LID_WITEMS(256, SIMDMode::SIMD32, HWLocalId::EITHER, 1024)



    // Singleton class to create only one instance
    class MaxGRFTable
    {
    private:
        MaxGRFTable() { }
        MaxGRFTable(const MaxGRFTable&) = delete;
        MaxGRFTable& operator=(const MaxGRFTable&) = delete;
        vector<MaxGRFEntry> table;
        void LoadTable(const CPlatform& platform);
        bool MatchHWLocalID(HWLocalId fromTable, HWLocalId target);
        uint16_t LookupMaxGRF(
            SIMDMode simt, HWLocalId hwlid, unsigned int witems,
            const vector<MaxGRFEntry>& table);

    public:
        // Static method to access the singleton instance
        static MaxGRFTable& GetInstance()
        {
            static MaxGRFTable grfTable;
            return grfTable;
        }

        uint16_t GetMaxGRF(SIMDMode simt, HWLocalId hwlid,
            uint16_t witems, const CPlatform& platform);
    };
}
