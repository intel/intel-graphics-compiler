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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include "common/LLVMWarningsPop.hpp"

#include "cmc.h"
#include "RT_Jitter_Interface.h"
#include "inc/common/igfxfmid.h"
#include "AdaptorOCL/OCL/sp/spp_g8.h"
#include "common/secure_mem.h"
#include "common/secure_string.h"

#include <llvm/Support/raw_ostream.h>

#include <string>
#include <iterator>
#include "Probe/Assertion.h"

#if defined(_WIN64)
#define CMC_LIBRARY_NAME "igcmc64.dll"
#elif defined(_WIN32)
#define CMC_LIBRARY_NAME "igcmc32.dll"
#else
#define CMC_LIBRARY_NAME "libigcmc.so"
#endif

#ifdef _WIN32
#include <Windows.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Return the current binary path. Windows only.
static std::string getCurrentLibraryPath()
{
    char CurPath[1024] = {};
    HINSTANCE Inst = reinterpret_cast<HINSTANCE>(&__ImageBase);
    ::GetModuleFileName(Inst, CurPath, sizeof(CurPath));
    return CurPath;
}
#else
static std::string getCurrentLibraryPath()
{
    return {};
}
#endif

using namespace cmc;

CMKernel::CMKernel(const PLATFORM& platform)
    : m_platform(platform)
    , m_btiLayout(new USC::SShaderStageBTLayout)
{
    std::memset(m_btiLayout.getModifiableLayout(), 0, sizeof(USC::SShaderStageBTLayout));
}

CMKernel::~CMKernel()
{
    delete m_btiLayout.getModifiableLayout();
}

void CMKernel::createConstArgumentAnnotation(unsigned argNo, unsigned sizeInBytes, unsigned payloadPosition)
{
    iOpenCL::ConstantArgumentAnnotation* constInput = new iOpenCL::ConstantArgumentAnnotation;

    constInput->AnnotationSize = sizeof(constInput);
    constInput->Offset = 0;
    constInput->PayloadPosition = payloadPosition;
    constInput->PayloadSizeInBytes = sizeInBytes;
    constInput->ArgumentNumber = argNo;
    constInput->LocationIndex = 0;
    constInput->LocationCount = 0;
    constInput->IsEmulationArgument = false;
    m_kernelInfo.m_constantArgumentAnnotation.push_back(constInput);
}

// TODO: this is incomplete.
void CMKernel::createSamplerAnnotation(unsigned argNo)
{
    iOpenCL::SAMPLER_OBJECT_TYPE samplerType;
    samplerType = iOpenCL::SAMPLER_OBJECT_TEXTURE;

    iOpenCL::SamplerArgumentAnnotation* samplerArg = new iOpenCL::SamplerArgumentAnnotation;
    samplerArg->AnnotationSize = sizeof(samplerArg);
    samplerArg->SamplerType = samplerType;
    samplerArg->ArgumentNumber = argNo;
    samplerArg->SamplerTableIndex = 0;
    samplerArg->LocationIndex = 0;
    samplerArg->LocationCount = 0;
    samplerArg->IsBindlessAccess = false;
    samplerArg->IsEmulationArgument = false;
    samplerArg->PayloadPosition = 0;

    m_kernelInfo.m_samplerArgument.push_back(samplerArg);
}

void CMKernel::createImageAnnotation(unsigned argNo, unsigned BTI, unsigned dim, bool isWriteable)
{
    iOpenCL::ImageArgumentAnnotation* imageInput = new iOpenCL::ImageArgumentAnnotation;

    imageInput->AnnotationSize = sizeof(imageInput);
    imageInput->ArgumentNumber = argNo;
    imageInput->IsFixedBindingTableIndex = true;
    imageInput->BindingTableIndex = BTI;
    if (dim == 1)
        imageInput->ImageType = iOpenCL::IMAGE_MEMORY_OBJECT_1D;
    else if (dim == 2)
        imageInput->ImageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK;
    else if (dim == 3)
        imageInput->ImageType = iOpenCL::IMAGE_MEMORY_OBJECT_3D;
    else
        IGC_ASSERT_MESSAGE(0, "unsupported image dimension");
    imageInput->LocationIndex = 0;
    imageInput->LocationCount = 0;
    imageInput->IsEmulationArgument = false;
    imageInput->AccessedByFloatCoords = false;
    imageInput->AccessedByIntCoords = false;
    imageInput->IsBindlessAccess = false;
    imageInput->PayloadPosition = 0;
    imageInput->Writeable = isWriteable;
    m_kernelInfo.m_imageInputAnnotations.push_back(imageInput);
}

void CMKernel::createImplicitArgumentsAnnotation(unsigned payloadPosition)
{
    for (int i = 0; i < 6; ++i) {
        iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation;
        DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

        constInput->AnnotationSize = sizeof(constInput);
        constInput->ConstantType = (i < 3 ? iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET
                                          : iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE);
        constInput->Offset = (i % 3) * sizeInBytes;
        constInput->PayloadPosition = payloadPosition;
        constInput->PayloadSizeInBytes = sizeInBytes;
        constInput->ArgumentNumber = 0;
        constInput->LocationIndex = 0;
        constInput->LocationCount = 0;
        m_kernelInfo.m_constantInputAnnotation.push_back(constInput);

        payloadPosition += sizeInBytes;
    }
}

void CMKernel::createPointerGlobalAnnotation(unsigned argNo, unsigned byteSize, unsigned payloadPosition, int BTI)
{
    iOpenCL::PointerArgumentAnnotation* ptrAnnotation = new iOpenCL::PointerArgumentAnnotation;
    ptrAnnotation->IsStateless = true;
    ptrAnnotation->IsBindlessAccess = false;
    ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
    ptrAnnotation->AnnotationSize = sizeof(ptrAnnotation);
    ptrAnnotation->ArgumentNumber = argNo;
    ptrAnnotation->BindingTableIndex = BTI;
    ptrAnnotation->PayloadPosition = payloadPosition;
    ptrAnnotation->PayloadSizeInBytes = byteSize;
    ptrAnnotation->LocationIndex = 0;
    ptrAnnotation->LocationCount = 0;
    ptrAnnotation->IsEmulationArgument = false;
    m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);
}

void CMKernel::createPrivateBaseAnnotation(
    unsigned argNo, unsigned byteSize, unsigned payloadPosition, int BTI,
    unsigned statelessPrivateMemSize) {
  iOpenCL::PrivateInputAnnotation *ptrAnnotation =
      new iOpenCL::PrivateInputAnnotation();

  ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_PRIVATE;
  ptrAnnotation->AnnotationSize = sizeof(ptrAnnotation);
  ptrAnnotation->ArgumentNumber = argNo;
  // PerThreadPrivateMemorySize is defined as "Total private memory requirements for each OpenCL work-item."
  ptrAnnotation->PerThreadPrivateMemorySize = statelessPrivateMemSize;
  ptrAnnotation->BindingTableIndex = BTI;
  ptrAnnotation->IsStateless = true;
  ptrAnnotation->PayloadPosition = payloadPosition;
  ptrAnnotation->PayloadSizeInBytes = byteSize;
  m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
}

void CMKernel::createBufferStatefulAnnotation(unsigned argNo)
{
    iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation;

    constInput->AnnotationSize = sizeof(constInput);
    constInput->ConstantType = iOpenCL::DATA_PARAMETER_BUFFER_STATEFUL;
    constInput->Offset = 0;
    constInput->PayloadPosition = 0;
    constInput->PayloadSizeInBytes = 0;
    constInput->ArgumentNumber = argNo;
    constInput->LocationIndex = 0;
    constInput->LocationCount = 0;
    m_kernelInfo.m_constantInputAnnotation.push_back(constInput);
}

void CMKernel::createSizeAnnotation(unsigned payloadPosition, int32_t Type)
{
    for (int i = 0; i < 3; ++i) {
        iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation;
        DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

        constInput->AnnotationSize = sizeof(constInput);
        constInput->ConstantType = Type;
        constInput->Offset = i * sizeInBytes;
        constInput->PayloadPosition = payloadPosition;
        constInput->PayloadSizeInBytes = sizeInBytes;
        constInput->ArgumentNumber = 0;
        constInput->LocationIndex = 0;
        constInput->LocationCount = 0;
        m_kernelInfo.m_constantInputAnnotation.push_back(constInput);

        payloadPosition += sizeInBytes;
    }
}

// TODO: refactor this function with the OCL part.
void CMKernel::RecomputeBTLayout(int numUAVs, int numResources)
{
    USC::SShaderStageBTLayout* layout = m_btiLayout.getModifiableLayout();

    // The BT layout contains the minimum and the maximum number BTI for each kind
    // of resource. E.g. UAVs may be mapped to BTIs 0..3, SRVs to 4..5, and the scratch
    // surface to 6.
    // Note that the names are somewhat misleading. They are used for the sake of consistency
    // with the ICBE sources.

    // Some fields are always 0 for OCL.
    layout->resourceNullBoundOffset = 0;
    layout->immediateConstantBufferOffset = 0;
    layout->interfaceConstantBufferOffset = 0;
    layout->constantBufferNullBoundOffset = 0;
    layout->JournalIdx = 0;
    layout->JournalCounterIdx = 0;

    // And TGSM (aka SLM) is always 254.
    layout->TGSMIdx = 254;

    int index = 0;

    // Now, allocate BTIs for all the SRVs.
    layout->minResourceIdx = index;
    if (numResources) {
        index += numResources - 1;
        layout->maxResourceIdx = index++;
    } else {
        layout->maxResourceIdx = index;
    }

    // Now, ConstantBuffers - used as a placeholder for the inline constants, if present.
    layout->minConstantBufferIdx = index;
    layout->maxConstantBufferIdx = index;

    // Now, the UAVs
    layout->minUAVIdx = index;
    if (numUAVs) {
        index += numUAVs - 1;
        layout->maxUAVIdx = index++;
    } else {
        layout->maxUAVIdx = index;
    }

    // And finally, the scratch surface
    layout->surfaceScratchIdx = index++;

    // Overall number of used BT entries, not including TGSM.
    layout->maxBTsize = index;
}

// Convert an opaque pointer to a function pointer.
template <typename func_ptr_type>
inline func_ptr_type getFunctionType(void* ptr)
{
    intptr_t val = reinterpret_cast<intptr_t>(ptr);
    return reinterpret_cast<func_ptr_type>(val);
}

CMCLibraryLoader::CMCLibraryLoader()
{
    Dylib = DL::getPermanentLibrary(CMC_LIBRARY_NAME, &ErrMsg);
    if (!Dylib.isValid()) {
        // Cannot locate CMC dll in PATH; try to locate it along with the current image.
        std::string DLLPath = getCurrentLibraryPath();
        if (!DLLPath.empty()) {
            llvm::SmallVector<char, 1024> ParentPath(DLLPath.begin(), DLLPath.end());
            llvm::sys::path::remove_filename(ParentPath);
            DLLPath.assign(ParentPath.data(), ParentPath.data() + ParentPath.size());
            DLLPath.append("\\").append(CMC_LIBRARY_NAME);
            Dylib = DL::getPermanentLibrary(DLLPath.c_str(), &ErrMsg);
        }
    }
    if (Dylib.isValid()) {
        compileFn_v2 = getFunctionType<compileFnTy_v2>(Dylib.getAddressOfSymbol("cmc_load_and_compile_v2"));
        freeFn_v2 = getFunctionType<freeFnTy_v2>(Dylib.getAddressOfSymbol("cmc_free_compile_info_v2"));
    }
}

bool CMCLibraryLoader::isValid()
{
    if (!Dylib.isValid())
        return false;
    if (!compileFn_v2) {
        ErrMsg = "cannot load symbol cmc_load_and_compile_v2";
        return false;
    }
    if (!freeFn_v2) {
        ErrMsg = "cannot load symbol cmc_free_compile_info_v2";
        return false;
    }
    return true;
}

const char* cmc::getPlatformStr(PLATFORM platform)
{
    switch (platform.eDisplayCoreFamily) {
    case IGFX_GEN9_CORE:
        return "SKL";
    case IGFX_GEN10_CORE:
        return "CNL";
    case IGFX_GEN11_CORE:
        if (platform.eProductFamily == IGFX_ICELAKE_LP ||
            platform.eProductFamily == IGFX_LAKEFIELD)
            return "ICLLP";
        return "ICL";
    default:
        IGC_ASSERT_MESSAGE(0, "unsupported platform");
        break;
    }
    return "SKL";
}

static void generatePatchTokens_v2(const cmc_kernel_info_v2 *info, CMKernel& kernel)
{
    // This is the starting constant thread payload
    // r0-r3 are reserved for SIMD8 dispatch.
    unsigned constantPayloadStart = info->GRFByteSize * 4;
    // r0-r1 are reserved for SIMD1 dispatch
    if (info->CompiledSIMDSize == 1)
        constantPayloadStart = info->GRFByteSize * 2;

    unsigned payloadPos = constantPayloadStart;

    // Allocate GLOBAL_WORK_OFFSET and LOCAL_WORK_SIZE, SIMD8 dispatch only.
    if (info->CompiledSIMDSize == 8) {
        kernel.createImplicitArgumentsAnnotation(0);
        payloadPos += info->GRFByteSize;
    }

    // Now all arguments from cmc. We keep track of the maximal offset.
    int32_t maxArgEnd = payloadPos;

    // Setup argument to BTI mapping.
    kernel.m_kernelInfo.m_argIndexMap.clear();

    for (unsigned i = 0; i < info->num_print_strings; i++) {
        IGC_ASSERT(info->print_string_descs);
        cmc_ocl_print_string& SI = info->print_string_descs[i];

        iOpenCL::PrintfStringAnnotation* stringAnnotation = new iOpenCL::PrintfStringAnnotation;
        stringAnnotation->Index = i;
        stringAnnotation->StringSize = cmc_ocl_print_string::max_width;
        stringAnnotation->StringData = new char[cmc_ocl_print_string::max_width];
        std::copy(SI.s, SI.s + cmc_ocl_print_string::max_width, stringAnnotation->StringData);
        kernel.m_kernelInfo.m_printfStringAnnotations.push_back(stringAnnotation);
    }

    for (unsigned i = 0; i < info->num_args; ++i) {
        IGC_ASSERT(info->arg_descs);
        cmc_arg_info& AI = info->arg_descs[i];
        if (AI.offset > 0)
            maxArgEnd = std::max(AI.offset + AI.sizeInBytes, maxArgEnd);
        bool isWriteable = AI.access != cmc_access_kind::read_only;

        switch (AI.kind) {
        default:
            break;
        case cmc_arg_kind::General:
            kernel.createConstArgumentAnnotation(AI.index, AI.sizeInBytes, AI.offset - constantPayloadStart);
            break;
        case cmc_arg_kind::LocalSize:
            kernel.createSizeAnnotation(AI.offset - constantPayloadStart, iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE);
            break;
        case cmc_arg_kind::GroupCount:
            kernel.createSizeAnnotation(AI.offset - constantPayloadStart, iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS);
            break;
        case cmc_arg_kind::Buffer:
            kernel.createPointerGlobalAnnotation(AI.index, AI.sizeInBytes, AI.offset - constantPayloadStart, AI.BTI);
            kernel.createBufferStatefulAnnotation(AI.index);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::SVM:
            kernel.createPointerGlobalAnnotation(AI.index, AI.sizeInBytes, AI.offset - constantPayloadStart, AI.BTI);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::Sampler:
            kernel.createSamplerAnnotation(AI.index);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::Image1d:
            kernel.createImageAnnotation(AI.index, AI.BTI, 1, isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::Image2d:
            kernel.createImageAnnotation(AI.index, AI.BTI, 2, isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::Image3d:
            kernel.createImageAnnotation(AI.index, AI.BTI, 3, isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::PrintBuffer:
            kernel.m_kernelInfo.m_printfBufferAnnotation = new iOpenCL::PrintfBufferAnnotation();
            kernel.m_kernelInfo.m_printfBufferAnnotation->AnnotationSize = sizeof(kernel.m_kernelInfo.m_printfBufferAnnotation);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            kernel.m_kernelInfo.m_printfBufferAnnotation->ArgumentNumber = AI.index;
            kernel.m_kernelInfo.m_printfBufferAnnotation->PayloadPosition = AI.offset - constantPayloadStart;
            kernel.m_kernelInfo.m_printfBufferAnnotation->Index = 0;
            kernel.m_kernelInfo.m_printfBufferAnnotation->DataSize = 8;
            break;
        case cmc_arg_kind::PrivateBase:
            if (info->StatelessPrivateMemSize) {
                kernel.createPrivateBaseAnnotation(AI.index, AI.sizeInBytes,
                    AI.offset - constantPayloadStart, AI.BTI, info->StatelessPrivateMemSize);
                kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            }
            break;
        }
    }

    int32_t ConstantBufferLength = maxArgEnd - constantPayloadStart;
    ConstantBufferLength = iSTD::Align(ConstantBufferLength, info->GRFByteSize) / info->GRFByteSize;
    kernel.m_kernelInfo.m_kernelProgram.ConstantBufferLength = ConstantBufferLength;

    IGC::SProgramOutput *kernelProgram = nullptr;
    if (info->CompiledSIMDSize == 8)
      kernelProgram = &kernel.m_kernelInfo.m_kernelProgram.simd8;
    else if (info->CompiledSIMDSize == 16)
      kernelProgram = &kernel.m_kernelInfo.m_kernelProgram.simd16;
    else if (info->CompiledSIMDSize == 32 || info->CompiledSIMDSize == 1)
      kernelProgram = &kernel.m_kernelInfo.m_kernelProgram.simd32;
    IGC_ASSERT(kernelProgram);
    if (info->RelocationTable.Size > 0) {
      kernelProgram->m_funcRelocationTable = info->RelocationTable.Buf;
      kernelProgram->m_funcRelocationTableSize = info->RelocationTable.Size;
      kernelProgram->m_funcRelocationTableEntries =
          info->RelocationTable.NumEntries;
    }
    if (info->SymbolTable.Size > 0) {
      kernelProgram->m_funcSymbolTable = info->SymbolTable.Buf;
      kernelProgram->m_funcSymbolTableSize = info->SymbolTable.Size;
      kernelProgram->m_funcSymbolTableEntries = info->SymbolTable.NumEntries;
    }
}

// Combine cmc compiler metadata with jitter info.
static void populateKernelInfo_v2(const cmc_kernel_info_v2* info,
                                  const FINALIZER_INFO& JITInfo,
                                  llvm::ArrayRef<uint8_t> genBin,
                                  CMKernel& kernel)
{
    // ExecutionEnivronment:
    //
    auto& kInfo = kernel.m_kernelInfo;
    kInfo.m_kernelName = info->name;
    // Fixed SIMD8.
    kInfo.m_executionEnivronment.CompiledSIMDSize = info->CompiledSIMDSize;
    // SLM size in bytes, align to 1KB.
    kInfo.m_executionEnivronment.SumFixedTGSMSizes = iSTD::Align(info->SLMSize, 1024);
    kInfo.m_executionEnivronment.HasBarriers = info->HasBarriers;
    kInfo.m_executionEnivronment.HasReadWriteImages = info->HasReadWriteImages;
    kInfo.m_executionEnivronment.SubgroupIndependentForwardProgressRequired = true;
    kInfo.m_executionEnivronment.NumGRFRequired = info->NumGRFRequired;

    // Allocate spill-fill buffer
    if (JITInfo.isSpill || JITInfo.hasStackcalls) {
        kInfo.m_executionEnivronment.PerThreadScratchSpace += JITInfo.spillMemUsed;
    }
    if (!JITInfo.hasStackcalls && info->ThreadPrivateMemSize) {
        // CM stack calls and thread-private memory use the same value to control
        // scratch space. Consequently, if we have stack calls, there is no need
        // to add this value for thread-private memory. It should be fixed if
        // these features begin to calculate the required space separately.
        kInfo.m_executionEnivronment.PerThreadScratchSpace +=
            info->ThreadPrivateMemSize;
    }

    // ThreadPayload
    kInfo.m_threadPayload.HasLocalIDx = info->HasLocalIDx;
    kInfo.m_threadPayload.HasLocalIDy = info->HasLocalIDy;
    kInfo.m_threadPayload.HasLocalIDz = info->HasLocalIDz;
    kInfo.m_threadPayload.HasGroupID = info->HasGroupID;
    kInfo.m_threadPayload.HasLocalID = info->HasLocalIDx || info->HasLocalIDy || info->HasLocalIDz;
    kInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
    kInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = JITInfo.offsetToSkipPerThreadDataLoad;

    // Kernel binary, padding is hard-coded.
    size_t size = genBin.size();
    size_t padding = iSTD::GetAlignmentOffset(size, 64);
    void* kernelBin = IGC::aligned_malloc(size + padding, 16);
    memcpy_s(kernelBin, size + padding, genBin.data(), size);
    // pad out the rest with 0s
    std::memset(static_cast<char*>(kernelBin) + size, 0, padding);

    // Update program info.
    std::memset(&kernel.m_prog, 0, sizeof(IGC::SProgramOutput));
    kernel.m_prog.m_programBin = kernelBin;
    kernel.m_prog.m_programSize = size + padding;
    kernel.m_prog.m_unpaddedProgramSize = size;
    kernel.m_prog.m_InstructionCount = JITInfo.numAsmCount;

    // Iterate kernel arguments (This should stay in sync with cmc on resource type.)
    int numUAVs = 0;
    int numResources = 0;

    auto isResource = [](cmc_arg_kind kind) {
        switch (kind) {
        case cmc_arg_kind::Buffer:
        case cmc_arg_kind::Image1d:
        case cmc_arg_kind::Image2d:
        case cmc_arg_kind::Image3d:
        case cmc_arg_kind::SVM:
            return true;
        default:
            break;
        }
        return false;
    };

    // cmc does not do stateless-to-stateful optimization, therefore
    // set >4GB to true by default, to false if we see any resource-type
    kInfo.m_executionEnivronment.CompiledForGreaterThan4GBBuffers = true;
    for (unsigned i = 0; i < info->num_args; ++i) {
        IGC_ASSERT(info->arg_descs);
        cmc_arg_info& AI = info->arg_descs[i];
        if (isResource(AI.kind)) {
            if (AI.kind == cmc_arg_kind::Buffer || AI.kind == cmc_arg_kind::SVM)
                numUAVs++;
            else if (AI.access == cmc_access_kind::write_only ||
                     AI.access == cmc_access_kind::read_write)
                numUAVs++;
            else
                numResources++;
            kInfo.m_executionEnivronment.CompiledForGreaterThan4GBBuffers = false;
        }
    }

    // update BTI layout.
    kernel.RecomputeBTLayout(numUAVs, numResources);

    // Generate OCL patch tokens.
    generatePatchTokens_v2(info, kernel);
}

int cmc::vISACompile_v2(cmc_compile_info_v2* output, iOpenCL::CGen8CMProgram& CMProgram,
                        std::vector<const char*> &opts)
{
    int status = 0;
    const char* platformStr = getPlatformStr(CMProgram.getPlatform());

    // JIT compile kernels in vISA
    IGC_ASSERT_MESSAGE(output->kernel_info_v2, "null kernel info");
    for (unsigned i = 0; i < output->num_kernels; ++i) {
        cmc_kernel_info_v2* info = output->kernel_info_v2 + i;
        void* genBinary = nullptr;
        unsigned genBinarySize = 0;
        FINALIZER_INFO JITInfo;
        status = JITCompile(
            info->name, output->binary, (unsigned)output->binary_size,
            genBinary, genBinarySize, platformStr, output->visa_major_version,
            output->visa_minor_version, (unsigned)opts.size(), opts.data(),
            nullptr, &JITInfo);
        if (status != 0)
            return status;

        // Populate kernel info.
        CMKernel* K = new CMKernel(CMProgram.getPlatform());
        CMProgram.m_kernels.push_back(K);
        llvm::ArrayRef<uint8_t> genBin(static_cast<uint8_t*>(genBinary), genBinarySize);
        populateKernelInfo_v2(info, JITInfo, genBin, *K);
        freeBlock(genBinary);
    }

    // build binary.
    CMProgram.CreateKernelBinaries();
    return status;
}
