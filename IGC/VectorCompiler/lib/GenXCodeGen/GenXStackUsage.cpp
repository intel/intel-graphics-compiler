/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXStackUsage is a module pass whose purpose is to analyse allocas
/// and spot possible places in code where memory may be exhausted
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include <llvm/Analysis/CallGraph.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/GenXIntrinsics/GenXMetadata.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/Debug.h>

#include "Probe/Assertion.h"

#include <sstream>

using namespace llvm;

#define DEBUG_TYPE "GENX_STACK_USAGE"

static cl::opt<bool>
    PerformStackAnalysis("stack-analysis", cl::init(true), cl::Hidden,
                         cl::desc("Perform static stack analysis to generate "
                                  "warning in case of stack overflow"));

namespace {

//--------------------------------------------------------------------
// GenXStackUsage pass. Analyzes allocas

class GenXStackUsage : public ModulePass {
public:
  static char ID;

  explicit GenXStackUsage() : ModulePass(ID) {}
  ~GenXStackUsage() = default;

  StringRef getPassName() const override { return "GenX stack usage"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

} // namespace

namespace llvm {
void initializeGenXStackUsagePass(PassRegistry &);
}

char GenXStackUsage::ID = 0;
INITIALIZE_PASS_BEGIN(GenXStackUsage, "GenXStackUsage", "GenXStackUsage", false,
                      true /*analysis*/)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXStackUsage, "GenXStackUsage", "GenXStackUsage", false,
                    true /*analysis*/)

ModulePass *llvm::createGenXStackUsagePass() {
  initializeGenXStackUsagePass(*PassRegistry::getPassRegistry());
  return new GenXStackUsage;
}

void GenXStackUsage::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesAll();
}

class StackAnalysis : public InstVisitor<StackAnalysis> {
  DataLayout const &m_DL;
  CallGraph const &m_CG;
  uint64_t const m_MaxStackSize{};

  // FunctionState contains information about function:
  // m_UsedSz => how much stack memory it takes within with called from it the
  //  most heavy function
  // m_pHeavyFunction => pointer to function that occupies
  //  the most stack memory
  // m_ProcessingFlag => current state of function
  struct FunctionState final {
    // enumeration used to diagnose recursion
    enum class ProcessingState {
      Started,   // function started to be processed but did not finish
      Finished,  // function has completely finished being processed
      NotStarted // function has not started processing but will start
    };
    uint64_t m_UsedSz{0};
    unsigned m_RequiredAlign{0};
    bool m_HasIndirect{false};
    Function *m_pHeavyFunction{nullptr};
    ProcessingState m_ProcessingFlag{ProcessingState::NotStarted};
  };

  // map between Function and its State
  std::unordered_map<Function *, FunctionState> m_ProcessedFs{};

  llvm::Optional<std::pair<uint64_t, unsigned>> checkFunction(Function &F);
  std::string GenerateCallSequence(Function &F);
  void checkKernel(Function &Kernel);

public:
  StackAnalysis() = delete;
  StackAnalysis(DataLayout const &DL, CallGraph const &CG,
                uint64_t MaxStackSize)
      : m_DL{DL}, m_CG{CG}, m_MaxStackSize{MaxStackSize} {}

  void visitAllocaInst(AllocaInst &AI);
  void visitCallInst(CallInst &CI);
  void visitFunction(Function &F);

  void doAnalysis(Module &M);
};

// Collect all allocas and updates stack usage of each function
void StackAnalysis::visitAllocaInst(AllocaInst &AI) {
  IGC_ASSERT_MESSAGE(AI.isStaticAlloca(), "Non-static alloca not supported");
  auto AllocaSize = llvm::divideCeil(*AI.getAllocationSizeInBits(m_DL),
                                     genx::ByteBits);
  auto AllocaAlign = std::max(AI.getAlignment(), visa::BytesPerSVMPtr);

  auto &CurFuncState = m_ProcessedFs[AI.getFunction()];
  CurFuncState.m_UsedSz = llvm::alignTo(CurFuncState.m_UsedSz, AllocaAlign);
  CurFuncState.m_UsedSz += AllocaSize;
  CurFuncState.m_RequiredAlign = std::max(CurFuncState.m_RequiredAlign,
                                          AllocaAlign);
}

// Check for indirect calls
void StackAnalysis::visitCallInst(CallInst &CI) {
  if (CI.isIndirectCall())
    m_ProcessedFs[CI.getFunction()].m_HasIndirect = true;
}

// Add function to map
void StackAnalysis::visitFunction(Function &F) {
  bool isInserted = m_ProcessedFs.insert({&F, {}}).second;
  IGC_ASSERT_MESSAGE(isInserted, "Error in insertion function in map");
}

// Check CallGraph and usage of allocas in function
llvm::Optional<std::pair<uint64_t, unsigned>>
StackAnalysis::checkFunction(Function &F) {
  auto pOnF = m_ProcessedFs.find(&F);
  IGC_ASSERT_MESSAGE(pOnF != m_ProcessedFs.end(),
                     "Function must be inserted before checking");

  auto &StateOfF = pOnF->second;

  // Can't predict stack usage if there are indirect calls
  if (StateOfF.m_HasIndirect)
    return None;

  // if function is stack call, we do not know stack usage
  if (vc::requiresStackCall(&F))
    return None;

  StateOfF.m_ProcessingFlag = FunctionState::ProcessingState::Started;

  uint64_t MostUsedStackSize = 0;
  for (auto &N : *m_CG[&F]) {
    Function *NextCalledF = N.second->getFunction();
    if (!NextCalledF || NextCalledF->isDeclaration()) {
      LLVM_DEBUG(dbgs() << (NextCalledF ? NextCalledF->getName() : "(null)")
                        << " is declaration\n");
      continue;
    }

    uint64_t UsedStackSize = 0;
    switch (m_ProcessedFs[NextCalledF].m_ProcessingFlag) {
    case FunctionState::ProcessingState::Started: {
      vc::diagnose(
          F.getContext(), "StackUsage",
          "Recursion has been found in call graph. Called function: \"" +
              NextCalledF->getName() + "\" from \"" + F.getName() +
              "\"\nStack overflow can occur, but cannot be diagnosed.",
          DS_Warning);
      return None;
    }
    case FunctionState::ProcessingState::NotStarted: {
      auto Res = checkFunction(*NextCalledF);
      if (!Res)
        return None;
      std::tie(UsedStackSize, std::ignore) = *Res;
      break;
    }
    case FunctionState::ProcessingState::Finished:
      UsedStackSize = m_ProcessedFs[NextCalledF].m_UsedSz;
      break;
    }

    LLVM_DEBUG(dbgs() << "Candidate size from " << NextCalledF->getName()
                      << " = " << UsedStackSize << "\n");
    if (UsedStackSize > MostUsedStackSize) {
      MostUsedStackSize = UsedStackSize;
      StateOfF.m_pHeavyFunction = NextCalledF;
    }
  }

  StateOfF.m_ProcessingFlag = FunctionState::ProcessingState::Finished;
  StateOfF.m_UsedSz += MostUsedStackSize;
  // Add the max alignment of the function to the total size used because of
  // run-time alignment that may vary from 0 to m_RequiredAlign - 1.
  StateOfF.m_UsedSz += StateOfF.m_RequiredAlign;

  LLVM_DEBUG(dbgs() << F.getName() << " size: " << StateOfF.m_UsedSz
                    << " alignment: " << StateOfF.m_RequiredAlign << "\n");

  return std::make_pair(StateOfF.m_UsedSz, StateOfF.m_RequiredAlign);
}

// Generate trace of functions most occupy stack memory
std::string StackAnalysis::GenerateCallSequence(Function &F) {
  auto &FunctionState = m_ProcessedFs[&F];
  std::string FunctionDump =
      F.getName().str() + '(' + std::to_string(FunctionState.m_UsedSz) + ')';

  if (FunctionState.m_pHeavyFunction)
    return FunctionDump + "->" +
           GenerateCallSequence(*FunctionState.m_pHeavyFunction);
  else
    return FunctionDump;
}

// Start from kernel and generate warning in case of possible stack overflow
void StackAnalysis::checkKernel(Function &Kernel) {
  LLVM_DEBUG(dbgs() << "Processing kernel: " << Kernel.getName() << "\n");
  auto Res = checkFunction(Kernel);
  // if we detected recursion or indirect call inside checkFunction
  if (!Res) {
    LLVM_DEBUG(
        dbgs() << "Stack usage analysis stuck on recursion or indirect call ("
               << Kernel.getName() << ")\n");
    return;
  }
  auto [KernelUsedStack, KernelAlignment] = *Res;

  KernelAlignment = std::max(KernelAlignment, visa::BytesPerSVMPtr);
  // align stack size to kernel alignment requirement
  KernelUsedStack = llvm::alignTo(KernelUsedStack, KernelAlignment);
  if (KernelUsedStack > m_MaxStackSize) {
    vc::diagnose(Kernel.getContext(), "StackUsage",
                 "Kernel \"" + Kernel.getName() +
                     "\" may overflow stack. Used " +
                     std::to_string(KernelUsedStack) + " bytes of " +
                     std::to_string(m_MaxStackSize) +
                     "\nCalls: " + GenerateCallSequence(Kernel),
                 DS_Warning);
    return;
  }

  IGC_ASSERT(!Kernel.hasFnAttribute(vc::FunctionMD::VCStackAmount));
  LLVM_DEBUG(dbgs() << "Used stack: " << KernelUsedStack << " ("
                    << Kernel.getName() << ")\n");

  std::ostringstream Os;
  Os << KernelUsedStack;
  Kernel.addFnAttr(vc::FunctionMD::VCStackAmount, Os.str());
}

void StackAnalysis::doAnalysis(Module &M) {
  std::vector<Function *> Kernels;
  Kernels.reserve(M.size());
  for (auto &F : M) {
    visit(F);
    if (vc::isKernel(&F))
      Kernels.push_back(&F);
  }

  for (auto *Kernel : Kernels)
    checkKernel(*Kernel);
}

/***********************************************************************
 * runOnModule : run GenXStackUsage analysis
 *
 * Allocas are processed
 */
bool GenXStackUsage::runOnModule(Module &M) {
  if (!PerformStackAnalysis)
    return false;
  auto ST = &getAnalysis<TargetPassConfig>()
                 .getTM<GenXTargetMachine>()
                 .getGenXSubtarget();
  auto BEConf = &getAnalysis<GenXBackendConfig>();

  bool ModuleModified = false;

  auto MemSize = BEConf->getStatelessPrivateMemSize();
  if (!ST->isOCLRuntime())
    MemSize = visa::StackPerThreadScratch;

  const DataLayout &DL = M.getDataLayout();
  CallGraph CG(M);

  StackAnalysis SA{DL, CG, MemSize};
  SA.doAnalysis(M);

  return ModuleModified;
}
