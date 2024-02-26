/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXTranslateSPIRVBuiltins
/// -----------
///
/// This pass translates SPIR-V builtin functions by replacing some of them with
/// intrinsic calls and linking a BiF module. The BiF module holds
/// implementation of those functions.
///
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/General/BiF.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "GENX_SPIRV_BUILTINS"

using namespace llvm;

class SPIRVExpander : public InstVisitor<SPIRVExpander, Value *> {
  friend InstVisitor<SPIRVExpander, Value *>;

public:
  explicit SPIRVExpander(Module *Mod) : M(Mod) {}

  Value *tryReplace(Instruction *I);

private:
  static constexpr double Ln2 = 0x1.62e42fefa39efp-1;
  static constexpr double Log10_2 = 0.3010299956639811952137388;
  static constexpr double Log2E = 0x1.71547652b82fep+0;
  static constexpr double Log2_10 = 3.3219280948873623478703194;

  Value *visitCallInst(CallInst &CI);
  Value *visitInstruction(Instruction &) { return nullptr; }

  CallInst *emitIntrinsic(IRBuilder<> &Builder, unsigned IID,
                          ArrayRef<Type *> Types, ArrayRef<Value *> Args);
  CallInst *emitIntrinsic(IRBuilder<> &Builder, unsigned IID, Type *Ty,
                          ArrayRef<Value *> Args) {
    SmallVector<Type *, 1> Types = {Ty};
    return emitIntrinsic(Builder, IID, Types, Args);
  }

  Value *emitMathIntrinsic(IRBuilder<> &Builder, unsigned IID, Type *Ty,
                           ArrayRef<Value *> Args, bool AFN = false);
  Value *emitFDiv(IRBuilder<> &Builder, Value *L, Value *R, bool ARCP = false);

  Module *M;
};

Value *SPIRVExpander::tryReplace(Instruction *I) {
  Value *R = visit(*I);
  if (!R)
    return nullptr;

  R->takeName(I);
  I->replaceAllUsesWith(R);

  return R;
}

CallInst *SPIRVExpander::emitIntrinsic(IRBuilder<> &Builder, unsigned IID,
                                       ArrayRef<Type *> Types,
                                       ArrayRef<Value *> Args) {
  IGC_ASSERT_EXIT(M);
  Function *IntrFunc = vc::getAnyDeclaration(M, IID, Types);
  return Builder.CreateCall(IntrFunc, Args);
}

Value *SPIRVExpander::emitMathIntrinsic(IRBuilder<> &Builder, unsigned IID,
                                        Type *Ty, ArrayRef<Value *> Args,
                                        bool AFN) {
  auto *NewCI = emitIntrinsic(Builder, IID, Ty, Args);
  NewCI->setHasApproxFunc(AFN);
  return NewCI;
}

Value *SPIRVExpander::emitFDiv(IRBuilder<> &Builder, Value *L, Value *R,
                               bool ARCP) {
  auto *FDiv = Builder.CreateFDiv(L, R);
  if (auto *FDivInst = dyn_cast<Instruction>(FDiv))
    FDivInst->setHasAllowReciprocal(ARCP);
  return FDiv;
}

Value *SPIRVExpander::visitCallInst(CallInst &CI) {
  if (CI.isInlineAsm())
    return nullptr;

  auto *Callee = CI.getCalledFunction();
  if (!Callee)
    return nullptr;

  IRBuilder<> Builder(&CI);
  auto *Ty = CI.getType();
  auto CalleeName = Callee->getName();

  // Check if it's a SPIR-V function
  const StringRef Prefix("__spirv_");
  const auto PrefixPos = CalleeName.find(Prefix);
  if (PrefixPos == StringRef::npos)
    return nullptr;

  CalleeName = CalleeName.drop_front(PrefixPos + Prefix.size());

  auto IID = StringSwitch<unsigned>(CalleeName)
                 .StartsWith("BitCount", Intrinsic::ctpop)
                 .Default(Intrinsic::not_intrinsic);

  if (IID != Intrinsic::not_intrinsic) {
    SmallVector<Value *, 3> Args(CI.args());
    return emitIntrinsic(Builder, IID, Ty, Args);
  }

  // Addrspace-related builtins.
  if (CalleeName.startswith("GenericCastToPtrExplicit"))
    return emitIntrinsic(Builder, vc::InternalIntrinsic::cast_to_ptr_explicit,
                         Ty, {CI.getArgOperand(0)});
  // SPV_INTEL_bfloat16_conversion extension.
  if (CalleeName.startswith("ConvertFToBF16INTEL")) {
    auto *Arg = CI.getArgOperand(0);
    auto *ArgTy = Arg->getType();
    return emitIntrinsic(Builder, vc::InternalIntrinsic::cast_to_bf16,
                         {Ty, ArgTy}, {Arg});
  }
  if (CalleeName.startswith("ConvertBF16ToFINTEL")) {
    auto *Arg = CI.getArgOperand(0);
    auto *ArgTy = Arg->getType();
    return emitIntrinsic(Builder, vc::InternalIntrinsic::cast_from_bf16,
                         {Ty, ArgTy}, {Arg});
  }
  // SPV_INTEL_tensor_float32_rounding extension.
  if (CalleeName.startswith("RoundFToTF32INTEL") ||
      CalleeName.startswith("ConvertFToTF32INTEL")) {
    auto *Arg = CI.getArgOperand(0);
    auto *ArgTy = Arg->getType();
    Type *ResTy = Builder.getInt32Ty();
    if (auto *ArgVTy = dyn_cast<IGCLLVM::FixedVectorType>(ArgTy))
      ResTy = IGCLLVM::FixedVectorType::get(ResTy, ArgVTy->getNumElements());

    auto *Intr = emitIntrinsic(Builder, vc::InternalIntrinsic::round_to_tf32,
                               {ResTy, ArgTy}, {Arg});
    return Builder.CreateBitCast(Intr, Ty);
  }
  // SPV_KHR_shader_clock extension.
  if (CalleeName.startswith("ReadClockKHR")) {
    auto *Intr =
        emitIntrinsic(Builder, Intrinsic::readcyclecounter, llvm::None, {});
    return Builder.CreateBitCast(Intr, CI.getType());
  }

  // OpenCL extended instruction set
  if (!CalleeName.consume_front("ocl_"))
    return nullptr;

  IID = StringSwitch<unsigned>(CalleeName)
            // Integer intrinsics
            .StartsWith("popcount", Intrinsic::ctpop)
            .StartsWith("s_abs", GenXIntrinsic::genx_absi)
            // Floating-point intrinsics
            .StartsWith("fabs", Intrinsic::fabs)
            .StartsWith("fma", Intrinsic::fma)
            .StartsWith("fmax", Intrinsic::maxnum)
            .StartsWith("fmin", Intrinsic::minnum)
            .StartsWith("mad", Intrinsic::fmuladd)
            .StartsWith("sqrt", Intrinsic::sqrt)
            .Default(Intrinsic::not_intrinsic);

  if (IID != Intrinsic::not_intrinsic) {
    SmallVector<Value *, 3> Args(CI.args());
    return emitIntrinsic(Builder, IID, Ty, Args);
  }

  IID = StringSwitch<unsigned>(CalleeName)
            .StartsWith("clz", Intrinsic::ctlz)
            .StartsWith("ctz", Intrinsic::cttz)
            .Default(Intrinsic::not_intrinsic);

  if (IID != Intrinsic::not_intrinsic) {
    SmallVector<Value *, 2> Args(CI.args());
    // is_zero_poison == 0
    Args.push_back(Constant::getNullValue(Builder.getInt1Ty()));
    return emitIntrinsic(Builder, IID, Ty, Args);
  }

  // Native subset
  if (!CalleeName.consume_front("native_") &&
      !CalleeName.consume_front("half_"))
    return nullptr;

  IID = StringSwitch<unsigned>(CalleeName)
            .StartsWith("cos", Intrinsic::cos)
            .StartsWith("exp2", Intrinsic::exp2)
            .StartsWith("log2", Intrinsic::log2)
            .StartsWith("powr", Intrinsic::pow)
            .StartsWith("sin", Intrinsic::sin)
            .StartsWith("sqrt", Intrinsic::sqrt)
            .Default(Intrinsic::not_intrinsic);

  if (IID != Intrinsic::not_intrinsic) {
    SmallVector<Value *, 2> Args(CI.args());
    return emitMathIntrinsic(Builder, IID, Ty, Args, true);
  }

  if (CalleeName.startswith("divide"))
    return emitFDiv(Builder, CI.getArgOperand(0), CI.getArgOperand(1), true);
  if (CalleeName.startswith("exp10")) {
    // exp10(x) == exp2(x * log2(10))
    auto *C = ConstantFP::get(Ty, Log2_10);
    auto *ArgV = Builder.CreateFMul(CI.getArgOperand(0), C);
    return emitMathIntrinsic(Builder, Intrinsic::exp2, Ty, {ArgV}, true);
  }
  if (CalleeName.startswith("exp")) {
    // exp(x) == exp2(x * log2(e))
    auto *C = ConstantFP::get(Ty, Log2E);
    auto *ArgV = Builder.CreateFMul(CI.getArgOperand(0), C);
    return emitMathIntrinsic(Builder, Intrinsic::exp2, Ty, {ArgV}, true);
  }
  if (CalleeName.startswith("log10")) {
    // log10(x) == log2(x) * log10(2)
    auto *LogV = emitMathIntrinsic(Builder, Intrinsic::log2, Ty,
                                   {CI.getArgOperand(0)}, true);
    auto *C = ConstantFP::get(Ty, Log10_2);
    return Builder.CreateFMul(LogV, C);
  }
  if (CalleeName.startswith("log")) {
    // ln(x) == log2(x) * ln(2)
    auto *LogV = emitMathIntrinsic(Builder, Intrinsic::log2, Ty,
                                   {CI.getArgOperand(0)}, true);
    auto *C = ConstantFP::get(Ty, Ln2);
    return Builder.CreateFMul(LogV, C);
  }
  if (CalleeName.startswith("recip")) {
    auto *OneC = ConstantFP::get(Ty, 1.0);
    return emitFDiv(Builder, OneC, CI.getArgOperand(0), true);
  }
  if (CalleeName.startswith("rsqrt")) {
    auto *OneC = ConstantFP::get(Ty, 1.0);
    auto *SqrtV = emitMathIntrinsic(Builder, Intrinsic::sqrt, Ty,
                                    {CI.getArgOperand(0)}, true);
    return emitFDiv(Builder, OneC, SqrtV, true);
  }
  if (CalleeName.startswith("tan")) {
    // tan(x) == sin(x) / cos(x)
    auto *ArgV = CI.getArgOperand(0);
    auto *SinV = emitMathIntrinsic(Builder, Intrinsic::sin, Ty, {ArgV}, true);
    auto *CosV = emitMathIntrinsic(Builder, Intrinsic::cos, Ty, {ArgV}, true);
    return emitFDiv(Builder, SinV, CosV, true);
  }

  return nullptr;
}

class GenXTranslateSPIRVBuiltins final : public ModulePass {
public:
  static char ID;
  GenXTranslateSPIRVBuiltins() : ModulePass(ID), Expander(nullptr) {}
  StringRef getPassName() const override {
    return "GenX translate SPIR-V builtins";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
  bool runOnFunction(Function &F);

private:
  std::unique_ptr<Module> getBiFModule(BiFKind Kind, LLVMContext &Ctx);
  SPIRVExpander Expander;
};

char GenXTranslateSPIRVBuiltins::ID = 0;

INITIALIZE_PASS_BEGIN(GenXTranslateSPIRVBuiltins, "GenXTranslateSPIRVBuiltins",
                      "GenXTranslateSPIRVBuiltins", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXTranslateSPIRVBuiltins, "GenXTranslateSPIRVBuiltins",
                    "GenXTranslateSPIRVBuiltins", false, false)

ModulePass *llvm::createGenXTranslateSPIRVBuiltinsPass() {
  initializeGenXTranslateSPIRVBuiltinsPass(*PassRegistry::getPassRegistry());
  return new GenXTranslateSPIRVBuiltins;
}

void GenXTranslateSPIRVBuiltins::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

// May have false positive, e.g. __spirv is in the middle of a function
// name. Having false positives is not that critical as they won't be linked
// anyway.
static bool isSPIRVBuiltinDecl(const Function &F) {
  auto Name = F.getName();
  // __devicelib_* functions may have implementations which VC should replace
  if (Name.startswith("__devicelib") || Name == "__assert_fail")
    return true;
  if (!F.isDeclaration())
    return false;
  if (auto IID = vc::getAnyIntrinsicID(&F); vc::isAnyNonTrivialIntrinsic(IID))
    return false;
  return Name.contains("__spirv");
}

bool GenXTranslateSPIRVBuiltins::runOnModule(Module &M) {
  bool Changed = false;
  Expander = SPIRVExpander(&M);
  for (auto &F : M.getFunctionList())
    Changed |= runOnFunction(F);

  // Collect SPIRV built-in functions to link.
  auto SPIRVBuiltins = vc::collectFunctionNamesIf(
      M, [](const Function &F) { return isSPIRVBuiltinDecl(F); });
  // Nothing to do if there are no spirv builtins.
  if (SPIRVBuiltins.empty())
    return Changed;

  std::unique_ptr<Module> SPIRVBuiltinsModule =
      getBiFModule(BiFKind::VCSPIRVBuiltins, M.getContext());
  SPIRVBuiltinsModule->setDataLayout(M.getDataLayout());
  SPIRVBuiltinsModule->setTargetTriple(M.getTargetTriple());

  // If the BiF module has the same function, we should select it
  for (auto &FuncName : SPIRVBuiltins) {
    auto *Func = M.getFunction(FuncName);
    auto *NewFunc = SPIRVBuiltinsModule->getFunction(FuncName);
    if (Func && !Func->isDeclaration() && NewFunc &&
        !NewFunc->isDeclaration() &&
        Func->getFunctionType() == NewFunc->getFunctionType())
      Func->deleteBody();
  }

  if (Linker::linkModules(M, std::move(SPIRVBuiltinsModule),
                          Linker::Flags::LinkOnlyNeeded)) {
    IGC_ASSERT_MESSAGE(0, "Error linking spirv implementation builtin module");
  }

  // If declaration appeared, then mark with internal linkage.
  vc::internalizeImportedFunctions(M, SPIRVBuiltins,
                                   /* SetAlwaysInline */ true);

  return true;
}

bool GenXTranslateSPIRVBuiltins::runOnFunction(Function &F) {
  std::vector<Instruction *> ToErase;

  for (auto &BB : F.getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end(); ++I) {
      Instruction *Inst = &*I;
      if (Expander.tryReplace(Inst))
        ToErase.push_back(Inst);
    }
  }

  for (auto *Inst : ToErase)
    Inst->eraseFromParent();

  return !ToErase.empty();
}

std::unique_ptr<Module>
GenXTranslateSPIRVBuiltins::getBiFModule(BiFKind Kind, LLVMContext &Ctx) {
  MemoryBufferRef BiFModuleBuffer =
      getAnalysis<GenXBackendConfig>().getBiFModule(Kind);
  return vc::getLazyBiFModuleOrReportError(BiFModuleBuffer, Ctx);
}
