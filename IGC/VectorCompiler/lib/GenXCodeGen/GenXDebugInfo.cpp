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

#include <llvm/Analysis/CallGraph.h>
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

#include <unordered_set>

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


static cl::opt<std::string> DbgOpt_VisaTransformInfoPath(
    "vc-dump-module-to-visa-transform-info-path", cl::init(""), cl::Hidden,
    cl::desc("filename into which MVTI is dumped"));

static void debugDump(const Twine &Name, const char *Content, size_t Size) {
  std::error_code EC;
  // no error handling since this is debug output
  raw_fd_ostream OS(Name.str(), EC);
  OS << StringRef(Content, Size);
}

template <typename ContainerT>
using EmplaceTy = decltype(std::declval<ContainerT>().emplace());

template <typename ContainerT>
using CheckedEmplace = decltype(std::declval<EmplaceTy<ContainerT>>().second);

template <typename ContainerT>
using IsCheckedEmplace = std::is_same<bool, CheckedEmplace<ContainerT>>;

template <typename ContainerT>
using IsNonCheckedEmplace =
    std::is_same<EmplaceTy<ContainerT>, typename ContainerT::iterator>;

// Naive function that checks the presence of copies.
// Container must be multimap-like, values must be comparable.
template <typename ContainerT>
static bool hasCopy(const ContainerT &Container,
                    typename ContainerT::iterator ToCheck) {
  auto Range = Container.equal_range(ToCheck->first);
  auto Result = std::count_if(Range.first, Range.second, [ToCheck](auto It) {
    return It.second == ToCheck->second;
  });

  return Result > 1;
}

// checkedEmplace for multimap-like containers. It will be called if
// Container.emplace() returns Container::iterator. For such containers, emplace
// will always happen and therefore copies can be silently inserted.
template <typename ContainerT, class... ArgsT>
static std::enable_if_t<IsNonCheckedEmplace<ContainerT>::value, void>
checkedEmplace(ContainerT &Container, ArgsT &&... Args) {
  auto Result = Container.emplace(std::forward<ArgsT>(Args)...);
  IGC_ASSERT_MESSAGE(!hasCopy(Container, Result),
                     "a copy of the existing element was emplaced");
  (void)Result;
}

// checkedEmplace for map/set-like containers. If Container.emplace() returns a
// pair whose second element has bool type, this version will be called.
template <typename ContainerT, class... ArgsT>
static std::enable_if_t<IsCheckedEmplace<ContainerT>::value, void>
checkedEmplace(ContainerT &Container, ArgsT &&... Args) {
  auto Result = Container.emplace(std::forward<ArgsT>(Args)...);
  IGC_ASSERT_MESSAGE(Result.second, "unexpected insertion failure");
  (void)Result;
}

static bool compareFunctionNames(const Function *LF, const Function *RF) {
  IGC_ASSERT(LF && RF);
  return LF->getName() > RF->getName();
}

template <typename ContainerT>
static std::vector<const Function *>
extractSortedFunctions(const ContainerT &C) {
  std::vector<const Function *> Result;
  std::transform(C.begin(), C.end(), std::back_inserter(Result),
                 [](const auto &It) { return It.first; });
  std::sort(Result.begin(), Result.end(), compareFunctionNames);
  return Result;
}

//
// ModuleToVisaTransformInfo
// Proides information about how LLVM IR functions are mapped onto various
// vISA (and genISA) objects.
class ModuleToVisaTransformInfo {
  using FunctionMapping =
      std::unordered_map<const Function *, const Function *>;
  using FunctionMultiMapping =
      std::unordered_multimap<const Function *, const Function *>;
  // Note: pointer to VISAKernel can represent either a true kernel or
  // VISAFunction, depending on the context (this is vISA API limitation)
  using FunctionToVisaMapping =
      std::unordered_map<const Function *, VISAKernel *>;

  // Records information about a subroutine and its "owner". The "owner" of
  // a subroutine is LLVM IR function that spawned *VISAFunction* that contains
  // vISA for the subroutine
  FunctionMapping SubroutineOwnersInfo;
  // "VisaSpanwer" is LLVM IR function that produce *VISAFunction*.
  // Different "VISAFunctions" have their own vISA instructions enumerated
  // separately, but they still can be compiled into a single gen object.
  // does not allow to distiguish those easily).
  FunctionToVisaMapping VisaSpawnerInfo;
  // A separate gen object is usually produced by KernelFunctions -
  // the relationsip between VisaFunction and KernelFunctions is
  // captured by the FunctionOnwers
  FunctionMultiMapping FunctionOwnersInfo;
  // "Kernel functions" are functions that produce genISA object
  // Usually these are FuntionGroup heads, but indirectly-called functions
  // also spawn there own genISA object files
  FunctionToVisaMapping KernelFunctionsInfo;
  std::unordered_set<const Function *> SourceLevelKernels;

  void extractSubroutineInfo(const Function &F, VISABuilder &VB,
                             const FunctionGroupAnalysis &FGA);
  void extractVisaFunctionsEmitters(VISABuilder &VB,
                                    const FunctionGroupAnalysis &FGA,
                                    const CallGraph &CG);

  void extractKernelFunctions(VISABuilder &VB,
                              const FunctionGroupAnalysis &FGA);
  void propagatePrimaryEmitter(const CallGraphNode &CGNode,
                               const Function &PrimaryEmitter);

public:
  void print(raw_ostream &OS) const;
  void dump() const;

  bool isSourceLevelKernel(const Function *F) const {
    return SourceLevelKernels.find(F) != SourceLevelKernels.end();
  }
  bool isKernelFunction(const Function *F) const {
    return KernelFunctionsInfo.find(F) != KernelFunctionsInfo.end();
  }
  bool isSubroutine(const Function *F) const {
    return SubroutineOwnersInfo.find(F) != SubroutineOwnersInfo.end();
  }
  bool isVisaFunctionSpawner(const Function *F) const {
    return VisaSpawnerInfo.find(F) != VisaSpawnerInfo.end();
  }
  // Currently unused
  // For a provided function returns visa object spawned by this function
  // visa object can represent either VISAKernel or VISAFunction
  VISAKernel *getSpawnedVISAFunction(const Function *F) const {
    IGC_ASSERT(!isSubroutine(F));
    auto SpawnedInfoIt = VisaSpawnerInfo.find(F);
    IGC_ASSERT(SpawnedInfoIt != VisaSpawnerInfo.end());
    return SpawnedInfoIt->second;
  }
  // Return a VISA object representing true *VISAKernel* that was spawned by a
  // "kernel" function: IR kernel or indirectly called function.
  VISAKernel *getSpawnedVISAKernel(const Function *F) const {
    IGC_ASSERT_MESSAGE(isKernelFunction(F),
                       "kernel or indirectly called function is expected");
    return KernelFunctionsInfo.at(F);
  }
  // return an "owner" (on vISA level) of the function representing a
  // subroutine
  const Function *getSubroutineOwner(const Function *F) const {
    IGC_ASSERT(isSubroutine(F));
    auto SubInfoIt = SubroutineOwnersInfo.find(F);
    IGC_ASSERT(SubInfoIt != SubroutineOwnersInfo.end());
    return SubInfoIt->second;
  }
  // PrimaryEmitter is the function spawning gen object, that
  // contains the vISA object emitted by the specified function
  std::unordered_set<const Function *>
  getPrimaryEmittersForVisa(const Function *F, bool Strict = true) const;

  std::vector<const Function *> getPrimaryFunctions() const {
    return extractSortedFunctions(KernelFunctionsInfo);
  }

  std::vector<const Function *>
    getSecondaryFunctions(const Function *PrimaryFunction) const;

  ModuleToVisaTransformInfo(VISABuilder &VB, const FunctionGroupAnalysis &FGA,
                            const CallGraph &CG);
};

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void ModuleToVisaTransformInfo::dump() const {
  print(errs());
  errs() << "\n";
}
#endif

void ModuleToVisaTransformInfo::print(raw_ostream &OS) const {

  auto KernelFunctions = extractSortedFunctions(KernelFunctionsInfo);
  auto Subroutines = extractSortedFunctions(SubroutineOwnersInfo);
  auto VisaProducers = extractSortedFunctions(VisaSpawnerInfo);

  // filter-out kernel functions
  VisaProducers.erase(
      std::remove_if(VisaProducers.begin(), VisaProducers.end(),
                     [this](const auto *F) { return isKernelFunction(F); }),
      VisaProducers.end());

  auto PrintFunctionSubroutines = [this, &OS, &Subroutines](const Function *F,
                                                            StringRef Prefix) {
    unsigned Counter = 0;
    for (const auto *LF : Subroutines) {
      if (getSubroutineOwner(LF) != F)
        continue;
      OS << Prefix << "l." << Counter << " " << LF->getName() << "\n";
      ++Counter;
    }
  };

  for (size_t i = 0, NumKF = KernelFunctions.size(); i < NumKF; ++i) {
    const auto *KF = KernelFunctions[i];
    OS << "[" << i << "] " << KF->getName() << " "
       << (SourceLevelKernels.count(KF) != 0 ? "(K)" : "(I)") << "\n";

    PrintFunctionSubroutines(KF, "    ");

    unsigned SubIdx = 0;
    for (const auto *VF : VisaProducers) {
      if (!getPrimaryEmittersForVisa(VF).count(KF))
        continue;
      OS << "    v." << SubIdx << " " << VF->getName() << "\n";
      PrintFunctionSubroutines(VF, "        ");
      ++SubIdx;
    }
  }
}
std::vector<const Function *> ModuleToVisaTransformInfo::getSecondaryFunctions(
    const Function *PrimaryFunction) const {
  auto IsSecondaryFunction = [PrimaryFunction, this](const Function *F) {
    if (F == PrimaryFunction)
      return false;
    return getPrimaryEmittersForVisa(F).count(PrimaryFunction) > 0;
  };
  IGC_ASSERT(isKernelFunction(PrimaryFunction));
  std::vector<const Function *> Result;
  for (const auto &[F, VF] : SubroutineOwnersInfo) {
    (void)VF;
    if (IsSecondaryFunction(F))
      Result.push_back(F);
  }
  for (const auto &[F, VF] : VisaSpawnerInfo) {
    (void)VF;
    if (IsSecondaryFunction(F))
      Result.push_back(F);
  }
  std::sort(Result.begin(), Result.end(), compareFunctionNames);
  return Result;
}

void ModuleToVisaTransformInfo::extractSubroutineInfo(
    const Function &F, VISABuilder &VB, const FunctionGroupAnalysis &FGA) {
  IGC_ASSERT(isVisaFunctionSpawner(&F));
  const auto *SubGr = FGA.getSubGroup(&F);
  IGC_ASSERT(SubGr);
  for (const Function *SF : *SubGr) {
    if (isKernelFunction(SF))
      continue;
    if (genx::requiresStackCall(SF))
      continue;
    checkedEmplace(SubroutineOwnersInfo, SF, &F);
  }
}

std::unordered_set<const Function *>
ModuleToVisaTransformInfo::getPrimaryEmittersForVisa(const Function *F,
                                                     bool Strict) const {
  if (isSubroutine(F)) {
    auto SubrInfoIt = SubroutineOwnersInfo.find(F);
    IGC_ASSERT(SubrInfoIt != SubroutineOwnersInfo.end());
    const Function *SubrOwner = SubrInfoIt->second;
    IGC_ASSERT(SubrOwner);
    IGC_ASSERT(!isSubroutine(SubrOwner));
    return getPrimaryEmittersForVisa(SubrOwner);
  }
  auto InfoRange = FunctionOwnersInfo.equal_range(F);
  std::unordered_set<const Function *> PrimaryEmitters;
  std::transform(InfoRange.first, InfoRange.second,
                 std::inserter(PrimaryEmitters, PrimaryEmitters.end()),
                 [](auto It) { return It.second; });

  if (Strict) {
    IGC_ASSERT_MESSAGE(!PrimaryEmitters.empty(),
                       "could not get primary emitter");
  }
  return PrimaryEmitters;
}

void ModuleToVisaTransformInfo::propagatePrimaryEmitter(
    const CallGraphNode &CGNode, const Function &PrimaryEmitter) {
  const Function *F = CGNode.getFunction();
  if (!F)
    return;
  if (genx::requiresStackCall(F) && !genx::isReferencedIndirectly(F)) {
    auto Range = FunctionOwnersInfo.equal_range(F);
    auto Res =
        std::find_if(Range.first, Range.second, [&PrimaryEmitter](auto Info) {
          return Info.second == &PrimaryEmitter;
        });
    // F -> PrimaryEmitter was already inserted. It happens if a recursion
    // exists.
    if (Res != Range.second)
      return;
    LLVM_DEBUG(dbgs() << "setting <" << PrimaryEmitter.getName()
                      << "> as a host of the stack-callee <" << F->getName()
                      << ">\n");
    checkedEmplace(FunctionOwnersInfo, F, &PrimaryEmitter);
  }

  for (const auto &CalleeCGNode : CGNode)
    propagatePrimaryEmitter(*CalleeCGNode.second, PrimaryEmitter);
}

void ModuleToVisaTransformInfo::extractVisaFunctionsEmitters(
    VISABuilder &VB, const FunctionGroupAnalysis &FGA, const CallGraph &CG) {

  // We've already collected kernels and indirect functions into
  // `KernelFunctionsInfo`.
  for (const auto &[F, VF] : KernelFunctionsInfo) {
    (void)VF;
    const auto *KFNode = CG[F];
    IGC_ASSERT(KFNode);
    propagatePrimaryEmitter(*KFNode, *F);
  }
  // Collect owned functions as a set of unique keys of FunctionOwnersInfo.
  std::unordered_set<const Function *> OwnedFunctions;
  std::transform(FunctionOwnersInfo.begin(), FunctionOwnersInfo.end(),
                 std::inserter(OwnedFunctions, OwnedFunctions.begin()),
                 [](auto Info) { return Info.first; });

  for (const Function *F : OwnedFunctions) {
    // Skip "KernelFunctions" because they have already been processed.
    if (genx::isReferencedIndirectly(F) || genx::isKernel(F))
      continue;
    VISAKernel *VF = VB.GetVISAKernel(F->getName().str());
    checkedEmplace(VisaSpawnerInfo, F, VF);
    extractSubroutineInfo(*F, VB, FGA);
  }
}

void ModuleToVisaTransformInfo::extractKernelFunctions(
    VISABuilder &VB, const FunctionGroupAnalysis &FGA) {
  for (const auto *FG : FGA) {
    for (const Function *F : *FG) {
      if (!genx::isReferencedIndirectly(F) && !genx::isKernel(F))
        continue;
      VISAKernel *VF = VB.GetVISAKernel(F->getName().str());
      if (genx::isKernel(F))
        checkedEmplace(SourceLevelKernels, F);
      checkedEmplace(KernelFunctionsInfo, F, VF);
      checkedEmplace(VisaSpawnerInfo, F, VF);
      checkedEmplace(FunctionOwnersInfo, F, F);

      extractSubroutineInfo(*F, VB, FGA);
    }
  }
}

ModuleToVisaTransformInfo::ModuleToVisaTransformInfo(
    VISABuilder &VB, const FunctionGroupAnalysis &FGA, const CallGraph &CG) {
  extractKernelFunctions(VB, FGA);
  extractVisaFunctionsEmitters(VB, FGA, CG);

  for (const auto *FG : FGA) {
    for (const Function *F : *FG) {
      if (isSourceLevelKernel(F))
        IGC_ASSERT(isKernelFunction(F) && isVisaFunctionSpawner(F) &&
                   !isSubroutine(F));
      if (isKernelFunction(F))
        IGC_ASSERT(isVisaFunctionSpawner(F) && !isSubroutine(F));
      if (isVisaFunctionSpawner(F))
        IGC_ASSERT(!isSubroutine(F));
      if (isSubroutine(F))
        IGC_ASSERT(!isVisaFunctionSpawner(F) && !isKernelFunction(F));
    }
  }
}

namespace {

class CompiledVisaWrapper {

  using FinalizedDI = IGC::DbgDecoder::DbgInfoFormat;

  std::vector<char> GenBinary;
  std::vector<char> DbgInfoBlob;
  std::unique_ptr<IGC::DbgDecoder> DecodedDebugInfo;

  FINALIZER_INFO *JitInfo = nullptr;
  // underlying data is owned by DecodedDebugInfo
  const FinalizedDI *VisaKernelDI = nullptr;

  std::string ErrMsg;

  void setErrorForFunction(const std::string &Err, const Function &F) {
    ErrMsg.append(Err).append("<").append(F.getName()).append(">");

    LLVM_DEBUG(dbgs() << "CW creation for <" << F.getName() << "> aborted: " <<
               ErrMsg);
  }

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

  static void releaseDebugInfoResources(const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    unsigned int DbgSize = 0;
    auto Result = VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize);
    IGC_ASSERT_MESSAGE(Result == 0,
                       "could not get debug blob during cleanup procedure");
    IGC_ASSERT(GenXdbgInfo);
    freeBlock(GenXdbgInfo);
  }

  static void printDecodedGenXDebug(raw_ostream &OS, const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    unsigned int DbgSize = 0;
    auto Result = VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize);
    IGC_ASSERT_MESSAGE(Result == 0,
                       "could not get debug blob during debug printing");
    IGC::DbgDecoder(GenXdbgInfo).print(OS);
  }

  CompiledVisaWrapper(const Function &F, const VISAKernel &VK,
                      StringRef CompiledObjectName) {
    LLVM_DEBUG(dbgs() << "creating CW for <" << F.getName() << ">, using <" <<
               CompiledObjectName << "> as a CompiledObject moniker\n");

    void *GenXdbgInfo = nullptr;
    unsigned int DbgSize = 0;
    if (VK.GetJitInfo(JitInfo) != 0) {
      setErrorForFunction("could not extract jitter info", F);
      return;
    }
    IGC_ASSERT(JitInfo);

    if (VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize) != 0) {
      setErrorForFunction("visa info decode error", F);
      return;
    }

    if (!GenXdbgInfo) {
      setErrorForFunction("could not get debug information from finalizer", F);
      return;
    }

    const char *DbgBlobBytes = static_cast<const char *>(GenXdbgInfo);
    DbgInfoBlob = std::vector<char>(DbgBlobBytes, DbgBlobBytes + DbgSize);

    DecodedDebugInfo = std::make_unique<IGC::DbgDecoder>(DbgInfoBlob.data());
    const auto &CO = DecodedDebugInfo->compiledObjs;
    auto FoundCoIt = std::find_if(
        CO.begin(), CO.end(), [&CompiledObjectName](const auto &DI) {
          return CompiledObjectName == StringRef(DI.kernelName);
        });
    VisaKernelDI = (FoundCoIt == CO.end()) ? nullptr : &*FoundCoIt;
    if (!VisaKernelDI) {
      setErrorForFunction("could not find debug information for", F);
      return;
    }
    if (VisaKernelDI->CISAIndexMap.empty()) {
      setErrorForFunction("empty CisaIndexMap for", F);
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
      setErrorForFunction("fatal error (debug info). inconsistent gen->visa "
                          "mapping: gen index is out of bounds",
                          F);
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
      setErrorForFunction("fatal error (debug info). inconsistent gen->visa "
                          "mapping: gen index are not ordered properly",
                          F);
      return;
    }
  }
};

class GenXFunction final : public IGC::VISAModule {

public:
  GenXFunction(const GenXSubtarget &STIn, const GenXVisaRegAlloc &RAIn,
               const Function &F, const CompiledVisaWrapper &CW,
               const genx::di::VisaMapping &V2I,
               const ModuleToVisaTransformInfo &MVTI)
      : F{F}, ST{STIn}, VisaMapping{V2I}, CompiledVisa{CW}, RA{RAIn},
        MVTI(MVTI), VISAModule(const_cast<Function *>(&F)) {

    if (MVTI.isSubroutine(&F))
       SetType(ObjectType::SUBROUTINE);
    else if (MVTI.isKernelFunction(&F))
       SetType(ObjectType::KERNEL);
    else
       SetType(ObjectType::STACKCALL_FUNC);
  }

  const IGC::DbgDecoder::DbgInfoFormat*
      getCompileUnit(const IGC::DbgDecoder& VD) const override {

    StringRef CompiledObjectName;
    if (MVTI.isSubroutine(&F)) {
      IGC_ASSERT(GetType() == ObjectType::SUBROUTINE);
      CompiledObjectName = MVTI.getSubroutineOwner(&F)->getName();
    } else {
      CompiledObjectName = F.getName();
    }

    auto FoundIt = std::find_if(VD.compiledObjs.begin(), VD.compiledObjs.end(),
                                [&CompiledObjectName](const auto &CO) {
                                  return (CO.kernelName == CompiledObjectName);
                                });
    if (FoundIt == VD.compiledObjs.end())
      return nullptr;

    return &*FoundIt;
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
    const DIVariable *VarDescr = nullptr;
    if (const auto *pDbgAddrInst = dyn_cast<DbgDeclareInst>(DbgInst)) {
      DbgValue = pDbgAddrInst->getAddress();
      VarDescr = pDbgAddrInst->getVariable();
      return EmptyLoc("llvm.dbg.declare is not supported");
    } else if (const auto *pDbgValInst = dyn_cast<DbgValueInst>(DbgInst)) {
      DbgValue = pDbgValInst->getValue();
      VarDescr = pDbgValInst->getVariable();
    } else {
      return EmptyLoc("unsupported Debug Intrinsic");
    }

    IGC_ASSERT(VarDescr);
    if (!DbgValue) {
      if (const auto *LocalVar = dyn_cast<DILocalVariable>(VarDescr))
        if (LocalVar->isParameter())
          return EmptyLoc("unsupported parameter description");
      return EmptyLoc("unsupported DbgInst");
    }
    IGC_ASSERT(DbgValue);
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
      return EmptyLoc("could not find virtual register");
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
  const ModuleToVisaTransformInfo &MVTI;
};

static void processGenXFunction(IGC::IDebugEmitter *Emitter, GenXFunction *GF) {
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

static void dumpDebugInfoFiles(const GenXBackendConfig &BC,
                               const StringRef &KernelName,
                               const ArrayRef<char> ElfBin,
                               const ArrayRef<char> GenDbgBlob) {
  std::string NamePrefix = "dbginfo_";
  if (!BC.dbgInfoDumpsNameOverride().empty())
    NamePrefix.append(BC.dbgInfoDumpsNameOverride()).append("_");

  auto DwarfDumpName = (NamePrefix + KernelName + "_dwarf.elf").str();
  auto GendbgDumpName = (NamePrefix + KernelName + "_gen.dump").str();
  if (BC.hasShaderDumper()) {
    BC.getShaderDumper().dumpBinary(ElfBin, DwarfDumpName);
    BC.getShaderDumper().dumpBinary(GenDbgBlob, GendbgDumpName);
  } else {
    debugDump(DwarfDumpName, ElfBin.data(), ElfBin.size());
    debugDump(GendbgDumpName, GenDbgBlob.data(), GenDbgBlob.size());
  }
}

void GenXDebugInfo::processKernel(const IGC::DebugEmitterOpts &DebugOpts,
                                  const ProgramInfo &PI) {

  IGC_ASSERT_MESSAGE(!PI.FIs.empty(),
                     "Program must include at least one function");
  IGC_ASSERT_MESSAGE(PI.MVTI.getPrimaryEmittersForVisa(&PI.FIs.front().F)
                             .count(&PI.FIs.front().F) == 1,
                     "The head of ProgramInfo is expected to be a kernel");

  LLVM_DEBUG(CompiledVisaWrapper::printDecodedGenXDebug(
      dbgs(), *PI.MVTI.getSpawnedVISAKernel(&PI.FIs.front().F)));

  auto Deleter = [](IGC::IDebugEmitter *Emitter) {
    IGC::IDebugEmitter::Release(Emitter);
  };
  using EmitterHolder = std::unique_ptr<IGC::IDebugEmitter, decltype(Deleter)>;
  EmitterHolder Emitter(IGC::IDebugEmitter::Create(), Deleter);

  using CompiledVisaWrappers =
      std::vector<std::unique_ptr<CompiledVisaWrapper>>;
  CompiledVisaWrappers CWs;
  const auto &MVTI = PI.MVTI;
  std::transform(PI.FIs.begin(), PI.FIs.end(), std::back_inserter(CWs),
                 [&MVTI](const auto &FI) {
                   StringRef CompiledObjectName = FI.F.getName();
                   if (MVTI.isSubroutine(&FI.F))
                     CompiledObjectName =
                         MVTI.getSubroutineOwner(&FI.F)->getName();
                   return std::make_unique<CompiledVisaWrapper>(
                       FI.F, FI.CompiledKernel, CompiledObjectName);
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
          auto GF = std::make_unique<GenXFunction>(ST, RA, FI.F, *CW,
                                                   FI.VisaMapping, PI.MVTI);
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
  if (BC.dbgInfoDumpsEnabled())
    dumpDebugInfoFiles(BC, KernelName, ElfBin,
                       GenXFunctions.front()->getGenDebug());

  // this reset is needed to gracefully cleanup resources held by CWs
  GenXFunctions.clear();
  Emitter.reset();
  CWs.front()->releaseDebugInfoResources(PI.FIs.front().CompiledKernel);
  CWs.clear();

  return;
}

void GenXDebugInfo::cleanup() {
  DISubprogramNodes.clear();
  ElfOutputs.clear();
}

void GenXDebugInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXVisaRegAlloc>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.setPreservesAll();
}

void GenXDebugInfo::processPrimaryFunction(
    const IGC::DebugEmitterOpts &Opts, const ModuleToVisaTransformInfo &MVTI,
    const GenXModule &GM, VISABuilder &VB, const Function &PF) {
  LLVM_DEBUG(dbgs() << "DbgInfo: processing <" << PF.getName() << ">\n");
  IGC_ASSERT(MVTI.isKernelFunction(&PF));
  VISAKernel *VKEntry = MVTI.getSpawnedVISAKernel(&PF);
  IGC_ASSERT(VKEntry);

  using FunctionInfo = ProgramInfo::FunctionInfo;
  std::vector<FunctionInfo> FIs;
  FIs.push_back(FunctionInfo{*GM.getVisaMapping(&PF), *VKEntry, PF});
  const auto &SecondaryFunctions = MVTI.getSecondaryFunctions(&PF);
  std::transform(SecondaryFunctions.begin(), SecondaryFunctions.end(),
                 std::back_inserter(FIs), [&GM, &VKEntry](const Function *F) {
                   const auto &Mapping = *GM.getVisaMapping(F);
                   return FunctionInfo{Mapping, *VKEntry, *F};
                 });
  processKernel(Opts, ProgramInfo{MVTI, std::move(FIs)});
}

static void fillDbgInfoOptions(const GenXBackendConfig &BC,
                               IGC::DebugEmitterOpts &DebugOpts) {
  DebugOpts.DebugEnabled = true;

  if (BC.emitDebugInfoForZeBin()) {
    DebugOpts.ZeBinCompatible = true;
    DebugOpts.EnableRelocation = true;
    DebugOpts.EnforceAMD64Machine = true;
  }
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
  IGC_ASSERT(VB);

  const auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  ModuleToVisaTransformInfo MVTI(*VB, FGA, CG);
  if (!DbgOpt_VisaTransformInfoPath.empty()) {
    std::error_code IgnoredEC;
    raw_fd_ostream DumpStream(DbgOpt_VisaTransformInfoPath, IgnoredEC);
    MVTI.print(DumpStream);
  }
  LLVM_DEBUG(MVTI.print(dbgs()); dbgs() << "\n");

  IGC::DebugEmitterOpts DebugInfoOpts;
  fillDbgInfoOptions(BC, DebugInfoOpts);

  for (const Function *PF : MVTI.getPrimaryFunctions())
    processPrimaryFunction(DebugInfoOpts, MVTI, GM, *VB, *PF);

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
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                    true /*analysis*/)
