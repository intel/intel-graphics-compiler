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

static zebin::PreDefinedAttrGetter::ArgType getZEArgType(iOpenCL::DATA_PARAMETER_TOKEN type) {
    switch(type) {
    case iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE:
        return zebin::PreDefinedAttrGetter::ArgType::local_size;
    case iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET:
        return zebin::PreDefinedAttrGetter::ArgType::global_id_offset;
    case iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS:
        // copied from OCL behavior
        return zebin::PreDefinedAttrGetter::ArgType::group_size;
    default:
        IGC_ASSERT_MESSAGE(0, "unsupported argument type");
        return zebin::PreDefinedAttrGetter::ArgType::arg_byvalue;
    }
}

static zebin::PreDefinedAttrGetter::ArgAccessType getZEArgAccessType(cmc_access_kind accessKind)
{
    switch(accessKind)
    {
    case cmc_access_kind::read_only:
        return zebin::PreDefinedAttrGetter::ArgAccessType::readonly;
    case cmc_access_kind::write_only:
        return zebin::PreDefinedAttrGetter::ArgAccessType::writeonly;
    case cmc_access_kind::read_write:
        return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    case cmc_access_kind::undef:
    default:
        IGC_ASSERT_MESSAGE(0, "invalid access type");
        return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    }
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

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
            zebin::ZEInfoBuilder::addPayloadArgumentByValue(m_kernelInfo.m_zePayloadArgs,
                payloadPosition, sizeInBytes, argNo);
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

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
        IGC_ASSERT_MESSAGE(0, "not yet supported for L0 binary");
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

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
        IGC_ASSERT_MESSAGE(0, "not yet supported for L0 binary");
}

void CMKernel::createImplicitArgumentsAnnotation(unsigned payloadPosition)
{
    createSizeAnnotation(payloadPosition, iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET);
    payloadPosition += 3 * iOpenCL::DATA_PARAMETER_DATA_SIZE;
    createSizeAnnotation(payloadPosition, iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE);
}

void CMKernel::createPointerGlobalAnnotation(const cmc_arg_info &argInfo)
{
    iOpenCL::PointerArgumentAnnotation* ptrAnnotation = new iOpenCL::PointerArgumentAnnotation;
    ptrAnnotation->IsStateless = true;
    ptrAnnotation->IsBindlessAccess = false;
    ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
    ptrAnnotation->AnnotationSize = sizeof(ptrAnnotation);
    ptrAnnotation->ArgumentNumber = argInfo.index;
    ptrAnnotation->BindingTableIndex = argInfo.BTI;
    ptrAnnotation->PayloadPosition = argInfo.offset;
    ptrAnnotation->PayloadSizeInBytes = argInfo.sizeInBytes;
    ptrAnnotation->LocationIndex = 0;
    ptrAnnotation->LocationCount = 0;
    ptrAnnotation->IsEmulationArgument = false;
    m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
    {
        zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
            argInfo.offset, argInfo.sizeInBytes, argInfo.index,
            zebin::PreDefinedAttrGetter::ArgAddrMode::stateless,
            zebin::PreDefinedAttrGetter::ArgAddrSpace::global,
            getZEArgAccessType(argInfo.access));
        zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs,
            argInfo.BTI, argInfo.index);
    }
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

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
        IGC_ASSERT_MESSAGE(0, "not yet supported for L0 binary");
}

void CMKernel::createBufferStatefulAnnotation(unsigned argNo, cmc_access_kind accessKind)
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

    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
    {
        zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
            0, 0, argNo,
            zebin::PreDefinedAttrGetter::ArgAddrMode::stateful,
            zebin::PreDefinedAttrGetter::ArgAddrSpace::global,
            getZEArgAccessType(accessKind));
    }
}

void CMKernel::createSizeAnnotation(unsigned initPayloadPosition,
                                    iOpenCL::DATA_PARAMETER_TOKEN Type)
{
    for (int i = 0, payloadPosition = initPayloadPosition;
         i < 3;
         ++i, payloadPosition += iOpenCL::DATA_PARAMETER_DATA_SIZE)
    {
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
    }
    if (IGC_IS_FLAG_ENABLED(EnableZEBinary))
        zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
            getZEArgType(Type), initPayloadPosition,
            iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
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

static void generateSymbols(const cmc_kernel_info_v2 &info,
                            IGC::SProgramOutput &kernelProgram)
{
    if (info.RelocationTable.Size > 0) {
      kernelProgram.m_funcRelocationTable = info.RelocationTable.Buf;
      kernelProgram.m_funcRelocationTableSize = info.RelocationTable.Size;
      kernelProgram.m_funcRelocationTableEntries =
          info.RelocationTable.NumEntries;
    }
    if (info.SymbolTable.Size > 0) {
      kernelProgram.m_funcSymbolTable = info.SymbolTable.Buf;
      kernelProgram.m_funcSymbolTableSize = info.SymbolTable.Size;
      kernelProgram.m_funcSymbolTableEntries = info.SymbolTable.NumEntries;
    }
}

static void generatePatchTokens_v2(const cmc_kernel_info_v2 *info,
                                   CMKernel& kernel)
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
        cmc_arg_info AI = info->arg_descs[i];
        if (AI.offset > 0)
            maxArgEnd = std::max(AI.offset + AI.sizeInBytes, maxArgEnd);
        bool isWriteable = AI.access != cmc_access_kind::read_only;
        AI.offset -= constantPayloadStart;

        switch (AI.kind) {
        default:
            break;
        case cmc_arg_kind::General:
            kernel.createConstArgumentAnnotation(AI.index, AI.sizeInBytes, AI.offset);
            break;
        case cmc_arg_kind::LocalSize:
            kernel.createSizeAnnotation(AI.offset, iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE);
            break;
        case cmc_arg_kind::GroupCount:
            kernel.createSizeAnnotation(AI.offset, iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS);
            break;
        case cmc_arg_kind::Buffer:
            kernel.createPointerGlobalAnnotation(AI);
            kernel.createBufferStatefulAnnotation(AI.index, AI.access);
            kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            break;
        case cmc_arg_kind::SVM:
            kernel.createPointerGlobalAnnotation(AI);
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
            kernel.m_kernelInfo.m_printfBufferAnnotation->PayloadPosition = AI.offset;
            kernel.m_kernelInfo.m_printfBufferAnnotation->Index = 0;
            kernel.m_kernelInfo.m_printfBufferAnnotation->DataSize = 8;
            break;
        case cmc_arg_kind::PrivateBase:
            if (info->StatelessPrivateMemSize) {
                kernel.createPrivateBaseAnnotation(AI.index, AI.sizeInBytes,
                    AI.offset, AI.BTI, info->StatelessPrivateMemSize);
                kernel.m_kernelInfo.m_argIndexMap[AI.index] = AI.BTI;
            }
            break;
        }
    }

    int32_t ConstantBufferLength = maxArgEnd - constantPayloadStart;
    ConstantBufferLength = iSTD::Align(ConstantBufferLength, info->GRFByteSize) / info->GRFByteSize;
    kernel.m_kernelInfo.m_kernelProgram.ConstantBufferLength = ConstantBufferLength;

    IGC_ASSERT_MESSAGE(info->CompiledSIMDSize == 1, "CM code must be dispatched in SIMD1 mode");
    IGC::SProgramOutput &kernelProgram = kernel.m_kernelInfo.m_kernelProgram.simd1;

    generateSymbols(*info, kernelProgram);

}

// Combine cmc compiler metadata with jitter info.
static void populateKernelInfo_v2(const cmc_kernel_info_v2* info,
                                  const FINALIZER_INFO& JITInfo,
                                  llvm::ArrayRef<uint8_t> genBin,
                                  CMKernel& kernel)
{
    kernel.m_GRFSizeInBytes = info->GRFByteSize;
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

static void getCmcArg(cmc_arg_info& CmcArg, const vc::ocl::ArgInfo& Arg)
{
    switch (Arg.Kind)
    {
    case vc::ocl::ArgKind::General:
        CmcArg.kind = cmc_arg_kind::General;
        break;
    case vc::ocl::ArgKind::LocalSize:
        CmcArg.kind = cmc_arg_kind::LocalSize;
        break;
    case vc::ocl::ArgKind::GroupCount:
        CmcArg.kind = cmc_arg_kind::GroupCount;
        break;
    case vc::ocl::ArgKind::Buffer:
        CmcArg.kind = cmc_arg_kind::Buffer;
        break;
    case vc::ocl::ArgKind::SVM:
        CmcArg.kind = cmc_arg_kind::SVM;
        break;
    case vc::ocl::ArgKind::Sampler:
        CmcArg.kind = cmc_arg_kind::Sampler;
        break;
    case vc::ocl::ArgKind::Image1d:
        CmcArg.kind = cmc_arg_kind::Image1d;
        break;
    case vc::ocl::ArgKind::Image2d:
        CmcArg.kind = cmc_arg_kind::Image2d;
        break;
    case vc::ocl::ArgKind::Image3d:
        CmcArg.kind = cmc_arg_kind::Image3d;
        break;
    case vc::ocl::ArgKind::PrintBuffer:
        CmcArg.kind = cmc_arg_kind::PrintBuffer;
        break;
    case vc::ocl::ArgKind::PrivateBase:
        CmcArg.kind = cmc_arg_kind::PrivateBase;
        break;
    }

    switch (Arg.AccessKind)
    {
    case vc::ocl::ArgAccessKind::None:
        CmcArg.access = cmc_access_kind::undef;
        break;
    case vc::ocl::ArgAccessKind::ReadOnly:
        CmcArg.access = cmc_access_kind::read_only;
        break;
    case vc::ocl::ArgAccessKind::WriteOnly:
        CmcArg.access = cmc_access_kind::write_only;
        break;
    case vc::ocl::ArgAccessKind::ReadWrite:
        CmcArg.access = cmc_access_kind::read_write;
        break;
    }

    CmcArg.index = Arg.Index;
    CmcArg.offset = Arg.Offset;
    CmcArg.sizeInBytes = Arg.SizeInBytes;
    CmcArg.BTI = Arg.BTI;
}

// Returns vector of cmc_arg_info with all fields initialized.
static std::vector<cmc_arg_info> getCmcArgInfos(const std::vector<vc::ocl::ArgInfo>& Args)
{
    std::vector<cmc_arg_info> CmcArgs{Args.size()};
    for (unsigned i = 0, e = Args.size(); i != e; ++i)
        getCmcArg(CmcArgs[i], Args[i]);
    return CmcArgs;
}

static std::vector<cmc_ocl_print_string> getCmcPrintStrings(
    const std::vector<std::string>& Original)
{
    std::vector<cmc_ocl_print_string> Converted;
    std::transform(Original.begin(), Original.end(), std::back_inserter(Converted),
        [](const std::string &str) {
            IGC_ASSERT_MESSAGE(str.size() < cmc_ocl_print_string::max_width, "illegal string length");
            cmc_ocl_print_string Tmp;
            strcpy_s(Tmp.s, cmc_ocl_print_string::max_width, str.c_str());
            return Tmp;
        });
    return Converted;
}

struct CmcContext
{
    std::vector<cmc_arg_info> Args;
    std::vector<cmc_ocl_print_string> PrintStrings;
};

// Fills non-owning cmc_kernel_info with all fields initialized.
static void getCmcKernelInfo(
    cmc_kernel_info_v2& CmcInfo,
    const vc::ocl::KernelInfo& Info,
    const FINALIZER_INFO& JitInfo,
    CmcContext& CmcCtx)
{
    IGC_ASSERT_MESSAGE(CmcCtx.PrintStrings.size() == Info.PrintStrings.size(), "inconsistent arguments");
    CmcInfo.name = Info.Name.c_str();
    CmcInfo.num_args = CmcCtx.Args.size();
    CmcInfo.arg_descs = CmcCtx.Args.data();
    CmcInfo.HasLocalIDx = true;
    CmcInfo.HasLocalIDy = true;
    CmcInfo.HasLocalIDz = true;
    CmcInfo.HasGroupID = Info.HasGroupID;
    CmcInfo.CompiledSIMDSize = 1;
    CmcInfo.SLMSize = Info.SLMSize;
    CmcInfo.NumGRFRequired = JitInfo.numGRFTotal;
    CmcInfo.GRFByteSize = Info.GRFSizeInBytes;
    CmcInfo.HasBarriers = Info.HasBarriers;
    CmcInfo.StatelessPrivateMemSize = Info.StatelessPrivateMemSize;
    CmcInfo.HasReadWriteImages = Info.HasReadWriteImages;
    CmcInfo.num_print_strings = CmcCtx.PrintStrings.size();
    CmcInfo.print_string_descs = CmcCtx.PrintStrings.data();
    // std::copy requires either reinteprets or implementation of operator= in
    // TableInfos from independent headers so memcpy seems to be the best option
    // for now
    memcpy_s(&CmcInfo.RelocationTable, sizeof(Info.RelocationTable), &Info.RelocationTable,
           sizeof(Info.RelocationTable));
    memcpy_s(&CmcInfo.SymbolTable, sizeof(Info.SymbolTable), &Info.SymbolTable,
           sizeof(Info.SymbolTable));
}

void vc::createBinary(
    iOpenCL::CGen8CMProgram& CMProgram,
    const std::vector<vc::ocl::CompileInfo>& CompileInfos)
{
    cmc_kernel_info_v2 CmcInfo;
    CmcContext CmcCtx;
    for (const vc::ocl::CompileInfo& Info : CompileInfos)
    {
        CmcCtx.Args = getCmcArgInfos(Info.KernelInfo.Args);
        CmcCtx.PrintStrings = getCmcPrintStrings(Info.KernelInfo.PrintStrings);
        getCmcKernelInfo(CmcInfo, Info.KernelInfo, Info.JitInfo, CmcCtx);
        CMKernel* K = new CMKernel(CMProgram.getPlatform());
        CMProgram.m_kernels.push_back(K);
        llvm::ArrayRef<uint8_t> GenBin{
            reinterpret_cast<const uint8_t*>(Info.GenBinary.data()),
            Info.GenBinary.size()};
        populateKernelInfo_v2(&CmcInfo, Info.JitInfo, GenBin, *K);
    }
}
