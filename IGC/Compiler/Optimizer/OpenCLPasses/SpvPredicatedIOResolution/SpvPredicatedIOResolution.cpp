/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpvPredicatedIOResolution.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Regex.h"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;

char SpvPredicatedIOResolution::ID = 0;

#define PASS_FLAG "igc-spv-predicatedio-resolution"
#define PASS_DESC "Lowering of SPIR-V INTEL Predicated IO instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-predicatedio-resolution"

IGC_INITIALIZE_PASS_BEGIN(SpvPredicatedIOResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SpvPredicatedIOResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

SpvPredicatedIOResolution::SpvPredicatedIOResolution() : ModulePass(ID) {
  initializeSpvPredicatedIOResolutionPass(*PassRegistry::getPassRegistry());
}

bool SpvPredicatedIOResolution::runOnModule(Module &M) {
  m_BuiltinsToRemove.clear();
  m_InstructionsToErase.clear();
  m_Module = &M;
  m_Changed = false;
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  visit(M);

  for (auto *I : m_InstructionsToErase)
    I->eraseFromParent();
  m_InstructionsToErase.clear();

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();
  m_BuiltinsToRemove.clear();

  return m_Changed;
}

void SpvPredicatedIOResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  static const Regex patternPredicatedReadSPV("_Z[0-9]+__spirv_PredicatedLoadINTEL.+");
  static const Regex patternPredicatedWriteSPV("_Z[0-9]+__spirv_PredicatedStoreINTEL.+");
  StringRef FuncName = F->getName();

  if (patternPredicatedReadSPV.match(FuncName)) {
    visitPredicatedSPVCallInst<Load>(CI);
    return;
  }

  if (patternPredicatedWriteSPV.match(FuncName))
    visitPredicatedSPVCallInst<Store>(CI);
}

namespace PredicatedIO {
namespace Load {
enum Operator { Pointer = 0, Predicate = 1, DefaultValue = 2, MemOperand = 3 };
} // namespace Load

namespace Store {
enum Operator { Pointer = 0, Object = 1, Predicate = 2, MemOperand = 3 };
} // namespace Store
} // namespace PredicatedIO

enum {
  None = 0,
  Volatile = 0x1,
  Aligned = 0x2,
  Nontemporal = 0x4,
  AllSupportedFlags = Volatile | Aligned | Nontemporal
};

Value *SpvPredicatedIOResolution::getDefaultAlignValue(Type *Ty) const {
  auto Align = m_Module->getDataLayout().getABITypeAlign(Ty);
  return ConstantInt::get(Type::getInt64Ty(m_Module->getContext()), Align.value());
}

bool SpvPredicatedIOResolution::validateOperandType(const Value *V) const {
  return isa<ConstantInt>(V) && V->getType()->isIntegerTy(32);
}

template <SpvPredicatedIOResolution::Operation operation>
void SpvPredicatedIOResolution::visitPredicatedSPVCallInst(CallInst &CI) {
  using OpVariant =
      typename std::conditional_t<operation == Load, PredicatedIO::Load::Operator, PredicatedIO::Store::Operator>;
  Function *F = CI.getCalledFunction();
  IGC_ASSERT_MESSAGE(F, "Failed to get called function");
  int NumArgs = IGCLLVM::getNumArgOperands(&CI);
  IGC_ASSERT_MESSAGE(NumArgs >= 3, "Predicated IO SPV call must have at least 3 arguments");

  Value *Ptr = CI.getArgOperand(OpVariant::Pointer);
  Value *Predicate = CI.getArgOperand(OpVariant::Predicate);
  Value *AlignVal = nullptr;

  Type *PtrType = Ptr->getType();

  // process memory operands
  bool IsNontemporal = false;
  if (NumArgs >= 4) {
    Value *MemOperandVal = CI.getArgOperand(OpVariant::MemOperand);
    if (!validateOperandType(MemOperandVal)) {
      m_Ctx->EmitError((F->getName() + ": Invalid memory operand type. Expected i32 constant.").str().c_str(), &CI);
      return;
    }

    uint32_t MemOperands = cast<ConstantInt>(MemOperandVal)->getZExtValue();
    if (MemOperands & Volatile) {
      m_Ctx->EmitError((F->getName() + ": Volatile memory operand is not allowed.").str().c_str(), &CI);
      return;
    }

    if (MemOperands & Aligned) {
      if (NumArgs < 5) {
        m_Ctx->EmitError((F->getName() + ": Aligned memory operand requires an alignment value.").str().c_str(), &CI);
        return;
      }
      AlignVal = CI.getArgOperand(OpVariant::MemOperand + 1);
      if (!validateOperandType(AlignVal)) {
        m_Ctx->EmitError((F->getName() + ": Invalid alignment value type. Expected i32 constant.").str().c_str(), &CI);
        return;
      }
      uint32_t Align = cast<ConstantInt>(AlignVal)->getZExtValue();
      if (!iSTD::IsPowerOfTwo(Align)) {
        m_Ctx->EmitError((F->getName() + ": Alignment value must be a power of two.").str().c_str(), &CI);
        return;
      }
      AlignVal = ConstantInt::get(Type::getInt64Ty(m_Module->getContext()), Align);
    }

    IsNontemporal = MemOperands & Nontemporal;
  }

  SmallVector<Value *, 4> Args;
  SmallVector<Type *, 3> ArgTypes;
  Args.push_back(Ptr);
  GenISAIntrinsic::ID Iid = GenISAIntrinsic::no_intrinsic;

  if constexpr (operation == Load) {
    Iid = GenISAIntrinsic::GenISA_PredicatedLoad;

    Value *DefaultValue = CI.getArgOperand(OpVariant::DefaultValue);
    Type *ValType = CI.getType();
    IGC_ASSERT_MESSAGE(ValType == DefaultValue->getType(), "Default value type must match return type");

    Args.push_back(AlignVal ? AlignVal : getDefaultAlignValue(ValType));
    Args.push_back(Predicate);
    Args.push_back(DefaultValue);

    ArgTypes.push_back(ValType);
    ArgTypes.push_back(PtrType);
    ArgTypes.push_back(ValType);
  } else {
    Iid = GenISAIntrinsic::GenISA_PredicatedStore;
    Value *Object = CI.getArgOperand(OpVariant::Object);
    Type *ObjectType = Object->getType();

    Args.push_back(Object);
    Args.push_back(AlignVal ? AlignVal : getDefaultAlignValue(ObjectType));
    Args.push_back(Predicate);

    ArgTypes.push_back(PtrType);
    ArgTypes.push_back(ObjectType);
  }

  Function *NewFunction = GenISAIntrinsic::getDeclaration(m_Module, Iid, ArgTypes);
  auto NewCall = CallInst::Create(NewFunction, Args, "", &CI);
  NewCall->setDebugLoc(CI.getDebugLoc());
  NewCall->setName(CI.getName());

  MDNode *MdNodeCacheControl = CI.getMetadata("lsc.cache.ctrl");
  if (MdNodeCacheControl) {
    if (IsNontemporal) {
      m_Ctx->EmitWarning(
          (F->getName() + ": Nontemporal memory operand is ignored, since cache control decorations are present.")
              .str()
              .c_str(),
          &CI);
    }
    NewCall->setMetadata("lsc.cache.ctrl", MdNodeCacheControl);
  } else if (IsNontemporal) {
    NewCall->setMetadata("lsc.cache.ctrl", MDNode::get(m_Module->getContext(),
                                                       ConstantAsMetadata::get(ConstantInt::get(
                                                           Type::getInt32Ty(m_Module->getContext()), LSC_L1UC_L3UC))));
  }

  CI.replaceAllUsesWith(NewCall);
  m_InstructionsToErase.push_back(&CI);
  m_Changed = true;

  // Cleanup unused function if all calls have been replaced with the internal version
  if (F->getNumUses() == 0)
    m_BuiltinsToRemove.insert(F);
}