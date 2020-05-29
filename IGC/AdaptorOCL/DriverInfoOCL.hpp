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
#include "Compiler/CISACodeGen/DriverInfo.hpp"

namespace TC
{
    /// caps common to all OCL runtimes
    class CDriverInfoOCLCommon : public IGC::CDriverInfo
    {
    public:
        bool AllowUnsafeHalf() const override { return false; }

        bool AllowSendFusion() const override { return false; }

        bool SupportsIEEEMinMax() const override { return true; }

        bool SupportsPreciseMath() const override { return true; }

        bool NeedCheckContractionAllowed() const override { return true; }

        bool NeedI64BitDivRem() const override { return true; }

        bool HasMemoryIntrinsics() const override { return true; }

        bool HasNonNativeLoadStore() const override { return true; }

        bool NeedLoweringInlinedConstants() const override { return true; }
        bool benefitFromTypeDemotion() const override { return true; }
        bool benefitFromPreRARematFlag() const override { return true; }

        bool NeedExtraPassesAfterAlwaysInlinerPass() const override { return true; }
        bool enableVISAPreRAScheduler() const override { return true; }

        bool NeedWAToTransformA32MessagesToA64() const override{ return true; }
        bool WALoadStorePatternMatch() const override { return true; }
        bool WADisableCustomPass() const override { return true; }
        bool WAEnableMemOpt2ForOCL() const override { return true; }

        unsigned int GetLoopUnrollThreshold() const override { return  1280; }
        bool Enable64BitEmu() const override { return true; }

        bool NeedIEEESPDiv() const override { return true; }

        // Not needed as OCL doesn't go through emitStore3DInner
        bool splitUnalignedVectors() const override { return false; }

        bool supportsStatelessSpacePrivateMemory() const override { return true; }

        bool NeedFP64(PRODUCT_FAMILY productFamily) const override {
#if defined(__linux__)
          if (IGC_IS_FLAG_ENABLED(EnableDPEmulation)) {
              return true;
          }
#endif
            return false;
        }

        bool EnableIntegerMad() const override { return true; }
    };

    // In case some cpas are specific to NEO
    class CDriverInfoOCLNEO : public CDriverInfoOCLCommon
    {
    public:
        bool SupportsStatelessToStatefullBufferTransformation() const override { return true; }
        unsigned getVISAPreRASchedulerCtrl() const override { return 6; }
        bool SupportStatefulToken() const override { return true; }
        bool SupportInlineAssembly() const override { return true; }
    };

}//namespace TC
