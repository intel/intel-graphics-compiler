/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

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

#include "vc/GenXOpts/Utils/InternalMetadata.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include <visaBuilder_interface.h>

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/InitializePasses.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <stack>

#include "Probe/Assertion.h"

#define CISA_CALL(c)                                                           \
  do {                                                                         \
    auto Result = (c);                                                         \
    (void)Result;                                                              \
    IGC_ASSERT_MESSAGE(Result == 0, "Call to VISA API failed: " #c);           \
  } while (0);

using namespace llvm;

char GenXOCLRuntimeInfo::ID = 0;

//===----------------------------------------------------------------------===//
//
// Kernel argument info implementation.
//
//===----------------------------------------------------------------------===//
// Supported kernel argument attributes.
struct OCLAttributes {
  // Type qualifiers for resources.
  static constexpr auto ReadOnly = "read_only";
  static constexpr auto WriteOnly = "write_only";
  static constexpr auto ReadWrite = "read_write";

  // Buffer surface.
  static constexpr auto Buffer = "buffer_t";
  // SVM pointer to buffer.
  static constexpr auto SVM = "svmptr_t";
  // OpenCL-like types.
  static constexpr auto Sampler = "sampler_t";
  static constexpr auto Image1d = "image1d_t";
  static constexpr auto Image1dArray = "image1d_array_t";
  // Same as 1D image. Seems that there is no difference in runtime.
  static constexpr auto Image1dBuffer = "image1d_buffer_t";
  static constexpr auto Image2d = "image2d_t";
  static constexpr auto Image2dArray = "image2d_array_t";
  static constexpr auto Image2dMediaBlock = "image2d_media_block_t";
  static constexpr auto Image3d = "image3d_t";
};

namespace llvm {
class KernelArgBuilder final {
  using ArgKindType = GenXOCLRuntimeInfo::KernelArgInfo::KindType;
  using ArgAccessKindType = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;

  const genx::KernelMetadata &KM;
  const DataLayout &DL;
  const GenXSubtarget &ST;
  const GenXBackendConfig &BC;

public:
  KernelArgBuilder(const genx::KernelMetadata &KMIn, const DataLayout &DLIn,
                   const GenXSubtarget &STIn, const GenXBackendConfig &BCIn)
      : KM(KMIn), DL(DLIn), ST(STIn), BC(BCIn) {}

  GenXOCLRuntimeInfo::KernelArgInfo
  translateArgument(const Argument &Arg) const;

private:
  static auto getStrPred(const char *Attr) {
    return [Attr](StringRef Token) { return Token == Attr; };
  }

  ArgKindType getOCLArgKind(ArrayRef<StringRef> Tokens, unsigned ArgNo) const;
  ArgAccessKindType getOCLArgAccessKind(ArrayRef<StringRef> Tokens,
                                        ArgKindType Kind) const;
  std::pair<ArgKindType, ArgAccessKindType>
  translateArgDesc(unsigned ArgNo) const;
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
  case ArgKindType::BindlessBuffer:
    if (any_of(Tokens, getStrPred(OCLAttributes::ReadOnly)))
      return ArgAccessKindType::ReadOnly;
    if (any_of(Tokens, getStrPred(OCLAttributes::WriteOnly)))
      return ArgAccessKindType::WriteOnly;
    return ArgAccessKindType::ReadWrite;
  default:
    return ArgAccessKindType::None;
  }
}

KernelArgBuilder::ArgKindType
KernelArgBuilder::getOCLArgKind(ArrayRef<StringRef> Tokens,
                                unsigned ArgNo) const {
  unsigned RawKind = KM.getArgKind(ArgNo);

  // Implicit arguments.
  genx::KernelArgInfo KAI{RawKind};
  if (KAI.isLocalSize())
    return ArgKindType::LocalSize;
  if (KAI.isGroupCount())
    return ArgKindType::GroupCount;
  if (KAI.isPrintBuffer())
    return ArgKindType::PrintBuffer;
  if (KAI.isPrivateBase())
    return ArgKindType::PrivateBase;
  if (KAI.isByValSVM())
    return ArgKindType::ByValSVM;

  // Explicit arguments.
  switch (KM.getArgCategory(ArgNo)) {
  default:
    return ArgKindType::General;
  case genx::RegCategory::GENERAL:
    if (any_of(Tokens, getStrPred(OCLAttributes::SVM)))
      return ArgKindType::SVM;
    // Bindless buffers have general category but buffer annotation.
    if (any_of(Tokens, getStrPred(OCLAttributes::Buffer)))
      return ArgKindType::BindlessBuffer;
    return ArgKindType::General;
  case genx::RegCategory::SURFACE:
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1d)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1dArray)))
      return ArgKindType::Image1DArray;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image1dBuffer)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2d))) {
      if (BC.usePlain2DImages())
        return ArgKindType::Image2D;
      // Legacy behavior to treat all 2d images as media block.
      return ArgKindType::Image2DMediaBlock;
    }
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2dArray)))
      return ArgKindType::Image2DArray;
    if (any_of(Tokens, getStrPred(OCLAttributes::Image2dMediaBlock))) {
      return ArgKindType::Image2DMediaBlock;
    }
    if (any_of(Tokens, getStrPred(OCLAttributes::Image3d)))
      return ArgKindType::Image3D;
    return ArgKindType::Buffer;
  case genx::RegCategory::SAMPLER:
    return ArgKindType::Sampler;
  }
}

// Retrieve Kind and AccessKind from given ArgTypeDesc in metadata.
std::pair<KernelArgBuilder::ArgKindType, KernelArgBuilder::ArgAccessKindType>
KernelArgBuilder::translateArgDesc(unsigned ArgNo) const {
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

  const ArgKindType Kind = getOCLArgKind(Tokens, ArgNo);
  const ArgAccessKindType AccessKind = getOCLArgAccessKind(Tokens, Kind);
  return {Kind, AccessKind};
}

unsigned KernelArgBuilder::getArgSizeInBytes(const Argument &Arg) const {
  Type *ArgTy = Arg.getType();
  if (ArgTy->isPointerTy())
    return DL.getPointerTypeSize(ArgTy);
  if (KM.isBufferType(Arg.getArgNo()))
    return DL.getPointerSize();
  return ArgTy->getPrimitiveSizeInBits() / genx::ByteBits;
}

GenXOCLRuntimeInfo::KernelArgInfo
KernelArgBuilder::translateArgument(const Argument &Arg) const {
  GenXOCLRuntimeInfo::KernelArgInfo Info;
  const unsigned ArgNo = Arg.getArgNo();
  std::tie(Info.Kind, Info.AccessKind) = translateArgDesc(ArgNo);
  Info.Offset = KM.getArgOffset(ArgNo);
  Info.SizeInBytes = getArgSizeInBytes(Arg);
  Info.BTI = KM.getBTI(ArgNo);
  // For implicit arguments that are byval argument linearization, index !=
  // ArgNo in the IR function.
  Info.Index = KM.getArgIndex(ArgNo);
  // Linearization arguments have a non-zero offset in the original explicit
  // byval arg.
  Info.OffsetInArg = KM.getOffsetInArg(ArgNo);

  return Info;
}

//===----------------------------------------------------------------------===//
//
// Kernel info implementation.
//
//===----------------------------------------------------------------------===//
// Just perform linear instructions scan to find usage stats.
void GenXOCLRuntimeInfo::KernelInfo::setInstructionUsageProperties(
    const FunctionGroup &FG, const GenXBackendConfig &BC) {
  for (Function *F : FG) {
    for (BasicBlock &BB : *F) {
      for (Instruction &I : BB) {
        switch (GenXIntrinsic::getGenXIntrinsicID(&I)) {
        default:
          break;
        case GenXIntrinsic::genx_group_id_x:
        case GenXIntrinsic::genx_group_id_y:
        case GenXIntrinsic::genx_group_id_z:
          UsesGroupId = true;
          break;
        case GenXIntrinsic::genx_barrier:
        case GenXIntrinsic::genx_sbarrier:
          UsesBarriers = true;
          break;
        case GenXIntrinsic::genx_ssdp4a:
        case GenXIntrinsic::genx_sudp4a:
        case GenXIntrinsic::genx_usdp4a:
        case GenXIntrinsic::genx_uudp4a:
        case GenXIntrinsic::genx_ssdp4a_sat:
        case GenXIntrinsic::genx_sudp4a_sat:
        case GenXIntrinsic::genx_usdp4a_sat:
        case GenXIntrinsic::genx_uudp4a_sat:
        case GenXIntrinsic::genx_dpas:
        case GenXIntrinsic::genx_dpas2:
        case GenXIntrinsic::genx_dpasw:
        case GenXIntrinsic::genx_dpas_nosrc0:
        case GenXIntrinsic::genx_dpasw_nosrc0:
          UsesDPAS = true;
          break;
#if 0
        // ThreadPrivateMemSize was not copied to igcmc structures
        // always defaulting to zero and everything worked. After
        // removal of igcmc structures TPMSize started to be
        // initialized to values other than zero and some ispc tests
        // started to fail.
        // Restore old behavior as temporary fix until proper
        // investigation will be performed. This is really strange.
        case GenXIntrinsic::genx_alloca:
          ThreadPrivateMemSize = BC.getStackSurfaceMaxSize();
          break;
#endif
        }
      }
    }
  }
}

void GenXOCLRuntimeInfo::KernelInfo::setMetadataProperties(
    genx::KernelMetadata &KM, const GenXSubtarget &ST) {
  Name = KM.getName().str();
  SLMSize = KM.getSLMSize();

}

void GenXOCLRuntimeInfo::KernelInfo::setArgumentProperties(
    const Function &Kernel, const genx::KernelMetadata &KM,
    const GenXSubtarget &ST, const GenXBackendConfig &BC) {
  IGC_ASSERT_MESSAGE(Kernel.arg_size() == KM.getNumArgs(),
    "Expected same number of arguments");
  // Some arguments are part of thread payload and do not require
  // entries in arguments info for OCL runtime.
  auto NonPayloadArgs =
      make_filter_range(Kernel.args(), [&KM](const Argument &Arg) {
        uint32_t ArgKind = KM.getArgKind(Arg.getArgNo());
        genx::KernelArgInfo KAI(ArgKind);
        return !KAI.isLocalIDs();
      });
  KernelArgBuilder ArgBuilder{KM, Kernel.getParent()->getDataLayout(), ST, BC};
  transform(NonPayloadArgs, std::back_inserter(ArgInfos),
            [&ArgBuilder](const Argument &Arg) {
              return ArgBuilder.translateArgument(Arg);
            });
  UsesReadWriteImages = std::any_of(
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

GenXOCLRuntimeInfo::KernelInfo::KernelInfo(const FunctionGroup &FG,
                                           const GenXSubtarget &ST,
                                           const GenXBackendConfig &BC) {
  setInstructionUsageProperties(FG, BC);

  GRFSizeInBytes = ST.getGRFByteSize();

  int StackAmount = genx::getStackAmount(FG.getHead());
  if (StackAmount == genx::VC_STACK_USAGE_UNKNOWN)
    StackAmount = BC.getStatelessPrivateMemSize();
  StatelessPrivateMemSize = StackAmount;

  SupportsDebugging = BC.emitDebuggableKernels();

  genx::KernelMetadata KM{FG.getHead()};
  IGC_ASSERT_MESSAGE(KM.isKernel(), "Expected kernel as head of function group");
  setMetadataProperties(KM, ST);
  setArgumentProperties(*FG.getHead(), KM, ST, BC);
  setPrintStrings(*FG.getHead()->getParent());
}

//===----------------------------------------------------------------------===//
//
// Compiled kernel implementation.
//
//===----------------------------------------------------------------------===//
GenXOCLRuntimeInfo::CompiledKernel::CompiledKernel(KernelInfo &&KI,
                                                   const FINALIZER_INFO &JI,
                                                   const GTPinInfo &GI,
                                                   std::vector<char> DbgInfoIn)
    : CompilerInfo(std::move(KI)), JitterInfo(JI),
      GtpinInfo(GI),
      DebugInfo{std::move(DbgInfoIn)} {
}

//===----------------------------------------------------------------------===//
//
// Runtime info pass implementation.
//
//===----------------------------------------------------------------------===//
namespace {

// Relates to GenXOCLRuntimeInfo::SectionInfo. GenXOCLRuntimeInfo::SectionInfo
// can be created from this struct.
struct RawSectionInfo {
  genx::BinaryDataAccumulator<const GlobalVariable *> Data;
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
    genx::BinaryDataAccumulator<const Function *> &GenBinary) {
  GenXOCLRuntimeInfo::SymbolSeq Symbols;
  Symbols.reserve(GenBinary.getNumSections());
  auto &KernelSection = GenBinary.front();
  Symbols.emplace_back(vISA::GenSymType::S_KERNEL, KernelSection.Info.Offset,
                       KernelSection.Info.getSize(),
                       KernelSection.Key->getName().str());

  // Skipping first section with the kernel.
  constructSymbols<vISA::GenSymType::S_FUNC>(std::next(GenBinary.begin()),
                                             GenBinary.end(),
                                             std::back_inserter(Symbols));

  return Symbols;
}

} // namespace

// Appends the binary of function/kernel represented by \p Func and \p BuiltFunc
// to \p GenBinary.
static void
appendFuncBinary(genx::BinaryDataAccumulator<const Function *> &GenBinary,
                 const Function &Func, const VISAKernel &BuiltFunc) {
  void *GenBin = nullptr;
  int GenBinSize = 0;
  CISA_CALL(BuiltFunc.GetGenxBinary(GenBin, GenBinSize));
  IGC_ASSERT_MESSAGE(GenBin,
      "Unexpected null buffer or zero-sized kernel (compilation failed?)");
  IGC_ASSERT_MESSAGE(GenBinSize,
      "Unexpected null buffer or zero-sized kernel (compilation failed?)");
  GenBinary.append(&Func, ArrayRef<uint8_t>{static_cast<uint8_t *>(GenBin),
                                            static_cast<size_t>(GenBinSize)});
  freeBlock(GenBin);
}

// Loads if it is possible external files.
// Returns the success status of the loading.
static bool
loadGenBinaryFromFile(genx::BinaryDataAccumulator<const Function *> &GenBinary,
                      const Function &Kernel, const Function &F,
                      vc::ShaderOverrider const &Loader,
                      vc::ShaderOverrider::Extensions Ext) {
  void *GenBin = nullptr;
  int GenBinSize = 0;

  if (!Loader.override(GenBin, GenBinSize, Kernel.getName(), F.getName(), Ext))
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
tryOverrideBinary(genx::BinaryDataAccumulator<const Function *> &GenBinary,
                  const Function &Kernel, const Function &F,
                  vc::ShaderOverrider const &Loader) {
  using Extensions = vc::ShaderOverrider::Extensions;

  // Attempts to override .asm
  if (loadGenBinaryFromFile(GenBinary, Kernel, F, Loader, Extensions::ASM))
    return true;

  // If it has failed then attempts to override .dat file
  return loadGenBinaryFromFile(GenBinary, Kernel, F, Loader, Extensions::DAT);
}

// Either loads binaries from VISABuilder or overrides from files.
// \p Kernel should always be kernel function, meanwhile if \p F is actually a
// kernel means we are loading kernel, and if \p F is a function means we are
// loading function.
static void
loadBinaries(genx::BinaryDataAccumulator<const Function *> &GenBinary,
             VISABuilder &VB, const Function &Kernel, const Function &F,
             GenXBackendConfig const &BC) {
  // Attempt to override
  if (BC.hasShaderOverrider() &&
      tryOverrideBinary(GenBinary, Kernel, F, BC.getShaderOverrider()))
    return;

  // If there is no overriding or attemp fails, then gets binary from compilation
  VISAKernel *BuiltKernel = VB.GetVISAKernel(F.getName().str());
  IGC_ASSERT_MESSAGE(BuiltKernel, "Kernel is null");
  appendFuncBinary(GenBinary, F, *BuiltKernel);
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

// Constructs gen binary for provided function group \p FG.
static genx::BinaryDataAccumulator<const Function *>
getGenBinary(const FunctionGroup &FG, VISABuilder &VB,
             GenXBackendConfig const &BC,
             std::set<const Function *> &ProcessedCalls) {
  Function const *Kernel = FG.getHead();
  genx::BinaryDataAccumulator<const Function *> GenBinary;
  // load kernel
  loadBinaries(GenBinary, VB, *Kernel, *Kernel, BC);

  const auto IndirectFunctions = collectCalledFunctions(
      FG, [](const Function *F) { return genx::isReferencedIndirectly(F); });
  for (const Function *F : IndirectFunctions) {
    if (ProcessedCalls.count(F) != 0)
      continue;
    ProcessedCalls.insert(F);
    // load functions
    loadBinaries(GenBinary, VB, *Kernel, *F, BC);
  }

  return std::move(GenBinary);
}

static void appendGlobalVariableData(RawSectionInfo &Sect,
                                     GVEncodingInfo GVInfo,
                                     const DataLayout &DL) {
  std::vector<char> Data;
  GenXOCLRuntimeInfo::RelocationSeq Relocations;
  vc::encodeConstant(*GVInfo.GV->getInitializer(), DL, std::back_inserter(Data),
                     std::back_inserter(Relocations));

  const auto CurrentGVAddress = Sect.Data.getFullSize();
  const auto UnalignedNextGVAddress = CurrentGVAddress + Data.size();
  const auto AlignedNextGVAddress =
      alignTo(UnalignedNextGVAddress, GVInfo.NextGVAlignment);

  // Pad before the next global.
  std::fill_n(std::back_inserter(Data),
              AlignedNextGVAddress - UnalignedNextGVAddress, 0);

  // vc::encodeConstant calculates offsets relative to GV. Need to make it
  // relative to section start.
  vc::shiftRelocations(std::make_move_iterator(Relocations.begin()),
                       std::make_move_iterator(Relocations.end()),
                       std::back_inserter(Sect.Relocations), CurrentGVAddress);

  Sect.Data.append(GVInfo.GV, Data.begin(), Data.end());
}

static unsigned getAlignment(const GlobalVariable &GV) {
  unsigned Align = GV.getAlignment();
  if (Align)
    return Align;
  return GV.getParent()->getDataLayout().getABITypeAlignment(GV.getValueType());
}

template <typename GlobalsRangeT>
std::vector<GVEncodingInfo>
prepareGlobalInfosForEncoding(GlobalsRangeT &&Globals) {
  auto RealGlobals = make_filter_range(Globals, [](const GlobalVariable &GV) {
    return genx::isRealGlobalVariable(GV);
  });
  if (RealGlobals.begin() == RealGlobals.end())
    return {};
  std::vector<GVEncodingInfo> Infos;
  std::transform(RealGlobals.begin(), std::prev(RealGlobals.end()),
                 std::next(RealGlobals.begin()), std::back_inserter(Infos),
                 [](const GlobalVariable &GV, const GlobalVariable &NextGV) {
                   return GVEncodingInfo{&GV, getAlignment(NextGV)};
                 });
  Infos.push_back({&*std::prev(RealGlobals.end()), 1u});
  return std::move(Infos);
}

ModuleDataT::ModuleDataT(const Module &M) {
  std::vector<GVEncodingInfo> GVInfos =
      prepareGlobalInfosForEncoding(M.globals());
  for (auto GVInfo : GVInfos) {
    if (GVInfo.GV->isConstant())
      appendGlobalVariableData(Constant, GVInfo, M.getDataLayout());
    else
      appendGlobalVariableData(Global, GVInfo, M.getDataLayout());
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

  ModuleInfo.Constant.Relocations = std::move(ModuleData.Constant.Relocations);
  ModuleInfo.Global.Relocations = std::move(ModuleData.Global.Relocations);

  ModuleInfo.Constant.Data.Buffer =
      std::move(ModuleData.Constant.Data).emitConsolidatedData();
  // IGC always sets 0
  ModuleInfo.Constant.Data.Alignment = 0;
  ModuleInfo.Constant.Data.AdditionalZeroedSpace = 0;

  ModuleInfo.Global.Data.Buffer =
      std::move(ModuleData.Global.Data).emitConsolidatedData();
  ModuleInfo.Global.Data.Alignment = 0;
  ModuleInfo.Global.Data.AdditionalZeroedSpace = 0;

  return std::move(ModuleInfo);
}

namespace {

class RuntimeInfoCollector final {
  const FunctionGroupAnalysis &FGA;
  const GenXBackendConfig &BC;
  VISABuilder &VB;
  const GenXSubtarget &ST;
  const Module &M;
  const GenXDebugInfo &DBG;
  std::set<const llvm::Function *> ProcessedCalls;

public:
  using KernelStorageTy = GenXOCLRuntimeInfo::KernelStorageTy;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;
  using CompiledModuleT = GenXOCLRuntimeInfo::CompiledModuleT;

public:
  RuntimeInfoCollector(const FunctionGroupAnalysis &InFGA,
                       const GenXBackendConfig &InBC, VISABuilder &InVB,
                       const GenXSubtarget &InST, const Module &InM,
                       const GenXDebugInfo &InDbg)
      : FGA{InFGA}, BC{InBC}, VB{InVB}, ST{InST}, M{InM}, DBG{InDbg} {}

  CompiledModuleT run();

private:
  CompiledKernel collectFunctionGroupInfo(const FunctionGroup &FG);
};

} // namespace

RuntimeInfoCollector::CompiledModuleT RuntimeInfoCollector::run() {
  KernelStorageTy Kernels;
  std::transform(FGA.begin(), FGA.end(), std::back_inserter(Kernels),
                 [this](const FunctionGroup *FG) {
                   return collectFunctionGroupInfo(*FG);
                 });
  return {getModuleInfo(M), std::move(Kernels),
          M.getDataLayout().getPointerSize()};
}

RuntimeInfoCollector::CompiledKernel
RuntimeInfoCollector::collectFunctionGroupInfo(const FunctionGroup &FG) {
  using KernelInfo = GenXOCLRuntimeInfo::KernelInfo;
  using GTPinInfo = GenXOCLRuntimeInfo::GTPinInfo;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;

  // Compiler info.
  KernelInfo Info{FG, ST, BC};

  const Function *KernelFunction = FG.getHead();
  const std::string KernelName = KernelFunction->getName().str();
  VISAKernel *VK = VB.GetVISAKernel(KernelName);
  IGC_ASSERT_MESSAGE(VK, "Kernel is null");
  FINALIZER_INFO *JitInfo = nullptr;
  CISA_CALL(VK->GetJitInfo(JitInfo));
  IGC_ASSERT_MESSAGE(JitInfo, "Jit info is not set by finalizer");
  // TODO: this a temporary solution for spill mem size
  // calculation. This has to be redesign properly, maybe w/ multiple
  // KernelInfos or by introducing FunctionInfos
  const auto StackCalls = collectCalledFunctions(
      FG, [](const Function *F) { return genx::requiresStackCall(F); });
  for (const Function *F : StackCalls) {
    const std::string FuncName = F->getName().str();
    VISAKernel *VF = VB.GetVISAKernel(FuncName);
    IGC_ASSERT_MESSAGE(VF, "Function is null");
    FINALIZER_INFO *FuncJitInfo = nullptr;
    CISA_CALL(VF->GetJitInfo(FuncJitInfo));
    IGC_ASSERT_MESSAGE(FuncJitInfo, "Func jit info is not set by finalizer");
    JitInfo->isSpill |= FuncJitInfo->isSpill;
    JitInfo->hasStackcalls |= FuncJitInfo->hasStackcalls;
    JitInfo->spillMemUsed += FuncJitInfo->spillMemUsed;
  }

  genx::BinaryDataAccumulator<const Function *> GenBinary =
      getGenBinary(FG, VB, BC, ProcessedCalls);

  const auto& Dbg = DBG.getModuleDebug();
  auto DbgIt = Dbg.find(KernelFunction);
  std::vector<char> DebugData;
  if (DbgIt != std::end(Dbg)) {
    const auto &ElfImage = DbgIt->second;
    DebugData = {ElfImage.begin(), ElfImage.end()};
  }
  CISA_CALL(VK->GetRelocations(Info.Func.Relocations));
  // Still have to duplicate function relocations because they are constructed
  // inside Finalizer.
  CISA_CALL(VK->GetGenRelocEntryBuffer(Info.LegacyFuncRelocations.Buffer,
                                       Info.LegacyFuncRelocations.Size,
                                       Info.LegacyFuncRelocations.Entries));
  Info.Func.Symbols = constructFunctionSymbols(GenBinary);

  void *GTPinBuffer = nullptr;
  unsigned GTPinBufferSize = 0;
  CISA_CALL(VK->GetGTPinBuffer(GTPinBuffer, GTPinBufferSize));

  auto *GTPinBytes = static_cast<char *>(GTPinBuffer);
  GTPinInfo gtpin{{GTPinBytes, GTPinBytes + GTPinBufferSize}};

  Info.Func.Data.Buffer = std::move(GenBinary).emitConsolidatedData();
  return CompiledKernel{std::move(Info), *JitInfo, std::move(gtpin),
                        std::move(DebugData)};
}

void GenXOCLRuntimeInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<GenXDebugInfo>();
  AU.addRequired<TargetPassConfig>();
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

  VISABuilder &VB =
      *(GM.HasInlineAsm() ? GM.GetVISAAsmReader() : GM.GetCisaBuilder());

  CompiledModule = RuntimeInfoCollector{FGA, BC, VB, ST, M, DBG}.run();
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
INITIALIZE_PASS_END(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                    "GenXOCLRuntimeInfo", false, true)
