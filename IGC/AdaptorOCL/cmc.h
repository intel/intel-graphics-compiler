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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/DynamicLibrary.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "VectorCompiler/include/vc/GenXCodeGen/GenXWrapper.h"
#include "common/LLVMWarningsPop.hpp"

// optional resource access kind
enum class cmc_access_kind : int32_t {
    undef,
    read_only,
    write_only,
    read_write
};

enum class cmc_arg_kind : int32_t {
    General,
    LocalSize,  // IMPLICIT_LOCAL_SIZE
    GroupCount, // IMPLICIT_NUM_GROUPS
    Buffer,     // 1D buffer
    SVM,        // stateless global pointer
    Sampler,
    Image1d,
    Image2d,
    Image3d,
    PrintBuffer,
    PrivateBase
};

struct cmc_ocl_print_string {
    static constexpr unsigned max_width = 256;
    char s[max_width];
};

struct cmc_arg_info {
    // The argument kind.
    cmc_arg_kind kind = cmc_arg_kind::General;

    // The argument index in this kernel.
    int32_t index = 0;

    // the byte offset of this argument in payload
    int32_t offset = 0;

    // The byte size of this argument in payload
    int32_t sizeInBytes = 0;

    // The BTI for this resource, if applicable.
    int32_t BTI = 0;

    // the optional resource access kind, if applicable.
    cmc_access_kind access = cmc_access_kind::undef;
};

// compilation interface bewteen cmc and igc
struct cmc_kernel_info_v2 {
    /// The kernel name.
    const char *name;

    /// The number of kernel arguments
    unsigned num_args;

    /// The kernel argument info.
    cmc_arg_info *arg_descs;

    // ThreadPayload
    bool HasLocalIDx = false;
    bool HasLocalIDy = false;
    bool HasLocalIDz = false;
    bool HasGroupID = false;

    // ExecutionEnivronment
    uint32_t CompiledSIMDSize = 8;
    uint32_t SLMSize = 0;
    uint32_t NumGRFRequired = 128;
    uint32_t GRFByteSize = 32;
    uint32_t HasBarriers = 0;
    bool HasReadWriteImages = false;
    uint32_t ThreadPrivateMemSize = 0;
    uint32_t StatelessPrivateMemSize = 0;

    /// number of format strings in the kernel
    unsigned num_print_strings = 0;
    /// The kernel format string storage
    cmc_ocl_print_string *print_string_descs = nullptr;

    struct TableInfo {
        void *Buf = nullptr;
        uint32_t Size = 0;
        uint32_t NumEntries = 0;
    };

    TableInfo RelocationTable;
    TableInfo SymbolTable;
    vc::ocl::ZEBinaryInfo ZEBinInfo;
};

struct cmc_compile_info_v2 {
    /// The vISA binary size in bytes.
    uint64_t binary_size;

    /// The vISA binary data.
    void* binary;

    uint32_t pointer_size_in_bytes;

    /// The vISA major version.
    uint32_t visa_major_version;

    /// The vISA minor version.
    uint32_t visa_minor_version;

    /// The number of kernel.
    unsigned num_kernels;

    /// Ugly stub for compatibility with cmc
    void *compatibility_stub;

    /// The context for this compilation. This opaque data holds all memory
    /// allocations that will be freed in the end.
    void* context;

    /// The kernel infomation.
    cmc_kernel_info_v2 *kernel_info_v2;
};

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
    IGC::SProgramOutput m_prog;
    IGC::COCLBTILayout m_btiLayout;
    uint32_t m_GRFSizeInBytes;

    // General argument
    void createConstArgumentAnnotation(unsigned argNo,
                                       unsigned sizeInBytes,
                                       unsigned payloadPosition);

    // 1D/2D/3D Surface
    void  createImageAnnotation(unsigned argNo,
                                unsigned BTI,
                                unsigned dim,
                                bool isWriteable);

    // add a pointer patch token.
    void createPointerGlobalAnnotation(const cmc_arg_info &argInfo);

    void createPrivateBaseAnnotation(unsigned argNo, unsigned byteSize,
                                     unsigned payloadPosition, int BTI,
                                     unsigned statelessPrivateMemSize);

    // add a stateful buffer patch token.
    void createBufferStatefulAnnotation(unsigned argNo,
                                        cmc_access_kind accessKind);

    // Local or global size
    void createSizeAnnotation(unsigned payloadPosition, iOpenCL::DATA_PARAMETER_TOKEN type);

    // Global work offset/local work size
    void createImplicitArgumentsAnnotation(unsigned payloadPosition);

    // Sampler
    void createSamplerAnnotation(unsigned argNo);

    void RecomputeBTLayout(int numUAVs, int numResources);
};

extern int vISACompile_v2(cmc_compile_info_v2 *output,
                          iOpenCL::CGen8CMProgram &CMProgram,
                          std::vector<const char*> &opts);

extern const char* getPlatformStr(PLATFORM platform);

} // namespace cmc

namespace vc {
void createBinary(iOpenCL::CGen8CMProgram &CMProgram,
                  const std::vector<vc::ocl::CompileInfo> &CompileInfos);
} // namespace vc
