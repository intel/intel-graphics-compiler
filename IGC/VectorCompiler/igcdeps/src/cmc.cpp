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

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include "vc/igcdeps/cmc.h"
#include "RT_Jitter_Interface.h"
#include "inc/common/igfxfmid.h"
#include "AdaptorOCL/OCL/sp/spp_g8.h"
#include "common/secure_mem.h"
#include "common/secure_string.h"

#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <numeric>
#include <string>

#include "Probe/Assertion.h"

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
        return zebin::PreDefinedAttrGetter::ArgType::group_count;
    default:
        IGC_ASSERT_MESSAGE(0, "unsupported argument type");
        return zebin::PreDefinedAttrGetter::ArgType::arg_byvalue;
    }
}

static zebin::PreDefinedAttrGetter::ArgAccessType
getZEArgAccessType(vc::ocl::ArgAccessKind accessKind)
{
    using ArgAccessKind = vc::ocl::ArgAccessKind;
    switch(accessKind)
    {
    case ArgAccessKind::ReadOnly:
        return zebin::PreDefinedAttrGetter::ArgAccessType::readonly;
    case ArgAccessKind::WriteOnly:
        return zebin::PreDefinedAttrGetter::ArgAccessType::writeonly;
    case ArgAccessKind::ReadWrite:
        return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    case ArgAccessKind::None:
    default:
        IGC_ASSERT_MESSAGE(0, "invalid access type");
        return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    }
}

namespace {
class KernelArgInfoBuilder
{
    struct AccessQualifiers
    {
        static constexpr const char* None = "NONE";
        static constexpr const char* ReadOnly = "read_only";
        static constexpr const char* WriteOnly = "write_only";
        static constexpr const char* ReadWrite = "read_write";

        static const char* get(vc::ocl::ArgAccessKind AccessKindID)
        {
            switch (AccessKindID)
            {
                case vc::ocl::ArgAccessKind::None:
                    return None;
                case vc::ocl::ArgAccessKind::ReadOnly:
                    return ReadOnly;
                case vc::ocl::ArgAccessKind::WriteOnly:
                    return WriteOnly;
                case vc::ocl::ArgAccessKind::ReadWrite:
                default:
                    return ReadWrite;
            }
        }
    };
    struct AddressQualifiers
    {
        static constexpr const char* Global = "__global";
        static constexpr const char* Local = "__local";
        static constexpr const char* Private = "__private";
        static constexpr const char* Constant = "__constant";
        static constexpr const char* NotSpecified = "not_specified";

        static const char* get(vc::ocl::ArgKind ArgKindID)
        {
            switch (ArgKindID)
            {
                case vc::ocl::ArgKind::General:
                    return Private;
                case vc::ocl::ArgKind::Buffer:
                case vc::ocl::ArgKind::SVM:
                case vc::ocl::ArgKind::Image1d:
                case vc::ocl::ArgKind::Image2d:
                case vc::ocl::ArgKind::Image3d:
                    return Global;
                case vc::ocl::ArgKind::Sampler:
                    return Constant;
                default:
                    IGC_ASSERT_EXIT_MESSAGE(0, "implicit args cannot appear in kernel arg info");
            }
            return NotSpecified;
        }
    };
    struct TypeQualifiers
    {
        static constexpr const char* None = "NONE";
        static constexpr const char* Const = "const";
        static constexpr const char* Volatile = "volatile";
        static constexpr const char* Restrict = "restrict";
        static constexpr const char* Pipe = "pipe";
    };
    using ArgInfoSeq = std::vector<iOpenCL::KernelArgumentInfoAnnotation*>;
    ArgInfoSeq ArgInfos;

public:
    void insert(int Index, vc::ocl::ArgKind ArgKindID, vc::ocl::ArgAccessKind AccessKindID)
    {
        resizeStorageIfRequired(Index + 1);
        ArgInfos[Index] = get(ArgKindID, AccessKindID);
    }

    // It is users responsibility to delete the annotation.
    static iOpenCL::KernelArgumentInfoAnnotation* get(vc::ocl::ArgKind ArgKindID,
            vc::ocl::ArgAccessKind AccessKind = vc::ocl::ArgAccessKind::None)
    {
        auto* Annotation = new iOpenCL::KernelArgumentInfoAnnotation;
        Annotation->AddressQualifier = AddressQualifiers::get(ArgKindID);
        Annotation->AccessQualifier = AccessQualifiers::get(AccessKind);
        Annotation->ArgumentName = "";
        Annotation->TypeName = "";
        Annotation->TypeQualifier = TypeQualifiers::None;
        return Annotation;
    }

    ArgInfoSeq emit() const &
    {
        IGC_ASSERT_MESSAGE(checkArgInfosCorrectness(),
                           "arg info token is incorrect");
        return ArgInfos;
    }

    ArgInfoSeq emit() &&
    {
        IGC_ASSERT_MESSAGE(checkArgInfosCorrectness(),
                           "arg info token is incorrect");
        return std::move(ArgInfos);
    }

private:
    void resizeStorageIfRequired(int RequiredSize)
    {
        IGC_ASSERT_MESSAGE(RequiredSize > 0, "invalid required size");
        if (RequiredSize <= static_cast<int>(ArgInfos.size()))
            return;
        ArgInfos.resize(RequiredSize, nullptr);
    }

    // Returns whether arg infos are correct.
    bool checkArgInfosCorrectness() const
    {
        return std::none_of(ArgInfos.begin(), ArgInfos.end(),
            [](iOpenCL::KernelArgumentInfoAnnotation* ArgInfo){ return ArgInfo == nullptr; });
    }
};
} // anonymous namespace

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

    // EnableZEBinary: ZEBinary related code
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

void CMKernel::createPointerGlobalAnnotation(unsigned index, unsigned offset,
                                             unsigned sizeInBytes, unsigned BTI,
                                             vc::ocl::ArgAccessKind access)
{
    iOpenCL::PointerArgumentAnnotation* ptrAnnotation = new iOpenCL::PointerArgumentAnnotation;
    ptrAnnotation->IsStateless = true;
    ptrAnnotation->IsBindlessAccess = false;
    ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
    ptrAnnotation->AnnotationSize = sizeof(ptrAnnotation);
    ptrAnnotation->ArgumentNumber = index;
    ptrAnnotation->BindingTableIndex = BTI;
    ptrAnnotation->PayloadPosition = offset;
    ptrAnnotation->PayloadSizeInBytes = sizeInBytes;
    ptrAnnotation->LocationIndex = 0;
    ptrAnnotation->LocationCount = 0;
    ptrAnnotation->IsEmulationArgument = false;
    m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);

    // EnableZEBinary: ZEBinary related code
    zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
        m_kernelInfo.m_zePayloadArgs, offset, sizeInBytes, index,
        zebin::PreDefinedAttrGetter::ArgAddrMode::stateless,
        zebin::PreDefinedAttrGetter::ArgAddrSpace::global,
        getZEArgAccessType(access));
    zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, BTI,
                                               index);
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

    // EnableZEBinary: ZEBinary related code
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
        zebin::PreDefinedAttrGetter::ArgType::private_base_stateless,
        payloadPosition, byteSize);
}

void CMKernel::createBufferStatefulAnnotation(unsigned argNo,
                                              vc::ocl::ArgAccessKind accessKind)
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

    // EnableZEBinary: ZEBinary related code
    zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
        0, 0, argNo,
        zebin::PreDefinedAttrGetter::ArgAddrMode::stateful,
        zebin::PreDefinedAttrGetter::ArgAddrSpace::global,
        getZEArgAccessType(accessKind));
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
    // EnableZEBinary: ZEBinary related code
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

static void generateSymbols(const vc::ocl::KernelInfo& info,
                            IGC::SProgramOutput& kernelProgram)
{
    if (info.RelocationTable.Size > 0) {
        kernelProgram.m_funcRelocationTable = info.RelocationTable.Buf;
        kernelProgram.m_funcRelocationTableSize = info.RelocationTable.Size;
        kernelProgram.m_funcRelocationTableEntries =
            info.RelocationTable.NumEntries;
        // EnableZEBinary: ZEBinary related code
        kernelProgram.m_relocs = info.ZEBinInfo.Relocations;
    }
    if (info.SymbolTable.Size > 0) {
        kernelProgram.m_funcSymbolTable = info.SymbolTable.Buf;
        kernelProgram.m_funcSymbolTableSize = info.SymbolTable.Size;
        kernelProgram.m_funcSymbolTableEntries = info.SymbolTable.NumEntries;
        // EnableZEBinary: ZEBinary related code
        kernelProgram.m_symbols.function = info.ZEBinInfo.Symbols.Functions;
    }
    // EnableZEBinary: ZEBinary related code
    kernelProgram.m_symbols.local = info.ZEBinInfo.Symbols.Local;
}

void generateKernelArgInfo(const std::vector<vc::ocl::ArgInfo> &Args,
                           std::vector<iOpenCL::KernelArgumentInfoAnnotation*> &ArgsAnnotation)
{
    KernelArgInfoBuilder ArgsAnnotationBuilder;
    for (auto &Arg : Args)
        switch(Arg.Kind)
        {
          case vc::ocl::ArgKind::General:
          case vc::ocl::ArgKind::Buffer:
          case vc::ocl::ArgKind::SVM:
          case vc::ocl::ArgKind::Sampler:
          case vc::ocl::ArgKind::Image1d:
          case vc::ocl::ArgKind::Image2d:
          case vc::ocl::ArgKind::Image3d:
              ArgsAnnotationBuilder.insert(Arg.Index, Arg.Kind, Arg.AccessKind);
              break;
          default:
              continue;
        }
    ArgsAnnotation = std::move(ArgsAnnotationBuilder).emit();
}

static void generatePatchTokens_v2(const vc::ocl::KernelInfo& info,
                                   const vc::ocl::GTPinInfo* ginfo,
                                   CMKernel& kernel)
{
    llvm::transform(
        llvm::enumerate(info.PrintStrings),
        std::back_inserter(kernel.m_kernelInfo.m_printfStringAnnotations),
        [](const auto EnumStr) {
            auto* stringAnnotation = new iOpenCL::PrintfStringAnnotation;
            stringAnnotation->Index = EnumStr.index();
            const std::string& printString = EnumStr.value();
            const unsigned stringSize = printString.size();
            stringAnnotation->StringSize = stringSize;
            // Though string size is present in annotation, string
            // should be null terminated: patchtokens processor
            // ignores size field for some reason.
            stringAnnotation->StringData = new char[stringSize + 1];
            std::copy_n(printString.c_str(), stringSize + 1,
                        stringAnnotation->StringData);
            return stringAnnotation;
        });

    // This is the starting constant thread payload
    // r0-r1 are reserved for SIMD1 dispatch
    const unsigned constantPayloadStart = info.GRFSizeInBytes * 2;

    // Setup argument to BTI mapping.
    kernel.m_kernelInfo.m_argIndexMap.clear();

    for (const vc::ocl::ArgInfo& arg : info.Args)
    {
        const bool isWriteable =
            arg.AccessKind != vc::ocl::ArgAccessKind::ReadOnly;
        IGC_ASSERT_MESSAGE(arg.Offset >= constantPayloadStart,
                           "Argument overlaps with thread payload");
        const unsigned argOffset = arg.Offset - constantPayloadStart;

        using ArgKind = vc::ocl::ArgKind;
        switch (arg.Kind)
        {
        default:
            break;
        case ArgKind::General:
            kernel.createConstArgumentAnnotation(arg.Index, arg.SizeInBytes,
                                                 argOffset);
            break;
        case ArgKind::LocalSize:
            kernel.createSizeAnnotation(
                argOffset, iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE);
            break;
        case ArgKind::GroupCount:
            kernel.createSizeAnnotation(
                argOffset, iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS);
            break;
        case ArgKind::Buffer:
            kernel.createPointerGlobalAnnotation(
                arg.Index, argOffset, arg.SizeInBytes, arg.BTI, arg.AccessKind);
            kernel.createBufferStatefulAnnotation(arg.Index, arg.AccessKind);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::SVM:
            kernel.createPointerGlobalAnnotation(
                arg.Index, argOffset, arg.SizeInBytes, arg.BTI, arg.AccessKind);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::Sampler:
            kernel.createSamplerAnnotation(arg.Index);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::Image1d:
            kernel.createImageAnnotation(arg.Index, arg.BTI, /*dim=*/1,
                                         isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::Image2d:
            kernel.createImageAnnotation(arg.Index, arg.BTI, /*dim=*/2,
                                         isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::Image3d:
            kernel.createImageAnnotation(arg.Index, arg.BTI, /*dim=*/3,
                                         isWriteable);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            break;
        case ArgKind::PrintBuffer:
            kernel.m_kernelInfo.m_printfBufferAnnotation = new iOpenCL::PrintfBufferAnnotation();
            kernel.m_kernelInfo.m_printfBufferAnnotation->AnnotationSize = sizeof(kernel.m_kernelInfo.m_printfBufferAnnotation);
            kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            kernel.m_kernelInfo.m_printfBufferAnnotation->ArgumentNumber =
                arg.Index;
            kernel.m_kernelInfo.m_printfBufferAnnotation->PayloadPosition =
                argOffset;
            kernel.m_kernelInfo.m_printfBufferAnnotation->Index = 0;
            kernel.m_kernelInfo.m_printfBufferAnnotation->DataSize = 8;
            break;
        case ArgKind::PrivateBase:
            if (info.StatelessPrivateMemSize)
            {
                kernel.createPrivateBaseAnnotation(
                    arg.Index, arg.SizeInBytes, argOffset, arg.BTI,
                    info.StatelessPrivateMemSize);
                kernel.m_kernelInfo.m_executionEnivronment
                    .PerThreadPrivateOnStatelessSize =
                    info.StatelessPrivateMemSize;
                kernel.m_kernelInfo.m_argIndexMap[arg.Index] = arg.BTI;
            }
            break;
        }
    }
    generateKernelArgInfo(info.Args, kernel.m_kernelInfo.m_kernelArgInfo);

    const unsigned maxArgEnd = std::accumulate(
        info.Args.begin(), info.Args.end(), constantPayloadStart,
        [](unsigned maxArgEnd, const vc::ocl::ArgInfo& arg) {
            return std::max(maxArgEnd, arg.Offset + arg.SizeInBytes);
        });
    const unsigned constantBufferLengthInGRF =
        iSTD::Align(maxArgEnd - constantPayloadStart, info.GRFSizeInBytes) /
        info.GRFSizeInBytes;
    kernel.m_kernelInfo.m_kernelProgram.ConstantBufferLength =
        constantBufferLengthInGRF;

    IGC_ASSERT_MESSAGE(
        kernel.m_kernelInfo.m_executionEnivronment.CompiledSIMDSize == 1,
        "CM code must be dispatched in SIMD1 mode");
    IGC::SProgramOutput& kernelProgram = kernel.getProgramOutput();

    generateSymbols(info, kernelProgram);

    // GTPin
    if (ginfo && ginfo->GTPinBuffer.size() > 0) {
        size_t bufSize = ginfo->GTPinBuffer.size();
        void *gtpinBuffer = IGC::aligned_malloc(bufSize, 16);
        memcpy_s(gtpinBuffer, bufSize, ginfo->GTPinBuffer.data(), bufSize);

        kernelProgram.m_gtpinBufferSize = bufSize;
        kernelProgram.m_gtpinBuffer = gtpinBuffer;
    }
}

// Combine vc compiler metadata with jitter info.
static void populateKernelInfo_v2(const vc::ocl::KernelInfo& info,
                                  const FINALIZER_INFO& JITInfo,
                                  const vc::ocl::GTPinInfo* GtpinInfo,
                                  llvm::ArrayRef<uint8_t> genBin,
                                  llvm::ArrayRef<uint8_t> dbgInfo,
                                  CMKernel& kernel)
{
    kernel.m_GRFSizeInBytes = info.GRFSizeInBytes;
    // ExecutionEnivronment:
    //
    IGC::SOpenCLKernelInfo& kInfo = kernel.m_kernelInfo;
    kInfo.m_kernelName = info.Name;
    // Fixed SIMD1.
    kInfo.m_executionEnivronment.CompiledSIMDSize = 1;
    // SLM size in bytes, align to 1KB.
    kInfo.m_executionEnivronment.SumFixedTGSMSizes =
        iSTD::Align(info.SLMSize, 1024);
    kInfo.m_executionEnivronment.HasBarriers = info.HasBarriers;
    kInfo.m_executionEnivronment.HasReadWriteImages = info.HasReadWriteImages;
    kInfo.m_executionEnivronment.SubgroupIndependentForwardProgressRequired = true;
    kInfo.m_executionEnivronment.NumGRFRequired = JITInfo.numGRFTotal;

    // Allocate spill-fill buffer
    if (JITInfo.isSpill || JITInfo.hasStackcalls) {
        kInfo.m_executionEnivronment.PerThreadScratchSpace += JITInfo.spillMemUsed;
    }
    if (!JITInfo.hasStackcalls && info.ThreadPrivateMemSize)
    {
        // CM stack calls and thread-private memory use the same value to control
        // scratch space. Consequently, if we have stack calls, there is no need
        // to add this value for thread-private memory. It should be fixed if
        // these features begin to calculate the required space separately.
        kInfo.m_executionEnivronment.PerThreadScratchSpace +=
            info.ThreadPrivateMemSize;
    }

    // ThreadPayload.
    {
        iOpenCL::ThreadPayload& payload = kInfo.m_threadPayload;
        // Local IDs are always present now.
        payload.HasLocalIDx = true;
        payload.HasLocalIDy = true;
        payload.HasLocalIDz = true;
        payload.HasGroupID = info.HasGroupID;
        payload.HasLocalID =
            payload.HasLocalIDx || payload.HasLocalIDy || payload.HasLocalIDz;
        payload.CompiledForIndirectPayloadStorage = true;
        payload.OffsetToSkipPerThreadDataLoad =
            JITInfo.offsetToSkipPerThreadDataLoad;
    }

    // Kernel binary, padding is hard-coded.
    const size_t size = genBin.size();
    const size_t padding = iSTD::GetAlignmentOffset(size, 64);
    void* kernelBin = IGC::aligned_malloc(size + padding, 16);
    memcpy_s(kernelBin, size + padding, genBin.data(), size);
    // pad out the rest with 0s
    std::memset(static_cast<char*>(kernelBin) + size, 0, padding);

    // Update program info.
    std::memset(&kernel.getProgramOutput(), 0, sizeof(IGC::SProgramOutput));
    kernel.getProgramOutput().m_programBin = kernelBin;
    kernel.getProgramOutput().m_programSize = size + padding;
    kernel.getProgramOutput().m_unpaddedProgramSize = size;
    kernel.getProgramOutput().m_InstructionCount = JITInfo.numAsmCount;

    const size_t DbgSize = dbgInfo.size();
    if (DbgSize)
    {
        void* debugInfo = IGC::aligned_malloc(DbgSize, sizeof(void*));
        memcpy_s(debugInfo, DbgSize, dbgInfo.data(), DbgSize);
        kernel.getProgramOutput().m_debugDataVISA = debugInfo;
        kernel.getProgramOutput().m_debugDataVISASize = DbgSize;
    }

    if (JITInfo.numBytesScratchGtpin > 0)
    {
        kernel.getProgramOutput().m_scratchSpaceUsedByGtpin = JITInfo.numBytesScratchGtpin;
        kInfo.m_executionEnivronment.PerThreadScratchSpace += JITInfo.numBytesScratchGtpin;
    }

    // Iterate kernel arguments (This should stay in sync with cmc on resource type.)
    int numUAVs = 0;
    int numResources = 0;

    auto isResource = [](vc::ocl::ArgKind kind) {
        using ArgKind = vc::ocl::ArgKind;
        switch (kind)
        {
        case ArgKind::Buffer:
        case ArgKind::Image1d:
        case ArgKind::Image2d:
        case ArgKind::Image3d:
        case ArgKind::SVM:
            return true;
        default:
            break;
        }
        return false;
    };

    // cmc does not do stateless-to-stateful optimization, therefore
    // set >4GB to true by default, to false if we see any resource-type
    kInfo.m_executionEnivronment.CompiledForGreaterThan4GBBuffers = true;
    for (const vc::ocl::ArgInfo& arg : info.Args)
    {
        if (isResource(arg.Kind))
        {
            if (arg.Kind == vc::ocl::ArgKind::Buffer ||
                arg.Kind == vc::ocl::ArgKind::SVM)
                numUAVs++;
            else if (arg.AccessKind == vc::ocl::ArgAccessKind::WriteOnly ||
                     arg.AccessKind == vc::ocl::ArgAccessKind::ReadWrite)
                numUAVs++;
            else
                numResources++;
            kInfo.m_executionEnivronment.CompiledForGreaterThan4GBBuffers = false;
        }
    }

    // update BTI layout.
    kernel.RecomputeBTLayout(numUAVs, numResources);

    // Generate OCL patch tokens.
    generatePatchTokens_v2(info, GtpinInfo, kernel);
}

void vc::createBinary(
    iOpenCL::CGen8CMProgram& CMProgram,
    const std::vector<vc::ocl::CompileInfo>& CompileInfos)
{
    bool ProgramIsDebuggable = false;
    for (const vc::ocl::CompileInfo& Info : CompileInfos)
    {
        CMKernel* K = new CMKernel(CMProgram.getPlatform());
        CMProgram.m_kernels.push_back(K);
        llvm::ArrayRef<uint8_t> GenBin{
            reinterpret_cast<const uint8_t*>(Info.GenBinary.data()),
            Info.GenBinary.size()};
        llvm::ArrayRef<uint8_t> DbgInfo{
            reinterpret_cast<const uint8_t*>(Info.DebugInfo.data()),
            Info.DebugInfo.size()};
        populateKernelInfo_v2(Info.KernelInfo, Info.JitInfo, &(Info.GtpinInfo),
                              GenBin, DbgInfo, *K);

        ProgramIsDebuggable |= (Info.DebugInfo.size() > 0);
    }
    CMProgram.m_ContextProvider.updateDebuggableStatus(ProgramIsDebuggable);
}
