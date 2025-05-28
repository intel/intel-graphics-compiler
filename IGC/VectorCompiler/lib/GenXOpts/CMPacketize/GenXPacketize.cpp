/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2025 Intel Corporation

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

#include <llvmWrapper/IR/BasicBlock.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Region.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXSimdCFLowering.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"

#include "Probe/Assertion.h"
#include <algorithm>
#include <set>
#include <stack>
#include <unordered_set>

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
  GenXPacketize(const GenXPacketize &) = delete;
  GenXPacketize &operator=(const GenXPacketize &) = delete;

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
  Function *getVectorIntrinsic(Module *M, unsigned ID, ArrayRef<Type *> ArgTy);
  Value *packetizeConstant(Constant *C);
  Value *packetizeGenXIntrinsic(Instruction *Inst);
  Value *packetizeLLVMIntrinsic(Instruction *Inst);
  Value *packetizeLLVMInstruction(Instruction *Inst);
  Value *packetizeInstruction(Instruction *Inst);

  void removeDeadInstructions();
  void fixupLLVMIntrinsics(Function &F);

  Function *vectorizeSIMTFunction(Function *F, unsigned Width);
  bool vectorizeSIMTEntry(Function &F);

  bool isUniformIntrinsic(unsigned ID);
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
static bool GenXUnifyReturnBlocks(Function &F) {
  std::vector<BasicBlock *> ReturningBlocks;
  for (auto &BB : F)
    if (isa<ReturnInst>(BB.getTerminator()))
      ReturningBlocks.push_back(&BB);
  if (ReturningBlocks.size() <= 1)
    return false;
  // Insert a new basic block into the function, add PHI nodes (if the function
  // returns values), and convert all of the return instructions into
  // unconditional branches.
  auto *NewRetBlock =
      BasicBlock::Create(F.getContext(), "UnifiedReturnBlock", &F);
  PHINode *PN = nullptr;
  if (F.getReturnType()->isVoidTy()) {
    ReturnInst::Create(F.getContext(), nullptr, NewRetBlock);
  } else {
    // If the function doesn't return void... add a PHI node to the block...
    PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(),
                         "UnifiedRetVal", NewRetBlock);
    ReturnInst::Create(F.getContext(), PN, NewRetBlock);
  }
  // Loop over all of the blocks, replacing the return instruction with an
  // unconditional branch.
  for (auto *BB : ReturningBlocks) {
    // Add an incoming element to the PHI node for every return instruction that
    // is merging into this new block...
    if (PN)
      PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

    IGCLLVM::popBackInstruction(BB); // Remove the return insn

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
  for (unsigned Idx = 0; Idx < NumFunc; ++Idx) {
    auto F = FuncOrder[Idx];
    findUniformArgs(*F);
  }
  UniformInsts.clear();
  std::vector<Function *> SIMTFuncs;
  // Process those functions called in the SIMT mode
  for (int Idx = NumFunc - 1; Idx >= 0; --Idx) {
    auto F = FuncOrder[Idx];
    auto It = FuncVectors.find(F);
    if (It != FuncVectors.end()) {
      auto WV = It->second;
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
  for (auto *F : ForkFuncs) {
    Modified |= vectorizeSIMTEntry(*F);
    SIMTFuncs.push_back(&(*F));
  }
  delete B;
  // perform reg-to-mem in order to generate simd-control-flow without phi
  // we then perform mem-to-reg after generating simd-control-flow.
  std::unique_ptr<FunctionPass> DemotePass(createDemoteRegisterToMemoryPass());
  for (auto *F : SIMTFuncs) {
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
  B->setTargetWidth(Width);
  // vectorize the argument and return types
  std::vector<Type *> ArgTypes;
  for (const Argument &Arg : F->args()) {
    auto *ArgTy = Arg.getType();
    if (UniformArgs.count(&Arg) || ArgTy->isOpaquePointerTy())
      ArgTypes.push_back(ArgTy);
    else if (ArgTy->isPointerTy()) {
      // FIXME: check the pointer defined by an argument or an alloca
      // [N x float]* should packetize to [N x <8 x float>]*
      auto *VTy = PointerType::get(
          B->getVectorType(IGCLLVM::getNonOpaquePtrEltTy(ArgTy)),
          ArgTy->getPointerAddressSpace());
      ArgTypes.push_back(VTy);
    } else {
      ArgTypes.push_back(B->getVectorType(ArgTy));
    }
  }
  auto *RetTy = B->getVectorType(F->getReturnType());
  // Create a new function type...
  IGC_ASSERT(!F->isVarArg());
  auto *FTy = FunctionType::get(RetTy, ArgTypes, false);
  // Create the vector function prototype
  StringRef VecFName = F->getName();
  const char *Suffix[] = {".vec00", ".vec08", ".vec16", ".vec24", ".vec32"};
  auto *ClonedFunc =
      Function::Create(FTy, GlobalValue::InternalLinkage,
                       VecFName + Suffix[Width / 8], F->getParent());
  ClonedFunc->setCallingConv(F->getCallingConv());
  ClonedFunc->setAttributes(F->getAttributes());
  if (F->getAlignment() > 0)
    ClonedFunc->setAlignment(IGCLLVM::getAlign(*F));
  // then use CloneFunctionInto
  ValueToValueMapTy ArgMap;
  auto ArgI = ClonedFunc->arg_begin();
  for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
    ArgI->setName(I->getName()); // Copy the name over...
    ArgMap[I] = ArgI;            // Add mapping to ValueMap
    if (UniformArgs.count(I)) {  // bookkeep the uniform set
      UniformArgs.insert(ArgI);
    }
    ArgI++;
  }
  SmallVector<ReturnInst *, 10> Returns;
  ClonedCodeInfo CloneInfo;
  IGCLLVM::CloneFunctionInto(ClonedFunc, F, ArgMap,
                             IGCLLVM::CloneFunctionChangeType::GlobalChanges,
                             Returns, Suffix[Width / 8], &CloneInfo);
  ReplaceMap.clear();
  // find uniform instructions related to uniform arguments
  findUniformInsts(*ClonedFunc);
  // vectorize instructions in the fork-regions
  std::vector<PHINode *> PhiRound;
  DominatorTree DT(*ClonedFunc);
  for (auto DI = df_begin(DT.getRootNode()), DE = df_end(DT.getRootNode());
       DI != DE; ++DI) {
    auto *BB = DI->getBlock();
    for (auto &I : *BB) {
      if (!UniformInsts.count(&I)) {
        Value *PacketizedInst = packetizeInstruction(&I);
        ReplaceMap[&I] = PacketizedInst;
      } else {
        for (int Idx = 0, N = I.getNumOperands(); Idx < N; ++Idx) {
          auto *OrigValue = I.getOperand(Idx);
          auto It = ReplaceMap.find(OrigValue);
          if (It != ReplaceMap.end() && It->second != OrigValue)
            I.setOperand(Idx, It->second);
        }
      }
      if (isa<PHINode>(&I))
        PhiRound.push_back(dyn_cast<PHINode>(&I));
    }
  }
  for (auto Phi : PhiRound) {
    for (int Idx = 0, N = Phi->getNumOperands(); Idx < N; ++Idx) {
      auto *OrigValue = Phi->getOperand(Idx);
      auto It = ReplaceMap.find(OrigValue);
      if (It != ReplaceMap.end() && It->second != OrigValue)
        Phi->setOperand(Idx, It->second);
    }
  }
  removeDeadInstructions();
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
  B->setTargetWidth(Width);
  ReplaceMap.clear();
  B->IRB->SetInsertPoint(&F.getEntryBlock(), F.getEntryBlock().begin());
  // vectorize instructions in the fork-regions
  std::vector<PHINode *> PhiRound;
  DominatorTree DT(F);
  for (auto DI = df_begin(DT.getRootNode()), DE = df_end(DT.getRootNode());
       DI != DE; ++DI) {
    auto *BB = DI->getBlock();
    for (auto &I : *BB) {
      if (!UniformInsts.count(&I)) {
        auto *PacketizedInst = packetizeInstruction(&I);
        ReplaceMap[&I] = PacketizedInst;
      } else {
        for (int Idx = 0, N = I.getNumOperands(); Idx < N; ++Idx) {
          auto *OrigValue = I.getOperand(Idx);
          auto It = ReplaceMap.find(OrigValue);
          if (It != ReplaceMap.end() && It->second != OrigValue)
            I.setOperand(Idx, It->second);
        }
      }
      if (auto *Phi = dyn_cast<PHINode>(&I))
        PhiRound.push_back(Phi);
    }
  }
  for (auto Phi : PhiRound) {
    for (int Idx = 0, N = Phi->getNumOperands(); Idx < N; ++Idx) {
      auto *OrigValue = Phi->getOperand(Idx);
      auto It = ReplaceMap.find(OrigValue);
      if (It != ReplaceMap.end() && It->second != OrigValue) {
        Phi->setOperand(Idx, It->second);
      }
    }
  }
  removeDeadInstructions();
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
  for (auto MI = M->begin(), ME = M->end(); MI != ME; ++MI) {
    Function *F = &*MI;
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
    for (auto UI = F->use_begin(), UE = F->use_end(); UI != UE; ++UI) {
      if (auto *CI = dyn_cast<CallInst>(UI->getUser())) {
        auto *Caller = CI->getFunction();
        auto *CallerNode = &CallGraph[Caller];
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
          uint32_t Width = 0;
          Caller->getFnAttribute("CMGenxSIMT")
              .getValueAsString()
              .getAsInteger(0, Width);
          if (Width > 1) {
            auto CalleeVectorIter = FuncVectors.find(F);
            if (CalleeVectorIter != FuncVectors.end())
              CalleeVectorIter->second.insert(Width);
            else {
              std::set<unsigned> WidthSet;
              WidthSet.insert(Width);
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
  for (unsigned Idx = 0; Idx != FuncOrder.size(); ++Idx) {
    CGNode *CGN = &CallGraph[FuncOrder[Idx]];
    for (auto CI = CGN->Callees.begin(), CE = CGN->Callees.end(); CI != CE;
         ++CI) {
      CGNode *Callee = *CI;
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
  auto It = FuncVectors.find(&F);
  if (It == FuncVectors.end()) {
    // non-simt function or simt-entry function
    for (const Argument &Arg : F.args())
      UniformArgs.insert(&Arg);
  } else {
    // simt functions that needs whole-function vectorization
    for (const Argument &Arg : F.args()) {
      bool IsUniform = true;
      // check every call-site
      for (auto *U : F.users()) {
        if (auto *CI = dyn_cast<CallInst>(U)) {
          auto *Def = CI->getArgOperand(Arg.getArgNo());
          if (auto *DA = dyn_cast<Argument>(Def)) {
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
        UniformArgs.insert(&Arg);
    }
  }
}

bool GenXPacketize::isUniformIntrinsic(unsigned ID) {
  switch (ID) {
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
      if (auto *LD = dyn_cast<LoadInst>(UI->getUser())) {
        UniformInsts.insert(LD);
      }
    }
  }
  // some intrinsics are always uniform
  for (auto &FD : M->getFunctionList()) {
    if (FD.isDeclaration()) {
      if (isUniformIntrinsic(GenXIntrinsic::getGenXIntrinsicID(&FD))) {
        for (auto UI = FD.use_begin(), UE = FD.use_end(); UI != UE; ++UI) {
          if (auto *Inst = dyn_cast<Instruction>(UI->getUser())) {
            UniformInsts.insert(Inst);
          }
        }
      }
    }
  }
  std::set<const Value *> ArgDefs;
  std::stack<Value *> UVSet;
  for (const Argument &I : F.args()) {
    if (!UniformArgs.count(&I))
      continue;
    ArgDefs.insert(&I);
    for (auto UI = I.user_begin(), E = I.user_end(); UI != E; ++UI) {
      const Value *Use = (*UI);
      if (auto *CastI = dyn_cast<CastInst>(Use)) {
        ArgDefs.insert(CastI);
        UniformInsts.insert(CastI);
        UVSet.push((Value *)CastI);
      }
    }
  }
  // first find out all the uniform alloca to store those uniform arguments
  for (auto Def : ArgDefs) {
    for (auto UI = Def->user_begin(), E = Def->user_end(); UI != E; ++UI) {
      const Value *Use = (*UI);
      if (auto *LI = dyn_cast<LoadInst>(Use)) {
        UniformInsts.insert(LI);
      } else if (auto *GEP = dyn_cast<GetElementPtrInst>(Use)) {
        if (GEP->getPointerOperand() == Def) {
          UniformInsts.insert(GEP);
          UVSet.push((Value *)GEP);
        }
      } else if (auto *SI = dyn_cast<StoreInst>(Use)) {
        if (SI->getPointerOperand() == Def)
          UniformInsts.insert(SI);
        else {
          auto *PI = SI->getPointerOperand();
          if (auto *AI = dyn_cast<AllocaInst>(PI)) {
            UniformInsts.insert(AI);
            UVSet.push((Value *)AI);
          }
        }
      } else if (auto *Cast = dyn_cast<BitCastInst>(Use)) {
        UniformInsts.insert(Cast);
        UVSet.push((Value *)Cast);
      } else if (auto *CI = dyn_cast<CallInst>(Use)) {
        if (Function *Callee = CI->getCalledFunction()) {
          if (GenXIntrinsic::isVLoadStore(Callee)) {
            UniformInsts.insert(CI);
          }
        }
      }
    }
  }
  // then find the uniform loads and stores in fork-region
  while (!UVSet.empty()) {
    Value *Def = UVSet.top();
    UVSet.pop();
    for (auto UI = Def->user_begin(), E = Def->user_end(); UI != E; ++UI) {
      Value *Use = (*UI);
      if (auto *UseI = dyn_cast<Instruction>(Use)) {
        if (isa<StoreInst>(UseI)) {
          UniformInsts.insert(UseI);
        } else if (auto *LI = dyn_cast<LoadInst>(UseI)) {
          UniformInsts.insert(UseI);
          if (LI->getType()->isPointerTy())
            UVSet.push(UseI);
        } else if (auto *GEP = dyn_cast<GetElementPtrInst>(UseI)) {
          if (GEP->hasAllConstantIndices()) {
            UVSet.push(UseI);
            UniformInsts.insert(UseI);
          }
        } else if (auto *Cast = dyn_cast<BitCastInst>(UseI)) {
          UVSet.push(UseI);
          UniformInsts.insert(UseI);
        }
      }
    }
  }
  return;
}

Value *GenXPacketize::getPacketizeValue(Value *OrigValue) {
  auto It = ReplaceMap.find(OrigValue);
  if (It != ReplaceMap.end()) {
    return It->second;
  } else if (auto *C = dyn_cast<Constant>(OrigValue)) {
    return packetizeConstant(C);
  } else if (auto *A = dyn_cast<Argument>(OrigValue)) {
    if (UniformArgs.count(A))
      return B->VBROADCAST(OrigValue, OrigValue->getName());
    // otherwise the argument should have been in the right vector form
    ReplaceMap[OrigValue] = OrigValue;
    return OrigValue;
  } else if (auto *Inst = dyn_cast<Instruction>(OrigValue)) {
    // need special handling for alloca
    if (auto *AI = dyn_cast<AllocaInst>(OrigValue)) {
      // this is not a uniform alloca
      if (!UniformInsts.count(Inst)) {
        auto *VecType = B->getVectorType(AI->getAllocatedType());
        auto *V = B->ALLOCA(VecType, nullptr, AI->getName());
        V->removeFromParent();
        V->insertBefore(Inst);
        ReplaceMap[OrigValue] = V;
        return V;
      }
      ReplaceMap[OrigValue] = OrigValue;
      return OrigValue;
    } else if (UniformInsts.count(Inst)) {
      auto *V = B->VBROADCAST(OrigValue);
      return V;
    }
  }
  return nullptr;
}

// this is used on operands that are expected to be uniform
Value *GenXPacketize::getUniformValue(Value *OrigValue) {
  if (auto *G = dyn_cast<GlobalValue>(OrigValue))
    return G;
  if (auto *C = dyn_cast<Constant>(OrigValue))
    return C;
  if (auto *A = dyn_cast<Argument>(OrigValue)) {
    if (UniformArgs.count(A)) {
      return A;
    }
  }
  if (auto *A = dyn_cast<Instruction>(OrigValue)) {
    if (UniformInsts.count(A)) {
      return A;
    }
  }
  auto *VV = getPacketizeValue(OrigValue);
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
static bool IsLLVMIntrinsic(Instruction *Inst) {
  if (isa<CallInst>(Inst)) {
    auto *CI = cast<CallInst>(Inst);
    auto *F = CI->getCalledFunction();
    IGC_ASSERT(F);
    return F->isIntrinsic();
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Packetize a scalar constant
Value *GenXPacketize::packetizeConstant(Constant *C) {
  if (isa<UndefValue>(C)) {
    return UndefValue::get(B->getVectorType(C->getType()));
  } else {
    return B->VBROADCAST(C);
  }
}

// Helper: vectorize scalar MD_range metadata for a vector instruction
static void vectorizeRangeMetadata(Instruction *OldCI, Instruction *NewCI) {
  auto *OldRangeMD = OldCI->getMetadata(LLVMContext::MD_range);
  if (!OldRangeMD)
    return;
  Type *Ty = NewCI->getType();

  // Currently, llvm15/16 work inconsistently with MD_range metadata for vectors.
  // Link1: Verifier.cpp Verifier::visitRangeMetadata
  //     -> "Range types must match instruction type!"
  // Link2: ValueTracking.cpp llvm::computeKnownBitsFromRangeMetadata
  //     -> ConstantInt *Lower = ... Ranges.getOperand(2 * i + 0)
  if (auto *VecTy = dyn_cast<VectorType>(Ty))
      NewCI->setMetadata(LLVMContext::MD_range, nullptr);
  return;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Packetize an LLVM intrinsic.  Generally this means replacing
///        a scalar intrinsic function call with a vectored equivalent.
Value *GenXPacketize::packetizeLLVMIntrinsic(Instruction *Inst) {
  B->IRB->SetInsertPoint(Inst);
  auto *CI = cast<CallInst>(Inst);
  auto *F = CI->getCalledFunction();
  IGC_ASSERT(F);
  IGC_ASSERT(F->isIntrinsic());
  // not sure how to handle debug intrinsics, just return
  if (isa<DbgInfoIntrinsic>(Inst))
    return Inst;
  auto ID = GenXIntrinsic::getAnyIntrinsicID(F);
  // packetize intrinsic operands
  std::vector<Type *> VectorArgTys;
  std::vector<Value *> Args;
  std::vector<Value *> PacketizedArgs;
  for (auto &Op : CI->args()) {
    auto *Arg = Op.get();
    Args.push_back(Arg);
    auto *VV = getPacketizeValue(Arg);
    PacketizedArgs.push_back(VV);
    VectorArgTys.push_back(VV->getType());
  }
  // override certain intrinsics
  switch (ID) {
  default:
    break;
  case Intrinsic::log2:
    if (!Inst->hasApproxFunc())
      return B->VLOG2PS(PacketizedArgs[0]);
    break;
  case Intrinsic::exp2:
    if (!Inst->hasApproxFunc())
      return B->VEXP2PS(PacketizedArgs[0]);
    break;
  }
  auto *NewF = getVectorIntrinsic(B->M, ID, VectorArgTys);
  // Some arguments are not overloadable and mush stay scalar
  std::vector<Value *> NewArgs;
  auto *NewFTy = NewF->getFunctionType();
  for (unsigned Idx = 0; Idx < NewFTy->getNumParams(); Idx++)
    NewArgs.push_back(NewFTy->getParamType(Idx) == VectorArgTys[Idx]
                          ? PacketizedArgs[Idx]
                          : Args[Idx]);
  auto *NewCI = CallInst::Create(NewF, NewArgs, "", CI);
  return NewCI;
}

Value *GenXPacketize::packetizeLLVMInstruction(Instruction *Inst) {
  Value *ReplacedInst = nullptr;
  B->IRB->SetInsertPoint(Inst);
  // packetize a call
  if (auto *CI = dyn_cast<CallInst>(Inst)) {
    auto *F = CI->getCalledFunction();
    auto FMI = FuncMap.find(std::pair<Function *, unsigned>(F, B->VWidth));
    if (FMI != FuncMap.end()) {
      std::vector<Value *> ArgOps;
      auto VF = FMI->second;
      for (auto &Arg : VF->args()) {
        auto Idx = Arg.getArgNo();
        if (UniformArgs.count(&Arg))
          ArgOps.push_back(getUniformValue(CI->getArgOperand(Idx)));
        else
          ArgOps.push_back(getPacketizeValue(CI->getArgOperand(Idx)));
      }
      ReplacedInst = CallInst::Create(VF, ArgOps, CI->getName(), CI);
      return ReplacedInst;
    } else
      IGC_ASSERT(0);
  }
  uint32_t Opcode = Inst->getOpcode();
  switch (Opcode) {
  case Instruction::AddrSpaceCast:
  case Instruction::BitCast: {
    // packetize the bitcast source
    auto *PacketizedSrc = getPacketizeValue(Inst->getOperand(0));
    auto *PacketizedSrcTy = PacketizedSrc->getType();
    // packetize dst type
    Type *ReturnTy;
    if (Inst->getType()->isPointerTy()) {
      // two types of pointers, <N x Ty>* or <N x Ty*>
      if (PacketizedSrc->getType()->isVectorTy()) {
        // <N x Ty*>
        uint32_t numElems =
            cast<IGCLLVM::FixedVectorType>(PacketizedSrcTy)->getNumElements();
        ReturnTy = IGCLLVM::FixedVectorType::get(Inst->getType(), numElems);
      } else if (!Inst->getType()->isOpaquePointerTy()) {
        // <N x Ty>*
        auto *DstScalarTy = IGCLLVM::getNonOpaquePtrEltTy(Inst->getType());
        if (VectorType::isValidElementType(DstScalarTy))
          // Map <N x OldTy>* to <N x NewTy>*
          ReturnTy =
              PointerType::get(B->getVectorType(DstScalarTy),
                               Inst->getType()->getPointerAddressSpace());
        else {
          // Map <N x OldTy>* to <N x NewTy*> using cast then GEP
          auto *TmpTy = llvm::ArrayType::get(DstScalarTy, B->VWidth);
          auto *TmpPtrTy = PointerType::get(
              TmpTy, Inst->getType()->getPointerAddressSpace());
          auto *TmpInst =
              B->CAST((Instruction::CastOps)Opcode, PacketizedSrc, TmpPtrTy);
          SmallVector<Value *, 2> VecIndices;
          VecIndices.push_back(B->C(0));
          VecIndices.push_back(B->CInc<uint32_t>(0, B->VWidth));
          ReplacedInst = B->GEPA(TmpTy, TmpInst, VecIndices);
          break;
        }
      } else {
        ReturnTy = Inst->getType();
      }
    } else {
      ReturnTy = B->getVectorType(Inst->getType());
    }
    ReplacedInst =
        B->CAST((Instruction::CastOps)Opcode, PacketizedSrc, ReturnTy);
    break;
  }
  case Instruction::GetElementPtr: {
    auto *GepInst = cast<GetElementPtrInst>(Inst);
    auto *VecSrc = GepInst->getPointerOperand();
    auto *VecSrcTy = GepInst->getSourceElementType();
    if (!isa<GlobalValue>(VecSrc) && !isa<Argument>(VecSrc) &&
        !(isa<Instruction>(VecSrc) &&
          UniformInsts.count(cast<Instruction>(VecSrc))))
      VecSrc = getPacketizeValue(VecSrc);
    if (!isa<AllocaInst>(VecSrc)) {
      // just packetize the GEP to a vector GEP.
      SmallVector<Value *, 8> VecIndices;
      for (uint32_t Idx = 0; Idx < GepInst->getNumIndices(); ++Idx)
        VecIndices.push_back(getPacketizeValue(GepInst->getOperand(1 + Idx)));
      ReplacedInst = B->GEPA(VecSrcTy, VecSrc, VecIndices);
    } else {
      if (GepInst->hasAllConstantIndices()) {
        // SOA GEP with scalar src and constant indices, result will be <N x
        // Ty>* Ex. gep [4 x <8 x float>]*, 0, 0 --> <8 x float>*
        SmallVector<Value *, 8> VecIndices;
        for (uint32_t Idx = 0; Idx < GepInst->getNumIndices(); ++Idx)
          VecIndices.push_back(GepInst->getOperand(1 + Idx));
        ReplacedInst = B->GEPA(VecSrcTy, VecSrc, VecIndices);
      } else {
        //// SOA GEP with non-uniform indices. Need to vector GEP to each SIMD
        /// lane.
        /// Result will be <N x Ty*>
        SmallVector<Value *, 8> VecIndices;
        for (uint32_t Idx = 0; Idx < GepInst->getNumIndices(); ++Idx)
          VecIndices.push_back(getPacketizeValue(GepInst->getOperand(1 + Idx)));
        // Step to the SIMD lane
        VecIndices.push_back(B->CInc<uint32_t>(0, B->VWidth));
        ReplacedInst = B->GEPA(VecSrcTy, VecSrc, VecIndices);
      }
    }
    break;
  }
  case Instruction::Load: {
    auto *LI = cast<LoadInst>(Inst);
    auto *VecSrc = getPacketizeValue(LI->getPointerOperand());
    auto *VecSrcTy = B->getVectorType(LI->getType());
    if (VecSrc->getType()->isVectorTy()) {
      IGC_ASSERT(
          cast<VectorType>(VecSrc->getType())->getElementType()->isPointerTy());
      auto Align = IGCLLVM::getAlignmentValue(LI);
      ReplacedInst = B->MASKED_GATHER(VecSrcTy, VecSrc, Align);
    } else
      ReplacedInst = B->ALIGNED_LOAD(VecSrcTy, VecSrc, IGCLLVM::getAlign(*LI));
    break;
  }
  case Instruction::Store: {
    auto *SI = cast<StoreInst>(Inst);
    auto *VecDstPtrs = getPacketizeValue(SI->getPointerOperand());
    auto *VecSrc = getPacketizeValue(SI->getOperand(0));
    if (VecDstPtrs->getType()->isVectorTy()) {
      IGC_ASSERT(cast<VectorType>(VecDstPtrs->getType())
                     ->getElementType()
                     ->isPointerTy());
      auto Align = IGCLLVM::getAlignmentValue(cast<StoreInst>(Inst));
      ReplacedInst = B->MASKED_SCATTER(VecSrc, VecDstPtrs, Align);
    } else {
      ReplacedInst = B->STORE(VecSrc, VecDstPtrs);
    }
    break;
  }

  case Instruction::ExtractElement: {
    auto *OldVec = Inst->getOperand(0);
    auto *Vec = getPacketizeValue(OldVec);
    auto *Idx = Inst->getOperand(1);
    auto N =
        cast<IGCLLVM::FixedVectorType>(OldVec->getType())->getNumElements();
    auto *ElemTy = Inst->getType();
    auto *VecDstTy = IGCLLVM::FixedVectorType::get(ElemTy, B->VWidth);
    // create an read-region
    vc::CMRegion R(VecDstTy, DL);
    if (auto *CI = dyn_cast<ConstantInt>(Idx)) {
      R.Offset = CI->getSExtValue() * ElemTy->getPrimitiveSizeInBits() / 8;
      R.Indirect = nullptr;
    } else {
      R.Offset = 0;
      auto *MulCTy = IntegerType::getInt16Ty(M->getContext());
      auto *MulC =
          ConstantInt::get(MulCTy, ElemTy->getPrimitiveSizeInBits() / 8);
      auto NBits = Idx->getType()->getIntegerBitWidth();
      if (NBits > 16)
        Idx = B->TRUNC(Idx, MulCTy);
      else if (NBits < 16)
        Idx = B->S_EXT(Idx, MulCTy);
      R.Indirect = B->MUL(Idx, MulC);
    }
    R.NumElements = B->VWidth;
    R.Width = B->VWidth;
    R.Stride = N;
    R.VStride = 0;
    ReplacedInst = R.createRdRegion(Vec, Inst->getName(), Inst /*InsertBefore*/,
                                    Inst->getDebugLoc(), true /*AllowScalar*/);
    break;
  }
  case Instruction::InsertElement: {
    auto *OldVec = Inst->getOperand(0);
    auto *Vec = getPacketizeValue(OldVec);
    auto *ElemVec = getPacketizeValue(Inst->getOperand(1));
    auto *Idx = Inst->getOperand(2);
    auto N =
        cast<IGCLLVM::FixedVectorType>(OldVec->getType())->getNumElements();
    auto *ElemTy = Inst->getOperand(1)->getType();
    // create an write-region
    vc::CMRegion R(Vec->getType(), DL);
    if (auto *CI = dyn_cast<ConstantInt>(Idx)) {
      // special case, this is really just like a bitcast
      if (CI->getZExtValue() == 0 && N == 1 && isa<UndefValue>(OldVec)) {
        auto *RetTy = B->getVectorType(Inst->getType());
        ReplacedInst = B->BITCAST(ElemVec, RetTy);
        break;
      }
      R.Offset = CI->getSExtValue() * ElemTy->getPrimitiveSizeInBits() / 8;
      R.Indirect = nullptr;
    } else {
      R.Offset = 0;
      auto *MulCTy = IntegerType::getInt16Ty(M->getContext());
      auto *MulC =
          ConstantInt::get(MulCTy, ElemTy->getPrimitiveSizeInBits() / 8);
      auto NBits = Idx->getType()->getIntegerBitWidth();
      if (NBits > 16)
        Idx = B->TRUNC(Idx, MulCTy);
      else if (NBits < 16)
        Idx = B->S_EXT(Idx, MulCTy);
      R.Indirect = B->MUL(Idx, MulC);
    }
    R.NumElements = B->VWidth;
    R.Width = B->VWidth;
    R.Stride = N;
    R.VStride = 0;
    ReplacedInst = R.createWrRegion(Vec, ElemVec, Inst->getName(),
                                    Inst /*InsertBefore*/, Inst->getDebugLoc());
    break;
  }
  case Instruction::Br: {
    // any conditional branches with vectored conditions need to preceded with
    // a genx_simdcf_any to ensure we branch iff all lanes are set
    auto *Branch = cast<BranchInst>(Inst);
    if (Branch->isConditional()) {
      auto *Condition = getPacketizeValue(Branch->getCondition());
      auto *NewFn = GenXIntrinsic::getGenXDeclaration(
          B->M, GenXIntrinsic::genx_simdcf_any, Condition->getType());
      auto *NewTest = CallInst::Create(NewFn, Condition, "", Inst);
      NewTest->setName("exit.cond.mask.test");
      Branch->setCondition(NewTest);
    }
    ReplacedInst = Branch;
    break;
  }
  case Instruction::Alloca: {
    auto *AI = cast<AllocaInst>(Inst);
    auto *VecTy = B->getVectorType(AI->getAllocatedType());
    auto *Return = B->ALLOCA(VecTy, nullptr, Inst->getName());
    ReplacedInst = Return;
    break;
  }
  case Instruction::ShuffleVector: {
    auto *Src1 = Inst->getOperand(0);
    auto *Src2 = Inst->getOperand(1);
    auto *Mask =
        IGCLLVM::getShuffleMaskForBitcode(cast<ShuffleVectorInst>(Inst));
    if (cast<IGCLLVM::FixedVectorType>(Src1->getType())->getNumElements() ==
            1 &&
        cast<IGCLLVM::FixedVectorType>(Mask->getType())->getNumElements() ==
            1) {
      if (cast<Constant>(Mask)->isAllOnesValue())
        ReplacedInst = getPacketizeValue(Src2);
      else
        ReplacedInst = getPacketizeValue(Src1);
    } else {
      vc::diagnose(Inst->getContext(), "GenXPacketize",
                   "ShuffleVector should've been replaced by Scalarizer.",
                   Inst);
    }
    break;
  }
  case Instruction::IntToPtr: {
    auto *IntToPtr = cast<IntToPtrInst>(Inst);
    auto *VecSrc = getPacketizeValue(Inst->getOperand(0));
    auto *VecDestTy =
        IGCLLVM::FixedVectorType::get(IntToPtr->getDestTy(), B->VWidth);
    ReplacedInst = B->INT_TO_PTR(VecSrc, VecDestTy);
    break;
  }
  case Instruction::Select: {
    auto *VecCond = getPacketizeValue(Inst->getOperand(0));
    auto *TrueSrc = getPacketizeValue(Inst->getOperand(1));
    auto *FalseSrc = getPacketizeValue(Inst->getOperand(2));
    if (!TrueSrc->getType()->isPointerTy() ||
        TrueSrc->getType()->isOpaquePointerTy()) {
      // simple select packetization
      ReplacedInst = B->SELECT(VecCond, TrueSrc, FalseSrc);
    } else {
      // vector struct input, need to loop over components and build up new
      // struct allocation
      auto *Ty = IGCLLVM::getNonOpaquePtrEltTy(Inst->getType());
      auto *VecTy = B->getVectorType(Ty);
      auto *Alloca = B->ALLOCA(VecTy);
      uint32_t NumElems = Ty->getArrayNumElements();
      for (uint32_t Idx = 0; Idx < NumElems; ++Idx) {
        auto *TrueSrcElem = B->LOAD(VecTy, TrueSrc, {0, Idx});
        auto *FalseSrcElem = B->LOAD(VecTy, FalseSrc, {0, Idx});
        // mask store true components
        auto *GEP = B->GEP(VecTy, Alloca, {0, Idx});
        B->MASKED_STORE(TrueSrcElem, GEP, 4, VecCond);
        // store false components to inverted mask
        B->MASKED_STORE(FalseSrcElem, GEP, 4, B->NOT(VecCond));
      }
      ReplacedInst = Alloca;
    }
    break;
  }
  case Instruction::Ret: {
    auto *Ret = cast<ReturnInst>(Inst);
    if (Ret->getReturnValue() != nullptr) {
      auto *Return = getPacketizeValue(Ret->getReturnValue());
      auto *NewRet = B->RET(Return);
      ReplacedInst = NewRet;
    } else {
      ReplacedInst = Inst;
    }
    break;
  }
  default: {
    // for the rest of the instructions includingi phi, vectorize
    // the instruction type as well as its args
    auto *VecTy = B->getVectorType(Inst->getType());
    // Set vectorized dbg value to undef, since currently it's not
    // salvageble
    llvm::replaceDbgUsesWithUndef(Inst);
    Inst->mutateType(VecTy);
    for (auto &Op : Inst->operands()) {
      auto *V = getPacketizeValue(Op.get());
      if (V)
        Op.set(V);
      else if (!isa<PHINode>(Inst))
        vc::diagnose(Inst->getContext(), "GenXPacketize",
                     "Failed to packetize instruction", Inst);
    }
    ReplacedInst = Inst;
  }
  }
  return ReplacedInst;
}

Value *GenXPacketize::packetizeGenXIntrinsic(Instruction *Inst) {
  B->IRB->SetInsertPoint(Inst);
  if (auto *CI = dyn_cast_or_null<CallInst>(Inst)) {
    if (auto *Callee = CI->getCalledFunction()) {
      auto IID = GenXIntrinsic::getGenXIntrinsicID(Callee);
      Value *Replacement = nullptr;
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
        vc::diagnose(CI->getContext(), "GenXPacketize",
                     "Unsupported genx intrinsic in SIMT mode.", CI);
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
      case GenXIntrinsic::genx_dword_atomic_fsub: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto Args = {Src0, BTI, Src2, Src3, Src4};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType(), Src2->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_inc:
      case GenXIntrinsic::genx_dword_atomic_dec: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto Args = {Src0, BTI, Src2, Src3};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_fcmpwr: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType(), Src2->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_dword_atomic_cmpxchg: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_bfn: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *Src1 = getPacketizeValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *BFN = getUniformValue(CI->getOperand(3));
        auto Args = {Src0, Src1, Src2, BFN};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_svm_gather: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *NBlk = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(NBlk));
        auto *Addr = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto Args = {Predicate, NBlk, Addr, Src3};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Predicate->getType(), Addr->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_svm_scatter: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *NBlk = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(NBlk));
        auto *Addr = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto Args = {Predicate, NBlk, Addr, Src3};
        // store, no return type
        auto Tys = {Predicate->getType(), Addr->getType(), Src3->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_svm_gather4_scaled: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *ChMask = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(ChMask));
        auto *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        auto *Addr = getUniformValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Predicate, ChMask, Scale, Addr, Src4, Src5};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Predicate->getType(), Src4->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_svm_scatter4_scaled: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *ChMask = CI->getOperand(1);
        IGC_ASSERT(isa<Constant>(ChMask));
        auto *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        auto *Addr = getUniformValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Predicate, ChMask, Scale, Addr, Src4, Src5};
        // store no return type
        auto Tys = {Predicate->getType(), Addr->getType(), Src4->getType(),
                    Src5->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_gather4_typed: {
        auto *ChMask = CI->getOperand(0);
        IGC_ASSERT(isa<Constant>(ChMask));
        auto *Predicate = getPacketizeValue(CI->getOperand(1));
        auto *BTI = getUniformValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto *Src6 = getPacketizeValue(CI->getOperand(6));
        auto Args = {ChMask, Predicate, BTI, Src3, Src4, Src5, Src6};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Predicate->getType(), Src3->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_scatter4_typed: {
        auto *ChMask = CI->getOperand(0);
        IGC_ASSERT(isa<Constant>(ChMask));
        auto *Predicate = getPacketizeValue(CI->getOperand(1));
        auto *BTI = getUniformValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto *Src6 = getPacketizeValue(CI->getOperand(6));
        auto Args = {ChMask, Predicate, BTI, Src3, Src4, Src5, Src6};
        // store no return type
        auto Tys = {Predicate->getType(), Src3->getType(), Src6->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_scatter4_scaled:
      case GenXIntrinsic::genx_scatter_scaled: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *NBlk = CI->getOperand(1); // or channel mask for scatter4
        IGC_ASSERT(isa<Constant>(NBlk));
        auto *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        auto *BTI = getUniformValue(CI->getOperand(3));
        auto *GOff = getUniformValue(CI->getOperand(4));
        auto *ElemOffsets = getPacketizeValue(CI->getOperand(5));
        auto *InData = getPacketizeValue(CI->getOperand(6));
        auto Args = {Predicate, NBlk, Scale, BTI, GOff, ElemOffsets, InData};
        // no return value for store
        auto Tys = {Predicate->getType(), ElemOffsets->getType(),
                    InData->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_gather4_scaled:
      case GenXIntrinsic::genx_gather_scaled: {
        auto *Predicate = getPacketizeValue(CI->getOperand(0));
        auto *NBlk = CI->getOperand(1); // or channel mask for gather4
        IGC_ASSERT(isa<Constant>(NBlk));
        auto *Scale = CI->getOperand(2);
        IGC_ASSERT(isa<Constant>(Scale));
        auto *BTI = getUniformValue(CI->getOperand(3));
        auto *GOff = getUniformValue(CI->getOperand(4));
        auto *ElemOffsets = getPacketizeValue(CI->getOperand(5));
        auto *InData = getPacketizeValue(CI->getOperand(6));
        auto Args = {Predicate, NBlk, Scale, BTI, GOff, ElemOffsets, InData};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Predicate->getType(), ElemOffsets->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
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
        auto *ElemOffsets = getPacketizeValue(CI->getOperand(4));
        Args.push_back(ElemOffsets);
        Tys.push_back(B->getVectorType(CI->getType()));
        Tys.push_back(ElemOffsets->getType());
        if (IID == GenXIntrinsic::genx_gather4_masked_scaled2 ||
            IID == GenXIntrinsic::genx_gather_masked_scaled2) {
          Args.push_back(getPacketizeValue(CI->getOperand(5))); // Predicate
          Tys.push_back(Args.back()->getType());
        }
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      }
      case GenXIntrinsic::genx_lane_id: {
        IGC_ASSERT_MESSAGE((CI->getType()->getIntegerBitWidth() == 32),
                           "Expected to return 32-bit integer.");
        Replacement = B->CInc<uint32_t>(0, B->VWidth);
        return Replacement;
      } break;
      case GenXIntrinsic::genx_rdregionf:
      case GenXIntrinsic::genx_rdregioni: {
        // packetize intrinsic operands
        const DebugLoc &DL = CI->getDebugLoc();
        auto *OrigV0 = CI->getOperand(0);
        vc::CMRegion R(CI);
        IGC_ASSERT(R.Width == 1);
        if (cast<IGCLLVM::FixedVectorType>(OrigV0->getType())
                ->getNumElements() == 1) {
          Replacement = getPacketizeValue(OrigV0);
        } else {
          R.NumElements = B->VWidth;
          if (R.Indirect) {
            R.Indirect = getPacketizeValue(R.Indirect);
          }
          Replacement = R.createRdRegion(getPacketizeValue(OrigV0),
                                         CI->getName(), CI, DL);
        }
        return Replacement;
      } break;
      case GenXIntrinsic::genx_wrregionf:
      case GenXIntrinsic::genx_wrregioni: {
        auto *NewV0 = CI->getOperand(1);
        const DebugLoc &DL = CI->getDebugLoc();
        vc::CMRegion R(CI);
        IGC_ASSERT(isa<IGCLLVM::FixedVectorType>(NewV0->getType()));
        IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(NewV0->getType())
                       ->getNumElements() == 1);
        auto *NewV1 = getPacketizeValue(NewV0);
        R.NumElements = B->VWidth;
        if (R.Indirect) {
          R.Indirect = getPacketizeValue(R.Indirect);
        }
        auto *Src0Pack = getPacketizeValue(CI->getOperand(0));
        auto *Src0PackVTy = cast<IGCLLVM::FixedVectorType>(Src0Pack->getType());
        auto *ResVTy = cast<IGCLLVM::FixedVectorType>(CI->getType());
        if (!R.Indirect && ResVTy->getNumElements() == 1 &&
            Src0PackVTy->getNumElements() == B->VWidth) {
          R.Width = B->VWidth;
          Replacement =
              R.createWrRegion(Src0Pack, NewV1, CI->getName(), CI, DL);
          return Replacement;
        }
        Replacement =
            R.createWrRegion(CI->getOperand(0), NewV1, CI->getName(), CI, DL);
        return Replacement;
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
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *GOFF = getUniformValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Src0, BTI, GOFF, Src3, Src4, Src5};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_untyped_atomic_inc:
      case GenXIntrinsic::genx_untyped_atomic_dec: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *GOFF = getUniformValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto Args = {Src0, BTI, GOFF, Src3, Src4};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_untyped_atomic_cmpxchg: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *GOFF = getUniformValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto *Src6 = getPacketizeValue(CI->getOperand(6));
        auto Args = {Src0, BTI, GOFF, Src3, Src4, Src5, Src6};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
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
      case GenXIntrinsic::genx_typed_atomic_fsub: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto *Src6 = getPacketizeValue(CI->getOperand(6));
        auto Args = {Src0, BTI, Src2, Src3, Src4, Src5, Src6};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType(), Src3->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_typed_atomic_inc:
      case GenXIntrinsic::genx_typed_atomic_dec: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto Args = {Src0, BTI, Src2, Src3, Src4, Src5};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType(), Src2->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
      } break;
      case GenXIntrinsic::genx_typed_atomic_fcmpwr:
      case GenXIntrinsic::genx_typed_atomic_cmpxchg: {
        auto *Src0 = getPacketizeValue(CI->getOperand(0));
        auto *BTI = getUniformValue(CI->getOperand(1));
        auto *Src2 = getPacketizeValue(CI->getOperand(2));
        auto *Src3 = getPacketizeValue(CI->getOperand(3));
        auto *Src4 = getPacketizeValue(CI->getOperand(4));
        auto *Src5 = getPacketizeValue(CI->getOperand(5));
        auto *Src6 = getPacketizeValue(CI->getOperand(6));
        auto *Src7 = getPacketizeValue(CI->getOperand(7));
        auto Args = {Src0, BTI, Src2, Src3, Src4, Src5, Src6, Src7};
        auto *RetTy = B->getVectorType(CI->getType());
        auto Tys = {RetTy, Src0->getType(), Src4->getType()};
        auto *Decl = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Replacement = CallInst::Create(Decl, Args, CI->getName(), CI);
        cast<CallInst>(Replacement)->setDebugLoc(CI->getDebugLoc());
        return Replacement;
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
Value *GenXPacketize::packetizeInstruction(Instruction *Inst) {
  // determine instruction type and call its packetizer
  auto *Result = packetizeGenXIntrinsic(Inst);
  if (!Result) {
    if (IsLLVMIntrinsic(Inst))
      Result = packetizeLLVMIntrinsic(Inst);
    else
      Result = packetizeLLVMInstruction(Inst);
  }
  if (Result) {
    if (Inst->getName() != "")
      Result->setName(Inst->getName());
    // When the resulting instruction has the same type
    // Debug values can be preserved
    if (Result->getType() == Inst->getType()) {
      SmallVector<DbgVariableIntrinsic *, 1> DbgUsers;
      llvm::findDbgUsers(DbgUsers, Inst);
      for (auto *DII : DbgUsers)
        DII->replaceVariableLocationOp(Inst, Result);
    }
    // Copy any metadata to new instruction
    if (Result != Inst && isa<Instruction>(Result)) {
      cast<Instruction>(Result)->copyMetadata(*Inst);
      vectorizeRangeMetadata(Inst, dyn_cast<Instruction>(Result));
    }
  }
  return Result;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Remove replaced instructions. DCE will not remove calls, etc.
///        So we have to remove these manually.
void GenXPacketize::removeDeadInstructions() {
  SmallVector<Instruction *, 8> Unused;
  for (const auto &RMI : ReplaceMap)
    if (RMI.first != RMI.second)
      if (Instruction *UnusedInst =
              (Instruction *)dyn_cast<Instruction>(RMI.first))
        Unused.push_back(UnusedInst);
  for (auto *UnusedInst : Unused) {
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
  std::unordered_set<Instruction *> RemoveSet;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isa<CallInst>(I)) {
        auto *CI = cast<CallInst>(&I);
        auto *F = CI->getCalledFunction();
        if (F) {
          if (F->getName().startswith("sqrt")) {
            B->IRB->SetInsertPoint(&I);
            auto *pSqrt = B->VSQRTPS(CI->getOperand(0));
            CI->replaceAllUsesWith(pSqrt);
            RemoveSet.insert(CI);
          } else if (F->getName().startswith("fabs")) {
            B->IRB->SetInsertPoint(&I);
            auto *pFabs = B->FABS(CI->getOperand(0));
            CI->replaceAllUsesWith(pFabs);
            RemoveSet.insert(CI);
          } else if (F->getName().startswith("exp2")) {
            B->IRB->SetInsertPoint(&I);
            auto *pExp2 = B->EXP2(CI->getOperand(0));
            CI->replaceAllUsesWith(pExp2);
            RemoveSet.insert(CI);
          } else if (F->getName().equals("ldexpf")) {
            B->IRB->SetInsertPoint(&I);
            auto *pArg = CI->getOperand(0);
            auto *pExp = CI->getOperand(1);
            // replace ldexp with arg * 2^exp = arg * (2 << arg)
            auto *pShift = B->SHL(B->C(1), pExp);
            pShift = B->UI_TO_FP(pShift, B->FP32Ty);
            auto *pResult = B->FMUL(pArg, pShift);
            CI->replaceAllUsesWith(pResult);
            RemoveSet.insert(CI);
          }
        }
      }
    }
  }
  for (auto *Inst : RemoveSet) {
    Inst->eraseFromParent();
  }
}

//////////////////////////////////////////////////////////////////////////
/// @brief find the global ExecMask variable if exists in order to lower
/// CM SIMD control-flow representation after packetization
GlobalVariable *GenXPacketize::findGlobalExecMask() {
  // look for the global EMask variable if exists
  for (auto &Global : M->getGlobalList()) {
    auto *Ty = Global.getValueType();
    if (Ty->isVectorTy() &&
        cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() ==
            CMSimdCFLower::MAX_SIMD_CF_WIDTH) {
      auto *ElemTy = cast<VectorType>(Ty)->getElementType();
      if (ElemTy->isIntegerTy() && ElemTy->getIntegerBitWidth() == 1) {
        // so far the type is right, then check the use
        for (auto EMUI = Global.use_begin(), EMUE = Global.use_end();
             EMUI != EMUE; ++EMUI) {
          if (auto *LD = dyn_cast<LoadInst>(EMUI->getUser())) {
            for (auto UI = LD->user_begin(), E = LD->user_end(); UI != E;
                 ++UI) {
              const Value *LocalUse = (*UI);
              if (auto *CI = dyn_cast_or_null<CallInst>(LocalUse)) {
                if (auto *Callee = CI->getCalledFunction()) {
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
  auto *EMVar = findGlobalExecMask();
  // create one if we cannot find one.
  if (!EMVar) {
    auto *EMTy = IGCLLVM::FixedVectorType::get(
        Type::getInt1Ty(M->getContext()), CMSimdCFLower::MAX_SIMD_CF_WIDTH);
    EMVar = new GlobalVariable(*M, EMTy, false /*isConstant*/,
                               GlobalValue::InternalLinkage,
                               Constant::getAllOnesValue(EMTy), "EM");
  }
  CMSimdCFLower CFL(EMVar);
  // Derive an order to process functions such that a function is visited
  // after anything that calls it.
  int N = SIMTFuncs.size();
  for (int Idx = N - 1; Idx >= 0; --Idx) {
    auto *Fn = SIMTFuncs[Idx];
    CFL.processFunction(Fn);
    // Remove 'NoInline' attributes from calls - up to this point, they
    // have only been removed from the definition of functions
    for(auto ui = Fn->use_begin(), ue = Fn->use_end(); ui != ue; ++ui) {
      if (auto *I = dyn_cast<CallInst>(ui->getUser())) {
        if (I->hasFnAttr(Attribute::NoInline)) {
          I->removeFnAttr(Attribute::NoInline);
          I->addFnAttr(Attribute::AlwaysInline);
        }
      }
    }
  }
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

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses
GenXPacketizePass::run(llvm::Module &M, llvm::AnalysisManager<llvm::Module> &) {
  GenXPacketize GenXPack;
  if (GenXPack.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif
