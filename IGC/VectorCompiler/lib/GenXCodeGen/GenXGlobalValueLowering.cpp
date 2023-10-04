/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXGlobalValueLowering
/// --------------------------
///
/// This pass lowers global variables the following way:
///
/// For SLM global variables it replaces all locally linked globals with
/// i32 offsets in SLM buffer. Offsets are allocated in order of variable's
/// alignment decline
/// The following restrictions currently apply:
///   1. Global SLM variables with initializer are not supported.
///   2. Global SLM variables with external linkage are not supported.
///   3. Global SLM variables used in indirectly-called or external functions
///     and their subroutines are not supported.
///   4. All paths to global SLM use must start from the same kernel.
///
/// For any other global values relocations are used.
/// For global value symbols (global variable or function) LLVM IR
/// represintation of this instruction is:
/// %g = call i64 @llvm.genx.gaddr.i64.p0a8i8([8 x i8]* @very_important_data)
///
/// The goal of this lowering pass is to make every user of a global value
/// llvm.genx.gaddr intrinsic.
/// For example:
/// store <2 x [8 x i32]*> <[8 x i32]* @array_a, [8 x i32]* @array_b>,
///       <2 x [8 x i32]*>* %ptr
/// Becomes:
/// %array_a.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_a)
/// %array_a.lowered = inttoptr i64 %array_a.gaddr to [8 x i32]*
/// %array_b.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_b)
/// %array_b.lowered = inttoptr i64 %array_b.gaddr to [8 x i32]*
/// %vec.half = insertelement <2 x [8 x i32]*> undef,
///                           [8 x i32]* %array_a.lowered, i64 0
/// %vec = insertelement <2 x [8 x i32]*> %vec.half,
///                      [8 x i32]* %array_b.lowered, i64 1
/// store <2 x [8 x i32]*> %vec, <2 x [8 x i32]*>* %ptr
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXUtil.h"
#include "GenXRegionUtils.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>

#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;

namespace {

// Cannot use llvm::Use here as uses will change during the construction.
// Plus some helper methods for covenience.
struct UserInfo {
  Instruction *Inst;
  unsigned OperandNo;

  Value *getOperand() const { return Inst->getOperand(OperandNo); }
  Use &getOperandUse() const { return Inst->getOperandUse(OperandNo); }
  Constant *getConstOperand() const { return cast<Constant>(getOperand()); }

  friend bool operator==(const UserInfo &LHS, const UserInfo &RHS) {
    return LHS.Inst == RHS.Inst && LHS.OperandNo == RHS.OperandNo;
  }
};
} // anonymous namespace

template <> struct std::hash<UserInfo> {
  std::size_t operator()(const UserInfo &Usr) const {
    return std::hash<Instruction *>{}(Usr.Inst) ^
           std::hash<unsigned>{}(Usr.OperandNo);
  }
};

namespace {

class GenXGlobalValueLowering : public ModulePass {
  using FuncConstantReplacementMap = std::unordered_map<Constant *, Value *>;
  using FuncConstantUsersSet = std::unordered_set<UserInfo>;
  struct FuncConstantLoweringInfo {
    FuncConstantReplacementMap Replacement;
    FuncConstantUsersSet Users;
  };
  using ModuleConstantLoweringInfo =
      std::unordered_map<Function *, FuncConstantLoweringInfo>;

  using FHead = Function;
  using FuncGroupMap =
      std::unordered_map<const Function *, std::unordered_set<const FHead *>>;

  using SLMVarInfo = std::unordered_map<const GlobalValue *, unsigned>;
  using KernelSLMInfo = std::unordered_map<const FHead *, SLMVarInfo>;

  const DataLayout *DL = nullptr;
  ModuleConstantLoweringInfo WorkList;
  FuncGroupMap FunctionList;
  KernelSLMInfo KernelInfo;

public:
  static char ID;
  explicit GenXGlobalValueLowering() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX global value lowering";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
  void releaseMemory() override;
private:
  void fillWorkListForGV(GlobalValue &GV);
  void fillWorkListForGVUse(Use &GVUse, const GlobalValue &GV);
  void fillWorkListForGVInstUse(Use &GVUse, const GlobalValue &GV);
  void fillWorkListForGVConstUse(Use &GVUse, const GlobalValue &GV);
  void buildAllConstantReplacementsInFunction(Function &Func);
  Value *buildConstantReplacement(Constant &Const, IRBuilder<> &Builder,
                                  Function &Func);
  Instruction *buildGVReplacement(GlobalValue &Const, IRBuilder<> &Builder,
                                  Function &Func);
  Value *buildConstAggrReplacement(ConstantAggregate &ConstAggr,
                                   IRBuilder<> &Builder, Function &Func);
  Value *buildConstExprReplacement(ConstantExpr &ConstExpr,
                                   IRBuilder<> &Builder, Function &Func);
  void updateUsers(Function &Func);

  // SLM global variable support.
  void diagSLMVarUsage(const Function &F, const GlobalValue &GV);
  void traverseCallGraph(const FHead &Head, CallGraph &CG);
  void updateSLMOffsets(unsigned &KernelSLMMeta, unsigned &SLMVarOffset,
                        const GlobalValue &GV);
  void allocateOnSLM();
  alignment_t getGlobalVarAlign(const GlobalValue &GV);
};

} // end namespace

char GenXGlobalValueLowering::ID = 0;
namespace llvm {
void initializeGenXGlobalValueLoweringPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXGlobalValueLowering, "GenXGlobalValueLowering",
                      "GenXGlobalValueLowering", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(GenXGlobalValueLowering, "GenXGlobalValueLowering",
                    "GenXGlobalValueLowering", false, false)

ModulePass *llvm::createGenXGlobalValueLoweringPass() {
  initializeGenXGlobalValueLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXGlobalValueLowering;
}

static bool isSLMVarToReplace(const GlobalValue &GV) {
  if (GV.getAddressSpace() != vc::AddrSpace::Local)
    return false;
  if (auto *GVar = dyn_cast<const GlobalVariable>(&GV))
    if (!vc::isRealGlobalVariable(*GVar))
      return false;
  if (!GV.hasLocalLinkage()) {
    vc::diagnose(GV.getContext(), "GenXGlobalValueLowering",
                 "SLM global variables with local linkage are only supported",
                 &GV);
    return false;
  }
  return true;
}

static bool hasSLMGlobals(const Module &M) {
  for (auto &GV : M.globals()) {
    if (isSLMVarToReplace(GV))
      return true;
  }
  return false;
}

void GenXGlobalValueLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<CallGraphWrapperPass>();
}

bool GenXGlobalValueLowering::runOnModule(Module &M) {
  DL = &M.getDataLayout();
  auto &&BECfg = getAnalysis<GenXBackendConfig>();
  auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

  bool isSLMVarFound = hasSLMGlobals(M);
  if (isSLMVarFound)
    for (auto &F : M.functions())
      if (vc::isKernel(&F))
        traverseCallGraph(F, CG);

  for (auto &GV : M.globals())
    if (vc::isRealGlobalVariable(GV))
      fillWorkListForGV(GV);
  for (auto &F : M)
    if (vc::isIndirect(F) && !BECfg.directCallsOnly(F.getName()))
      fillWorkListForGV(F);

  if (WorkList.empty())
    return false;

  if (isSLMVarFound)
    allocateOnSLM();

  for (auto &FuncInfo : WorkList) {
    buildAllConstantReplacementsInFunction(*FuncInfo.first);
    updateUsers(*FuncInfo.first);
  }

  return true;
}

void GenXGlobalValueLowering::releaseMemory() {
  WorkList.clear();
  FunctionList.clear();
  KernelInfo.clear();
}

void GenXGlobalValueLowering::fillWorkListForGV(GlobalValue &GV) {
  IGC_ASSERT_MESSAGE(isa<GlobalObject>(GV),
                     "only global objects are yet supported");
  for (Use &GVUse : GV.uses())
    fillWorkListForGVUse(GVUse, GV);
}

// Only constants that are directly used in an instruction are added to
// replacement map. Other constants aren't added during this phase as
// it is hard to define in which fuction they are used, until instruction
// user is met.
void GenXGlobalValueLowering::fillWorkListForGVUse(Use &GVUse,
                                                   const GlobalValue &GV) {
  User *Usr = GVUse.getUser();
  if (isa<Instruction>(Usr)) {
    fillWorkListForGVInstUse(GVUse, GV);
    return;
  }
  IGC_ASSERT_MESSAGE(isa<Constant>(Usr), "unexpected global variable user");
  fillWorkListForGVConstUse(GVUse, GV);
}

static Instruction *buildGaddr(IRBuilder<> &Builder, GlobalValue &GV) {
  IGC_ASSERT_MESSAGE(isa<GlobalObject>(GV),
                     "only global objects are yet supported");
  Module *M = GV.getParent();
  Type *IntPtrTy = M->getDataLayout().getIntPtrType(GV.getType());
  Function *GAddrDecl = GenXIntrinsic::getGenXDeclaration(
      M, llvm::GenXIntrinsic::genx_gaddr, {IntPtrTy, GV.getType()});
  return Builder.CreateCall(GAddrDecl->getFunctionType(), GAddrDecl, &GV,
                            GV.getName() + ".gaddr");
}

void GenXGlobalValueLowering::fillWorkListForGVInstUse(Use &GVUse,
                                                       const GlobalValue &GV) {
  auto *Usr = cast<Instruction>(GVUse.getUser());
  auto *ConstWithGV = cast<Constant>(GVUse.get());
  IGC_ASSERT_MESSAGE(vc::getAnyIntrinsicID(Usr) !=
                         GenXIntrinsic::genx_gaddr,
                     "llvm.gaddr must be inserted by this pass, but someone "
                     "inserted it before");
  // Do not skip direct calls as an indirectly-called function can be called
  // only indirectly. See GenXCloneIndirectFunctions
  Function *Func = Usr->getFunction();
  WorkList[Func].Users.insert({Usr, GVUse.getOperandNo()});
  WorkList[Func].Replacement[ConstWithGV] = nullptr;

  if (isSLMVarToReplace(GV)) {
    diagSLMVarUsage(*Func, GV);
    KernelInfo[*FunctionList[Func].begin()][&GV] = 0;
  }
}

// For constant use continue recursively travers through users, until
// instruction user is met.
void GenXGlobalValueLowering::fillWorkListForGVConstUse(Use &GVUse,
                                                        const GlobalValue &GV) {
  auto *Usr = cast<Constant>(GVUse.getUser());
  for (Use &U : Usr->uses())
    fillWorkListForGVUse(U, GV);
}

// Build all the instruction that will replace constant uses in the provided
// function. All the instructions are inserted at the function entry.
void GenXGlobalValueLowering::buildAllConstantReplacementsInFunction(
    Function &Func) {
  auto &ConstantsInfo = WorkList[&Func];
  // Have to copy the original set of constants as replacement map will change
  // during the construction.
  std::vector<Constant *> Constants;
  std::transform(ConstantsInfo.Replacement.begin(),
                 ConstantsInfo.Replacement.end(), std::back_inserter(Constants),
                 [](FuncConstantReplacementMap::value_type &ReplacementInfo) {
                   return ReplacementInfo.first;
                 });
  IRBuilder<> Builder{&*Func.getEntryBlock().getFirstInsertionPt()};
  for (Constant *Const : Constants)
    buildConstantReplacement(*Const, Builder, Func);
}

Value *GenXGlobalValueLowering::buildConstantReplacement(Constant &Const,
                                                         IRBuilder<> &Builder,
                                                         Function &Func) {
  if (WorkList[&Func].Replacement.count(&Const) &&
      WorkList[&Func].Replacement.at(&Const))
    // the constant already has a replacement
    return WorkList[&Func].Replacement.at(&Const);

  if (isa<GlobalValue>(Const))
    return buildGVReplacement(cast<GlobalValue>(Const), Builder, Func);
  if (isa<ConstantAggregate>(Const))
    return buildConstAggrReplacement(cast<ConstantAggregate>(Const), Builder,
                                     Func);
  if (isa<ConstantExpr>(Const))
    return buildConstExprReplacement(cast<ConstantExpr>(Const), Builder, Func);
  // The rest of constants cannot have global variables as part of them.
  return &Const;
}

// Global variables are replaced with:
// 1. Int32 offset to ptr for SLM Variables.
//    %a.lowered = inttoptr i32 [offset] to [8 x i32] addrspace(3)*
// 2. A pair of gaddr + inttoptr for other global variables:
//    %a.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @a)
//    %a.lowered = inttoptr i64 %a.gaddr to [8 x i32]*
Instruction *GenXGlobalValueLowering::buildGVReplacement(GlobalValue &GV,
                                                         IRBuilder<> &Builder,
                                                         Function &Func) {
  IGC_ASSERT_MESSAGE(isa<GlobalObject>(GV),
                     "only global objects are yet supported");
  IGC_ASSERT_MESSAGE(WorkList[&Func].Replacement.count(&GV) == 0 ||
                         !WorkList[&Func].Replacement[&GV],
                     "the constant shouldn't have a replacement already");
  Value *GVAddr = nullptr;
  if (isSLMVarToReplace(GV))
    GVAddr = ConstantInt::get(Type::getInt32Ty(GV.getContext()),
                              KernelInfo[*FunctionList[&Func].begin()][&GV]);
  else
    GVAddr = buildGaddr(Builder, GV);
  // Force creating IntToPtr instruction, as CreateIntToPtr can create
  // a constexpr here.
  auto *GVReplacement = IntToPtrInst::Create(
      Instruction::CastOps::IntToPtr, GVAddr, GV.getType(),
      GV.getName() + ".lowered", &*Builder.GetInsertPoint());
  GVReplacement->setDebugLoc(Builder.getCurrentDebugLocation());
  // May create a new map entry here.
  WorkList[&Func].Replacement[&GV] = GVReplacement;
  return GVReplacement;
}

Value *GenXGlobalValueLowering::buildConstAggrReplacement(
    ConstantAggregate &ConstAggr, IRBuilder<> &Builder, Function &Func) {
  Value *Replacement = UndefValue::get(ConstAggr.getType());
  for (auto &IndexedOp : enumerate(ConstAggr.operands())) {
    auto *OpReplacement = buildConstantReplacement(
        *cast<Constant>(IndexedOp.value().get()), Builder, Func);

    if (isa<ConstantVector>(ConstAggr)) {
      Replacement = Builder.CreateInsertElement(
          Replacement, OpReplacement, IndexedOp.index(), "gaddr.lowering");
      continue;
    }
    Replacement = Builder.CreateInsertValue(
        Replacement, OpReplacement, IndexedOp.index(), "gaddr.lowering");
  }
  // May create a new map entry here.
  WorkList[&Func].Replacement[&ConstAggr] = Replacement;
  return Replacement;
}

Value *GenXGlobalValueLowering::buildConstExprReplacement(
    ConstantExpr &ConstExpr, IRBuilder<> &Builder, Function &Func) {
  std::vector<Value *> NewOps;
  std::transform(ConstExpr.op_begin(), ConstExpr.op_end(),
                 std::back_inserter(NewOps), [this, &Builder, &Func](Use &U) {
                   return buildConstantReplacement(*cast<Constant>(U.get()),
                                                   Builder, Func);
                 });
  Instruction *Replacement = ConstExpr.getAsInstruction();
  // set new operands
  std::copy(NewOps.begin(), NewOps.end(), Replacement->op_begin());
  Replacement->insertBefore(&*Builder.GetInsertPoint());
  Replacement->setName("gaddr.lowering");
  // May create a new map entry here.
  WorkList[&Func].Replacement[&ConstExpr] = Replacement;
  return Replacement;
}

// When all replacing instructions are generated, this method
// just replaces const operands with generated instructions.
void GenXGlobalValueLowering::updateUsers(Function &Func) {
  auto &ConstantsInfo = WorkList[&Func];
  std::unordered_set<Instruction *> PotentialToErase;
  std::vector<Instruction *> ToErase;

  for (UserInfo Usr : ConstantsInfo.Users) {
    Instruction *ConstReplacement =
        cast<Instruction>(ConstantsInfo.Replacement[Usr.getConstOperand()]);
    IGC_ASSERT_MESSAGE(ConstReplacement, "no replacement was generated");
    if (isa<PtrToIntInst>(Usr.Inst) && isa<IntToPtrInst>(ConstReplacement) &&
        Usr.Inst->getType() == ConstReplacement->getOperand(0)->getType()) {
      // can optimize here
      Usr.Inst->replaceAllUsesWith(ConstReplacement->getOperand(0));
      ToErase.push_back(Usr.Inst);
      PotentialToErase.insert(ConstReplacement);
      continue;
    }
    Usr.getOperandUse() = ConstReplacement;
  }

  for (Instruction *Inst : ToErase)
    Inst->eraseFromParent();
  for (Instruction *Inst : PotentialToErase)
    if (Inst->use_empty())
      Inst->eraseFromParent();
}

void GenXGlobalValueLowering::diagSLMVarUsage(const Function &F,
                                              const GlobalValue &GV) {
  if (!FunctionList.count(&F)) {
    if (vc::isIndirect(F))
      vc::diagnose(F.getContext(), "GenXGlobalValueLowering",
                   "SLM variable usage in indirectly called functions"
                   "is not supported");
    vc::fatal(F.getContext(), "GenXGlobalValueLowering",
              "function has not been mapped");
  } else if (FunctionList[&F].size() != 1) {
    vc::fatal(GV.getContext(), "GenXGlobalValueLowering",
              "SLM variable usage in functions which can be invoked from "
              "another kernel is not supported",
              &GV);
  }
}

// Create a function group map where each key is a function and
// its values are heads(kernels).
void GenXGlobalValueLowering::traverseCallGraph(const FHead &Head,
                                                CallGraph &CG) {
  SmallVector<const Function *, 8> ToProcess;
  ToProcess.push_back(&Head);

  while (!ToProcess.empty()) {
    const auto *F = ToProcess.pop_back_val();

    CallGraphNode &N = *CG[F];
    for (IGCLLVM::CallRecord CE : N) {
      const Function *Child = CE.second->getFunction();
      if (!Child || Child->isDeclaration())
        continue;
      bool NotVisited = FunctionList[Child].insert(&Head).second;
      if (NotVisited)
        ToProcess.push_back(Child);
    }
  }
  FunctionList[&Head].insert(&Head);
}

alignment_t GenXGlobalValueLowering::getGlobalVarAlign(const GlobalValue &V) {
  auto *GV = cast<const GlobalVariable>(&V);
  auto Align = GV->getAlignment();
  if (!Align)
    return DL->getABITypeAlignment(GV->getValueType());
  return Align;
}

// Asign offset to global SLM variable in SLM buffer. Update kernels slm buffer
// size metadata.
void GenXGlobalValueLowering::updateSLMOffsets(unsigned &KernelSLMMeta,
                                               unsigned &SLMVarOffset,
                                               const GlobalValue &GV) {
  auto Size = DL->getTypeStoreSize(GV.getValueType());
  auto Align = getGlobalVarAlign(GV);

  KernelSLMMeta =
      IGCLLVM::alignTo(KernelSLMMeta, IGCLLVM::getCorrectAlign(Align));
  SLMVarOffset = KernelSLMMeta;
  KernelSLMMeta += Size;
}

void GenXGlobalValueLowering::allocateOnSLM() {
  for (auto &KI : KernelInfo) {
    const FHead *Head = KI.first;
    vc::KernelMetadata KM(Head);
    unsigned OffsetCounter = KM.getSLMSize();

    // Have to make a copy to sort variables in alignment decline order.
    std::vector<const GlobalValue *> GVInfo;
    std::transform(
        KI.second.begin(), KI.second.end(), std::back_inserter(GVInfo),
        [](SLMVarInfo::value_type &SLMInfo) { return SLMInfo.first; });
    std::sort(GVInfo.begin(), GVInfo.end(),
              [this](const auto *lhs, const auto *rhs) {
                return getGlobalVarAlign(*lhs) > getGlobalVarAlign(*rhs);
              });
    for (auto GV : GVInfo)
      updateSLMOffsets(OffsetCounter, KI.second[GV], *GV);
    KM.updateSLMSizeMD(OffsetCounter);
  }
}
