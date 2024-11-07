/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

#include "OCLAPI/oclapi.h"

// Interface : GT_SYS_INFO
//             GT System Info
// Interface for defining target device

namespace IGC {

CIF_DECLARE_INTERFACE(GTSystemInfo, "GT_SYS_INFO")

CIF_DEFINE_INTERFACE_VER(GTSystemInfo, 1){
  CIF_INHERIT_CONSTRUCTOR();

 OCL_API_CALL virtual uint32_t GetEUCount() const;
 OCL_API_CALL virtual void SetEUCount(uint32_t v);
 OCL_API_CALL virtual uint32_t GetThreadCount() const;
 OCL_API_CALL virtual void SetThreadCount(uint32_t v);
 OCL_API_CALL virtual uint32_t GetSliceCount() const;
 OCL_API_CALL virtual void SetSliceCount(uint32_t v);
 OCL_API_CALL virtual uint32_t GetSubSliceCount() const;
 OCL_API_CALL virtual void SetSubSliceCount(uint32_t v);
 OCL_API_CALL virtual uint64_t GetL3CacheSizeInKb() const;
 OCL_API_CALL virtual void SetL3CacheSizeInKb(uint64_t v);
 OCL_API_CALL virtual uint64_t GetLLCCacheSizeInKb() const;
 OCL_API_CALL virtual void SetLLCCacheSizeInKb(uint64_t v);
 OCL_API_CALL virtual uint64_t GetEdramSizeInKb() const;
 OCL_API_CALL virtual void SetEdramSizeInKb(uint64_t v);
 OCL_API_CALL virtual uint32_t GetL3BankCount() const;
 OCL_API_CALL virtual void SetL3BankCount(uint32_t v);
 OCL_API_CALL virtual uint32_t GetMaxFillRate() const;
 OCL_API_CALL virtual void SetMaxFillRate(uint32_t v);
 OCL_API_CALL virtual uint32_t GetEuCountPerPoolMax() const;
 OCL_API_CALL virtual void SetEuCountPerPoolMax(uint32_t v);
 OCL_API_CALL virtual uint32_t GetEuCountPerPoolMin() const;
 OCL_API_CALL virtual void SetEuCountPerPoolMin(uint32_t v);

 OCL_API_CALL virtual uint32_t GetTotalVsThreads() const;
 OCL_API_CALL virtual void SetTotalVsThreads(uint32_t v);
 OCL_API_CALL virtual uint32_t GetTotalHsThreads() const;
 OCL_API_CALL virtual void SetTotalHsThreads(uint32_t v);
 OCL_API_CALL virtual uint32_t GetTotalDsThreads() const;
 OCL_API_CALL virtual void SetTotalDsThreads(uint32_t v);
 OCL_API_CALL virtual uint32_t GetTotalGsThreads() const;
 OCL_API_CALL virtual void SetTotalGsThreads(uint32_t v);
 OCL_API_CALL virtual uint32_t GetTotalPsThreadsWindowerRange() const;
 OCL_API_CALL virtual void SetTotalPsThreadsWindowerRange(uint32_t v);

 OCL_API_CALL virtual uint32_t GetCsrSizeInMb() const;
 OCL_API_CALL virtual void SetCsrSizeInMb(uint32_t v);

 OCL_API_CALL virtual uint32_t GetMaxEuPerSubSlice() const;
 OCL_API_CALL virtual void SetMaxEuPerSubSlice(uint32_t v);
 OCL_API_CALL virtual uint32_t GetMaxSlicesSupported() const;
 OCL_API_CALL virtual void SetMaxSlicesSupported(uint32_t v);
 OCL_API_CALL virtual uint32_t GetMaxSubSlicesSupported() const;
 OCL_API_CALL virtual void SetMaxSubSlicesSupported(uint32_t v);
 OCL_API_CALL virtual bool GetIsL3HashModeEnabled() const;
 OCL_API_CALL virtual void SetIsL3HashModeEnabled(bool v);

 OCL_API_CALL virtual bool GetIsDynamicallyPopulated() const;
 OCL_API_CALL virtual void SetIsDynamicallyPopulated(bool v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 3, 1) {
    CIF_INHERIT_CONSTRUCTOR();

   OCL_API_CALL virtual uint32_t GetMaxDualSubSlicesSupported() const;
   OCL_API_CALL virtual void SetMaxDualSubSlicesSupported(uint32_t v);
   OCL_API_CALL virtual uint32_t GetDualSubSliceCount() const;
   OCL_API_CALL virtual void SetDualSubSliceCount(uint32_t v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 4, 3) {
    CIF_INHERIT_CONSTRUCTOR();

    OCL_API_CALL virtual uint32_t GetSLMSizeInKb() const;
    OCL_API_CALL virtual void SetSLMSizeInKb(uint32_t v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 5, 4) {
    CIF_INHERIT_CONSTRUCTOR();
};

CIF_GENERATE_VERSIONS_LIST(GTSystemInfo);
CIF_MARK_LATEST_VERSION(GTSystemInfoLatest, GTSystemInfo);
using GTSystemInfoTagOCL = GTSystemInfo<5>;    // Note : can tag with different version for
                                               //        transition periods

}

#include "cif/macros/disable.h"
