/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///
/// GenXPrologEpilogInsertion
/// ---------------------
/// This pass generates IR for stack ABI we need to comply with on the IGC side
///
/// To make vISA emit accesses to its predefined registers we generate
/// genx.read/write.predef.reg intrinsics baled into rd/wrregions.
/// VISA regalloc makes sure that dests & sources of the generated instructions
/// are allocated to the proper predefined VISA regs
///
/// Lets say before transformation we have call with arg loaded from surface:
/// %arg = call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 1, i32 %addr)
/// %ret = call spir_func <8 x float> @foo(<8 x float> %arg)
///
/// Then after we will see something like this:
/// %arg = call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 1, i32 %addr)
/// %R8 = call <256 x float> read.predef.reg(i32 PREDEFINED_ARG, undef)
/// %NEWR8 = <256 x float> wrregion(<256 x float> %R8, <8 x float> %arg,
///                                 i32 0, i32 8, i32 1, OFFSET, ...)
/// ; Here OFFSET starts from 0 and is argument offset in predef register
/// %newarg = call <8 x float> write.predef.reg(i32 PREDEFINED_ARG,
///                                             <256 x float> %NEWR8)
/// %ret = call spir_func <8 x float> @foo(<8 x float> %newarg)
///
/// So we have explicit stack layout relatively early to use 64-bit splitting
/// on later stages if 64-bit pointers are used as SP/FP
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/IndexFlattener.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#define DEBUG_TYPE "GENX_PROLOGUE"

#include <unordered_set>

using namespace llvm;
using namespace genx;

static cl::opt<unsigned> ArgRegSizeInGRFsOpt(
    "vc-arg-reg-size", cl::init(visa::ArgRegSizeInGRFs), cl::Hidden,
    cl::desc("Set max ARG size in registers to use for arguments passing"));

static cl::opt<unsigned> RetRegSizeInGRFsOpt(
    "vc-ret-reg-size", cl::init(visa::RetRegSizeInGRFs), cl::Hidden,
    cl::desc("Set max RET size in registers to use for return value passing"));

namespace {

inline unsigned calcPadding(uint64_t Val, unsigned Align) {
  IGC_ASSERT(Align);
  if (Val % Align == 0)
    return 0;
  return Align - Val % Align;
}

// FIXME: This should be transformed to stack analysis eventually.
class ArgsAnalyzer {
  const DataLayout &DL;
  const unsigned ArgRegByteSize;
  const unsigned RetRegByteSize;
  const unsigned GRFByteSize;

  unsigned StackBytesUsedForArgs = 0;
  unsigned StackBytesUsedForRet = 0;
  unsigned ArgRegBytesUsed = 0;
  unsigned RetRegBytesUsed = 0;

  struct ArgPlace {
    unsigned ArgNo = 0;
    unsigned Offset = 0;
  };
  using StorageTy = std::vector<ArgPlace>;
  using const_iterator = StorageTy::const_iterator;
  StorageTy InReg;
  StorageTy InStack;

public:
  ArgsAnalyzer(unsigned ArgRegByteSizeIn, unsigned RetRegByteSizeIn,
               unsigned GRFByteSizeIn, const DataLayout &DLIn)
      : ArgRegByteSize(ArgRegByteSizeIn), RetRegByteSize(RetRegByteSizeIn),
        GRFByteSize(GRFByteSizeIn), DL(DLIn) {}

  void analyze(const FunctionType &FTy, const GenXBackendConfig &BEConf,
               bool InternalCC);
  void clear();

  unsigned getStackBytesUsed() const {
    return StackBytesUsedForArgs + StackBytesUsedForRet;
  }
  unsigned getStackBytesUsedForArgs() const { return StackBytesUsedForArgs; }
  unsigned getStackBytesUsedForRet() const { return StackBytesUsedForRet; }
  unsigned getArgRegBytesUsed() const { return ArgRegBytesUsed; }
  unsigned getRetRegBytesUsed() const { return RetRegBytesUsed; }

  iterator_range<const_iterator> in_reg() const {
    return make_range(InReg.begin(), InReg.end());
  }
  iterator_range<const_iterator> in_stack() const {
    return make_range(InStack.begin(), InStack.end());
  }
};

class GenXPrologEpilogInsertion
    : public FunctionPass,
      public InstVisitor<GenXPrologEpilogInsertion> {

  const DataLayout *DL = nullptr;
  const GenXSubtarget *ST = nullptr;
  const GenXBackendConfig *BEConf = nullptr;

  unsigned ArgRegByteSize = 0;
  unsigned RetRegByteSize = 0;

  // is stack used for stack calls (args, ret) or private memory.
  bool StackUsed = false;
  // is arg reg used for arguments passing.
  bool ArgRegUsed = false;
  // is ret reg used for return value passing.
  bool RetRegUsed = false;

  // Variable length arrays
  bool HasVLA = false;

  std::vector<AllocaInst *> Allocas;

  void clear();

  void initializeStack(Function &F);
  void generatePrologEpilog(Function &F);
  Value *generateStackCallProlog(Function &F, const ArgsAnalyzer &Info);
  Value *generateSubroutineProlog(Function &F) const;
  void generateStackCallEpilog(ReturnInst &RI, Value *TmpFP,
                               const ArgsAnalyzer &Info);
  void generateSubroutineEpilog(ReturnInst &RI, Value *TmpFP) const;

  void removeAttrs(Function &F) const;
  bool isStackPrimitivesUsed() const;

  Value *saveFP(IRBuilder<> &IRB) const;
  void restoreFPAndSP(Value *TmpFP, IRBuilder<> &IRB) const;

  // caller side argument layout
  void generateStackCall(CallInst &CI);

  void emitPrivateMemoryAllocations();

  void passArgsInReg(CallInst &CI, const ArgsAnalyzer &Info,
                     IRBuilder<> &IRB) const;
  void passInReg(Value &Op, unsigned OffsetInReg, IRBuilder<> &IRB,
                 PreDefined_Vars RegID, unsigned RegSize) const;
  void passNonAggrInReg(Value &Op, unsigned OffsetInReg, IRBuilder<> &IRB,
                        PreDefined_Vars RegID, unsigned RegSize) const;
  void passAggrInReg(Value &Op, unsigned OffsetInReg, IRBuilder<> &IRB,
                     PreDefined_Vars RegID, unsigned RegSize) const;
  void readArgsFromReg(Function &F, const ArgsAnalyzer &Info,
                       IRBuilder<> &IRB) const;
  void readFromRegAndReplace(Value &V, unsigned OffsetInReg, IRBuilder<> &IRB,
                             PreDefined_Vars RegID, unsigned RegSize) const;
  Value *readNonAggrFromReg(Type &Ty, unsigned OffsetInReg, IRBuilder<> &IRB,
                            PreDefined_Vars RegID, unsigned RegSize) const;
  Value *readAggrFromReg(Type &Ty, unsigned OffsetInReg, IRBuilder<> &IRB,
                         PreDefined_Vars RegID, unsigned RegSize) const;
  void passArgsOnStack(CallInst &CI, const ArgsAnalyzer &Info, IRBuilder<> &IRB,
                       Value &InitSP) const;
  void readArgsFromStack(Function &F, const ArgsAnalyzer &Info,
                         IRBuilder<> &IRB, Value &InitSP) const;

  void push(Value &V, IRBuilder<> &IRB, Value &SP) const;
  Value *pop(Type &ArgTy, IRBuilder<> &IRB, Value &InitSP) const;
  Value *getNextSP(Type &Ty, IRBuilder<> &IRB, Value &SP) const;

  // *** Helper functions ***
  // All params should be easy to understand.
  // BuildTempVal is used if the predefined register must be copied instead of
  // simply read. For example, when an argument is in ARGV and we cannot be sure
  // that between its uses ARGV will not be overwritten, the argument must be
  // read as a copy from ARG.
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Type *Ty, bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 1) const;
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Type *Ty, Value *Dep,
                                  bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 1) const;

  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB, Type *Ty) const;
  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB, Type *Ty,
                                          Value *Dep) const;

  Instruction *buildWritePredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                   Value *Input) const;
  Instruction *buildWritePredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                   Value *Input, Value *Dep,
                                   unsigned Offset = 0) const;

  std::pair<Value *, Value *>
  createBinOpPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                       Instruction::BinaryOps Opc, Value* Val,
                       bool Copy = false, bool UpdateReg = true) const;

  std::pair<Value *, Value *>
  createBinOpPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                       Instruction::BinaryOps Opc, unsigned long long Val,
                       bool Copy = false, bool UpdateReg = true) const;

  alignment_t getAllocaAlignment(AllocaInst *AI) const;

  Value *getThreadID(Module *M, IRBuilder<> &IRB) const;

public:
  static char ID;
  explicit GenXPrologEpilogInsertion();
  StringRef getPassName() const override {
    return "GenX prolog & epilog insertion";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
  void visitAllocaInst(AllocaInst &I);
  void visitCallInst(CallInst &I);
};

} // namespace

char GenXPrologEpilogInsertion::ID = 0;
namespace llvm {
void initializeGenXPrologEpilogInsertionPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPrologEpilogInsertion, "GenXPrologEpilogInsertion",
                      "GenXPrologEpilogInsertion", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPrologEpilogInsertion, "GenXPrologEpilogInsertion",
                    "GenXPrologEpilogInsertion", false, false)

FunctionPass *llvm::createGenXPrologEpilogInsertionPass() {
  return new GenXPrologEpilogInsertion();
}

namespace {

static bool isStackCallImplicitlyVectorized(const llvm::Type &Ty) {
  return Ty.isFloatingPointTy() || Ty.isIntegerTy() || Ty.isPointerTy();
}

// FIXME: Do we really need this MD? Isn't it better to use some analysis here
// and in cisa builder.
template <typename T>
static void appendArgRetMetadata(T &AppendTo, unsigned ArgSize,
                                 unsigned RetSize, IRBuilder<> &IRB) {
  LLVMContext &Ctx = AppendTo.getContext();
  MDNode *ArgMD =
      MDNode::get(Ctx, ConstantAsMetadata::get(IRB.getInt32(ArgSize)));
  MDNode *RetMD =
      MDNode::get(Ctx, ConstantAsMetadata::get(IRB.getInt32(RetSize)));
  AppendTo.setMetadata(vc::InstMD::FuncArgSize, ArgMD);
  AppendTo.setMetadata(vc::InstMD::FuncRetSize, RetMD);
}

void ArgsAnalyzer::clear() {
  InReg.clear();
  InStack.clear();
  ArgRegBytesUsed = 0;
  RetRegBytesUsed = 0;
  StackBytesUsedForArgs = 0;
  StackBytesUsedForRet = 0;
}

// FIXME: Revisit this after trampolineInsertion enabling
static bool isInternalCC(const Function &F) {
  return F.hasLocalLinkage() && !vc::isIndirect(&F);
}

static bool isInternalCC(const CallInst &CI) {
  if (CI.isIndirectCall())
    return false;
  Function *F = CI.getCalledFunction();
  IGC_ASSERT(F);
  return isInternalCC(*F);
}

static unsigned getTypeSizeNoPadding(Type *Ty, const DataLayout *DL) {
  unsigned NumElts = IndexFlattener::getNumElements(Ty);
  unsigned TotalSize = 0;
  for (unsigned i = 0; i < NumElts; ++i) {
    Type *EltTy = IndexFlattener::getElementType(Ty, i);
    TotalSize += vc::getTypeSize(EltTy, DL).inBytes();
  }
  return TotalSize;
}

// FIXME: bool InternalCC might be replaced with llvm::CallingConv but firstly
// we should create/adapt some stack analyzes.
void ArgsAnalyzer::analyze(const FunctionType &FTy,
                           const GenXBackendConfig &BEConf, bool InternalCC) {
  clear();

  for (unsigned i = 0; i < FTy.getNumParams(); ++i) {
    // Each arg passed on ArgReg must be GRF aligned. Each arg passed on stack
    // must be oword aligned.
    IGC_ASSERT_MESSAGE(calcPadding(ArgRegBytesUsed, GRFByteSize) == 0,
                       "Must be grf aligned");
    IGC_ASSERT_MESSAGE(calcPadding(StackBytesUsedForArgs, OWordBytes) == 0,
                       "Must be oword aligned");
    Type *ArgTy = FTy.getParamType(i);
    // FIXME: Do we have a way to unambiguously understand whether an aggregate
    // type satisfies the IGC calling conv or not?
    unsigned ArgSize = getTypeSizeNoPadding(ArgTy, &DL);
    IGC_ASSERT_MESSAGE(ArgSize, "Arg size cannot be zero");
    bool Overflow = ArgRegBytesUsed + ArgSize > ArgRegByteSize;

    if (!Overflow) {
      // Fits in ArgReg.
      InReg.push_back({i, ArgRegBytesUsed});
      ArgRegBytesUsed += ArgSize;
      ArgRegBytesUsed += calcPadding(ArgRegBytesUsed, GRFByteSize);
    } else {
      // Doesn't fit. The stack must be used.
      // Stack args are OWord aligned.
      InStack.push_back({i, StackBytesUsedForArgs});
      if (ArgTy->isAggregateType()) {
        IGC_ASSERT_MESSAGE(InternalCC,
                           "IGC calling convention does not allow passing "
                           "aggregate operands by value on stack");
        // Do not pack aggregates when passing on stack.
        ArgSize = vc::getTypeSize(ArgTy, &DL).inBytes();
      }
      StackBytesUsedForArgs += ArgSize;
      StackBytesUsedForArgs += calcPadding(StackBytesUsedForArgs, OWordBytes);
    }
  }

  Type *RetTy = FTy.getReturnType();
  if (RetTy->isVoidTy())
    return;

  const auto RetSize = getTypeSizeNoPadding(RetTy, &DL) *
                       (!InternalCC && isStackCallImplicitlyVectorized(*RetTy)
                            ? BEConf.getInteropSubgroupSize()
                            : 1);

  if (RetSize > RetRegByteSize)
    StackBytesUsedForRet = RetSize + calcPadding(RetSize, OWordBytes);
  else
    RetRegBytesUsed = RetSize + calcPadding(RetSize, GRFByteSize);
}

GenXPrologEpilogInsertion::GenXPrologEpilogInsertion() : FunctionPass(ID) {
  initializeGenXPrologEpilogInsertionPass(*PassRegistry::getPassRegistry());
}

void GenXPrologEpilogInsertion::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
}

bool GenXPrologEpilogInsertion::isStackPrimitivesUsed() const {
  return StackUsed || ArgRegUsed || RetRegUsed;
}

// If stack or ARG/RET reg used, the function cannot have restrictions on
// read/write memory accesses.
// FIXME: actually, it can, but it requires deeper analysis. Maybe we can
// improve this after stack usage analysis improvement.
void GenXPrologEpilogInsertion::removeAttrs(Function &F) const {
  if (!isStackPrimitivesUsed())
    return;

  IGCLLVM::MemoryEffects ME(IGCLLVM::ModRefInfo::ModRef);
  IGCLLVM::setMemoryEffects(F, ME);
}

bool GenXPrologEpilogInsertion::runOnFunction(Function &F) {
  clear();

  BEConf = &getAnalysis<GenXBackendConfig>();
  DL = &F.getParent()->getDataLayout();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  ArgRegByteSize = ArgRegSizeInGRFsOpt * ST->getGRFByteSize();
  RetRegByteSize = RetRegSizeInGRFsOpt * ST->getGRFByteSize();
  LLVM_DEBUG(dbgs() << "ArgReg size is " << ArgRegByteSize << "\n");
  LLVM_DEBUG(dbgs() << "RetReg size is " << RetRegByteSize << "\n");

  // Generate caller code for stack calls and private memory allocations first.
  // Only after that generate prolog and epilog code. Such order is important
  // because it allows saying whether the stack is used inside the function.
  // This knowledge is required to decide on what must be generated in prolog
  // and epilog. For example, we can omit to save FP if the stack won't be used.
  // visit alloca
  // visit call inst
  visit(F);
  // All allocations were collected. Now we can efficiently generate
  // private memory allocations.
  emitPrivateMemoryAllocations();

  if (vc::isKernel(&F))
    initializeStack(F);
  else
    // stack calls and subroutines
    generatePrologEpilog(F);

  removeAttrs(F);

  return isStackPrimitivesUsed();
}

Value *GenXPrologEpilogInsertion::getThreadID(Module *M,
                                              IRBuilder<> &IRB) const {
  Function *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, vc::InternalIntrinsic::logical_thread_id, {});
  auto *TID = IRB.CreateCall(Func);
  return TID;
}

// FE_SP = PrivateBase + HWTID * PrivMemPerThread
// FE_FP = FE_SP
void GenXPrologEpilogInsertion::initializeStack(Function &F) {
  IGC_ASSERT(vc::isKernel(&F));
  LLVM_DEBUG(dbgs() << "Initialize stack for kenrel " << F.getName() << "\n");
  IRBuilder<> IRB(&*F.getEntryBlock().getFirstInsertionPt());

  int StackAmount =
      vc::getStackAmount(&F, BEConf->getStatelessPrivateMemSize());

  // Nothing to do. Stack is guaranteed not to be used.
  if (StackAmount == 0)
    return;

  StackUsed |= true;

  Value *TID = getThreadID(F.getParent(), IRB);
  Value *StackAmountPerThread = IRB.getInt32(StackAmount);

  if ((uint64_t)ST->getMaxThreadsNumPerSubDevice() * StackAmount >
      std::numeric_limits<uint32_t>::max()) {
    TID = IRB.CreateZExt(TID, IRB.getInt64Ty());
    StackAmountPerThread =
        IRB.CreateZExt(StackAmountPerThread, IRB.getInt64Ty());
  }

  auto *Mul = IRB.CreateMul(TID, StackAmountPerThread);
  auto *MulCasted = IRB.CreateZExtOrBitCast(Mul, IRB.getInt64Ty());

  auto &PrivBase =
      vc::getImplicitArg(F, vc::KernelMetadata::IMP_OCL_PRIVATE_BASE);
  auto *Add = IRB.CreateAdd(&PrivBase, MulCasted);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, Add);
  Value *SP = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                 IRB.getInt64Ty(), false);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_FP, IRB, SP);
}

Value *GenXPrologEpilogInsertion::saveFP(IRBuilder<> &IRB) const {
  Value *TmpFP = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_FP, IRB,
                                    IRB.getInt64Ty(), true);
  Value *SP = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                 IRB.getInt64Ty(), false);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_FP, IRB, SP);
  return TmpFP;
}

void GenXPrologEpilogInsertion::restoreFPAndSP(Value *TmpFP,
                                               IRBuilder<> &IRB) const {
  Value *FP = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_FP, IRB,
                                 IRB.getInt64Ty(), false);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, FP);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_FP, IRB, TmpFP);
}

// Stack call calling conv: behave in accordance with IGC calling conv.
//   entry:
//     tmpFP = FP
//     FP = SP
//   ...
//   retBlock: (any)
//     SP = FP
//     FP = tmpFP
//     ret
//
// Subroutine calling conv: internal calling conv. We have to create tmpFP
// variable to store SP so as to restore SP before the returns after possible
// allocations + run-time alignments.
//   entry:
//     tmpFP = SP
//   ...
//   retBlock: (any)
//     SP = tmpFP
void GenXPrologEpilogInsertion::generatePrologEpilog(Function &F) {
  /*NB/TODO: ArgsAnalyzer Info is only used for stack call cases below and
    isInternalCC is always false in those cases. If ArgsAnalyzer can not be
    reused for subroutine-related parts, the code needs to be refactored to
    eliminate excessive entities.
  */
  ArgsAnalyzer Info(ArgRegByteSize, RetRegByteSize, ST->getGRFByteSize(), *DL);
  Value *TmpFP = nullptr;

  if (vc::requiresStackCall(&F)) {
    Info.analyze(*F.getFunctionType(), *BEConf,
                 /*NB/TODO: this is always false here*/ isInternalCC(F));
    TmpFP = generateStackCallProlog(F, Info);
  } else
    TmpFP = generateSubroutineProlog(F);

  for (auto &BB : F) {
    auto *RI = dyn_cast<ReturnInst>(BB.getTerminator());
    if (!RI)
      continue;

    if (vc::requiresStackCall(&F))
      generateStackCallEpilog(*RI, TmpFP, Info);
    else
      generateSubroutineEpilog(*RI, TmpFP);
  }
}

void GenXPrologEpilogInsertion::generateSubroutineEpilog(ReturnInst &RI,
                                                         Value *TmpFP) const {
  if (!TmpFP) {
    IGC_ASSERT_MESSAGE(
        !StackUsed,
        "SP must be restored from temporary value if stack was used");
    return;
  }

  IGC_ASSERT_MESSAGE(StackUsed, "Stack was not used, there is no need to "
                                "restore SP from temporary value");
  IRBuilder<> IRB(&RI);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, TmpFP);
}

void GenXPrologEpilogInsertion::generateStackCallEpilog(
    ReturnInst &RI, Value *TmpFP, const ArgsAnalyzer &Info) {
  IRBuilder<> IRB(&RI);

  if (TmpFP)
    restoreFPAndSP(TmpFP, IRB);

  if (!Info.getRetRegBytesUsed() && !Info.getStackBytesUsedForRet())
    return;

  Value *RetVal = RI.getReturnValue();
  // Implicitly vectorize the returned value based on calling convention
  // requirement. This is supported only for scalars.
  // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_invoke_simd.asciidoc
  if (isStackCallImplicitlyVectorized(*RetVal->getType()))
    RetVal = IRB.CreateVectorSplat(BEConf->getInteropSubgroupSize(), RetVal,
                                   "call_conv_vectorized_retval");
  if (Info.getRetRegBytesUsed()) {
    IGC_ASSERT(!Info.getStackBytesUsedForRet());
    passInReg(*RetVal, 0, IRB, PreDefined_Vars::PREDEFINED_RET, RetRegByteSize);
    RetRegUsed |= true;
  } else if (Info.getStackBytesUsedForRet()) {
    Value *RetSP = nullptr;
    std::tie(std::ignore, RetSP) = createBinOpPredefReg(
        PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Sub,
        Info.getStackBytesUsed(), false, false);
    push(*RetVal, IRB, *RetSP);
    StackUsed |= true;
  }
}

Value *GenXPrologEpilogInsertion::generateSubroutineProlog(Function &F) const {
  LLVM_DEBUG(dbgs() << "Generating function prologue for " << F.getName()
                    << " (subroutine)\n");
  IRBuilder<> IRB(&F.getEntryBlock().front());

  if (StackUsed)
    return buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                              IRB.getInt64Ty(), true);
  return nullptr;
}

Value *
GenXPrologEpilogInsertion::generateStackCallProlog(Function &F,
                                                   const ArgsAnalyzer &Info) {
  LLVM_DEBUG(dbgs() << "Generating function prologue for " << F.getName()
                    << " (stackcall)\n");
  IRBuilder<> IRB(&F.getEntryBlock().front());

  Value *TmpFP = nullptr;
  if (StackUsed)
    TmpFP = saveFP(IRB);

  if (Info.getArgRegBytesUsed()) {
    readArgsFromReg(F, Info, IRB);
    ArgRegUsed |= true;
  }
  if (Info.getStackBytesUsedForArgs()) {
    StackUsed |= true;
    Value *SpArgBegin = nullptr;
    std::tie(std::ignore, SpArgBegin) = createBinOpPredefReg(
        PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Sub,
        Info.getStackBytesUsedForArgs(), false, false);
    IGC_ASSERT(SpArgBegin);
    readArgsFromStack(F, Info, IRB, *SpArgBegin);
  }

  IGC_ASSERT(Info.getArgRegBytesUsed() % ST->getGRFByteSize() == 0);
  IGC_ASSERT(Info.getRetRegBytesUsed() % ST->getGRFByteSize() == 0);
  appendArgRetMetadata(F, Info.getArgRegBytesUsed() / ST->getGRFByteSize(),
                       Info.getRetRegBytesUsed() / ST->getGRFByteSize(), IRB);

  return TmpFP;
}

void GenXPrologEpilogInsertion::visitAllocaInst(AllocaInst &AI) {
  IGC_ASSERT(!AI.isUsedWithInAlloca());
  const BasicBlock *Parent = AI.getParent();
  if (Parent == &Parent->getParent()->front()) {
    Allocas.push_back(&AI);
    if (!isa<ConstantInt>(AI.getArraySize()))
      HasVLA = true;
  } else {
    IRBuilder<> IRB(&AI);
    unsigned Alignment = getAllocaAlignment(&AI);
    createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                         Instruction::Add, Alignment - 1);
    createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                         Instruction::And, ~(Alignment - 1));
    Value *AllocaSize = nullptr;
    if (isa<ConstantInt>(AI.getArraySize())) {
      AllocaSize = IRB.getInt64(
          divideCeil(*AI.getAllocationSizeInBits(*DL), genx::ByteBits));
    } else {
      unsigned ElementSize = llvm::divideCeil(
          DL->getTypeAllocSizeInBits(AI.getAllocatedType()), genx::ByteBits);
      AllocaSize =
          IRB.CreateMul(IRB.getInt64(ElementSize),
                        IRB.CreateZExt(AI.getOperand(0), IRB.getInt64Ty()));
    }
    auto [OrigSP, _] =
        createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                             Instruction::Add, AllocaSize, true);
    auto *AllocaAddr = IRB.CreateIntToPtr(OrigSP, AI.getType(), AI.getName());
    AI.replaceAllUsesWith(AllocaAddr);
    AI.eraseFromParent();
  }
}

void GenXPrologEpilogInsertion::emitPrivateMemoryAllocations() {
  LLVM_DEBUG(dbgs() << "In emitPrivateMemoryAllocations\n");
  if (Allocas.empty()) {
    LLVM_DEBUG(dbgs() << "no alloca instructions in the entry basic block\n");
    return;
  }

  StackUsed |= true;

  IRBuilder<> IRB(Allocas.front());

  // Sort to have allocas alignment in decreasing order.
  std::stable_sort(Allocas.begin(), Allocas.end(),
                   [this](AllocaInst *LHS, AllocaInst *RHS) {
                     return getAllocaAlignment(LHS) > getAllocaAlignment(RHS);
                   });

  // SP is oword bytes aligned at this point.
  unsigned long long LargestAlignment = getAllocaAlignment(Allocas.front());
  if (LargestAlignment > OWordBytes) {
    createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                         Instruction::Add, LargestAlignment - 1, false);
    createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                         Instruction::And, ~(LargestAlignment - 1), false);
  }

  std::vector<unsigned> NextAlignment;
  std::transform(std::next(Allocas.begin()), Allocas.end(),
                 std::back_inserter(NextAlignment), [this](AllocaInst *AI) {
                   // visa::BytesPerSVMPtr is a minimal possible alignment
                   // that suits all data types.
                   return std::max(getAllocaAlignment(AI),
                                   visa::BytesPerSVMPtr);
                 });
  // After all private memory allocations SP must be oword aligned so as to
  // keep SP alignment statically known.
  NextAlignment.push_back(OWordBytes);

  unsigned TotalSize = 0;
  for (auto &&[AI, Alignment] : zip(Allocas, NextAlignment)) {
    Value *OrigSP = nullptr;
    if (HasVLA) {
      if (isa<ConstantInt>(AI->getArraySize())) {
        unsigned AllocaSize =
            llvm::divideCeil(*AI->getAllocationSizeInBits(*DL), genx::ByteBits);
        std::tie(OrigSP, std::ignore) = createBinOpPredefReg(
            PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Add,
            AllocaSize + Alignment - 1, true);
      } else {
        unsigned ElementSize = llvm::divideCeil(
            DL->getTypeAllocSizeInBits(AI->getAllocatedType()), genx::ByteBits);
        Value *AllocaSize =
            IRB.CreateMul(IRB.getInt64(ElementSize),
                          IRB.CreateZExt(AI->getOperand(0), IRB.getInt64Ty()));
        std::tie(OrigSP, std::ignore) = createBinOpPredefReg(
            PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Add,
            IRB.CreateAdd(AllocaSize, IRB.getInt64(Alignment - 1)), true);
      }
      createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                           Instruction::And, ~(Alignment - 1));
    } else {
      unsigned AllocaSize =
          llvm::divideCeil(*AI->getAllocationSizeInBits(*DL), genx::ByteBits);
      TotalSize += AllocaSize;
      unsigned AllocaPadding = calcPadding(TotalSize, Alignment);
      TotalSize += AllocaPadding;
      std::tie(OrigSP, std::ignore) = createBinOpPredefReg(
          PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Add,
          AllocaSize + AllocaPadding, true);
    }
    CastInst *AllocaAddr = CastInst::Create(Instruction::IntToPtr, OrigSP,
                                            AI->getType(), AI->getName(), AI);
    AllocaAddr->setDebugLoc(AI->getDebugLoc());
    AI->replaceAllUsesWith(AllocaAddr);
  }

  // erase allocas in a separate loop so as not to invalidate IRBuilder
  // insertion point.
  std::for_each(Allocas.begin(), Allocas.end(),
                [](AllocaInst *AI) { AI->eraseFromParent(); });
}

void GenXPrologEpilogInsertion::visitCallInst(CallInst &I) {
  if (I.isInlineAsm())
    return;
  if (vc::isAnyNonTrivialIntrinsic(&I))
    return;
  bool IsIndirectCall = I.isIndirectCall();
// FIXME: Temporary solution until SPIRV translator conversion of unnamed
// structure types is fixed for intrinsics.
  bool IsIntrinsicIndirect = false;
  Value *Op = I.getCalledOperand();
  if (auto *CE = dyn_cast<ConstantExpr>(Op);
      CE && CE->getOpcode() == Instruction::BitCast) {
    auto *CalledFunction = cast<Function>(CE->getOperand(0));
    IsIntrinsicIndirect = vc::isAnyNonTrivialIntrinsic(CalledFunction);
    IGC_ASSERT_MESSAGE(IsIntrinsicIndirect, "Only intrinsic is expected");
  }

  bool IsStackCall =
      !IsIntrinsicIndirect &&
      (IsIndirectCall || vc::requiresStackCall(I.getCalledFunction()));

  // We have a subroutine or stack call that won't be correctly analyzed. The
  // analysis is supposed to answer a question of whether ARG, RET registers
  // are used. Until this analysis is implemented, let's assume that the call
  // can overwrite ARG and RET. This allows generating always correct code.
  // FIXME: Generated code may be non-optimal since it will always lead to
  // ARG, REG copy generation in the presence of any calls. The described
  // analysis should be added.
  ArgRegUsed = true;
  RetRegUsed = true;

  if (!IsStackCall)
    return;

  generateStackCall(I);
  IGCLLVM::MemoryEffects ME(IGCLLVM::ModRefInfo::ModRef);
  IGCLLVM::setMemoryEffects(I, ME);
}

void GenXPrologEpilogInsertion::clear() {
  DL = nullptr;
  ST = nullptr;
  BEConf = nullptr;

  ArgRegByteSize = 0;
  RetRegByteSize = 0;

  // is stack used for stack calls or private memory.
  StackUsed = false;
  ArgRegUsed = false;
  RetRegUsed = false;

  Allocas.clear();
}

// generate caller site of stack call
void GenXPrologEpilogInsertion::generateStackCall(CallInst &CI) {
  ArgsAnalyzer Info(ArgRegByteSize, RetRegByteSize, ST->getGRFByteSize(), *DL);
  Info.analyze(*CI.getFunctionType(), *BEConf, isInternalCC(CI));

  IRBuilder<> IRB(&CI);
  ArgRegUsed |= static_cast<bool>(Info.getArgRegBytesUsed());
  RetRegUsed |= static_cast<bool>(Info.getRetRegBytesUsed());
  passArgsInReg(CI, Info, IRB);

  // Generate stack-related instructions only if it is used.
  if (Info.getStackBytesUsed()) {
    StackUsed |= true;

    // Allocate space in stack for the ret value.
    if (Info.getStackBytesUsedForRet())
      createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                           Instruction::Add, Info.getStackBytesUsedForRet(),
                           false);

    // Allocate space in stack for the args and copy arg by arg.
    if (Info.getStackBytesUsedForArgs()) {
      Value *OrigSp = nullptr;
      std::tie(OrigSp, std::ignore) = createBinOpPredefReg(
          PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Add,
          Info.getStackBytesUsedForArgs(), true);
      IGC_ASSERT(OrigSp);

      passArgsOnStack(CI, Info, IRB, *OrigSp);
    }
  }

  IRB.SetInsertPoint(CI.getNextNode());

  IGC_ASSERT(Info.getArgRegBytesUsed() % ST->getGRFByteSize() == 0);
  IGC_ASSERT(Info.getRetRegBytesUsed() % ST->getGRFByteSize() == 0);

  appendArgRetMetadata(CI, Info.getArgRegBytesUsed() / ST->getGRFByteSize(),
                       Info.getRetRegBytesUsed() / ST->getGRFByteSize(), IRB);

  if (Info.getRetRegBytesUsed())
    readFromRegAndReplace(CI, 0, IRB, PreDefined_Vars::PREDEFINED_RET,
                          RetRegByteSize);
  else if (Info.getStackBytesUsedForRet()) {
    Value *RetSP = nullptr;
    std::tie(std::ignore, RetSP) = createBinOpPredefReg(
        PreDefined_Vars::PREDEFINED_FE_SP, IRB, Instruction::Sub,
        Info.getStackBytesUsed(), false, false);
    Value *Ret = pop(*CI.getType(), IRB, *RetSP);
    CI.replaceAllUsesWith(Ret);
  }

  if (Info.getStackBytesUsed())
    createBinOpPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                         Instruction::Sub, Info.getStackBytesUsed(), false);
}

std::pair<Value *, Value *> GenXPrologEpilogInsertion::createBinOpPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Instruction::BinaryOps Opc,
    unsigned long long Val, bool Copy, bool UpdateReg) const {
  return createBinOpPredefReg(RegID, IRB, Opc, IRB.getInt64(Val), Copy,
                              UpdateReg);
}

std::pair<Value *, Value *> GenXPrologEpilogInsertion::createBinOpPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Instruction::BinaryOps Opc,
    Value *Val, bool Copy, bool UpdateReg) const {
  Value *RegRead = buildReadPredefReg(RegID, IRB, IRB.getInt64Ty(), Copy, true);
  Value *NewValue = IRB.CreateBinOp(Opc, RegRead, Val);
  if (UpdateReg)
    buildWritePredefReg(RegID, IRB, NewValue);
  if (!Copy)
    RegRead = nullptr;
  return std::make_pair(RegRead, NewValue);
}

void GenXPrologEpilogInsertion::readArgsFromStack(Function &F,
                                                  const ArgsAnalyzer &Info,
                                                  IRBuilder<> &IRB,
                                                  Value &InitSP) const {
  LLVM_DEBUG(dbgs() << "Reading these args from stack. Function " << F.getName()
                    << "\n");

  Value *SP = &InitSP;
  for (const auto &ArgInfo : Info.in_stack()) {
    Value *Arg = IGCLLVM::getArg(F, ArgInfo.ArgNo);
    Type *ArgTy = Arg->getType();
    Value *RealArg = pop(*ArgTy, IRB, *SP);
    Arg->replaceAllUsesWith(RealArg);
    SP = getNextSP(*ArgTy, IRB, *SP);
  }
}

void GenXPrologEpilogInsertion::passArgsOnStack(CallInst &CI,
                                                const ArgsAnalyzer &Info,
                                                IRBuilder<> &IRB,
                                                Value &InitSP) const {
  Value *SP = &InitSP;
  for (const auto &ArgInfo : Info.in_stack()) {
    Value *Arg = CI.getArgOperand(ArgInfo.ArgNo);
    push(*Arg, IRB, *SP);
    SP = getNextSP(*Arg->getType(), IRB, *SP);
  }
}

Value *GenXPrologEpilogInsertion::pop(Type &ArgTy, IRBuilder<> &IRB,
                                      Value &InitSP) const {
  // FIXME: There must be a correct addrspace.
  Type *SPPtrTy = PointerType::getUnqual(&ArgTy);
  Value *SPPtr = IRB.CreateIntToPtr(&InitSP, SPPtrTy);

  return IRB.CreateLoad(&ArgTy, SPPtr);
}

// It does not allocate memory
Value *GenXPrologEpilogInsertion::getNextSP(Type &Ty, IRBuilder<> &IRB,
                                            Value &SP) const {
  // FIXME: type size calculations and padding should be moved to some
  // analysis.
  unsigned Size = vc::getTypeSize(&Ty, DL).inBytes();
  Size += calcPadding(Size, OWordBytes);
  return IRB.CreateAdd(&SP, IRB.getInt64(Size));
}

void GenXPrologEpilogInsertion::push(Value &V, IRBuilder<> &IRB,
                                     Value &SP) const {
  // FIXME: There must be a correct addrspace.
  Type *SPPtrTy = PointerType::getUnqual(V.getType());
  Value *SPPtr = IRB.CreateIntToPtr(&SP, SPPtrTy);

  IRB.CreateStore(&V, SPPtr);
}

void GenXPrologEpilogInsertion::readArgsFromReg(Function &F,
                                                const ArgsAnalyzer &Info,
                                                IRBuilder<> &IRB) const {
  for (const auto &ArgInfo : Info.in_reg()) {
    Argument *Arg = IGCLLVM::getArg(F, ArgInfo.ArgNo);
    readFromRegAndReplace(*Arg, ArgInfo.Offset, IRB,
                          PreDefined_Vars::PREDEFINED_ARG, ArgRegByteSize);
  }
}

void GenXPrologEpilogInsertion::readFromRegAndReplace(Value &V,
                                                      unsigned OffsetInReg,
                                                      IRBuilder<> &IRB,
                                                      PreDefined_Vars RegID,
                                                      unsigned RegSize) const {
  Type *Ty = V.getType();
  Value *Read = nullptr;
  if (Ty->isAggregateType())
    Read = readAggrFromReg(*Ty, OffsetInReg, IRB, RegID, RegSize);
  else
    Read = readNonAggrFromReg(*Ty, OffsetInReg, IRB, RegID, RegSize);
  V.replaceAllUsesWith(Read);
}

Value *GenXPrologEpilogInsertion::readNonAggrFromReg(Type &Ty,
                                                     unsigned OffsetInReg,
                                                     IRBuilder<> &IRB,
                                                     PreDefined_Vars RegID,
                                                     unsigned RegSize) const {
  IGC_ASSERT(!Ty.isAggregateType());
  Type *ToReadTy = &Ty;
  Type *ScalarTy = ToReadTy->getScalarType();

  if (ScalarTy->isIntegerTy(1)) {
    auto *VTy = cast<IGCLLVM::FixedVectorType>(ToReadTy);
    IGC_ASSERT(VTy->getNumElements() % genx::ByteBits == 0);
    ScalarTy = IRB.getInt8Ty();
    ToReadTy = IGCLLVM::FixedVectorType::get(ScalarTy, VTy->getNumElements() /
                                                           genx::ByteBits);
  }

  // Create a type for read.predef
  Type *RegTy = IGCLLVM::FixedVectorType::get(
      ScalarTy, RegSize / vc::getTypeSize(ScalarTy, DL).inBytes());

  unsigned NumElts = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(ToReadTy))
    NumElts = VT->getNumElements();

  bool CreateCopy = true;
  if (RegID == PreDefined_Vars::PREDEFINED_ARG && !ArgRegUsed)
    CreateCopy = false;
  Value *Read =
      buildReadPredefReg(RegID, IRB, RegTy, CreateCopy, !ToReadTy->isVectorTy(),
                         OffsetInReg, NumElts);

  if (ToReadTy != &Ty)
    Read = IRB.CreateBitCast(Read, &Ty);
  return Read;
}

Value *GenXPrologEpilogInsertion::readAggrFromReg(Type &Ty,
                                                  unsigned OffsetInReg,
                                                  IRBuilder<> &IRB,
                                                  PreDefined_Vars RegID,
                                                  unsigned RegSize) const {
  IGC_ASSERT(Ty.isAggregateType());

  unsigned AggrNumElts = IndexFlattener::getNumElements(&Ty);
  unsigned OffsetInAggr = 0;
  Value *AggrRes = UndefValue::get(&Ty);
  for (unsigned i = 0; i < AggrNumElts; ++i) {
    Type *EltTy = IndexFlattener::getElementType(&Ty, i);
    auto EltTySize = vc::getTypeSize(EltTy, DL);

    // Reading each aggregate element as a vector of bytes.
    Type *EltTyAsBytes =
        IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(), EltTySize.inBytes());
    Value *Read = readNonAggrFromReg(*EltTyAsBytes, OffsetInReg + OffsetInAggr,
                                     IRB, RegID, RegSize);
    if (EltTy->isPointerTy()) {
      Value *IntP = IRB.CreateBitCast(Read, IRB.getIntNTy(EltTySize.inBits()));
      Read = IRB.CreateIntToPtr(IntP, EltTy);
    } else
      Read = IRB.CreateBitCast(Read, EltTy);
    OffsetInAggr += EltTySize.inBytes();

    auto Indices = IndexFlattener::unflatten(&Ty, i);

    AggrRes = IRB.CreateInsertValue(AggrRes, Read, Indices);
  }

  return AggrRes;
}

void GenXPrologEpilogInsertion::passArgsInReg(CallInst &CI,
                                              const ArgsAnalyzer &Info,
                                              IRBuilder<> &IRB) const {
  for (const auto &ArgInfo : Info.in_reg()) {
    Value *Op = CI.getArgOperand(ArgInfo.ArgNo);
    passInReg(*Op, ArgInfo.Offset, IRB, PreDefined_Vars::PREDEFINED_ARG,
              ArgRegByteSize);
  }
}

void GenXPrologEpilogInsertion::passInReg(Value &Op, unsigned OffsetInReg,
                                          IRBuilder<> &IRB,
                                          PreDefined_Vars RegID,
                                          unsigned RegSize) const {
  Type *OpTy = Op.getType();
  if (OpTy->isAggregateType())
    passAggrInReg(Op, OffsetInReg, IRB, RegID, RegSize);
  else
    passNonAggrInReg(Op, OffsetInReg, IRB, RegID, RegSize);
}

void GenXPrologEpilogInsertion::passNonAggrInReg(Value &Op,
                                                 unsigned OffsetInReg,
                                                 IRBuilder<> &IRB,
                                                 PreDefined_Vars RegID,
                                                 unsigned RegSize) const {
  Value *ToWrite = &Op;
  Type *ToWriteTy = ToWrite->getType();
  IGC_ASSERT(!ToWriteTy->isAggregateType());
  Type *ScalarTy = ToWriteTy->getScalarType();
  if (ScalarTy->isIntegerTy(1)) {
    auto *VOpTy = cast<IGCLLVM::FixedVectorType>(ToWriteTy);
    IGC_ASSERT(VOpTy->getNumElements() % genx::ByteBits == 0);
    ScalarTy = IRB.getInt8Ty();
    ToWriteTy = IGCLLVM::FixedVectorType::get(
        ScalarTy, VOpTy->getNumElements() / genx::ByteBits);
    ToWrite = IRB.CreateBitCast(ToWrite, ToWriteTy);
  }

  // Each arg is copied to ARG in the way described below. It also contains a
  // small example for %arg2.
  //    %arg2 = shl ...
  //    call void @f(i32 %arg1, i32 %arg2)
  //
  // 1. Read current ARG as original arg will only partially override its
  // content. The scalar type or the read.predef is chosen to be the same as
  // the original arg has (i32 for %arg2). This simplify the future steps.
  //    %17 = call <256 x i32> @llvm.genx.read.predef.reg.v256i32.v256i32(i32
  //    8, <256 x i32> undef)
  //
  // 2. Copy arg to the correct place in the read ARG (Example: copy i32 %arg2
  // with offset = 32, because of alignment requirements).
  //    %18 = call <256 x i32> @llvm.genx.wrregioni.v256i32.i32.i16.i1(<256 x
  //    i32> %17, i32 %arg2, i32 0, i32 1, i32 1, i16 32, i32 undef, i1 true)
  //
  // 3. Save the updated ARG
  //    %19 = call i32 @llvm.genx.write.predef.reg.i32.v256i32(i32 8, <256 x
  //    i32> %18)

  // Create a type for read.predef
  Type *RegTy = IGCLLVM::FixedVectorType::get(
      ScalarTy, RegSize / vc::getTypeSize(ScalarTy, DL).inBytes());
  // Read ARG
  Instruction *Reg = buildReadPredefRegNoRegion(RegID, IRB, RegTy);
  // Write the arg to ARG
  buildWritePredefReg(RegID, IRB, ToWrite, Reg, OffsetInReg);
}

void GenXPrologEpilogInsertion::passAggrInReg(Value &Op, unsigned OffsetInReg,
                                              IRBuilder<> &IRB,
                                              PreDefined_Vars RegID,
                                              unsigned RegSize) const {
  IGC_ASSERT(Op.getType()->isAggregateType());

  unsigned AggrNumElts = IndexFlattener::getNumElements(Op.getType());
  unsigned OffsetInAggr = 0;
  for (unsigned i = 0; i < AggrNumElts; ++i) {
    auto Indices = IndexFlattener::unflatten(Op.getType(), i);

    Value *Elt = IRB.CreateExtractValue(&Op, Indices);
    IGC_ASSERT_MESSAGE(!Elt->getType()->isAggregateType(),
                       "Stack call argument was not flattened correctly");

    // Passing each aggregate element as a vector of bytes.
    auto EltTySize = vc::getTypeSize(Elt->getType(), DL);
    Type *EltTyAsBytes =
        IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(), EltTySize.inBytes());
    if (Elt->getType()->isPointerTy())
      Elt = IRB.CreatePtrToInt(Elt, IRB.getIntNTy(EltTySize.inBits()));
    Elt = IRB.CreateBitCast(Elt, EltTyAsBytes);

    passNonAggrInReg(*Elt, OffsetInReg + OffsetInAggr, IRB, RegID, RegSize);

    OffsetInAggr += EltTySize.inBytes();
  }
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty, bool BuildTempVal,
    bool AllowScalar, unsigned Offset, unsigned Width) const {
  return buildReadPredefReg(RegID, IRB, Ty, UndefValue::get(Ty), BuildTempVal,
                            AllowScalar, Offset, Width);
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty, Value *Dep,
    bool BuildTempVal, bool AllowScalar, unsigned Offset,
    unsigned Width) const {
  auto *Result = buildReadPredefRegNoRegion(RegID, IRB, Ty, Dep);

  Region R(Result);
  R.Offset = Offset;
  if (Width) {
    R.NumElements = Width;
    R.Width = Width;
  }
  Result = R.createRdRegion(Result, "", &*IRB.GetInsertPoint(),
                            IRB.getCurrentDebugLocation(),
                            AllowScalar && !BuildTempVal);
  if (BuildTempVal) {
    R.Offset = 0;
    Result =
        R.createWrRegion(UndefValue::get(Result->getType()), Result, "",
                         &*IRB.GetInsertPoint(), IRB.getCurrentDebugLocation());
    if (Width == 1 && AllowScalar)
      Result = R.createRdRegion(Result, "", &*IRB.GetInsertPoint(),
                                IRB.getCurrentDebugLocation(), AllowScalar);
  }

  return Result;
}
Instruction *GenXPrologEpilogInsertion::buildReadPredefRegNoRegion(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty) const {
  return buildReadPredefRegNoRegion(RegID, IRB, Ty, UndefValue::get(Ty));
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefRegNoRegion(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty, Value *Dep) const {
  Function *RegReadIntr = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertPoint()->getModule(),
      llvm::GenXIntrinsic::genx_read_predef_reg,
      {(isa<VectorType>(Ty) ? Ty : IGCLLVM::FixedVectorType::get(Ty, 1)),
       Dep->getType()});
  auto *RegRead = IRB.CreateCall(RegReadIntr, {IRB.getInt32(RegID), Dep});
  return RegRead;
}

Instruction *GenXPrologEpilogInsertion::buildWritePredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Value *Input) const {
  return buildWritePredefReg(RegID, IRB, Input,
                             UndefValue::get(Input->getType()));
}

Instruction *GenXPrologEpilogInsertion::buildWritePredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Value *Input, Value *Dep,
    unsigned Offset) const {
  Region RWrite(Input, DL);
  RWrite.Offset = Offset;
  auto *Wrr =
      RWrite.createWrRegion(Dep, Input, "", &*IRB.GetInsertPoint(), DebugLoc());
  Function *RegWriteIntr = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertPoint()->getModule(),
      llvm::GenXIntrinsic::genx_write_predef_reg,
      {Input->getType(), Wrr->getType()});
  auto *RegWrite = IRB.CreateCall(RegWriteIntr, {IRB.getInt32(RegID), Wrr});
  return RegWrite;
}

alignment_t
GenXPrologEpilogInsertion::getAllocaAlignment(AllocaInst *AI) const {
  auto Align = IGCLLVM::getAlignmentValue(AI);
  if (Align == 0)
    Align = DL->getPrefTypeAlign(AI->getAllocatedType()).value();
  return Align;
}
} // namespace
