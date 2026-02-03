/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>
#include <sstream>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

static cl::opt<bool> PrintWiaCheck("print-wia-check", cl::init(false), cl::Hidden,
                                   cl::desc("Debug wia-check analysis"));

// Register pass to igc-opt
#define PASS_FLAG "igc-wi-analysis"
#define PASS_DESCRIPTION "WIAnalysis provides work item dependency info"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(TranslationTable)
IGC_INITIALIZE_PASS_END(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIAnalysis::ID = 0;

WIAnalysis::WIAnalysis() : FunctionPass(ID) { initializeWIAnalysisPass(*PassRegistry::getPassRegistry()); }

const unsigned int WIAnalysisRunner::MinIndexBitwidthToPreserve = 16;

// For dumpping WIA info per each invocation
DenseMap<const Function *, int> WIAnalysisRunner::m_funcInvocationId;

/// Define shorter names for dependencies, for clarity of the conversion maps
/// Note that only UGL/UWG/UTH/RND are supported.
#define UGL WIAnalysis::UNIFORM_GLOBAL
#define UWG WIAnalysis::UNIFORM_WORKGROUP
#define UTH WIAnalysis::UNIFORM_THREAD
#define SEQ WIAnalysis::CONSECUTIVE
#define PTR WIAnalysis::PTR_CONSECUTIVE
#define STR WIAnalysis::STRIDED
#define RND WIAnalysis::RANDOM

static const char *const dep_str[] = {"uniform_global", "uniform_workgroup", "uniform_thread", "consecu",
                                      "p_conse",        "strided",           "random "};

/// Dependency maps (define output dependency according to 2 input deps)

static const WIAnalysis::WIDependancy add_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UGL, UWG, UTH, SEQ, PTR, STR, RND */
    // clang-format off
    /* UGL */  {UGL, UWG, UTH, SEQ, PTR, STR, RND},
    /* UWG */  {UWG, UWG, UTH, SEQ, PTR, STR, RND},
    /* UTH */  {UTH, UTH, UTH, SEQ, PTR, STR, RND},
    /* SEQ */  {SEQ, SEQ, SEQ, STR, STR, STR, RND},
    /* PTR */  {PTR, PTR, PTR, STR, STR, STR, RND},
    /* STR */  {STR, STR, STR, STR, STR, STR, RND},
    /* RND */  {RND, RND, RND, RND, RND, RND, RND}
    // clang-format on
};

static const WIAnalysis::WIDependancy
    // clang-format off
sub_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UGL, UWG, UTH, SEQ, PTR, STR, RND */
    /* UGL */  {UGL, UWG, UTH, STR, RND, RND, RND},
    /* UWG */  {UWG, UWG, UTH, STR, RND, RND, RND},
    /* UTH */  {UTH, UTH, UTH, STR, RND, RND, RND},
    /* SEQ */  {SEQ, SEQ, SEQ, RND, RND, RND, RND},
    /* PTR */  {PTR, PTR, PTR, RND, RND, RND, RND},
    /* STR */  {STR, STR, STR, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND, RND, RND}
        // clang-format on
};

static const WIAnalysis::WIDependancy mul_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    // clang-format off
    /*          UGL, UWG, UTH, SEQ, PTR, STR, RND */
    /* UGL */  {UGL, UWG, UTH, STR, STR, STR, RND},
    /* UWG */  {UWG, UWG, UTH, STR, STR, STR, RND},
    /* UTH */  {UTH, UTH, UTH, STR, STR, STR, RND},
    /* SEQ */  {STR, STR, STR, RND, RND, RND, RND},
    /* PTR */  {STR, STR, STR, RND, RND, RND, RND},
    /* STR */  {STR, STR, STR, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND, RND, RND}
    // clang-format on
};

// select is to have a weaker dep of two
static const WIAnalysis::WIDependancy select_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    // clang-format off
    /*          UGL, UWG, UTH, SEQ, PTR, STR, RND */
    /* UGL */  {UGL, UWG, UTH, STR, STR, STR, RND},
    /* UWG */  {UWG, UWG, UTH, STR, STR, STR, RND},
    /* UTH */  {UTH, UTH, UTH, STR, STR, STR, RND},
    /* SEQ */  {STR, STR, STR, SEQ, STR, STR, RND},
    /* PTR */  {STR, STR, STR, STR, PTR, STR, RND},
    /* STR */  {STR, STR, STR, STR, STR, STR, RND},
    /* RND */  {RND, RND, RND, RND, RND, RND, RND}
    // clang-format on
};

static const WIAnalysis::WIDependancy gep_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    // clang-format off
    /* ptr\index UGL, UWG, UTH, SEQ, PTR, STR, RND */
    /* UGL */  {UGL, UWG, UTH, PTR, RND, RND, RND},
    /* UWG */  {UWG, UWG, UTH, PTR, RND, RND, RND},
    /* UTH */  {UTH, UTH, UTH, PTR, RND, RND, RND},
    /* SEQ */  {RND, RND, RND, RND, RND, RND, RND},
    /* PTR */  {PTR, PTR, PTR, RND, RND, RND, RND},
    /* STR */  {RND, RND, RND, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND, RND, RND}
    // clang-format on
};

// For better readability, the rank of a dependency is used to compare two dependencies
// to see which of them is weaker or stronger.
//
//   Dependancy rank : an integer value for each Dependancy, starting from 0.
//   Property of rank: the lower (smaller) the rank, the stronger the dependancy.
//
// Currently, enum value of each dependency is used exactly as its rank.
inline int depRank(WIAnalysis::WIDependancy D) { return (int)D; }

namespace IGC {
/// @Brief, given a conditional branch and its immediate post dominator,
/// find its influence-region and partial joins within the influence region
class BranchInfo {
public:
  BranchInfo(const IGCLLVM::TerminatorInst *inst, const llvm::BasicBlock *ipd);

  void print(llvm::raw_ostream &OS) const;

  const IGCLLVM::TerminatorInst *cbr;
  const llvm::BasicBlock *full_join;
  llvm::DenseSet<llvm::BasicBlock *> influence_region;
  llvm::SmallPtrSet<llvm::BasicBlock *, 4> partial_joins;
};
} // namespace IGC

void WIAnalysisRunner::print(raw_ostream &OS, const Module *) const {
  DenseMap<BasicBlock *, int> BBIDs;
  int id = 0;
  for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E; ++I, ++id) {
    BasicBlock *BB = &*I;
    BBIDs[BB] = id;
  }

  std::stringstream ss;
  ss << "WIAnalysis: " << m_func->getName().str();
  Banner(OS, ss.str());

  OS << "Args: \n";
  for (Function::arg_iterator I = m_func->arg_begin(), E = m_func->arg_end(); I != E; ++I) {
    Value *AVal = &*I;

    if (m_depMap.GetAttributeWithoutCreating(AVal) != m_depMap.end())
      OS << "    " << dep_str[m_depMap.GetAttributeWithoutCreating(AVal)] << " " << *AVal << "\n";
    else
      OS << "  unknown " << *AVal << "\n";
  }
  OS << "\n";

  for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E; ++I) {
    BasicBlock *BB = &*I;
    OS << "BB:" << BBIDs[BB];
    if (BB->hasName())
      OS << " " << BB->getName();
    OS << "       ; preds =";
    bool isFirst = true;
    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
      BasicBlock *pred = *PI;
      OS << ((isFirst) ? " " : ", ") << "BB:" << BBIDs[pred] << "  ";
      if (pred->hasName())
        OS << pred->getName();
      isFirst = false;
    }
    {
      auto dep = getCFDependency(BB);
      OS << "[ " << dep_str[dep] << " ]";
    }
    OS << "\n";
    for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
      Instruction *I = &*it;
      if (m_depMap.GetAttributeWithoutCreating(I) != m_depMap.end()) {
        OS << "  " << dep_str[m_depMap.GetAttributeWithoutCreating(I)] << " " << *I;
      } else {
        OS << "  unknown " << *I;
      }
      if (I->isTerminator()) {
        IGCLLVM::TerminatorInst *TI = dyn_cast<IGCLLVM::TerminatorInst>(I);
        OS << " [";
        for (unsigned i = 0, e = TI->getNumSuccessors(); i < e; ++i) {
          BasicBlock *succ = TI->getSuccessor(i);
          OS << " BB:" << BBIDs[succ];
        }
        OS << " ]";
      }
      OS << "\n";
    }
    OS << "\n";
  }
}

void WIAnalysisRunner::lock_print() {
  IGC::Debug::DumpLock();
  {
    int id = m_funcInvocationId[m_func]++;
    std::stringstream ss;
    ss << m_func->getName().str() << "_WIAnalysis_" << id;
    auto name = DumpName(IGC::Debug::GetShaderOutputName())
                    .Hash(m_CGCtx->hash)
                    .Type(m_CGCtx->type)
                    .Pass(ss.str().c_str())
                    .Extension("txt");
    print(Dump(name, DumpType::DBG_MSG_TEXT).stream());
  }
  IGC::Debug::DumpUnlock();
}

// Used for dumpping into a file with a fixed name while running in debugger
void WIAnalysisRunner::dump() const {
  auto name = DumpName(IGC::Debug::GetShaderOutputName())
                  .Hash(m_CGCtx->hash)
                  .Type(m_CGCtx->type)
                  .Pass("WIAnalysis")
                  .Extension("txt");
  print(Dump(name, DumpType::DBG_MSG_TEXT).stream());
}

void WIAnalysisRunner::init(llvm::Function *F, llvm::LoopInfo *LI, llvm::DominatorTree *DT,
                            llvm::PostDominatorTree *PDT, IGC::IGCMD::MetaDataUtils *MDUtils,
                            IGC::CodeGenContext *CGCtx, IGC::ModuleMetaData *ModMD, IGC::TranslationTable *TransTable,
                            bool ForCodegen) {
  m_func = F;
  this->LI = LI;
  this->DT = DT;
  this->PDT = PDT;
  this->ForCodegen = ForCodegen;
  m_pMdUtils = MDUtils;
  m_CGCtx = CGCtx;
  m_ModMD = ModMD;
  m_TT = TransTable;

  // CS uniformness
  m_localIDxUniform = false;
  m_localIDyUniform = false;
  m_localIDzUniform = false;
}

bool WIAnalysisRunner::run() {
  auto &F = *m_func;
  if (m_pMdUtils->findFunctionsInfoItem(&F) == m_pMdUtils->end_FunctionsInfo())
    return false;
  if (IGC::isIntelSymbolTableVoidProgram(&F))
    return false;

  m_depMap.Initialize(m_TT);
  m_TT->RegisterListener(&m_depMap);

  m_changed1.clear();
  m_changed2.clear();
  m_pChangedNew = &m_changed1;
  m_pChangedOld = &m_changed2;
  m_ctrlBranches.clear();

  m_storeDepMap.clear();
  m_allocaDepMap.clear();
  m_forcedUniforms.clear();

  updateArgsDependency(&F);

  if (!IGC_IS_FLAG_ENABLED(DisableUniformAnalysis)) {
    // Compute the  first iteration of the WI-dep according to ordering
    // instructions this ordering is generally good (as it ususally correlates
    // well with dominance).
    inst_iterator it = inst_begin(F);
    inst_iterator e = inst_end(F);
    for (; it != e; ++it) {
      calculate_dep(&*it);
    }

    // Recursively check if WI-dep changes and if so reclaculates
    // the WI-dep and marks the users for re-checking.
    // This procedure is guranteed to converge since WI-dep can only
    // become less unifrom (uniform->consecutive->ptr->stride->random).
    updateDeps();

    // sweep the dataflow started from those GenISA_vectorUniform,
    // force all the insert-elements and phi-nodes to uniform
    std::set<const Value *> visited;
    while (!m_forcedUniforms.empty()) {
      const Value *V = m_forcedUniforms.back();
      m_forcedUniforms.pop_back();
      visited.insert(V);
      for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
        const Value *use = (*UI);
        if (!visited.count(use) && use->getType() == V->getType()) {
          if (auto INS = dyn_cast<InsertElementInst>(use)) {
            if (!isUniform(use))
              m_depMap.SetAttribute(INS, WIAnalysis::UNIFORM_THREAD);
            m_forcedUniforms.push_back(use);
          } else if (auto PHI = dyn_cast<PHINode>(use)) {
            if (!isUniform(use))
              m_depMap.SetAttribute(PHI, WIAnalysis::UNIFORM_THREAD);
            m_forcedUniforms.push_back(use);
          }
        }
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(DumpWIA)) {
    // Dump into a unique file under dump dir, per each function invocation.
    lock_print();
  }

  // Original print to stdout
  //   Need igc key PrintToConsole and llvm flag -print-wia-check
  if (PrintWiaCheck) {
    print(ods());
  }
  return false;
}

bool WIAnalysis::runOnFunction(Function &F) {
  auto *MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  auto *PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  auto *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto *CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto *ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  auto *pTT = &getAnalysis<TranslationTable>();

  Runner.init(&F, LI, DT, PDT, MDUtils, CGCtx, ModMD, pTT);
  return Runner.run();
}

void WIAnalysisRunner::updateDeps() {
  // As lonst as we have values to update
  while (!m_pChangedNew->empty()) {
    // swap between changedSet pointers - recheck the newChanged(now old)
    std::swap(m_pChangedNew, m_pChangedOld);
    // clear the newChanged set so it will be filled with the users of
    // instruction which their WI-dep canged during the current iteration
    m_pChangedNew->clear();

    // update all changed values
    std::vector<const Value *>::iterator it = m_pChangedOld->begin();
    std::vector<const Value *>::iterator e = m_pChangedOld->end();
    for (; it != e; ++it) {
      // remove first instruction
      // calculate its new dependencey value
      calculate_dep(*it);
    }
  }
}

bool WIAnalysisRunner::isInstructionSimple(const Instruction *inst) {
  // avoid changing cb load to sampler load, since sampler load
  // has longer latency.
  if (isa<LoadInst>(inst)) {
    return false;
  }

  if (isa<UnaryInstruction>(inst) || isa<BinaryOperator>(inst) || isa<CmpInst>(inst) || isa<SelectInst>(inst)) {
    return true;
  }
  if (IsMathIntrinsic(GetOpCode((Instruction *)inst))) {
    return true;
  }

  return false;
}

bool WIAnalysisRunner::needToBeUniform(const Value *val) {
  for (auto UI = val->user_begin(), E = val->user_end(); UI != E; ++UI) {
    if (const RTWriteIntrinsic *use = dyn_cast<RTWriteIntrinsic>(*UI)) {
      if (use->getSampleIndex() == val || use->getBlendStateIndex() == val) {
        return true;
      }
    }
    // TODO add sampler cases
  }
  return false;
}

bool WIAnalysisRunner::allUsesRandom(const Value *val) {
  for (auto UI = val->user_begin(), E = val->user_end(); UI != E; ++UI) {
    const Value *use = (*UI);
    if (getDependency(use) != WIAnalysis::RANDOM) {
      return false;
    }
  }
  return true;
}

void WIAnalysisRunner::updateArgsDependency(llvm::Function *pF) {
  /*
  Function Signature: define void @kernel(
      [OCL function args...],
      [implicit args...],
      [push analysis args...])

  Example push analysis args:
      float %urb_read_0, float %urb_read_1, float %urb_read_2, float %urb_read_3, float %urb_read_4

  Metadata Generated:
  !igc.pushanalysis.wi.info = !{!3, !4, !5, !6, !7}

  !3 = metadata !{metadata !"urb_read_0", i32 0, i32 4}
  !4 = metadata !{metadata !"urb_read_1", i32 1, i32 4}
  !5 = metadata !{metadata !"urb_read_2", i32 2, i32 4}
  !6 = metadata !{metadata !"urb_read_3", i32 3, i32 4}
  !7 = metadata !{metadata !"urb_read_4", i32 4, i32 4}

  Assumption is that the order of metadata matches the order of arguments in function.
  */

  // For a subroutine, conservatively assume that all user provided arguments
  // are random. Note that all other functions are treated as kernels.
  // To enable subroutine for other FEs, we need to update this check.
  bool IsSubroutine = !isEntryFunc(m_pMdUtils, pF) || isNonEntryMultirateShader(pF);

  ImplicitArgs implicitArgs(*pF, m_pMdUtils);
  int implicitArgStart = (unsigned)(pF->arg_size() - implicitArgs.size() -
                                    (IsSubroutine ? 0 : m_ModMD->pushInfo.pushAnalysisWIInfos.size()));
  IGC_ASSERT_MESSAGE(implicitArgStart >= 0, "Function arg size does not match meta data and push args.");

  llvm::Function::arg_iterator ai, ae;
  ai = pF->arg_begin();
  ae = pF->arg_end();

  // 1. add all kernel function args as uniform, or
  //    add all subroutine function args as random
  for (int i = 0; i < implicitArgStart; ++i, ++ai) {
    IGC_ASSERT(ai != ae);
    incUpdateDepend(&(*ai), IsSubroutine ? WIAnalysis::RANDOM : WIAnalysis::UNIFORM_GLOBAL);
  }

  // 2. add implicit args
  //    By default, local IDs are not uniform. But if we know that the runtime dispatchs
  //    order (intel_reqd_workgroup_walk_order()) and work group size (reqd_work_group_size()),
  //    we may derive that some of local IDs are uniform.
  bool localX_uniform = false, localY_uniform = false, localZ_uniform = false;

  if (!IsSubroutine) {
    checkLocalIdUniform(pF, localX_uniform, localY_uniform, localZ_uniform);
  }

  for (unsigned i = 0; i < implicitArgs.size(); ++i, ++ai) {
    IGC_ASSERT(ai != ae);
    const ImplicitArg &iArg = implicitArgs[ai->getArgNo() - implicitArgStart];
    WIAnalysis::WIDependancy dependency = iArg.getDependency();
    if ((localX_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_X) ||
        (localY_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_Y) ||
        (localZ_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_Z)) {
      // todo: may improve it to have UNIFORM_WORKGROUP
      dependency = WIAnalysis::UNIFORM_THREAD;
    }

    incUpdateDepend(&(*ai), dependency);
  }

  // 3. add push analysis args
  if (!IsSubroutine) {
    for (unsigned i = 0; i < m_ModMD->pushInfo.pushAnalysisWIInfos.size(); ++i, ++ai) {
      IGC_ASSERT(ai != ae);
      WIAnalysis::WIDependancy dependency =
          static_cast<WIAnalysis::WIDependancy>(m_ModMD->pushInfo.pushAnalysisWIInfos[i].argDependency);
      incUpdateDepend(&(*ai), dependency);
    }
  }
}

void WIAnalysis::print(llvm::raw_ostream &OS, const llvm::Module *M) const { Runner.print(OS, M); }

void WIAnalysis::dump() const { Runner.dump(); }

void WIAnalysis::incUpdateDepend(const llvm::Value *val, WIDependancy dep) { Runner.incUpdateDepend(val, dep); }

WIAnalysis::WIDependancy WIAnalysis::whichDepend(const llvm::Value *val) { return Runner.whichDepend(val); }

bool WIAnalysis::isUniform(const Value *val) const { return Runner.isUniform(val); }

bool WIAnalysis::isGlobalUniform(const Value *val) { return Runner.isGlobalUniform(val); }

bool WIAnalysis::isWorkGroupOrGlobalUniform(const Value *val) { return Runner.isWorkGroupOrGlobalUniform(val); }

bool WIAnalysis::insideDivergentCF(const Value *val) const { return Runner.insideDivergentCF(val); }

bool WIAnalysis::insideWorkgroupDivergentCF(const Value *val) const { return Runner.insideWorkgroupDivergentCF(val); }

WIAnalysis::WIDependancy WIAnalysisRunner::whichDepend(const Value *val) const {
  IGC_ASSERT_MESSAGE(m_pChangedNew->empty(), "set should be empty before query");
  IGC_ASSERT_MESSAGE(nullptr != val, "Bad value");
  if (isa<Constant>(val)) {
    return WIAnalysis::UNIFORM_GLOBAL;
  } else if (isa<StaticConstantPatchIntrinsic>(val)) {
    return WIAnalysis::UNIFORM_GLOBAL;
  }
  auto EL = m_depMap.GetAttributeWithoutCreating(val);
  if (IGC_IS_FLAG_ENABLED(DisableUniformAnalysis)) {
    if (EL == m_depMap.end()) {
      return WIAnalysis::RANDOM;
    }
  }
  IGC_ASSERT(EL != m_depMap.end());
  return EL;
}

bool WIAnalysisRunner::isUniform(const Value *val) const {
  if (!hasDependency(val))
    return false;

  return WIAnalysis::isDepUniform(whichDepend(val));
}

bool WIAnalysisRunner::isWorkGroupOrGlobalUniform(const Value *val) const {
  if (!hasDependency(val))
    return false;
  WIAnalysis::WIDependancy dep = whichDepend(val);
  return dep == WIAnalysis::UNIFORM_GLOBAL || dep == WIAnalysis::UNIFORM_WORKGROUP;
}

bool WIAnalysisRunner::isGlobalUniform(const Value *val) const {
  if (!hasDependency(val))
    return false;
  WIAnalysis::WIDependancy dep = whichDepend(val);
  return dep == WIAnalysis::UNIFORM_GLOBAL;
}

WIAnalysis::WIDependancy WIAnalysisRunner::getCFDependency(const BasicBlock *BB) const {
  auto II = m_ctrlBranches.find(BB);
  if (II == m_ctrlBranches.end())
    return WIAnalysis::UNIFORM_GLOBAL;

  WIAnalysis::WIDependancy dep = WIAnalysis::UNIFORM_GLOBAL;
  for (auto *BI : II->second) {
    auto newDep = whichDepend(BI);
    if (depRank(dep) < depRank(newDep))
      dep = newDep;
  }

  return dep;
}

bool WIAnalysisRunner::insideWorkgroupDivergentCF(const Value *val) const {
  if (auto *I = dyn_cast<Instruction>(val)) {
    auto dep = getCFDependency(I->getParent());
    return depRank(dep) > WIAnalysis::UNIFORM_WORKGROUP;
  }

  return false;
}

WIAnalysis::WIDependancy WIAnalysisRunner::getDependency(const Value *val) {
  if (m_depMap.GetAttributeWithoutCreating(val) == m_depMap.end()) {
    // Make sure that constants are not added in the map.
    if (!isa<Instruction>(val) && !isa<Argument>(val)) {
      return WIAnalysis::UNIFORM_GLOBAL;
    }
    // Don't expect this happens, let's assertion fail
    IGC_ASSERT_MESSAGE(0, "Dependence for 'val' should bave been set already!");
  }
  IGC_ASSERT(m_depMap.GetAttributeWithoutCreating(val) != m_depMap.end());
  return m_depMap.GetAttributeWithoutCreating(val);
}

bool WIAnalysisRunner::hasDependency(const Value *val) const {

  if (!isa<Instruction>(val) && !isa<Argument>(val)) {
    return true;
  }
  return (m_depMap.GetAttributeWithoutCreating(val) != m_depMap.end());
}

static bool HasPhiUse(const llvm::Value *inst) {
  for (auto UI = inst->user_begin(), E = inst->user_end(); UI != E; ++UI) {
    if (llvm::isa<llvm::PHINode>(*UI)) {
      return true;
    }
  }
  return false;
}

void WIAnalysisRunner::calculate_dep(const Value *val) {
  IGC_ASSERT_MESSAGE(nullptr != val, "Bad value");

  // Not an instruction, must be a constant or an argument
  // Could this vector type be of a constant which
  // is not uniform ?
  IGC_ASSERT_MESSAGE(isa<Instruction>(val), "Could we reach here with non instruction value?");

  const Instruction *const inst = dyn_cast<Instruction>(val);
  IGC_ASSERT_MESSAGE(nullptr != inst, "This Value is not an Instruction");
  if (inst) {
    bool hasOriginal = hasDependency(inst);
    WIAnalysis::WIDependancy orig;
    // We only calculate dependency on unset instructions if all their operands
    // were already given dependency. This is good for compile time since these
    // instructions will be visited again after the operands dependency is set.
    // An exception are phi nodes since they can be the ancestor of themselves in
    // the def-use chain. Note that in this case we force the phi to have the
    // pre-header value already calculated.
    //
    // Another case is that an inst might be set under control dependence (for example, phi)
    // before any of its operands have been set. In this case, we will skip here. Here
    // is the example (derived from ocl scheduler):
    //      B0:  (p) goto Bt
    //      B1:  goto Bf
    //  L   B2:  x.lcssa = phi (x.0, Bn)      // B2: partial join
    //      ...
    //      Bt: ...
    //      ...
    //      Bf:
    //      ...
    //          goto Bm (out of loop)
    //      Bn:
    //          x.0 = ...
    //          goto  B2
    //      Bm:  ...
    //      ...
    //      B_ipd  ( iPDOM(B0) = B_ipd)
    //
    // B0's branch instruction has random dependency, which triggers control dependence calculation.
    // B2 is a partial join in InfluenceRegion. Thus its phi is marked as random, but its operand
    // x.0 is still not set yet.
    unsigned int unsetOpNum = 0;
    for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
      if (!hasDependency(inst->getOperand(i)))
        unsetOpNum++;
    }
    if (isa<PHINode>(inst)) {
      // We do not calculate PhiNode with all incoming values unset.
      //
      // This seems right as we don't expect a phi that only depends upon other
      // phi's (if it happens, those phis form a cycle dependency) so any phi's
      // calculation will eventually be triggered from calculating a non-phi one
      // which the phi depends upon.
      if (unsetOpNum == inst->getNumOperands())
        return;
    } else {
      // We do not calculate non-PhiNode instruction that have unset operands
      if (unsetOpNum > 0)
        return;

      // We have all operands set. Check a special case from calculate_dep for
      // binary ops (see the details below). It checks for ASHR+ADD and ASHR+SHL
      // cases, and in particular it accesses dependency for ADD operands. It
      // could happen these operands are not processed yet and in such case
      // getDependency raises the assertion. Thus check if dependency is set.
      // Currently we need to check dependency for ASHR->ADD operands only.
      // For SHR, its operands are checked to be constant so skip this case.
      // This code could be extended further depending on requirements.
      if (inst->getOpcode() == Instruction::AShr) {
        BinaryOperator *op0 = dyn_cast<BinaryOperator>(inst->getOperand(0));
        if (op0 && op0->getOpcode() == Instruction::Add && !hasDependency(op0->getOperand(1))) {
          return;
        }
      }
    }

    if (!hasOriginal) {
      orig = WIAnalysis::UNIFORM_GLOBAL;
    } else {
      orig = m_depMap.GetAttributeWithoutCreating(inst);

      // if inst is already marked random, it cannot get better
      if (orig == WIAnalysis::RANDOM) {
        return;
      }
    }

    WIAnalysis::WIDependancy dep = orig;

    // LLVM does not have compile time polymorphisms
    // TODO: to make things faster we may want to sort the list below according
    // to the order of their probability of appearance.
    if (const BinaryOperator *BI = dyn_cast<BinaryOperator>(inst))
      dep = calculate_dep(BI);
    else if (const CallInst *CI = dyn_cast<CallInst>(inst))
      dep = calculate_dep(CI);
    else if (isa<CmpInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (isa<ExtractElementInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
      dep = calculate_dep(GEP);
    else if (isa<InsertElementInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (isa<InsertValueInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (const PHINode *Phi = dyn_cast<PHINode>(inst))
      dep = calculate_dep(Phi);
    else if (isa<ShuffleVectorInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (isa<StoreInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (inst->isTerminator())
      dep = calculate_dep_terminator(dyn_cast<IGCLLVM::TerminatorInst>(inst));
    else if (const SelectInst *SI = dyn_cast<SelectInst>(inst))
      dep = calculate_dep(SI);
    else if (const AllocaInst *AI = dyn_cast<AllocaInst>(inst))
      dep = calculate_dep(AI);
    else if (const CastInst *CI = dyn_cast<CastInst>(inst))
      dep = calculate_dep(CI);
    else if (isa<ExtractValueInst>(inst))
      dep = calculate_dep_simple(inst);
    else if (const LoadInst *LI = dyn_cast<LoadInst>(inst))
      dep = calculate_dep(LI);
    else if (const VAArgInst *VAI = dyn_cast<VAArgInst>(inst))
      dep = calculate_dep(VAI);
    else if (inst->getOpcode() == Instruction::FNeg)
      dep = calculate_dep_simple(inst);
    else if (inst->getOpcode() == Instruction::Freeze)
      dep = calculate_dep_simple(inst);

    if (m_func->hasFnAttribute("KMPLOCK")) {
      dep = WIAnalysis::UNIFORM_THREAD;
    }

    // Spec enforces subgroup broadcast to use thread-uniform local ID.
    if (isUsedByWaveBroadcastAsLocalID(inst)) {
#ifdef _DEBUG
      // Print warning exactly once per kernel.
      static SmallPtrSet<Function *, 8> detectedKernels;
      if (dep > WIAnalysis::UNIFORM_THREAD && !detectedKernels.count(m_func)) {
        detectedKernels.insert(m_func);
        std::string msg =
            "Detected llvm.genx.GenISA.WaveBroadcast with potentially non-uniform LocalID in kernel " +
            m_func->getName().str() +
            "; such operation doesn't meet specification of OpGroupBroadcast and can lead to unexpected results.";
        m_CGCtx->EmitWarning(msg.c_str());
      }
#endif // DEBUG
    }

    // If the value was changed in this calculation
    if (!hasOriginal || dep != orig) {
      // i1 instructions used in phi cannot be uniform as it may prevent us from removing the phi of 1
      if (ForCodegen && inst->getType()->isIntegerTy(1) && WIAnalysis::isDepUniform(dep) && HasPhiUse(inst)) {
        dep = WIAnalysis::RANDOM;
      }
      // Update dependence of this instruction if dep is weaker than orig.
      // Note depRank(orig) could be higher than depRank(dep) for phi.
      // (Algo will never decrease the rank of a value.)
      WIAnalysis::WIDependancy newDep = depRank(orig) < depRank(dep) ? dep : orig;
      if (!hasOriginal || newDep != orig) {
        // update only if it is a new dep
        updateDepMap(inst, newDep);
      }
      // divergent branch, trigger updates due to control-dependence
      if (inst->isTerminator() && dep != WIAnalysis::UNIFORM_GLOBAL) {
        update_cf_dep(dyn_cast<IGCLLVM::TerminatorInst>(inst));
      }
    }
  }
}

bool WIAnalysisRunner::isRegionInvariant(const llvm::Instruction *defi, BranchInfo *brInfo) {
  constexpr uint8_t MAX_DEPTH = 4;
  struct RegionOperand {
    const llvm::Instruction *inst;
    uint8_t operandNum;
  };

  std::array<RegionOperand, MAX_DEPTH> operands{};
  int ind = 0;
  operands[0] = {defi, 0};

  while (ind >= 0) {
    auto &rop = operands[ind];
    if (isa<PHINode>(rop.inst)) {
      return false;
    }

    if (rop.operandNum < rop.inst->getNumOperands()) {
      Value *op = rop.inst->getOperand(rop.operandNum);
      rop.operandNum++;
      auto *srci = dyn_cast<Instruction>(op);
      if (srci) {
        if (!brInfo->influence_region.contains(srci->getParent())) {
          // go on to check the next operand
          continue;
        }
        if (ind + 2 > MAX_DEPTH) {
          return false;
        }
        ind += 1;
        operands[ind] = {srci, 0};
      }
    } else {
      ind -= 1;
    }
  }
  return true;
}

bool WIAnalysisRunner::isUsedByWaveBroadcastAsLocalID(const llvm::Instruction *inst) {
  for (auto it = inst->users().begin(); it != inst->users().end(); ++it) {
    const GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(*it);
    if (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBroadcast && GII->getOperand(1) == inst)
      return true;
  }

  return false;
}

void WIAnalysisRunner::update_cf_dep(const IGCLLVM::TerminatorInst *inst) {
  IGC_ASSERT(hasDependency(inst));
  WIBaseClass::WIDependancy instDep = getDependency(inst);

  BasicBlock *blk = (BasicBlock *)(inst->getParent());
  BasicBlock *ipd = PDT->getNode(blk)->getIDom()->getBlock();
  // a branch can have NULL immediate post-dominator when a function
  // has multiple exits in llvm-ir
  // compute influence region and the partial-joins
  BranchInfo br_info(inst, ipd);
  // debug: dump influence region and partial-joins
  // br_info.print(ods());
  auto *CbrLoop = LI->getLoopFor(blk);
  // Loop* IPDLoop = nullptr;
  // check dep-type for every phi in the full join
  if (ipd) {
    updatePHIDepAtJoin(ipd, &br_info);
    // IPDLoop = LI->getLoopFor(ipd);
  }
  // check dep-type for every phi in the partial-joins
  for (SmallPtrSet<BasicBlock *, 4>::iterator join_it = br_info.partial_joins.begin(),
                                              join_e = br_info.partial_joins.end();
       join_it != join_e; ++join_it) {
    auto *PJ = *join_it;
    // skip the special loop-entry case
    if (DT->dominates(PJ, blk)) {
      int NumPreds = 0;
      for (auto *pred : predecessors(PJ)) {
        if (br_info.influence_region.count(pred)) {
          NumPreds++;
        }
      }
      if (NumPreds <= 1)
        continue;
    }
    auto *PJNode = DT->getNode(PJ);
    if (PJNode == nullptr) {
      IGC_ASSERT(!DT->isReachableFromEntry(PJ));
      continue;
    }
    auto PJDom = PJNode->getIDom()->getBlock();

    // If both partial-join and it IDom are in partial-join region
    // there are cases in which phi-nodes in partial-joins are not
    // relevant to the cbr under the investigation
    auto LoopA = LI->getLoopFor(PJDom);
    auto LoopB = LI->getLoopFor(PJ);
    if (br_info.partial_joins.count(PJDom)) {
      // both PJ and its IDom are outside the CBR loop
      if (!CbrLoop || !CbrLoop->contains(LoopA))
        continue;
      // CbrLoop contains both PJDom and PJ
      if (CbrLoop->contains(LoopB)) {
        // Either Cbr-block strongly dominates the PJDom
        // Or PJ dominates Cbr-block
        if ((blk != PJDom && DT->dominates(blk, PJDom)) || DT->dominates(PJ, blk))
          continue;
      }
    }
    updatePHIDepAtJoin(PJ, &br_info);
  }

  // walk through all the instructions in the influence-region
  // update the dep-type based upon its uses
  DenseSet<BasicBlock *>::iterator blk_it = br_info.influence_region.begin();
  DenseSet<BasicBlock *>::iterator blk_e = br_info.influence_region.end();
  for (; blk_it != blk_e; ++blk_it) {
    BasicBlock *def_blk = *blk_it;
    // add the branch into the controlling-branch set of the block
    // if the block is in the influence-region
    IGC_ASSERT(def_blk != br_info.full_join);
    m_ctrlBranches[def_blk].insert(inst);

    for (BasicBlock::iterator I = def_blk->begin(), E = def_blk->end(); I != E; ++I) {
      Instruction *defi = &(*I);
      if (hasDependency(defi) && depRank(getDependency(defi)) >= depRank(instDep)) {
        // defi is already weaker than or equal to inst (br), do nothing.
        continue;
      }

      if (const auto *st = dyn_cast<StoreInst>(defi)) {
        // If we encounter a store in divergent control flow,
        // we need to process the associated alloca (if any) again
        // because it might need to be RANDOM.
        auto it = m_storeDepMap.find(st);
        if (it != m_storeDepMap.end())
          m_pChangedNew->push_back(it->second);
      }

      // This is an optimization that tries to detect instruction
      // not really affected by control-flow divergency because
      // all the sources are outside the region.
      // However this is only as good as we can get because we
      // only search limited depth
      if (isRegionInvariant(defi, &br_info)) {
        continue;
      }
      // We need to look at where the use is in order to decide
      // we should make def to be "random" when loop is not in
      // LCSSA form because we do not have LCSSA phi-nodes.
      // 1) if use is in the full-join
      // 2) if use is even outside the full-join
      // 3) if use is in partial-join but def is not in partial-join
      // 4) if def and use are in partial-join but def inside loop
      Value::use_iterator use_it = defi->use_begin();
      Value::use_iterator use_e = defi->use_end();
      for (; use_it != use_e; ++use_it) {
        Instruction *user = dyn_cast<Instruction>((*use_it).getUser());
        IGC_ASSERT(user);
        BasicBlock *user_blk = user->getParent();
        PHINode *phi = dyn_cast<PHINode>(user);
        if (phi) {
          // another place we assume all critical edges have been
          // split and phi-move will be placed on those splitters
          user_blk = phi->getIncomingBlock(*use_it);
        }
        if (user_blk == def_blk) {
          // local def-use, not related to control-dependence
          continue; // skip
        }
        auto DefLoop = LI->getLoopFor(def_blk);
        auto UseLoop = LI->getLoopFor(user_blk);
        if (user_blk == br_info.full_join || !br_info.influence_region.count(user_blk) ||
            (br_info.partial_joins.count(user_blk) &&
             (!br_info.partial_joins.count(def_blk) || (DefLoop && !DefLoop->contains(UseLoop))))) {
          updateDepMap(defi, instDep);
          // break out of the use loop
          // since def is changed to RANDOM, all uses will be changed later
          break;
        }
      } // end of usei loop
    } // end of defi loop within a block
  } // end of influence-region block loop
}

void WIAnalysisRunner::updatePHIDepAtJoin(BasicBlock *blk, BranchInfo *brInfo) {
  // This is to bring down PHI's dep to br's dep.
  // If PHI's dep is already weaker than br's dep, do nothing.
  IGC_ASSERT(hasDependency(brInfo->cbr));
  WIAnalysis::WIDependancy brDep = getDependency(brInfo->cbr);

  for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ++I) {
    Instruction *defi = &(*I);
    PHINode *phi = dyn_cast<PHINode>(defi);
    if (!phi) {
      break;
    }
    if (hasDependency(phi) && depRank(getDependency(phi)) >= depRank(brDep)) {
      // phi's dep is already the same or weaker, do nothing.
      continue;
    }
    Value *trickySrc = nullptr;
    for (unsigned predIdx = 0; predIdx < phi->getNumOperands(); ++predIdx) {
      Value *srcVal = phi->getOperand(predIdx);
      Instruction *defi = dyn_cast<Instruction>(srcVal);
      if (defi && brInfo->influence_region.count(defi->getParent())) {
        updateDepMap(phi, brDep);
        break;
      } else {
        // if the src is an immed, or an argument, or defined outside,
        // think about the phi-move that can be placed in the incoming block.
        // this phi should be random if we have two different src-values like that.
        // this is one place where we assume all critical edges have been split
        BasicBlock *predBlk = phi->getIncomingBlock(predIdx);
        if (brInfo->influence_region.count(predBlk)) {
          if (!trickySrc) {
            trickySrc = srcVal;
          } else if (trickySrc != srcVal) {
            updateDepMap(phi, brDep);
            break;
          }
        }
      }
    }
  }
}

void WIAnalysisRunner::updateDepMap(const Instruction *inst, WIAnalysis::WIDependancy dep) {
  // Save the new value of this instruction
  m_depMap.SetAttribute(inst, dep);
  // Register for update all of the dependent values of this updated
  // instruction.
  Value::const_user_iterator it = inst->user_begin();
  Value::const_user_iterator e = inst->user_end();
  for (; it != e; ++it) {
    m_pChangedNew->push_back(*it);
  }
  if (const StoreInst *st = dyn_cast<StoreInst>(inst)) {
    auto it = m_storeDepMap.find(st);
    if (it != m_storeDepMap.end()) {
      m_pChangedNew->push_back(it->second);
    }
  }

  if (dep == WIAnalysis::RANDOM) {
    EOPCODE eopcode = GetOpCode((Instruction *)inst);
    if (eopcode == llvm_insert) {
      updateInsertElements((const InsertElementInst *)inst);
    } else if (const InsertValueInst *IVI = dyn_cast<const InsertValueInst>(inst)) {
      updateInsertValues(IVI);
    }
  }
}

/// if one of insert-element is random, turn all the insert-elements into random
void WIAnalysisRunner::updateInsertElements(const InsertElementInst *inst) {
  /// find the first one in the sequence
  InsertElementInst *curInst = (InsertElementInst *)inst;
  InsertElementInst *srcInst = dyn_cast<InsertElementInst>(curInst->getOperand(0));
  while (srcInst) {
    if (hasDependency(srcInst) && getDependency(srcInst) == WIAnalysis::RANDOM)
      return;
    curInst = srcInst;
    srcInst = dyn_cast<InsertElementInst>(curInst->getOperand(0));
  }
  if (curInst != inst) {
    m_depMap.SetAttribute(curInst, WIAnalysis::RANDOM);
    Value::user_iterator it = curInst->user_begin();
    Value::user_iterator e = curInst->user_end();
    for (; it != e; ++it) {
      m_pChangedNew->push_back(*it);
    }
  }
}

/// If one of an insertvalue chain is random, turn all into random
///   The change of insertvalue is like the following:
///       st0 = insertvalue %INIT, %e0, 0
///       st1 = insertvalue %st0,  %e1, 1
///       st2 = insertvalue %st1,  %e2, 2
///       st3 = insertvalue %st2,  %e3, 3
///    here, {st0, st1, st2, st3} is one insertvalue chain, in which st0, st1
///    and st2 are all single use, st3 is either not a single use or used
///    in a non-insertvalue instruction.
void WIAnalysisRunner::updateInsertValues(const InsertValueInst *Inst) {
  /// find the first one in the sequence
  const InsertValueInst *pI = Inst;
  const InsertValueInst *aI = dyn_cast<const InsertValueInst>(pI->getOperand(0));
  while (aI && aI->hasOneUse()) {
    if (hasDependency(aI) && getDependency(aI) == WIAnalysis::RANDOM)
      return;
    pI = aI;
    aI = dyn_cast<const InsertValueInst>(aI->getOperand(0));
  }
  if (pI != Inst) {
    m_depMap.SetAttribute(pI, WIAnalysis::RANDOM);
    auto it = pI->user_begin();
    auto e = pI->user_end();
    for (; it != e; ++it) {
      m_pChangedNew->push_back(*it);
    }
  }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep_simple(const Instruction *I) {
  // simply check that all operands are uniform, if so return uniform, else random
  const unsigned nOps = I->getNumOperands();
  WIAnalysis::WIDependancy dep = WIAnalysis::UNIFORM_GLOBAL;
  for (unsigned i = 0; i < nOps; ++i) {
    const Value *op = I->getOperand(i);
    WIAnalysis::WIDependancy D = getDependency(op);
    dep = add_conversion[dep][D];
    if (dep == WIAnalysis::RANDOM) {
      break;
    }
  }
  return dep;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const LoadInst *inst) { return calculate_dep_simple(inst); }

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const BinaryOperator *inst) {
  // Calculate the dependency type for each of the operands
  Value *op0 = inst->getOperand(0);
  Value *op1 = inst->getOperand(1);

  WIAnalysis::WIDependancy dep0 = getDependency(op0);
  IGC_ASSERT(dep0 < WIAnalysis::NumDeps);
  WIAnalysis::WIDependancy dep1 = getDependency(op1);
  IGC_ASSERT(dep1 < WIAnalysis::NumDeps);

  // For whatever binary operation,
  // uniform returns uniform
  WIAnalysis::WIDependancy dep = select_conversion[dep0][dep1];
  if (WIAnalysis::isDepUniform(dep)) {
    return dep;
  }

  // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
  // The pattern (and (X, C)), where C preserves the lower k bits of the value,
  // is often used for truncating of numbers in 64bit. We assume that the index
  // properties are not hurt by this.
  if (inst->getOpcode() == Instruction::And) {
    ConstantInt *C0 = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt *C1 = dyn_cast<ConstantInt>(inst->getOperand(1));
    // Use any of the constants. Instcombine places constants on Op1
    // so try Op1 first.
    if (C1 || C0) {
      ConstantInt *C = C1 ? C1 : C0;
      dep = C1 ? dep0 : dep1;
      // Cannot look at bit pattern of huge integers.
      if (C->getBitWidth() < 65) {
        uint64_t val = C->getZExtValue();
        uint64_t ptr_mask = (1 << MinIndexBitwidthToPreserve) - 1;
        // Zero all bits above the lower k bits that we are interested in
        val &= (ptr_mask);
        // Make sure that all of the remaining bits are active
        if (val == ptr_mask) {
          return dep;
        }
      }
    }
  }

  // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
  // The pattern (ashr (shl X, C)C) is used for truncating of numbers in 64bit
  // The constant C must leave at least 32bits of the original number
  if (inst->getOpcode() == Instruction::AShr) {
    BinaryOperator *SHL = dyn_cast<BinaryOperator>(inst->getOperand(0));
    // We also allow add of uniform value between the ashr and shl instructions
    // since instcombine creates this pattern when adding a constant.
    // The shl forces all low bits to be zero, so there can be no carry to the
    // high bits due to the addition. Addition with uniform preservs WI-dep.
    if (SHL && SHL->getOpcode() == Instruction::Add) {
      Value *addedVal = SHL->getOperand(1);
      if (WIAnalysis::isDepUniform(getDependency(addedVal))) {
        SHL = dyn_cast<BinaryOperator>(SHL->getOperand(0));
      }
    }

    if (SHL && SHL->getOpcode() == Instruction::Shl) {
      ConstantInt *c_ashr = dyn_cast<ConstantInt>(inst->getOperand(1));
      ConstantInt *c_shl = dyn_cast<ConstantInt>(SHL->getOperand(1));
      const IntegerType *AshrTy = cast<IntegerType>(inst->getType());
      if (c_ashr && c_shl && c_ashr->getZExtValue() == c_shl->getZExtValue()) {
        // If wordWidth - shift_width >= 32 bits
        if ((AshrTy->getBitWidth() - c_shl->getZExtValue()) >= MinIndexBitwidthToPreserve) {
          // return the dep of the original X
          return getDependency(SHL->getOperand(0));
        }
      }
    }
  }

  switch (inst->getOpcode()) {
    // Addition simply adds the stride value, except for ptr_consecutive
    // which is promoted to strided.
    // Another exception is when we subtract the tid: 1 - X which turns the
    // tid order to random.
  case Instruction::Add:
  case Instruction::FAdd:
    return add_conversion[dep0][dep1];
  case Instruction::Sub:
  case Instruction::FSub:
    return sub_conversion[dep0][dep1];

  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::Shl:
    if (WIAnalysis::isDepUniform(dep0) || WIAnalysis::isDepUniform(dep1)) {
      // If one of the sides is uniform, then we can adopt
      // the other side (stride*uniform is still stride).
      // stride size is K, where K is the uniform input.
      // An exception to this is ptr_consecutive, which is
      // promoted to strided.
      return mul_conversion[dep0][dep1];
    }
  default:
    // TODO: Support more arithmetic if needed
    return WIAnalysis::RANDOM;
  }
  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const CallInst *inst) {
  // handle 3D specific intrinsics
  EOPCODE intrinsic_name = GetOpCode((Instruction *)(inst));
  GenISAIntrinsic::ID GII_id = GenISAIntrinsic::no_intrinsic;
  if (const GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(inst)) {
    GII_id = GII->getIntrinsicID();
  }

  const llvm::IntrinsicInst *llvmintrin = dyn_cast<llvm::IntrinsicInst>(inst);
  if (llvmintrin != nullptr && (llvmintrin->getIntrinsicID() == llvm::Intrinsic::stacksave ||
                                llvmintrin->getIntrinsicID() == llvm::Intrinsic::stackrestore)) {
    return WIAnalysis::UNIFORM_THREAD;
  }

  if (IsMathIntrinsic(intrinsic_name) || intrinsic_name == llvm_input || intrinsic_name == llvm_sgv ||
      intrinsic_name == llvm_shaderinputvec || intrinsic_name == llvm_getbufferptr ||
      intrinsic_name == llvm_runtimeValue || intrinsic_name == llvm_getMessagePhaseX ||
      intrinsic_name == llvm_getMessagePhaseXV || intrinsic_name == llvm_readsurfacetypeandformat ||
      intrinsic_name == llvm_simdSize || intrinsic_name == llvm_resinfoptr || intrinsic_name == llvm_sampleinfoptr ||
      intrinsic_name == llvm_ldrawvector_indexed || intrinsic_name == llvm_ldraw_indexed ||
      intrinsic_name == llvm_cycleCounter || intrinsic_name == llvm_waveShuffleIndex ||
      intrinsic_name == llvm_waveBroadcast || intrinsic_name == llvm_waveClusteredBroadcast ||
      intrinsic_name == llvm_waveBallot || intrinsic_name == llvm_waveClusteredBallot ||
      intrinsic_name == llvm_waveAll || intrinsic_name == llvm_waveClustered || intrinsic_name == llvm_waveInterleave ||
      intrinsic_name == llvm_waveClusteredInterleave || intrinsic_name == llvm_ld_ptr ||
      intrinsic_name == llvm_ldlptr ||
      (IGC_IS_FLAG_DISABLED(DisableUniformTypedAccess) && intrinsic_name == llvm_typed_read) ||
      (IGC_IS_FLAG_DISABLED(DisableUniformTypedAccess) && intrinsic_name == llvm_typed_load_status) ||
      intrinsic_name == llvm_add_pair || intrinsic_name == llvm_sub_pair || intrinsic_name == llvm_mul_pair ||
      intrinsic_name == llvm_ptr_to_pair || intrinsic_name == llvm_pair_to_ptr || intrinsic_name == llvm_fma ||
      intrinsic_name == llvm_canonicalize || GII_id == GenISAIntrinsic::GenISA_uitof_rtz ||
      GII_id == GenISAIntrinsic::GenISA_ftobf || GII_id == GenISAIntrinsic::GenISA_bftof ||
      GII_id == GenISAIntrinsic::GenISA_2fto2bf || GII_id == GenISAIntrinsic::GenISA_dual_subslice_id ||
      GII_id == GenISAIntrinsic::GenISA_hftobf8 || GII_id == GenISAIntrinsic::GenISA_bf8tohf ||
      GII_id == GenISAIntrinsic::GenISA_srnd_ftohf || GII_id == GenISAIntrinsic::GenISA_srnd_hftobf8 ||
      GII_id == GenISAIntrinsic::GenISA_srnd_hftohf8 || GII_id == GenISAIntrinsic::GenISA_srnd_bftobf8 ||
      GII_id == GenISAIntrinsic::GenISA_srnd_bftohf8 || GII_id == GenISAIntrinsic::GenISA_dnscl ||
      GII_id == GenISAIntrinsic::GenISA_ShflIdx4Lut || GII_id == GenISAIntrinsic::GenISA_lfsr ||
      GII_id == GenISAIntrinsic::GenISA_Int4VectorUnpack || GII_id == GenISAIntrinsic::GenISA_Int4VectorPack ||
      GII_id == GenISAIntrinsic::GenISA_hftohf8 || GII_id == GenISAIntrinsic::GenISA_hf8tohf ||
      GII_id == GenISAIntrinsic::GenISA_ftotf32 || GII_id == GenISAIntrinsic::GenISA_GlobalBufferPointer ||
      GII_id == GenISAIntrinsic::GenISA_LocalBufferPointer || GII_id == GenISAIntrinsic::GenISA_KSPPointer ||
      GII_id == GenISAIntrinsic::GenISA_InlinedData ||
      GII_id == GenISAIntrinsic::GenISA_GetShaderRecordPtr || GII_id == GenISAIntrinsic::GenISA_URBWrite ||
      GII_id == GenISAIntrinsic::GenISA_URBRead || GII_id == GenISAIntrinsic::GenISA_URBReadOutput ||
      GII_id == GenISAIntrinsic::GenISA_getSR0 || GII_id == GenISAIntrinsic::GenISA_getSR0_0 ||
      GII_id == GenISAIntrinsic::GenISA_mul_rtz || GII_id == GenISAIntrinsic::GenISA_fma_rtz ||
      GII_id == GenISAIntrinsic::GenISA_fma_rtp || GII_id == GenISAIntrinsic::GenISA_fma_rtn ||
      GII_id == GenISAIntrinsic::GenISA_add_rtz || GII_id == GenISAIntrinsic::GenISA_slice_id ||
      GII_id == GenISAIntrinsic::GenISA_subslice_id || GII_id == GenISAIntrinsic::GenISA_logical_subslice_id ||
      GII_id == GenISAIntrinsic::GenISA_dual_subslice_id || GII_id == GenISAIntrinsic::GenISA_eu_id ||
      GII_id == GenISAIntrinsic::GenISA_eu_thread_id || GII_id == GenISAIntrinsic::GenISA_movcr ||
      GII_id == GenISAIntrinsic::GenISA_hw_thread_id || GII_id == GenISAIntrinsic::GenISA_hw_thread_id_alloca ||
      GII_id == GenISAIntrinsic::GenISA_StackAlloca || GII_id == GenISAIntrinsic::GenISA_vectorUniform ||
      GII_id == GenISAIntrinsic::GenISA_getR0 || GII_id == GenISAIntrinsic::GenISA_getPayloadHeader ||
      GII_id == GenISAIntrinsic::GenISA_getGlobalOffset || GII_id == GenISAIntrinsic::GenISA_getWorkDim ||
      GII_id == GenISAIntrinsic::GenISA_getNumWorkGroups || GII_id == GenISAIntrinsic::GenISA_getLocalSize ||
      GII_id == GenISAIntrinsic::GenISA_getGlobalSize || GII_id == GenISAIntrinsic::GenISA_getEnqueuedLocalSize ||
      GII_id == GenISAIntrinsic::GenISA_getLocalID_X || GII_id == GenISAIntrinsic::GenISA_getLocalID_Y ||
      GII_id == GenISAIntrinsic::GenISA_getLocalID_Z || GII_id == GenISAIntrinsic::GenISA_getPrivateBase ||
      GII_id == GenISAIntrinsic::GenISA_getPrintfBuffer || GII_id == GenISAIntrinsic::GenISA_getStageInGridOrigin ||
      GII_id == GenISAIntrinsic::GenISA_getStageInGridSize || GII_id == GenISAIntrinsic::GenISA_getSyncBuffer ||
      GII_id == GenISAIntrinsic::GenISA_getRtGlobalBufferPtr || GII_id == GenISAIntrinsic::GenISA_GetGlobalBufferArg ||
      GII_id == GenISAIntrinsic::GenISA_GetImplicitBufferPtr || GII_id == GenISAIntrinsic::GenISA_GetLocalIdBufferPtr ||
      GII_id == GenISAIntrinsic::GenISA_ReadFromReservedArgSpace ||
      GII_id == GenISAIntrinsic::GenISA_getAssertBufferPtr || GII_id == GenISAIntrinsic::GenISA_getIndirectDataPtr ||
      GII_id == GenISAIntrinsic::GenISA_getScratchPtr || GII_id == GenISAIntrinsic::GenISA_getRegionGroupSize ||
      GII_id == GenISAIntrinsic::GenISA_getRegionGroupWGCount ||
      GII_id == GenISAIntrinsic::GenISA_getRegionGroupBarrierBufferPtr ||
      GII_id == GenISAIntrinsic::GenISA_staticConstantPatchValue ||
      GII_id == GenISAIntrinsic::GenISA_bitcastfromstruct || GII_id == GenISAIntrinsic::GenISA_bitcasttostruct ||
      GII_id == GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload ||
      GII_id == GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload || GII_id == GenISAIntrinsic::GenISA_PredicatedLoad ||
      GII_id == GenISAIntrinsic::GenISA_PredicatedStore || GII_id == GenISAIntrinsic::GenISA_bfn) {
    switch (GII_id) {
    default:
      break;
    case GenISAIntrinsic::GenISA_vectorUniform:
      // collect the seeds for forcing uniform vectors
      m_forcedUniforms.push_back(inst);
      return WIAnalysis::UNIFORM_THREAD;
    case GenISAIntrinsic::GenISA_getSR0:
    case GenISAIntrinsic::GenISA_getSR0_0:
    case GenISAIntrinsic::GenISA_eu_id:
    case GenISAIntrinsic::GenISA_hw_thread_id:
      return WIAnalysis::UNIFORM_THREAD;
    case GenISAIntrinsic::GenISA_slice_id:
    case GenISAIntrinsic::GenISA_subslice_id:
    case GenISAIntrinsic::GenISA_logical_subslice_id:
    case GenISAIntrinsic::GenISA_dual_subslice_id:
      // Make sure they are UNIFORM_WORKGROUP
      // return WIAnalysis::UNIFORM_WORKGROUP;
      return WIAnalysis::UNIFORM_THREAD;
    case GenISAIntrinsic::GenISA_GetImplicitBufferPtr:
    case GenISAIntrinsic::GenISA_GetLocalIdBufferPtr:
    case GenISAIntrinsic::GenISA_GetGlobalBufferArg:
    case GenISAIntrinsic::GenISA_ReadFromReservedArgSpace:
    case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload:
      return WIAnalysis::UNIFORM_THREAD;
    case GenISAIntrinsic::GenISA_getR0:
    case GenISAIntrinsic::GenISA_getPayloadHeader:
    case GenISAIntrinsic::GenISA_getGlobalOffset:
    case GenISAIntrinsic::GenISA_getWorkDim:
    case GenISAIntrinsic::GenISA_getNumWorkGroups:
    case GenISAIntrinsic::GenISA_getLocalSize:
    case GenISAIntrinsic::GenISA_getGlobalSize:
    case GenISAIntrinsic::GenISA_getEnqueuedLocalSize:
    case GenISAIntrinsic::GenISA_getLocalID_X:
    case GenISAIntrinsic::GenISA_getLocalID_Y:
    case GenISAIntrinsic::GenISA_getLocalID_Z:
    case GenISAIntrinsic::GenISA_getPrivateBase:
    case GenISAIntrinsic::GenISA_getPrintfBuffer:
    case GenISAIntrinsic::GenISA_getStageInGridOrigin:
    case GenISAIntrinsic::GenISA_getStageInGridSize:
    case GenISAIntrinsic::GenISA_getSyncBuffer:
    case GenISAIntrinsic::GenISA_getRtGlobalBufferPtr:
    case GenISAIntrinsic::GenISA_getAssertBufferPtr:
    case GenISAIntrinsic::GenISA_getIndirectDataPtr:
    case GenISAIntrinsic::GenISA_getScratchPtr:
    case GenISAIntrinsic::GenISA_getRegionGroupSize:
    case GenISAIntrinsic::GenISA_getRegionGroupWGCount:
    case GenISAIntrinsic::GenISA_getRegionGroupBarrierBufferPtr:
      return ImplicitArgs::getArgDep(GII_id);
    }

    if (intrinsic_name == llvm_input || intrinsic_name == llvm_shaderinputvec) {
      if (auto *CI = dyn_cast<ConstantInt>(inst->getOperand(1))) {
        e_interpolation mode = (e_interpolation)CI->getZExtValue();
        if (mode != EINTERPOLATION_CONSTANT
        ) {
          return WIAnalysis::RANDOM;
        }
      } else {
        return WIAnalysis::RANDOM;
      }
    }


    if (intrinsic_name == llvm_sgv) {
      IGC_ASSERT(isa<SGVIntrinsic>(inst));
      const SGVIntrinsic *systemValueIntr = cast<SGVIntrinsic>(inst);
      switch (systemValueIntr->getUsage()) {
      case VFACE:                     // palygon front/back facing from PS payload
      case RENDER_TARGET_ARRAY_INDEX: // render target array index from PS payload
      case VIEWPORT_INDEX:            // viewport index from PS payload
      {
        IGC_ASSERT(m_CGCtx->type == ShaderType::PIXEL_SHADER);
        const bool hasMultipolyDispatch = m_CGCtx->platform.hasDualKSPPS() || m_CGCtx->platform.supportDualSimd8PS();
        return hasMultipolyDispatch ? WIAnalysis::RANDOM : WIAnalysis::UNIFORM_THREAD;
      }
      case POSITION_X:              // position from VUE header in GS or pixel position X in PS
      case POSITION_Y:              // position from VUE header in GS or pixel position Y in PS
      case POSITION_Z:              // position from VUE header in GS or source depth in PS
      case POSITION_W:              // position from VUE header in GS or source W in PS
      case PRIMITIVEID:             // primitive id payload phase in GS
      case GS_INSTANCEID:           // GS instance id, calculated from URB handles
      case POINT_WIDTH:             // point width in VUE header in GS
      case INPUT_COVERAGE_MASK:     // pixel coverage mask payload phase in PS
      case SAMPLEINDEX:             // sample index from PS payload
      case CLIP_DISTANCE:           // unused
      case THREAD_ID_X:             // global invocation id X in CS or OCL Kernel
      case THREAD_ID_Y:             // global invocation id Y in CS or OCL Kernel
      case THREAD_ID_Z:             // global invocation id Z in CS or OCL Kernel
      case OUTPUT_CONTROL_POINT_ID: // unused
      case DOMAIN_POINT_ID_X:       // domain point U from DS payload
      case DOMAIN_POINT_ID_Y:       // domain point V from DS payload
      case DOMAIN_POINT_ID_Z:       // domain point W from DS payload
      case VERTEX_ID:               // vertex id in VS, delivered as an attribute
      case REQUESTED_COARSE_SIZE_X: // requested per-subspan coarse pixel size X from PS payload
      case REQUESTED_COARSE_SIZE_Y: // requested per-subspan coarse pixel size Y from PS payload
      case CLIP_DISTANCE_X:         // DX10 clip distance X from VUE header in GS
      case CLIP_DISTANCE_Y:         // DX10 clip distance Y from VUE header in GS
      case CLIP_DISTANCE_Z:         // DX10 clip distance Z from VUE header in GS
      case CLIP_DISTANCE_W:         // DX10 clip distance W from VUE header in GS
      case CLIP_DISTANCE_HI_X:
      case CLIP_DISTANCE_HI_Y:
      case CLIP_DISTANCE_HI_Z:
      case CLIP_DISTANCE_HI_W:
      case POSITION_X_OFFSET: // pixel position offset X in PS
      case POSITION_Y_OFFSET: // pixel position offset Y in PS
      case POINT_COORD_X: // point-sprite coordinate X from PS attributes
      case POINT_COORD_Y: // point-sprite coordinate Y from PS attributes
      {
        return WIAnalysis::RANDOM;
      }
      case THREAD_ID_IN_GROUP_X: // local invocation id X in CS or OCL Kernel
      {
        return m_localIDxUniform ? WIAnalysis::UNIFORM_THREAD : WIAnalysis::RANDOM;
      }
      case THREAD_ID_IN_GROUP_Y: // local invocation id Y in CS or OCL Kernel
      {
        return m_localIDyUniform ? WIAnalysis::UNIFORM_THREAD : WIAnalysis::RANDOM;
      }
      case THREAD_ID_IN_GROUP_Z: // local invocation id Z in CS or OCL Kernel
      {
        return m_localIDzUniform ? WIAnalysis::UNIFORM_THREAD : WIAnalysis::RANDOM;
      }
      case MSAA_RATE: // multisample rate from PS payload
      case DISPATCH_DIMENSION_X:  // dispatch size X from MS payload
      case DISPATCH_DIMENSION_Y:  // dispatch size Y from MS payload
      case DISPATCH_DIMENSION_Z:  // dispatch size Z from MS payload
      case INDIRECT_DATA_ADDRESS: // indirect data address from MS payload
      case SHADER_TYPE: {
        return WIAnalysis::UNIFORM_GLOBAL;
      }
      case THREAD_GROUP_ID_X: // workgroup id X in CS or OCL Kernel
      case THREAD_GROUP_ID_Y: // workgroup id Y in CS or OCL Kernel
      case THREAD_GROUP_ID_Z: // workgroup id Z in CS or OCL Kernel
      {
        return WIAnalysis::UNIFORM_WORKGROUP;
      }
      case ACTUAL_COARSE_SIZE_X:          // actual coarse pixel size X from PS payload
      case ACTUAL_COARSE_SIZE_Y:          // actual coarse pixel size Y from PS payload
      case THREAD_ID_WITHIN_THREAD_GROUP: // (physical) thread id in thread group from CS payload
      {
        return WIAnalysis::UNIFORM_THREAD;
      }
      case XP0: // base vertex from VS attributes
      case XP1: // base instance from VS attributes
      case XP2: // draw index from VS attributes
      {
        if (m_CGCtx->type == ShaderType::TASK_SHADER || m_CGCtx->type == ShaderType::MESH_SHADER) {
          // XP0 is used for draw index in mesh and task
          return WIAnalysis::UNIFORM_GLOBAL;
        }
        // Extended parameters are delivered in VS as attributes. Values
        // are uniform but delivered per-vertex, frontends can use
        // subgroup operations to get the uniform value.
        return WIAnalysis::RANDOM;
      }
      case NUM_SGV:
        IGC_ASSERT_MESSAGE(0, "Unexpected value");
        break;
        // This switch intentionally has no `default:` case. Whenever a new
        // SGV type is added this code must be updated.
      }
    }
    if (intrinsic_name == llvm_getMessagePhaseX || intrinsic_name == llvm_getMessagePhaseXV) {
      return WIAnalysis::UNIFORM_THREAD;
    }

    if (intrinsic_name == llvm_waveShuffleIndex || intrinsic_name == llvm_waveBroadcast) {
      Value *op0 = inst->getArgOperand(0);
      Value *op1 = inst->getArgOperand(1);
      WIAnalysis::WIDependancy dep0 = getDependency(op0);
      IGC_ASSERT(dep0 < WIAnalysis::NumDeps);
      WIAnalysis::WIDependancy dep1 = getDependency(op1);
      IGC_ASSERT(dep1 < WIAnalysis::NumDeps);
      bool isUniform0 = WIAnalysis::isDepUniform(dep0);
      bool isUniform1 = WIAnalysis::isDepUniform(dep1);
      if ((isUniform0 && isUniform1) || (!isUniform0 && !isUniform1)) {
        // Select worse one
        return select_conversion[dep0][dep1];
      } else {
        // Select uniform one if only one is uniform
        return isUniform0 ? dep0 : dep1;
      }
    }

    if (GII_id == GenISAIntrinsic::GenISA_StackAlloca) {
      WIAnalysis::WIDependancy dep0 = WIAnalysis::UNIFORM_THREAD;
      Value *op0 = inst->getArgOperand(0);
      WIAnalysis::WIDependancy dep1 = getDependency(op0);
      // Select worse one
      return select_conversion[dep0][dep1];
    }

    if (GII_id == GenISAIntrinsic::GenISA_staticConstantPatchValue) {
      return WIAnalysis::UNIFORM_GLOBAL;
    }

    if (intrinsic_name == llvm_waveBallot) {
      return WIAnalysis::UNIFORM_THREAD;
    }

    if (intrinsic_name == llvm_waveAll) {
      auto *waveAllInst = cast<WaveAllIntrinsic>(inst);
      auto *pred = waveAllInst->getPredicate();
      WIAnalysis::WIDependancy depPred = getDependency(pred);
      if (WIAnalysis::isDepUniform(depPred)) {
        return WIAnalysis::UNIFORM_THREAD;
      } else {
        return WIAnalysis::RANDOM;
      }
    }

    if (intrinsic_name == llvm_waveClustered) {
      const unsigned clusterSize =
          static_cast<unsigned>(cast<llvm::ConstantInt>(inst->getArgOperand(2))->getZExtValue());

      constexpr unsigned maxSimdSize = 32;
      if (clusterSize == maxSimdSize) {
        // TODO: do the same for SIMD8 and SIMD16 if possible.
        return WIAnalysis::UNIFORM_THREAD;
      } else {
        return WIAnalysis::RANDOM;
      }
    }

    if (intrinsic_name == llvm_waveInterleave || intrinsic_name == llvm_waveClusteredInterleave) {
      return WIAnalysis::RANDOM;
    }

    if (intrinsic_name == llvm_URBRead || intrinsic_name == llvm_URBReadOutput) {
      if (!m_CGCtx->platform.isProductChildOf(IGFX_DG2)) {
        return WIAnalysis::RANDOM;
      }
      if (m_CGCtx->type != ShaderType::TASK_SHADER && m_CGCtx->type != ShaderType::MESH_SHADER) {
        return WIAnalysis::RANDOM;
      }
    }

    if (intrinsic_name == llvm_URBWrite) {
      // TODO: enable this for other platforms/shader types if needed
      if (!m_CGCtx->platform.isProductChildOf(IGFX_DG2)) {
        return WIAnalysis::RANDOM;
      }
      if (m_CGCtx->type != ShaderType::TASK_SHADER && m_CGCtx->type != ShaderType::MESH_SHADER) {
        return WIAnalysis::RANDOM;
      }
    }

    // Iterate over all input dependencies. If all are uniform - propagate it.
    // otherwise - return RANDOM
    unsigned numParams = IGCLLVM::getNumArgOperands(inst);
    WIAnalysis::WIDependancy dep = WIAnalysis::UNIFORM_GLOBAL;
    for (unsigned i = 0; i < numParams; ++i) {
      Value *op = inst->getArgOperand(i);
      WIAnalysis::WIDependancy tdep = getDependency(op);
      dep = select_conversion[dep][tdep];
      if (dep == WIAnalysis::RANDOM) {
        break; // Uniformity check failed. no need to continue
      }
    }
    return dep;
  }
  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const GetElementPtrInst *inst) {
  const Value *opPtr = inst->getOperand(0);
  WIAnalysis::WIDependancy dep = getDependency(opPtr);
  // running over the all indices arguments except for the last
  // here we assume the pointer is the first operand
  unsigned num = inst->getNumIndices();
  for (unsigned i = 1; i < num; ++i) {
    const Value *op = inst->getOperand(i);
    WIAnalysis::WIDependancy tdep = getDependency(op);
    dep = select_conversion[dep][tdep];
    if (!WIAnalysis::isDepUniform(dep)) {
      return WIAnalysis::RANDOM;
    }
  }
  const Value *lastInd = inst->getOperand(num);
  WIAnalysis::WIDependancy lastIndDep = getDependency(lastInd);
  return gep_conversion[dep][lastIndDep];
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const PHINode *inst) {
  unsigned num = inst->getNumIncomingValues();
  bool foundFirst = 0;
  WIAnalysis::WIDependancy totalDep = WIAnalysis::WIDependancy::INVALID;

  for (unsigned i = 0; i < num; ++i) {
    Value *op = inst->getIncomingValue(i);
    if (hasDependency(op)) {
      if (!foundFirst) {
        totalDep = getDependency(op);
      } else {
        totalDep = select_conversion[totalDep][getDependency(op)];
      }
      foundFirst = 1;
    }
  }

  IGC_ASSERT_MESSAGE(foundFirst, "We should not reach here with All incoming values are unset");

  return totalDep;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep_terminator(const IGCLLVM::TerminatorInst *inst) {
  // Instruction has no return value
  // Just need to know if this inst is uniform or not
  // because we may want to avoid predication if the control flows
  // in the function are uniform...
  switch (inst->getOpcode()) {
  case Instruction::Br: {
    const BranchInst *brInst = cast<BranchInst>(inst);
    if (brInst->isConditional()) {
      // Conditional branch is uniform, if its condition is uniform
      Value *op = brInst->getCondition();
      WIAnalysis::WIDependancy dep = getDependency(op);
      if (WIAnalysis::isDepUniform(dep)) {
        return dep;
      }
      return WIAnalysis::RANDOM;
    }
    // Unconditional branch is non TID-dependent
    return WIAnalysis::UNIFORM_GLOBAL;
  }
  // Return instructions are unconditional
  case Instruction::Ret:
    return WIAnalysis::UNIFORM_GLOBAL;
  case Instruction::Unreachable:
    return WIAnalysis::UNIFORM_GLOBAL;
  case Instruction::IndirectBr:
    return WIAnalysis::RANDOM;
    // TODO: Define the dependency requirements of indirectBr
  case Instruction::Switch: {
    // Same as conditional br
    const SwitchInst *swInst = cast<SwitchInst>(inst);
    Value *op = swInst->getCondition();
    WIAnalysis::WIDependancy dep = getDependency(op);
    if (WIAnalysis::isDepUniform(dep)) {
      return dep;
    }
    return WIAnalysis::RANDOM;
  }
  default:
    return WIAnalysis::RANDOM;
  }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const SelectInst *inst) {
  Value *op0 = inst->getOperand(0); // mask
  WIAnalysis::WIDependancy dep0 = getDependency(op0);
  if (WIAnalysis::isDepUniform(dep0)) {
    Value *op1 = inst->getOperand(1);
    Value *op2 = inst->getOperand(2);
    WIAnalysis::WIDependancy dep1 = getDependency(op1);
    WIAnalysis::WIDependancy dep2 = getDependency(op2);
    // In case of constant scalar select we can choose according to the mask.
    if (ConstantInt *C = dyn_cast<ConstantInt>(op0)) {
      uint64_t val = C->getZExtValue();
      if (val)
        return dep1;
      else
        return dep2;
    }
    // Select the "weaker" dep, but if only one dep is ptr_consecutive,
    // it must be promoted to strided ( as this data may
    // propagate to Load/Store instructions.
    WIAnalysis::WIDependancy tDep = select_conversion[dep1][dep2];
    return select_conversion[dep0][tDep];
  }
  // In case the mask is non-uniform the select outcome can be a combination
  // so we don't know nothing about it.
  return WIAnalysis::RANDOM;
}

bool WIAnalysisRunner::TrackAllocaDep(const Value *I, AllocaDep &dep) {
  bool trackable = true;
  for (Value::const_user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it) {
    if (const GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*use_it)) {
      trackable &= TrackAllocaDep(gep, dep);
    } else if (const llvm::LoadInst *pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it)) {
      trackable &= (pLoad->isSimple());
    } else if (const llvm::StoreInst *pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it)) {
      trackable &= (pStore->isSimple());
      // Not supported case: GEP instruction is the stored value of the StoreInst
      trackable &= (pStore->getValueOperand() != I);
      dep.stores.push_back(pStore);
    } else if (const llvm::BitCastInst *pBitCast = llvm::dyn_cast<llvm::BitCastInst>(*use_it)) {
      trackable &= TrackAllocaDep(pBitCast, dep);
    } else if (const llvm::AddrSpaceCastInst *pAddrCast = llvm::dyn_cast<llvm::AddrSpaceCastInst>(*use_it)) {
      trackable &= TrackAllocaDep(pAddrCast, dep);
    } else if (const GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(*use_it)) {
      GenISAIntrinsic::ID IID = intr->getIntrinsicID();
      if (IID == GenISAIntrinsic::GenISA_assume_uniform) {
        dep.assume_uniform = true;
      } else
        trackable = false;
    } else if (const IntrinsicInst *intr = dyn_cast<IntrinsicInst>(*use_it)) {
      llvm::Intrinsic::ID IID = intr->getIntrinsicID();
      if (IID != llvm::Intrinsic::lifetime_start && IID != llvm::Intrinsic::lifetime_end) {
        trackable = false;
      } else if (IID == llvm::Intrinsic::lifetime_start)
        dep.lifetimes.push_back(intr);
    } else {
      // This is some other instruction. Right now we don't want to handle these
      trackable = false;
    }
  }
  return trackable;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const AllocaInst *inst) {
  if (m_CGCtx->platform.getWATable().WaNoA32ByteScatteredStatelessMessages) {
    // avoid generating A32 byte scatter on platforms not supporting it
    return WIAnalysis::RANDOM;
  }
  if (!hasDependency(inst)) {
    AllocaDep dep;
    dep.assume_uniform = false;
    bool trackable = TrackAllocaDep(inst, dep);

    if (trackable || dep.assume_uniform) {
      m_allocaDepMap.insert(std::make_pair(inst, dep));
      for (auto it : dep.stores) {
        m_storeDepMap.insert(std::make_pair(&(*it), inst));
      }
    }
  }
  auto depIt = m_allocaDepMap.find(inst);
  if (depIt == m_allocaDepMap.end()) {
    // If we haven't been able to track the dependency of the alloca make it random
    return WIAnalysis::RANDOM;
  }
  // find assume-uniform
  if (depIt->second.assume_uniform) {
    return WIAnalysis::UNIFORM_THREAD;
  }
  // find the common dominator block among all the life-time starts
  // that can be considered as the nearest logical location for alloca.
  const BasicBlock *CommonDomBB = nullptr;
  for (auto *SI : depIt->second.lifetimes) {
    auto BB = SI->getParent();
    IGC_ASSERT(BB);
    if (!CommonDomBB)
      CommonDomBB = BB;
    else
      CommonDomBB = DT->findNearestCommonDominator(CommonDomBB, BB);
  }
  if (!CommonDomBB) {
    CommonDomBB = inst->getParent();
  }
  // if any store is not uniform, then alloca is not uniform
  // if any store is affected by a divergent branch after alloca,
  // then alloca is also not uniform
  for (auto *SI : depIt->second.stores) {
    if (hasDependency(SI)) {
      if (!WIAnalysis::isDepUniform(getDependency(SI))) {
        return WIAnalysis::RANDOM;
      }

      if (auto I = m_ctrlBranches.find(SI->getParent()); I != m_ctrlBranches.end()) {
        auto &Branches = I->second;
        WIAnalysis::WIDependancy cntrDep = WIAnalysis::UNIFORM_GLOBAL;
        for (auto *BrI : Branches) {
          // exclude those branches that dominates alloca
          if (!DT->dominates(BrI, CommonDomBB)) {
            // select a weaker one
            IGC_ASSERT(hasDependency(BrI));
            cntrDep = select_conversion[cntrDep][getDependency(BrI)];
            if (cntrDep == WIAnalysis::RANDOM)
              break;
          }
        }
        if (cntrDep == WIAnalysis::RANDOM)
          return WIAnalysis::RANDOM;
      }
    }
  }

  return WIAnalysis::UNIFORM_THREAD;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const CastInst *inst) {
  Value *op0 = inst->getOperand(0);
  WIAnalysis::WIDependancy dep0 = getDependency(op0);

  // independent remains independent
  if (WIAnalysis::isDepUniform(dep0))
    return dep0;

  switch (inst->getOpcode()) {
  case Instruction::SExt:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::AddrSpaceCast:
  case Instruction::UIToFP:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::SIToFP:
    return dep0;
  case Instruction::BitCast:
  case Instruction::ZExt:
    return WIAnalysis::RANDOM;
    // FIXME:: assumes that the value does not cross the +/- border - risky !!!!
  case Instruction::Trunc: {
    const Type *destType = inst->getDestTy();
    const IntegerType *intType = dyn_cast<IntegerType>(destType);
    if (intType && (intType->getBitWidth() >= MinIndexBitwidthToPreserve)) {
      return dep0;
    }
    return WIAnalysis::RANDOM;
  }
  default:
    IGC_ASSERT_MESSAGE(0, "no such opcode");
    // never get here
    return WIAnalysis::RANDOM;
  }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const VAArgInst *inst) {
  IGC_ASSERT_MESSAGE(0, "Are we supporting this ??");
  return WIAnalysis::RANDOM;
}

void WIAnalysisRunner::CS_checkLocalIDs(Function *F) {
  CS_WALK_ORDER walkOrder = CS_WALK_ORDER::WO_XYZ;
  ThreadIDLayout idLayout = ThreadIDLayout::X;

  {
    if (IGC_IS_FLAG_DISABLED(OverrideCsWalkOrderEnable) || IGC_IS_FLAG_DISABLED(OverrideCsTileLayoutEnable))
      return;

    walkOrder = (CS_WALK_ORDER)IGC_GET_FLAG_VALUE(OverrideCsWalkOrder);
    idLayout = (ThreadIDLayout)IGC_GET_FLAG_VALUE(OverrideCsTileLayout);
  }

  if (idLayout == ThreadIDLayout::TileY || idLayout == ThreadIDLayout::QuadTile) {
    // Need clarification on semantics. Skip for now.
    return;
  }

  Module *M = F->getParent();
  uint32_t X = 0, Y = 0, Z = 0;
  if (GlobalVariable *pX = M->getGlobalVariable("ThreadGroupSize_X")) {
    X = int_cast<unsigned>(cast<ConstantInt>(pX->getInitializer())->getZExtValue());
  }
  if (GlobalVariable *pY = M->getGlobalVariable("ThreadGroupSize_Y")) {
    Y = int_cast<unsigned>(cast<ConstantInt>(pY->getInitializer())->getZExtValue());
  }
  if (GlobalVariable *pZ = M->getGlobalVariable("ThreadGroupSize_Z")) {
    Z = int_cast<unsigned>(cast<ConstantInt>(pZ->getInitializer())->getZExtValue());
  }

  // sanity
  if (X == 0 || Y == 0 || Z == 0) {
    return;
  }

  if (X == 1) {
    m_localIDxUniform = true;
  }
  if (Y == 1) {
    m_localIDyUniform = true;
  }
  if (Z == 1) {
    m_localIDzUniform = true;
  }

  constexpr uint32_t simdSize = 32;
  if (idLayout == ThreadIDLayout::X) {
    auto setUniform = [this](int dim) {
      switch (dim) {
      case 0:
        m_localIDxUniform = true;
        break;
      case 1:
        m_localIDyUniform = true;
        break;
      case 2:
        m_localIDzUniform = true;
        break;
      default:
        IGC_ASSERT_UNREACHABLE();
      }
    };

    const uint32_t dims[3] = {X, Y, Z};
    struct {
      char first;
      char second;
      char third;
    } orders;

    switch (walkOrder) {
    case CS_WALK_ORDER::WO_XYZ:
      orders = {0, 1, 2};
      break;
    case CS_WALK_ORDER::WO_XZY:
      orders = {0, 2, 1};
      break;
    case CS_WALK_ORDER::WO_YXZ:
      orders = {1, 0, 2};
      break;
    case CS_WALK_ORDER::WO_ZXY:
      orders = {2, 0, 1};
      break;
    case CS_WALK_ORDER::WO_YZX:
      orders = {1, 2, 0};
      break;
    case CS_WALK_ORDER::WO_ZYX:
      orders = {2, 1, 0};
      break;
    default:
      return;
    }

    if ((dims[orders.first] % simdSize) == 0) {
      // first dim is multiple of simd size
      setUniform(orders.second);
      setUniform(orders.third);
    } else if (((dims[orders.first] * dims[orders.second]) % simdSize) == 0) {
      // combination of first and second dims are multiple of simd size
      setUniform(orders.third);
    }
    return;
  }
}

// Set IsLxUniform/IsLyUniform/IsLxUniform to true if they are uniform;
// do nothing otherwise.
void WIAnalysisRunner::checkLocalIdUniform(Function *F, bool &IsLxUniform, bool &IsLyUniform, bool &IsLzUniform) {
  if (m_CGCtx->type == ShaderType::COMPUTE_SHADER) {
    CS_checkLocalIDs(F);
    return;
  }
  if (m_CGCtx->type != ShaderType::OPENCL_SHADER) {
    return;
  }

  FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(F);
  ModuleMetaData *modMD = m_CGCtx->getModuleMetaData();
  auto funcMD = modMD->FuncMD.find(F);

  int32_t WO_0 = -1, WO_1 = -1, WO_2 = -1;
  if (funcMD == modMD->FuncMD.end()) {
    return;
  }

  WorkGroupWalkOrderMD workGroupWalkOrder = funcMD->second.workGroupWalkOrder;
  if (!workGroupWalkOrder.dim0 && !workGroupWalkOrder.dim1 && !workGroupWalkOrder.dim2) {
    return;
  }

  WO_0 = workGroupWalkOrder.dim0;
  WO_1 = workGroupWalkOrder.dim1;
  WO_2 = workGroupWalkOrder.dim2;

  // We expect that the work group walk order is always fixed.
  IGC_ASSERT(WO_0 == 0 && WO_1 == 1 && WO_2 == 2);

  uint32_t simdSize = 0;
  SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
  if (subGroupSize->hasValue()) {
    simdSize = (uint32_t)subGroupSize->getSIMDSize();
  }
  simdSize = simdSize >= 8 ? simdSize : 32;

  int32_t X = -1, Y = -1, Z = -1;
  ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
  if (threadGroupSize->hasValue()) {
    X = (int32_t)threadGroupSize->getXDim();
    Y = (int32_t)threadGroupSize->getYDim();
    Z = (int32_t)threadGroupSize->getZDim();
  }

  if (WO_0 == 0 && ((X / simdSize) * simdSize) == X) {
    // each thread will have Y and Z unchanged.
    IsLyUniform = true;
    IsLzUniform = true;
  } else if (WO_0 == 1 && ((Y / simdSize) * simdSize) == Y) {
    // each thread will have X and Z unchanged.
    IsLxUniform = true;
    IsLzUniform = true;
  } else if (WO_0 == 2 && ((Z / simdSize) * simdSize) == Z) {
    // each thread will have X and Y unchanged.
    IsLxUniform = true;
    IsLyUniform = true;
  }

  if (X == 1) {
    IsLxUniform = true;
  }
  if (Y == 1) {
    IsLyUniform = true;
  }
  if (Z == 1) {
    IsLzUniform = true;
  }

  // linear order dispatch
  uint32_t XxY = X * Y;
  if (X > 0 && (X % simdSize) == 0) {
    // X is multiple of simdSize
    IsLyUniform = true;
    IsLzUniform = true;
  } else if (X > 0 && Y > 0 && (XxY % simdSize) == 0) {
    // X*Y is multiple of simdSize
    IsLzUniform = true;
  }
}

BranchInfo::BranchInfo(const IGCLLVM::TerminatorInst *inst, const BasicBlock *ipd) : cbr(inst), full_join(ipd) {
  auto *fork_blk = const_cast<BasicBlock *>(inst->getParent());
  IGC_ASSERT_MESSAGE(cbr == fork_blk->getTerminator(), "block terminator mismatch");

  llvm::SmallVector<BasicBlock *, 4> WorkSet;
  if (cbr->getNumSuccessors() != 2) {
    llvm::DenseSet<BasicBlock *> Reached;
    llvm::DenseSet<BasicBlock *> Visited;
    for (auto *Succ : successors(fork_blk)) {
      if (Succ == full_join)
        continue;
      Visited.clear();
      WorkSet.push_back(Succ);
      while (!WorkSet.empty()) {
        BasicBlock *BB = WorkSet.pop_back_val();
        Visited.insert(BB);
        influence_region.insert(const_cast<BasicBlock *>(BB));
        if (Reached.count(BB))
          partial_joins.insert(const_cast<BasicBlock *>(BB));
        for (auto *SBB : successors(BB)) {
          if (SBB != full_join && !Visited.count(SBB))
            WorkSet.push_back(SBB);
        }
      }
      // Merge Visited into Reached.
      Reached.insert(Visited.begin(), Visited.end());
    }
    return;
  }
  llvm::DenseSet<BasicBlock *> f_set, t_set;
  if (cbr->getSuccessor(0) != full_join) {
    WorkSet.push_back(cbr->getSuccessor(0));
    while (!WorkSet.empty()) {
      BasicBlock *cur_blk = WorkSet.pop_back_val();
      f_set.insert(cur_blk);
      influence_region.insert(cur_blk);
      for (auto *succ_blk : successors(cur_blk)) {
        if (succ_blk != full_join && !f_set.count(succ_blk)) {
          WorkSet.push_back(succ_blk);
        }
      }
    }
  }
  if (cbr->getSuccessor(1) != full_join) {
    WorkSet.push_back(cbr->getSuccessor(1));
    while (!WorkSet.empty()) {
      BasicBlock *cur_blk = WorkSet.pop_back_val();
      t_set.insert(cur_blk);
      influence_region.insert(cur_blk);
      if (f_set.count(cur_blk)) {
        partial_joins.insert(cur_blk);
      }
      for (auto *succ_blk : successors(cur_blk)) {
        if (succ_blk != full_join && !t_set.count(succ_blk)) {
          WorkSet.push_back(succ_blk);
        }
      }
    }
  }
}

void BranchInfo::print(raw_ostream &OS) const {
  OS << "\nCBR: " << *cbr;
  OS << "\nIPD: ";
  if (full_join) {
    full_join->print(IGC::Debug::ods());
  }
  OS << "\nPartial Joins:";
  SmallPtrSet<BasicBlock *, 4>::iterator join_it = partial_joins.begin();
  SmallPtrSet<BasicBlock *, 4>::iterator join_e = partial_joins.end();
  for (; join_it != join_e; ++join_it) {
    BasicBlock *cur_blk = *join_it;
    OS << "\n    ";
    cur_blk->print(IGC::Debug::ods());
  }
  OS << "\nInfluence Region:";
  DenseSet<BasicBlock *>::const_iterator blk_it = influence_region.begin();
  DenseSet<BasicBlock *>::const_iterator blk_e = influence_region.end();
  for (; blk_it != blk_e; ++blk_it) {
    BasicBlock *cur_blk = *blk_it;
    OS << "\n    ";
    cur_blk->print(IGC::Debug::ods());
  }
  OS << "\n";
}

extern "C" {
void *createWIAnalysisPass() { return new WIAnalysis(); }
}
