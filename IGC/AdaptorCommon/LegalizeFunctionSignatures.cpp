/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LegalizeFunctionSignatures.h"
#include "common/debug/Debug.hpp"
#include "common/Types.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-legalize-function-signatures"
#define PASS_DESCRIPTION "Legalize calls to functions/subroutines and their signatures"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LegalizeFunctionSignaturesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LegalizeFunctionSignaturesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LegalizeFunctionSignaturesLPM::ID = 0;

LegalizeFunctionSignaturesLPM::LegalizeFunctionSignaturesLPM() : ModulePass(ID) {
  initializeLegalizeFunctionSignaturesLPMPass(*PassRegistry::getPassRegistry());
}

//*********************** PASS DESCRIPTION ***********************//

// This pass transforms functions and their callers to match the IGC function call ABI.
//
// classifySignature() decides how each argument and the return value are lowered. The decision
// made for a function drives its new declaration, its new body and all direct calls to it.
//
// Arguments:
// 1. Int and int vector arguments of illegal widths are widened to i8, i16, i32 or i64.
// 2. "byval" struct pointers are passed as struct values, in registers, if the struct is
//    promotable: only scalar members, and at most 512 bits for subroutines or 128 bits for
//    stackcalls. Clang-based frontends pass all struct arguments as "byval" pointers.
// 3. Struct values, which SPIR-V allows, are passed as "byval" pointers if a "byval" pointer to
//    the same struct would not be promoted, so that both forms are passed the same way. Arrays
//    are always passed as "byval" pointers.
//
// Return values:
// 4. Int and int vector return values of illegal widths are widened the same way.
// 5. Promotable "sret" struct pointers are returned as struct values.
// 6. Values codegen can't return in registers are returned through an "sret" pointer: arrays,
//    structs with aggregate members, and for stackcalls structs that are not promotable.
// 7. Structs of up to 128 bits with only scalar members returned by value keep the signature,
//    but returns are replaced by member-wise loads from the first alloca of the returned type in
//    the entry block, if there is one.
//
// See IGC StackCall ABI for details on stackcall calling conventions.

//****************************************************************//

static constexpr unsigned MaxStructSizeInBits = 128;
static constexpr unsigned MaxSubroutineStructSizeInBits = 512;

bool LegalizeFunctionSignatures::run(Module &M, IGC::IGCMD::MetaDataUtils *MDUtils, IGC::CodeGenContext *Ctx) {
  this->MDUtils = MDUtils;
  this->Ctx = Ctx;

  fixFunctionSignatures(M);
  fixFunctionUsers(M);
  fixFunctionBodies(M);

  MDUtils->save(M.getContext());
  return true;
}

// Returns false for ints and int vectors whose element width is not 8, 16, 32 or 64 bits.
static bool isLegalIntVectorType(const Module &M, Type *Ty) {
  if (!Ty->isIntOrIntVectorTy())
    return true;
  switch (M.getDataLayout().getTypeSizeInBits(Ty->getScalarType())) {
  case 8:
  case 16:
  case 32:
  case 64:
    return true;
  default:
    return false;
  }
}

static Type *getLegalIntVectorType(const Module &M, Type *Ty) {
  IGC_ASSERT(Ty && Ty->isIntOrIntVectorTy());

  unsigned Size = (unsigned)M.getDataLayout().getTypeSizeInBits(Ty->getScalarType());
  unsigned NewSize = 0;
  if (Size <= 8)
    NewSize = 8;
  else if (Size <= 16)
    NewSize = 16;
  else if (Size <= 32)
    NewSize = 32;
  else if (Size <= 64)
    NewSize = 64;
  else
    IGC_ASSERT_MESSAGE(0, "Currently don't support upscaling int sizes > 64 bits");

  Type *NewIntTy = IntegerType::get(M.getContext(), NewSize);
  if (Ty->isIntegerTy())
    return NewIntTy;
  return IGCLLVM::FixedVectorType::get(NewIntTy, (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements());
}

// A struct of at most MaxSizeInBits whose members are all scalars, no vectors or aggregates.
static bool isFlatStruct(const Module &M, Type *Ty, unsigned MaxSizeInBits) {
  auto *STy = dyn_cast<StructType>(Ty);
  return STy && M.getDataLayout().getStructLayout(STy)->getSizeInBits() <= MaxSizeInBits &&
         all_of(STy->elements(), [](Type *EltTy) { return EltTy->isSingleValueType() && !EltTy->isVectorTy(); });
}

// A struct passed in registers: as an argument, by value or as a "byval" pointer, or as a promoted
// "sret" return value.
static bool isPromotableStructType(const Module &M, Type *Ty, bool IsStackCall) {
  unsigned MaxSizeInBits = IsStackCall ? MaxStructSizeInBits : MaxSubroutineStructSizeInBits;
  return IGC_IS_FLAG_ENABLED(EnableByValStructArgPromotion) && isFlatStruct(M, Ty, MaxSizeInBits);
}

static bool canReturnInRegisters(const Module &M, Type *Ty, bool IsStackCall) {
  if (Ty->isArrayTy())
    return false;
  auto *STy = dyn_cast<StructType>(Ty);
  if (!STy)
    return true;
  if (IsStackCall)
    return isFlatStruct(M, STy, MaxStructSizeInBits);
  // Codegen does not support struct values with aggregate members.
  return none_of(STy->elements(), [](Type *EltTy) { return EltTy->isAggregateType(); });
}

static ArgLowering classifyArg(const Module &M, Type *Ty, AttributeSet Attrs, bool IsStackCall) {
  // Structs are passed in registers if they are promotable and in memory otherwise, the same way
  // whether they are passed as "byval" pointers or by value.
  if (Type *ByValTy = Ty->isPointerTy() ? Attrs.getByValType() : nullptr)
    return isPromotableStructType(M, ByValTy, IsStackCall) ? ArgLowering::ByValToValue : ArgLowering::Keep;
  if (Ty->isAggregateType())
    return isPromotableStructType(M, Ty, IsStackCall) ? ArgLowering::Keep : ArgLowering::ValueToByVal;
  if (!isLegalIntVectorType(M, Ty))
    return ArgLowering::WidenInt;
  return ArgLowering::Keep;
}

static RetLowering classifyRet(const Module &M, Type *RetTy, Type *SRetTy, bool IsStackCall,
                               bool CanReturnStructValue) {
  if (RetTy->isVoidTy() && SRetTy && isPromotableStructType(M, SRetTy, IsStackCall))
    return RetLowering::SRetToValue;
  if (CanReturnStructValue && isFlatStruct(M, RetTy, MaxStructSizeInBits))
    return RetLowering::StructValue;
  if (!canReturnInRegisters(M, RetTy, IsStackCall))
    return RetLowering::ValueToSRet;
  if (!isLegalIntVectorType(M, RetTy))
    return RetLowering::WidenInt;
  return RetLowering::Keep;
}

// The only place that decides how a signature is lowered. The types and attributes are the callee's,
// or the call's for indirect calls.
static SignatureLowering classifySignature(const Module &M, Type *RetTy, ArrayRef<Type *> ParamTys,
                                           const AttributeList &Attrs, bool IsStackCall, bool CanReturnStructValue) {
  SignatureLowering L;
  Type *SRetTy = !ParamTys.empty() && ParamTys[0]->isPointerTy() ? Attrs.getParamStructRetType(0) : nullptr;
  L.Ret = classifyRet(M, RetTy, SRetTy, IsStackCall, CanReturnStructValue);

  SmallVector<Type *, 8> LoweredParamTys;
  if (L.Ret == RetLowering::ValueToSRet)
    LoweredParamTys.push_back(IGCLLVM::PointerType::get(RetTy, 0));

  for (unsigned ArgNo = 0; ArgNo < ParamTys.size(); ++ArgNo) {
    Type *Ty = ParamTys[ArgNo];
    ArgLowering A = (ArgNo == 0 && L.Ret == RetLowering::SRetToValue)
                        ? ArgLowering::SRet
                        : classifyArg(M, Ty, Attrs.getParamAttrs(ArgNo), IsStackCall);
    L.Args.push_back(A);

    switch (A) {
    case ArgLowering::Keep:
      LoweredParamTys.push_back(Ty);
      break;
    case ArgLowering::WidenInt:
      LoweredParamTys.push_back(getLegalIntVectorType(M, Ty));
      break;
    case ArgLowering::ByValToValue:
      LoweredParamTys.push_back(Attrs.getParamByValType(ArgNo));
      break;
    case ArgLowering::ValueToByVal:
      LoweredParamTys.push_back(IGCLLVM::PointerType::get(Ty, 0));
      break;
    case ArgLowering::SRet:
      break;
    }
  }

  Type *LoweredRetTy = RetTy;
  if (L.Ret == RetLowering::ValueToSRet)
    LoweredRetTy = Type::getVoidTy(M.getContext());
  else if (L.Ret == RetLowering::SRetToValue)
    LoweredRetTy = SRetTy;
  else if (L.Ret == RetLowering::WidenInt)
    LoweredRetTy = getLegalIntVectorType(M, RetTy);
  L.LoweredTy = FunctionType::get(LoweredRetTy, LoweredParamTys, false);
  return L;
}

// Codegen doesn't handle struct loads and stores, so struct values are stored and loaded member by
// member.
static void storeToStruct(IGCLLVM::IRBuilder<> &Builder, Value *StructVal, Value *StructPtr) {
  IGC_ASSERT(StructPtr->getType()->isPointerTy());
  auto *STy = cast<StructType>(StructVal->getType());
  for (unsigned I = 0; I < STy->getNumElements(); ++I) {
    Value *Indices[] = {Builder.getInt32(0), Builder.getInt32(I)};
    Value *ElementPtr = Builder.CreateInBoundsGEP(STy, StructPtr, Indices);
    Builder.CreateStore(Builder.CreateExtractValue(StructVal, I), ElementPtr);
  }
}

static Value *loadFromStruct(IGCLLVM::IRBuilder<> &Builder, Value *StructPtr, Type *Ty) {
  IGC_ASSERT(StructPtr->getType()->isPointerTy());
  auto *STy = cast<StructType>(Ty);
  Value *StructVal = UndefValue::get(STy);
  for (unsigned I = 0; I < STy->getNumElements(); ++I) {
    Value *Indices[] = {Builder.getInt32(0), Builder.getInt32(I)};
    Value *ElementPtr = Builder.CreateInBoundsGEP(STy, StructPtr, Indices);
    Value *Element = Builder.CreateLoad(STy->getElementType(I), ElementPtr);
    StructVal = Builder.CreateInsertValue(StructVal, Element, I);
  }
  return StructVal;
}

void LegalizeFunctionSignatures::fixFunctionSignatures(Module &M) {
  for (Function &F : M) {
    if (isEntryFunc(MDUtils, &F))
      continue;

    // An internally-linked function that eventually gets inlined doesn't need this transformation.
    if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) && F.hasInternalLinkage())
      continue;

    // Empty function names can cause funny behavior later on. If duplicates, LLVM will insert a
    // unique tag.
    if (F.getName().empty())
      F.setName("__function__");

    // For binary linking, calling a function outside the module is possible, so declaration
    // signatures have to be fixed as well.
    if (F.isDeclaration() && !F.hasFnAttribute("referenced-indirectly") && !F.hasFnAttribute("invoke_simd_target"))
      continue;

    FunctionType *FTy = F.getFunctionType();
    SignatureLowering L = classifySignature(M, FTy->getReturnType(), FTy->params(), F.getAttributes(),
                                            F.hasFnAttribute("visaStackCall"), !F.isDeclaration());
    if (!L.isNeeded())
      continue;

    if (F.isDeclaration()) {
      // fixFunctionBodies() doesn't process declarations, so drop the "byval" attributes of promoted
      // arguments here.
      for (Argument &Arg : F.args()) {
        if (L.Args[Arg.getArgNo()] == ArgLowering::ByValToValue) {
          Arg.removeAttr(llvm::Attribute::ByVal);
          Arg.removeAttr(llvm::Attribute::Alignment);
        }
      }
    }

    Function *NewF = Function::Create(L.LoweredTy, F.getLinkage(), F.getName(), F.getParent());
    NewF->takeName(&F);
    NewF->setCallingConv(F.getCallingConv());
    copyAttributesAndAdjustForSkippedFunctionArgs(&F, NewF, L.Ret == RetLowering::SRetToValue);

    if (L.Ret == RetLowering::ValueToSRet) {
      // The new function writes the return value through the new pointer argument, and its callers
      // allocate it.
      IGCLLVM::MemoryEffects ME(IGCLLVM::ModRefInfo::ModRef);
      IGCLLVM::setMemoryEffects(*NewF, ME);
      Ctx->m_instrTypes.hasNonPrimitiveAlloca = true;
    }

    OldToNewFuncMap[&F] = {NewF, std::move(L)};
  }
}

void LegalizeFunctionSignatures::copyAttributesAndAdjustForSkippedFunctionArgs(Function *F, Function *NewF,
                                                                               bool HasPromotedSRetArg) {
  AttributeList OldAttrs = F->getAttributes();
  SmallVector<AttributeSet, 4> NewArgAttrs(F->arg_size());
  bool SkipFirstArg = HasPromotedSRetArg && F->hasParamAttribute(0, llvm::Attribute::StructRet);
  for (Argument &Arg : F->args()) {
    unsigned ArgNo = Arg.getArgNo();
    if (SkipFirstArg && ArgNo == 0)
      continue;
    NewArgAttrs[SkipFirstArg ? ArgNo - 1 : ArgNo] = OldAttrs.getParamAttrs(ArgNo);
  }
  NewF->setAttributes(AttributeList::get(F->getContext(), OldAttrs.getFnAttrs(), OldAttrs.getRetAttrs(), NewArgAttrs));
}

void LegalizeFunctionSignatures::fixFunctionBodies(Module &M) {
  for (const auto &[F, Lowered] : OldToNewFuncMap) {
    Function *NewF = Lowered.NewFunc;
    const SignatureLowering &L = Lowered.Lowering;

    if (!F->isDeclaration()) {
      const DataLayout &DL = M.getDataLayout();
      ValueToValueMapTy VMap;
      SmallVector<ReturnInst *, 8> Returns;
      auto NewArgIt = NewF->arg_begin();
      Value *SRetPtr = nullptr;
      Type *SRetTy = nullptr;

      // Skip the "sret" argument added for the return value.
      if (L.Ret == RetLowering::ValueToSRet)
        ++NewArgIt;

      BasicBlock *EntryBB = BasicBlock::Create(M.getContext(), "", NewF);
      IGCLLVM::IRBuilder<> Builder(EntryBB);
      for (Argument &OldArg : F->args()) {
        ArgLowering A = L.Args[OldArg.getArgNo()];
        if (A == ArgLowering::SRet) {
          // Map the old "sret" pointer to a temporary alloca, removed later by SROA.
          SRetTy = L.LoweredTy->getReturnType();
          SRetPtr = Builder.CreateAddrSpaceCast(Builder.CreateAlloca(SRetTy), OldArg.getType());
          VMap[&OldArg] = SRetPtr;
          continue;
        }
        NewArgIt->setName(OldArg.getName());
        switch (A) {
        case ArgLowering::WidenInt:
          VMap[&OldArg] = Builder.CreateTrunc(&*NewArgIt, OldArg.getType());
          break;
        case ArgLowering::ByValToValue: {
          AllocaInst *NewArgPtr = Builder.CreateAlloca(NewArgIt->getType());
          NewArgIt->removeAttr(llvm::Attribute::ByVal);
          storeToStruct(Builder, &*NewArgIt, NewArgPtr);
          IGC_ASSERT(OldArg.getType()->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC ||
                     OldArg.getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE);
          VMap[&OldArg] = Builder.CreateAddrSpaceCast(NewArgPtr, OldArg.getType());
          break;
        }
        case ArgLowering::ValueToByVal:
          VMap[&OldArg] = Builder.CreateLoad(OldArg.getType(), &*NewArgIt);
          NewArgIt->addAttr(llvm::Attribute::getWithByValType(M.getContext(), OldArg.getType()));
          break;
        case ArgLowering::Keep:
          VMap[&OldArg] = &*NewArgIt;
          break;
        case ArgLowering::SRet:
          IGC_ASSERT_UNREACHABLE();
        }
        ++NewArgIt;
      }

      IGCLLVM::CloneFunctionInto(NewF, F, VMap, IGCLLVM::CloneFunctionChangeType::GlobalChanges, Returns);

      // Merge the block with the argument fixups into the cloned entry block.
      auto *ClonedEntryBB = cast<BasicBlock>(VMap[&F->getEntryBlock()]);
      Builder.CreateBr(ClonedEntryBB);
      MergeBlockIntoPredecessor(ClonedEntryBB);

      switch (L.Ret) {
      case RetLowering::ValueToSRet: {
        Argument *RetArg = NewF->arg_begin();
        RetArg->addAttr(llvm::Attribute::NoAlias);
        RetArg->addAttr(llvm::Attribute::getWithStructRetType(M.getContext(), F->getReturnType()));
        for (ReturnInst *RI : Returns) {
          IGCLLVM::IRBuilder<> RetBuilder(RI);
          Type *RetTy = RI->getReturnValue()->getType();
          Value *RetValPtr = RetBuilder.CreateAlloca(RetTy);
          RetBuilder.CreateStore(RI->getReturnValue(), RetValPtr);
          RetBuilder.CreateMemCpy(RetArg, RetValPtr, DL.getTypeAllocSize(RetTy), DL.getPointerSize());
          RetBuilder.CreateRetVoid();
          RI->eraseFromParent();
        }
        break;
      }
      case RetLowering::SRetToValue:
        for (ReturnInst *RI : Returns) {
          IGCLLVM::IRBuilder<> RetBuilder(RI);
          RetBuilder.CreateRet(loadFromStruct(RetBuilder, SRetPtr, SRetTy));
          RI->eraseFromParent();
        }
        break;
      case RetLowering::StructValue:
        for (Instruction &I : NewF->getEntryBlock()) {
          auto *AI = dyn_cast<AllocaInst>(&I);
          if (AI && AI->getAllocatedType() == NewF->getReturnType()) {
            SRetPtr = AI;
            SRetTy = NewF->getReturnType();
            break;
          }
        }
        if (SRetPtr) {
          for (ReturnInst *RI : Returns) {
            IGCLLVM::IRBuilder<> RetBuilder(RI);
            RetBuilder.CreateRet(loadFromStruct(RetBuilder, SRetPtr, SRetTy));
            RI->eraseFromParent();
          }
        }
        break;
      case RetLowering::WidenInt:
        for (ReturnInst *RI : Returns) {
          IGCLLVM::IRBuilder<> RetBuilder(RI);
          Value *RetVal = RI->getReturnValue();
          RetBuilder.CreateRet(RetBuilder.CreateZExt(RetVal, getLegalIntVectorType(M, RetVal->getType())));
          RI->eraseFromParent();
        }
        break;
      case RetLowering::Keep:
        break;
      }
    }

    F->removeDeadConstantUsers();
    // The old function may still be referenced by metadata. All other uses are gone, so replace
    // them with poison and delete the function.
    F->replaceAllUsesWith(PoisonValue::get(F->getType()));
    F->eraseFromParent();
  }
}

void LegalizeFunctionSignatures::fixFunctionUsers(Module &M) {
  std::vector<CallInst *> CallsToFix;

  for (const auto &[F, Lowered] : OldToNewFuncMap) {
    std::vector<User *> Users(F->user_begin(), F->user_end());
    for (User *U : Users) {
      auto *CI = dyn_cast<CallInst>(U);
      if (CI && CI->getCalledFunction() == F) {
        CallsToFix.push_back(CI);
      } else if (auto *I = dyn_cast<Instruction>(U)) {
        // Other uses, e.g. taking the address, see the old function type.
        IGCLLVM::IRBuilder<> Builder(I);
        I->replaceUsesOfWith(F, Builder.CreatePointerCast(Lowered.NewFunc, F->getType()));
      }
    }
  }

  for (Function &F : M) {
    for (Instruction &I : instructions(F)) {
      auto *CI = dyn_cast<CallInst>(&I);
      if (CI && !CI->isInlineAsm() && !CI->getCalledFunction())
        CallsToFix.push_back(CI);
    }
  }

  for (CallInst *CI : CallsToFix)
    fixCallInstruction(M, CI);
}

void LegalizeFunctionSignatures::fixCallInstruction(Module &M, CallInst *CI) {
  Function *Callee = CI->getCalledFunction();
  AttributeList Attrs = CI->getAttributes();

  // Direct calls follow the lowering of the callee. Indirect calls are always stackcalls, and are
  // classified by their operands, so that variadic operands are lowered too.
  SignatureLowering IndirectL;
  if (!Callee) {
    SmallVector<Type *, 8> OperandTys;
    for (Value *Arg : CI->args())
      OperandTys.push_back(Arg->getType());
    IndirectL = classifySignature(M, CI->getType(), OperandTys, Attrs, /*IsStackCall=*/true,
                                  /*CanReturnStructValue=*/false);
  }
  IGC_ASSERT(!Callee || OldToNewFuncMap.count(Callee));
  const SignatureLowering &L = Callee ? OldToNewFuncMap[Callee].Lowering : IndirectL;
  if (!L.isNeeded())
    return;
  IGC_ASSERT(L.Args.size() == IGCLLVM::getNumArgOperands(CI));

  SmallVector<Value *, 16> NewArgs;
  SmallVector<AttributeSet, 8> NewArgAttrs;
  IGCLLVM::IRBuilder<> Builder(CI);

  Value *RetPtr = nullptr;
  if (L.Ret == RetLowering::ValueToSRet) {
    RetPtr = Builder.CreateAlloca(CI->getType());
    NewArgs.push_back(RetPtr);
    AttrBuilder SRetAttrs(M.getContext());
    SRetAttrs.addAttribute(llvm::Attribute::NoAlias);
    SRetAttrs.addStructRetAttr(CI->getType());
    NewArgAttrs.push_back(AttributeSet::get(M.getContext(), SRetAttrs));
  }

  for (unsigned ArgNo = 0; ArgNo < IGCLLVM::getNumArgOperands(CI); ++ArgNo) {
    Value *Arg = CI->getArgOperand(ArgNo);
    switch (L.Args[ArgNo]) {
    case ArgLowering::SRet:
      break;
    case ArgLowering::WidenInt:
      NewArgs.push_back(Builder.CreateZExt(Arg, getLegalIntVectorType(M, Arg->getType())));
      NewArgAttrs.push_back(AttributeSet());
      break;
    case ArgLowering::ByValToValue:
      NewArgs.push_back(loadFromStruct(Builder, Arg, L.LoweredTy->getParamType(NewArgs.size())));
      NewArgAttrs.push_back(AttributeSet());
      break;
    case ArgLowering::ValueToByVal: {
      Value *ArgPtr = Builder.CreateAlloca(Arg->getType());
      Builder.CreateStore(Arg, ArgPtr);
      NewArgs.push_back(ArgPtr);
      auto ByValAttr = llvm::Attribute::getWithByValType(M.getContext(), Arg->getType());
      NewArgAttrs.push_back(AttributeSet::get(M.getContext(), {ByValAttr}));
      break;
    }
    case ArgLowering::Keep:
      NewArgs.push_back(Arg);
      NewArgAttrs.push_back(Attrs.getParamAttrs(ArgNo));
      break;
    }
  }

  Value *NewCallee =
      Callee ? static_cast<Value *>(OldToNewFuncMap[Callee].NewFunc)
             : Builder.CreatePointerCast(IGCLLVM::getCalledValue(CI), IGCLLVM::PointerType::get(L.LoweredTy, 0));
  CallInst *NewCI = Builder.CreateCall(L.LoweredTy, NewCallee, NewArgs);
  NewCI->setCallingConv(CI->getCallingConv());
  NewCI->setAttributes(AttributeList::get(M.getContext(), Attrs.getFnAttrs(), Attrs.getRetAttrs(), NewArgAttrs));
  NewCI->setDebugLoc(CI->getDebugLoc());

  switch (L.Ret) {
  case RetLowering::ValueToSRet:
    CI->replaceAllUsesWith(Builder.CreateLoad(CI->getType(), RetPtr));
    break;
  case RetLowering::SRetToValue:
    storeToStruct(Builder, NewCI, CI->getArgOperand(0));
    break;
  case RetLowering::WidenInt:
    CI->replaceAllUsesWith(Builder.CreateTrunc(NewCI, CI->getType()));
    break;
  case RetLowering::StructValue:
  case RetLowering::Keep:
    CI->replaceAllUsesWith(NewCI);
    break;
  }
  CI->eraseFromParent();
}

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses LegalizeFunctionSignaturesNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<IGC::MetaDataUtilsAnalysis>(M);
  bool Changed = Impl.run(M, MDU.MdUtils, AM.getResult<IGC::CodeGenContextAnalysis>(M).Ctx);
  return Changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
