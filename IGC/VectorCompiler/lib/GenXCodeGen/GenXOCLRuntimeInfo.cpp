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

#include "GenXOCLRuntimeInfo.h"

#include "GenX.h"
#include "GenXModule.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include <visaBuilder_interface.h>

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/DataLayout.h>

#include <cctype>
#include <functional>
#include <iterator>

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
// Kernel info implementation.
//
//===----------------------------------------------------------------------===//
// Just perform linear instructions scan to find usage stats.
// Intrinsic set copied from igcmc.
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
  Name = KM.getName();
  SLMSize = KM.getSLMSize();
  // FIXME: replace with 8k * simdSize * numDispatchedThreads
  if (KM.getFunction()->getParent()->getModuleFlag("genx.useGlobalMem"))
    StatelessPrivateMemSize = 16 * 8192;

}

void GenXOCLRuntimeInfo::KernelInfo::setArgumentProperties(
    const Function &Kernel, genx::KernelMetadata &KM) {
  IGC_ASSERT(Kernel.arg_size() == KM.getNumArgs() &&
         "Expected same number of arguments");
  // Some arguments are part of thread payload and do not require
  // entries in arguments info for OCL runtime.
  auto NonPayloadArgs =
      make_filter_range(Kernel.args(), [&KM](const Argument &Arg) {
        uint32_t ArgKind = KM.getArgKind(Arg.getArgNo());
        genx::KernelArgInfo KAI(ArgKind);
        return !(KAI.isLocalIDX() || KAI.isLocalIDY() || KAI.isLocalIDZ() ||
                 KAI.isGroupOrLocalSize() || KAI.isLocalIDs());
      });
  const DataLayout &DL = Kernel.getParent()->getDataLayout();
  transform(NonPayloadArgs, std::back_inserter(ArgInfos),
            [&KM, &DL](const Argument &Arg) {
              return KernelArgInfo{Arg, KM, DL};
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

  GRFSizeInBytes = ST.getGRFWidth();

  genx::KernelMetadata KM{FG.getHead()};
  IGC_ASSERT(KM.isKernel() && "Expected kernel as head of function group");
  setMetadataProperties(KM, ST);
  setArgumentProperties(*FG.getHead(), KM);
  setPrintStrings(*FG.getHead()->getParent());
}

//===----------------------------------------------------------------------===//
//
// Kernel argument info implementation.
//
//===----------------------------------------------------------------------===//
// Supported kernel argument attributes.
// Copied from igcmc.h.
struct OCLAttributes {
  static constexpr auto ReadOnly =
      "read_only"; // This resource is for read only.
  static constexpr auto WriteOnly =
      "write_only"; // This resource is for write only.
  static constexpr auto ReadWrite =
      "read_write"; // This resource is for read and write.
  static constexpr auto Buffer = "buffer_t";   // This resource is a buffer.
  static constexpr auto SVM = "svmptr_t";      // This resource is a SVM buffer.
  static constexpr auto Sampler = "sampler_t"; // This resource is a sampler.
  static constexpr auto Image1d = "image1d_t"; // This resource is a 1D surface.
  static constexpr auto Image1d_buffer = "image1d_buffer_t"; // This resource is a 1D surface.
  static constexpr auto Image2d = "image2d_t"; // This resource is a 2D surface.
  static constexpr auto Image3d = "image3d_t"; // This resource is a 3D surface.
};

using ArgKindType = GenXOCLRuntimeInfo::KernelArgInfo::KindType;

static auto GetStrPred = [](const char *Attr) {
  return [Attr](StringRef Token) { return Token == Attr; };
};

static ArgKindType getOCLArgKind(const SmallVectorImpl<StringRef> &Tokens,
                                 unsigned ArgNo, genx::KernelMetadata &KM) {
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

  // Explicit arguments.
  switch (KM.getArgCategory(ArgNo)) {
  default:
    return ArgKindType::General;
  case genx::RegCategory::GENERAL:
    if (any_of(Tokens, GetStrPred(OCLAttributes::SVM)))
      return ArgKindType::SVM;
    return ArgKindType::General;
  case genx::RegCategory::SURFACE:
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image1d)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image1d_buffer)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image2d)))
      return ArgKindType::Image2D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image3d)))
      return ArgKindType::Image3D;
    return ArgKindType::Buffer;
  case genx::RegCategory::SAMPLER:
    return ArgKindType::Sampler;
  }
}

using ArgAccessKindType = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;

static ArgAccessKindType
getOCLArgAccessKind(const SmallVectorImpl<StringRef> &Tokens,
                    ArgKindType Kind) {
  // As in igcmc.cpp.
  switch (Kind) {
  case ArgKindType::Buffer:
  case ArgKindType::Image1D:
  case ArgKindType::Image2D:
  case ArgKindType::Image3D:
  case ArgKindType::SVM:
    if (any_of(Tokens, GetStrPred(OCLAttributes::ReadOnly)))
      return ArgAccessKindType::ReadOnly;
    if (any_of(Tokens, GetStrPred(OCLAttributes::WriteOnly)))
      return ArgAccessKindType::WriteOnly;
    return ArgAccessKindType::ReadWrite;
  default:
    return ArgAccessKindType::None;
  }
}

// Initialize Kind and AccessKind from given ArgTypeDesc in metadata.
void GenXOCLRuntimeInfo::KernelArgInfo::translateArgDesc(
    genx::KernelMetadata &KM) {
  std::string Translated{KM.getArgTypeDesc(Index)};
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

  Kind = getOCLArgKind(Tokens, Index, KM);
  AccessKind = getOCLArgAccessKind(Tokens, Kind);
}

static unsigned getArgSizeInBytes(const Argument &Arg, genx::KernelMetadata &KM,
                                  const DataLayout &DL) {
  Type *ArgTy = Arg.getType();
  if (ArgTy->isPointerTy())
    return DL.getPointerTypeSize(ArgTy);
  if (KM.isBufferType(Arg.getArgNo()))
    return DL.getPointerSize();
  return ArgTy->getPrimitiveSizeInBits() / genx::ByteBits;
}

GenXOCLRuntimeInfo::KernelArgInfo::KernelArgInfo(const Argument &Arg,
                                                 genx::KernelMetadata &KM,
                                                 const DataLayout &DL)
    : Index(Arg.getArgNo()) {
  translateArgDesc(KM);
  Offset = KM.getArgOffset(Index);
  SizeInBytes = getArgSizeInBytes(Arg, KM, DL);
  BTI = KM.getBTI(Index);
}

//===----------------------------------------------------------------------===//
//
// Compiled kernel implementation.
//
//===----------------------------------------------------------------------===//
GenXOCLRuntimeInfo::CompiledKernel::CompiledKernel(KernelInfo &&KI,
                                                   const FINALIZER_INFO &JI,
                                                   const GTPinInfo &GI,
                                                   std::vector<char> GenBinIn,
                                                   std::vector<char> DbgInfoIn)
    : CompilerInfo(std::move(KI)), JitterInfo(JI),
      GtpinInfo(GI),
      GenBinary{std::move(GenBinIn)},
      DebugInfo{std::move(DbgInfoIn)} {
}

//===----------------------------------------------------------------------===//
//
// Runtime info pass implementation.
//
//===----------------------------------------------------------------------===//
namespace {

// SymbolTableBuilder: it's a helper class to generate symbol table.
// Collecting/constructing symbols and emitting structures
// with collected symbols are separated. Emitter part supports 2 formats:
// legacy (patch tokens), zebin.
class SymbolTableBuilder final {
  // Using ZE struct as it covers all that is required, e.g. uses std::string
  // instead of C char array.
  using SymbolsInfo = GenXOCLRuntimeInfo::ZEBinaryInfo::SymbolsInfo;
  using SymbolSeq = SymbolsInfo::ZESymEntrySeq;
  using SymbolT = vISA::ZESymEntry;
  using GenBinaryT = genx::BinaryDataAccumulator<const Function *>;

  const GenBinaryT &GenBinary;
  SymbolsInfo Symbols;

public:
  SymbolTableBuilder(const GenBinaryT &GenBinaryIn) : GenBinary{GenBinaryIn} {
    validateInitialData();
  }

  void constructFunctionSymbols() {
    // Skipping first section with the kernel.
    std::transform(std::next(GenBinary.begin()), GenBinary.end(),
                   std::back_inserter(Symbols.Functions),
                   [](const auto &Section) -> SymbolT {
                     return {vISA::GenSymType::S_FUNC,
                             static_cast<uint32_t>(Section.Info.Offset),
                             static_cast<uint32_t>(Section.Info.getSize()),
                             Section.Key->getName().str()};
                   });
  }

  // EnableZEBinary: this is requred by ZEBinary
  void constructLocalSymbols() {
    auto &KernelSection = getKernelSection();
    Symbols.Local.emplace_back(
        vISA::GenSymType::S_KERNEL, KernelSection.Info.Offset,
        KernelSection.Info.getSize(), KernelSection.Key->getName().str());
    // for now only kernel symbol is required
  }

  void constructAllSymbols() {
    constructFunctionSymbols();
    constructLocalSymbols();
  }

  GenXOCLRuntimeInfo::TableInfo emitLegacySymbolTable() const {
    GenXOCLRuntimeInfo::TableInfo TI;
    // Local symbols are ZE binary only, thus it is not part of the sum.
    TI.Entries = Symbols.Functions.size();
    TI.Size = TI.Entries * sizeof(vISA::GenSymEntry);
    // this will be eventually freed in AdaptorOCL
    auto *SymbolStorage = new vISA::GenSymEntry[TI.Entries];
    TI.Buffer = SymbolStorage;
    auto *Inserter = SymbolStorage;
    appendLegacySymbolTable(Symbols.Functions.begin(), Symbols.Functions.end(),
                            Inserter);
    return std::move(TI);
  }

  SymbolsInfo emitZESymbolTable() const & { return Symbols; }

  SymbolsInfo emitZESymbolTable() && { return std::move(Symbols); }

private:
  // appendLegacySymbolTable: a helper function to append symbols to a legasy
  // symbol table.
  // Output iterator is represented by a pointer \p OutIt, so the table/array
  // it points to has to have enough space.
  // The range [\p First, \p Last) must consist of SybolSeq::value elements.
  template <typename InputIter>
  static void appendLegacySymbolTable(InputIter First, InputIter Last,
                                      vISA::GenSymEntry *OutIt) {
    std::transform(
        First, Last, OutIt,
        [](SymbolSeq::const_reference SI) -> vISA::GenSymEntry {
          vISA::GenSymEntry Entry;
          Entry.s_offset = SI.s_offset;
          Entry.s_size = SI.s_size;
          Entry.s_type = SI.s_type;
          IGC_ASSERT_MESSAGE(SI.s_name.size() < vISA::MAX_SYMBOL_NAME_LENGTH,
                             "no solution for long symbol names for legacy "
                             "symbol info is yet provided");
          std::copy(SI.s_name.begin(), SI.s_name.end(), Entry.s_name);
          // Null-terminating the string.
          Entry.s_name[SI.s_name.size()] = '\0';
          return std::move(Entry);
        });
  }

  GenBinaryT::const_reference getKernelSection() const {
    return GenBinary.front();
  }

  void validateInitialData() const {
    IGC_ASSERT_MESSAGE(!GenBinary.empty(),
                       "The binary must contain a least the kernel");
    auto &KernelSection = getKernelSection();
    IGC_ASSERT_MESSAGE(
        KernelSection.Info.Offset == 0,
        "It's presumed that the kernel is at the front of the binary");
  }
};

} // namespace

// Appends the binary of function/kernel represented by \p Func and \p BuiltFunc
// to \p GenBinary.
static void
appendFuncBinary(genx::BinaryDataAccumulator<const Function *> &GenBinary,
                 const Function &Func, const VISAKernel &BuiltFunc) {
  void *GenBin = nullptr;
  int GenBinSize = 0;
  CISA_CALL(BuiltFunc.GetGenxBinary(GenBin, GenBinSize));
  IGC_ASSERT_MESSAGE(
      GenBin && GenBinSize,
      "Unexpected null buffer or zero-sized kernel (compilation failed?)");
  GenBinary.append(&Func, ArrayRef<char>{reinterpret_cast<char *>(GenBin),
                                         static_cast<size_t>(GenBinSize)});
  freeBlock(GenBin);
}

// Constructs gen binary for provided function group \p FG.
static genx::BinaryDataAccumulator<const Function *>
getGenBinary(const FunctionGroup &FG, VISABuilder &VB) {
  genx::BinaryDataAccumulator<const Function *> GenBinary;
  VISAKernel *BuiltKernel = VB.GetVISAKernel(FG.getHead()->getName().str());
  appendFuncBinary(GenBinary, *FG.getHead(), *BuiltKernel);
  for (Function *F : FG) {
    if (F->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly)) {
      VISAKernel *ExtKernel = VB.GetVISAKernel(F->getName().str());
      IGC_ASSERT_MESSAGE(ExtKernel, "Kernel is null");
      appendFuncBinary(GenBinary, *F, *ExtKernel);
    }
  }
  return std::move(GenBinary);
}

namespace {

class RuntimeInfoCollector final {
  const FunctionGroupAnalysis &FGA;
  const GenXBackendConfig &BC;
  VISABuilder &VB;
  const GenXSubtarget &ST;
  const Module &M;
  const GenXDebugInfo &DBG;

public:
  using KernelStorageTy = GenXOCLRuntimeInfo::KernelStorageTy;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;

public:
  RuntimeInfoCollector(const FunctionGroupAnalysis &InFGA,
                       const GenXBackendConfig &InBC, VISABuilder &InVB,
                       const GenXSubtarget &InST, const Module &InM,
                       const GenXDebugInfo &InDbg)
      : FGA{InFGA}, BC{InBC}, VB{InVB}, ST{InST}, M{InM}, DBG{InDbg} {}

  KernelStorageTy run() &&;

private:
  CompiledKernel collectFunctionGroupInfo(const FunctionGroup &FG) const;
};

} // namespace

RuntimeInfoCollector::KernelStorageTy RuntimeInfoCollector::run() && {
  KernelStorageTy Kernels;
  std::transform(FGA.begin(), FGA.end(), std::back_inserter(Kernels),
                 [this](const FunctionGroup *FG) {
                   return collectFunctionGroupInfo(*FG);
                 });
  return std::move(Kernels);
}

RuntimeInfoCollector::CompiledKernel
RuntimeInfoCollector::collectFunctionGroupInfo(const FunctionGroup &FG) const {
  using KernelInfo = GenXOCLRuntimeInfo::KernelInfo;
  using GTPinInfo = GenXOCLRuntimeInfo::GTPinInfo;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;
  using TableInfo = GenXOCLRuntimeInfo::TableInfo;

  // Compiler info.
  KernelInfo Info{FG, ST, BC};

  const Function *KernelFunction = FG.getHead();
  const std::string KernelName = KernelFunction->getName().str();
  VISAKernel *VK = VB.GetVISAKernel(KernelName);
  IGC_ASSERT_MESSAGE(VK, "Kernel is null");
  FINALIZER_INFO *JitInfo = nullptr;
  CISA_CALL(VK->GetJitInfo(JitInfo));
  IGC_ASSERT_MESSAGE(JitInfo, "Jit info is not set by finalizer");
  genx::BinaryDataAccumulator<const Function *> GenBinary =
      getGenBinary(FG, VB);

  const auto& Dbg = DBG.getModuleDebug();
  auto DbgIt = Dbg.find(KernelFunction);
  std::vector<char> DebugData;
  if (DbgIt != std::end(Dbg)) {
    const auto &ElfImage = DbgIt->second;
    DebugData = {ElfImage.begin(), ElfImage.end()};
  }
  TableInfo &RTable = Info.getRelocationTable();
  CISA_CALL(
      VK->GetGenRelocEntryBuffer(RTable.Buffer, RTable.Size, RTable.Entries));
  // EnableZEBinary: this is requred by ZEBinary
  CISA_CALL(VK->GetRelocations(Info.ZEBinInfo.Relocations));
  SymbolTableBuilder STBuilder{GenBinary};
  STBuilder.constructAllSymbols();
  Info.getSymbolTable() = STBuilder.emitLegacySymbolTable();
  Info.ZEBinInfo.Symbols = std::move(STBuilder).emitZESymbolTable();

  void *GTPinBuffer = nullptr;
  unsigned GTPinBufferSize = 0;
  CISA_CALL(VK->GetGTPinBuffer(GTPinBuffer, GTPinBufferSize));

  auto *GTPinBytes = static_cast<char *>(GTPinBuffer);
  GTPinInfo gtpin{{GTPinBytes, GTPinBytes + GTPinBufferSize}};

  return CompiledKernel{std::move(Info), *JitInfo, std::move(gtpin),
                        std::move(GenBinary).emitConsolidatedData(),
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

  Kernels = RuntimeInfoCollector{FGA, BC, VB, ST, M, DBG}.run();
  return false;
}

INITIALIZE_PASS_BEGIN(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                      "GenXOCLRuntimeInfo", false, true)
INITIALIZE_PASS_END(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                    "GenXOCLRuntimeInfo", false, true)
