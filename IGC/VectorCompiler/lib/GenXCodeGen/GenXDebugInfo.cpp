/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FunctionGroup.h"

#include "GenXDebugInfo.h"
#include "GenXTargetMachine.h"
#include "GenXVisaRegAlloc.h"

#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/BackendConfig.h"

#include "visa/include/visaBuilder_interface.h"

#include "DebugInfo/StreamEmitter.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>

#include "Probe/Assertion.h"

//
/// GenXDebugInfo
/// -------------
///
/// The goal of the pass is to provide debug information for each generated
/// genisa instruction (if such information is available).  The debug
/// information is encoded in DWARF format.
///
/// Ultimately, the pass gets data from 2 sources:
///
///   1. LLVM debug information encoded in LLVM IR itself. It captures the
///   important pieces of the source language's Abstract Syntax Tree and
///   maps it onto LLVM code.
///   LLVM framework should maintain it automatically, given that we follow
///   relatively simple rules while designing IR transformations:
///      https://llvm.org/docs/HowToUpdateDebugInfo.html
///
///   2. Debug information obtained from the finalizer. This information is
///   encoded in some proprietary format (blob) and contains the following:
///     a. mapping between vISA and genISA instructions
///     b. live intervals of the virtual registers, information about spilled
///     values, etc.
///     c. call frame information
///
/// The pass feeds the above information to the DebugInfo library which in turn
/// produces the final DWARF.
///
/// Operation of the pass
/// ^^^^^^^^^^^^^^^^^^^^^
///
/// The pass assumes that some data is already being made available by other
/// passes/analysis.
///
/// * FunctionGroupAnalysis:
///     provides information about the overall "structure"
///     of the program: functions, stack calls, indirect calls, subroutines and
///     relationships.
///
/// * GenXModule:
///     1. for each LLVM Function provides information about
///        LLVM instruction -> vISA instructions mapping. This information is
///        produced/maintained during operation of CISABuilder pass.
///     2. for each LLVM Function provides access to a corresponding
///      *VISAKernel* object.
///
/// * GenXVisaRegAlloc:
///     provides the mapping between LLVM values and virtual registers.
///
/// * GenXCisaBuilder:
///     provides access to VISABuilder, which allows us to have access to
///     VISAKernel objects (some Functions from LLVM IR, like the ones
///     representing kernel spawns these) that contain:
///         a. debug information maintained by finalizer (see above)
///         b. the respected gen binaries
///
/// Data Structures
/// ^^^^^^^^^^^^^^^
///
/// Since data is aggregated from different sources, some extra data structures
/// are used to simplify bookkeeping.
///
/// - *genx::di::VisaMapping*
///   provides the mapping from LLMV IR instruction to vISA instruction index,
///   that represents the first vISA instruction spawned by the LLVM IR
///   instruction. A single LLVM IR instruction can spawn several
///   vISA instructions - currently the number of spawned instructions is
///   derived implicitly (which is not always correct but works in most of the
///   cases).
///
/// - *ProgramInfo*
///   A transient object that groups several llvm Functions that are eventually
///   get compiled into a single gen entity. A separate elf file with the
///   debug information is generated for each gen entity.
///
///   The grouping is done as follows:
///   - We piggyback on FunctionGroup analysis. Each kernel function becomes the
///   head of the group. Different FunctionGroups always result in different
///   *ProgramInfo* objects. However, a single FunctionGroup can be split even
///   further. This can happen if we have an indirect call to some function. In
///   this case, this function shall is compiled into a separate gen object
///   (and a separate VISAKernel is produced aswell).
///
///   The above approach does not work correctly in all cases. See
///   *KNOWN ISSUES* section.
///
/// - *CompiledVisaWrapper*
///  For an arbitrary pair of llvm IR Function and VISAKernel objects,
///  does the following:
///     + Validates that IR Function and VISAKernel object are related (that is
///       the vISA spawned by IR Function is owned by the VISAKernel.
///     + Extracts Gen Binary.
///     + Extracts Debug Info Blob from finalizer and decodes it.
///
/// *GenXFunction*
///  An object that loosely resembles MachineFunctoin from the LLVM Machine IR.
///  This is an object that for a given LLVM IR Function can access to:
///     - LLVM IR Function
///     - VisaMapping
///     - Subtarget
///     - CompiledVisaWrapper
///     - GenXVisaRegAlloc
///  GenXFunctoin serves as a primary method to communicate with the DebugInfo
///  library. The data these objects hold allow us to reason about the debug
///  information for any Gen construct (instruction, variable, etc).
///
/// Examples
/// ^^^^^^^^
///
/// Examples below use the following naming conventions:
///     K* - kernel function
///     L* - subroutine (non-inlined function)
///     S* - simple stack call
///     I* - indirectly-called function
///
/// FunctionGroup construction peculiarities.
///
///   When function groups are constructed, we do some peculiar transformations.
///
///    Case_1 (FG):
///         Source Code: { K1 calls L1, K2 calls L1 }
///         IR after function groups: { G1 = {K1, L1}, G2 = { K2, L1'} },
///             where L1' is a clone of L1.
///    Case_2 (FG):
///         Source Code: { K1 calls S_1, both call L1 }.
///         IR after function groups: { G1 = {K1, L1, S1, L1' } }.
///    Case_3 (FG):
///         Source Code: { K1 calls I1 and I2 }.
///         IR after function grups { G1 = {K1}, G2 = {I1}, G3={I2} }.
///
/// VISA/genISA  construction peculiarities.
///
///   Case 1:
///     Source code: K1, K2.
///     Compilation phase:
///         two function groups are created, K1 and K2 are heads.
///         two different VISAKernel produced.
///     DebugInfoGeneration:
///         Decoded Debug info for each VISAKernel contains:
///           one compiled object description.
///           two "*.elf" files are created.
///
///   Case 2:
///     Source code: K1, S1. K1 calls S1.
///     Compilation phase:
///         1 function group is created, K1 is the head.
///         1 VISAKernel and 1 VISAFunction are created.
///     DebugInfoGeneratation:
///         Decoded debug info contains *2* compiled objects.
///         Each object has separate vISA indexes - visa instructions are
///         counted separately. Still, both are compiled into the same gen
///         object, so only one "*.elf" file is emitted.
///
///   Case 3:
///     Source code: K1, I1. K1 calls I1
///     Compilation phase:
///         1 function group is created, K1 is the head.
///         Somehow 2 VISAKernels are created.
///     DebugInfoGeneratation:
///         Decoded debug info contains *1* compiled objects (but we have 2
///         VISAKernel).
///         In the end, we emit two "*.elf" files.
///
/// KNOWN ISSUES
/// ^^^^^^^^^^^^
///
/// Note: see the "Examples" section for the description of the used naming
/// convention.
///
///   Case 1: (debug info can't be emitted)
///     Source code: *K1*, *L1* *K1* calls *L1*.
///     Compilation phase:
///         1 function group is created.
///         1 VISAKernel produced.
///     DebugInfoGeneration:
///       1 *ProgramInfo* created { K1, L1}.
///       Decoded Debug info contains 1 compiled object, that has 1 subroutines.
///
///   Problem: way to map LLVM Function onto subroutine is not implemented.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_DEBUG_INFO"

using namespace llvm;

std::vector<llvm::DISubprogram*> gatherDISubprogramNodes(llvm::Module& M);

namespace {

static void debugDump(const Twine &Name, const char *Content, size_t Size) {
  std::error_code EC;
  // no error handling since this is debug output
  raw_fd_ostream OS(Name.str(), EC);
  OS << StringRef(Content, Size);
}

class CompiledVisaWrapper {

  using FinalizedDI = IGC::DbgDecoder::DbgInfoFormat;

  std::vector<char> GenBinary;
  std::vector<char> DbgInfoBlob;
  std::unique_ptr<IGC::DbgDecoder> DecodedDebugInfo;

  FINALIZER_INFO *JitInfo = nullptr;
  // underlying data is owned by DecodedDebugInfo
  const FinalizedDI *VisaKernelDI = nullptr;

  std::string ErrMsg;

public:
  const FINALIZER_INFO &getJitInfo() const {
    IGC_ASSERT(ErrMsg.empty() && JitInfo);
    return *JitInfo;
  };
  const FinalizedDI &getFinalizerDI() const {
    IGC_ASSERT(ErrMsg.empty() && VisaKernelDI);
    return *VisaKernelDI;
  }
  IGC::DbgDecoder *getDIDecoder() const { return DecodedDebugInfo.get(); }
  const std::vector<char> &getGenBinary() const { return GenBinary; }
  const std::vector<char> &getDbgInfoBlob() const { return DbgInfoBlob; }
  const std::string &getError() const { return ErrMsg; }
  bool hasErrors() const { return !getError().empty(); }

  struct Gen2VisaIdx {
    unsigned GenOffset;
    unsigned VisaIdx;
  };

  void releaseDebugInfoResources(const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    unsigned int DbgSize = 0;
    auto Result = VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize);
    IGC_ASSERT_MESSAGE(Result == 0,
                       "could not get debug blob during cleanup procedure");
    IGC_ASSERT(GenXdbgInfo);
    freeBlock(GenXdbgInfo);
  }

  CompiledVisaWrapper(const Function &F, const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    unsigned int DbgSize = 0;
    if (VK.GetJitInfo(JitInfo) != 0) {
      ErrMsg = "could not extract jitter info";
      return;
    }
    IGC_ASSERT(JitInfo);

    if (VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize) != 0) {
      ErrMsg = "visa info decode error";
      return;
    }

    if (!GenXdbgInfo) {
      ErrMsg = "could not get debug information from finalizer";
      return;
    }

    const char *DbgBlobBytes = static_cast<const char *>(GenXdbgInfo);
    DbgInfoBlob = std::vector<char>(DbgBlobBytes, DbgBlobBytes + DbgSize);

    DecodedDebugInfo = std::make_unique<IGC::DbgDecoder>(DbgInfoBlob.data());
    auto GetDebugInfoForKernel = [this](StringRef KernelName) {
      const auto &CO = DecodedDebugInfo->compiledObjs;
      auto FoundIt =
          std::find_if(CO.begin(), CO.end(), [&KernelName](const auto &DI) {
            return KernelName == StringRef(DI.kernelName);
          });
      const IGC::DbgDecoder::DbgInfoFormat *Result = nullptr;
      if (FoundIt != CO.end())
        Result = &*FoundIt;
      return Result;
    };
    VisaKernelDI = GetDebugInfoForKernel(F.getName());
    if (!VisaKernelDI) {
      ErrMsg = "could not find debug information for <" +
               std::string(F.getName()) + ">";
      return;
    }

    LLVM_DEBUG(VisaKernelDI->dump(); dbgs() << "\n";);

    if (VisaKernelDI->CISAIndexMap.empty()) {
      ErrMsg = "empty CisaIndexMap for <" + std::string(F.getName()) + ">";
      return;
    }

    std::vector<Gen2VisaIdx> Gen2Visa;
    std::transform(VisaKernelDI->CISAIndexMap.begin(),
                   VisaKernelDI->CISAIndexMap.end(),
                   std::back_inserter(Gen2Visa), [](const auto &V2G) {
                     return Gen2VisaIdx{V2G.second, V2G.first};
                   });

    // Extract Gen Binary (will need it for line table generation)
    void *GenBin = nullptr;
    int GenBinSize = 0; // Finalizer uses signed int for size...
    VK.GetGenxBinary(GenBin, GenBinSize);
    IGC_ASSERT(GenBinSize >= 0);
    const auto *GenBinBytes = reinterpret_cast<const char *>(GenBin);
    GenBinary.insert(GenBinary.end(), GenBinBytes, GenBinBytes + GenBinSize);

    // Make Sure that gen isa indeces are inside GenBinary
    const bool InBounds =
        std::all_of(Gen2Visa.begin(), Gen2Visa.end(), [&](const auto &Idx) {
          // <= Is because last index can be equal to the binary size
          return Idx.GenOffset <= GenBinary.size();
        });
    if (!InBounds) {
      ErrMsg = "fatal error (debug info). inconsistent gen->visa mapping: "
               "gen index is out of bounds";
      return;
    }

    // Make Sure that gen isa indeces are unique and sorted
    const bool Sorted = std::is_sorted(
        Gen2Visa.begin(), Gen2Visa.end(),
        [](const auto &L, const auto &R) { return L.GenOffset < R.GenOffset; });
    const bool Validated =
        Sorted && (Gen2Visa.end() ==
                   std::adjacent_find(Gen2Visa.begin(), Gen2Visa.end(),
                                      [](const auto &L, const auto &R) {
                                        return L.GenOffset == R.GenOffset;
                                      }));
    if (!Validated) {
      ErrMsg = "fatal error (debug info). inconsistent gen->visa mapping: "
               "gen index are not ordered properly";
      return;
    }
  }
};

class GenXFunction final : public IGC::VISAModule {

public:
  GenXFunction(const GenXSubtarget &STIn, const GenXVisaRegAlloc &RAIn,
               const Function &F, const CompiledVisaWrapper &CW,
               const genx::di::VisaMapping &V2I)
      : F{F}, ST{STIn}, VisaMapping{V2I}, CompiledVisa{CW}, RA{RAIn},
        VISAModule(const_cast<Function *>(&F)) {

    isDirectElfInput = true;
  }

  unsigned int getUnpaddedProgramSize() const override {
    return CompiledVisa.getGenBinary().size();
  }
  bool isLineTableOnly() const override {
    IGC_ASSERT_MESSAGE(0, "isLineTableOnly()");
    return false;
  }
  unsigned getPrivateBaseReg() const override {
    IGC_ASSERT_MESSAGE(0, "getPrivateBaseReg() - not implemented");
    return 0;
  }
  unsigned getGRFSize() const override {
    return ST.getGRFWidth();
  }
  unsigned getNumGRFs() const override {
    return CompiledVisa.getJitInfo().numGRFTotal;
  }
  unsigned getPointerSize() const override {
    return F.getParent()->getDataLayout().getPointerSize();
  }
  ArrayRef<char> getGenDebug() const override {
    return CompiledVisa.getDbgInfoBlob();
  }
  ArrayRef<char> getGenBinary() const override {
    return CompiledVisa.getGenBinary();
  }
  IGC::DbgDecoder *getDIDecoder() const { return CompiledVisa.getDIDecoder(); }
  const IGC::DbgDecoder::DbgInfoFormat &getFinalizerDI() const {
    return CompiledVisa.getFinalizerDI();
  }

  const genx::di::VisaMapping &getVisaMapping() const { return VisaMapping; }
  std::vector<IGC::VISAVariableLocation>
  GetVariableLocation(const Instruction *DbgInst) const override {

    using Location = IGC::VISAVariableLocation;
    auto EmptyLoc = [this](StringRef Reason) {
      LLVM_DEBUG(dbgs() << "  Empty Location Returned (" << Reason
                        << ")\n <<<\n");
      std::vector<Location> Res;
      Res.emplace_back(this);
      return Res;
    };
    auto ConstantLoc = [this](const Constant *C) {
      LLVM_DEBUG(dbgs() << "  ConstantLoc\n <<<\n");
      std::vector<Location> Res;
      Res.emplace_back(C, this);
      return Res;
    };

    IGC_ASSERT(isa<DbgInfoIntrinsic>(DbgInst));

    LLVM_DEBUG(dbgs() << " >>>\n  GetVariableLocation for " << *DbgInst
                      << "\n");
    const Value *DbgValue = nullptr;
    DIVariable *VarDescr = nullptr;
    if (auto *pDbgAddrInst = dyn_cast<DbgDeclareInst>(DbgInst)) {
      DbgValue = pDbgAddrInst->getAddress();
      VarDescr = pDbgAddrInst->getVariable();
      return EmptyLoc("llvm.dbg.declare is not supported");
    } else if (auto *pDbgValInst = dyn_cast<DbgValueInst>(DbgInst)) {
      DbgValue = pDbgValInst->getValue();
      VarDescr = pDbgValInst->getVariable();
    } else {
      return EmptyLoc("Unsupported Debug Intrinsic");
    }
    IGC_ASSERT(DbgValue);
    IGC_ASSERT(VarDescr);
    LLVM_DEBUG(dbgs() << "   Value:" << *DbgValue << "\n");
    LLVM_DEBUG(dbgs() << "   Var: " << VarDescr->getName()
                      << "/Type:" << *VarDescr->getType() << "\n");
    if (isa<UndefValue>(DbgValue)) {
      return EmptyLoc("UndefValue");
    }
    if (auto *ConstVal = dyn_cast<Constant>(DbgValue)) {
      return ConstantLoc(ConstVal);
    }
    auto *Reg = RA.getRegForValueUntyped(&F, const_cast<Value *>(DbgValue));
    if (!Reg) {
      return EmptyLoc("   could not find virtual register");
    }
    const bool IsRegister = true;
    const bool IsMemory = false;
    const bool IsGlobalASI = false;
    auto *VTy = dyn_cast<VectorType>(DbgValue->getType());
    unsigned NumElements = VTy ? VTy->getNumElements() : 1;
    const bool IsVectorized = false;

    std::vector<Location> Res;
    // Source/IGC/DebugInfo/VISAModule.hpp:128
    Res.emplace_back(GENERAL_REGISTER_BEGIN + Reg->Num, IsRegister,
                     IsMemory, NumElements, IsVectorized, IsGlobalASI, this);
    return Res;
  }

  void UpdateVisaId() override {
    // do nothing (the moment we need to advance index is controlled explicitly)
  }
  void ValidateVisaId() override {
    // do nothing (we don't need validation since VISA is built already)
  }
  uint16_t GetSIMDSize() const override { return 1; }

  void* getPrivateBase() const override { return nullptr; };
  void setPrivateBase(void*) override {};

  bool hasPTO() const override { return false; }
  int getPTOReg() const override { return -1; }
  int getFPReg() const override { return -1; }
  uint64_t getFPOffset() const override { return 16; }

private:
  const Function &F;
  const GenXSubtarget &ST;
  const genx::di::VisaMapping &VisaMapping;
  const CompiledVisaWrapper &CompiledVisa;
  const GenXVisaRegAlloc &RA;
};

void processGenXFunction(IGC::IDebugEmitter *Emitter, GenXFunction *GF) {
  Emitter->setCurrentVISA(GF);
  const auto &V2I = GF->getVisaMapping().V2I;
  const auto &FDI = GF->getFinalizerDI();
  for (auto MappingIt = V2I.cbegin(); MappingIt != V2I.cend(); ++MappingIt) {

    // "NextIndex" is an index in vISA stream which points to an end
    // of instructions sequence generated by a particular llvm instruction
    // For istructions which do not produce any visa instructions
    // (like llvm.dbg.*) "NextIndex" should point to the "CurrentIndex"
    auto FindNextIndex = [&FDI, &V2I](decltype(MappingIt) ItCur) {
      auto *Inst = ItCur->Inst;
      if (isa<DbgInfoIntrinsic>(Inst)) {
        return ItCur->VisaIdx;
      }
      auto NextIt = std::next(ItCur);
      if (NextIt == V2I.end()) {
        IGC_ASSERT(!FDI.CISAIndexMap.empty());
        return FDI.CISAIndexMap.back().first;
      }
      return NextIt->VisaIdx;
    };
    auto VisaIndexCurr = MappingIt->VisaIdx;
    auto VisaIndexNext = FindNextIndex(MappingIt);

    // Note: "index - 1" is because we mimic index values as if they were
    // before corresponding instructions were inserted
    GF->SetVISAId(VisaIndexCurr - 1);
    // we need this const_cast because of the flawed VISA Emitter API
    auto *Inst = const_cast<Instruction *>(MappingIt->Inst);
    Emitter->BeginInstruction(Inst);
    GF->SetVISAId(VisaIndexNext - 1);
    Emitter->EndInstruction(Inst);

    LLVM_DEBUG(dbgs() << "  VisaMapping: [" << VisaIndexCurr << ";"
                      << VisaIndexNext << "):" << *Inst << "\n");
  }
}

} // namespace

namespace llvm {

void GenXDebugInfo::processKernel(const ProgramInfo &PI) {

  IGC_ASSERT_MESSAGE(!PI.FIs.empty(),
                     "Program must include at least one function");

  IGC::DebugEmitterOpts DebugOpts;
  DebugOpts.DebugEnabled = true;
  DebugOpts.isDirectElf = true;
  DebugOpts.UseNewRegisterEncoding = true;

  auto Deleter = [](IGC::IDebugEmitter *Emitter) {
    IGC::IDebugEmitter::Release(Emitter);
  };
  using EmitterHolder = std::unique_ptr<IGC::IDebugEmitter, decltype(Deleter)>;
  EmitterHolder Emitter(IGC::IDebugEmitter::Create(), Deleter);

  using CompiledVisaWrappers =
      std::vector<std::unique_ptr<CompiledVisaWrapper>>;
  CompiledVisaWrappers CWs;
  std::transform(PI.FIs.begin(), PI.FIs.end(), std::back_inserter(CWs),
                 [](const auto &FI) {
                   return std::make_unique<CompiledVisaWrapper>(
                       FI.F, FI.CompiledKernel);
                 });
  auto FaultyCwIt = std::find_if(
      CWs.begin(), CWs.end(), [](const auto &CW) { return CW->hasErrors(); });
  if (FaultyCwIt != CWs.end())
    report_fatal_error((*FaultyCwIt)->getError(), false);

  const auto &ST = getAnalysis<TargetPassConfig>()
      .getTM<GenXTargetMachine>()
      .getGenXSubtarget();
  auto &RA = getAnalysis<GenXVisaRegAlloc>();

  auto PrepareEmitter =
      [&Emitter](const GenXVisaRegAlloc &RA, const GenXSubtarget &ST,
                 const IGC::DebugEmitterOpts &DebugOpts,
                 CompiledVisaWrappers &CWs, const ProgramInfo &PI) {
        using GenXFunctionList = std::vector<GenXFunction *>;
        GenXFunctionList GFs;

        IGC_ASSERT(CWs.size() == PI.FIs.size());
        for (auto &&[FI, CW] : llvm::zip(PI.FIs, CWs)) {
          auto GF =
              std::make_unique<GenXFunction>(ST, RA, FI.F, *CW, FI.VisaMapping);
          GFs.push_back(GF.get());
          if (&FI.F == &PI.FIs.front().F) {
            Emitter->Initialize(std::move(GF), DebugOpts);
          } else {
            Emitter->registerVISA(GF.get());
            Emitter->resetModule(std::move(GF));
          }
        }
        // Currently Debug Info Emitter expects that GenXFunctions are
        // processed in the same order as they appear in the visa object
        // (in terms of genisa instructions order)
        std::sort(GFs.begin(), GFs.end(), [](auto *LGF, auto *RGF) {
          const auto &LDI = LGF->getFinalizerDI();
          const auto &RDI = RGF->getFinalizerDI();
          return LDI.relocOffset < RDI.relocOffset;
        });
        return GFs;
      };

  auto &KF = PI.FIs.front().F;
  IGC_ASSERT(ElfOutputs.count(&KF) == 0);
  auto &ElfBin = ElfOutputs[&KF];

  std::vector<GenXFunction *> GenXFunctions =
      PrepareEmitter(RA, ST, DebugOpts, CWs, PI);
  for (auto *GF : GenXFunctions) {
    LLVM_DEBUG(dbgs() << "--- Processing GenXFunction:  "
                      << GF->getFunction()->getName().str() << " ---\n");
    processGenXFunction(Emitter.get(), GF);
    bool ExpectMore = GF != GenXFunctions.back();
    LLVM_DEBUG(dbgs() << "--- Starting Debug Info Finalization (final:  "
                      << !ExpectMore << ") ---\n");
    auto Out = Emitter->Finalize(!ExpectMore, GF->getDIDecoder(),
                                 DISubprogramNodes);
    if (!ExpectMore) {
      ElfBin = std::move(Out);
    } else {
      IGC_ASSERT(Out.empty());
    }
    LLVM_DEBUG(dbgs() << "---     \\ Debug Info Finalized /     ---\n");
  }

  const auto &KernelName = KF.getName();
  LLVM_DEBUG(dbgs() << "got Debug Info for <" << KernelName.str() << "> "
                    << "- " << ElfBin.size() << " bytes\n");

  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (BC.dbgInfoDumpsEnabled()) {

    std::string NamePrefix = "dbginfo_";
    if (!BC.dbgInfoDumpsNameOverride().empty())
      NamePrefix.append(BC.dbgInfoDumpsNameOverride()).append("_");

    auto DwarfDumpName = (NamePrefix + KernelName + "_dwarf.elf").str();
    auto GendbgDumpName = (NamePrefix + KernelName + "_gen.dump").str();
    const auto &GenDbgBlob = GenXFunctions.front()->getGenDebug();
    if (BC.hasShaderDumper()) {
      BC.getShaderDumper().dumpBinary(ElfBin, DwarfDumpName);
      BC.getShaderDumper().dumpBinary(GenDbgBlob, GendbgDumpName);
    } else {
      debugDump(DwarfDumpName, ElfBin.data(), ElfBin.size());
      debugDump(GendbgDumpName, GenDbgBlob.data(), GenDbgBlob.size());
    }
  }

  // this reset is needed to gracefully cleanup resources held by CWs
  GenXFunctions.clear();
  Emitter.reset();
  CWs.front()->releaseDebugInfoResources(PI.FIs.front().CompiledKernel);
  CWs.clear();

  return;
}

void GenXDebugInfo::cleanup() { ElfOutputs.clear(); }

void GenXDebugInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXVisaRegAlloc>();
  AU.setPreservesAll();
}

void GenXDebugInfo::processFunctionGroup(GenXModule &GM, VISABuilder &VB,
                                         const FunctionGroup &FG) {
  const auto *KF = FG.getHead();
  VISAKernel *VKEntry = VB.GetVISAKernel(KF->getName().str());
  IGC_ASSERT(VKEntry);
  LLVM_DEBUG(dbgs() << "DbgInfo: processing <" << KF->getName() << ">\n");

  auto BuildFunctionInfo = [&GM](VISAKernel *VF, Function *F) {
    const auto &Mapping = *GM.getVisaMapping(F);
    return ProgramInfo::FunctionInfo{Mapping, *VF, *F};
  };
  // Currently, llvm Function can produce vISA which is incorporated in
  // the main vISA object or in case of vISA-external functions - it can spawn
  // a completely new vISA object.
  // Thus, to create debug info, we split each function group into
  // the set of "primary" and "indirectly-called" functions
  std::vector<Function *> PrimaryFunctions, IndirectlyCalledFunctions;
  std::partition_copy(
      FG.begin(), FG.end(), std::back_inserter(IndirectlyCalledFunctions),
      std::back_inserter(PrimaryFunctions), [](Function *F) {
        return F->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly);
      });
  for (auto *F : IndirectlyCalledFunctions) {
    LLVM_DEBUG(dbgs() << "  F: " << F->getName().str() << " called indirectly!\n");
    // Each indirectly-called function is compiled into a separate vISA kernel
    auto *VF = VB.GetVISAKernel(F->getName().str());
    processKernel(ProgramInfo{{BuildFunctionInfo(VF, F)}});
  }
  std::vector<ProgramInfo::FunctionInfo> PrimaryFIs;
  std::transform(PrimaryFunctions.begin(), PrimaryFunctions.end(),
                 std::back_inserter(PrimaryFIs),
                 [&VKEntry, &BuildFunctionInfo](auto *F) {
                   return BuildFunctionInfo(VKEntry, F);
                 });
  LLVM_DEBUG({
    dbgs() << " - main kernel structure: ";
    for (const auto *F : PrimaryFunctions)
      dbgs() << F->getName() << ",";
    dbgs() << "\n";
  });
  processKernel(ProgramInfo{std::move(PrimaryFIs)});
}
bool GenXDebugInfo::runOnModule(Module &M) {

  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (!BC.emitDebugInformation())
    return false;

  const FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
  auto &GM = getAnalysis<GenXModule>();

  DISubprogramNodes = gatherDISubprogramNodes(M);

  VISABuilder *VB = GM.GetCisaBuilder();
  if (GM.HasInlineAsm())
    VB = GM.GetVISAAsmReader();

  for (const auto *FG : FGA)
    processFunctionGroup(GM, *VB, *FG);

  return false;
}

char GenXDebugInfo::ID = 0;

ModulePass *createGenXDebugInfoPass() {
  initializeGenXDebugInfoPass(*PassRegistry::getPassRegistry());
  return new GenXDebugInfo;
}

} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                      true /*analysis*/)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(GenXModule)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GenXVisaRegAlloc)
INITIALIZE_PASS_END(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                    true /*analysis*/)
