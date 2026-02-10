/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ReduceOptPass.hpp"
#include "IGCIRBuilder.h"
#include <llvm/IR/Function.h>

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace llvm::PatternMatch;

char ReduceOptPass::ID = 0;

#define PASS_FLAG "opt-reduce-pass"
#define PASS_DESCRIPTION "Optimization of the reduce instruction used by work item 0"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ReduceOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ReduceOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

#define GlobalSizeName "__spirv_BuiltInGlobalSize"
#define GlobalInvocationIdName "__spirv_BuiltInGlobalInvocationId"
#define GlobalOffsetName "__spirv_BuiltInGlobalOffset"
#define WorkgroupSizeName "__spirv_BuiltInWorkgroupSize"
#define LocalInvocationIdName "__spirv_BuiltInLocalInvocationId"
#define ReduceWorkGroupName "__spirv_Group"

ReduceOptPass::ReduceOptPass() : FunctionPass(ID) { initializeReduceOptPassPass(*PassRegistry::getPassRegistry()); }

bool ReduceOptPass::createReduceWI0(Instruction *ReduceInstr) {
  CallInst *Call = cast<CallInst>(ReduceInstr);
  Function *Func = Call->getCalledFunction();
  std::string Name = Func->getName().str();
  std::vector<std::string> OpSet = {"IAdd",          "FAdd",          "UMin",          "FMin",
                                    "SMin",          "FMax",          "UMax",          "SMax",
                                    "IMulKHR",       "FMulKHR",       "BitwiseAndKHR", "BitwiseOrKHR",
                                    "BitwiseXorKHR", "LogicalAndKHR", "LogicalOrKHR",  "LogicalXorKHR"};

  std::string Prefix = "__builtin_IB_WorkGroupReduce_WI0_";

  std::string Op = "";
  for (const auto &I : OpSet) {
    if (Name.find(I) != std::string::npos) {
      Op = I;
      break;
    }
  }

  if (Op == "")
    return false;

  auto TypePos = Name.find_last_of('_');
  if (TypePos == std::string::npos)
    return false;

  std::string Type = Name.substr(TypePos);
  std::string NameWI0 = Prefix + Op + Type;

  IRBuilder<> Builder(ReduceInstr);

  FunctionType *NewReduceFuncT =
      FunctionType::get(ReduceInstr->getOperand(2)->getType(), ReduceInstr->getOperand(2)->getType(), false);

  FunctionCallee NewReduceCallee = M->getOrInsertFunction(NameWI0, NewReduceFuncT);
  cast<Function>(NewReduceCallee.getCallee())->setCallingConv(CallingConv::SPIR_FUNC);

  CallInst *NewReduce = Builder.CreateCall(NewReduceCallee, ReduceInstr->getOperand(2));
  NewReduce->setCallingConv(CallingConv::SPIR_FUNC);
  ReduceInstr->replaceAllUsesWith(NewReduce);
  ReduceInstr->eraseFromParent();

  return true;
}

bool ReduceOptPass::checkBuiltInName(Value *Instr, const std::string &Name) {
  CallInst *Call = dyn_cast<CallInst>(Instr);
  if (!Call)
    return false;

  Function *Func = Call->getCalledFunction();
  if (!Func)
    return false;

  std::string FuncName = Func->getName().str();

  if (FuncName == Name)
    return true;

  if ((FuncName.find(Name) != std::string::npos) && (Name == ReduceWorkGroupName)) {
    Value *Op0 = Call->getOperand(0);
    Value *Op1 = Call->getOperand(1);

    if (isa<ConstantInt>(Op0) && isa<ConstantInt>(Op1)) {
      ConstantInt *ConstOp0 = cast<ConstantInt>(Op0);
      ConstantInt *ConstOp1 = cast<ConstantInt>(Op1);

      int64_t Op0Value = ConstOp0->getSExtValue();
      int64_t Op1Value = ConstOp1->getSExtValue();

      // Check that group built-in is work group reduce function
      // Op0Value == 2 means that built-in operates with work group
      // Op1Value == 0 describes the type of group operation (reduce)
      if (Op0Value == 2 && Op1Value == 0)
        return true;
    }

    return false;
  }

  return false;
}

// This function checks if val is work item's global linear id.
// In other words, it checks the given pattern:
//    %3 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalSize() #5
//    %4 = extractelement <3 x i64> %3, i32 1
//    %5 = extractelement <3 x i64> %3, i32 0
//    %6 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId() #5
//    %7 = extractelement <3 x i64> %6, i32 2
//    %8 = extractelement <3 x i64> %6, i32 1
//    %9 = extractelement <3 x i64> %6, i32 0
//    %10 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalOffset() #5
//    %11 = extractelement <3 x i64> %10, i32 2
//    %12 = extractelement <3 x i64> %10, i32 1
//    %13 = extractelement <3 x i64> %10, i32 0
//    %14 = sub i64 %7, %11
//    %15 = mul i64 %14, %4
//    %16 = sub i64 %8, %12
//    %17 = add i64 %15, %16
//    %18 = mul i64 %17, %5
//    %19 = sub i64 %9, %13
//    %20 = add i64 %18, %19
bool ReduceOptPass::checkGlobalId(Value *Val) {
  Value *MulVal1 = nullptr;
  Value *ExtrVal1 = nullptr;
  Value *ExtrVal2 = nullptr;
  Value *ExtrVal3 = nullptr;
  auto AddPat1 = m_Add(m_Mul(m_Value(MulVal1), m_Value(ExtrVal1)), m_Sub(m_Value(ExtrVal2), m_Value(ExtrVal3)));
  if (!match(Val, AddPat1))
    return false;

  Value *SubVal2 = nullptr;
  Value *ExtrVal4 = nullptr;
  Value *ExtrVal5 = nullptr;
  Value *ExtrVal6 = nullptr;
  auto AddPat2 = m_Add(m_Mul(m_Value(SubVal2), m_Value(ExtrVal4)), m_Sub(m_Value(ExtrVal5), m_Value(ExtrVal6)));
  if (!match(MulVal1, AddPat2))
    return false;

  Value *ExtrVal7 = nullptr;
  Value *ExtrVal8 = nullptr;
  auto SubPat = m_Sub(m_Value(ExtrVal7), m_Value(ExtrVal8));
  if (!match(SubVal2, SubPat))
    return false;

  Value *Bi1 = nullptr;
  Value *Bi2 = nullptr;
  Value *Bi3 = nullptr;
  Value *Bi4 = nullptr;
  Value *Bi5 = nullptr;
  Value *Bi6 = nullptr;
  Value *Bi7 = nullptr;
  Value *Bi8 = nullptr;

  auto ExtElemPat1 = m_ExtractElt(m_Value(Bi1), m_SpecificInt(0));
  auto ExtElemPat2 = m_ExtractElt(m_Value(Bi2), m_SpecificInt(0));
  auto ExtElemPat3 = m_ExtractElt(m_Value(Bi3), m_SpecificInt(0));
  if (!match(ExtrVal1, ExtElemPat1) || !match(ExtrVal2, ExtElemPat2) || !match(ExtrVal3, ExtElemPat3))
    return false;

  auto ExtElemPat4 = m_ExtractElt(m_Value(Bi4), m_SpecificInt(1));
  auto ExtElemPat5 = m_ExtractElt(m_Value(Bi5), m_SpecificInt(1));
  auto ExtElemPat6 = m_ExtractElt(m_Value(Bi6), m_SpecificInt(1));
  if (!match(ExtrVal4, ExtElemPat4) || !match(ExtrVal5, ExtElemPat5) || !match(ExtrVal6, ExtElemPat6))
    return false;

  auto ExtElemPat7 = m_ExtractElt(m_Value(Bi7), m_SpecificInt(2));
  auto ExtElemPat8 = m_ExtractElt(m_Value(Bi8), m_SpecificInt(2));
  if (!match(ExtrVal7, ExtElemPat7) || !match(ExtrVal8, ExtElemPat8))
    return false;

  if (!checkBuiltInName(Bi1, GlobalSizeName) || !checkBuiltInName(Bi4, GlobalSizeName)) {
    return false;
  }

  if (!checkBuiltInName(Bi2, GlobalInvocationIdName) || !checkBuiltInName(Bi5, GlobalInvocationIdName) ||
      !checkBuiltInName(Bi7, GlobalInvocationIdName)) {
    return false;
  }

  if (!checkBuiltInName(Bi3, GlobalOffsetName) || !checkBuiltInName(Bi6, GlobalOffsetName) ||
      !checkBuiltInName(Bi8, GlobalOffsetName)) {
    return false;
  }

  return true;
}

// This function checks if val is work item's local linear id.
// In other words, it checks the given pattern:
//    %3 = call spir_func <3 x i64> @__builtin_spirv_BuiltInWorkgroupSize() #5
//    %4 = extractelement <3 x i64> %3, i32 1
//    %5 = extractelement <3 x i64> %3, i32 0
//    %6 = call spir_func <3 x i64> @__builtin_spirv_BuiltInLocalInvocationId() #5
//    %7 = extractelement <3 x i64> %6, i32 2
//    %8 = extractelement <3 x i64> %6, i32 1
//    %9 = extractelement <3 x i64> %6, i32 0
//    %10 = mul i64 %7, %4
//    %11 = add i64 %10, %8
//    %12 = mul i64 %11, %5
//    %13 = add i64 %12, %9
bool ReduceOptPass::checkLocalId(Value *Val) {
  Value *MulVal1 = nullptr;
  Value *ExtrVal1 = nullptr;
  Value *ExtrVal2 = nullptr;
  auto AddPat1 = m_Add(m_Mul(m_Value(MulVal1), m_Value(ExtrVal1)), m_Value(ExtrVal2));
  if (!match(Val, AddPat1))
    return false;

  Value *ExtrVal3 = nullptr;
  Value *ExtrVal4 = nullptr;
  Value *ExtrVal5 = nullptr;
  auto AddPat2 = m_Add(m_Mul(m_Value(ExtrVal3), m_Value(ExtrVal4)), m_Value(ExtrVal5));
  if (!match(MulVal1, AddPat2))
    return false;

  Value *Bi1 = nullptr;
  Value *Bi2 = nullptr;
  Value *Bi3 = nullptr;
  Value *Bi4 = nullptr;
  Value *Bi5 = nullptr;

  auto ExtelemPat1 = m_ExtractElt(m_Value(Bi1), m_SpecificInt(0));
  auto ExtelemPat2 = m_ExtractElt(m_Value(Bi2), m_SpecificInt(0));
  if (!match(ExtrVal1, ExtelemPat1) || !match(ExtrVal2, ExtelemPat2))
    return false;

  auto ExtelemPat3 = m_ExtractElt(m_Value(Bi3), m_SpecificInt(2));
  if (!match(ExtrVal3, ExtelemPat3))
    return false;

  auto ExtelemPat4 = m_ExtractElt(m_Value(Bi4), m_SpecificInt(1));
  auto ExtelemPat5 = m_ExtractElt(m_Value(Bi5), m_SpecificInt(1));
  if (!match(ExtrVal5, ExtelemPat5) || !match(ExtrVal4, ExtelemPat4))
    return false;

  if (!checkBuiltInName(Bi1, WorkgroupSizeName) || !checkBuiltInName(Bi4, WorkgroupSizeName)) {
    return false;
  }

  if (!checkBuiltInName(Bi2, LocalInvocationIdName) || !checkBuiltInName(Bi3, LocalInvocationIdName) ||
      !checkBuiltInName(Bi5, LocalInvocationIdName)) {
    return false;
  }

  return true;
}

// This function checks if Val is a cmp instruction that compares
// the work item's global linear ID or work item's local linear ID to zero.
bool ReduceOptPass::checkCmp(Value *Val) {
  if (isa<CmpInst>(Val)) {
    CmpInst *Cmp = cast<CmpInst>(Val);
    Value *CmpOp0 = Cmp->getOperand(0);
    Value *CmpOp1 = Cmp->getOperand(1);

    [[maybe_unused]] Instruction *GetIdInstr = nullptr;
    ConstantInt *CmpConst = nullptr;

    if (isa<ConstantInt>(CmpOp1) && (checkGlobalId(CmpOp0) || checkLocalId(CmpOp0))) {
      CmpConst = cast<ConstantInt>(CmpOp1);
      GetIdInstr = cast<Instruction>(CmpOp0);
    } else if (isa<ConstantInt>(CmpOp0) && (checkGlobalId(CmpOp1) || checkLocalId(CmpOp1))) {
      CmpConst = cast<ConstantInt>(CmpOp0);
      GetIdInstr = cast<Instruction>(CmpOp1);
    } else {
      return false;
    }

    if ((CmpConst->getSExtValue() == 0) && (Cmp->getPredicate() == CmpInst::ICMP_EQ)) {
      return true;
    }
  }
  return false;
}

// This function checks that the select statement has a condition like
// global_linear_id == 0
// local_linear_id == 0
bool ReduceOptPass::checkSelect(Instruction *Sel) {
  Value *SelOp0 = Sel->getOperand(0);
  return checkCmp(SelOp0);
}

// This function checks that the BB is only executed
// by a work item with a global_linear_id or local_linear_id = 0.
bool ReduceOptPass::checkBranch(BasicBlock *Bb) {
  BasicBlock *Pred = Bb->getSinglePredecessor();
  if (!Pred)
    return false;

  BranchInst *BrInst = dyn_cast<BranchInst>(Pred->getTerminator());
  if (!BrInst)
    return false;

  if (BrInst->isConditional() && (BrInst->getSuccessor(0) == Bb)) {
    return checkCmp(BrInst->getOperand(0));
  }

  return false;
}

bool ReduceOptPass::checkUsers(Value *MainVal, Value *Val, BasicBlock *Bb) {
  Instruction *Instr = dyn_cast<Instruction>(Val);
  if (!Instr)
    return false;

  BasicBlock *InstrBb = Instr->getParent();

  if (SelectInst *Sel = dyn_cast<SelectInst>(Instr)) {
    if (MainVal != Sel->getOperand(1))
      return false;

    if (!checkSelect(Sel))
      return false;
  } else {
    if (Bb != InstrBb)
      return checkBranch(InstrBb);

    if (Val->hasNUsesOrMore(1)) {
      for (auto *U : Val->users())
        if (!checkUsers(Val, dyn_cast<Value>(U), Bb))
          return false;
    } else {
      return false;
    }
  }
  return true;
}

bool ReduceOptPass::runOnFunction(Function &F) {
  Changed = false;
  M = F.getParent();
  SmallVector<Instruction *, 8> ReduceToProcess;

  // Collect all reduce and atomic instructions that matches the pattern.
  for (auto &Bb : F) {
    for (auto &Inst : Bb) {
      Value *Val = cast<Value>(&Inst);
      if (checkBuiltInName(Val, ReduceWorkGroupName)) {
        // Check if ReduceToProcess SmallVector is full
        if (ReduceToProcess.size() == 65535)
          continue;

        bool PassedFlag = true;
        for (auto U : Val->users()) {
          if (!checkUsers(&Inst, U, &Bb)) {
            PassedFlag = false;
            break;
          }
        }

        if (PassedFlag)
          ReduceToProcess.push_back(cast<Instruction>(Val));
      }
    }
  }

  for (auto ReduceInstr : ReduceToProcess)
    Changed = createReduceWI0(ReduceInstr);

  return Changed;
}