/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/gt_system_info.h"
#include "ocl_igc_interface/impl/gt_system_info_impl.h"

#include "cif/macros/enable.h"

namespace IGC{
// Helpers for clarity
// Basically, these forward GetX/SetX from interface (of given version)
// to GT_SYSTEM_INFO inside pImpl
#define DEFINE_GET_SET(INTERFACE, VERSION, NAME, TYPE)\
    TYPE CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Get##NAME() const {\
        return CIF_GET_PIMPL()->gsi.NAME;\
    }\
    void CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Set##NAME(TYPE v) {\
        CIF_GET_PIMPL()->gsi.NAME = v;\
    }

DEFINE_GET_SET(GTSystemInfo, 1, EUCount, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, ThreadCount, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, SliceCount, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, SubSliceCount, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, L3CacheSizeInKb, uint64_t);
DEFINE_GET_SET(GTSystemInfo, 1, LLCCacheSizeInKb, uint64_t);
DEFINE_GET_SET(GTSystemInfo, 1, EdramSizeInKb, uint64_t);
DEFINE_GET_SET(GTSystemInfo, 1, L3BankCount, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, MaxFillRate, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, EuCountPerPoolMax, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, EuCountPerPoolMin, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, TotalVsThreads, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, TotalHsThreads, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, TotalDsThreads, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, TotalGsThreads, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, TotalPsThreadsWindowerRange, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, CsrSizeInMb, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, MaxEuPerSubSlice, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, MaxSlicesSupported, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, MaxSubSlicesSupported, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 1, IsL3HashModeEnabled, bool);
DEFINE_GET_SET(GTSystemInfo, 1, IsDynamicallyPopulated, bool);

DEFINE_GET_SET(GTSystemInfo, 3, MaxDualSubSlicesSupported, uint32_t);
DEFINE_GET_SET(GTSystemInfo, 3, DualSubSliceCount, uint32_t);

}

#include "cif/macros/disable.h"
