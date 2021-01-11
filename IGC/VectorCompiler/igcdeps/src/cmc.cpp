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

void CMKernel::createImageAnnotation(unsigned argNo, unsigned BTI,
                                     unsigned dim, vc::ocl::ArgAccessKind Access)
{
    iOpenCL::ImageArgumentAnnotation* imageInput = new iOpenCL::ImageArgumentAnnotation;
    // As VC uses only statefull addrmode.
    constexpr int PayloadPosition = 0;
    constexpr int ArgSize = 0;

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
    imageInput->PayloadPosition = PayloadPosition;
    imageInput->Writeable = Access != vc::ocl::ArgAccessKind::ReadOnly;
    m_kernelInfo.m_imageInputAnnotations.push_back(imageInput);

    zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs,
        PayloadPosition, ArgSize, argNo, zebin::PreDefinedAttrGetter::ArgAddrMode::stateful,
        zebin::PreDefinedAttrGetter::ArgAddrSpace::image,
        getZEArgAccessType(Access));
    zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, BTI,
                                               argNo);
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
        return IGC_MANGLE("SKL");
    case IGFX_GEN10_CORE:
        return IGC_MANGLE("CNL");
    case IGFX_GEN11_CORE:
        if (platform.eProductFamily == IGFX_ICELAKE_LP ||
            platform.eProductFamily == IGFX_LAKEFIELD)
            return IGC_MANGLE("ICLLP");
        return IGC_MANGLE("ICL");
    case IGFX_GEN12_CORE:
    case IGFX_GEN12LP_CORE:
        if (platform.eProductFamily == IGFX_TIGERLAKE_LP ||
            platform.eProductFamily == IGFX_DG1)
            return IGC_MANGLE("TGLLP");
    default:
        IGC_ASSERT_MESSAGE(0, "unsupported platform");
        break;
    }
    return IGC_MANGLE("SKL");
}

static void setSymbolsInfo(const vc::ocl::KernelInfo &Info,
                           IGC::SProgramOutput &KernelProgram) {
  if (Info.RelocationTable.Size > 0) {
    KernelProgram.m_funcRelocationTable = Info.RelocationTable.Buf;
    KernelProgram.m_funcRelocationTableSize = Info.RelocationTable.Size;
    KernelProgram.m_funcRelocationTableEntries =
        Info.RelocationTable.NumEntries;
    // EnableZEBinary: ZEBinary related code
    KernelProgram.m_relocs = Info.ZEBinInfo.Relocations;
  }
  if (Info.SymbolTable.Size > 0) {
    KernelProgram.m_funcSymbolTable = Info.SymbolTable.Buf;
    KernelProgram.m_funcSymbolTableSize = Info.SymbolTable.Size;
    KernelProgram.m_funcSymbolTableEntries = Info.SymbolTable.NumEntries;
    // EnableZEBinary: ZEBinary related code
    KernelProgram.m_symbols.function = Info.ZEBinInfo.Symbols.Functions;
    KernelProgram.m_symbols.global = Info.ZEBinInfo.Symbols.Globals;
    KernelProgram.m_symbols.globalConst = Info.ZEBinInfo.Symbols.Constants;
  }
  // EnableZEBinary: ZEBinary related code
  KernelProgram.m_symbols.local = Info.ZEBinInfo.Symbols.Local;
}

static void generateKernelArgInfo(
    const std::vector<vc::ocl::ArgInfo> &Args,
    std::vector<iOpenCL::KernelArgumentInfoAnnotation *> &ArgsAnnotation) {
  KernelArgInfoBuilder ArgsAnnotationBuilder;
  for (auto &Arg : Args)
    switch (Arg.Kind) {
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

static void setArgumentsInfo(const vc::ocl::KernelInfo &Info,
                             CMKernel &Kernel) {
  llvm::transform(
      llvm::enumerate(Info.PrintStrings),
      std::back_inserter(Kernel.m_kernelInfo.m_printfStringAnnotations),
      [](const auto EnumStr) {
        auto *StringAnnotation = new iOpenCL::PrintfStringAnnotation;
        StringAnnotation->Index = EnumStr.index();
        const std::string &PrintString = EnumStr.value();
        const unsigned StringSize = PrintString.size();
        StringAnnotation->StringSize = StringSize;
        // Though string size is present in annotation, string
        // should be null terminated: patchtokens processor
        // ignores size field for some reason.
        StringAnnotation->StringData = new char[StringSize + 1];
        std::copy_n(PrintString.c_str(), StringSize + 1,
                    StringAnnotation->StringData);
        return StringAnnotation;
      });

  // This is the starting constant thread payload
  // r0-r1 are reserved for SIMD1 dispatch
  const unsigned ConstantPayloadStart = Info.GRFSizeInBytes * 2;

  // Setup argument to BTI mapping.
  Kernel.m_kernelInfo.m_argIndexMap.clear();

  for (const vc::ocl::ArgInfo &Arg : Info.Args) {
    IGC_ASSERT_MESSAGE(Arg.Offset >= ConstantPayloadStart,
                       "Argument overlaps with thread payload");
    const unsigned ArgOffset = Arg.Offset - ConstantPayloadStart;

    using ArgKind = vc::ocl::ArgKind;
    switch (Arg.Kind) {
    default:
      break;
    case ArgKind::General:
      Kernel.createConstArgumentAnnotation(Arg.Index, Arg.SizeInBytes,
                                           ArgOffset);
      break;
    case ArgKind::LocalSize:
      Kernel.createSizeAnnotation(
          ArgOffset, iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE);
      break;
    case ArgKind::GroupCount:
      Kernel.createSizeAnnotation(ArgOffset,
                                  iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS);
      break;
    case ArgKind::Buffer:
      Kernel.createPointerGlobalAnnotation(
          Arg.Index, ArgOffset, Arg.SizeInBytes, Arg.BTI, Arg.AccessKind);
      Kernel.createBufferStatefulAnnotation(Arg.Index, Arg.AccessKind);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::SVM:
      Kernel.createPointerGlobalAnnotation(
          Arg.Index, ArgOffset, Arg.SizeInBytes, Arg.BTI, Arg.AccessKind);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::Sampler:
      Kernel.createSamplerAnnotation(Arg.Index);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::Image1d:
      Kernel.createImageAnnotation(Arg.Index, Arg.BTI, /*dim=*/1,
                                   Arg.AccessKind);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::Image2d:
      Kernel.createImageAnnotation(Arg.Index, Arg.BTI, /*dim=*/2,
                                   Arg.AccessKind);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::Image3d:
      Kernel.createImageAnnotation(Arg.Index, Arg.BTI, /*dim=*/3,
                                   Arg.AccessKind);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      break;
    case ArgKind::PrintBuffer:
      Kernel.m_kernelInfo.m_printfBufferAnnotation =
          new iOpenCL::PrintfBufferAnnotation();
      Kernel.m_kernelInfo.m_printfBufferAnnotation->AnnotationSize =
          sizeof(Kernel.m_kernelInfo.m_printfBufferAnnotation);
      Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      Kernel.m_kernelInfo.m_printfBufferAnnotation->ArgumentNumber = Arg.Index;
      Kernel.m_kernelInfo.m_printfBufferAnnotation->PayloadPosition = ArgOffset;
      Kernel.m_kernelInfo.m_printfBufferAnnotation->Index = 0;
      Kernel.m_kernelInfo.m_printfBufferAnnotation->DataSize = 8;
      break;
    case ArgKind::PrivateBase:
      if (Info.StatelessPrivateMemSize) {
        Kernel.createPrivateBaseAnnotation(Arg.Index, Arg.SizeInBytes,
                                           ArgOffset, Arg.BTI,
                                           Info.StatelessPrivateMemSize);
        Kernel.m_kernelInfo.m_executionEnivronment
            .PerThreadPrivateOnStatelessSize = Info.StatelessPrivateMemSize;
        Kernel.m_kernelInfo.m_argIndexMap[Arg.Index] = Arg.BTI;
      }
      break;
    }
  }
  generateKernelArgInfo(Info.Args, Kernel.m_kernelInfo.m_kernelArgInfo);

  const unsigned MaxArgEnd = std::accumulate(
      Info.Args.begin(), Info.Args.end(), ConstantPayloadStart,
      [](unsigned MaxArgEnd, const vc::ocl::ArgInfo &Arg) {
        return std::max(MaxArgEnd, Arg.Offset + Arg.SizeInBytes);
      });
  const unsigned ConstantBufferLengthInGRF =
      iSTD::Align(MaxArgEnd - ConstantPayloadStart, Info.GRFSizeInBytes) /
      Info.GRFSizeInBytes;
  Kernel.m_kernelInfo.m_kernelProgram.ConstantBufferLength =
      ConstantBufferLengthInGRF;
}

static void setExecutionInfo(const vc::ocl::KernelInfo &BackendInfo,
                             const FINALIZER_INFO &JitterInfo,
                             CMKernel &Kernel) {
  Kernel.m_GRFSizeInBytes = BackendInfo.GRFSizeInBytes;
  Kernel.m_kernelInfo.m_kernelName = BackendInfo.Name;

  iOpenCL::ExecutionEnivronment &ExecEnv =
      Kernel.m_kernelInfo.m_executionEnivronment;
  // Fixed SIMD1.
  ExecEnv.CompiledSIMDSize = 1;
  // SLM size in bytes, align to 1KB.
  ExecEnv.SumFixedTGSMSizes = iSTD::Align(BackendInfo.SLMSize, 1024);
  ExecEnv.HasBarriers = BackendInfo.HasBarriers;
  ExecEnv.HasReadWriteImages = BackendInfo.HasReadWriteImages;
  ExecEnv.SubgroupIndependentForwardProgressRequired = true;
  ExecEnv.NumGRFRequired = JitterInfo.numGRFTotal;

  // Allocate spill-fill buffer
  if (JitterInfo.isSpill || JitterInfo.hasStackcalls)
    ExecEnv.PerThreadScratchSpace += JitterInfo.spillMemUsed;
  if (!JitterInfo.hasStackcalls && BackendInfo.ThreadPrivateMemSize)
    // CM stack calls and thread-private memory use the same value to control
    // scratch space. Consequently, if we have stack calls, there is no need
    // to add this value for thread-private memory. It should be fixed if
    // these features begin to calculate the required space separately.
    ExecEnv.PerThreadScratchSpace += BackendInfo.ThreadPrivateMemSize;

  // ThreadPayload.
  {
    iOpenCL::ThreadPayload &Payload = Kernel.m_kernelInfo.m_threadPayload;
    // Local IDs are always present now.
    Payload.HasLocalIDx = true;
    Payload.HasLocalIDy = true;
    Payload.HasLocalIDz = true;
    Payload.HasGroupID = BackendInfo.HasGroupID;
    Payload.HasLocalID =
        Payload.HasLocalIDx || Payload.HasLocalIDy || Payload.HasLocalIDz;
    Payload.CompiledForIndirectPayloadStorage = true;
    Payload.OffsetToSkipPerThreadDataLoad =
        JitterInfo.offsetToSkipPerThreadDataLoad;
  }

  // Iterate kernel arguments (This should stay in sync with cmc on resource
  // type.)
  auto isResource = [](vc::ocl::ArgKind Kind) {
    using ArgKind = vc::ocl::ArgKind;
    switch (Kind) {
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

  int NumUAVs = 0;
  int NumResources = 0;
  // cmc does not do stateless-to-stateful optimization, therefore
  // set >4GB to true by default, to false if we see any resource-type.
  ExecEnv.CompiledForGreaterThan4GBBuffers = true;
  for (const vc::ocl::ArgInfo &Arg : BackendInfo.Args) {
    if (isResource(Arg.Kind)) {
      if (Arg.Kind == vc::ocl::ArgKind::Buffer ||
          Arg.Kind == vc::ocl::ArgKind::SVM)
        NumUAVs++;
      else if (Arg.AccessKind == vc::ocl::ArgAccessKind::WriteOnly ||
               Arg.AccessKind == vc::ocl::ArgAccessKind::ReadWrite)
        NumUAVs++;
      else
        NumResources++;
      ExecEnv.CompiledForGreaterThan4GBBuffers = false;
    }
  }
  // Update BTI layout.
  Kernel.RecomputeBTLayout(NumUAVs, NumResources);
}

static void setGenBinary(const FINALIZER_INFO &JitterInfo,
                         const std::vector<char> &GenBinary, CMKernel &Kernel) {
  // Kernel binary, padding is hard-coded.
  const size_t Size = GenBinary.size();
  const size_t Padding = iSTD::GetAlignmentOffset(Size, 64);
  void *KernelBin = IGC::aligned_malloc(Size + Padding, 16);
  memcpy_s(KernelBin, Size + Padding, GenBinary.data(), Size);
  // Pad out the rest with 0s.
  std::memset(static_cast<char *>(KernelBin) + Size, 0, Padding);

  // Update program info.
  IGC::SProgramOutput &PO = Kernel.getProgramOutput();
  PO.m_programBin = KernelBin;
  PO.m_programSize = Size + Padding;
  PO.m_unpaddedProgramSize = Size;
  PO.m_InstructionCount = JitterInfo.numAsmCount;
}

static void setDebugInfo(const std::vector<char> &DebugInfo, CMKernel &Kernel) {
  if (DebugInfo.empty())
    return;

  const size_t DebugInfoSize = DebugInfo.size();
  void *DebugInfoBuf = IGC::aligned_malloc(DebugInfoSize, sizeof(void *));
  memcpy_s(DebugInfoBuf, DebugInfoSize, DebugInfo.data(), DebugInfoSize);
  Kernel.getProgramOutput().m_debugDataVISA = DebugInfoBuf;
  Kernel.getProgramOutput().m_debugDataVISASize = DebugInfoSize;
}

static void setGtpinInfo(const FINALIZER_INFO &JitterInfo,
                         const vc::ocl::GTPinInfo &GtpinInfo,
                         CMKernel &Kernel) {
  Kernel.getProgramOutput().m_scratchSpaceUsedByGtpin =
      JitterInfo.numBytesScratchGtpin;
  Kernel.m_kernelInfo.m_executionEnivronment.PerThreadScratchSpace +=
      JitterInfo.numBytesScratchGtpin;

  if (!GtpinInfo.GTPinBuffer.empty()) {
    const size_t BufSize = GtpinInfo.GTPinBuffer.size();
    void *GtpinBuffer = IGC::aligned_malloc(BufSize, 16);
    memcpy_s(GtpinBuffer, BufSize, GtpinInfo.GTPinBuffer.data(), BufSize);
    Kernel.getProgramOutput().m_gtpinBufferSize = BufSize;
    Kernel.getProgramOutput().m_gtpinBuffer = GtpinBuffer;
  }
}

// Transform backend collected into encoder format (OCL patchtokens or L0
// structures).
static void fillKernelInfo(const vc::ocl::CompileInfo &CompKernel,
                           CMKernel &ResKernel) {
  setExecutionInfo(CompKernel.KernelInfo, CompKernel.JitInfo, ResKernel);
  setArgumentsInfo(CompKernel.KernelInfo, ResKernel);
  setSymbolsInfo(CompKernel.KernelInfo, ResKernel.getProgramOutput());

  setGenBinary(CompKernel.JitInfo, CompKernel.GenBinary, ResKernel);
  setDebugInfo(CompKernel.DebugInfo, ResKernel);
  setGtpinInfo(CompKernel.JitInfo, CompKernel.GtpinInfo, ResKernel);
}

template <typename AnnotationT>
std::unique_ptr<AnnotationT>
getDataAnnotation(const vc::ocl::DataInfoT &DataInfo) {
  auto AllocSize = DataInfo.Buffer.size() + DataInfo.AdditionalZeroedSpace;
  IGC_ASSERT_MESSAGE(AllocSize >= 0, "illegal allocation size");
  if (AllocSize == 0)
    return nullptr;
  auto InitConstant = std::make_unique<AnnotationT>();
  InitConstant->Alignment = DataInfo.Alignment;
  InitConstant->AllocSize = AllocSize;

  auto BufferSize = DataInfo.Buffer.size();
  InitConstant->InlineData.resize(BufferSize);
  memcpy_s(InitConstant->InlineData.data(), BufferSize, DataInfo.Buffer.data(),
           BufferSize);

  return std::move(InitConstant);
}

static void fillOCLProgramInfo(IGC::SOpenCLProgramInfo &ProgramInfo,
                               const vc::ocl::ModuleInfoT &ModuleInfo) {
  auto ConstantAnnotation = getDataAnnotation<iOpenCL::InitConstantAnnotation>(
      ModuleInfo.ConstantData);
  if (ConstantAnnotation)
    ProgramInfo.m_initConstantAnnotation.push_back(
        std::move(ConstantAnnotation));
  auto GlobalAnnotation =
      getDataAnnotation<iOpenCL::InitGlobalAnnotation>(ModuleInfo.GlobalData);
  if (GlobalAnnotation)
    ProgramInfo.m_initGlobalAnnotation.push_back(std::move(GlobalAnnotation));
};

void vc::createBinary(iOpenCL::CGen8CMProgram &CMProgram,
                      const vc::ocl::CompileOutput &CompileInfos) {
  bool ProgramIsDebuggable = false;
  fillOCLProgramInfo(*CMProgram.m_programInfo, CompileInfos.ModuleInfo);
  for (const vc::ocl::CompileInfo &Info : CompileInfos.Kernels) {
    CMKernel *K = new CMKernel(CMProgram.getPlatform());
    CMProgram.m_kernels.push_back(K);
    fillKernelInfo(Info, *K);
    ProgramIsDebuggable |= !Info.DebugInfo.empty();
  }
  CMProgram.m_ContextProvider.updateDebuggableStatus(ProgramIsDebuggable);
}
