/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include "ConstantEncoder.h"
#include "GenX.h"
#include "GenXModule.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "OCLRuntimeInfoPrinter.h"
#include "GenXGlobalUniform.h"

#include "vc/Utils/GenX/CostInfo.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include <visaBuilder_interface.h>

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/InitializePasses.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <numeric>
#include <stack>

#include "Probe/Assertion.h"

#define DEBUG_TYPE "genx-runtime-info"

#define CISA_CALL(c)                                                           \
  do {                                                                         \
    auto Result = (c);                                                         \
    (void)Result;                                                              \
    IGC_ASSERT_MESSAGE(Result == 0, "Call to VISA API failed: " #c);           \
  } while (0);

using namespace llvm;
using namespace vc;

char GenXOCLRuntimeInfo::ID = 0;

//===----------------------------------------------------------------------===//
//
// Kernel argument info implementation.
//
//===----------------------------------------------------------------------===//
namespace llvm {
class KernelArgBuilder final {
  using ArgKindType = GenXOCLRuntimeInfo::KernelArgInfo::KindType;
  using ArgAccessKindType = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;
  using ArgAddressModeType = GenXOCLRuntimeInfo::KernelArgInfo::AddressModeType;

  const vc::KernelMetadata &KM;
  const DataLayout &DL;
  const GenXSubtarget &ST;
  const GenXBackendConfig &BC;

public:
  KernelArgBuilder(const vc::KernelMetadata &KMIn, const DataLayout &DLIn,
                   const GenXSubtarget &STIn, const GenXBackendConfig &BCIn)
      : KM(KMIn), DL(DLIn), ST(STIn), BC(BCIn) {}

  GenXOCLRuntimeInfo::KernelArgInfo
  translateArgument(const Argument &Arg) const;

private:
  static auto getStrPred(const char *Attr) {
    return [Attr](StringRef Token) { return Token == Attr; };
  }

  ArgKindType getOCLArgKind(ArrayRef<StringRef> Tokens,
                            const Argument &Arg) const;
  ArgAccessKindType getOCLArgAccessKind(ArrayRef<StringRef> Tokens,
                                        ArgKindType Kind) const;
  ArgAddressModeType getOCLArgAddressMode(ArgKindType Kind,
                                          const Argument &Arg) const;

  std::tuple<ArgKindType, ArgAddressModeType, ArgAccessKindType>
  translateArgDesc(const Argument &Arg) const;

  unsigned getArgSizeInBytes(const Argument &Arg) const;
};
} // namespace llvm

KernelArgBuilder::ArgAccessKindType
KernelArgBuilder::getOCLArgAccessKind(ArrayRef<StringRef> Tokens,
                                      ArgKindType Kind) const {
  switch (Kind) {
  case ArgKindType::Buffer:
  case ArgKindType::Image1D:
  case ArgKindType::Image1DArray:
  case ArgKindType::Image2D:
  case ArgKindType::Image2DArray:
  case ArgKindType::Image2DMediaBlock:
  case ArgKindType::Image3D:
  case ArgKindType::SVM:
    if (any_of(Tokens, getStrPred(OCLAttributes::ReadOnly)))
      return ArgAccessKindType::ReadOnly;
    if (any_of(Tokens, getStrPred(OCLAttributes::WriteOnly)))
      return ArgAccessKindType::WriteOnly;
  case ArgKindType::SLM:
    return ArgAccessKindType::ReadWrite;
  default:
    return ArgAccessKindType::None;
  }
}

KernelArgBuilder::ArgAddressModeType
KernelArgBuilder::getOCLArgAddressMode(ArgKindType Kind,
                                       const Argument &Arg) const {
  unsigned ArgNo = Arg.getArgNo();

  switch (Kind) {
  default:
    break;
  case ArgKindType::SVM:
  case ArgKindType::AssertBuffer:
  case ArgKindType::PrintBuffer:
  case ArgKindType::SyncBuffer:
  case ArgKindType::PrivateBase:
    return ArgAddressModeType::Stateless;
  case ArgKindType::Sampler:
    if (KM.getArgCategory(ArgNo) == vc::RegCategory::Sampler)
      return ArgAddressModeType::Stateful;
    IGC_ASSERT_MESSAGE(KM.getArgCategory(ArgNo) == vc::RegCategory::General,
                       "Expected general category");
    return ArgAddressModeType::Bindless;
  case ArgKindType::Buffer:
  case ArgKindType::Image1D:
  case ArgKindType::Image1DArray:
  case ArgKindType::Image2D:
  case ArgKindType::Image2DArray:
  case ArgKindType::Image2DMediaBlock:
  case ArgKindType::Image3D:
    if (KM.getArgCategory(ArgNo) == vc::RegCategory::Surface)
      return ArgAddressModeType::Stateful;
    IGC_ASSERT_MESSAGE(KM.getArgCategory(ArgNo) == vc::RegCategory::General,
                       "Expected general category");
    return ArgAddressModeType::Bindless;
  }

  return ArgAddressModeType::None;
}

KernelArgBuilder::ArgKindType
KernelArgBuilder::getOCLArgKind(ArrayRef<StringRef> Tokens,
                                const Argument &Arg) const {
  unsigned ArgNo = Arg.getArgNo();
  unsigned RawKind = KM.getArgKind(ArgNo);
  const auto *ArgTy = Arg.getType();

  // Implicit arguments.
  if (vc::isLocalSizeKind(RawKind))
    return ArgKindType::LocalSize;
  if (vc::isGroupCountKind(RawKind))
    return ArgKindType::GroupCount;
  if (vc::isAssertBufferKind(RawKind))
    return ArgKindType::AssertBuffer;
  if (vc::isPrintBufferKind(RawKind))
    return ArgKindType::PrintBuffer;
  if (vc::isSyncBufferKind(RawKind))
    return ArgKindType::SyncBuffer;
  if (vc::isPrivateBaseKind(RawKind))
    return ArgKindType::PrivateBase;
  if (vc::isByValSVMKind(RawKind))
    return ArgKindType::ByValSVM;
  if (vc::isImplicitArgsBufferKind(RawKind))
    return ArgKindType::ImplicitArgsBuffer;

  // Explicit arguments.
  switch (auto Cat = KM.getArgCategory(ArgNo)) {
  default:
    break;
  case vc::RegCategory::General:
    if (any_of(Tokens, getStrPred(OCLAttributes::SVM)))
      return ArgKindType::SVM;
    if (ArgTy->isPointerTy() && vc::getAddrSpace(ArgTy) == vc::AddrSpace::Local)
      return ArgKindType::SLM;
    if (any_of(Tokens, getStrPred(OCLAttributes::Sampler)))
      return ArgKindType::Sampler;
    LLVM_FALLTHROUGH;
  case vc::RegCategory::Surface:
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1d)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1dArray)))
      return ArgKindType::Image1DArray;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1dBuffer)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2d))) {
      if (ST.noLegacyDataport() || BC.usePlain2DImages())
        return ArgKindType::Image2D;
      // Legacy behavior to treat all 2d images as media block.
      return ArgKindType::Image2DMediaBlock;
    }
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2dArray)))
      return ArgKindType::Image2DArray;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2dMediaBlock))) {
      if (ST.translateMediaBlockMessages())
        return ArgKindType::Image2D;
      return ArgKindType::Image2DMediaBlock;
    }
    if (any_of(Tokens, getStrPred(OCLAttributes::Image3d)))
      return ArgKindType::Image3D;
    if (Cat == vc::RegCategory::Surface ||
        any_of(Tokens, getStrPred(OCLAttributes::Buffer)))
      return ArgKindType::Buffer;
    break;
  case vc::RegCategory::Sampler:
    return ArgKindType::Sampler;
  }

  return ArgKindType::General;
}

// Retrieve Kind and AccessKind from given ArgTypeDesc in metadata.
std::tuple<KernelArgBuilder::ArgKindType, KernelArgBuilder::ArgAddressModeType,
           KernelArgBuilder::ArgAccessKindType>
KernelArgBuilder::translateArgDesc(const Argument &Arg) const {
  unsigned ArgNo = Arg.getArgNo();
  std::string Translated{KM.getArgTypeDesc(ArgNo)};
  // Transform each separator to space.
  std::transform(Translated.begin(), Translated.end(), Translated.begin(),
                 [](char C) {
                   if (C != '-' && C != '_' && C != '=' && !std::isalnum(C))
                     return ' ';
                   return C;
                 });

  // Split and delete duplicates.
  SmallVector<StringRef, 4> Tokens;
  StringRef(Translated)
      .split(Tokens, ' ', -1 /* MaxSplit */, false /* AllowEmpty */);
  std::sort(Tokens.begin(), Tokens.end());
  Tokens.erase(std::unique(Tokens.begin(), Tokens.end()), Tokens.end());

  const auto Kind = getOCLArgKind(Tokens, Arg);
  const auto AddressMode = getOCLArgAddressMode(Kind, Arg);
  const auto AccessKind = getOCLArgAccessKind(Tokens, Kind);
  return {Kind, AddressMode, AccessKind};
}

unsigned KernelArgBuilder::getArgSizeInBytes(const Argument &Arg) const {
  Type *ArgTy = Arg.getType();
  if (ArgTy->isPointerTy())
    return DL.getPointerTypeSize(ArgTy);
  if (KM.isBufferType(Arg.getArgNo()))
    return DL.getPointerSize();
  return DL.getTypeSizeInBits(ArgTy) / genx::ByteBits;
}

GenXOCLRuntimeInfo::KernelArgInfo
KernelArgBuilder::translateArgument(const Argument &Arg) const {
  GenXOCLRuntimeInfo::KernelArgInfo Info;
  unsigned ArgNo = Arg.getArgNo();

  std::tie(Info.Kind, Info.AddrMode, Info.AccessKind) = translateArgDesc(Arg);

  Info.Offset = KM.getArgOffset(ArgNo);
  Info.SizeInBytes = getArgSizeInBytes(Arg);
  Info.BTI = KM.getBTI(ArgNo);
  // For implicit arguments that are byval argument linearization, index !=
  // ArgNo in the IR function.
  Info.ArgNo = ArgNo;
  Info.Index = KM.getArgIndex(ArgNo);
  // Linearization arguments have a non-zero offset in the original explicit
  // byval arg.
  Info.OffsetInArg = KM.getOffsetInArg(ArgNo);
  if (Info.Kind == GenXOCLRuntimeInfo::KernelArgInfo::KindType::SLM)
    // Max SLM alignment is 16 or 8 bytes and depends on what memory
    // messages are used: legacy or LSC.
    Info.Alignment = ST.translateLegacyMessages() ? 8 : 16;
  else
    Info.Alignment = 0;

  return Info;
}

//===----------------------------------------------------------------------===//
//
// Function info implementation.
//
//===----------------------------------------------------------------------===//
GenXOCLRuntimeInfo::FunctionInfo::FunctionInfo(StringRef Name,
                                               const GenXSubtarget &ST)
    : Name(Name.str()), GRFSizeInBytes(ST.getGRFByteSize()) {}

GenXOCLRuntimeInfo::FunctionInfo::FunctionInfo(const FunctionGroup &FG,
                                               GenXOCLRuntimeInfo &RI,
                                               const GenXSubtarget &ST,
                                               const GenXBackendConfig &BC)
    : DisableEUFusion(BC.isDisableEUFusion()),
      SupportsDebugging(BC.emitDebuggableKernels()),
      GRFSizeInBytes(ST.getGRFByteSize()),
      StatelessPrivateMemSize(
          vc::getStackAmount(FG.getHead(), BC.getStatelessPrivateMemSize())) {
  initInstructionLevelProperties(FG, RI, ST, BC);

  auto *Func = FG.getHead();

  if (vc::isKernel(Func)) {
    vc::KernelMetadata KernelMD(FG.getHead());
    Name = KernelMD.getName().str();
    SLMSize = KernelMD.getSLMSize();
    IndirectCount = KernelMD.getIndirectCount();

    if (Func->getParent()->getNamedMetadata(
            FunctionMD::VCDisableMidThreadPreemption)) {
      DisableMidThreadPreemption = true;
    }

    if (ST.hasNBarrier())
      NumBarriers = KernelMD.getAlignedBarrierCnt(NumBarriers);
  } else {
    Name = Func->getName().str();
  }
}

void GenXOCLRuntimeInfo::FunctionInfo::initInstructionLevelProperties(
    Function *Func, GenXOCLRuntimeInfo &RI, const GenXSubtarget &ST,
    const GenXBackendConfig &BC) {
  LLVM_DEBUG(dbgs() << "> Function: " << Func->getName() << "\n");
  for (auto &Inst : instructions(*Func)) {
    auto IID = vc::getAnyIntrinsicID(&Inst);
    switch (IID) {
    default:
      break;
    case GenXIntrinsic::genx_group_id_x:
    case GenXIntrinsic::genx_group_id_y:
    case GenXIntrinsic::genx_group_id_z:
      LLVM_DEBUG(dbgs() << ">> UsesGroupId: true\n");
      UsesGroupId = true;
      break;
    case GenXIntrinsic::genx_barrier:
    case GenXIntrinsic::genx_sbarrier:
      NumBarriers = 1;
      LLVM_DEBUG(dbgs() << ">> NumBarriers: " << NumBarriers << "\n");
      break;
    case GenXIntrinsic::genx_sample_unorm:
    case vc::InternalIntrinsic::sample_bti:
    case vc::InternalIntrinsic::sample_predef_surface:
    case vc::InternalIntrinsic::sampler_load_bti:
    case vc::InternalIntrinsic::sampler_load_predef_surface:
      UsesSample = true;
      LLVM_DEBUG(dbgs() << ">> UsesSample: true\n");
      break;
    case GenXIntrinsic::genx_dpas2:
    case GenXIntrinsic::genx_dpas:
    case GenXIntrinsic::genx_dpas_nosrc0:
      if (!DisableEUFusion && ST.hasFusedEU()) {
        const auto &GUA = RI.getAnalysis<GenXGlobalUniformAnalysis>(*Func);
        DisableEUFusion = !GUA.isUniform(*Inst.getParent());
        LLVM_DEBUG(dbgs() << ">> DisableEUFusion: " << DisableEUFusion << "\n");
      }
      LLVM_FALLTHROUGH;
    case GenXIntrinsic::genx_dpasw:
    case GenXIntrinsic::genx_dpasw_nosrc0:
      LLVM_DEBUG(dbgs() << ">> UsesDPAS: true\n");
      UsesDPAS = true;
      break;
    }

    if (BC.reportLSCStoresWithNonDefaultL1CacheControls()) {
      // a store intrinsic
      if (auto *CI = dyn_cast<CallInst>(&Inst);
          CI && !CI->doesNotAccessMemory() && !CI->onlyReadsMemory()) {
        // a store intrinsic has cache opt
        if (auto CacheOptsIndex =
                vc::InternalIntrinsic::getMemoryCacheControlOperandIndex(IID);
            CacheOptsIndex >= 0) {
          auto *CacheOpts = cast<Constant>(Inst.getOperand(CacheOptsIndex));
          auto *L1Opt = cast<ConstantInt>(CacheOpts->getAggregateElement(0u));
          HasLscStoresWithNonDefaultL1CacheControls |=
              static_cast<LSC_CACHE_OPT>(L1Opt->getZExtValue()) !=
              LSC_CACHING_DEFAULT;
        }
      }
    }
  }
}

void GenXOCLRuntimeInfo::FunctionInfo::initInstructionLevelProperties(
    const FunctionGroup &FG, GenXOCLRuntimeInfo &RI, const GenXSubtarget &ST,
    const GenXBackendConfig &BC) {
  LLVM_DEBUG(dbgs() << "Function group: " << FG.getHead()->getName() << "\n");
  // Collect data from the kernel and subroutine callees
  for (Function *Func : FG)
    initInstructionLevelProperties(Func, RI, ST, BC);
  // Collect data from directly-called stackcall functions
  for (const auto *Subgroup : FG.subgroups())
    for (Function *Func : *Subgroup)
      initInstructionLevelProperties(Func, RI, ST, BC);
}

//===----------------------------------------------------------------------===//
//
// Kernel info implementation.
//
//===----------------------------------------------------------------------===//
void GenXOCLRuntimeInfo::KernelInfo::setArgumentProperties(
    const Function &Kernel, const vc::KernelMetadata &KM,
    const GenXSubtarget &ST, const GenXBackendConfig &BC) {
  IGC_ASSERT_MESSAGE(Kernel.arg_size() == KM.getNumArgs(),
                     "Expected same number of arguments");
  // Some arguments are part of thread payload and do not require
  // entries in arguments info for OCL runtime.
  auto NonPayloadArgs =
      make_filter_range(Kernel.args(), [&KM](const Argument &Arg) {
        uint32_t ArgKind = KM.getArgKind(Arg.getArgNo());
        return !vc::isLocalIDKind(ArgKind);
      });
  KernelArgBuilder ArgBuilder{KM, Kernel.getParent()->getDataLayout(), ST, BC};
  transform(NonPayloadArgs, std::back_inserter(ArgInfos),
            [&ArgBuilder](const Argument &Arg) {
              return ArgBuilder.translateArgument(Arg);
            });
  FuncInfo.UsesReadWriteImages = std::any_of(
      ArgInfos.begin(), ArgInfos.end(), [](const KernelArgInfo &AI) {
        return AI.isImage() &&
               AI.getAccessKind() == KernelArgInfo::AccessKindType::ReadWrite;
      });
}

void GenXOCLRuntimeInfo::KernelInfo::setPrintStrings(
    const Module &KernelModule) {
  const auto *StringsMeta = KernelModule.getNamedMetadata("cm_print_strings");
  if (!StringsMeta)
    return;
  std::transform(StringsMeta->op_begin(), StringsMeta->op_end(),
                 std::back_inserter(PrintStrings), [](const auto *StringMeta) {
                   StringRef Str =
                       cast<MDString>(StringMeta->getOperand(0))->getString();
                   return std::string{Str.begin(), Str.end()};
                 });
}

GenXOCLRuntimeInfo::KernelInfo::KernelInfo(const GenXSubtarget &ST)
    : FuncInfo("Intel_Symbol_Table_Void_Program", ST) {}

GenXOCLRuntimeInfo::KernelInfo::KernelInfo(const FunctionGroup &FG,
                                           GenXOCLRuntimeInfo &RI,
                                           const GenXSubtarget &ST,
                                           const GenXBackendConfig &BC)
    : FuncInfo(FG, RI, ST, BC) {
  vc::KernelMetadata KM{FG.getHead()};
  IGC_ASSERT_MESSAGE(KM.isKernel(),
                     "Expected kernel as head of function group");
  setArgumentProperties(*FG.getHead(), KM, ST, BC);
  setPrintStrings(*FG.getHead()->getParent());
}

//===----------------------------------------------------------------------===//
//
// Compiled kernel implementation.
//
//===----------------------------------------------------------------------===//
GenXOCLRuntimeInfo::CompiledKernel::CompiledKernel(
    KernelInfo &&KI, const CostInfoT &CI, const vISA::FINALIZER_INFO &JI,
    const GTPinInfo &GI, std::vector<char> DbgInfoIn)
    : CompilerInfo(std::move(KI)), CostInfo(CI), JitterInfo(JI), GtpinInfo(GI),
      DebugInfo{std::move(DbgInfoIn)} {}

//===----------------------------------------------------------------------===//
//
// Runtime info pass implementation.
//
//===----------------------------------------------------------------------===//
namespace {

// Relates to GenXOCLRuntimeInfo::SectionInfo. GenXOCLRuntimeInfo::SectionInfo
// can be created from this struct.
struct RawSectionInfo {
  genx::BinaryDataAccumulator<const GlobalValue *> Data;
  GenXOCLRuntimeInfo::RelocationSeq Relocations;
};

struct GVEncodingInfo {
  const GlobalVariable *GV;
  // Alignment requirments of a global variable that will be encoded after
  // the considered GV variable.
  unsigned NextGVAlignment;
};

struct ModuleDataT {
  RawSectionInfo Constant;
  RawSectionInfo Global;
  RawSectionInfo ConstString;
  GenXOCLRuntimeInfo::SymbolSeq ExternalGlobals;
  GenXOCLRuntimeInfo::SymbolSeq ZeroInitGlobals;

  ModuleDataT() = default;
  ModuleDataT(const Module &M);
};

template <vISA::GenSymType SymbolClass, typename InputIter, typename OutputIter>
void constructSymbols(InputIter First, InputIter Last, OutputIter Out) {
  std::transform(First, Last, Out, [](const auto &Section) -> vISA::ZESymEntry {
    return {SymbolClass, static_cast<uint32_t>(Section.Info.Offset),
            static_cast<uint32_t>(Section.Info.getSize()),
            Section.Key->getName().str()};
  });
}

static GenXOCLRuntimeInfo::SymbolSeq constructFunctionSymbols(
    genx::BinaryDataAccumulator<const GlobalValue *> &GenBinary,
    bool HasKernel) {
  GenXOCLRuntimeInfo::SymbolSeq Symbols;
  if (GenBinary.begin() == GenBinary.end())
    return Symbols;
  Symbols.reserve(GenBinary.getNumSections());
  if (HasKernel) {
    auto &KernelSection = GenBinary.front();
    Symbols.emplace_back(vISA::GenSymType::S_KERNEL, KernelSection.Info.Offset,
                         KernelSection.Info.getSize(),
                         KernelSection.Key->getName().str());
  }

  // Skipping first section if binary has a kernel.
  constructSymbols<vISA::GenSymType::S_FUNC>(
      HasKernel ? std::next(GenBinary.begin()) : GenBinary.begin(),
      GenBinary.end(), std::back_inserter(Symbols));

  return Symbols;
}

} // namespace

// Appends the binary of function/kernel represented by \p Func and \p BuiltFunc
// to \p GenBinary.
static void
appendFuncBinary(genx::BinaryDataAccumulator<const GlobalValue *> &GenBinary,
                 const Function &Func, const VISAKernel &BuiltFunc) {
  void *GenBin = nullptr;
  int GenBinSize = 0;
  CISA_CALL(BuiltFunc.GetGenxBinary(GenBin, GenBinSize));
  IGC_ASSERT_MESSAGE(
      GenBin,
      "Unexpected null buffer or zero-sized kernel (compilation failed?)");
  IGC_ASSERT_MESSAGE(
      GenBinSize,
      "Unexpected null buffer or zero-sized kernel (compilation failed?)");
  GenBinary.append(&Func, ArrayRef<uint8_t>{static_cast<uint8_t *>(GenBin),
                                            static_cast<size_t>(GenBinSize)});
  freeBlock(GenBin);
}

// Loads if it is possible external files.
// Returns the success status of the loading.
static bool loadGenBinaryFromFile(
    genx::BinaryDataAccumulator<const GlobalValue *> &GenBinary,
    const Function &F, vc::ShaderOverrider const &Loader,
    vc::ShaderOverrider::Extensions Ext) {
  void *GenBin = nullptr;
  int GenBinSize = 0;

  if (!Loader.override(GenBin, GenBinSize, F.getName(), Ext))
    return false;

  if (!GenBin || !GenBinSize) {
    llvm::errs()
        << "Unexpected null buffer or zero-sized kernel (loading failed?)\n";
    return false;
  }

  GenBinary.append(&F, ArrayRef<uint8_t>{static_cast<uint8_t *>(GenBin),
                                         static_cast<size_t>(GenBinSize)});
  freeBlock(GenBin);
  return true;
}

// Constructs gen binary for Function but loading is from injected file.
// Returns the success status of the overriding.
static bool
tryOverrideBinary(genx::BinaryDataAccumulator<const GlobalValue *> &GenBinary,
                  const Function &F, vc::ShaderOverrider const &Loader) {
  using Extensions = vc::ShaderOverrider::Extensions;

  // Attempts to override .asm
  if (loadGenBinaryFromFile(GenBinary, F, Loader, Extensions::ASM))
    return true;

  // If it has failed then attempts to override .dat file
  return loadGenBinaryFromFile(GenBinary, F, Loader, Extensions::DAT);
}

template <typename UnaryPred>
std::vector<const Function *> collectCalledFunctions(const FunctionGroup &FG,
                                                     UnaryPred &&Pred) {
  std::vector<const Function *> Collected;
  std::set<const FunctionGroup *> Visited;
  std::stack<const FunctionGroup *> Worklist;
  Worklist.push(&FG);

  while (!Worklist.empty()) {
    const FunctionGroup *CurFG = Worklist.top();
    Worklist.pop();
    if (Visited.count(CurFG))
      continue;

    for (const FunctionGroup *SubFG : CurFG->subgroups())
      Worklist.push(SubFG);
    Visited.insert(CurFG);

    const Function *SubgroupHead = CurFG->getHead();
    if (Pred(SubgroupHead))
      Collected.push_back(SubgroupHead);
  }

  return Collected;
}

// Appends relocations of \p Func to \p SectionRelocations. Added relocations
// are shifted by \p Offset.
static void
appendTextRelocations(GenXOCLRuntimeInfo::RelocationSeq &SectionRelocations,
                      VISAKernel &Func, std::size_t Offset) {
  if (!Offset) {
    CISA_CALL(Func.GetRelocations(SectionRelocations));
    return;
  }
  GenXOCLRuntimeInfo::RelocationSeq FuncRelocations;
  CISA_CALL(Func.GetRelocations(FuncRelocations));
  vc::shiftRelocations(FuncRelocations.begin(), FuncRelocations.end(),
                       std::back_inserter(SectionRelocations), Offset);
}

// Either loads binary from VISABuilder or overrides from file.
static void loadBinary(RawSectionInfo &TextSection, VISABuilder &VB,
                       const FunctionGroup &FG, const GenXBackendConfig &BC) {
  const Function &F = *FG.getHead();

  // Attempt to override
  if (BC.hasShaderOverrider() &&
      tryOverrideBinary(TextSection.Data, F, BC.getShaderOverrider()))
    return;

  // If there is no overriding or attemp fails, then gets binary from
  // compilation
  VISAKernel *BuiltKernel = VB.GetVISAKernel(F.getName().str());
  IGC_ASSERT_MESSAGE(BuiltKernel, "Kernel is null");
  appendTextRelocations(TextSection.Relocations, *BuiltKernel,
                        TextSection.Data.getFullSize());
  appendFuncBinary(TextSection.Data, F, *BuiltKernel);
}

static alignment_t getAlignment(const GlobalVariable &GV) {
  auto Align = GV.getAlignment();
  if (Align)
    return Align;
  return GV.getParent()
      ->getDataLayout()
      .getABITypeAlign(GV.getValueType())
      .value();
}

static void appendGlobalVariableData(RawSectionInfo &Sect,
                                     const GlobalVariable &GV,
                                     const DataLayout &DL) {
  std::vector<char> Data;
  GenXOCLRuntimeInfo::RelocationSeq Relocations;
  vc::encodeConstant(*GV.getInitializer(), DL, std::back_inserter(Data),
                     std::back_inserter(Relocations));

  Sect.Data.append(&GV, Data.begin(), Data.end(), getAlignment(GV));

  // vc::encodeConstant calculates offsets relative to GV. Need to make it
  // relative to section start.
  vc::shiftRelocations(std::make_move_iterator(Relocations.begin()),
                       std::make_move_iterator(Relocations.end()),
                       std::back_inserter(Sect.Relocations),
                       Sect.Data.back().Info.Offset);
}

// Fetches DWARF data associated with the specified function.
// Empty vector is returned if none is found.
static GenXDebugInfo::ElfBin getDebugInformation(const GenXDebugInfo &Dbg,
                                                 const Function *F) {
  const auto &DbgInfoStorage = Dbg.getModuleDebug();
  auto DbgInfoIt = DbgInfoStorage.find(F);
  if (DbgInfoIt == DbgInfoStorage.end())
    return {};
  const auto &ElfImage = DbgInfoIt->second;
  return {ElfImage.begin(), ElfImage.end()};
}

static GenXDebugInfo::ElfBin getDebugInfoForIndirectFunctions(
    const GenXDebugInfo &Dbg, const std::vector<FunctionGroup *> &Subgroups) {
  if (Subgroups.empty())
    return {};

  // FIXME: current implementation does not properly handle debug information
  // in the presence of indirect calls. Several indirect functions are
  // embedded into one "section" - which means that the associated DWARF file
  // should contain information about all of them. Currently, we provide DWARF
  // info only about the first function.
  return getDebugInformation(Dbg, Subgroups.front()->getHead());
}

ModuleDataT::ModuleDataT(const Module &M) {
  auto &DL = M.getDataLayout();
  auto RealGlobals =
      make_filter_range(M.globals(), [](const GlobalVariable &GV) {
        return vc::isRealGlobalVariable(GV);
      });
  SmallVector<const GlobalVariable *, 4> ZeroInitialized;
  for (auto &GV : RealGlobals) {
    if (GV.isConstant()) {
      if (GV.hasAttribute(vc::PrintfStringVariable))
        // This section is always empty for oclbin flow. This happens because
        // of printf legalization that separates globals that will be indexed
        // and real globals. Only indexed globals are left marked as printf
        // strings but indexed strings aren't real global variables so they're
        // skipped here. Indexed strings are handled separately.
        appendGlobalVariableData(ConstString, GV, DL);
      else
        appendGlobalVariableData(Constant, GV, DL);
    } else if (GV.hasInitializer()) {
      IGC_ASSERT_MESSAGE(!GV.hasAttribute(vc::PrintfStringVariable),
                         "non-const global variable cannot be a printf string");
      auto *Init = GV.getInitializer();
      if (Init->isNullValue())
        ZeroInitialized.push_back(&GV);
      else
        appendGlobalVariableData(Global, GV, DL);
    } else {
      // External global variables
      auto Name = GV.getName().str();
      ExternalGlobals.emplace_back(vISA::S_UNDEF, 0, 0, Name.c_str());
    }
  }

  uint32_t Offset = Global.Data.getFullSize();
  for (auto *GV : ZeroInitialized) {
    Offset = alignTo(Offset, getAlignment(*GV));
    uint32_t Size =
        vc::getTypeSize(GV->getInitializer()->getType(), &DL).inBytes();
    ZeroInitGlobals.emplace_back(vISA::S_GLOBAL_VAR, Offset, Size,
                                 GV->getName().str().c_str());
    Offset += Size;
  }
}

static GenXOCLRuntimeInfo::ModuleInfoT getModuleInfo(const Module &M) {
  ModuleDataT ModuleData{M};
  GenXOCLRuntimeInfo::ModuleInfoT ModuleInfo;

  constructSymbols<vISA::GenSymType::S_GLOBAL_VAR_CONST>(
      ModuleData.Constant.Data.begin(), ModuleData.Constant.Data.end(),
      std::back_inserter(ModuleInfo.Constant.Symbols));
  constructSymbols<vISA::GenSymType::S_GLOBAL_VAR>(
      ModuleData.Global.Data.begin(), ModuleData.Global.Data.end(),
      std::back_inserter(ModuleInfo.Global.Symbols));
  constructSymbols<vISA::GenSymType::S_GLOBAL_VAR_CONST>(
      ModuleData.ConstString.Data.begin(), ModuleData.ConstString.Data.end(),
      std::back_inserter(ModuleInfo.ConstString.Symbols));

  llvm::copy(ModuleData.ZeroInitGlobals,
             std::back_inserter(ModuleInfo.Global.Symbols));
  llvm::copy(ModuleData.ExternalGlobals,
             std::back_inserter(ModuleInfo.Global.Symbols));

  ModuleInfo.Constant.Relocations = std::move(ModuleData.Constant.Relocations);
  ModuleInfo.Global.Relocations = std::move(ModuleData.Global.Relocations);
  ModuleInfo.ConstString.Relocations =
      std::move(ModuleData.ConstString.Relocations);

  ModuleInfo.Constant.Data.Buffer =
      std::move(ModuleData.Constant.Data).emitConsolidatedData();
  // IGC always sets 0
  ModuleInfo.Constant.Data.Alignment = 0;
  ModuleInfo.Constant.Data.AdditionalZeroedSpace = 0;

  ModuleInfo.Global.Data.Buffer =
      std::move(ModuleData.Global.Data).emitConsolidatedData();
  ModuleInfo.Global.Data.Alignment = 0;
  if (!ModuleData.ZeroInitGlobals.empty()) {
    auto &Last = ModuleData.ZeroInitGlobals.back();
    ModuleInfo.Global.Data.AdditionalZeroedSpace =
        Last.s_offset + Last.s_size - ModuleInfo.Global.Data.Buffer.size();
  } else
    ModuleInfo.Global.Data.AdditionalZeroedSpace = 0;

  ModuleInfo.ConstString.Data.Buffer =
      std::move(ModuleData.ConstString.Data).emitConsolidatedData();
  ModuleInfo.ConstString.Data.Alignment = 0;
  ModuleInfo.ConstString.Data.AdditionalZeroedSpace = 0;

  return std::move(ModuleInfo);
}

namespace {

class RuntimeInfoCollector final {
  GenXOCLRuntimeInfo &RI;
  const FunctionGroupAnalysis &FGA;
  const GenXBackendConfig &BC;
  VISABuilder &VB;
  const GenXSubtarget &ST;
  const Module &M;
  const GenXDebugInfo &DBG;

public:
  using KernelStorageTy = GenXOCLRuntimeInfo::KernelStorageTy;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;
  using CompiledModuleT = GenXOCLRuntimeInfo::CompiledModuleT;

public:
  RuntimeInfoCollector(GenXOCLRuntimeInfo &InRI,
                       const FunctionGroupAnalysis &InFGA,
                       const GenXBackendConfig &InBC, VISABuilder &InVB,
                       const GenXSubtarget &InST, const Module &InM,
                       const GenXDebugInfo &InDbg)
      : RI{InRI}, FGA{InFGA}, BC{InBC}, VB{InVB}, ST{InST}, M{InM}, DBG{InDbg} {
  }

  CompiledModuleT run();

private:
  CompiledKernel collectFunctionGroupInfo(const FunctionGroup &FG) const;
  // Collects all subgroups info in a dummy kernel. Also stores visaasm for the
  // whole module in this dummy kernel.
  template <typename Range>
  CompiledKernel
  collectFunctionSubgroupsInfo(const std::vector<FunctionGroup *> &Subgroups,
                               const Range &DeclsRange) const;
};

} // namespace

RuntimeInfoCollector::CompiledModuleT RuntimeInfoCollector::run() {
  KernelStorageTy Kernels;
  std::transform(FGA.begin(), FGA.end(), std::back_inserter(Kernels),
                 [this](const FunctionGroup *FG) {
                   return collectFunctionGroupInfo(*FG);
                 });

  std::vector<FunctionGroup *> IndirectlyReferencedFuncs;
  std::copy_if(FGA.subgroup_begin(), FGA.subgroup_end(),
               std::back_inserter(IndirectlyReferencedFuncs),
               [&BECfg = BC](const FunctionGroup *FG) {
                 return vc::isIndirect(FG->getHead()) &&
                        !BECfg.directCallsOnly(FG->getHead()->getName());
               });
  auto &&DeclsRange =
      llvm::make_filter_range(M.functions(), [](const Function &F) {
        if (!F.isDeclaration())
          return false;
        return vc::isIndirect(F);
      });
  if (!IndirectlyReferencedFuncs.empty() ||
      DeclsRange.begin() != DeclsRange.end() || BC.emitZebinVisaSections())
    Kernels.push_back(
        collectFunctionSubgroupsInfo(IndirectlyReferencedFuncs, DeclsRange));

  return {getModuleInfo(M), std::move(Kernels),
          M.getDataLayout().getPointerSize()};
}

RuntimeInfoCollector::CompiledKernel
RuntimeInfoCollector::collectFunctionGroupInfo(const FunctionGroup &FG) const {
  using KernelInfo = GenXOCLRuntimeInfo::KernelInfo;
  using CostInfoT = GenXOCLRuntimeInfo::CostInfoT;
  using GTPinInfo = GenXOCLRuntimeInfo::GTPinInfo;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;

  // Compiler info.
  KernelInfo Info{FG, RI, ST, BC};

  const Function *KernelFunction = FG.getHead();
  const std::string KernelName = KernelFunction->getName().str();
  VISAKernel *VK = VB.GetVISAKernel(KernelName);
  IGC_ASSERT_MESSAGE(VK, "Kernel is null");
  vISA::FINALIZER_INFO *JitInfo = nullptr;
  CISA_CALL(VK->GetJitInfo(JitInfo));
  IGC_ASSERT_MESSAGE(JitInfo, "Jit info is not set by finalizer");
  // TODO: this a temporary solution for spill mem size
  // calculation. This has to be redesign properly, maybe w/ multiple
  // KernelInfos or by introducing FunctionInfos
  const auto StackCalls = collectCalledFunctions(
      FG, [](const Function *F) { return vc::requiresStackCall(F); });
  for (const Function *F : StackCalls) {
    const std::string FuncName = F->getName().str();
    VISAKernel *VF = VB.GetVISAKernel(FuncName);
    IGC_ASSERT_MESSAGE(VF, "Function is null");
    vISA::FINALIZER_INFO *FuncJitInfo = nullptr;
    CISA_CALL(VF->GetJitInfo(FuncJitInfo));
    IGC_ASSERT_MESSAGE(FuncJitInfo, "Func jit info is not set by finalizer");
    JitInfo->hasStackcalls |= FuncJitInfo->hasStackcalls;
    JitInfo->stats.spillMemUsed += FuncJitInfo->stats.spillMemUsed;
  }

  RawSectionInfo TextSection;
  if (!BC.emitVisaOnly()) {
    loadBinary(TextSection, VB, FG, BC);
  }

  auto DebugData = getDebugInformation(DBG, KernelFunction);

  Info.Func.Relocations = TextSection.Relocations;
  Info.Func.Symbols =
      constructFunctionSymbols(TextSection.Data, /*HasKernel=*/true);

  void *GTPinBuffer = nullptr;
  unsigned GTPinBufferSize = 0;
  CISA_CALL(VK->GetGTPinBuffer(GTPinBuffer, GTPinBufferSize,
                               JitInfo->stats.spillMemUsed));

  auto *GTPinBytes = static_cast<char *>(GTPinBuffer);
  GTPinInfo gtpin{GTPinBytes, GTPinBytes + GTPinBufferSize};

  Info.Func.Data.Buffer = std::move(TextSection.Data).emitConsolidatedData();

  CostInfoT CInfo{};
  if (BC.isCostModelEnabled()) {
    // Get cost metrics from finalizer.
    const vISA::KernelCostInfo *KernelCostInfo = nullptr;
    VISAKernel *VF = VB.GetVISAKernel(FG.getName().str());
    VF->getKernelCostInfo(KernelCostInfo);
    if (KernelCostInfo) {
      auto &VISAKernelMetric = KernelCostInfo->kernelCost.C;
      CInfo.Costs.push_back({(int)VISAKernelMetric.cycles,
                             (int)VISAKernelMetric.loadBytes,
                             (int)VISAKernelMetric.storeBytes, 0});
      for (auto &VISACost : KernelCostInfo->allLoopCosts) {
        auto &VISAMetric = VISACost.loopBodyCost.C;
        CInfo.Costs.push_back(
            {(int)VISAMetric.cycles, (int)VISAMetric.loadBytes,
             (int)VISAMetric.storeBytes, VISACost.numChildLoops});
        // Count the number of top-level loops for the kernel.
        if (VISACost.nestingLevel == 1)
          CInfo.Costs[0].NumLoops += 1;
      }
    } else
      IGC_ASSERT_MESSAGE(0, "Failed to get cost metrics from finalizer");

    // Finalize cost expressions.
    auto &LI = RI.getAnalysis<LoopInfoWrapperPass>(*FG.getHead()).getLoopInfo();
    SmallVector<Loop *, 4> Loops = LI.getLoopsInPreorder();
    for (auto *L : Loops) {
      auto Expression = vc::restoreLCEFromMetadata(*L);
      if (Expression.IsUndef || Expression.Factor == 0.0f) {
        CInfo.Expressions.push_back({Expression.Factor, -1, Expression.Addend});
        continue;
      }
      CostInfoT::ArgSymInfo Symbol{
          Expression.Symbol.Num, Expression.Symbol.Offset,
          Expression.Symbol.Size, Expression.Symbol.IsIndirect};
      auto SymIt =
          std::find(CInfo.Symbols.begin(), CInfo.Symbols.end(), Symbol);
      int Idx = std::distance(CInfo.Symbols.begin(), SymIt);
      if (SymIt == CInfo.Symbols.end())
        CInfo.Symbols.push_back(Symbol);
      CInfo.Expressions.push_back({Expression.Factor, Idx, Expression.Addend});
    }
  }
  return CompiledKernel{std::move(Info), std::move(CInfo), *JitInfo,
                        std::move(gtpin), std::move(DebugData)};
}

// Goes through function groups in FGRange and collects their vISA asms into a
// string.
template <typename Range>
static std::vector<GenXOCLRuntimeInfo::KernelInfo::NamedVISAAsm>
collectVISAAsm(const VISABuilder &VB, Range &&FGRange) {
  std::vector<GenXOCLRuntimeInfo::KernelInfo::NamedVISAAsm> VISAAsm;
  std::transform(FGRange.begin(), FGRange.end(), std::back_inserter(VISAAsm),
                 [&VB](const FunctionGroup *FG) {
                   auto Name = FG->getName();
                   return std::make_pair(
                       Name.str(), VB.GetVISAKernel(Name.str())->getVISAAsm());
                 });
  return VISAAsm;
}

template <typename Range>
RuntimeInfoCollector::CompiledKernel
RuntimeInfoCollector::collectFunctionSubgroupsInfo(
    const std::vector<FunctionGroup *> &Subgroups,
    const Range &DeclsRange) const {
  using KernelInfo = GenXOCLRuntimeInfo::KernelInfo;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;

  IGC_ASSERT(!Subgroups.empty() || DeclsRange.begin() != DeclsRange.end() ||
             BC.emitZebinVisaSections());

  RawSectionInfo TextSection;
  if (!BC.emitVisaOnly()) {
    for (auto *FG : Subgroups) {
      auto *Func = FG->getHead();
      IGC_ASSERT(genx::fg::isSubGroupHead(*Func));
      loadBinary(TextSection, VB, *FG, BC);
    }
  }

  auto DebugInfo = getDebugInfoForIndirectFunctions(DBG, Subgroups);

  KernelInfo Info{ST};
  if (BC.emitZebinVisaSections())
    Info.VISAAsm = collectVISAAsm(VB, FGA.AllGroups());
  // FIXME: cannot initialize legacy relocations as the relocation structure is
  // opaque and cannot be modified. But having multiple functions inside a
  // section requires shifting (modifying) the relocations.
  Info.Func.Relocations = TextSection.Relocations;
  Info.Func.Symbols =
      constructFunctionSymbols(TextSection.Data, /*HasKernel*/ false);
  for (auto &&Decl : DeclsRange)
    Info.Func.Symbols.emplace_back(vISA::GenSymType::S_UNDEF, 0, 0,
                                   Decl.getName().str());
  Info.Func.Data.Buffer = TextSection.Data.emitConsolidatedData();

  return CompiledKernel{std::move(Info), /*CostInfo*/ {},
                        vISA::FINALIZER_INFO{},
                        /*GtpinInfo*/ {}, std::move(DebugInfo)};
}

void GenXOCLRuntimeInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<GenXDebugInfo>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXGlobalUniformAnalysis>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.setPreservesAll();
}

bool GenXOCLRuntimeInfo::runOnModule(Module &M) {
  const auto &FGA = getAnalysis<FunctionGroupAnalysis>();
  const auto &BC = getAnalysis<GenXBackendConfig>();
  // Getters for builders are not constant.
  auto &GM = getAnalysis<GenXModule>();
  const auto &ST = getAnalysis<TargetPassConfig>()
                       .getTM<GenXTargetMachine>()
                       .getGenXSubtarget();
  const auto &DBG = getAnalysis<GenXDebugInfo>();

  VISABuilder &VB = *((GM.HasInlineAsm() || !BC.getVISALTOStrings().empty())
                          ? GM.GetVISAAsmReader()
                          : GM.GetCisaBuilder());

  CompiledModule = RuntimeInfoCollector{*this, FGA, BC, VB, ST, M, DBG}.run();
  return false;
}

void GenXOCLRuntimeInfo::print(raw_ostream &OS, const Module *M) const {
  vc::printOCLRuntimeInfo(OS, CompiledModule);
}

INITIALIZE_PASS_BEGIN(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                      "GenXOCLRuntimeInfo", false, true)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis);
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig);
INITIALIZE_PASS_DEPENDENCY(GenXModule);
INITIALIZE_PASS_DEPENDENCY(GenXDebugInfo);
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig);
INITIALIZE_PASS_DEPENDENCY(GenXGlobalUniformAnalysis);
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
INITIALIZE_PASS_END(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                    "GenXOCLRuntimeInfo", false, true)
