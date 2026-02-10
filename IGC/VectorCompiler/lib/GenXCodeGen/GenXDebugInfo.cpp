/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FunctionGroup.h"

#include "GenXDebugInfo.h"
#include "GenXTargetMachine.h"
#include "GenXVisaRegAlloc.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "visa/include/visaBuilder_interface.h"

#include "DebugInfo/DwarfCompileUnit.hpp"
#include "DebugInfo/StreamEmitter.hpp"
#include "DebugInfo/VISADebugInfo.hpp"
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

#include "llvmWrapper/IR/DerivedTypes.h"

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
/// - *ModuleToVisaTransformInfo*
///   Provides information about how LLVM IR functions are mapped onto various
///   vISA (and genISA) objects. Allows us to answer the following questions:
///     - Is a function a subroutine on vISA level?
///     - If a function is a subroutine, what LLVM IR function corresponds to
///     vISA-level "owner" of this subroutine. An "owner" in this case is
///     either VISAFunction or VISAKernel containing the subroutine.
///     - Is LLVM IR function a "primary" one? "primary" function is the one
///     that spawns vISA entity that gets compiled into a separate gen object
///     - For an arbitrary LLVM IR function, get a set of "primary" functions
///     that contain a compiled vISA corresponding to the function in question
///     compiled into their gen objects.
///
/// - *ProgramInfo*
///   A transient object that groups several llvm Functions that are eventually
///   get compiled into a single gen entity. A separate elf file with the
///   debug information is generated for each gen entity.
///   The grouping is with a help of *ModuleToVisaTransformInfo* object.
///
/// - *GenObjectWrapper*
///  Wrapper over the data produced by the finalizer after a kernel gets
///  compiled. Simplifies/Provides access to the following:
///     + gen binary (gen machine instructions)
///     + decoded *gen* debug info and raw gen debug info blob
///     + FINALIZER_INFO structure
///
/// - *CompiledVisaWrapper*
///  For an arbitrary pair of llvm IR Function and VISAKernel objects,
///  does the following:
///     + Validates that IR Function and VISAKernel object are related (that is
///       the vISA spawned by IR Function is owned by the VISAKernel.
///     + Provides services to access *gen* debug info from an appropriate
///     compiled object (*gen* debug info concept).
///
/// *GenXFunction*
///  An object that loosely resembles MachineFunction from the LLVM Machine IR.
///  This is an object that for a given LLVM IR Function provides access to:
///     - LLVM IR Function
///     - VisaMapping
///     - Subtarget
///     - data from CompiledVisaWrapper/GenObjectWrapper
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
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_DEBUG_INFO"

using namespace llvm;

static cl::opt<bool>
    DbgOpt_ValidationEnable("vc-dbginfo-enable-validation", cl::init(false),
                            cl::Hidden,
                            cl::desc("same as IGC_DebugInfoValidation"));
static cl::opt<bool>
    DbgOpt_ZeBinCompatible("vc-experimental-dbg-info-zebin-compatible",
                           cl::init(false), cl::Hidden,
                           cl::desc("same as IGC_ZeBinCompatibleDebugging"));

static cl::opt<std::string> DbgOpt_VisaTransformInfoPath(
    "vc-dump-module-to-visa-transform-info-path", cl::init(""), cl::Hidden,
    cl::desc("filename into which MVTI is dumped"));

static cl::opt<bool> DbgOpt_VisaMappingPrintDbgIntrinsics(
    "vc-dump-visa-mapping-includes-dbgintrin", cl::init(false), cl::Hidden,
    cl::desc("include llvm.dbg intrinsics in visa mapping dump"));

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
checkedEmplace(ContainerT &Container, ArgsT &&...Args) {
  auto Result = Container.emplace(std::forward<ArgsT>(Args)...);
  IGC_ASSERT_MESSAGE(!hasCopy(Container, Result),
                     "a copy of the existing element was emplaced");
  (void)Result;
}

// checkedEmplace for map/set-like containers. If Container.emplace() returns a
// pair whose second element has bool type, this version will be called.
template <typename ContainerT, class... ArgsT>
static std::enable_if_t<IsCheckedEmplace<ContainerT>::value, void>
checkedEmplace(ContainerT &Container, ArgsT &&...Args) {
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

// NOTE: the term "program" is used to avoid a potential confusion
// since the term "kernel" may introduce some ambiguity.
// Here a "program" represents a kind of wrapper over a standalone vISA
// object (which currently is produced by function groups and
// visa-external functions) that finally gets compiled into a stand-alone
// gen entity (binary gen kernel) with some auxiliary information
struct ProgramInfo {
  struct FunctionInfo {
    const genx::di::VisaMapping &VisaMapping;
    const Function &F;
  };

  const ModuleToVisaTransformInfo &MVTI;
  VISAKernel &CompiledKernel;
  std::vector<FunctionInfo> FIs;

  const Function &getEntryPoint() const {
    IGC_ASSERT(!FIs.empty());
    return FIs.front().F;
  }
};

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
    IGC_ASSERT_EXIT(SpawnedInfoIt != VisaSpawnerInfo.end());
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
    IGC_ASSERT_EXIT(SubInfoIt != SubroutineOwnersInfo.end());
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

using VMap = decltype(genx::di::VisaMapping::V2I);
// Compare two functions with their visa-mapping
static bool visaMapComparer(const Function *L, const Function *R,
                            const VMap &V2IL, const VMap &V2IR) {
  if (V2IL.empty() && V2IR.empty())
    return compareFunctionNames(L, R);
  if (V2IL.empty())
    return false;
  if (V2IR.empty())
    return true;
  return V2IL.front().VisaIdx < V2IR.front().VisaIdx;
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
  return Result;
}

void ModuleToVisaTransformInfo::extractSubroutineInfo(
    const Function &F, VISABuilder &VB, const FunctionGroupAnalysis &FGA) {
  IGC_ASSERT(isVisaFunctionSpawner(&F));
  const auto *Gr = FGA.getAnyGroup(&F);
  IGC_ASSERT(Gr);
  for (const Function *SF : *Gr) {
    if (isKernelFunction(SF))
      continue;
    if (vc::requiresStackCall(SF))
      continue;
    checkedEmplace(SubroutineOwnersInfo, SF, &F);
  }
}

std::unordered_set<const Function *>
ModuleToVisaTransformInfo::getPrimaryEmittersForVisa(const Function *F,
                                                     bool Strict) const {
  if (isSubroutine(F)) {
    auto SubrInfoIt = SubroutineOwnersInfo.find(F);
    IGC_ASSERT_EXIT(SubrInfoIt != SubroutineOwnersInfo.end());
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
  if (vc::requiresStackCall(F) && !vc::isIndirect(F)) {
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
    if (vc::isIndirect(F) || vc::isKernel(F))
      continue;
    VISAKernel *VF = VB.GetVISAKernel(F->getName().str());
    checkedEmplace(VisaSpawnerInfo, F, VF);
    extractSubroutineInfo(*F, VB, FGA);
  }
}

void ModuleToVisaTransformInfo::extractKernelFunctions(
    VISABuilder &VB, const FunctionGroupAnalysis &FGA) {
  for (const auto *FG : FGA.AllGroups()) {
    for (const Function *F : *FG) {
      if (!vc::isIndirect(F) && !vc::isKernel(F))
        continue;
      VISAKernel *VF = VB.GetVISAKernel(F->getName().str());
      if (vc::isKernel(F))
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

  for (const auto *FG : FGA.AllGroups()) {
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

class GenObjectWrapper {
  vISA::FINALIZER_INFO *JitInfo = nullptr;
  std::unique_ptr<IGC::VISADebugInfo> VISADebugInfo;
  // TODO: remove this once DbgDecoder is refactored
  unsigned GenDbgInfoDataSize = 0;
  void *GenDbgInfoDataPtr = nullptr;

  int GenBinaryDataSize = 0;
  void *GenBinaryDataPtr = nullptr;

  const Function &EntryPoint;

  std::string ErrMsg;

  void setError(const Twine &Msg) {
    ErrMsg.append((Msg + "<" + EntryPoint.getName().str() + ">").str());

    LLVM_DEBUG(dbgs() << "GOW creation for <" << EntryPoint.getName()
                      << "> aborted: " << Msg.str());
  }

public:
  const Function &getEntryPoint() const { return EntryPoint; }

  ArrayRef<char> getGenDebug() const {
    IGC_ASSERT(GenDbgInfoDataPtr);
    return ArrayRef<char>(static_cast<char *>(GenDbgInfoDataPtr),
                          GenDbgInfoDataSize);
  }

  ArrayRef<char> getGenBinary() const {
    IGC_ASSERT(GenBinaryDataPtr);
    return ArrayRef<char>(static_cast<char *>(GenBinaryDataPtr),
                          GenBinaryDataSize);
  }

  const IGC::VISADebugInfo &getVISADebugInfo() const {
    IGC_ASSERT(VISADebugInfo);
    return *VISADebugInfo;
  }

  const vISA::FINALIZER_INFO &getJitInfo() const {
    IGC_ASSERT(!hasErrors() && JitInfo);
    return *JitInfo;
  };

  GenObjectWrapper(VISAKernel &VK, const Function &F);
  ~GenObjectWrapper() { releaseDebugInfoResources(); }
  GenObjectWrapper(const GenObjectWrapper &) = delete;
  GenObjectWrapper &operator=(const GenObjectWrapper &) = delete;

  bool hasErrors() const { return !ErrMsg.empty(); }

  const std::string &getError() const { return ErrMsg; }

  void releaseDebugInfoResources() {
    if (!GenDbgInfoDataPtr) {
      IGC_ASSERT(GenDbgInfoDataSize == 0);
      return;
    }
    freeBlock(GenDbgInfoDataPtr);
    GenDbgInfoDataPtr = nullptr;
    GenDbgInfoDataSize = 0;
  }

  void printDecodedGenXDebug(raw_ostream &OS) const {
    IGC_ASSERT(!hasErrors());
    LLVM_DEBUG(dbgs() << "GenXDebugInfo size: " << GenDbgInfoDataSize << "\n");
    getVISADebugInfo().print(OS);
  }
};

GenObjectWrapper::GenObjectWrapper(VISAKernel &VK, const Function &F)
    : EntryPoint(F) {
  if (VK.GetJitInfo(JitInfo) != 0) {
    setError("could not extract jitter info");
    return;
  }
  IGC_ASSERT(JitInfo);

  // Extract Gen Binary (will need it for line table generation)
  VK.GetGenxBinary(GenBinaryDataPtr, GenBinaryDataSize);
  if (GenBinaryDataSize <= 0) {
    setError("could not extract gen binary from finalizer");
    return;
  }

  if (VK.GetGenxDebugInfo(GenDbgInfoDataPtr, GenDbgInfoDataSize) != 0) {
    setError("could not get gen debug information from finalizer");
    return;
  }
  if (!GenDbgInfoDataPtr) {
    setError("gen debug information reported by finalizer is inconsistent");
    return;
  }
  VISADebugInfo = std::make_unique<IGC::VISADebugInfo>(GenDbgInfoDataPtr);
};

class CompiledVisaWrapper {

  using FinalizedDI = IGC::DbgDecoder::DbgInfoFormat;

  const GenObjectWrapper &GOW;
  // underlying data is owned by VISADebugInfo, owned by GOW
  const FinalizedDI *VisaKernelDI = nullptr;

  std::string ErrMsg;

  void setErrorForFunction(const std::string &Err, const Function &F) {
    ErrMsg.append(Err).append("<").append(F.getName().str()).append(">");

    LLVM_DEBUG(dbgs() << "CW creation for <" << F.getName()
                      << "> aborted: " << ErrMsg);
  }

public:
  const vISA::FINALIZER_INFO &getJitInfo() const { return GOW.getJitInfo(); };

  const FinalizedDI &getFinalizerDI() const {
    IGC_ASSERT(ErrMsg.empty() && VisaKernelDI);
    return *VisaKernelDI;
  }

  const IGC::VISADebugInfo &getVISADebugInfo() const {
    return GOW.getVISADebugInfo();
  }

  ArrayRef<char> getGenDebug() const { return GOW.getGenDebug(); }
  ArrayRef<char> getGenBinary() const { return GOW.getGenBinary(); }

  const std::string &getError() const { return ErrMsg; }

  bool hasErrors() const { return !getError().empty(); }

  CompiledVisaWrapper(CompiledVisaWrapper &&Other) = default;
  CompiledVisaWrapper(const Function &F, StringRef CompiledObjectName,
                      const GenObjectWrapper &GOWIn)
      : GOW(GOWIn) {
    struct Gen2VisaIdx {
      unsigned GenOffset;
      unsigned VisaIdx;
    };
    LLVM_DEBUG(dbgs() << "creating CW for <" << F.getName() << ">, using <"
                      << CompiledObjectName
                      << "> as a CompiledObject moniker\n");
    IGC_ASSERT(!GOW.hasErrors());

    const auto &CO = GOW.getVISADebugInfo().getRawDecodedData().compiledObjs;
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
    std::transform(
        VisaKernelDI->CISAIndexMap.begin(), VisaKernelDI->CISAIndexMap.end(),
        std::back_inserter(Gen2Visa),
        [](const auto &V2G) { return Gen2VisaIdx{V2G.second, V2G.first}; });

    const auto &GenBinary = GOW.getGenBinary();
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
               const GenXBaling &BAn, const Function &F,
               CompiledVisaWrapper &&CW, const genx::di::VisaMapping &V2I,
               const ModuleToVisaTransformInfo &MVTI, bool IsPrimary)
      : F{F}, ST{STIn}, VisaMapping{V2I}, CompiledVisa{std::move(CW)}, RA{RAIn},
        BA{BAn}, MVTI(MVTI), VISAModule(const_cast<Function *>(&F), IsPrimary) {

    if (MVTI.isSubroutine(&F))
      SetType(ObjectType::SUBROUTINE);
    else if (MVTI.isKernelFunction(&F))
      SetType(ObjectType::KERNEL);
    else
      SetType(ObjectType::STACKCALL_FUNC);
  }

  ~GenXFunction() {
    LLVM_DEBUG(dbgs() << "~GenXFunction() called for " << F.getName() << "\n");
  }
  GenXFunction(const GenXFunction &) = delete;
  GenXFunction &operator=(const GenXFunction &) = delete;

  llvm::StringRef GetVISAFuncName() const override {
    // TODO: this is not quite correct since VISA names is defined by VISA label
    return F.getName();
  }

  bool isSubroutine() const { return GetType() == ObjectType::SUBROUTINE; }

  bool isStackCall() const { return GetType() == ObjectType::STACKCALL_FUNC; }

  bool isKernel() const { return GetType() == ObjectType::KERNEL; }

  const IGC::VISAObjectDebugInfo &
  getVisaObjectDI(const IGC::VISADebugInfo &VDI) const override {
    StringRef CompiledObjectName =
        isSubroutine() ? MVTI.getSubroutineOwner(&F)->getName() : F.getName();
    return VDI.getVisaObjectByCompliledObjectName(CompiledObjectName);
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
  unsigned getGRFSizeInBytes() const override { return ST.getGRFByteSize(); }
  unsigned getNumGRFs() const override {
    return CompiledVisa.getJitInfo().stats.numGRFTotal;
  }
  unsigned getPointerSize() const override {
    return F.getParent()->getDataLayout().getPointerSize();
  }
  uint64_t getTypeSizeInBits(Type *Ty) const override {
    return F.getParent()->getDataLayout().getTypeSizeInBits(Ty);
  }
  ArrayRef<char> getGenDebug() const override {
    return CompiledVisa.getGenDebug();
  }
  ArrayRef<char> getGenBinary() const override {
    return CompiledVisa.getGenBinary();
  }

  const IGC::VISADebugInfo &getVISADebugInfo() const {
    return CompiledVisa.getVISADebugInfo();
  }

  const IGC::DbgDecoder::DbgInfoFormat &getFinalizerDI() const {
    return CompiledVisa.getFinalizerDI();
  }

  const genx::di::VisaMapping &getVisaMapping() const { return VisaMapping; }

  static constexpr unsigned RdIndex =
      GenXIntrinsic::GenXRegion::RdIndexOperandNum;
  static constexpr unsigned RdVstride =
      GenXIntrinsic::GenXRegion::RdVStrideOperandNum;
  static constexpr unsigned RdWidth =
      GenXIntrinsic::GenXRegion::RdWidthOperandNum;
  static constexpr unsigned RdStride =
      GenXIntrinsic::GenXRegion::RdStrideOperandNum;
  static constexpr unsigned RdNumOp =
      GenXIntrinsic::GenXRegion::OldValueOperandNum;

  using OffsetsVector = llvm::SmallVector<unsigned, 0>;

  std::tuple<const Value *, OffsetsVector>
  calculateBaledLocation(const CallInst *UseInst, const GenXBaling &BA,
                         const DataLayout &DL) const {
    IGC_ASSERT(UseInst);
    if (!GenXIntrinsic::isRdRegion(UseInst))
      return std::make_tuple(UseInst, OffsetsVector());
    auto BI = BA.getBaleInfo(UseInst);

    if (BI.Type != genx::BaleInfo::RDREGION ||
        !dyn_cast<ConstantInt>(UseInst->getOperand(RdIndex)) ||
        BI.isOperandBaled(RdNumOp) || !BA.isBaled(UseInst))
      return std::make_tuple(UseInst, OffsetsVector());

    auto GetSignConstant = [](Value *Operand) {
      auto *CI = cast<ConstantInt>(Operand);
      return CI->getSExtValue();
    };

    // In this place comes rdregion, whose operand is not baled - here we
    // build location for its operand
    LLVM_DEBUG(dbgs() << "   Found Bale candidate for propagation:\n";
               UseInst->dump(););
    auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(UseInst->getType());
    // TODO: Investigate scalar
    if (!VTy)
      return std::make_tuple(UseInst, OffsetsVector());
    auto Vstride = GetSignConstant(UseInst->getOperand(RdVstride));
    auto Width = GetSignConstant(UseInst->getOperand(RdWidth));
    auto Stride = GetSignConstant(UseInst->getOperand(RdStride));
    // Convert start index from bytes to bits
    auto StartIdx =
        GetSignConstant(UseInst->getOperand(RdIndex)) * vc::ByteBits;
    auto ElSizeInBits = vc::getTypeSize(VTy->getElementType(), &DL).inBits();
    IGC_ASSERT(Width);
    unsigned NumElements = VTy->getNumElements() / Width;
    OffsetsVector Offsets;

    for (unsigned I = 0; I < NumElements; ++I) {
      for (unsigned J = 0; J < Width; ++J) {
        auto CurrOffset = StartIdx + ElSizeInBits * (I * Vstride + J * Stride);
        // Check type overflow
        IGC_ASSERT(CurrOffset <= std::numeric_limits<unsigned>::max());
        if ((CurrOffset % getGRFSizeInBits()) + ElSizeInBits >
            getGRFSizeInBits()) {
          LLVM_DEBUG(dbgs() << "  Fail to generate Bale location element has "
                               "crossGRF access\n");
          return std::make_tuple(UseInst, OffsetsVector());
        }
        Offsets.push_back(CurrOffset);
      }
    }
    // Replace value to source of rdregion
    return std::make_tuple(UseInst->getOperand(RdNumOp), std::move(Offsets));
  }

  IGC::VISAVariableLocation
  GetVariableLocation(const Instruction *DbgInst) const override {
    using Location = IGC::VISAVariableLocation;
    auto EmptyLoc = [this](StringRef Reason) {
      LLVM_DEBUG(dbgs() << "  Empty Location Returned (" << Reason
                        << ")\n <<<\n");
      return Location(this);
    };

    IGC_ASSERT(isa<DbgInfoIntrinsic>(DbgInst));

    LLVM_DEBUG(dbgs() << " >>>\n  GetVariableLocation for " << *DbgInst
                      << "\n");
    const DIVariable *VarDescr = nullptr;
    if (const auto *PDbgAddrInst = dyn_cast<DbgDeclareInst>(DbgInst)) {
      VarDescr = PDbgAddrInst->getVariable();
    } else if (const auto *PDbgValInst = dyn_cast<DbgValueInst>(DbgInst)) {
      VarDescr = PDbgValInst->getVariable();
    } else {
      return EmptyLoc("unsupported Debug Intrinsic");
    }
    const Value *DbgValue =
        IGCLLVM::getVariableLocation(cast<DbgVariableIntrinsic>(DbgInst));

    OffsetsVector Offsets;
    if (auto *UseInst = dyn_cast_or_null<CallInst>(DbgValue)) {
      std::tie(DbgValue, Offsets) =
          calculateBaledLocation(UseInst, BA, F.getParent()->getDataLayout());
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
      LLVM_DEBUG(dbgs() << "  ConstantLoc\n <<<\n");
      return Location(ConstVal, this);
    }

    auto *Reg = getRegisterForValue(DbgValue);
    if (!Reg) {
      return EmptyLoc("could not find virtual register");
    }

    return Location(Reg->Num, std::move(Offsets), this);
  }

  void UpdateVisaId() override {
    // do nothing (the moment we need to advance index is controlled explicitly)
  }
  void ValidateVisaId() override {
    // do nothing (we don't need validation since VISA is built already)
  }
  uint16_t GetSIMDSize() const override { return 1; }

  void *getPrivateBase() const override { return nullptr; };
  void setPrivateBase(void *) override {};

  bool hasPTO() const override { return false; }
  int getPTOReg() const override { return -1; }
  int getFPReg() const override { return -1; }
  uint64_t getFPOffset() const override { return 16; }

  bool usesSlot1ScratchSpill() const override { return false; }

  const GenXVisaRegAlloc::Reg *getRegisterForValue(const Value *V) const {
    return RA.getRegForValueOrNull(const_cast<Value *>(V));
  }

  void printVisaMapping(raw_ostream &OS, unsigned Level = 0) const {
    const std::string Prefix(Level * 4, ' ');
    OS << Prefix;
    OS << "VisaMapping for <" << getFunction()->getName() << ">(";
    for (const auto &Arg : enumerate(getFunction()->args())) {
      OS << "a" << Arg.index() << ":";
      auto *Reg = getRegisterForValue(&Arg.value());
      if (Reg)
        Reg->print(OS);
      else
        OS << "_";
      OS << ";";
    }
    OS << ") - {\n";

    size_t SkippedIndex = 0;
    size_t SkippedCount = 0;
    auto PrintSkippedAndClearCount = [&SkippedIndex, &SkippedCount, &Prefix,
                                      &OS]() {
      if (!SkippedCount)
        return;
      OS << Prefix << "    <" << SkippedIndex << ">: "
         << "skipped " << SkippedCount << " llvm.dbg.* intrinsics\n";
      SkippedCount = 0;
    };

    for (const auto &Mapping : VisaMapping.V2I) {
      auto VisaIndexCurr = Mapping.VisaIdx;
      auto VisaIndexNext = Mapping.VisaIdx + Mapping.VisaCount;
      const auto *Inst = Mapping.Inst;

      if (Mapping.IsDbgInst && !DbgOpt_VisaMappingPrintDbgIntrinsics) {
        if (!SkippedCount)
          SkippedIndex = VisaIndexCurr;
        ++SkippedCount;
        continue;
      }

      PrintSkippedAndClearCount();

      OS << Prefix;
      OS << " [" << VisaIndexCurr << ";" << VisaIndexNext << "): ";

      auto *Reg = getRegisterForValue(Inst);
      OS << "<";
      if (Reg)
        Reg->print(OS);
      OS << "> ";

      Inst->print(OS);

      if (auto DbgLoc = Inst->getDebugLoc()) {
        StringRef Filename = DbgLoc->getFilename();
        auto Line = DbgLoc->getLine();
        auto Col = DbgLoc->getColumn();
        OS << " [" << Filename << ":" << Line << "," << Col << "]";
      }
      OS << "\n";
    }

    PrintSkippedAndClearCount();

    OS << Prefix;
    OS << "}\n";
  }

  const ModuleToVisaTransformInfo &getMVTI() const { return MVTI; }

private:
  const Function &F;
  const GenXSubtarget &ST;
  const genx::di::VisaMapping &VisaMapping;
  CompiledVisaWrapper CompiledVisa;
  const GenXVisaRegAlloc &RA;
  const GenXBaling &BA;
  const ModuleToVisaTransformInfo &MVTI;
};

using VisaMapType = std::vector<genx::di::VisaMapping::Mapping>;

[[maybe_unused]] static bool validateVisaMapping(const VisaMapType &V2I) {
  // Last used visa index
  auto ExpectedNextId = V2I.cbegin()->VisaIdx;
  for (auto MappingIt = V2I.cbegin(); MappingIt != V2I.cend(); ++MappingIt) {
    auto VisaIndexCurr = MappingIt->VisaIdx;
    auto VisaIndexNext = MappingIt->VisaIdx + MappingIt->VisaCount;
    const auto *Inst = MappingIt->Inst;

    IGC_ASSERT(VisaIndexCurr <= VisaIndexNext);
    if (MappingIt->IsDbgInst) {
      IGC_ASSERT(isa<DbgInfoIntrinsic>(MappingIt->Inst));
      IGC_ASSERT(MappingIt->VisaCount == 0);
    } else {
      // Check that in map only real instructions
      IGC_ASSERT(MappingIt->VisaCount > 0);
    }

    // ExpectedNextId (from the previous iteration) should be the same as
    // the current index
    // In other words, we should have no gaps in vISA mapping
    if (ExpectedNextId != VisaIndexCurr) {
      LLVM_DEBUG(dbgs() << "Detected inconsistency, current visa-Index: "
                        << VisaIndexCurr
                        << " is not equal to expected visa-Index: "
                        << ExpectedNextId << "\n");
      return false;
    }

    ExpectedNextId = VisaIndexNext;
    // Mapping may interupts in calls, because functions may be inlined.
    // Just do not check ExpectedNextId for the next instruction.
    if (isa<CallInst>(Inst) && !isa<DbgInfoIntrinsic>(Inst))
      ExpectedNextId = std::next(MappingIt)->VisaIdx;

    // Marker that this is the last instruction of a BB
    bool lastBlockInst = ((std::next(MappingIt) != V2I.cend()) &&
                          (MappingIt->Inst->getParent() !=
                           (std::next(MappingIt)->Inst->getParent())));

    // Current implementation does not create mapping for vISA labels.
    // That's why the next mapping is considered to be correct:
    //    VisaMapping: [18;20):  br label %1, !dbg !147
    //    VisaMapping: [21;22):  %icmp = icmp ult i32 %.06, 8, !dbg !148
    // In visaasm-file 20-th instruction will be a bb-label:
    //     lifetime.start V51                        /// $19
    //   BB_1:
    //     cmp.lt (M1, 1) P1 V105(0,0)<0;1,0> 0x8:ud /// $21
    // We avoid checking of the ExpectedNextId in such cases - [21;22).
    if (lastBlockInst) {
      ExpectedNextId = std::next(MappingIt)->VisaIdx;
    }
  }
  return true;
}

static void processGenXFunction(IGC::IDebugEmitter &Emitter, GenXFunction &GF) {
  Emitter.setCurrentVISA(&GF);
  const auto &V2I = GF.getVisaMapping().V2I;
  for (auto MappingIt = V2I.cbegin(); MappingIt != V2I.cend(); ++MappingIt) {
    auto VisaIndexCurr = MappingIt->VisaIdx;
    auto VisaIndexNext = MappingIt->VisaIdx + MappingIt->VisaCount;

    // Note: "index - 1" is because we mimic index values as if they were
    // before corresponding instructions were inserted
    GF.SetVISAId(VisaIndexCurr - 1);
    // we need this const_cast because of the flawed VISA Emitter API
    auto *Inst = const_cast<Instruction *>(MappingIt->Inst);
    Emitter.BeginInstruction(Inst);
    GF.SetVISAId(VisaIndexNext - 1);
    Emitter.EndInstruction(Inst);
  }
}

using GenXObjectHolder = std::unique_ptr<GenXFunction>;
GenXObjectHolder buildGenXFunctionObject(const ModuleToVisaTransformInfo &MVTI,
                                         const GenObjectWrapper &GOW,
                                         const ProgramInfo::FunctionInfo &FI,
                                         const GenXSubtarget &ST,
                                         const GenXVisaRegAlloc &RA,
                                         const GenXBaling &BA) {
  StringRef CompiledObjectName = FI.F.getName();
  if (MVTI.isSubroutine(&FI.F))
    CompiledObjectName = MVTI.getSubroutineOwner(&FI.F)->getName();

  CompiledVisaWrapper CW(FI.F, CompiledObjectName, GOW);
  if (CW.hasErrors())
    vc::diagnose(FI.F.getContext(), "GenXDebugInfo", CW.getError());

  bool IsPrimaryFunction = &GOW.getEntryPoint() == &FI.F;
  return std::make_unique<GenXFunction>(
      ST, RA, BA, FI.F, std::move(CW), FI.VisaMapping, MVTI, IsPrimaryFunction);
}

using GenXObjectHolderList = std::vector<GenXObjectHolder>;
GenXObjectHolderList translateProgramInfoToGenXFunctionObjects(
    const GenObjectWrapper &GOW, const ProgramInfo &PI, const GenXSubtarget &ST,
    const std::vector<const GenXVisaRegAlloc *> &RAs,
    const std::vector<const GenXBaling *> &BAs) {
  const auto &MVTI = PI.MVTI;
  GenXObjectHolderList GenXFunctionHolders;
  IGC_ASSERT(PI.FIs.size() == RAs.size());
  IGC_ASSERT(BAs.size() == RAs.size());
  auto Zippy = llvm::zip(RAs, BAs);
  std::transform(PI.FIs.begin(), PI.FIs.end(), Zippy.begin(),
                 std::back_inserter(GenXFunctionHolders),
                 [&ST, &MVTI, &GOW](const auto &FI, const auto &ZIP) {
                   const GenXVisaRegAlloc *RA = std::get<0>(ZIP);
                   const GenXBaling *BA = std::get<1>(ZIP);
                   return buildGenXFunctionObject(MVTI, GOW, FI, ST, *RA, *BA);
                 });
  return GenXFunctionHolders;
}

using GenXFunctionPtrList = std::vector<GenXFunction *>;
using GenXFunctionConstPtrList = std::vector<const GenXFunction *>;
GenXFunctionPtrList initializeDebugEmitter(
    IGC::IDebugEmitter &Emitter, const IGC::DebugEmitterOpts &DebugOpts,
    const ProgramInfo &PI, GenXObjectHolderList &&GFsHolderIn) {

  GenXFunctionPtrList GFPointers;
  for (auto &&GF : GFsHolderIn) {
    GFPointers.push_back(GF.get());

    if (GF->isPrimaryFunc()) {
      Emitter.Initialize(std::move(GF), DebugOpts);
    } else {
      Emitter.registerVISA(GFPointers.back());
      Emitter.resetModule(std::move(GF));
    }
  }
  // Currently Debug Info Emitter expects that GenXFunctions are
  // processed in the same order as they appear in the visa object
  // Ideally, the order should not matter - but we are not there yet
  // due to DwarfEmitter limitations
  std::sort(GFPointers.begin(), GFPointers.end(), [](auto *LGF, auto *RGF) {
    const auto &LDI = LGF->getFinalizerDI();
    const auto &RDI = RGF->getFinalizerDI();
    if (LDI.relocOffset == RDI.relocOffset)
      return visaMapComparer(LGF->getFunction(), RGF->getFunction(),
                             LGF->getVisaMapping().V2I,
                             RGF->getVisaMapping().V2I);
    return LDI.relocOffset < RDI.relocOffset;
  });
  return GFPointers;
}

std::string makePrefixForAuxiliaryShaderDump(const GenXBackendConfig &BC,
                                             const GenObjectWrapper &GOW) {

  const auto &KernelName = GOW.getEntryPoint().getName();
  std::string Prefix = "dbginfo_";
  if (!BC.dbgInfoDumpsNameOverride().empty())
    Prefix.append(BC.dbgInfoDumpsNameOverride()).append("_");
  Prefix.append(KernelName.str());
  return Prefix;
}

std::string serializeDecodedGenDebugInfo(const GenObjectWrapper &GOW) {
  std::string Result;
  llvm::raw_string_ostream OS(Result);
  GOW.printDecodedGenXDebug(OS);
  OS.flush();
  return Result;
}

using GFPtrSet = std::unordered_set<const GenXFunction *>;
using KernelAndVisaOwners =
    std::pair<const GenXFunction *, GenXFunctionConstPtrList>;

void printVisaMapping(raw_ostream &OS, const GenXFunction &GF, unsigned Level,
                      GFPtrSet &NotPrinted) {
  IGC_ASSERT(NotPrinted.count(&GF));
  GF.printVisaMapping(OS, Level);
  NotPrinted.erase(&GF);
}

void printSubroutinesVisaMapping(raw_ostream &OS,
                                 GenXFunctionConstPtrList Subroutines,
                                 unsigned Level, GFPtrSet &NotPrinted) {
  // Sort is needed for printing elements with less visa index first
  std::sort(Subroutines.begin(), Subroutines.end(),
            [](const GenXFunction *L, const GenXFunction *R) {
              const auto &V2IL = L->getVisaMapping().V2I;
              const auto &V2IR = R->getVisaMapping().V2I;
              return visaMapComparer(L->getFunction(), R->getFunction(), V2IL,
                                     V2IR);
            });
  for (const auto *SGF : Subroutines)
    printVisaMapping(OS, *SGF, Level, NotPrinted);
}

void printVisaOwnerVisaMapping(raw_ostream &OS, const GenXFunction &VO,
                               const GenXFunctionConstPtrList &Subroutines,
                               unsigned Level, GFPtrSet &NotPrinted) {
  printVisaMapping(OS, VO, Level, NotPrinted);

  printSubroutinesVisaMapping(OS, Subroutines, Level + 1, NotPrinted);
}

bool isStackCallGF(const GenXFunction *GF) {
  IGC_ASSERT(GF);
  return GF->isStackCall();
}

bool isKernelGF(const GenXFunction *GF) {
  IGC_ASSERT(GF);
  return GF->isKernel();
}

KernelAndVisaOwners
getKernelAndVisaOwners(const GenXFunctionConstPtrList &GFs) {
  IGC_ASSERT(!GFs.empty());
  GenXFunctionConstPtrList StackCalls;
  std::copy_if(GFs.begin(), GFs.end(), std::back_inserter(StackCalls),
               isStackCallGF);
  IGC_ASSERT(std::count_if(GFs.begin(), GFs.end(), isKernelGF) == 1);
  auto KGFIt = std::find_if(GFs.begin(), GFs.end(), isKernelGF);
  IGC_ASSERT_EXIT(KGFIt != GFs.end());
  return {*KGFIt, std::move(StackCalls)};
}

GenXFunctionConstPtrList
getSubroutinesForVisaOwner(const GenXFunction &VO,
                           const GenXFunctionConstPtrList &AllGFs) {
  const auto &MVTI = VO.getMVTI();
  GenXFunctionConstPtrList Result;
  std::copy_if(AllGFs.begin(), AllGFs.end(), std::back_inserter(Result),
               [&MVTI, &VO](const auto *GF) {
                 if (!GF->isSubroutine())
                   return false;
                 const Function *F = GF->getFunction();
                 return MVTI.getSubroutineOwner(F) == VO.getFunction();
               });
  return Result;
}

std::string serializeGFsVisaMapping(const GenXFunctionConstPtrList &GFs) {
  std::string Result;
  llvm::raw_string_ostream OS(Result);

  GFPtrSet ToPrint(GFs.begin(), GFs.end());

  auto [KGF, VisaOwners] = getKernelAndVisaOwners(GFs);

  printVisaOwnerVisaMapping(OS, *KGF, getSubroutinesForVisaOwner(*KGF, GFs), 0,
                            ToPrint);
  for (const auto *SpawnerGF : VisaOwners)
    printVisaOwnerVisaMapping(OS, *SpawnerGF,
                              getSubroutinesForVisaOwner(*SpawnerGF, GFs), 1,
                              ToPrint);
  IGC_ASSERT(ToPrint.empty());

  OS.flush();
  return Result;
}

void dumpDebugInfo(const GenXBackendConfig &BC, const GenObjectWrapper &GOW,
                   const GenXFunctionPtrList &GFs, const ArrayRef<char> ElfBin,
                   const ArrayRef<char> ErrLog) {

  auto DecodedGenInfo = serializeDecodedGenDebugInfo(GOW);
  GenXFunctionConstPtrList CGFs;
  std::copy(GFs.begin(), GFs.end(), std::back_inserter(CGFs));
  auto SerializedVisaMapping = serializeGFsVisaMapping(CGFs);

  std::string Prefix = makePrefixForAuxiliaryShaderDump(BC, GOW);

  vc::produceAuxiliaryShaderDumpFile(BC, Twine(Prefix) + "_dwarf.elf", ElfBin);
  vc::produceAuxiliaryShaderDumpFile(BC, Twine(Prefix) + "_gen.dump",
                                     GOW.getGenDebug());
  vc::produceAuxiliaryShaderDumpFile(BC, Twine(Prefix) + "_gen.decoded.dump",
                                     DecodedGenInfo);
  vc::produceAuxiliaryShaderDumpFile(BC, Twine(Prefix) + "_visa.mapping",
                                     SerializedVisaMapping);

  if (!ErrLog.empty())
    vc::produceAuxiliaryShaderDumpFile(BC, Twine(Prefix) + ".dbgerr", ErrLog);
}

} // namespace

void GenXDebugInfo::processKernel(const IGC::DebugEmitterOpts &DebugOpts,
                                  const ProgramInfo &PI) {

  IGC_ASSERT_MESSAGE(!PI.FIs.empty(),
                     "Program must include at least one function");
  IGC_ASSERT_MESSAGE(PI.MVTI.getPrimaryEmittersForVisa(&PI.getEntryPoint())
                             .count(&PI.getEntryPoint()) == 1,
                     "The head of ProgramInfo is expected to be a kernel");

  GenObjectWrapper GOW(PI.CompiledKernel, PI.getEntryPoint());
  if (GOW.hasErrors())
    vc::diagnose(GOW.getEntryPoint().getContext(), "GenXDebugInfo",
                 GOW.getError());

  LLVM_DEBUG(GOW.printDecodedGenXDebug(dbgs()));

  auto Deleter = [](IGC::IDebugEmitter *Emitter) {
    IGC::IDebugEmitter::Release(Emitter);
  };
  using EmitterHolder = std::unique_ptr<IGC::IDebugEmitter, decltype(Deleter)>;
  EmitterHolder Emitter(IGC::IDebugEmitter::Create(), Deleter);

  const auto &ST = getAnalysis<TargetPassConfig>()
                       .getTM<GenXTargetMachine>()
                       .getGenXSubtarget();
  auto *FGA = &getAnalysis<FunctionGroupAnalysis>();
  std::vector<const GenXVisaRegAlloc *> VisaRegAllocs;
  std::vector<const GenXBaling *> BalingList;
  for (const auto &FP : PI.FIs) {
    FunctionGroup *currentFG = FGA->getAnyGroup(&FP.F);
    VisaRegAllocs.push_back(
        &(getAnalysis<GenXVisaRegAllocWrapper>().getFGPassImpl(currentFG)));
    GenXBaling *Baling =
        &(getAnalysis<GenXGroupBalingWrapper>().getFGPassImpl(currentFG));
    BalingList.push_back(Baling);
  }

  GenXFunctionPtrList GFPointers =
      initializeDebugEmitter(*Emitter, DebugOpts, PI,
                             translateProgramInfoToGenXFunctionObjects(
                                 GOW, PI, ST, VisaRegAllocs, BalingList));

  auto &KF = GOW.getEntryPoint();
  IGC_ASSERT(ElfOutputs.count(&KF) == 0);
  auto &ElfBin = ElfOutputs[&KF];

  for (auto *GF : GFPointers) {
    LLVM_DEBUG(dbgs() << "\n--- Processing GenXFunction:  "
                      << GF->getFunction()->getName().str() << " ---\n");
    LLVM_DEBUG(GF->printVisaMapping(dbgs()));
    IGC_ASSERT(validateVisaMapping(GF->getVisaMapping().V2I));
    processGenXFunction(*Emitter, *GF);
    bool ExpectMore = GF != GFPointers.back();
    LLVM_DEBUG(dbgs() << "--- Starting Debug Info Finalization (final:  "
                      << !ExpectMore << ") ---\n");
    auto Out = Emitter->Finalize(!ExpectMore, GF->getVISADebugInfo());
    if (!ExpectMore) {
      ElfBin = std::move(Out);
    } else {
      IGC_ASSERT(Out.empty());
    }
    LLVM_DEBUG(dbgs() << "---     \\ Debug Info Finalized /     ---\n");
  }

  [[maybe_unused]] const auto &KernelName = KF.getName();
  LLVM_DEBUG(dbgs() << "got Debug Info for <" << KernelName.str() << "> "
                    << "- " << ElfBin.size() << " bytes\n");

  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (BC.dbgInfoDumpsEnabled()) {
    const auto &ErrLog = Emitter->getErrors();
    dumpDebugInfo(BC, GOW, GFPointers, ElfBin, {ErrLog.data(), ErrLog.size()});
  }

  return;
}

void GenXDebugInfo::cleanup() { ElfOutputs.clear(); }

void GenXDebugInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXVisaRegAllocWrapper>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.addRequired<GenXGroupBaling>();
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
  FIs.push_back(FunctionInfo{*GM.getVisaMapping(&PF), PF});
  auto SecondaryFunctions = MVTI.getSecondaryFunctions(&PF);
  // Sorting by visa-elements, because finalizer expect sorted sequence
  // for emmiting correct debug hi/low pc for functions
  std::sort(SecondaryFunctions.begin(), SecondaryFunctions.end(),
            [&GM](const Function *L, const Function *R) {
              const auto &V2IL = GM.getVisaMapping(L)->V2I;
              const auto &V2IR = GM.getVisaMapping(R)->V2I;
              return visaMapComparer(L, R, V2IL, V2IR);
            });
  std::transform(SecondaryFunctions.begin(), SecondaryFunctions.end(),
                 std::back_inserter(FIs), [&GM](const Function *F) {
                   const auto &Mapping = *GM.getVisaMapping(F);
                   return FunctionInfo{Mapping, *F};
                 });
  processKernel(Opts, ProgramInfo{MVTI, *VKEntry, std::move(FIs)});
}

static void fillDbgInfoOptions(const GenXBackendConfig &BC,
                               IGC::DebugEmitterOpts &DebugOpts) {
  DebugOpts.DebugEnabled = true;

  if (BC.emitDWARFDebugInfoForZeBin() || DbgOpt_ZeBinCompatible) {
    DebugOpts.ZeBinCompatible = true;
    DebugOpts.EnableRelocation = true;
    DebugOpts.EnforceAMD64Machine = true;
  }
  if (BC.enableDebugInfoValidation() || DbgOpt_ValidationEnable) {
    DebugOpts.EnableDebugInfoValidation = true;
  }
}

bool GenXDebugInfo::runOnModule(Module &M) {
  auto &GM = getAnalysis<GenXModule>();
  // Note: we check that MVTI dumps were not requested here,
  // since it is possible to request those without the presence of
  // debug information
  if (!GM.emitDebugInformation() && DbgOpt_VisaTransformInfoPath.empty())
    return false;

  const auto &BC = getAnalysis<GenXBackendConfig>();
  const FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();

  VISABuilder *VB = GM.GetCisaBuilder();
  if (GM.HasInlineAsm() || !BC.getVISALTOStrings().empty())
    VB = GM.GetVISAAsmReader();
  IGC_ASSERT(VB);

  const auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  ModuleToVisaTransformInfo MVTI(*VB, FGA, CG);
  if (!DbgOpt_VisaTransformInfoPath.empty()) {
    std::string SerializedMVTI;
    llvm::raw_string_ostream OS(SerializedMVTI);
    MVTI.print(OS);
    vc::produceAuxiliaryShaderDumpFile(BC, DbgOpt_VisaTransformInfoPath,
                                       OS.str());
  }
  LLVM_DEBUG(MVTI.print(dbgs()); dbgs() << "\n");

  if (!GM.emitDebugInformation())
    return false;

  IGC::DebugEmitterOpts DebugInfoOpts;
  fillDbgInfoOptions(BC, DebugInfoOpts);

  for (const Function *PF : MVTI.getPrimaryFunctions())
    processPrimaryFunction(DebugInfoOpts, MVTI, GM, *VB, *PF);

  return false;
}

char GenXDebugInfo::ID = 0;

namespace llvm {

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
INITIALIZE_PASS_DEPENDENCY(GenXVisaRegAllocWrapper)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                    true /*analysis*/)
