/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPacketize
/// -------------
///
///   - Vectorize the SIMT functions
///
///   - Vectorize the generic function called by the SIMT functions
///
///   - Replace generic control-flow with SIMD control-flow
///
//===----------------------------------------------------------------------===//

#include "PacketBuilder.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"

#include "vc/Utils/GenX/Region.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXSimdCFLowering.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"

#include <algorithm>
#include <set>
#include <stack>
#include <unordered_set>
#include "Probe/Assertion.h"

using namespace pktz;

namespace llvm {

/// Packetizing SIMT functions
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// a) Look for functions with attributes CMGenXSIMT
///    If no such function, end the pass
///
/// b) sort functions in call-graph topological order
///    find those generic functions called by the SIMT functions
///    find all the possible widthes those functions should be vectorized to
///
/// c) find those uniform function arguments
///    arguments for non-SIMT functions are uniform
///    arguments for SIMT-entry are uniform
///    arguments for SIMT-functions are uniform if it is only defined by
///       callers' uniform argument.
///
/// d) Run reg2mem pass to remove phi-nodes
///    This is because we need to generate simd-control-flow
///    after packetization. simd-control-flow lowering cannot handle phi-node.
///
/// e) for uniform arguments
///    Mark the allocas for those arguments as uniform
///    Mark the load/store for those allocas as uniform
///
/// f) vectorize generic functions to its SIMT width, callee first
///    - create the vector prototype
///    - clone the function-body into the vector prototype
///    - vectorize the function-body
///    - note: original function is kept because it may be used outside SIMT
///
/// g) vectorize SIMT-entry functions
///    - no change of function arguments
///    - no cloning, direct-vectorization on the function-body
///
/// h) SIMD-control-flow lowering
///
/// i) run mem2reg pass to create SSA
///
/// j) CMABI pass to remove global Execution-Mask
///
class GenXPacketize : public ModulePass {
public:
  static char ID;
  explicit GenXPacketize() : ModulePass(ID) {}
  ~GenXPacketize() { releaseMemory(); }
  StringRef getPassName() const override { return "GenX Packetize"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredID(BreakCriticalEdgesID);
  };
  bool runOnModule(Module &M) override;
  void releaseMemory() override {
    ReplaceMap.clear();
    UniformArgs.clear();
    UniformInsts.clear();
    FuncOrder.clear();
    FuncVectors.clear();
    FuncMap.clear();
  }

private:
  void findFunctionVectorizationOrder(Module *M);

  Value *getPacketizeValue(Value *OrigValue);
  Value *getUniformValue(Value *OrigValue);
  Function *getVectorIntrinsic(Module *M, unsigned id, ArrayRef<Type *> ArgTy);
  Value *packetizeConstant(Constant *pConstant);
  Value *packetizeGenXIntrinsic(Instruction *pInst);
  Value *packetizeLLVMIntrinsic(Instruction *pInst);
  Value *packetizeLLVMInstruction(Instruction *pInst);
  Value *packetizeInstruction(Instruction *pInst);

  void replaceAllUsesNoTypeCheck(Value *pInst, Value *pNewInst);
  void removeDeadInstructions(Function &F);
  void fixupLLVMIntrinsics(Function &F);

  Function *vectorizeSIMTFunction(Function *F, unsigned Width);
  bool vectorizeSIMTEntry(Function &F);

  bool isUniformIntrinsic(unsigned id);
  void findUniformArgs(Function &F);
  void findUniformInsts(Function &F);

  void lowerControlFlowAfter(std::vector<Function *> &SIMTFuncs);
  GlobalVariable *findGlobalExecMask();

private:
  Module *M = nullptr;
  PacketBuilder *B = nullptr;

  // track already packetized values
  ValueToValueMapTy ReplaceMap;

  /// uniform set for arguments
  std::set<const Argument *> UniformArgs;
  /// uniform set for alloca, load, store, and GEP
  std::set<const Instruction *> UniformInsts;
  /// sort function in caller-first order
  std::vector<Function *> FuncOrder;
  /// map: function ==> a set of vectorization width
  std::map<Function *, std::set<unsigned>> FuncVectors;
  /// Map: original function and vectorization width ==> vectorized version
  std::map<std::pair<Function *, unsigned>, Function *> FuncMap;

  const DataLayout *DL = nullptr;
};

// Adapted from llvm::UnifyFunctionExitNodes.cpp
// Loop over all of the blocks in a function, tracking all of the blocks
// that return.
static bool GenXUnifyReturnBlocks(Function& F) {
  std::vector<BasicBlock*> ReturningBlocks;

  for (BasicBlock& I : F)
    if (isa<ReturnInst>(I.getTerminator()))
      ReturningBlocks.push_back(&I);

  if (ReturningBlocks.size() <= 1)
    return false;

  // Insert a new basic block into the function, add PHI nodes (if the function
  // returns values), and convert all of the return instructions into
  // unconditional branches.
  BasicBlock* NewRetBlock = BasicBlock::Create(F.getContext(),
                                               "UnifiedReturnBlock", &F);

  PHINode* PN = nullptr;
  if (F.getReturnType()->isVoidTy()) {
    ReturnInst::Create(F.getContext(), nullptr, NewRetBlock);
  }
  else {
    // If the function doesn't return void... add a PHI node to the block...
    PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(),
                         "UnifiedRetVal");
    NewRetBlock->getInstList().push_back(PN);
        ReturnInst::Create(F.getContext(), PN, NewRetBlock);
  }

  // Loop over all of the blocks, replacing the return instruction with an
  // unconditional branch.
  for (BasicBlock* BB : ReturningBlocks) {
    // Add an incoming element to the PHI node for every return instruction that
    // is merging into this new block...
    if (PN)
      PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

    BB->getInstList().pop_back();  // Remove the return insn
    BranchInst::Create(NewRetBlock, BB);
  }

  return true;
}

bool GenXPacketize::runOnModule(Module &Module) {
  M = &Module;
  // find all the SIMT enntry-functions
  std::vector<Function *> ForkFuncs;
  for (auto &F : M->getFunctionList()) {
    if (F.hasFnAttribute("CMGenxSIMT")) {
      uint32_t Width = 0;
      F.getFnAttribute("CMGenxSIMT").getValueAsString().getAsInteger(0, Width);
      if (Width > 1) {
        IGC_ASSERT(Width == 8 || Width == 16 || Width == 32);
        ForkFuncs.push_back(&F);
      }
    }
  }
  if (ForkFuncs.empty())
    return false;

  DL = &(M->getDataLayout());
  B = new PacketBuilder(M);

  // sort functions in order, also find those functions that are used in
  // the SIMT mode, therefore need whole-function vectorization.
  findFunctionVectorizationOrder(M);

  unsigned NumFunc = FuncOrder.size();
  // find uniform arguments
  UniformArgs.clear();
  for (unsigned i = 0; i < NumFunc; ++i) {
    auto F = FuncOrder[i];
    findUniformArgs(*F);
  }

  UniformInsts.clear();

  std::vector<Function *> SIMTFuncs;
  // Process those functions called in the SIMT mode
  for (int i = NumFunc - 1; i >= 0; --i) {
    auto F = FuncOrder[i];
    auto iter = FuncVectors.find(F);
    if (iter != FuncVectors.end()) {
      auto WV = iter->second;
      for (auto W : WV) {
        auto VF = vectorizeSIMTFunction(F, W);
        auto Key = std::pair<Function *, unsigned>(F, W);
        FuncMap.insert(
            std::pair<std::pair<Function *, unsigned>, Function *>(Key, VF));
        SIMTFuncs.push_back(VF);
      }
    }
  }

  // vectorize SIMT entry-functions
  bool Modified = false;
  for (auto F : ForkFuncs) {
    Modified |= vectorizeSIMTEntry(*F);
    SIMTFuncs.push_back(&(*F));
  }

  delete B;

  // perform reg-to-mem in order to generate simd-control-flow without phi
  // we then perform mem-to-reg after generating simd-control-flow.
  std::unique_ptr<FunctionPass> DemotePass(createDemoteRegisterToMemoryPass());
  for (auto F : SIMTFuncs) {
    GenXUnifyReturnBlocks(*F);
    DemotePass->runOnFunction(*F);
  }
  // lower the SIMD control-flow
  lowerControlFlowAfter(SIMTFuncs);

  return Modified;
}

/***************************************************************************
 * vectorize a functions that is used in the fork-region
 */
Function *GenXPacketize::vectorizeSIMTFunction(Function *F, unsigned Width) {
  IGC_ASSERT(!F->hasFnAttribute("CMGenxSIMT"));
  B->SetTargetWidth(Width);

  // vectorize the argument and return types
  std::vector<Type *> ArgTypes;
  for (const Argument &I : F->args()) {
    if (UniformArgs.count(&I))
      ArgTypes.push_back(I.getType());
    else if (I.getType()->isPointerTy()) {
      // FIXME: check the pointer defined by an argument or an alloca
      // [N x float]* should packetize to [N x <8 x float>]*
      auto VTy = PointerType::get(
          B->GetVectorType(IGCLLVM::getNonOpaquePtrEltTy(I.getType())),
          I.getType()->getPointerAddressSpace());
      ArgTypes.push_back(VTy);
    }
    else {
      ArgTypes.push_back(B->GetVectorType(I.getType()));
    }
  }
  Type *RetTy = B->GetVectorType(F->getReturnType());
  // Create a new function type...
  IGC_ASSERT(!F->isVarArg());
  FunctionType *FTy = FunctionType::get(RetTy, ArgTypes, false);

  // Create the vector function prototype
  StringRef VecFName = F->getName();
  const char *Suffix[] = {".vec00", ".vec08", ".vec16", ".vec24", ".vec32"};
  Function *ClonedFunc =
      Function::Create(FTy, GlobalValue::InternalLinkage,
                       VecFName + Suffix[Width / 8], F->getParent());
  ClonedFunc->setCallingConv(F->getCallingConv());
  ClonedFunc->setAttributes(F->getAttributes());
  if (F->getAlignment() > 0)
    ClonedFunc->setAlignment(IGCLLVM::getAlign(*F));

  // then use CloneFunctionInto
  ValueToValueMapTy ArgMap;
  Function::arg_iterator ArgI = ClonedFunc->arg_begin();
  for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
       I != E; ++I) {
    ArgI->setName(I->getName()); // Copy the name over...
    ArgMap[I] = ArgI;            // Add mapping to ValueMap
    if (UniformArgs.count(I)) {  // bookkeep the uniform set
      UniformArgs.insert(ArgI);
    }
    ArgI++;
  }
  SmallVector<ReturnInst *, 10> returns;
  ClonedCodeInfo CloneInfo;
  IGCLLVM::CloneFunctionInto(ClonedFunc, F, ArgMap,
      IGCLLVM::CloneFunctionChangeType::GlobalChanges,
      returns, Suffix[Width / 8], &CloneInfo);

  ReplaceMap.clear();
  // find uniform instructions related to uniform arguments
  findUniformInsts(*ClonedFunc);

  // vectorize instructions in the fork-regions
  std::vector<PHINode*> PhiRound;
  DominatorTree DT(*ClonedFunc);
  for (df_iterator<DomTreeNode*> DI = df_begin(DT.getRootNode()),
      DE = df_end(DT.getRootNode()); DI != DE; ++DI) {
    BasicBlock *BB = DI->getBlock();
    for (auto &I : BB->getInstList()) {
      if (!UniformInsts.count(&I)) {
        Value *pPacketizedInst = packetizeInstruction(&I);
        ReplaceMap[&I] = pPacketizedInst;
      } else {
        for (int i = 0, n = I.getNumOperands(); i < n; ++i) {
          Value *OrigValue = I.getOperand(i);
          auto iter = ReplaceMap.find(OrigValue);
          if (iter != ReplaceMap.end() && iter->second != OrigValue) {
            I.setOperand(i, iter->second);
          }
        }
      }
      if (isa<PHINode>(&I))
        PhiRound.push_back(dyn_cast<PHINode>(&I));
    }
  }
  for (auto Phi : PhiRound) {
    for (int i = 0, n = Phi->getNumOperands(); i < n; ++i) {
      Value* OrigValue = Phi->getOperand(i);
      auto iter = ReplaceMap.find(OrigValue);
      if (iter != ReplaceMap.end() && iter->second != OrigValue) {
        Phi->setOperand(i, iter->second);
      }
    }
  }

  removeDeadInstructions(*ClonedFunc);

  return ClonedFunc;
}

/***************************************************************************
 * vectorize a SIMT-entry function
 */
bool GenXPacketize::vectorizeSIMTEntry(Function &F) {
  IGC_ASSERT(F.hasFnAttribute("CMGenxSIMT"));

  // find uniform instructions related to uniform arguments
  findUniformInsts(F);

  uint32_t Width = 0;
  F.getFnAttribute("CMGenxSIMT").getValueAsString().getAsInteger(0, Width);

  B->SetTargetWidth(Width);

  ReplaceMap.clear();

  B->IRB()->SetInsertPoint(&F.getEntryBlock(), F.getEntryBlock().begin());

  // vectorize instructions in the fork-regions
  std::vector<PHINode*> PhiRound;
  DominatorTree DT(F);
  for (df_iterator<DomTreeNode*> DI = df_begin(DT.getRootNode()),
      DE = df_end(DT.getRootNode()); DI != DE; ++DI) {
    BasicBlock *BB = DI->getBlock();
    for (auto &I : BB->getInstList()) {
      if (!UniformInsts.count(&I)) {
        Value *pPacketizedInst = packetizeInstruction(&I);
        ReplaceMap[&I] = pPacketizedInst;
      } else {
        for (int i = 0, n = I.getNumOperands(); i < n; ++i) {
          Value *OrigValue = I.getOperand(i);
          auto iter = ReplaceMap.find(OrigValue);
          if (iter != ReplaceMap.end() && iter->second != OrigValue) {
            I.setOperand(i, iter->second);
          }
        }
      }
      if (auto Phi = dyn_cast<PHINode>(&I))
        PhiRound.push_back(Phi);
    }
  }
  for (auto Phi : PhiRound) {
    for (int i = 0, n = Phi->getNumOperands(); i < n; ++i) {
      Value* OrigValue = Phi->getOperand(i);
      auto iter = ReplaceMap.find(OrigValue);
      if (iter != ReplaceMap.end() && iter->second != OrigValue) {
        Phi->setOperand(i, iter->second);
      }
    }
  }
  removeDeadInstructions(F);
  // a SIMT entry is always inlined after vectorization
  // This is required in order to handle structure argument,
  // for example, generated from lambda capture.
  if (F.hasFnAttribute(Attribute::NoInline))
    F.removeFnAttr(Attribute::NoInline);
  F.addFnAttr(Attribute::AlwaysInline);
  F.removeFnAttr("CMGenxSIMT");
  F.setLinkage(GlobalValue::InternalLinkage);

  return true;
}

/************************************************************************
 * findFunctionVectorizationOrder : calculate the order we want to visit
 * functions, such that a function is not visited until all its callees
 * have been visited. Also if a function is called directly or indirectly
 * in the SIMT mode, add it to the list that need vectorization
 */
// Call graph node
struct CGNode {
  Function *F;
  std::set<CGNode *> UnvisitedCallers;
  std::set<CGNode *> Callees;
};

void GenXPacketize::findFunctionVectorizationOrder(Module *M) {
  // First build the call graph.
  // We roll our own call graph here, because it is simpler than the general
  // case supported by LLVM's call graph analysis (CM does not support
  // recursion or function pointers), and we want to modify it (using the
  // UnvisitedCallers set) when we traverse it.
  std::map<Function *, CGNode> CallGraph;
  for (auto mi = M->begin(), me = M->end(); mi != me; ++mi) {
    Function *F = &*mi;
    if (F->empty())
      continue;

    fixupLLVMIntrinsics(*F);

    // For each defined function: for each use (a call), add it to our
    // UnvisitedCallers set, and add us to its Callees set.
    // We are ignoring an illegal non-call use of a function; someone
    // else can spot and diagnose that later.
    // If the function has no callers, then add it straight in to FuncOrder.
    CGNode *CGN = &CallGraph[F];
    CGN->F = F;
    if (F->use_empty()) {
      FuncOrder.push_back(F);
      continue;
    }
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui) {
      if (auto CI = dyn_cast<CallInst>(ui->getUser())) {
        BasicBlock *Blk = CI->getParent();
        Function *Caller = Blk->getParent();
        CGNode *CallerNode = &CallGraph[Caller];
        CallerNode->F = Caller;
        CGN->UnvisitedCallers.insert(CallerNode);
        CallerNode->Callees.insert(CGN);
        // find the vectorization width of callee
        auto CallerVectorIter = FuncVectors.find(Caller);
        if (CallerVectorIter != FuncVectors.end()) {
          auto CalleeVectorIter = FuncVectors.find(F);
          if (CalleeVectorIter != FuncVectors.end())
            CalleeVectorIter->second.insert(CallerVectorIter->second.begin(),
                                            CallerVectorIter->second.end());
          else
            FuncVectors.insert(std::pair<Function *, std::set<unsigned>>(
                F, CallerVectorIter->second));
        } else if (Caller->hasFnAttribute("CMGenxSIMT")) {
          uint32_t width = 0;
          Caller->getFnAttribute("CMGenxSIMT")
              .getValueAsString()
              .getAsInteger(0, width);
          if (width > 1) {
            auto CalleeVectorIter = FuncVectors.find(F);
            if (CalleeVectorIter != FuncVectors.end())
              CalleeVectorIter->second.insert(width);
            else {
              std::set<unsigned> WidthSet;
              WidthSet.insert(width);
              FuncVectors.insert(
                  std::pair<Function *, std::set<unsigned>>(F, WidthSet));
            }
          }
        }
      }
    }
  }
  // Run through the visit order. For each function, remove it from each
  // callee's UnvisitedCallers set, and, if now empty, add the callee to
  // the end of the visit order.
  for (unsigned i = 0; i != FuncOrder.size(); ++i) {
    CGNode *CGN = &CallGraph[FuncOrder[i]];
    for (auto ci = CGN->Callees.begin(), ce = CGN->Callees.end(); ci != ce;
         ++ci) {
      CGNode *Callee = *ci;
      Callee->UnvisitedCallers.erase(CGN);
      if (Callee->UnvisitedCallers.empty())
        FuncOrder.push_back(Callee->F);
      // find the vectorization width of callee
      auto CallerVectorIter = FuncVectors.find(CGN->F);
      if (CallerVectorIter != FuncVectors.end()) {
        auto CalleeVectorIter = FuncVectors.find(Callee->F);
        if (CalleeVectorIter != FuncVectors.end())
          CalleeVectorIter->second.insert(CallerVectorIter->second.begin(),
                                          CallerVectorIter->second.end());
        else
          FuncVectors.insert(
              std::make_pair(Callee->F, CallerVectorIter->second));
      }
    }
  }
}

void GenXPacketize::findUniformArgs(Function &F) {
  auto iter = FuncVectors.find(&F);
  if (iter == FuncVectors.end()) {
    // non-simt function or simt-entry function
    for (const Argument &I : F.args())
      UniformArgs.insert(&I);
  } else {
    // simt functions that needs whole-function vectorization
    for (const Argument &I : F.args()) {
      bool IsUniform = true;
      // check every call-site
      for (User *U : F.users()) {
        if (CallInst *CI = dyn_cast<CallInst>(U)) {
          auto Def = CI->getArgOperand(I.getArgNo());
          if (Argument *DA = dyn_cast<Argument>(Def)) {
            if (!UniformArgs.count(DA)) {
              IsUniform = false;
              break;
            }
          } else {
            IsUniform = false;
            break;
          }
        } else {
          IsUniform = false;
          break;
        }
      }
      if (IsUniform)
        UniformArgs.insert(&I);
    }
  }
}

bool GenXPacketize::isUniformIntrinsic(unsigned id) {
  switch (id) {
  case GenXIntrinsic::genx_get_color:
  case GenXIntrinsic::genx_get_hwid:
  case GenXIntrinsic::genx_get_scoreboard_bti:
  case GenXIntrinsic::genx_get_scoreboard_deltas:
  case GenXIntrinsic::genx_get_scoreboard_depcnt:
  case GenXIntrinsic::genx_local_id:
  case GenXIntrinsic::genx_local_id16:
  case GenXIntrinsic::genx_local_size:
  case GenXIntrinsic::genx_group_count:
  case GenXIntrinsic::genx_group_id_x:
  case GenXIntrinsic::genx_group_id_y:
  case GenXIntrinsic::genx_group_id_z:
  case GenXIntrinsic::genx_predefined_surface:
  case GenXIntrinsic::genx_barrier:
  case GenXIntrinsic::genx_sbarrier:
  case GenXIntrinsic::genx_nbarrier:
  case GenXIntrinsic::genx_cache_flush:
  case GenXIntrinsic::genx_fence:
  case GenXIntrinsic::genx_wait:
  case GenXIntrinsic::genx_yield:
  case GenXIntrinsic::genx_print_buffer:
  case GenXIntrinsic::genx_r0:
  case GenXIntrinsic::genx_sr0:
  case GenXIntrinsic::genx_timestamp:
  case GenXIntrinsic::genx_thread_x:
  case GenXIntrinsic::genx_thread_y:
      return true;
  default:
    break;
  }
  return false;
}

void GenXPacketize::findUniformInsts(Function &F) {
  // global variable load is uniform
  for (auto &Global : M->getGlobalList()) {
    for (auto UI = Global.use_begin(), UE = Global.use_end(); UI != UE; ++UI) {
      if (auto LD = dyn_cast<LoadInst>(UI->getUser())) {
        UniformInsts.insert(LD);
      }
    }
  }
  // some intrinsics are always uniform
  for (auto &FD : M->getFunctionList()) {
    if (FD.isDeclaration()) {
      if (isUniformIntrinsic(GenXIntrinsic::getGenXIntrinsicID(&FD))) {
        for (auto UI = FD.use_begin(), UE = FD.use_end(); UI != UE; ++UI) {
          if (auto Inst = dyn_cast<Instruction>(UI->getUser())) {
            UniformInsts.insert(Inst);
          }
        }
      }
    }
  }
  std::set<const Value*> argDefs;
  std::stack<Value*> uvset;
  for (const Argument& I : F.args()) {
    if (!UniformArgs.count(&I))
      continue;
    argDefs.insert(&I);
    for (auto UI = I.user_begin(), E = I.user_end(); UI != E; ++UI) {
      const Value* use = (*UI);
      if (auto CastI = dyn_cast<CastInst>(use)) {
        argDefs.insert(CastI);
        UniformInsts.insert(CastI);
        uvset.push((Value*)CastI);
      }
    }
  }
  // first find out all the uniform alloca to store those uniform arguments
  for (auto def : argDefs) {
    for (auto UI = def->user_begin(), E = def->user_end(); UI != E; ++UI) {
      const Value* use = (*UI);
      if (auto LI = dyn_cast<LoadInst>(use)) {
        UniformInsts.insert(LI);
      } else if (auto GEP = dyn_cast<GetElementPtrInst>(use)) {
        if (GEP->getPointerOperand() == def) {
          UniformInsts.insert(GEP);
          uvset.push((Value *)GEP);
        }
      } else if (auto SI = dyn_cast<StoreInst>(use)) {
        if (SI->getPointerOperand() == def)
          UniformInsts.insert(SI);
        else {
          auto PI = SI->getPointerOperand();
          if (auto AI = dyn_cast<AllocaInst>(PI)) {
            UniformInsts.insert(AI);
            uvset.push((Value *)AI);
          }
        }
      } else if (auto *Cast = dyn_cast<BitCastInst>(use)) {
        UniformInsts.insert(Cast);
        uvset.push((Value *)Cast);
      } else if (auto CI = dyn_cast<CallInst>(use)) {
        if (Function *Callee = CI->getCalledFunction()) {
          if (GenXIntrinsic::isVLoadStore(Callee)) {
            UniformInsts.insert(CI);
          }
        }
      }
    }
  }

  // then find the uniform loads and stores in fork-region
  while (!uvset.empty()) {
    Value *Def = uvset.top();
    uvset.pop();
    for (auto UI = Def->user_begin(), E = Def->user_end(); UI != E; ++UI) {
      Value *use = (*UI);
      if (auto UseI = dyn_cast<Instruction>(use)) {
        if (isa<StoreInst>(UseI)) {
          UniformInsts.insert(UseI);
        } else if (auto LI = dyn_cast<LoadInst>(UseI)) {
          UniformInsts.insert(UseI);
          if (LI->getType()->isPointerTy())
            uvset.push(UseI);
        } else if (auto GEP = dyn_cast<GetElementPtrInst>(UseI)) {
          if (GEP->hasAllConstantIndices()) {
            uvset.push(UseI);
            UniformInsts.insert(UseI);
          }
        } else if (auto *Cast = dyn_cast<BitCastInst>(UseI)) {
          uvset.push(UseI);
          UniformInsts.insert(UseI);
        }
      }
    }
  }
  return;
}

Value *GenXPacketize::getPacketizeValue(Value *OrigValue) {
  auto iter = ReplaceMap.find(OrigValue);
  if (iter != ReplaceMap.end()) {
    return iter->second;
  } else if (auto C = dyn_cast<Constant>(OrigValue)) {
    return packetizeConstant(C);
  } else if (auto A = dyn_cast<Argument>(OrigValue)) {
    if (UniformArgs.count(A))
      return B->VBROADCAST(OrigValue, OrigValue->getName());
    // otherwise the argument should have been in the right vector form
    ReplaceMap[OrigValue] = OrigValue;
    return OrigValue;
  } else if (auto Inst = dyn_cast<Instruction>(OrigValue)) {
    // need special handling for alloca
    if (auto AI = dyn_cast<AllocaInst>(OrigValue)) {
      // this is not a uniform alloca
      if (!UniformInsts.count(Inst)) {
        Type *VecType = B->GetVectorType(AI->getAllocatedType());
        auto V = B->ALLOCA(VecType, nullptr, AI->getName());
        V->removeFromParent();
        V->insertBefore(Inst);
        ReplaceMap[OrigValue] = V;
        return V;
      }
      ReplaceMap[OrigValue] = OrigValue;
      return OrigValue;
    } else if (UniformInsts.count(Inst)) {
      auto V = B->VBROADCAST(OrigValue);
      return V;
    }
  }

  return nullptr;
}

// this is used on operands that are expected to be uniform
Value *GenXPacketize::getUniformValue(Value *OrigValue) {
  if (auto G = dyn_cast<GlobalValue>(OrigValue))
    return G;
  if (auto C = dyn_cast<Constant>(OrigValue))
    return C;
  if (auto A = dyn_cast<Argument>(OrigValue)) {
    if (UniformArgs.count(A)) {
      return A;
    }
  }
  if (auto A = dyn_cast<Instruction>(OrigValue)) {
    if (UniformInsts.count(A)) {
      return A;
    }
  }
  auto VV = getPacketizeValue(OrigValue);
  return B->VEXTRACT(VV, (uint64_t)0, OrigValue->getName());
}

//////////////////////////////////////////////////////////////////////////
/// @brief Returns the equivalent vector intrinsic for the input scalar
/// intrinsic
Function *GenXPacketize::getVectorIntrinsic(Module *M, unsigned ID,
                                            ArrayRef<Type *> ArgTy) {
  if (GenXIntrinsic::isGenXIntrinsic(ID))
    return GenXIntrinsic::getGenXDeclaration(
        M, static_cast<GenXIntrinsic::ID>(ID), ArgTy);
  return Intrinsic::getDeclaration(M, static_cast<Intrinsic::ID>(ID),
                                   {ArgTy[0]});
}

//////////////////////////////////////////////////////////////////////////
/// @brief Determines if instruction is an llvm intrinsic (which may include
///        x86 intrinsics
static bool IsLLVMIntrinsic(Instruction *pInst) {
  if (isa<CallInst>(pInst)) {
    CallInst *call = cast<CallInst>(pInst);
    Function *f = call->getCalledFunction();
    IGC_ASSERT(f);
    return f->isIntrinsic();
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Packetize a scalar constant
Value *GenXPacketize::packetizeConstant(Constant *pConstant) {
  if (isa<UndefValue>(pConstant)) {
    return UndefValue::get(B->GetVectorType(pConstant->getType()));
  } else {
    return B->VBROADCAST(pConstant);
  }
}

//////////////////////////////////////////////////////////////////////////
/// @brief Packetize an LLVM intrinsic.  Generally this means replacing
///        a scalar intrinsic function call with a vectored equivalent.
Value *GenXPacketize::packetizeLLVMIntrinsic(Instruction *pInst) {
  Module *M = B->mpModule;

  B->IRB()->SetInsertPoint(pInst);
  CallInst *pCall = cast<CallInst>(pInst);
  Function *f = pCall->getCalledFunction();
  IGC_ASSERT(f);
  IGC_ASSERT(f->isIntrinsic());
  // not sure how to handle debug intrinsics, just return
  if (isa<DbgInfoIntrinsic>(pInst))
    return pInst;

  auto id = GenXIntrinsic::getAnyIntrinsicID(f);

  // packetize intrinsic operands
  std::vector<Type *> vectorArgTys;
  std::vector<Value *> packetizedArgs;
  for (auto &operand : pCall->args()) {
    auto VV = getPacketizeValue(operand.get());
    packetizedArgs.push_back(VV);
    vectorArgTys.push_back(VV->getType());
  }

  // override certain intrinsics
  Value *pNewCall;
  switch (id) {
  case Intrinsic::log2:
    pNewCall = B->VLOG2PS(packetizedArgs[0]);
    break;
  case Intrinsic::exp2:
    pNewCall = B->VEXP2PS(packetizedArgs[0]);
    break;
  default: {
    Function *newF = getVectorIntrinsic(M, id, vectorArgTys);
    pNewCall = CallInst::Create(newF, packetizedArgs, "", pCall);
  }
  }
  return pNewCall;
}

Value *GenXPacketize::packetizeLLVMInstruction(Instruction *pInst) {
  Value *pReplacedInst = nullptr;
  B->IRB()->SetInsertPoint(pInst);
  // packetize a call
  if (auto CI = dyn_cast<CallInst>(pInst)) {
    auto F = CI->getCalledFunction();
    auto FMI = FuncMap.find(std::pair<Function *, unsigned>(F, B->mVWidth));
    if (FMI != FuncMap.end()) {
      std::vector<Value *> ArgOps;
      auto VF = FMI->second;
      for (Argument &Arg : VF->args()) {
        auto i = Arg.getArgNo();
        if (UniformArgs.count(&Arg))
          ArgOps.push_back(getUniformValue(CI->getArgOperand(i)));
        else
          ArgOps.push_back(getPacketizeValue(CI->getArgOperand(i)));
      }
      pReplacedInst = CallInst::Create(VF, ArgOps, CI->getName(), CI);
      return pReplacedInst;
    } else
      IGC_ASSERT(0);
  }
  uint32_t opcode = pInst->getOpcode();

  switch (opcode) {
  case Instruction::AddrSpaceCast:
  case Instruction::BitCast: {
    // packetize the bitcast source
    Value *pPacketizedSrc = getPacketizeValue(pInst->getOperand(0));
    Type *pPacketizedSrcTy = pPacketizedSrc->getType();

    // packetize dst type
    Type *pReturnTy;
    if (pInst->getType()->isPointerTy()) {
      // two types of pointers, <N x Ty>* or <N x Ty*>
      Type *pDstScalarTy = IGCLLVM::getNonOpaquePtrEltTy(pInst->getType());

      if (pPacketizedSrc->getType()->isVectorTy()) {
        // <N x Ty*>
        Type *pDstPtrTy = PointerType::get(
            pDstScalarTy, pInst->getType()->getPointerAddressSpace());
        uint32_t numElems =
            cast<IGCLLVM::FixedVectorType>(pPacketizedSrcTy)->getNumElements();
        pReturnTy = IGCLLVM::FixedVectorType::get(pDstPtrTy, numElems);
      }
      else {
        // <N x Ty>*
        if (VectorType::isValidElementType(pDstScalarTy))
          // Map <N x OldTy>* to <N x NewTy>*
          pReturnTy = PointerType::get(B->GetVectorType(pDstScalarTy),
                          pInst->getType()->getPointerAddressSpace());
        else {
          // Map <N x OldTy>* to <N x NewTy*> using cast then GEP
          auto* pTmpTy = PointerType::get(
                            llvm::ArrayType::get(pDstScalarTy, B->mVWidth),
                            pInst->getType()->getPointerAddressSpace());
          auto* pTmpInst =
              B->CAST((Instruction::CastOps)opcode, pPacketizedSrc, pTmpTy);
          SmallVector<Value*, 2> vecIndices;
          vecIndices.push_back(B->C(0));
          vecIndices.push_back(B->CInc<uint32_t>(0, B->mVWidth));
          pReplacedInst = B->GEPA(pTmpInst, vecIndices);
          break;
        }
      }
    } else {
      pReturnTy = B->GetVectorType(pInst->getType());
    }

    pReplacedInst =
        B->CAST((Instruction::CastOps)opcode, pPacketizedSrc, pReturnTy);
    break;
  }

  case Instruction::GetElementPtr: {
    GetElementPtrInst *pGepInst = cast<GetElementPtrInst>(pInst);
    auto pBase = pGepInst->getPointerOperand();
    Value *pVecSrc = nullptr;
    if (dyn_cast<GlobalValue>(pBase))
      pVecSrc = pBase;
    else if (dyn_cast<Argument>(pBase))
      pVecSrc = pBase;
    else if (dyn_cast<Instruction>(pBase) &&
             UniformInsts.count(dyn_cast<Instruction>(pBase)))
      pVecSrc = pBase;
    else
      pVecSrc = getPacketizeValue(pBase);

    if (!isa<AllocaInst>(pVecSrc)) {
      // just packetize the GEP to a vector GEP.
      SmallVector<Value *, 8> vecIndices;
      for (uint32_t i = 0; i < pGepInst->getNumIndices(); ++i) {
        vecIndices.push_back(getPacketizeValue(pGepInst->getOperand(1 + i)));
      }
      pReplacedInst = B->GEPA(pVecSrc, vecIndices);
    } else {
      if (pGepInst->hasAllConstantIndices()) {
        // SOA GEP with scalar src and constant indices, result will be <N x
        // Ty>* Ex. gep [4 x <8 x float>]*, 0, 0 --> <8 x float>*
        SmallVector<Value *, 8> vecIndices;
        for (uint32_t i = 0; i < pGepInst->getNumIndices(); ++i) {
          vecIndices.push_back(pGepInst->getOperand(1 + i));
        }
        pReplacedInst = B->GEPA(pVecSrc, vecIndices);
      } else {
        //// SOA GEP with non-uniform indices. Need to vector GEP to each SIMD
        /// lane.
        /// Result will be <N x Ty*>
        SmallVector<Value *, 8> vecIndices;
        for (uint32_t i = 0; i < pGepInst->getNumIndices(); ++i) {
          vecIndices.push_back(getPacketizeValue(pGepInst->getOperand(1 + i)));
        }
        // Step to the SIMD lane
        vecIndices.push_back(B->CInc<uint32_t>(0, B->mVWidth));
        pReplacedInst = B->GEPA(pVecSrc, vecIndices);
      }
    }
    break;
  }

  case Instruction::Load: {
    LoadInst *pLoadInst = cast<LoadInst>(pInst);
    Value *pSrc = pLoadInst->getPointerOperand();
    Value *pVecSrc = getPacketizeValue(pSrc);
    auto LI = cast<LoadInst>(pInst);
    if (pVecSrc->getType()->isVectorTy()) {
      IGC_ASSERT(cast<VectorType>(pVecSrc->getType())
                     ->getElementType()
                     ->isPointerTy());
      auto Align = IGCLLVM::getAlignmentValue(LI);
      pReplacedInst = B->MASKED_GATHER(pVecSrc, Align);
    } else
      pReplacedInst = B->ALIGNED_LOAD(pVecSrc, IGCLLVM::getAlign(*LI));
    break;
  }

  case Instruction::Store: {
    StoreInst *pStoreInst = cast<StoreInst>(pInst);
    Value *pVecDstPtrs = getPacketizeValue(pStoreInst->getPointerOperand());
    Value *pVecSrc = getPacketizeValue(pStoreInst->getOperand(0));
    if (pVecDstPtrs->getType()->isVectorTy()) {
      IGC_ASSERT(cast<VectorType>(pVecDstPtrs->getType())
                     ->getElementType()
                     ->isPointerTy());
      auto Align = IGCLLVM::getAlignmentValue(cast<StoreInst>(pInst));
      pReplacedInst = B->MASKED_SCATTER(pVecSrc, pVecDstPtrs, Align);
    } else {
      pReplacedInst = B->STORE(pVecSrc, pVecDstPtrs);
    }
    break;
  }

  case Instruction::ExtractElement: {
    auto OldVec = pInst->getOperand(0);
    auto Vec = getPacketizeValue(OldVec);
    auto Idx = pInst->getOperand(1);
    auto N =
        cast<IGCLLVM::FixedVectorType>(OldVec->getType())->getNumElements();
    auto ElemType = pInst->getType();
    auto VecDstTy = IGCLLVM::FixedVectorType::get(ElemType, B->mVWidth);
    // create an read-region
    vc::CMRegion R(VecDstTy);
    if (ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
      R.Offset = CI->getSExtValue() * ElemType->getPrimitiveSizeInBits() / 8;
      R.Indirect = nullptr;
    } else {
      R.Offset = 0;
      auto MulCType = IntegerType::getInt16Ty(M->getContext());
      auto MulC =
          ConstantInt::get(MulCType, ElemType->getPrimitiveSizeInBits() / 8);
      auto NBits = Idx->getType()->getIntegerBitWidth();
      if (NBits > 16)
          Idx = B->TRUNC(Idx, MulCType);
      else if (NBits < 16)
          Idx = B->S_EXT(Idx, MulCType);
      R.Indirect = B->MUL(Idx, MulC);
    }
    R.NumElements = B->mVWidth;
    R.Width = B->mVWidth;
    R.Stride = N;
    R.VStride = 0;
    pReplacedInst =
        R.createRdRegion(Vec, pInst->getName(), pInst /*InsertBefore*/,
                         pInst->getDebugLoc(), true /*AllowScalar*/);
    break;
  }

  case Instruction::InsertElement: {
    auto OldVec = pInst->getOperand(0);
    auto Vec = getPacketizeValue(OldVec);
    auto ElmVec = getPacketizeValue(pInst->getOperand(1));
    auto Idx = pInst->getOperand(2);
    auto N =
        cast<IGCLLVM::FixedVectorType>(OldVec->getType())->getNumElements();
    auto ElemType = pInst->getOperand(1)->getType();
    // create an write-region
    vc::CMRegion R(Vec->getType());
    if (ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
      // special case, this is really just like a bitcast
      if (CI->getZExtValue() == 0 && N == 1 && isa<UndefValue>(OldVec)) {
        auto pRetTy = B->GetVectorType(pInst->getType());
        pReplacedInst = B->BITCAST(ElmVec, pRetTy);
        break;
      }
      R.Offset = CI->getSExtValue() * ElemType->getPrimitiveSizeInBits() / 8;
      R.Indirect = nullptr;
    } else {
      R.Offset = 0;
      auto MulCType = IntegerType::getInt16Ty(M->getContext());
      auto MulC =
          ConstantInt::get(MulCType, ElemType->getPrimitiveSizeInBits() / 8);
      auto NBits = Idx->getType()->getIntegerBitWidth();
      if (NBits > 16)
          Idx = B->TRUNC(Idx, MulCType);
      else if (NBits < 16)
          Idx = B->S_EXT(Idx, MulCType);
      R.Indirect = B->MUL(Idx, MulC);
    }
    R.NumElements = B->mVWidth;
    R.Width = B->mVWidth;
    R.Stride = N;
    R.VStride = 0;
    pReplacedInst =
        R.createWrRegion(Vec, ElmVec, pInst->getName(), pInst /*InsertBefore*/,
                         pInst->getDebugLoc());
    break;
  }

  case Instruction::Br: {
    // any conditional branches with vectored conditions need to preceded with
    // a genx_simdcf_any to ensure we branch iff all lanes are set
    BranchInst *pBranch = cast<BranchInst>(pInst);
    if (pBranch->isConditional()) {
      Value *vCondition = getPacketizeValue(pBranch->getCondition());
      llvm::Function *NewFn = GenXIntrinsic::getGenXDeclaration(
        B->mpModule,
        GenXIntrinsic::genx_simdcf_any,
        vCondition->getType());
      llvm::CallInst *NewTest = CallInst::Create(NewFn, vCondition, "", pInst);
      NewTest->setName("exit.cond.mask.test");
      pBranch->setCondition(NewTest);
    }
    pReplacedInst = pBranch;
    break;
  }

  case Instruction::Alloca: {
    AllocaInst *pAllocaInst = cast<AllocaInst>(pInst);
    Type *pVecType = B->GetVectorType(pAllocaInst->getAllocatedType());
    Value *pReturn = B->ALLOCA(pVecType, nullptr, pInst->getName());
    pReplacedInst = pReturn;
    break;
  }

  case Instruction::ShuffleVector: {
    auto Src1 = pInst->getOperand(0);
    auto Src2 = pInst->getOperand(1);
    auto Mask = IGCLLVM::getShuffleMaskForBitcode(cast<ShuffleVectorInst>(pInst));
    if (cast<IGCLLVM::FixedVectorType>(Src1->getType())->getNumElements() ==
            1 &&
        cast<IGCLLVM::FixedVectorType>(Mask->getType())->getNumElements() ==
            1) {
      if (cast<Constant>(Mask)->isAllOnesValue())
        pReplacedInst = getPacketizeValue(Src2);
      else
        pReplacedInst = getPacketizeValue(Src1);
    } else
      report_fatal_error(
          "ShuffleVector should've been replaced by Scalarizer.");
    break;
  }

  case Instruction::IntToPtr: {
    IntToPtrInst *pIntToPtrInst = cast<IntToPtrInst>(pInst);
    Value *pVecSrc = getPacketizeValue(pInst->getOperand(0));
    Type *pVecDestTy =
        IGCLLVM::FixedVectorType::get(pIntToPtrInst->getDestTy(), B->mVWidth);
    pReplacedInst = B->INT_TO_PTR(pVecSrc, pVecDestTy);
    break;
  }

  case Instruction::Select: {
    Value *pVecCond = getPacketizeValue(pInst->getOperand(0));
    Value *pTrueSrc = getPacketizeValue(pInst->getOperand(1));
    Value *pFalseSrc = getPacketizeValue(pInst->getOperand(2));

    if (!pTrueSrc->getType()->isPointerTy()) {
      // simple select packetization
      pReplacedInst = B->SELECT(pVecCond, pTrueSrc, pFalseSrc);
    } else {
      // vector struct input, need to loop over components and build up new
      // struct allocation
      Value *pAlloca = B->ALLOCA(
          B->GetVectorType(IGCLLVM::getNonOpaquePtrEltTy(pInst->getType())));
      uint32_t numElems =
          IGCLLVM::getNonOpaquePtrEltTy(pInst->getType())->getArrayNumElements();

      for (uint32_t i = 0; i < numElems; ++i) {
        Value *pTrueSrcElem = B->LOAD(pTrueSrc, {0, i});
        Value *pFalseSrcElem = B->LOAD(pFalseSrc, {0, i});

        // mask store true components
        Value *pGep = B->GEP(pAlloca, {0, i});
        B->MASKED_STORE(pTrueSrcElem, pGep, 4, pVecCond);

        // store false components to inverted mask
        B->MASKED_STORE(pFalseSrcElem, pGep, 4, B->NOT(pVecCond));
      }
      pReplacedInst = pAlloca;
    }
    break;
  }

  case Instruction::Ret: {
    ReturnInst *pRet = cast<ReturnInst>(pInst);
    if (pRet->getReturnValue() != nullptr) {
      Value *pReturn = getPacketizeValue(pRet->getReturnValue());
      ReturnInst *pNewRet = B->RET(pReturn);
      pReplacedInst = pNewRet;
    } else {
      pReplacedInst = pInst;
    }

    break;
  }
  default: {
    // for the rest of the instructions includingi phi, vectorize
    // the instruction type as well as its args
    Type *vecType = B->GetVectorType(pInst->getType());

    // Set vectorized dbg value to undef, since currently it's not
    // salvageble
    llvm::replaceDbgUsesWithUndef(pInst);
    pInst->mutateType(vecType);

    for (Use &op : pInst->operands()) {
      auto v = getPacketizeValue(op.get());
      if (v)
        op.set(v);
      else if (!isa<PHINode>(pInst))
        report_fatal_error("Cannot find packetized value!");
    }
    pReplacedInst = pInst;
  }
  }

  return pReplacedInst;
}

Value *GenXPacketize::packetizeGenXIntrinsic(Instruction *inst) {
  B->IRB()->SetInsertPoint(inst);

  if (auto CI = dyn_cast_or_null<CallInst>(inst)) {
    if (Function *Callee = CI->getCalledFunction()) {
      auto IID = GenXIntrinsic::getGenXIntrinsicID(Callee);
      Value *replacement = nullptr;
      // some intrinsics are uniform therefore should not get here
      IGC_ASSERT(!isUniformIntrinsic(IID));
      switch (IID) {
      case GenXIntrinsic::genx_line:
      case GenXIntrinsic::genx_pln:
      case GenXIntrinsic::genx_dp2:
      case GenXIntrinsic::genx_dp3:
      case GenXIntrinsic::genx_dp4:
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
      case GenXIntrinsic::genx_dph:
      case GenXIntrinsic::genx_transpose_ld:
      case GenXIntrinsic::genx_oword_ld:
      case GenXIntrinsic::genx_oword_ld_unaligned:
      case GenXIntrinsic::genx_oword_st:
      case GenXIntrinsic::genx_svm_block_ld:
      case GenXIntrinsic::genx_svm_block_ld_unaligned:
      case GenXIntrinsic::genx_svm_block_st:
      case GenXIntrinsic::genx_load:
      case GenXIntrinsic::genx_3d_load:
      case GenXIntrinsic::genx_3d_sample:
      case GenXIntrinsic::genx_avs:
      case GenXIntrinsic::genx_sample:
      case GenXIntrinsic::genx_sample_unorm:
      case GenXIntrinsic::genx_simdcf_any:
      case GenXIntrinsic::genx_simdcf_goto:
      case GenXIntrinsic::genx_simdcf_join:
      case GenXIntrinsic::genx_simdcf_predicate:
      case GenXIntrinsic::genx_rdpredregion:
      case GenXIntrinsic::genx_wrconstregion:
      case GenXIntrinsic::genx_wrpredregion:
      case GenXIntrinsic::genx_wrpredpredregion:
      case GenXIntrinsic::genx_output:
      case GenXIntrinsic::genx_va_1d_convolve_horizontal:
      case GenXIntrinsic::genx_va_1d_convolve_vertical:
      case GenXIntrinsic::genx_va_1pixel_convolve:
      case GenXIntrinsic::genx_va_1pixel_convolve_1x1mode:
      case GenXIntrinsic::genx_va_bool_centroid:
      case GenXIntrinsic::genx_va_centroid:
      case GenXIntrinsic::genx_va_convolve2d:
      case GenXIntrinsic::genx_va_correlation_search:
      case GenXIntrinsic::genx_va_dilate:
      case GenXIntrinsic::genx_va_erode:
      case GenXIntrinsic::genx_va_flood_fill:
      case GenXIntrinsic::genx_va_hdc_1d_convolve_horizontal:
      case GenXIntrinsic::genx_va_hdc_1d_convolve_vertical:
      case GenXIntrinsic::genx_va_hdc_1pixel_convolve:
      case GenXIntrinsic::genx_va_hdc_convolve2d:
      case GenXIntrinsic::genx_va_hdc_dilate:
      case GenXIntrinsic::genx_va_hdc_erode:
      case GenXIntrinsic::genx_va_hdc_lbp_correlation:
      case GenXIntrinsic::genx_va_hdc_lbp_creation:
      case GenXIntrinsic::genx_va_hdc_minmax_filter:
      case GenXIntrinsic::genx_va_lbp_correlation:
      case GenXIntrinsic::genx_va_lbp_creation:
      case GenXIntrinsic::genx_va_minmax:
      case GenXIntrinsic::genx_va_minmax_filter:
      case GenXIntrinsic::genx_media_ld:
      case GenXIntrinsic::genx_media_st:
      case GenXIntrinsic::genx_raw_send:
      case GenXIntrinsic::genx_raw_send_noresult:
      case GenXIntrinsic::genx_raw_sends:
      case GenXIntrinsic::genx_raw_sends_noresult:
        report_fatal_error("Unsupported genx intrinsic in SIMT mode.");
        return nullptr;
      case GenXIntrinsic::genx_dword_atomic_add:
      case GenXIntrinsic::genx_dword_atomic_sub:
      case GenXIntrinsic::genx_dword_atomic_min:
      case GenXIntrinsic::genx_dword_atomic_max:
      case GenXIntrinsic::genx_dword_atomic_xchg:
      case GenXIntrinsic::genx_dword_atomic_and:
      case GenXIntrinsic::genx_dword_atomic_or:
      case GenXIntrinsic::genx_dword_atomic_xor:
      case GenXIntrinsic::genx_dword_atomic_imin:
      case GenXIntrinsic::genx_dword_atomic_imax:
      case GenXIntrinsic::genx_dword_atomic_fmin:
      case GenXIntrinsic::genx_dword_atomic_fmax:
      case GenXIntrinsic::genx_dword_atomic_fadd:
      case GenXIntrinsic::genx_dword_atomic_fsub:
      {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType(), Src2->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_inc:
      case GenXIntrinsic::genx_dword_atomic_dec: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Args[] = {Src0, BTI, Src2, Src3};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_fcmpwr: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType(), Src2->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_cmpxchg: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_bfn: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *Src1 = getPacketizeValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *BFN = getUniformValue(CI->getOperand(3));
        Value *Args[] = {Src0, Src1, Src2, BFN};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_svm_gather: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *NBlk = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(NBlk));
        Value *Addr = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Args[] = {Predicate, NBlk, Addr, Src3};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Predicate->getType(), Addr->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_svm_scatter: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *NBlk = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(NBlk));
        Value *Addr = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Args[] = {Predicate, NBlk, Addr, Src3};
        // store, no return type
        Type *Tys[] = {Predicate->getType(), Addr->getType(), Src3->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_svm_gather4_scaled: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *ChMask = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(ChMask));
        Value *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        Value *Addr = getUniformValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Predicate, ChMask, Scale, Addr, Src4, Src5};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Predicate->getType(), Src4->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_svm_scatter4_scaled: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *ChMask = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(ChMask));
        Value *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        Value *Addr = getUniformValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Predicate, ChMask, Scale, Addr, Src4, Src5};
        // store no return type
        Type *Tys[] = {Predicate->getType(), Addr->getType(), Src4->getType(),
                       Src5->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_gather4_typed: {
        Value *ChMask = CI->getOperand(0);
        IGC_ASSERT(isa<Constant>(ChMask));
        Value *Predicate = getPacketizeValue(CI->getOperand(1));
        Value *BTI = getUniformValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Src6 = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {ChMask, Predicate, BTI, Src3, Src4, Src5, Src6};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Predicate->getType(), Src3->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_scatter4_typed: {
        Value *ChMask = CI->getOperand(0);
        IGC_ASSERT(isa<Constant>(ChMask));
        Value *Predicate = getPacketizeValue(CI->getOperand(1));
        Value *BTI = getUniformValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Src6 = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {ChMask, Predicate, BTI, Src3, Src4, Src5, Src6};
        // store no return type
        Type *Tys[] = {Predicate->getType(), Src3->getType(), Src6->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_scatter4_scaled:
      case GenXIntrinsic::genx_scatter_scaled: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *NBlk = CI->getOperand(1); // or channel mask for scatter4
        IGC_ASSERT(isa<Constant>(NBlk));
        Value *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        Value *BTI = getUniformValue(CI->getOperand(3));
        Value *GOff = getUniformValue(CI->getOperand(4));
        Value *ElemOffsets = getPacketizeValue(CI->getOperand(5));
        Value *InData = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {Predicate, NBlk,        Scale, BTI,
                         GOff,      ElemOffsets, InData};
        // no return value for store
        Type *Tys[] = {Args[0]->getType(), Args[5]->getType(),
                       Args[6]->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_gather4_scaled:
      case GenXIntrinsic::genx_gather_scaled: {
        Value *Predicate = getPacketizeValue(CI->getOperand(0));
        Value *NBlk = CI->getOperand(1); // or channel mask for gather4
        IGC_ASSERT(isa<Constant>(NBlk));
        Value *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        Value *BTI = getUniformValue(CI->getOperand(3));
        Value *GOff = getUniformValue(CI->getOperand(4));
        Value *ElemOffsets = getPacketizeValue(CI->getOperand(5));
        Value *InData = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {Predicate, NBlk,        Scale, BTI,
                         GOff,      ElemOffsets, InData};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Args[0]->getType(), Args[5]->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_gather4_masked_scaled2:
      case GenXIntrinsic::genx_gather_masked_scaled2:
      case GenXIntrinsic::genx_gather4_scaled2:
      case GenXIntrinsic::genx_gather_scaled2: {
        SmallVector<Value *, 6> Args;
        SmallVector<Type *, 3> Tys;
        Args.push_back(CI->getOperand(0)); // Nblk
        IGC_ASSERT(isa<Constant>(Args.back()));
        Args.push_back(CI->getOperand(1)); // Scale
        IGC_ASSERT(isa<Constant>(Args.back()));
        Args.push_back(getUniformValue(CI->getOperand(2))); // BTI
        Args.push_back(getUniformValue(CI->getOperand(3))); // GOff
        Value *ElemOffsets = getPacketizeValue(CI->getOperand(4));
        Args.push_back(ElemOffsets);
        Tys.push_back(B->GetVectorType(CI->getType()));
        Tys.push_back(ElemOffsets->getType());
        if (IID == GenXIntrinsic::genx_gather4_masked_scaled2 ||
            IID == GenXIntrinsic::genx_gather_masked_scaled2) {
          Args.push_back(getPacketizeValue(CI->getOperand(5))); // Predicate
          Tys.push_back(Args.back()->getType());
        }
        Function *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      }
      case GenXIntrinsic::genx_lane_id: {
        IGC_ASSERT_MESSAGE((CI->getType()->getIntegerBitWidth() == 32),
          "Expected to return 32-bit integer.");
        replacement = B->CInc<uint32_t>(0, B->mVWidth);
        return replacement;
      } break;
      case GenXIntrinsic::genx_rdregionf:
      case GenXIntrinsic::genx_rdregioni: {
        // packetize intrinsic operands
        const DebugLoc &DL = CI->getDebugLoc();
        auto OrigV0 = CI->getOperand(0);
        vc::CMRegion R(CI);
        IGC_ASSERT(R.Width == 1);
        if (cast<IGCLLVM::FixedVectorType>(OrigV0->getType())
                ->getNumElements() == 1) {
          replacement = getPacketizeValue(OrigV0);
        } else {
          R.NumElements = B->mVWidth;
          if (R.Indirect) {
            R.Indirect = getPacketizeValue(R.Indirect);
          }
          replacement = R.createRdRegion(getPacketizeValue(OrigV0),
                                         CI->getName(), CI, DL);
        }
        return replacement;
      } break;
      case GenXIntrinsic::genx_wrregionf:
      case GenXIntrinsic::genx_wrregioni: {
        auto NewV0 = CI->getOperand(1);
        const DebugLoc &DL = CI->getDebugLoc();
        vc::CMRegion R(CI);
        IGC_ASSERT(isa<IGCLLVM::FixedVectorType>(NewV0->getType()));
        IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(NewV0->getType())
                       ->getNumElements() == 1);
        auto NewV1 = getPacketizeValue(NewV0);
        R.NumElements = B->mVWidth;
        if (R.Indirect) {
          R.Indirect = getPacketizeValue(R.Indirect);
        }
        replacement =
            R.createWrRegion(CI->getOperand(0), NewV1, CI->getName(), CI, DL);
        return replacement;
      } break;
      case GenXIntrinsic::genx_untyped_atomic_add:
      case GenXIntrinsic::genx_untyped_atomic_sub:
      case GenXIntrinsic::genx_untyped_atomic_min:
      case GenXIntrinsic::genx_untyped_atomic_max:
      case GenXIntrinsic::genx_untyped_atomic_xchg:
      case GenXIntrinsic::genx_untyped_atomic_and:
      case GenXIntrinsic::genx_untyped_atomic_or:
      case GenXIntrinsic::genx_untyped_atomic_xor:
      case GenXIntrinsic::genx_untyped_atomic_imin:
      case GenXIntrinsic::genx_untyped_atomic_imax: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *GOFF = getUniformValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Src0, BTI, GOFF, Src3, Src4, Src5};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_untyped_atomic_inc:
      case GenXIntrinsic::genx_untyped_atomic_dec: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *GOFF = getUniformValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Args[] = {Src0, BTI, GOFF, Src3, Src4};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_untyped_atomic_cmpxchg: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *GOFF = getUniformValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Src6 = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {Src0, BTI, GOFF, Src3, Src4, Src5, Src6};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;

      case GenXIntrinsic::genx_typed_atomic_add:
      case GenXIntrinsic::genx_typed_atomic_sub:
      case GenXIntrinsic::genx_typed_atomic_min:
      case GenXIntrinsic::genx_typed_atomic_max:
      case GenXIntrinsic::genx_typed_atomic_xchg:
      case GenXIntrinsic::genx_typed_atomic_and:
      case GenXIntrinsic::genx_typed_atomic_or:
      case GenXIntrinsic::genx_typed_atomic_xor:
      case GenXIntrinsic::genx_typed_atomic_imin:
      case GenXIntrinsic::genx_typed_atomic_imax:
      case GenXIntrinsic::genx_typed_atomic_fmin:
      case GenXIntrinsic::genx_typed_atomic_fmax:
      case GenXIntrinsic::genx_typed_atomic_fadd:
      case GenXIntrinsic::genx_typed_atomic_fsub:
      {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Src6 = getPacketizeValue(CI->getOperand(6));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4, Src5, Src6};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType(), Src3->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_typed_atomic_inc:
      case GenXIntrinsic::genx_typed_atomic_dec: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType(), Src2->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      case GenXIntrinsic::genx_typed_atomic_fcmpwr:
      case GenXIntrinsic::genx_typed_atomic_cmpxchg: {
        Value *Src0 = getPacketizeValue(CI->getOperand(0));
        Value *BTI = getUniformValue(CI->getOperand(1));
        Value *Src2 = getPacketizeValue(CI->getOperand(2));
        Value *Src3 = getPacketizeValue(CI->getOperand(3));
        Value *Src4 = getPacketizeValue(CI->getOperand(4));
        Value *Src5 = getPacketizeValue(CI->getOperand(5));
        Value *Src6 = getPacketizeValue(CI->getOperand(6));
        Value *Src7 = getPacketizeValue(CI->getOperand(7));
        Value *Args[] = {Src0, BTI, Src2, Src3, Src4, Src5, Src6, Src7};
        auto RetTy = B->GetVectorType(CI->getType());
        Type *Tys[] = {RetTy, Src0->getType(), Src4->getType()};
        auto Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(replacement)->setDebugLoc(CI->getDebugLoc());
        return replacement;
      } break;
      // default llvm-intrinsic packetizing rule should work for svm atomics
      default:
        break;
      }
    }
  }
  return nullptr;
}

/// - map old instruction to new in case we revisit the old instruction
Value *GenXPacketize::packetizeInstruction(Instruction *pInst) {
  // determine instruction type and call its packetizer
  Value *pResult = packetizeGenXIntrinsic(pInst);
  if (!pResult) {
    if (IsLLVMIntrinsic(pInst))
      pResult = packetizeLLVMIntrinsic(pInst);
    else
      pResult = packetizeLLVMInstruction(pInst);
  }

  if (pResult) {
    if (pInst->getName() != "") {
      pResult->setName(pInst->getName());
    }
    // When the resulting instruction has the same type
    // Debug values can be preserved
    if (pResult->getType() == pInst->getType()) {
      SmallVector<DbgVariableIntrinsic *, 1> DbgUsers;
      llvm::findDbgUsers(DbgUsers, pInst);
      for (auto *DII : DbgUsers) {
#if LLVM_VERSION_MAJOR >= 13
        DII->replaceVariableLocationOp(pInst, pResult);
#else
        DII->setOperand(0, llvm::MetadataAsValue::get(DII->getContext(),
                                                      llvm::ValueAsMetadata::get(pResult)));
#endif
      }
    }

    // Copy any metadata to new instruction
    if (pResult != pInst && isa<Instruction>(pResult)) {
      cast<Instruction>(pResult)->copyMetadata(*pInst);
    }
  }

  return pResult;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Replace all uses but avoid any type checking as instructions
///        maybe in a partial bad state.
/// @param pInst - old instruction we're replacing.
/// @param pNewInst - new instruction
void GenXPacketize::replaceAllUsesNoTypeCheck(Value *pInst, Value *pNewInst) {
  SmallVector<User *, 8> users;
  SmallVector<uint32_t, 8> opNum;

  for (auto &U : pInst->uses()) {
    users.push_back(U.getUser());
    opNum.push_back(U.getOperandNo());
  }
  for (uint32_t i = 0; i < users.size(); ++i) {
    users[i]->setOperand(opNum[i], pNewInst);
  }
}

//////////////////////////////////////////////////////////////////////////
/// @brief Remove replaced instructions. DCE will not remove calls, etc.
///        So we have to remove these manually.
void GenXPacketize::removeDeadInstructions(Function &F) {
  SmallVector<Instruction *, 8> unused;
  for (auto RMI : ReplaceMap) {
    if (RMI.first != RMI.second) {
      if (Instruction *UnusedInst =
              (Instruction *)dyn_cast<Instruction>(RMI.first)) {
        unused.push_back(UnusedInst);
      }
    }
  }
  for (auto UnusedInst : unused) {
    UnusedInst->replaceAllUsesWith(UndefValue::get(UnusedInst->getType()));
    UnusedInst->eraseFromParent();
  }
}

//////////////////////////////////////////////////////////////////////////
/// @brief LLVM optimizes certain operations and replaces with general C
/// functions instead
///        of llvm intrinsics (sqrtf() instead of llvm.sqrt() for example). We
///        convert these back to known llvm intrinsics before packetization,
///        which are handled natively
/// @param F - function to analyze
void GenXPacketize::fixupLLVMIntrinsics(Function &F) {
  std::unordered_set<Instruction *> removeSet;

  for (auto &BB : F.getBasicBlockList()) {
    for (auto &I : BB.getInstList()) {
      if (isa<CallInst>(I)) {
        CallInst *pCallInst = cast<CallInst>(&I);
        Function *pFunc = pCallInst->getCalledFunction();
        if (pFunc) {
          if (pFunc->getName().startswith("sqrt")) {
            B->IRB()->SetInsertPoint(&I);
            Value *pSqrt = B->VSQRTPS(pCallInst->getOperand(0));
            pCallInst->replaceAllUsesWith(pSqrt);
            removeSet.insert(pCallInst);
          } else if (pFunc->getName().startswith("fabs")) {
            B->IRB()->SetInsertPoint(&I);
            Value *pFabs = B->FABS(pCallInst->getOperand(0));
            pCallInst->replaceAllUsesWith(pFabs);
            removeSet.insert(pCallInst);
          } else if (pFunc->getName().startswith("exp2")) {
            B->IRB()->SetInsertPoint(&I);
            Value *pExp2 = B->EXP2(pCallInst->getOperand(0));
            pCallInst->replaceAllUsesWith(pExp2);
            removeSet.insert(pCallInst);
          } else if (pFunc->getName().equals("ldexpf")) {
            B->IRB()->SetInsertPoint(&I);
            Value *pArg = pCallInst->getOperand(0);
            Value *pExp = pCallInst->getOperand(1);

            // replace ldexp with arg * 2^exp = arg * (2 << arg)
            Value *pShift = B->SHL(B->C(1), pExp);
            pShift = B->UI_TO_FP(pShift, B->mFP32Ty);
            Value *pResult = B->FMUL(pArg, pShift);
            pCallInst->replaceAllUsesWith(pResult);
            removeSet.insert(pCallInst);
          }
        }
      }
    }
  }

  for (auto *pInst : removeSet) {
    pInst->eraseFromParent();
  }
}

//////////////////////////////////////////////////////////////////////////
/// @brief find the global ExecMask variable if exists in order to lower
/// CM SIMD control-flow representation after packetization
GlobalVariable *GenXPacketize::findGlobalExecMask() {
  // look for the global EMask variable if exists
  for (auto &Global : M->getGlobalList()) {
    auto Ty = IGCLLVM::getNonOpaquePtrEltTy(Global.getType());
    if (Ty->isVectorTy() &&
        cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() ==
            CMSimdCFLower::MAX_SIMD_CF_WIDTH) {
      auto ElemTy = cast<VectorType>(Ty)->getElementType();
      if (ElemTy->isIntegerTy() && ElemTy->getIntegerBitWidth() == 1) {
        // so far the type is right, then check the use
        for (auto EMUI = Global.use_begin(), EMUE = Global.use_end();
             EMUI != EMUE; ++EMUI) {
          if (auto LD = dyn_cast<LoadInst>(EMUI->getUser())) {
            for (auto UI = LD->user_begin(), E = LD->user_end(); UI != E;
                 ++UI) {
              const Value *LocalUse = (*UI);
              if (auto CI = dyn_cast_or_null<CallInst>(LocalUse)) {
                if (Function *Callee = CI->getCalledFunction()) {
                  if (GenXIntrinsic::getGenXIntrinsicID(Callee) ==
                      GenXIntrinsic::genx_simdcf_goto)
                    return &Global;
                }
              }
            }
          }
        }
      }
    }
  }
  return nullptr;
}
//////////////////////////////////////////////////////////////////////////
/// @brief lower CM SIMD control-flow representation after packetization
///
void GenXPacketize::lowerControlFlowAfter(std::vector<Function *> &SIMTFuncs) {
  auto EMVar = findGlobalExecMask();
  // create one if we cannot find one.
  if (!EMVar) {
    auto EMTy = IGCLLVM::FixedVectorType::get(Type::getInt1Ty(M->getContext()),
                                              CMSimdCFLower::MAX_SIMD_CF_WIDTH);
    EMVar = new GlobalVariable(*M, EMTy, false /*isConstant*/,
                               GlobalValue::InternalLinkage,
                               Constant::getAllOnesValue(EMTy), "EM");
  }
  CMSimdCFLower CFL(EMVar);
  // Derive an order to process functions such that a function is visited
  // after anything that calls it.
  int n = SIMTFuncs.size();
  for (int i = n - 1; i >= 0; --i)
    CFL.processFunction(SIMTFuncs[i]);
}

// foward declare the initializer
void initializeGenXPacketizePass(PassRegistry &);

} // namespace llvm

using namespace llvm;

char GenXPacketize::ID = 0;
INITIALIZE_PASS_BEGIN(GenXPacketize, "GenXPacketize", "GenXPacketize", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(BreakCriticalEdges)
INITIALIZE_PASS_END(GenXPacketize, "GenXPacketize", "GenXPacketize", false,
                    false)

namespace llvm {
ModulePass *createGenXPacketizePass() {
  initializeGenXPacketizePass(*PassRegistry::getPassRegistry());
  return new GenXPacketize();
}
} // namespace llvm
