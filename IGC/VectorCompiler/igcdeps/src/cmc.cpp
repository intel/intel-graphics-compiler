/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include "AdaptorOCL/OCL/sp/spp_g8.h"
#include "RT_Jitter_Interface.h"
#include "common/secure_mem.h"
#include "common/secure_string.h"
#include "inc/common/igfxfmid.h"
#include "vc/igcdeps/cmc.h"

#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <numeric>
#include <string>

#include "Probe/Assertion.h"

using namespace vc;
using namespace llvm;

CMKernel::CMKernel(const PLATFORM &platform)
    : m_platform(platform), m_btiLayout(new USC::SShaderStageBTLayout) {
  std::memset(m_btiLayout.getModifiableLayout(), 0,
              sizeof(USC::SShaderStageBTLayout));
}

CMKernel::~CMKernel() {
  // TODO: refactor memory managment.
  m_kernelInfo.m_kernelProgram.simd1.Destroy();
  delete m_btiLayout.getModifiableLayout();
}

static zebin::PreDefinedAttrGetter::ArgType
getZEArgType(iOpenCL::DATA_PARAMETER_TOKEN type) {
  switch (type) {
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

static zebin::PreDefinedAttrGetter::ArgAccessType getZEArgAccessType(
    GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType accessKind) {
  using ArgAccessKind = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;
  switch (accessKind) {
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

void CMKernel::createConstArgumentAnnotation(unsigned argNo,
                                             unsigned sizeInBytes,
                                             unsigned payloadPosition,
                                             unsigned offsetInArg) {
  zebin::ZEInfoBuilder::addPayloadArgumentByValue(m_kernelInfo.m_zePayloadArgs,
                                                  payloadPosition, sizeInBytes,
                                                  argNo, offsetInArg, false);
}

void CMKernel::createSamplerAnnotation(const KernelArgInfo &ArgInfo,
                                       unsigned Offset) {
  using namespace zebin;
  using namespace iOpenCL;

  const auto SamplerType = SAMPLER_OBJECT_TEXTURE;

  const auto Index = ArgInfo.getIndex();
  const auto Kind = ArgInfo.getKind();
  const auto Access = ArgInfo.getAccessKind();
  const auto AddrMode = ArgInfo.getAddressMode();
  auto BTI = ArgInfo.getBTI();
  auto Size = ArgInfo.getSizeInBytes();

  IGC_ASSERT(AddrMode == ArgAddressMode::Stateful ||
             AddrMode == ArgAddressMode::Bindless);

  bool IsBindless = AddrMode == ArgAddressMode::Bindless;

  // For stateful image, we don't need any data to be passed thru the kernel
  // argument buffer.
  if (!IsBindless) {
    Size = 0;
    Offset = 0;
  } else {
    // BTI should be -1 in order to be omitted
    // however LevelZero runtime still requires sampler index to be set
    // so this is a incomplete workaround as it has to be incremented
    // if more than one sampler index exists
    BTI = 0;
  }

  const auto ZeAddrMode = IsBindless
                              ? PreDefinedAttrGetter::ArgAddrMode::bindless
                              : PreDefinedAttrGetter::ArgAddrMode::stateful;
  const auto ZeAccessType = PreDefinedAttrGetter::ArgAccessType::readwrite;

  ZEInfoBuilder::addPayloadArgumentSampler(
      m_kernelInfo.m_zePayloadArgs, Offset, Size, Index, BTI, ZeAddrMode,
      ZeAccessType, getZESamplerType(SamplerType));
}

static iOpenCL::IMAGE_MEMORY_OBJECT_TYPE
getOCLImageType(llvm::GenXOCLRuntimeInfo::KernelArgInfo::KindType Kind) {
  using KindType = llvm::GenXOCLRuntimeInfo::KernelArgInfo::KindType;
  switch (Kind) {
  case KindType::Image1D:
    return iOpenCL::IMAGE_MEMORY_OBJECT_1D;
  case KindType::Image1DArray:
    return iOpenCL::IMAGE_MEMORY_OBJECT_1D_ARRAY;
  case KindType::Image2D:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D;
  case KindType::Image2DArray:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY;
  case KindType::Image2DMediaBlock:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK;
  case KindType::Image3D:
    return iOpenCL::IMAGE_MEMORY_OBJECT_3D;
  default:
    IGC_ASSERT_MESSAGE(0, "Unexpected image kind");
    return iOpenCL::IMAGE_MEMORY_OBJECT_INVALID;
  }
}

static inline bool checkStateful(unsigned int BTI) {
  static constexpr unsigned int STATELESS_NONCOHERENT_BTI = 253;
  static constexpr unsigned int STATELESS_BTI = 255;
  // BTI > 255 - will be mapped to entry 0, it is still stateful access
  return (BTI < STATELESS_NONCOHERENT_BTI || BTI > STATELESS_BTI);
}

void CMKernel::createImageAnnotation(const KernelArgInfo &ArgInfo,
                                     unsigned Offset) {
  using namespace zebin;
  using namespace iOpenCL;

  const auto Index = ArgInfo.getIndex();
  const auto Kind = ArgInfo.getKind();
  const auto Access = ArgInfo.getAccessKind();
  const auto AddrMode = ArgInfo.getAddressMode();
  const auto BTI = ArgInfo.getBTI();
  auto Size = ArgInfo.getSizeInBytes();

  IGC_ASSERT(AddrMode == ArgAddressMode::Stateful ||
             AddrMode == ArgAddressMode::Bindless);

  bool IsBindless = AddrMode == ArgAddressMode::Bindless;

  // For stateful image, we don't need any data to be passed thru the kernel
  // argument buffer.
  if (!IsBindless) {
    Size = 0;
    Offset = 0;
  }

  auto ImageType = getOCLImageType(Kind);

  const auto ZeAddrMode = IsBindless
                              ? PreDefinedAttrGetter::ArgAddrMode::bindless
                              : PreDefinedAttrGetter::ArgAddrMode::stateful;

  ZEInfoBuilder::addPayloadArgumentImage(
      m_kernelInfo.m_zePayloadArgs, Offset, Size, Index, ZeAddrMode,
      getZEArgAccessType(Access), getZEImageType(ImageType));

  if (!IsBindless)
    ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, BTI, Index);
}

void CMKernel::createImplicitArgumentsAnnotation(unsigned payloadPosition) {
  createSizeAnnotation(payloadPosition,
                       iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET);
  payloadPosition += 3 * iOpenCL::DATA_PARAMETER_DATA_SIZE;
  createSizeAnnotation(payloadPosition,
                       iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE);
}

void CMKernel::createPointerGlobalAnnotation(const KernelArgInfo &ArgInfo,
                                             unsigned Offset) {
  using namespace zebin;
  using namespace iOpenCL;

  const auto Index = ArgInfo.getIndex();
  const auto Kind = ArgInfo.getKind();
  const auto Access = ArgInfo.getAccessKind();
  const auto AddrMode = ArgInfo.getAddressMode();
  const auto BTI = ArgInfo.getBTI();
  const auto Size = ArgInfo.getSizeInBytes();
  const auto SourceOffset = ArgInfo.getOffsetInArg();
  const bool IsLinearization = SourceOffset != 0 || ArgInfo.getArgNo() != Index;

  PreDefinedAttrGetter::ArgAddrMode ZeAddrMode;
  if (AddrMode == ArgAddressMode::Bindless) {
    IGC_ASSERT(!IsLinearization);
    ZeAddrMode = PreDefinedAttrGetter::ArgAddrMode::bindless;
  } else if (AddrMode == ArgAddressMode::Stateful) {
    IGC_ASSERT(!IsLinearization);
    ZeAddrMode = PreDefinedAttrGetter::ArgAddrMode::stateful;
  } else {
    IGC_ASSERT(AddrMode == ArgAddressMode::Stateless);
    ZeAddrMode = PreDefinedAttrGetter::ArgAddrMode::stateless;
  }

  if (IsLinearization) { // Pass the argument as by_value with is_ptr = true
    IGC_ASSERT(AddrMode == ArgAddressMode::Stateless);
    ZEInfoBuilder::addPayloadArgumentByValue(
        m_kernelInfo.m_zePayloadArgs, Offset, Size, Index, SourceOffset, true);
  } else {
    ZEInfoBuilder::addPayloadArgumentByPointer(
        m_kernelInfo.m_zePayloadArgs, Offset, Size, Index, ZeAddrMode,
        PreDefinedAttrGetter::ArgAddrSpace::global, getZEArgAccessType(Access));
  }

  if (AddrMode == ArgAddressMode::Stateful)
    ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, BTI, Index);
}

void CMKernel::createPointerLocalAnnotation(unsigned index, unsigned offset,
                                            unsigned sizeInBytes,
                                            unsigned alignment) {
  zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
      m_kernelInfo.m_zePayloadArgs, offset, sizeInBytes, index,
      zebin::PreDefinedAttrGetter::ArgAddrMode::slm,
      zebin::PreDefinedAttrGetter::ArgAddrSpace::local,
      zebin::PreDefinedAttrGetter::ArgAccessType::readwrite, alignment);
}

void CMKernel::createPrivateBaseAnnotation(unsigned argNo, unsigned byteSize,
                                           unsigned payloadPosition, int BTI,
                                           unsigned statelessPrivateMemSize,
                                           bool isStateful) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs,
      zebin::PreDefinedAttrGetter::ArgType::private_base_stateless,
      payloadPosition, byteSize);
}

void CMKernel::createBufferStatefulAnnotation(unsigned argNo,
                                              ArgAccessKind accessKind) {
  zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
      m_kernelInfo.m_zePayloadArgs, 0, 0, argNo,
      zebin::PreDefinedAttrGetter::ArgAddrMode::stateful,
      zebin::PreDefinedAttrGetter::ArgAddrSpace::global,
      getZEArgAccessType(accessKind));
}

void CMKernel::createSizeAnnotation(unsigned initPayloadPosition,
                                    iOpenCL::DATA_PARAMETER_TOKEN Type) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs, getZEArgType(Type), initPayloadPosition,
      iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
}

void CMKernel::createAssertBufferArgAnnotation(unsigned Index, unsigned BTI,
                                               unsigned Size,
                                               unsigned ArgOffset) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs,
      zebin::PreDefinedAttrGetter::ArgType::assert_buffer, ArgOffset, Size);
}

void CMKernel::createPrintfBufferArgAnnotation(unsigned Index, unsigned BTI,
                                               unsigned Size,
                                               unsigned ArgOffset) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs,
      zebin::PreDefinedAttrGetter::ArgType::printf_buffer, ArgOffset, Size);
}

void CMKernel::createSyncBufferArgAnnotation(unsigned Index, unsigned BTI,
                                             unsigned Size,
                                             unsigned ArgOffset) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs,
      zebin::PreDefinedAttrGetter::ArgType::sync_buffer, ArgOffset, Size);
}

void CMKernel::createImplArgsBufferAnnotation(unsigned Size,
                                              unsigned ArgOffset) {
  zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
      m_kernelInfo.m_zePayloadArgs,
      zebin::PreDefinedAttrGetter::ArgType::implicit_arg_buffer, ArgOffset,
      Size);
}

// TODO: refactor this function with the OCL part.
void CMKernel::RecomputeBTLayout(int numUAVs, int numResources) {
  USC::SShaderStageBTLayout *layout = m_btiLayout.getModifiableLayout();

  // The BT layout contains the minimum and the maximum number BTI for each kind
  // of resource. E.g. UAVs may be mapped to BTIs 0..3, SRVs to 4..5, and the
  // scratch surface to 6. Note that the names are somewhat misleading. They are
  // used for the sake of consistency with the ICBE sources.

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

  // Now, ConstantBuffers - used as a placeholder for the inline constants, if
  // present.
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

static void setFuncSectionInfo(const GenXOCLRuntimeInfo::KernelInfo &Info,
                               IGC::SProgramOutput &KernelProgram) {
  KernelProgram.m_relocs = Info.Func.Relocations;

  // Validate function symbols
  auto IsKernel = [](const vISA::ZESymEntry &Entry) {
    return Entry.s_type == vISA::GenSymType::S_KERNEL;
  };
  IGC_ASSERT_MESSAGE(std::count_if(Info.Func.Symbols.begin(),
                                   Info.Func.Symbols.end(), IsKernel) < 2u,
                     "There can be only one or no kernel symbols");
  IGC_ASSERT_MESSAGE(std::is_partitioned(Info.Func.Symbols.begin(),
                                         Info.Func.Symbols.end(), IsKernel),
                     "Kernel symbols should be partitioned");

  // Points to the first function symbol and also one past last kernel symbol.
  const auto FirstFuncIt =
      std::partition_point(Info.Func.Symbols.begin(), Info.Func.Symbols.end(),
                           [](const vISA::ZESymEntry &Entry) {
                             return Entry.s_type == vISA::GenSymType::S_KERNEL;
                           });

  KernelProgram.m_symbols.function =
      IGC::SProgramOutput::SymbolListTy{FirstFuncIt, Info.Func.Symbols.end()};
  KernelProgram.m_symbols.local =
      IGC::SProgramOutput::SymbolListTy{Info.Func.Symbols.begin(), FirstFuncIt};
}

static void setArgumentsInfo(const GenXOCLRuntimeInfo::KernelInfo &Info,
                             CMKernel &Kernel) {
  // This is the starting constant thread payload
  // r0-r1 are reserved for SIMD1 dispatch
  const unsigned ConstantPayloadStart = Info.getGRFSizeInBytes() * 2;

  for (const GenXOCLRuntimeInfo::KernelArgInfo &Arg : Info.args()) {
    IGC_ASSERT_MESSAGE(Arg.getOffset() >= ConstantPayloadStart,
                       "Argument overlaps with thread payload");
    const unsigned ArgOffset = Arg.getOffset() - ConstantPayloadStart;

    using ArgKind = GenXOCLRuntimeInfo::KernelArgInfo::KindType;
    switch (Arg.getKind()) {
    default:
      break;
    case ArgKind::General:
      Kernel.createConstArgumentAnnotation(Arg.getIndex(), Arg.getSizeInBytes(),
                                           ArgOffset, Arg.getOffsetInArg());
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
      Kernel.createPointerGlobalAnnotation(Arg, ArgOffset);
      if (checkStateful(Arg.getBTI()))
        Kernel.createBufferStatefulAnnotation(Arg.getIndex(),
                                              Arg.getAccessKind());
      break;
    case ArgKind::SVM:
      Kernel.createPointerGlobalAnnotation(Arg, ArgOffset);
      break;
    case ArgKind::SLM:
      Kernel.createPointerLocalAnnotation(
          Arg.getIndex(), ArgOffset, Arg.getSizeInBytes(), Arg.getAlignment());
      break;
    case ArgKind::Sampler:
      Kernel.createSamplerAnnotation(Arg, ArgOffset);
      break;
    case ArgKind::Image1D:
    case ArgKind::Image1DArray:
    case ArgKind::Image2D:
    case ArgKind::Image2DArray:
    case ArgKind::Image2DMediaBlock:
    case ArgKind::Image3D:
      Kernel.createImageAnnotation(Arg, ArgOffset);
      break;
    case ArgKind::AssertBuffer:
      Kernel.createAssertBufferArgAnnotation(Arg.getIndex(), Arg.getBTI(),
                                             Arg.getSizeInBytes(), ArgOffset);
      break;
    case ArgKind::PrintBuffer:
      Kernel.createPrintfBufferArgAnnotation(Arg.getIndex(), Arg.getBTI(),
                                             Arg.getSizeInBytes(), ArgOffset);
      break;
    case ArgKind::SyncBuffer:
      Kernel.createSyncBufferArgAnnotation(Arg.getIndex(), Arg.getBTI(),
                                           Arg.getSizeInBytes(), ArgOffset);
      break;
    case ArgKind::PrivateBase: {
      auto PrivMemSize = Info.getStatelessPrivMemSize();
      Kernel.createPrivateBaseAnnotation(Arg.getIndex(), Arg.getSizeInBytes(),
                                         ArgOffset, Arg.getBTI(), PrivMemSize,
                                         checkStateful(Arg.getBTI()));
      Kernel.m_kernelInfo.m_executionEnvironment
          .PerThreadPrivateOnStatelessSize = PrivMemSize;
    } break;
    case ArgKind::ByValSVM:
      // Do nothing because it has already been linearized and implicit args
      // will be set instead of it.
      break;
    case ArgKind::ImplicitArgsBuffer:
      Kernel.createImplArgsBufferAnnotation(Arg.getSizeInBytes(), ArgOffset);
      break;
    }
  }

  const unsigned MaxArgEnd = std::accumulate(
      Info.arg_begin(), Info.arg_end(), ConstantPayloadStart,
      [](unsigned MaxArgEnd, const GenXOCLRuntimeInfo::KernelArgInfo &Arg) {
        return std::max(MaxArgEnd, Arg.getOffset() + Arg.getSizeInBytes());
      });
  const unsigned ConstantBufferLengthInGRF =
      iSTD::Align(MaxArgEnd - ConstantPayloadStart, Info.getGRFSizeInBytes()) /
      Info.getGRFSizeInBytes();
  Kernel.m_kernelInfo.m_kernelProgram.ConstantBufferLength =
      ConstantBufferLengthInGRF;
}

// a helper function to get the conservative vISA stack size when having
// stack call
static uint32_t getConservativeVISAStackSize(const PLATFORM &platform) {
  auto isPVCXT = [](const PLATFORM &platform) {
    return platform.eProductFamily == IGFX_PVC &&
           platform.usRevId >= REVISION_B;
  };
  // PVC-XT needs a smaller default size to avoid stack OOM
  // Ref: IGC::CEncoder::getSpillMemSizeWithFG
  if (isPVCXT(platform))
    return 64 * 1024;
  return 128 * 1024;
}

static void setExecutionInfo(const GenXOCLRuntimeInfo::KernelInfo &BackendInfo,
                             const vISA::FINALIZER_INFO &JitterInfo,
                             CMKernel &Kernel) {
  Kernel.m_GRFSizeInBytes = BackendInfo.getGRFSizeInBytes();
  Kernel.m_kernelInfo.m_kernelName = BackendInfo.getName();

  iOpenCL::ExecutionEnvironment &ExecEnv =
      Kernel.m_kernelInfo.m_executionEnvironment;
  ExecEnv.CompiledSIMDSize = BackendInfo.getSIMDSize();
  // SLM size in bytes, align to 1KB.
  ExecEnv.SumFixedTGSMSizes = iSTD::Align(BackendInfo.getSLMSize(), 1024);
  ExecEnv.HasStackCalls = JitterInfo.hasStackcalls;
  // HasBarriers isn't bool and preserves number of barriers for PVC+ targets
  // dispite misleading naming.
  ExecEnv.HasBarriers = BackendInfo.getNumBarriers();
  ExecEnv.HasSample = BackendInfo.usesSample();
  ExecEnv.HasDPAS = BackendInfo.usesDPAS();
  ExecEnv.DisableMidThreadPreemption = BackendInfo.disableMidThreadPreemption();
  ExecEnv.numThreads = BackendInfo.getNumThreads();
  ExecEnv.HasReadWriteImages = BackendInfo.usesReadWriteImages();
  ExecEnv.SubgroupIndependentForwardProgressRequired = true;
  ExecEnv.NumGRFRequired = JitterInfo.stats.numGRFTotal;
  ExecEnv.RequireDisableEUFusion = BackendInfo.requireDisableEUFusion();
  ExecEnv.IndirectStatelessCount = BackendInfo.getIndirectCount();
  ExecEnv.HasLscStoresWithNonDefaultL1CacheControls =
      BackendInfo.hasLscStoresWithNonDefaultL1CacheControls();


  // Allocate spill-fill buffer
  if (JitterInfo.hasStackcalls) {
    ExecEnv.PerThreadScratchSpace +=
        getConservativeVISAStackSize(Kernel.m_platform);
  } else {
    ExecEnv.PerThreadScratchSpace += JitterInfo.stats.spillMemUsed;
  }

  if (!JitterInfo.hasStackcalls && BackendInfo.getTPMSize() != 0)
    // CM stack calls and thread-private memory use the same value to control
    // scratch space. Consequently, if we have stack calls, there is no need
    // to add this value for thread-private memory. It should be fixed if
    // these features begin to calculate the required space separately.
    ExecEnv.PerThreadScratchSpace += BackendInfo.getTPMSize();

  // ThreadPayload.
  {
    iOpenCL::ThreadPayload &Payload = Kernel.m_kernelInfo.m_threadPayload;
    // Local IDs are always present now.
    Payload.HasLocalIDx = BackendInfo.usesLocalIdX();
    Payload.HasLocalIDy = BackendInfo.usesLocalIdY();
    Payload.HasLocalIDz = BackendInfo.usesLocalIdZ();
    Payload.HasGroupID = BackendInfo.usesGroupId();
    Payload.HasLocalID =
        Payload.HasLocalIDx || Payload.HasLocalIDy || Payload.HasLocalIDz;
    Payload.CompiledForIndirectPayloadStorage = true;
    Payload.OffsetToSkipPerThreadDataLoad =
        JitterInfo.offsetToSkipPerThreadDataLoad;
  }

  int NumUAVs = 0;
  int NumResources = 0;
  // cmc does not do stateless-to-stateful optimization, therefore
  // set >4GB to true by default, to false if we see any resource-type.
  ExecEnv.CompiledForGreaterThan4GBBuffers = true;
  for (const GenXOCLRuntimeInfo::KernelArgInfo &Arg : BackendInfo.args()) {
    if (Arg.isResource()) {
      if (!Arg.isImage() || Arg.isWritable())
        NumUAVs++;
      else
        NumResources++;
      ExecEnv.CompiledForGreaterThan4GBBuffers = false;
    }
  }
  // Update BTI layout.
  Kernel.RecomputeBTLayout(NumUAVs, NumResources);
}

static void setGenBinary(const vISA::FINALIZER_INFO &JitterInfo,
                         const std::vector<uint8_t> &GenBinary,
                         CMKernel &Kernel) {
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
  PO.m_InstructionCount = JitterInfo.stats.numAsmCountUnweighted;
}

static void setVISAAsm(
    const std::vector<GenXOCLRuntimeInfo::KernelInfo::NamedVISAAsm> &VISAAsm,
    CMKernel &Kernel) {
  Kernel.getProgramOutput().m_VISAAsm = VISAAsm;
}

static void setDebugInfo(const std::vector<char> &DebugInfo, CMKernel &Kernel) {
  if (DebugInfo.empty())
    return;

  const size_t DebugInfoSize = DebugInfo.size();
  void *DebugInfoBuf = IGC::aligned_malloc(DebugInfoSize, sizeof(void *));
  memcpy_s(DebugInfoBuf, DebugInfoSize, DebugInfo.data(), DebugInfoSize);
  Kernel.getProgramOutput().m_debugData = DebugInfoBuf;
  Kernel.getProgramOutput().m_debugDataSize = DebugInfoSize;
}

static void setGtpinInfo(const vISA::FINALIZER_INFO &JitterInfo,
                         const GenXOCLRuntimeInfo::GTPinInfo &GtpinInfo,
                         CMKernel &Kernel) {
  Kernel.getProgramOutput().m_scratchSpaceUsedByGtpin =
      JitterInfo.numBytesScratchGtpin;
  Kernel.m_kernelInfo.m_executionEnvironment.PerThreadScratchSpace +=
      JitterInfo.numBytesScratchGtpin;

  if (!GtpinInfo.empty()) {
    const size_t BufSize = GtpinInfo.size();
    void *GtpinBuffer = IGC::aligned_malloc(BufSize, 16);
    memcpy_s(GtpinBuffer, BufSize, GtpinInfo.data(), BufSize);
    Kernel.getProgramOutput().m_gtpinBufferSize = BufSize;
    Kernel.getProgramOutput().m_gtpinBuffer = GtpinBuffer;
  }
}

static void setCostInfo(const GenXOCLRuntimeInfo::CostInfoT &CostInfo,
                        CMKernel &Kernel) {
  for (auto &Sym : CostInfo.Symbols) {
    auto &ZeSym = Kernel.m_kernelCostExpInfo.argsSym.emplace_back();
    ZeSym.argNo = Sym.ArgNo;
    ZeSym.byteOffset = Sym.ByteOffset;
    ZeSym.sizeInBytes = Sym.SizeInBytes;
    ZeSym.isInDirect = Sym.IsIndirect;
  }
  for (auto &Expr : CostInfo.Expressions) {
    auto &ZeExpr = Kernel.m_kernelCostExpInfo.loopLCE.emplace_back();
    ZeExpr.factor = Expr.Factor;
    ZeExpr.argsym_index = Expr.ArgSymIdx;
    ZeExpr.C = Expr.C;
  }
  for (auto &Cost : CostInfo.Costs) {
    auto &ZeCost = Kernel.m_kernelCostExpInfo.kernelCost.emplace_back();
    ZeCost.cycle = Cost.Cycle;
    ZeCost.bytes_loaded = Cost.BytesLoaded;
    ZeCost.bytes_stored = Cost.BytesStored;
    ZeCost.num_loops = Cost.NumLoops;
  }
}

// Transform backend collected into encoder format.
static void fillKernelInfo(const GenXOCLRuntimeInfo::CompiledKernel &CompKernel,
                           CMKernel &ResKernel) {
  setExecutionInfo(CompKernel.getKernelInfo(), CompKernel.getJitterInfo(),
                   ResKernel);
  setArgumentsInfo(CompKernel.getKernelInfo(), ResKernel);
  setFuncSectionInfo(CompKernel.getKernelInfo(), ResKernel.getProgramOutput());

  setGenBinary(CompKernel.getJitterInfo(), CompKernel.getGenBinary(),
               ResKernel);
  setVISAAsm(CompKernel.getKernelInfo().VISAAsm, ResKernel);
  setDebugInfo(CompKernel.getDebugInfo(), ResKernel);
  setGtpinInfo(CompKernel.getJitterInfo(), CompKernel.getGTPinInfo(),
               ResKernel);
  setCostInfo(CompKernel.getCostInfo(), ResKernel);
}

template <typename AnnotationT>
std::unique_ptr<AnnotationT>
getDataAnnotation(const GenXOCLRuntimeInfo::DataInfo &Data) {
  auto AllocSize = Data.Buffer.size() + Data.AdditionalZeroedSpace;
  IGC_ASSERT_MESSAGE(AllocSize >= 0, "illegal allocation size");
  if (AllocSize == 0)
    return nullptr;
  auto InitConstant = std::make_unique<AnnotationT>();
  InitConstant->Alignment = Data.Alignment;
  InitConstant->AllocSize = AllocSize;

  auto BufferSize = Data.Buffer.size();
  InitConstant->InlineData.resize(BufferSize);
  memcpy_s(InitConstant->InlineData.data(), BufferSize, Data.Buffer.data(),
           BufferSize);

  return std::move(InitConstant);
}

static void
fillOCLProgramInfo(IGC::SOpenCLProgramInfo &ProgramInfo,
                   const GenXOCLRuntimeInfo::ModuleInfoT &ModuleInfo,
                   bool HasCrossThreadOffsetRelocations,
                   bool HasPerThreadOffsetRelocations) {
  auto ConstantAnnotation = getDataAnnotation<iOpenCL::InitConstantAnnotation>(
      ModuleInfo.Constant.Data);
  if (ConstantAnnotation)
    ProgramInfo.m_initConstantAnnotation = std::move(ConstantAnnotation);
  auto GlobalAnnotation =
      getDataAnnotation<iOpenCL::InitGlobalAnnotation>(ModuleInfo.Global.Data);
  if (GlobalAnnotation)
    ProgramInfo.m_initGlobalAnnotation = std::move(GlobalAnnotation);
  auto ConstStringAnnotation =
      getDataAnnotation<iOpenCL::InitConstantAnnotation>(
          ModuleInfo.ConstString.Data);
  if (ConstStringAnnotation)
    ProgramInfo.m_initConstantStringAnnotation =
        std::move(ConstStringAnnotation);

  // Symbols.
  ProgramInfo.m_zebinSymbolTable.global = ModuleInfo.Global.Symbols;
  ProgramInfo.m_zebinSymbolTable.globalConst = ModuleInfo.Constant.Symbols;
  ProgramInfo.m_zebinSymbolTable.globalStringConst =
      ModuleInfo.ConstString.Symbols;

  // Relocations.
  ProgramInfo.m_GlobalPointerAddressRelocAnnotation.globalReloc =
      ModuleInfo.Global.Relocations;
  ProgramInfo.m_GlobalPointerAddressRelocAnnotation.globalConstReloc =
      ModuleInfo.Constant.Relocations;
  IGC_ASSERT_MESSAGE(
      ModuleInfo.ConstString.Relocations.empty(),
      "relocations inside constant string section are not supported");
  ProgramInfo.m_hasCrossThreadOffsetRelocations =
      HasCrossThreadOffsetRelocations;
  ProgramInfo.m_hasPerThreadOffsetRelocations = HasPerThreadOffsetRelocations;
};

void vc::createBinary(
    vc::CGen8CMProgram &CMProgram,
    const GenXOCLRuntimeInfo::CompiledModuleT &CompiledModule) {
  for (const GenXOCLRuntimeInfo::CompiledKernel &CompKernel :
       CompiledModule.Kernels) {
    auto K = std::make_unique<CMKernel>(CMProgram.getPlatform());
    fillKernelInfo(CompKernel, *K);
    CMProgram.m_kernels.push_back(std::move(K));
  }
  fillOCLProgramInfo(*CMProgram.m_programInfo, CompiledModule.ModuleInfo,
                     CMProgram.HasCrossThreadOffsetRelocations(),
                     CMProgram.HasPerThreadOffsetRelocations());
}
