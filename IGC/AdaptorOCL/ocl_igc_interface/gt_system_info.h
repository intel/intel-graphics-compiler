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

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

// Interface : GT_SYS_INFO
//             GT System Info
// Interface for defining target device

namespace IGC {

CIF_DECLARE_INTERFACE(GTSystemInfo, "GT_SYS_INFO")

CIF_DEFINE_INTERFACE_VER(GTSystemInfo, 1){
  CIF_INHERIT_CONSTRUCTOR();

  virtual uint32_t GetEUCount() const;
  virtual void SetEUCount(uint32_t v);
  virtual uint32_t GetThreadCount() const;
  virtual void SetThreadCount(uint32_t v);
  virtual uint32_t GetSliceCount() const;
  virtual void SetSliceCount(uint32_t v);
  virtual uint32_t GetSubSliceCount() const;
  virtual void SetSubSliceCount(uint32_t v);
  virtual uint64_t GetL3CacheSizeInKb() const;
  virtual void SetL3CacheSizeInKb(uint64_t v);
  virtual uint64_t GetLLCCacheSizeInKb() const;
  virtual void SetLLCCacheSizeInKb(uint64_t v);
  virtual uint64_t GetEdramSizeInKb() const;
  virtual void SetEdramSizeInKb(uint64_t v);
  virtual uint32_t GetL3BankCount() const;
  virtual void SetL3BankCount(uint32_t v);
  virtual uint32_t GetMaxFillRate() const;
  virtual void SetMaxFillRate(uint32_t v);
  virtual uint32_t GetEuCountPerPoolMax() const;
  virtual void SetEuCountPerPoolMax(uint32_t v);
  virtual uint32_t GetEuCountPerPoolMin() const;
  virtual void SetEuCountPerPoolMin(uint32_t v);

  virtual uint32_t GetTotalVsThreads() const;
  virtual void SetTotalVsThreads(uint32_t v);
  virtual uint32_t GetTotalHsThreads() const;
  virtual void SetTotalHsThreads(uint32_t v);
  virtual uint32_t GetTotalDsThreads() const;
  virtual void SetTotalDsThreads(uint32_t v);
  virtual uint32_t GetTotalGsThreads() const;
  virtual void SetTotalGsThreads(uint32_t v);
  virtual uint32_t GetTotalPsThreadsWindowerRange() const;
  virtual void SetTotalPsThreadsWindowerRange(uint32_t v);

  virtual uint32_t GetCsrSizeInMb() const;
  virtual void SetCsrSizeInMb(uint32_t v);

  virtual uint32_t GetMaxEuPerSubSlice() const;
  virtual void SetMaxEuPerSubSlice(uint32_t v);
  virtual uint32_t GetMaxSlicesSupported() const;
  virtual void SetMaxSlicesSupported(uint32_t v);
  virtual uint32_t GetMaxSubSlicesSupported() const;
  virtual void SetMaxSubSlicesSupported(uint32_t v);
  virtual bool GetIsL3HashModeEnabled() const;
  virtual void SetIsL3HashModeEnabled(bool v);

  virtual bool GetIsDynamicallyPopulated() const;
  virtual void SetIsDynamicallyPopulated(bool v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(GTSystemInfo, 3, 1) {
    CIF_INHERIT_CONSTRUCTOR();

    virtual uint32_t GetMaxDualSubSlicesSupported() const;
    virtual void SetMaxDualSubSlicesSupported(uint32_t v);
    virtual uint32_t GetDualSubSliceCount() const;
    virtual void SetDualSubSliceCount(uint32_t v);
};

CIF_GENERATE_VERSIONS_LIST(GTSystemInfo);
CIF_MARK_LATEST_VERSION(GTSystemInfoLatest, GTSystemInfo);
using GTSystemInfoTagOCL = GTSystemInfo<3>;    // Note : can tag with different version for
                                               //        transition periods

}

#include "cif/macros/disable.h"
