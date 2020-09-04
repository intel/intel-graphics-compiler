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

#include <string>
#include <type_traits>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/DynamicLibrary.h>

#include "Compiler/CodeGenPublic.h"

#include "vc/GenXCodeGen/GenXWrapper.h"

namespace iOpenCL {
  class CGen8CMProgram;
}

namespace cmc {

// Interface to compile and package cm kernels into OpenCL binars.
class CMKernel {
public:
    explicit CMKernel(const PLATFORM& platform);
    ~CMKernel();

    PLATFORM m_platform;
    IGC::SOpenCLKernelInfo m_kernelInfo;
    IGC::COCLBTILayout m_btiLayout;
    uint32_t m_GRFSizeInBytes;

    // getter for convenience
    const IGC::SProgramOutput &getProgramOutput() const {
        return m_kernelInfo.m_kernelProgram.simd1;
    }

    // getter for convenience
    IGC::SProgramOutput &getProgramOutput() {
        return const_cast<IGC::SProgramOutput&>(
            static_cast<const CMKernel*>(this)->getProgramOutput());
    }

    // General argument
    void createConstArgumentAnnotation(unsigned argNo,
                                       unsigned sizeInBytes,
                                       unsigned payloadPosition);

    // 1D/2D/3D Surface
    void createImageAnnotation(unsigned argNo, unsigned BTI, unsigned dim,
                               bool isWriteable);

    // add a pointer patch token.
    void createPointerGlobalAnnotation(unsigned index, unsigned offset,
                                       unsigned sizeInBytes, unsigned BTI,
                                       vc::ocl::ArgAccessKind access);

    void createPrivateBaseAnnotation(unsigned argNo, unsigned byteSize,
                                     unsigned payloadPosition, int BTI,
                                     unsigned statelessPrivateMemSize);

    // add a stateful buffer patch token.
    void createBufferStatefulAnnotation(unsigned argNo,
                                        vc::ocl::ArgAccessKind accessKind);

    // Local or global size
    void createSizeAnnotation(unsigned payloadPosition, iOpenCL::DATA_PARAMETER_TOKEN type);

    // Global work offset/local work size
    void createImplicitArgumentsAnnotation(unsigned payloadPosition);

    // Sampler
    void createSamplerAnnotation(unsigned argNo);

    void RecomputeBTLayout(int numUAVs, int numResources);
};

extern const char* getPlatformStr(PLATFORM platform);

} // namespace cmc

namespace vc {
void createBinary(iOpenCL::CGen8CMProgram &CMProgram,
                  const std::vector<vc::ocl::CompileInfo> &CompileInfos);
} // namespace vc
