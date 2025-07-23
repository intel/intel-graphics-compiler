/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/LegalizationPass.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include <llvmWrapper/IR/BasicBlock.h>
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "llvm/Support/Casting.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;
using namespace IGC::IGCMD;

namespace IGC {

bool expandFDIVInstructions(llvm::Function &F, ShaderType ShaderTy);

} // namespace IGC

static cl::opt<bool> PreserveNan("preserve-nan", cl::init(false), cl::Hidden, cl::desc("Preserve NAN (default false)"));

// Register pass to igc-opt
#define PASS_FLAG "igc-legalization"
#define PASS_DESCRIPTION "VISA Legalizer"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Legalization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Legalization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char Legalization::ID = 0;

Legalization::Legalization(bool preserveNan)
    : FunctionPass(ID), m_preserveNan(preserveNan), m_preserveNanCheck(m_preserveNan), m_DL(0) {
  initializeLegalizationPass(*PassRegistry::getPassRegistry());
}

bool Legalization::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto MD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo()) {
    return false;
  }
  if (MD->compOpt.FiniteMathOnly) {
    m_preserveNan = false;
    // Do not preserve nan but honor nan checks.
    m_preserveNanCheck = true;
  }
  llvm::IRBuilder<> builder(F.getContext());
  m_builder = &builder;
  // Emit pass doesn't support constant expressions, therefore we do not expect to run into them in this pass
  for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    for (auto OI = I->op_begin(), OE = I->op_end(); OI != OE; ++OI) {
      IGC_ASSERT_MESSAGE(!isa<ConstantExpr>(OI), "Function must not contain constant expressions");
    }
  }

  // Create a unique return instruction for this funciton if necessary.
  unifyReturnInsts(F);

  m_DL = &F.getParent()->getDataLayout();
  // recalculate this field
  m_ctx->m_instrTypes.numInsts = 0;

  visit(F);

  for (auto I : m_instructionsToRemove) {
    I->eraseFromParent();
  }

  m_instructionsToRemove.clear();

  // Legalize fdiv if any
  if (!m_ctx->platform.hasFDIV())
    expandFDIVInstructions(F, m_ctx->type);
  return true;
}

void Legalization::unifyReturnInsts(llvm::Function &F) {
  // Adapted from llvm::UnifyFunctionExitNodes.cpp
  //
  // Loop over all of the blocks in a function, tracking all of the blocks
  // that return.
  SmallVector<BasicBlock *, 16> ReturningBlocks;
  for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
    if (isa<ReturnInst>(I->getTerminator()))
      ReturningBlocks.push_back(&(*I));

  // Now handle return blocks.
  if (ReturningBlocks.size() <= 1)
    return;

  // Otherwise, we need to insert a new basic block into the function,
  // add a PHI nodes (if the function returns values), and convert
  // all of the return instructions into unconditional branches.
  BasicBlock *NewRetBlock = BasicBlock::Create(F.getContext(), "UnifiedReturnBlock", &F);

  PHINode *PN = nullptr;
  if (F.getReturnType()->isVoidTy())
    ReturnInst::Create(F.getContext(), nullptr, NewRetBlock);
  else {
    // If the function doesn't return void... add a PHI node to the block...
    PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(), "UnifiedRetVal");

    IGCLLVM::pushBackInstruction(NewRetBlock, PN);
    ReturnInst::Create(F.getContext(), PN, NewRetBlock);
  }

  // Loop over all of the blocks, replacing the return instruction with an
  // unconditional branch.
  for (auto BB : ReturningBlocks) {
    // Add an incoming element to the PHI node for every return instruction that
    // is merging into this new block...
    if (PN)
      PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

    IGCLLVM::popBackInstruction(BB); // Remove the return inst.
    BranchInst::Create(NewRetBlock, BB);
  }
}

void Legalization::visitInstruction(llvm::Instruction &I) {
  if (!llvm::isa<llvm::DbgInfoIntrinsic>(&I))
    m_ctx->m_instrTypes.numInsts++;

  BasicBlock *dBB = I.getParent();

  for (auto U : I.users()) {
    auto UI = cast<Instruction>(U);
    BasicBlock *uBB = UI->getParent();
    if (uBB != dBB) {
      m_ctx->m_instrTypes.numGlobalInsts++;
      return;
    }
  }
  m_ctx->m_instrTypes.numLocalInsts++;
}

void Legalization::visitUnaryInstruction(UnaryInstruction &I) {
  // Legalize bfloat unary instructions that need intermediate
  // step with float type.
  auto SrcOperand = I.getOperand(0);
  if (!IGCLLVM::isBFloatTy(SrcOperand->getType()->getScalarType()) &&
      !IGCLLVM::isBFloatTy(I.getType()->getScalarType()))
    return;

  bool convertSourceToFloat = false;
  bool extendDestToFloat = false;
  bool needsTrunc = false;
  switch (I.getOpcode()) {
  case Instruction::FPToUI:
  case Instruction::FPToSI:
    convertSourceToFloat = true;
    break;
  case Instruction::UIToFP:
  case Instruction::SIToFP:
    needsTrunc = true;
    extendDestToFloat = true;
    break;
  case Instruction::FPExt:
    if (I.getType()->getScalarType()->isDoubleTy()) {
      convertSourceToFloat = true;
    } else {
      return;
    }
    break;
  case Instruction::FPTrunc:
    if (SrcOperand->getType()->getScalarType()->isDoubleTy()) {
      convertSourceToFloat = true;
    } else {
      return;
    }
    break;
  case Instruction::FNeg:
    convertSourceToFloat = true;
    extendDestToFloat = true;
    needsTrunc = true;
    break;
  default:
    return;
  }

  // helper lambda to get corresponding vector or scalar float type.
  auto getFloatTypeBasedOnType = [](Type *orgType) {
    Type *FloatTy = Type::getFloatTy(orgType->getContext());
    return orgType->isVectorTy() ? IGCLLVM::FixedVectorType::get(
                                       FloatTy, (unsigned)cast<IGCLLVM::FixedVectorType>(orgType)->getNumElements())
                                 : FloatTy;
  };

  m_builder->SetInsertPoint(&I);
  auto NewInst = I.clone();
  NewInst->setDebugLoc(I.getDebugLoc());

  if (convertSourceToFloat) {
    auto NewType = getFloatTypeBasedOnType(SrcOperand->getType());
    Value *SourceConverted = nullptr;
    if (NewType->getScalarSizeInBits() > SrcOperand->getType()->getScalarSizeInBits()) {
      SourceConverted = m_builder->CreateFPExt(SrcOperand, NewType);
    } else {
      SourceConverted = m_builder->CreateFPTrunc(SrcOperand, NewType);
    }
    NewInst->setOperand(0, SourceConverted);
    cast<Instruction>(SourceConverted)->setDebugLoc(I.getDebugLoc());
  }

  m_builder->Insert(NewInst);

  if (extendDestToFloat) {
    NewInst->mutateType(getFloatTypeBasedOnType(I.getType()));
  }

  if (needsTrunc) {
    auto TruncInst = cast<Instruction>(m_builder->CreateFPTrunc(NewInst, I.getType()));
    TruncInst->setDebugLoc(I.getDebugLoc());
    NewInst = TruncInst;
  }

  I.replaceAllUsesWith(NewInst);
  m_instructionsToRemove.push_back(&I);

  m_ctx->m_instrTypes.numInsts++;
}

void Legalization::visitBinaryOperator(llvm::BinaryOperator &I) {
  if (I.getOpcode() == Instruction::FRem && I.getType()->isFloatTy()) {
    Function *floorFunc = Intrinsic::getDeclaration(m_ctx->getModule(), Intrinsic::floor, I.getType());
    m_builder->SetInsertPoint(&I);
    Value *a = I.getOperand(0);
    Value *b = I.getOperand(1);
    Value *mulab = m_builder->CreateFMul(a, b);
    Value *sign = m_builder->CreateFCmpOGE(mulab, m_builder->CreateFNeg(mulab));
    Value *sel = m_builder->CreateSelect(sign, b, m_builder->CreateFNeg(b));
    Value *selInv = m_builder->CreateFDiv(ConstantFP::get(m_builder->getFloatTy(), 1.f), sel);
    Value *div = m_builder->CreateFMul(a, selInv);
    Value *floordiv = m_builder->CreateCall(floorFunc, div);
    Value *frc = m_builder->CreateFSub(div, floordiv);
    Value *result = m_builder->CreateFMul(frc, sel);
    I.replaceAllUsesWith(result);
    I.eraseFromParent();
  } else if (I.getOpcode() == Instruction::And || I.getOpcode() == Instruction::Or) {
    // convert (!a and !b) to !(a or b)
    // convert (!a or !b) to !(a and b)
    // then remove the negate by flipping all the uses (select or branch)
    Value *src0 = I.getOperand(0);
    Value *src1 = I.getOperand(1);
    if (IGCLLVM::BinaryOperator::isNot(src0) && IGCLLVM::BinaryOperator::isNot(src1) && src0->hasOneUse() &&
        src1->hasOneUse()) {
      // check all uses are select or branch
      bool flippable = true;
      for (auto U = I.user_begin(), E = I.user_end(); U != E; ++U) {
        if (!isa<SelectInst>(*U) && !isa<BranchInst>(*U)) {
          flippable = false;
          break;
        }
        // check select i1 with I not used as condition
       if (isa<SelectInst>(*U) &&
           (U->getOperand(0) != &I ||
            U->getOperand(1) == &I ||
            U->getOperand(2) == &I)) {
          flippable = false;
          break;
        }
      }
      if (flippable) {
        Instruction *invert = nullptr;
        if (I.getOpcode() == Instruction::And) {
          invert = llvm::BinaryOperator::CreateOr(cast<llvm::Instruction>(src0)->getOperand(0),
                                                  cast<llvm::Instruction>(src1)->getOperand(0), "", &I);
        } else {
          invert = llvm::BinaryOperator::CreateAnd(cast<llvm::Instruction>(src0)->getOperand(0),
                                                   cast<llvm::Instruction>(src1)->getOperand(0), "", &I);
        }
        if (invert) {
          invert->setDebugLoc(I.getDebugLoc());
        }
        // Collect users before modifying them
        std::vector<User *> users(I.user_begin(), I.user_end());
        for (auto U : users) {
          if (SelectInst *s = dyn_cast<SelectInst>(U)) {
            Value *trueValue = s->getTrueValue();
            Value *falseValue = s->getFalseValue();
            s->setOperand(1, falseValue);
            s->setOperand(2, trueValue);
            s->setOperand(0, invert);
          } else if (BranchInst *br = dyn_cast<BranchInst>(U)) {
            IGC_ASSERT(br->isConditional());
            br->swapSuccessors();
            br->setCondition(invert);
          }
        }
        IGC_ASSERT(
            I.user_empty() &&
            "Instruction should have no remaining uses after transformation"
        );
        I.eraseFromParent();
        cast<llvm::Instruction>(src0)->eraseFromParent();
        cast<llvm::Instruction>(src1)->eraseFromParent();
      }
    }
  }
  m_ctx->m_instrTypes.numInsts++;
}

void Legalization::visitCallInst(llvm::CallInst &I) {
  if (auto FPIntrin = dyn_cast<FPBinaryOperatorIntrinsic>(&I)) {
    Value *L = FPIntrin->getLValue();
    Value *R = FPIntrin->getRValue();
    using FPOp = FPBinaryOperatorIntrinsic::FPBinaryOperators;
    FPOp opcode = static_cast<FPOp>(FPIntrin->getFPInstOpcode());
    Value *newInst = nullptr;
    m_builder->SetInsertPoint(&I);

    switch (opcode) {
    case FPOp::FAdd:
      newInst = m_builder->CreateFAdd(L, R);
      break;
    case FPOp::FSub:
      newInst = m_builder->CreateFSub(L, R);
      break;
    case FPOp::FMul:
      newInst = m_builder->CreateFMul(L, R);
      break;
    case FPOp::FDiv:
      newInst = m_builder->CreateFDiv(L, R);
      break;
    };

    if (isa<Instruction>(newInst)) {
      FastMathFlags Flags = I.getFastMathFlags();
      cast<Instruction>(newInst)->setFastMathFlags(Flags);
    }

    I.replaceAllUsesWith(newInst);
    newInst->takeName(&I);
    I.eraseFromParent();
  } else if (!m_ctx->platform.supportSamplerFp16Input() &&
             (llvm::isa<SampleIntrinsic>(&I) || llvm::isa<SamplerGatherIntrinsic>(&I)) &&
             I.getOperand(0)->getType()->isHalfTy()) {
    PromoteFp16ToFp32OnGenSampleCall(I);
  } else if (auto *GII = dyn_cast<GenIntrinsicInst>(&I)) {
    if (m_ctx->platform.hasNoFullI64Support() &&
        (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_imulH ||
         GII->getIntrinsicID() == GenISAIntrinsic::GenISA_umulH) &&
        GII->getArgOperand(0)->getType()->isIntegerTy(64)) {
      BasicBlock *const bb = GII->getParent();
      IGC_ASSERT(nullptr != bb);
      Function *const f = bb->getParent();
      IGC_ASSERT(nullptr != f);
      IGCLLVM::IRBuilder<> Builder(GII);

      const bool isSigned = GII->getIntrinsicID() == GenISAIntrinsic::GenISA_imulH;

      IGC_ASSERT(GII->getArgOperand(0)->getType() == GII->getArgOperand(1)->getType());
      Value *newInst = CreateMulh(*f, Builder, isSigned, GII->getArgOperand(0), GII->getArgOperand(1));
      IGC_ASSERT_MESSAGE(nullptr != newInst, "CreateMulh failed.");
      GII->replaceAllUsesWith(newInst);
      GII->eraseFromParent();
    }
  }
  m_ctx->m_instrTypes.numInsts++;
}

// Match and legalize the following patterns out of GVN:
//
// (1)
// %23 = bitcast <3 x half> %assembled.vect35 to i48
// %trunc = trunc i48 %23 to i16
// %bitcast = bitcast i16 %trunc to half
//
// (2)
// %23 = bitcast <3 x half> %assembled.vect35 to i48
// %27 = lshr i48 %23, 16
// %trunc = trunc i48 %27 to i16
// %bitcast = bitcast i16 %28 to half
//
// into
//
// (1-legalized)
// %30 = extract <3 x half> %assembled.vect35, i32 0
// <replace all uses of %bitcast by %30>
//
// (2-legalized)
// 31 = extract <3 x half> %assembled.vect35, i32 1
// <replace all uses of %bitcast by %31>
//
// Case 3:
//
// %158 = bitcast <4 x float> %130 to i128
// %trunc = trunc i128 %158 to i96
// %bitcast = bitcast i96 %trunc to <3 x float>
// %scalar92 = extractelement <3 x float> %bitcast, i32 0
// %scalar93 = extractelement <3 x float> %bitcast, i32 1
// %scalar94 = extractelement <3 x float> %bitcast, i32 2
//
// into
//
// (3-legalized)
// %scalar92_0 = extractelement <4 x float> %130, i32 0
// %scalar93_1 = extractelement <4 x float> %130, i32 1
// %scalar94_2 = extractelement <4 x float> %130, i32 2
// <replace all uses of %scalar9{2,3,4}>
//
// Case 4:
//
// (1)
// %24 = bitcast <4 x i32> %22 to i128
// %29 = trunc i128 %24 to i8
//
// (2)
// %24 = bitcast <4 x i32> %22 to i128
// %28 = lshr i128 %24, 8
// %29 = trunc i128 %28 to i8
//
// into
//
// (1-legalized)
// %24 = bitcast <4 x i32> %22 to <16 x i8>
// %28 = extractelement <16 x i8> %24 i32 0
//
// (2-legalized)
// %24 = bitcast <4 x i32> %22 to <16 x i8>
// %28 = extractelement <16 x i8> %24 i32 1
//

static bool LegalizeGVNBitCastPattern(IRBuilder<> *Builder, const DataLayout *DL, BitCastInst &I,
                                      std::vector<Instruction *> *m_instructionsToRemove) {
  IntegerType *DstTy = dyn_cast<IntegerType>(I.getType());
  VectorType *SrcTy = dyn_cast<VectorType>(I.getOperand(0)->getType());
  if (!DstTy || !SrcTy || DL->isLegalInteger(DstTy->getBitWidth())) {
    return false;
  }

  Type *EltTy = cast<VectorType>(SrcTy)->getElementType();
  auto match1 = [=](Value *V, BinaryOperator *&BO, TruncInst *&TI, BitCastInst *&BI, int &Index) {
    // The leading instruction is optional.
    BO = dyn_cast<BinaryOperator>(V);
    if (BO) {
      if (BO->getOpcode() != Instruction::LShr || !BO->hasOneUse())
        return false;

      // The shift amount shall be a constant.
      Value *BOp1 = BO->getOperand(1);
      auto CI = dyn_cast<ConstantInt>(BOp1);
      if (!CI)
        return false;

      // The shift amount shall be a multiple of base element.
      uint64_t ShAmt = CI->getZExtValue();
      const unsigned int denominator = (const unsigned int)EltTy->getPrimitiveSizeInBits();
      IGC_ASSERT(denominator);

      if (ShAmt % denominator != 0)
        return false;

      // Compute the index of the element to be extracted.
      Index = int_cast<int>(ShAmt / denominator);
    }

    // The second instruction is *not* optional.
    if (BO)
      TI = dyn_cast<TruncInst>(BO->user_back());
    else
      TI = dyn_cast<TruncInst>(V);

    if (!TI || !TI->hasOneUse())
      return false;

    // Optionally, followed by a bitcast.
    // Assign null to BI if this is not ending with a bitcast.
    BI = dyn_cast<BitCastInst>(TI->user_back());

    // This guarantees all uses of BI could be replaced by the source.
    if (BI && BI->getType() != EltTy)
      return false;
    else if (TI->getType()->getPrimitiveSizeInBits() != EltTy->getPrimitiveSizeInBits())
      return false;

    return true;
  };

  // %158 = bitcast <4 x float> %130 to i128
  // %trunc = trunc i128 %158 to i96                          // V, TI
  // %bitcast = bitcast i96 %trunc to <3 x float>             // BI
  // %scalar92 = extractelement <3 x float> %bitcast, i32 0   // EEI[0]
  // %scalar93 = extractelement <3 x float> %bitcast, i32 1   // EEI[1]
  // %scalar94 = extractelement <3 x float> %bitcast, i32 2   // EEI[2]
  //
  // Match the above pattern and initialize TI, BI, EEIs
  auto match2 = [=](Value *V, TruncInst *&TI, BitCastInst *&BI, SmallVectorImpl<ExtractElementInst *> &EEIs) {
    TI = dyn_cast<TruncInst>(V);
    if (!TI || !TI->hasOneUse())
      return false;

    BI = dyn_cast<BitCastInst>(TI->user_back());

    // Only valid for vector type.
    if (!BI || !BI->getType()->isVectorTy())
      return false;

    // All uses must be EEI and must have the same element size as the original element type.
    for (auto U : BI->users()) {
      auto EEI = dyn_cast<ExtractElementInst>(U);
      if (!EEI || EEI->getType()->getPrimitiveSizeInBits() != EltTy->getPrimitiveSizeInBits())
        return false;
      EEIs.push_back(EEI);
    }
    return true;
  };

  auto match3 = [=](Value *V, BinaryOperator *&BO, TruncInst *&TI, int &Index) {
    // The lshr instruction is optional.
    BO = dyn_cast<BinaryOperator>(V);
    if (BO && (BO->getOpcode() != Instruction::LShr || !BO->hasOneUse()))
      return false;

    // The trunc instruction is *not* optional.
    if (BO)
      TI = dyn_cast<TruncInst>(BO->user_back());
    else
      TI = dyn_cast<TruncInst>(V);

    if (!TI)
      return false;

    int srcSize = int_cast<int>(TI->getOperand(0)->getType()->getPrimitiveSizeInBits());
    int dstSize = int_cast<int>(TI->getType()->getPrimitiveSizeInBits());
    if (srcSize % dstSize != 0)
      return false;

    if (BO) {
      // The shift amount shall be a constant.
      Value *BOp1 = BO->getOperand(1);
      auto CI = dyn_cast<ConstantInt>(BOp1);
      if (!CI)
        return false;

      // The shift amount shall be a multiple of base element.
      uint64_t ShAmt = CI->getZExtValue();
      uint64_t ElSize = TI->getType()->getPrimitiveSizeInBits();
      if (ShAmt % ElSize != 0)
        return false;

      // Compute the index of the element to be extracted.
      Index = int_cast<int>(ShAmt / ElSize);
    }

    return true;
  };

  for (auto U : I.users()) {
    // Case 1 and 2 and 4.
    BinaryOperator *BO = nullptr; // the lshr instruction, optional
    TruncInst *TI = nullptr;      // not optional
    BitCastInst *BI = nullptr;    // optional
    int Index = 0;                // the vector element index.

    // Case 3 only.
    SmallVector<ExtractElementInst *, 8> EEIs;

    if (match1(U, BO, TI, BI, Index)) {
      if (BI)
        Builder->SetInsertPoint(BI);
      else
        Builder->SetInsertPoint(TI);

      Value *V =
          Builder->CreateExtractElement(I.getOperand(0), ConstantInt::get(Type::getInt32Ty(I.getContext()), Index));

      if (BI) {
        IGC_ASSERT(BI->getType() == EltTy);

        // BO, TI, and BI are dead.
        BI->replaceAllUsesWith(V);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(BI);
        }

        Value *tempUndefValue = UndefValue::get(TI->getType());
        Instruction *UndefValueAsTempInst = dyn_cast<Instruction>(tempUndefValue);
        if (UndefValueAsTempInst)
          UndefValueAsTempInst->setDebugLoc(TI->getDebugLoc());

        TI->replaceAllUsesWith(tempUndefValue);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(TI);
        }
      } else {
        IGC_ASSERT(TI->getType()->getPrimitiveSizeInBits() == EltTy->getPrimitiveSizeInBits());
        if (V->getType() != TI->getType())
          V = Builder->CreateBitCast(V, TI->getType());

        // BO and TI are dead.
        TI->replaceAllUsesWith(V);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(TI);
        }
      }

      if (BO) {
        Value *tempUndefValue = UndefValue::get(BO->getType());
        Instruction *UndefValueAsTempInst = dyn_cast<Instruction>(tempUndefValue);
        if (UndefValueAsTempInst)
          UndefValueAsTempInst->setDebugLoc(BO->getDebugLoc());

        BO->replaceAllUsesWith(tempUndefValue);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(BO);
        }
      }
    } else if (match2(U, TI, BI, EEIs)) {
      for (auto EEI : EEIs) {
        Builder->SetInsertPoint(EEI);
        // The index operand remains the same since there is no
        // shift on the wide integer source.
        Value *V = Builder->CreateExtractElement(I.getOperand(0), EEI->getIndexOperand());
        if (V->getType() != EEI->getType()) {
          V = Builder->CreateBitCast(V, EEI->getType());
        }
        EEI->replaceAllUsesWith(V);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(EEI);
        }
      }
    } else if (match3(U, BO, TI, Index)) {
      // Example:
      // %24 = bitcast <4 x i32> %22 to i128
      // %28 = lshr i128 %24, 8
      // %29 = trunc i128 %28 to i8
      Type *castType = TI->getType();
      int srcSize = int_cast<int>(TI->getOperand(0)->getType()->getPrimitiveSizeInBits());
      int dstSize = int_cast<int>(castType->getPrimitiveSizeInBits());

      // vecSize is 128/8 = 16 in above example
      IGC_ASSERT(dstSize);
      IGC_ASSERT(srcSize % dstSize == 0);
      uint vecSize = srcSize / dstSize;

      Builder->SetInsertPoint(TI);
      Value *BC = Builder->CreateBitCast(I.getOperand(0), IGCLLVM::FixedVectorType::get(castType, vecSize));
      Value *EE = Builder->CreateExtractElement(BC, ConstantInt::get(Type::getInt32Ty(I.getContext()), Index));

      // BO and TI are dead
      TI->replaceAllUsesWith(EE);
      if (m_instructionsToRemove) {
        m_instructionsToRemove->push_back(TI);
      }
      if (BO) {
        Value *tempUndefValue = UndefValue::get(BO->getType());
        Instruction *UndefValueAsTempInst = dyn_cast<Instruction>(tempUndefValue);
        if (UndefValueAsTempInst)
          UndefValueAsTempInst->setDebugLoc(BO->getDebugLoc());

        BO->replaceAllUsesWith(tempUndefValue);
        if (m_instructionsToRemove) {
          m_instructionsToRemove->push_back(BO);
        }
      }
    }
  }

  return true;
}

void Legalization::visitBitCastInst(llvm::BitCastInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  // This is the pass that folds 2x Float into a Double replacing the bitcast instruction
  if (ConstantDataVector *vec = dyn_cast<ConstantDataVector>(I.getOperand(0))) {
    unsigned int nbElement = vec->getNumElements();
    // nbElement == 2 implies the bitcast instruction has a 2X Float src and we are checking if the destination is of
    // Type Double
    if (nbElement == 2 && I.getType()->isDoubleTy() && vec->getElementType()->isFloatTy()) {
      // Extracting LSB form srcVec
      ConstantFP *srcLSB = cast<ConstantFP>(vec->getElementAsConstant(0));
      uint64_t LSB = srcLSB->getValueAPF().bitcastToAPInt().getZExtValue();

      // Extracting MSB form srcVec
      ConstantFP *srcMSB = cast<ConstantFP>(vec->getElementAsConstant(1));
      uint64_t MSB = srcMSB->getValueAPF().bitcastToAPInt().getZExtValue();

      // Replacing the bitcast instruction with 2x float to a emit a double value
      uint64_t rslt = ((MSB << 32) | LSB);
      // Yes, this is a hack. double result = static_cast<double>(rslt) didn't generate the correct double equivalent
      // for rslt
      double result = *(double *)&rslt;
      ConstantFP *newVec = cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(I.getContext()), result));
      auto tempInst = dyn_cast<Instruction>(newVec);
      if (tempInst)
        tempInst->setDebugLoc(I.getDebugLoc());

      I.replaceAllUsesWith(newVec);
      I.eraseFromParent();
      return;
    }
  }

  // GVN creates patterns that use large integer or illegal types (i128, i256,
  // i48 etc.) from vectors of smaller types. The cases we see can be easily
  // modified to use extracts.
  if (LegalizeGVNBitCastPattern(m_builder, m_DL, I, &m_instructionsToRemove)) {
    if (I.use_empty()) {
      m_instructionsToRemove.push_back(&I);
    }
    return;
  }

  [&]() {
    // Example:
    // %y = trunc i64 %x to i48
    // %z = bitcast i48 %y to <3 x half>
    // ==>
    // %y = bitcast i64 %x to <4 x half>
    // %z = shufflevector <4 x half> %y, <4 x half> undef, <3 x i32> <i32 0, i32 1, i32 2>

    auto *pZ = &I;

    if (!pZ->getSrcTy()->isIntegerTy(48) && !pZ->getSrcTy()->isIntegerTy(24))
      return;

    if (!isa<VectorType>(pZ->getDestTy()))
      return;

    if (!isa<TruncInst>(pZ->getOperand(0)))
      return;

    auto *pVecTy = cast<IGCLLVM::FixedVectorType>(pZ->getDestTy());
    if (pVecTy->getNumElements() != 3)
      return;

    auto *pEltTy = pVecTy->getElementType();

    auto *pY = cast<TruncInst>(pZ->getOperand(0));
    auto *pX = pY->getOperand(0);

    if (!pX->getType()->isIntegerTy(64) && !pX->getType()->isIntegerTy(32))
      return;

    uint numElt =
        (unsigned int)pX->getType()->getPrimitiveSizeInBits() / (unsigned int)pEltTy->getPrimitiveSizeInBits();
    auto *pBCType = IGCLLVM::FixedVectorType::get(pEltTy, numElt);

    SmallVector<uint32_t, 4> maskVals;
    for (uint i = 0; i < pVecTy->getNumElements(); i++) {
      maskVals.push_back(i);
    }
    auto *pMask = ConstantDataVector::get(I.getContext(), maskVals);

    auto *pNewY = BitCastInst::CreateBitOrPointerCast(pX, pBCType, "", pZ);
    pNewY->setDebugLoc(pY->getDebugLoc());

    auto *pNewZ = new ShuffleVectorInst(pNewY, UndefValue::get(pBCType), pMask);
    pNewZ->insertAfter(pNewY);
    pNewZ->setDebugLoc(pZ->getDebugLoc());

    pZ->replaceAllUsesWith(pNewZ);
    pZ->eraseFromParent();

    if (pY->use_empty()) {
      pY->eraseFromParent();
    }

    // Legalize the shuffle vector that we just generated.
    visitShuffleVectorInst(*pNewZ);
  }();
}

void Legalization::visitSelectInst(SelectInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  if (I.getType()->isIntegerTy(1)) {
    llvm::Value *pCond = I.getOperand(0);
    llvm::Value *pSrc0 = I.getOperand(1);
    llvm::Value *pSrc1 = I.getOperand(2);
    LLVMContext &context = I.getContext();
    llvm::Instruction *pSrc0ZExt = llvm::CastInst::CreateZExtOrBitCast(pSrc0, Type::getInt32Ty(context), "", &I);
    pSrc0ZExt->setDebugLoc(I.getDebugLoc());

    llvm::Instruction *pSrc1ZExt = llvm::CastInst::CreateZExtOrBitCast(pSrc1, Type::getInt32Ty(context), "", &I);
    pSrc1ZExt->setDebugLoc(I.getDebugLoc());

    // Create a new Select instruction
    llvm::SelectInst *pNewSel = llvm::SelectInst::Create(pCond, pSrc0ZExt, pSrc1ZExt, "", &I);
    pNewSel->setDebugLoc(I.getDebugLoc());

    llvm::CastInst *pTruncInst = llvm::CastInst::CreateTruncOrBitCast(pNewSel, Type::getInt1Ty(context), "", &I);
    pTruncInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(pTruncInst);
    I.eraseFromParent();
  } else if (I.getType()->isDoubleTy() && (IGC_IS_FLAG_ENABLED(ForceDPEmulation) || m_ctx->platform.hasNoFP64Inst())) {
    // Split double select to i32 select.

    Value *lo[2];
    Value *hi[2];
    Type *intTy = Type::getInt32Ty(I.getContext());
    VectorType *vec2Ty = IGCLLVM::FixedVectorType::get(intTy, 2);
    Constant *Zero = ConstantInt::get(intTy, 0);
    Constant *One = ConstantInt::get(intTy, 1);
    m_builder->SetInsertPoint(&I);
    for (int i = 0; i < 2; ++i) {
      Value *twoi32 = m_builder->CreateBitCast(I.getOperand(i + 1), vec2Ty);
      lo[i] = m_builder->CreateExtractElement(twoi32, Zero);
      hi[i] = m_builder->CreateExtractElement(twoi32, One);
    }

    Value *new_lo = m_builder->CreateSelect(I.getCondition(), lo[0], lo[1]);
    Value *new_hi = m_builder->CreateSelect(I.getCondition(), hi[0], hi[1]);

    Value *newVal = m_builder->CreateInsertElement(UndefValue::get(vec2Ty), new_lo, Zero);
    newVal = m_builder->CreateInsertElement(newVal, new_hi, One);
    newVal = m_builder->CreateBitCast(newVal, I.getType());

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();
  } else if (I.getType()->isVectorTy()) {
    unsigned int vecSize = (unsigned)cast<IGCLLVM::FixedVectorType>(I.getType())->getNumElements();
    Value *newVec = UndefValue::get(I.getType());
    m_builder->SetInsertPoint(&I);
    for (unsigned int i = 0; i < vecSize; i++) {
      Value *idx = m_builder->getInt32(i);
      Value *condVal = I.getCondition();
      if (condVal->getType()->isVectorTy()) {
        condVal = m_builder->CreateExtractElement(condVal, idx);
      }
      Value *trueVal = m_builder->CreateExtractElement(I.getTrueValue(), idx);
      Value *falseVal = m_builder->CreateExtractElement(I.getFalseValue(), idx);
      Value *sel = m_builder->CreateSelect(condVal, trueVal, falseVal);
      newVec = m_builder->CreateInsertElement(newVec, sel, idx);
    }
    I.replaceAllUsesWith(newVec);
    I.eraseFromParent();
  }
}

void Legalization::visitPHINode(PHINode &phi) {
  m_ctx->m_instrTypes.numInsts++;
  // break down phi of i1
  LLVMContext &context = phi.getContext();
  if (phi.getType()->isIntegerTy(1)) {
    unsigned int nbOperand = phi.getNumOperands();
    Type *newType = Type::getInt32Ty(context);
    PHINode *newPhi = PHINode::Create(newType, nbOperand, "", &phi);
    newPhi->setDebugLoc(phi.getDebugLoc());
    for (unsigned int i = 0; i < nbOperand; i++) {
      Value *source = phi.getOperand(i);
      Instruction *terminator = phi.getIncomingBlock(i)->getTerminator();
      m_builder->SetInsertPoint(terminator);
      Value *newSource = m_builder->CreateSExt(source, newType);
      newPhi->addIncoming(newSource, phi.getIncomingBlock(i));
    }
    Instruction *boolean = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, newPhi, ConstantInt::get(newType, 0),
                                           "", phi.getParent()->getFirstNonPHI());
    boolean->setDebugLoc(phi.getDebugLoc());
    phi.replaceAllUsesWith(boolean);
    phi.eraseFromParent();
  }
}

static Value *GetMaskedValue(IRBuilder<> *IRB, bool Signed, Value *Src, Type *Ty) {
  IntegerType *SrcITy = dyn_cast<IntegerType>(Src->getType());
  IntegerType *ITy = dyn_cast<IntegerType>(Ty);

  IGC_ASSERT_MESSAGE(SrcITy, "The source integer must be wider than the target integer.");
  IGC_ASSERT_MESSAGE(ITy, "The source integer must be wider than the target integer.");
  IGC_ASSERT_MESSAGE(SrcITy->getBitWidth() > ITy->getBitWidth(),
                     "The source integer must be wider than the target integer.");

  if (!Signed) // For unsigned value, just mask off non-significant bits.
    return IRB->CreateAnd(Src, ITy->getBitMask());

  auto ShAmt = SrcITy->getBitWidth() - ITy->getBitWidth();
  return IRB->CreateAShr(IRB->CreateShl(Src, ShAmt), ShAmt);
}

void Legalization::visitICmpInst(ICmpInst &IC) {
  Value *Op0 = IC.getOperand(0);
  Value *Op1 = IC.getOperand(1);
  Type *Ty = Op0->getType();
  if (Ty->isIntegerTy(1)) {
    Instruction *operand0_i8 =
        CastInst::CreateIntegerCast(Op0, Type::getInt8Ty(IC.getContext()), IC.isSigned(), "", &IC);
    operand0_i8->setDebugLoc(IC.getDebugLoc());
    Instruction *operand1_i8 =
        CastInst::CreateIntegerCast(Op1, Type::getInt8Ty(IC.getContext()), IC.isSigned(), "", &IC);
    operand1_i8->setDebugLoc(IC.getDebugLoc());
    IRBuilder<> m_build(&IC);
    Value *new_IC = m_build.CreateICmp(IC.getPredicate(), operand0_i8, operand1_i8, "");
    IC.replaceAllUsesWith(new_IC);
    IC.eraseFromParent();
  }

  if (Ty->isIntegerTy() && m_DL->isIllegalInteger(Ty->getIntegerBitWidth()) && isa<TruncInst>(Op0) &&
      isa<ConstantInt>(Op1)) {
    // Legalize
    //
    // (icmp (trunc i32 to i28) C)
    //
    // TODO: It should be straightforward to supoprt other cases.
    //
    TruncInst *TI = cast<TruncInst>(Op0);
    Value *Src = TI->getOperand(0);
    Type *SrcTy = Src->getType();

    m_builder->SetInsertPoint(&IC);

    Value *NOp0 = GetMaskedValue(m_builder, IC.isSigned(), Src, Ty);
    Value *NOp1 = IC.isSigned() ? m_builder->CreateSExt(Op1, SrcTy) : m_builder->CreateZExt(Op1, SrcTy);
    Value *NCmp = m_builder->CreateICmp(IC.getPredicate(), NOp0, NOp1);
    IC.replaceAllUsesWith(NCmp);
    IC.eraseFromParent();
  }
}

Value *Legalization::addFCmpWithORD(FCmpInst &FC) {
  m_builder->SetInsertPoint(&FC);

  // Are both sources Not NaN's ?
  //  %c = fcmp ord %a %b
  //  =>
  //  %1 = fcmp oeq %a %a
  //  %2 = fcmp oeq %b %b
  //  %c = and %1 %2

  Value *Op0 = FC.getOperand(0);
  Value *Op1 = FC.getOperand(1);

  return m_builder->CreateAnd(m_builder->CreateFCmpOEQ(Op0, Op0), m_builder->CreateFCmpOEQ(Op1, Op1));
}

Value *Legalization::addFCmpWithUNO(FCmpInst &FC) {
  // Is any of the sources NaN's
  //  %c = fcmp uno %a %b
  //  =>
  //  %1 = fcmp une %a %a
  //  %2 = fcmp une %b %b
  //  %c = or %1 %2
  Value *src0 = FC.getOperand(0);
  Value *src1 = FC.getOperand(1);

  if (isa<ConstantFP>(src0))
    std::swap(src0, src1);

  Instruction *c0 = FCmpInst::Create(Instruction::FCmp, CmpInst::FCMP_UNE, src0, src0, "", &FC);
  c0->setDebugLoc(FC.getDebugLoc());

  if (ConstantFP *CFP = dyn_cast<ConstantFP>(src1)) {
    if (!CFP->isNaN())
      return c0;
  }

  Instruction *c1 = FCmpInst::Create(Instruction::FCmp, CmpInst::FCMP_UNE, src1, src1, "", &FC);
  c1->setDebugLoc(FC.getDebugLoc());

  Instruction *isAnySourceUnordered = llvm::BinaryOperator::CreateOr(c0, c1, "", &FC);
  isAnySourceUnordered->setDebugLoc(FC.getDebugLoc());

  return isAnySourceUnordered;
}

void Legalization::visitFCmpInstUndorderedPredicate(FCmpInst &FC) {
  Value *result = nullptr;
  switch (FC.getPredicate()) {
  case CmpInst::FCMP_ORD:
    result = addFCmpWithORD(FC);
    break;
  case CmpInst::FCMP_UNO:
    result = addFCmpWithUNO(FC);
    break;
  case CmpInst::FCMP_ONE: {
    // %c = fcmp one %a %b
    // =>
    // %1 = fcmp ord %a %b
    // %2 = fcmp une %a %b
    // %c = and %1 %2
    Value *sourcesOrdered = addFCmpWithORD(FC);
    Instruction *fcmpNotEqual =
        FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_UNE, FC.getOperand(0), FC.getOperand(1), "", &FC);
    fcmpNotEqual->setDebugLoc(FC.getDebugLoc());
    result = llvm::BinaryOperator::CreateAnd(sourcesOrdered, fcmpNotEqual, "", &FC);
  } break;
  case CmpInst::FCMP_UEQ: {
    // %c = fcmp ueq %a %b
    // =>
    // %1 = fcmp uno %a %b
    // %2 = fcmp oeq %a %b
    // %c = or %1 %2
    Value *sourcesUnordered = addFCmpWithUNO(FC);
    Instruction *fcmpEqual =
        FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_OEQ, FC.getOperand(0), FC.getOperand(1), "", &FC);
    fcmpEqual->setDebugLoc(FC.getDebugLoc());
    result = llvm::BinaryOperator::CreateOr(sourcesUnordered, fcmpEqual, "", &FC);
  } break;
  case CmpInst::FCMP_UGE:
  case CmpInst::FCMP_UGT:
  case CmpInst::FCMP_ULE:
  case CmpInst::FCMP_ULT: {
    // To handle Unordered predicates, convert them to inverted ordered
    // and than not the result
    //  e.g. %c = fcmp uge %a %b
    //       =>
    //       %1 = fcmp olt %a %b
    //       %c = not %1
    Instruction *invertedOrderedInst =
        FCmpInst::Create(Instruction::FCmp, FCmpInst::getInversePredicate(FC.getPredicate()), FC.getOperand(0),
                         FC.getOperand(1), "", &FC);
    invertedOrderedInst->setDebugLoc(FC.getDebugLoc());

    while (!FC.user_empty()) {
      auto I = FC.user_begin();
      if (SelectInst *s = dyn_cast<SelectInst>(*I)) {
        // check whether FC is condition
        if (s->getOperand(0) == &FC) {
          Value *trueValue = s->getTrueValue();
          Value *falseValue = s->getFalseValue();
          s->setOperand(1, falseValue);
          s->setOperand(2, trueValue);
          s->setOperand(0, invertedOrderedInst);
        } else {
          break;
        }
      } else if (BranchInst *br = dyn_cast<BranchInst>(*I)) {
        IGC_ASSERT(br->isConditional());
        br->swapSuccessors();
        br->setCondition(invertedOrderedInst);
      } else {
        break;
      }
    }

    if (!FC.use_empty()) {
      result = llvm::BinaryOperator::CreateNot(invertedOrderedInst, "", &FC);
    } else {
      FC.eraseFromParent();
    }

  } break;
  default:
    break;
  }

  if (result) {
    auto resultAsTempInst = dyn_cast<Instruction>(result);
    if (resultAsTempInst)
      resultAsTempInst->setDebugLoc(FC.getDebugLoc());
    FC.replaceAllUsesWith(result);
    FC.eraseFromParent();
  }
}

void Legalization::visitFCmpInst(FCmpInst &FC) {
  m_ctx->m_instrTypes.numInsts++;
  // Handling NaN's for FCmp.
  if (FCmpInst::isUnordered(FC.getPredicate()) || FC.getPredicate() == CmpInst::FCMP_ORD ||
      FC.getPredicate() == CmpInst::FCMP_ONE) {
    if ((m_preserveNan || PreserveNan) && !FC.isFast()) {
      visitFCmpInstUndorderedPredicate(FC);
    } else if (m_preserveNanCheck && isNaNCheck(FC)) {
      visitFCmpInstUndorderedPredicate(FC);
    } else {
      visitFCmpInstUndorderedFlushNan(FC);
    }
  }
}

CmpInst::Predicate getOrderedPredicate(CmpInst::Predicate pred) {
  switch (pred) {
  case CmpInst::FCMP_UEQ:
    return CmpInst::FCMP_OEQ;
  case CmpInst::FCMP_UNE:
    return CmpInst::FCMP_ONE;
  case CmpInst::FCMP_UGT:
    return CmpInst::FCMP_OGT;
  case CmpInst::FCMP_ULT:
    return CmpInst::FCMP_OLT;
  case CmpInst::FCMP_UGE:
    return CmpInst::FCMP_OGE;
  case CmpInst::FCMP_ULE:
    return CmpInst::FCMP_OLE;
  default:
    IGC_ASSERT_MESSAGE(0, "wrong predicate");
    break;
  }
  return pred;
}

// legalize compare predicate ignoring Nan
void Legalization::visitFCmpInstUndorderedFlushNan(FCmpInst &FC) {
  Value *result = nullptr;
  switch (FC.getPredicate()) {
  case CmpInst::FCMP_ORD:
    result = ConstantInt::getTrue(FC.getType());
    break;
  case CmpInst::FCMP_UNO:
    result = ConstantInt::getFalse(FC.getType());
    break;
  case CmpInst::FCMP_ONE:
    result = FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_UNE, FC.getOperand(0), FC.getOperand(1), "", &FC);
    cast<Instruction>(result)->setFastMathFlags(FC.getFastMathFlags());
    break;
  case CmpInst::FCMP_UEQ:
    result = FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_OEQ, FC.getOperand(0), FC.getOperand(1), "", &FC);
    cast<Instruction>(result)->setFastMathFlags(FC.getFastMathFlags());
    break;
  case CmpInst::FCMP_UGE:
  case CmpInst::FCMP_UGT:
  case CmpInst::FCMP_ULE:
  case CmpInst::FCMP_ULT:
    result = FCmpInst::Create(Instruction::FCmp, getOrderedPredicate(FC.getPredicate()), FC.getOperand(0),
                              FC.getOperand(1), "", &FC);
    cast<Instruction>(result)->setFastMathFlags(FC.getFastMathFlags());
    break;
  default:
    break;
  }

  if (result) {
    auto tempInst = dyn_cast<Instruction>(result);
    if (tempInst)
      tempInst->setDebugLoc(FC.getDebugLoc());
    FC.replaceAllUsesWith(result);
    FC.eraseFromParent();
  }
}

void Legalization::visitStoreInst(StoreInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  if (ConstantDataVector *vec = dyn_cast<ConstantDataVector>(I.getOperand(0))) {
    Value *newVec = UndefValue::get(vec->getType());
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getElementAsConstant(i);
      if (!isa<UndefValue>(cst)) {
        newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(I.getDebugLoc());
      }
    }
    I.setOperand(0, newVec);
  } else if (ConstantVector *vec = dyn_cast<ConstantVector>(I.getOperand(0))) {
    Value *newVec = UndefValue::get(vec->getType());
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getOperand(i);
      if (!isa<UndefValue>(cst)) {
        newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(I.getDebugLoc());
      }
    }
    I.setOperand(0, newVec);
  } else if (ConstantAggregateZero *vec = dyn_cast<ConstantAggregateZero>(I.getOperand(0))) {
    auto *vecTy = vec->getType();
    IGC_ASSERT_MESSAGE(isa<FixedVectorType>(vecTy), "Unexpected aggregate type");
    Value *newVec = UndefValue::get(vecTy);
    unsigned nbElement = cast<FixedVectorType>(vecTy)->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getElementValue(i);
      if (!isa<UndefValue>(cst)) {
        newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(I.getDebugLoc());
      }
    }
    I.setOperand(0, newVec);
  } else if (I.getOperand(0)->getType()->isIntegerTy(1)) {
    m_builder->SetInsertPoint(&I);
    Value *newVal = m_builder->CreateZExt(I.getOperand(0), m_builder->getInt8Ty());

    PointerType *ptrTy = cast<PointerType>(I.getPointerOperand()->getType());
    unsigned addressSpace = ptrTy->getAddressSpace();
    PointerType *I8PtrTy = m_builder->getInt8PtrTy(addressSpace);
    Value *I8PtrOp = m_builder->CreateBitCast(I.getPointerOperand(), I8PtrTy);

    IGC::cloneStore(&I, newVal, I8PtrOp);
    I.eraseFromParent();
  } else if (I.getOperand(0)->getType()->isIntegerTy()) {
    m_builder->SetInsertPoint(&I);

    unsigned srcWidth = I.getOperand(0)->getType()->getScalarSizeInBits();
    if (IGC_IS_FLAG_ENABLED(EnableTestSplitI64)) {
      // Store with element size 64bit/32bit/16bit/8bit are always good.
      // (Note that VectorPreProcess/VectorProcess will legalize them.)
      if (srcWidth == 64 || srcWidth == 32 || srcWidth == 16 || srcWidth == 8)
        return;
    }
    if (m_DL->isLegalInteger(srcWidth)) // nothing to legalize
      return;

    // Find largest legal int size to break into vectors
    unsigned intSize = 0;
    for (unsigned i = m_DL->getLargestLegalIntTypeSizeInBits(); i >= 8; i >>= 1) {
      if (srcWidth % i == 0) {
        intSize = i;
        break;
      }
    }
    if (intSize == 0) // unaligned sizes not supported
      return;

    Type *legalTy = IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), intSize), srcWidth / intSize);
    Instruction *storeVal = BitCastInst::Create(Instruction::BitCast, I.getOperand(0), legalTy, "", &I);
    storeVal->setDebugLoc(I.getDebugLoc());
    Value *storePtr = I.getPointerOperand();

    IGC_ASSERT(nullptr != storePtr);
    IGC_ASSERT(nullptr != storePtr->getType());

    PointerType *ptrTy = PointerType::get(legalTy, storePtr->getType()->getPointerAddressSpace());
    IntToPtrInst *intToPtr = dyn_cast<IntToPtrInst>(storePtr);

    if (intToPtr) {
      // Direct cast int to the legal type
      storePtr = IntToPtrInst::Create(Instruction::CastOps::IntToPtr, intToPtr->getOperand(0), ptrTy, "", &I);
    } else {
      storePtr = BitCastInst::CreatePointerCast(storePtr, ptrTy, "", &I);
    }
    Instruction *storePtrAsTempInst = dyn_cast<Instruction>(storePtr);
    if (storePtrAsTempInst)
      storePtrAsTempInst->setDebugLoc(I.getDebugLoc());
    IGC::cloneStore(&I, storeVal, storePtr);
    I.eraseFromParent();

    if (intToPtr && intToPtr->use_empty()) {
      intToPtr->eraseFromParent();
    }
  }
}

void Legalization::visitLoadInst(LoadInst &I) {
  if (I.getType()->isIntegerTy(1)) {
    m_builder->SetInsertPoint(&I);
    PointerType *ptrTy = cast<PointerType>(I.getPointerOperand()->getType());
    unsigned addressSpace = ptrTy->getAddressSpace();
    PointerType *I8PtrTy = m_builder->getInt8PtrTy(addressSpace);
    Value *I8PtrOp = m_builder->CreateBitCast(I.getPointerOperand(), I8PtrTy);

    LoadInst *pNewLoadInst = IGC::cloneLoad(&I, m_builder->getInt8Ty(), I8PtrOp);
    Value *newVal = m_builder->CreateTrunc(pNewLoadInst, I.getType());
    I.replaceAllUsesWith(newVal);
  }
}

void Legalization::PromoteInsertElement(Value *I, Value *newVec) {
  if (InsertElementInst *IEinst = dyn_cast<InsertElementInst>(I)) {
    m_builder->SetInsertPoint(IEinst);
    newVec = InsertElementInst::Create(newVec,
                                       m_builder->CreateSExt(IEinst->getOperand(1), Type::getInt32Ty(I->getContext())),
                                       IEinst->getOperand(2), "", IEinst);
    Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
    if (newVecAsTempInst)
      newVecAsTempInst->setDebugLoc(IEinst->getDebugLoc());

    for (Value::user_iterator useI = I->user_begin(), useE = I->user_end(); useI != useE; ++useI) {
      PromoteInsertElement(*useI, newVec);
    }

  } else if (ExtractElementInst *EEinst = dyn_cast<ExtractElementInst>(I)) {
    newVec = ExtractElementInst::Create(newVec, EEinst->getOperand(1), "", EEinst);
    Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
    if (newVecAsTempInst)
      newVecAsTempInst->setDebugLoc(EEinst->getDebugLoc());

    for (Value::user_iterator useI = I->user_begin(), useE = I->user_end(); useI != useE; ++useI) {
      CastInst *castI = dyn_cast<CastInst>(*useI);
      if (castI && castI->getOpcode() == Instruction::SExt && castI->getSrcTy()->isIntegerTy(1) &&
          castI->getDestTy()->isIntegerTy(32)) {
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(castI->getDebugLoc());
        castI->replaceAllUsesWith(newVec);
      } else {
        llvm::Instruction *pSrc1ZExt =
            llvm::CastInst::CreateTruncOrBitCast(newVec, Type::getInt1Ty(I->getContext()), "", EEinst);
        pSrc1ZExt->setDebugLoc(EEinst->getDebugLoc());
        I->replaceAllUsesWith(pSrc1ZExt);
      }
    }
  }
}

void Legalization::visitInsertElementInst(InsertElementInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  if (ConstantDataVector *vec = dyn_cast<ConstantDataVector>(I.getOperand(0))) {
    Value *newVec = UndefValue::get(vec->getType());
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getElementAsConstant(i);
      if (!isa<UndefValue>(cst)) {
        newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(I.getDebugLoc());
      }
    }
    newVec = InsertElementInst::Create(newVec, I.getOperand(1), I.getOperand(2), "", &I);
    Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
    if (newVecAsTempInst)
      newVecAsTempInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(newVec);
  } else if (ConstantVector *vec = dyn_cast<ConstantVector>(I.getOperand(0))) {
    Value *newVec = UndefValue::get(I.getType());
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getOperand(i);
      if (!isa<UndefValue>(cst)) {
        newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
        Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
        if (newVecAsTempInst)
          newVecAsTempInst->setDebugLoc(I.getDebugLoc());
      }
    }
    newVec = InsertElementInst::Create(newVec, I.getOperand(1), I.getOperand(2), "", &I);
    Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
    if (newVecAsTempInst)
      newVecAsTempInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(newVec);
  } else if (ConstantAggregateZero *vec = dyn_cast<ConstantAggregateZero>(I.getOperand(0))) {
    Value *newVec = UndefValue::get(I.getType());
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned int i = 0; i < nbElement; i++) {
      Constant *cst = vec->getElementValue(i);
      newVec = InsertElementInst::Create(newVec, cst, ConstantInt::get(Type::getInt32Ty(I.getContext()), i), "", &I);
      Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
      if (newVecAsTempInst)
        newVecAsTempInst->setDebugLoc(I.getDebugLoc());
    }
    newVec = InsertElementInst::Create(newVec, I.getOperand(1), I.getOperand(2), "", &I);
    Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
    if (newVecAsTempInst)
      newVecAsTempInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(newVec);
  } else if (I.getOperand(1)->getType()->isIntegerTy(1)) {
    // This promotes i1 insertelement to i32
    unsigned int nbElement = (unsigned)cast<IGCLLVM::FixedVectorType>(I.getOperand(0)->getType())->getNumElements();
    Value *newVec = UndefValue::get(IGCLLVM::FixedVectorType::get(m_builder->getInt32Ty(), nbElement));
    PromoteInsertElement(&I, newVec);
  }
}

void Legalization::visitShuffleVectorInst(ShuffleVectorInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  // Replace the shuffle with a series of inserts.
  // If the original vector is a constant, just use the scalar constant,
  // otherwise extract from the original vector.

  IGCLLVM::FixedVectorType *resType = cast<IGCLLVM::FixedVectorType>(I.getType());
  Value *newVec = UndefValue::get(resType);
  Value *src0 = I.getOperand(0);
  Value *src1 = I.getOperand(1);
  // The mask is guaranteed by the LLVM IR spec to be constant
  Constant *mask = IGCLLVM::getShuffleMaskForBitcode(&I);
  // The two inputs are guaranteed to be of the same type
  IGCLLVM::FixedVectorType *inType = cast<IGCLLVM::FixedVectorType>(src0->getType());
  int inCount = int_cast<int>(inType->getNumElements());
  int inBase = 2; // 2 means using undef
  // if inType == resType, use src0/src1 as the input
  if (inType == resType) {
    int srcMatch0 = 0;
    int srcMatch1 = 0;
    for (unsigned int dstIndex = 0; dstIndex < resType->getNumElements(); ++dstIndex) {
      // The mask value can be either an integer or undef.
      // If it's undef, do nothing.
      // Otherwise, create an insert with the appropriate value.
      ConstantInt *index = dyn_cast<ConstantInt>(mask->getAggregateElement(dstIndex));
      if (index) {
        int indexVal = int_cast<int>(index->getZExtValue());
        if (indexVal == dstIndex)
          srcMatch0++;
        else if (indexVal == inCount + dstIndex)
          srcMatch1++;
      }
    }
    if (srcMatch0 > srcMatch1 && srcMatch0 > 0) {
      newVec = src0;
      inBase = 0;
    } else if (srcMatch1 > srcMatch0 && srcMatch1 > 0) {
      inBase = 1;
      newVec = src1;
    }
  }

  for (unsigned int dstIndex = 0; dstIndex < resType->getNumElements(); ++dstIndex) {
    // The mask value can be either an integer or undef.
    // If it's undef, do nothing.
    // Otherwise, create an insert with the appropriate value.
    ConstantInt *index = dyn_cast<ConstantInt>(mask->getAggregateElement(dstIndex));
    if (index) {
      int indexVal = int_cast<int>(index->getZExtValue());

      if (inBase == 0 && indexVal == dstIndex)
        continue;
      else if (inBase == 1 && indexVal == dstIndex + inCount)
        continue;

      Value *srcVector = nullptr;
      int srcIndex = 0;
      if (indexVal < inCount) {
        srcVector = src0;
        srcIndex = indexVal;
      } else {
        srcVector = src1;
        srcIndex = indexVal - inCount;
      }

      // If the source is a constant vector (undef counts) just get the scalar
      // constant and insert that. Otherwise, add an extract from the appropriate
      // index.
      Value *srcVal = nullptr;
      Constant *constSrc = dyn_cast<Constant>(srcVector);
      if (constSrc) {
        srcVal = constSrc->getAggregateElement(srcIndex);
      } else {
        // Try to find the original inserted value.
        srcVal = findInsert(srcVector, srcIndex);

        // If we couldn't find it, just create a new extract.
        if (!srcVal) {
          srcVal = ExtractElementInst::Create(srcVector, ConstantInt::get(index->getType(), srcIndex), "", &I);
          Instruction *srcValAsTempInst = dyn_cast<Instruction>(srcVal);
          if (srcValAsTempInst)
            srcValAsTempInst->setDebugLoc(I.getDebugLoc());
        }
      }

      newVec = InsertElementInst::Create(newVec, srcVal, ConstantInt::get(index->getType(), dstIndex), "", &I);
      Instruction *newVecAsTempInst = dyn_cast<Instruction>(newVec);
      if (newVecAsTempInst)
        newVecAsTempInst->setDebugLoc(I.getDebugLoc());
    }
  }
  auto newVecAsTempInst = dyn_cast<Instruction>(newVec);
  if (newVecAsTempInst)
    newVecAsTempInst->setDebugLoc(I.getDebugLoc());
  I.replaceAllUsesWith(newVec);
  I.eraseFromParent();
}

llvm::Value *Legalization::findInsert(llvm::Value *vector, unsigned int index) {
  // If the vector was constructed by a chain of inserts,
  // walk up the chain until we find the correct value.
  InsertElementInst *IE = dyn_cast<InsertElementInst>(vector);
  while (IE) {
    ConstantInt *indexOp = dyn_cast<ConstantInt>(IE->getOperand(2));
    // There was a non-constant index, so all bets are off
    if (!indexOp)
      return nullptr;

    uint insertIndex = static_cast<uint>(indexOp->getZExtValue());
    if (insertIndex == index)
      return IE->getOperand(1);

    IE = dyn_cast<InsertElementInst>(IE->getOperand(0));
  }

  // Couldn't find an insert, so the index did not change from the initial
  // value of the chain.
  return nullptr;
}

Value *Cast(Value *val, Type *type, Instruction *insertBefore) {
  Instruction *newVal = nullptr;
  if (type->isIntegerTy()) {
    newVal = CastInst::CreateIntegerCast(val, type, false, "", insertBefore);
  } else if (type->isFloatingPointTy()) {
    newVal = CastInst::CreateFPCast(val, type, "", insertBefore);
  } else {
    IGC_ASSERT_MESSAGE(0, "unexpected type");
  }
  newVal->setDebugLoc(insertBefore->getDebugLoc());
  return newVal;
}

void Legalization::RecursivelyChangePointerType(Instruction *oldPtr, llvm::Type *Ty, Instruction *newPtr) {
  for (auto II = oldPtr->user_begin(), IE = oldPtr->user_end(); II != IE; ++II) {
    Value *newVal = nullptr;
    if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*II)) {
      SmallVector<Value *, 8> Idx(gep->idx_begin(), gep->idx_end());
      Type *BaseTy = Ty;
      GetElementPtrInst *newGep = GetElementPtrInst::Create(BaseTy, newPtr, Idx, "", gep);
      newGep->setDebugLoc(gep->getDebugLoc());
      RecursivelyChangePointerType(gep, newGep->getResultElementType(), newGep);
    } else if (LoadInst *load = dyn_cast<LoadInst>(*II)) {
      Instruction *newLoad = IGC::cloneLoad(load, Ty, newPtr);
      newVal = Cast(newLoad, load->getType(), load->getNextNode());
      auto tempInst = dyn_cast<Instruction>(newVal);
      if (tempInst)
        tempInst->setDebugLoc(load->getDebugLoc());
      load->replaceAllUsesWith(newVal);
    } else if (StoreInst *store = dyn_cast<StoreInst>(*II)) {
      Value *StoredValue = store->getValueOperand();
      Value *newData = Cast(StoredValue, Ty, store);
      IGC::cloneStore(store, newData, newPtr);
    } else if (CastInst *cast = dyn_cast<CastInst>(*II)) {
      Instruction *newCast = CastInst::CreatePointerCast(newPtr, cast->getType(), "", cast);
      newCast->setDebugLoc(cast->getDebugLoc());
      cast->replaceAllUsesWith(newCast);
    }
    // We cannot delete any instructions as the visitor
    m_instructionsToRemove.push_back(cast<Instruction>(*II));
  }
}

Type *Legalization::LegalStructAllocaType(Type *type) const {
  // Recursively legalize the struct type
  StructType *StTy = cast<StructType>(type);
  SmallVector<Type *, 8> Elems;
  bool IsIllegal = false;
  for (auto I = StTy->element_begin(), IE = StTy->element_end(); I != IE; ++I) {
    Type *LegalTy = LegalAllocaType(*I);
    Elems.push_back(LegalTy);
    IsIllegal = IsIllegal || LegalTy != *I;
  }
  if (IsIllegal) {
    type = StructType::get(type->getContext(), Elems);
  }
  return type;
}

Type *Legalization::LegalAllocaType(Type *type) const {
  Type *legalType = type;
  switch (type->getTypeID()) {
  case Type::IntegerTyID:
    if (type->isIntegerTy(1)) {
      unsigned int size = int_cast<unsigned int>(m_DL->getTypeAllocSizeInBits(type));
      legalType = Type::getIntNTy(type->getContext(), size);
    }
    break;
  case Type::ArrayTyID:
    legalType = ArrayType::get(LegalAllocaType(cast<ArrayType>(type)->getElementType()), type->getArrayNumElements());
    break;
  case Type::FixedVectorTyID:
    legalType = IGCLLVM::FixedVectorType::get(LegalAllocaType(cast<VectorType>(type)->getElementType()),
                                              (unsigned)cast<IGCLLVM::FixedVectorType>(type)->getNumElements());
    break;
  case Type::StructTyID:
    return LegalStructAllocaType(type);
  case Type::HalfTyID:
  case Type::FloatTyID:
  case Type::DoubleTyID:
  case Type::PointerTyID:
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "Alloca of unsupported type");
    break;
  }
  return legalType;
}

void Legalization::visitAlloca(AllocaInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  Type *type = I.getAllocatedType();
  Type *legalAllocaType = LegalAllocaType(type);
  if (type != legalAllocaType) {
    // Remaining alloca of i1 need to be promoted
    AllocaInst *newAlloca = new AllocaInst(legalAllocaType, 0, "", &I);
    RecursivelyChangePointerType(&I, legalAllocaType, newAlloca);
    m_instructionsToRemove.push_back(&I);
  }
}

void Legalization::visitIntrinsicInst(llvm::IntrinsicInst &I) {
  m_ctx->m_instrTypes.numInsts++;
  IGCLLVM::IRBuilder<> Builder(&I);

  auto intrinsicID = I.getIntrinsicID();

  switch (intrinsicID) {
  case Intrinsic::usub_sat:
  case Intrinsic::ssub_sat:
  case Intrinsic::uadd_sat:
  case Intrinsic::sadd_sat: {
    llvm::Intrinsic::ID OverflowIntrinID = Intrinsic::not_intrinsic;
    switch (I.getIntrinsicID()) {
    case Intrinsic::usub_sat:
      OverflowIntrinID = Intrinsic::usub_with_overflow;
      break;
    case Intrinsic::ssub_sat:
      OverflowIntrinID = Intrinsic::ssub_with_overflow;
      break;
    case Intrinsic::uadd_sat:
      OverflowIntrinID = Intrinsic::uadd_with_overflow;
      break;
    case Intrinsic::sadd_sat:
      OverflowIntrinID = Intrinsic::sadd_with_overflow;
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Incorrect intrinsic");
      break;
    }

    int BitWidth = I.getType()->getIntegerBitWidth();
    auto OverFlowIntrin =
        Builder.CreateIntrinsic(OverflowIntrinID, {I.getArgOperand(0)->getType(), I.getArgOperand(1)->getType()},
                                {I.getArgOperand(0), I.getArgOperand(1)});
    Value *Result = Builder.CreateExtractValue(OverFlowIntrin, (uint64_t)0);
    Value *Overflow = Builder.CreateExtractValue(OverFlowIntrin, (uint64_t)1);

    Value *Boundary = nullptr;
    switch (I.getIntrinsicID()) {
    case Intrinsic::usub_sat:
      Boundary = Builder.getInt(APInt::getMinValue(BitWidth));
      break;
    case Intrinsic::ssub_sat: {
      Value *isMaxOrMinOverflow = Builder.CreateICmpSLT(Builder.getIntN(BitWidth, 0), I.getArgOperand(1));
      APInt MinVal = APInt::getSignedMinValue(BitWidth);
      APInt MaxVal = APInt::getSignedMaxValue(BitWidth);
      Boundary = Builder.CreateSelect(isMaxOrMinOverflow, Builder.getInt(MinVal), Builder.getInt(MaxVal));
    } break;
    case Intrinsic::uadd_sat:
      Boundary = Builder.getInt(APInt::getMaxValue(BitWidth));
      break;
    case Intrinsic::sadd_sat: {
      Value *isMaxOrMinOverflow = Builder.CreateICmpSLT(Builder.getIntN(BitWidth, 0), I.getArgOperand(1));
      APInt MinVal = APInt::getSignedMinValue(BitWidth);
      APInt MaxVal = APInt::getSignedMaxValue(BitWidth);
      Boundary = Builder.CreateSelect(isMaxOrMinOverflow, Builder.getInt(MaxVal), Builder.getInt(MinVal));
    } break;
    default:
      IGC_ASSERT_MESSAGE(0, "Incorrect intrinsic");
      break;
    }

    Value *Saturated = Builder.CreateSelect(Overflow, Boundary, Result);
    I.replaceAllUsesWith(Saturated);
    I.eraseFromParent();
    visit(*OverFlowIntrin);
  } break;

  case Intrinsic::sadd_with_overflow:
  case Intrinsic::usub_with_overflow:
  case Intrinsic::ssub_with_overflow:
  case Intrinsic::uadd_with_overflow:
  case Intrinsic::umul_with_overflow:
  case Intrinsic::smul_with_overflow: {
    Value *src0 = I.getArgOperand(0);
    Value *src1 = I.getArgOperand(1);

    Instruction *res = nullptr;
    Instruction *isOverflow = nullptr;

    switch (intrinsicID) {
    case Intrinsic::uadd_with_overflow:
      res = BinaryOperator::Create(Instruction::Add, src0, src1, "", &I);
      // Unsigned a + b overflows if a + b < a (for an unsigned comparison)
      isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_ULT, res, src0, "", &I);
      break;
    case Intrinsic::usub_with_overflow:
      res = BinaryOperator::Create(Instruction::Sub, src0, src1, "", &I);
      // Unsigned a - b overflows if a - b > a (for an unsigned comparison)
      isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_UGT, res, src0, "", &I);
      break;
    case Intrinsic::sadd_with_overflow:
    case Intrinsic::ssub_with_overflow: {
      Instruction *usrc0 = BitCastInst::CreateZExtOrBitCast(src0, src0->getType(), "", &I);
      usrc0->setDebugLoc(I.getDebugLoc());
      Instruction *usrc1 = BitCastInst::CreateZExtOrBitCast(src1, src1->getType(), "", &I);
      usrc1->setDebugLoc(I.getDebugLoc());
      res = BinaryOperator::Create(intrinsicID == Intrinsic::sadd_with_overflow ? Instruction::Add : Instruction::Sub,
                                   usrc0, usrc1, "", &I);
      if (intrinsicID == Intrinsic::ssub_with_overflow) {
        usrc1 = BinaryOperator::CreateNot(usrc1, "", &I);
        usrc1->setDebugLoc(I.getDebugLoc());
      }
      Instruction *usrc0_xor_usrc1 = BinaryOperator::Create(Instruction::Xor, usrc0, usrc1, "", &I);
      usrc0_xor_usrc1->setDebugLoc(I.getDebugLoc());
      Instruction *res_xor_usrc0 = BinaryOperator::Create(Instruction::Xor, res, usrc0, "", &I);
      res_xor_usrc0->setDebugLoc(I.getDebugLoc());
      Instruction *negOpt = BinaryOperator::CreateNot(usrc0_xor_usrc1, "", &I);
      negOpt->setDebugLoc(I.getDebugLoc());
      Instruction *andOpt = BinaryOperator::CreateAnd(negOpt, res_xor_usrc0, "", &I);
      andOpt->setDebugLoc(I.getDebugLoc());
      auto zero = ConstantInt::get(src0->getType(), 0, true);
      // Signed a - b overflows if the sign of a and -b are the same, but diffrent from the result
      // Signed a + b overflows if the sign of a and b are the same, but diffrent from the result
      if (src0->getType()->getIntegerBitWidth() == 8 && !m_ctx->platform.supportByteALUOperation()) {
        // The promotion char->short is breaking the current logic for overflow algorithm.
        // For ex. in PVC we are losing the sign bit here, if we are operating on signed char.
        // In first half of the short (the original char) we still have the correct sign bit -
        // so we are checking if original char has sign bit light-on.
        auto charSignBit = ConstantInt::get(Builder.getInt8Ty(), 1 << 7, true);
        andOpt = BinaryOperator::CreateAnd(andOpt, charSignBit, "", &I);
        andOpt->setDebugLoc(I.getDebugLoc());
        isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, andOpt, zero, "", &I);
      } else {
        isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_SLT, andOpt, zero, "", &I);
      }
    } break;
    case Intrinsic::umul_with_overflow:
    case Intrinsic::smul_with_overflow: {
      IGC_ASSERT(nullptr != src0);
      IGC_ASSERT(nullptr != src1);
      IGC_ASSERT(src0->getType() == src1->getType());
      IGC_ASSERT(src0->getType()->isIntegerTy());
      bool isSigned = intrinsicID == Intrinsic::smul_with_overflow;
      res = BinaryOperator::Create(Instruction::Mul, src0, src1, "", &I);
      const unsigned int bitWidth = src0->getType()->getIntegerBitWidth();
      if (bitWidth == 64 || bitWidth == 32) {
        BasicBlock *const bb = I.getParent();
        IGC_ASSERT(nullptr != bb);
        Function *const f = bb->getParent();
        IGC_ASSERT(nullptr != f);
        Value *hiDst = CreateMulh(*f, Builder, isSigned, src0, src1);
        IGC_ASSERT_MESSAGE(nullptr != hiDst, "CreateMulh failed.");
        if (isSigned) {
          // Signed a * b overflows if Mulh(a, b) != 0 or -1   and consequently
          // if Mulh(a, b) + 1 > 1 (for an unsigned comparison)
          Value *const one = ConstantInt::get(src0->getType(), 1, true);
          Instruction *const add1 = BinaryOperator::Create(Instruction::Add, hiDst, one, "", &I);
          add1->setDebugLoc(I.getDebugLoc());
          isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_UGT, add1, one, "", &I);
        } else {
          // Unsigned a * b overflows if Mulh(a, b) != 0
          Value *const zero = ConstantInt::get(src0->getType(), 0, isSigned);
          isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, hiDst, zero, "", &I);
        }
      } else {
        IGC_ASSERT(bitWidth < 32);
        Instruction *ores = nullptr;
        if (isSigned) {
          // Signed a * b overflows if (a * b) + (1 << (bitWidth-1)) >= (1 << bitWidth)
          //   (for an unsigned comparison)
          // For example. If src0 is 0xFF (-1) and src0 is 0xFF (-1) then
          //   src0SExt is 0xFFFFFFFF
          //   src1SExt is 0xFFFFFFFF
          //   mulRes is 0x00000001   (0xFFFFFFFF * 0xFFFFFFFF or -1 * -1)
          //   oneShl is 0x00000080   (1 << 7)
          //   ores is 0x00000081     (1 + 128)
          //   overflowed is 0x00000100    (1 << 8)
          //   isOverflow is false    (129 >= 256)
          Instruction *const src0SExt = BitCastInst::CreateSExtOrBitCast(src0, Builder.getInt32Ty(), "", &I);
          src0SExt->setDebugLoc(I.getDebugLoc());
          Instruction *const src1SExt = BitCastInst::CreateSExtOrBitCast(src1, Builder.getInt32Ty(), "", &I);
          src1SExt->setDebugLoc(I.getDebugLoc());
          Instruction *const mulRes = BinaryOperator::Create(Instruction::Mul, src0SExt, src1SExt, "", &I);
          mulRes->setDebugLoc(I.getDebugLoc());
          Value *const oneShl = ConstantInt::get(Builder.getInt32Ty(), 1LL << (bitWidth - 1), true);
          ores = BinaryOperator::Create(Instruction::Add, mulRes, oneShl, "", &I);
        } else {
          // Unsigned a * b overflows if a * b >= (1 << bitWidth) (for an unsigned comparison)
          Instruction *const src0ZExt = BitCastInst::CreateZExtOrBitCast(src0, Builder.getInt32Ty(), "", &I);
          src0ZExt->setDebugLoc(I.getDebugLoc());
          Instruction *const src1ZExt = BitCastInst::CreateZExtOrBitCast(src1, Builder.getInt32Ty(), "", &I);
          src1ZExt->setDebugLoc(I.getDebugLoc());
          ores = BinaryOperator::Create(Instruction::Mul, src0ZExt, src1ZExt, "", &I);
        }
        Value *const overflowed = ConstantInt::get(Builder.getInt32Ty(), 1LL << bitWidth, false);
        ores->setDebugLoc(I.getDebugLoc());
        isOverflow = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_UGE, ores, overflowed, "", &I);
      }
    } break;
    default:
      IGC_ASSERT_MESSAGE(0, "Incorrect intrinsic");
      break;
    }

    // llvm.x.with.overflow returns a struct, where the first element is the operation result,
    // and the second is the overflow flag.
    // Replace each extract with the correct instruction.
    for (auto U = I.user_begin(), EU = I.user_end(); U != EU; ++U) {
      ExtractValueInst *extract = dyn_cast<ExtractValueInst>(*U);
      if (!extract) {
        IGC_ASSERT_MESSAGE(0, "Did not expect anything but an extract after uadd_with_overflow");
        continue;
      }

      ArrayRef<unsigned int> indices = extract->getIndices();
      if (indices[0] == 0) {
        res->setDebugLoc(I.getDebugLoc());
        extract->replaceAllUsesWith(res);
      } else if (indices[0] == 1) {
        isOverflow->setDebugLoc(I.getDebugLoc());
        extract->replaceAllUsesWith(isOverflow);
      } else {
        IGC_ASSERT_MESSAGE(0, "Unexpected index when handling uadd_with_overflow");
      }

      m_instructionsToRemove.push_back(extract);
    }

    m_instructionsToRemove.push_back(&I);
    break;
  }
  case Intrinsic::assume:
    m_instructionsToRemove.push_back(&I);
    break;
  case Intrinsic::floor:
  case Intrinsic::ceil:
  case Intrinsic::trunc: {
    if (!m_ctx->platform.supportFP16Rounding() && I.getType()->isHalfTy()) {
      // On platform lacking of FP16 rounding, promote them to FP32 and
      // demote back.
      Value *Val = Builder.CreateFPExt(I.getOperand(0), Builder.getFloatTy());
      Value *Callee =
          Intrinsic::getDeclaration(I.getParent()->getParent()->getParent(), intrinsicID, Builder.getFloatTy());
      Val = Builder.CreateCall(Callee, ArrayRef<Value *>(Val));
      Val = Builder.CreateFPTrunc(Val, I.getType());
      I.replaceAllUsesWith(Val);
      I.eraseFromParent();
    } else if (I.getType()->isDoubleTy()) {
      auto lowerIntrinsicWithFunc = [&I, this](Value *(LLVM3DBuilder<>::*replacementFunc)(Value *)) {
        PLATFORM platform = m_ctx->platform.getPlatformInfo();
        LLVM3DBuilder<> llvmBuilder(I.getParent()->getParent()->getParent()->getContext(), platform);
        llvmBuilder.SetInsertPoint(&I);
        Value *argument = I.getArgOperand(0);
        if (argument->getType()->isDoubleTy()) {
          Value *result = (llvmBuilder.*replacementFunc)(argument);
          InlineFunctionInfo IFI;
          I.replaceAllUsesWith(result);
          IGCLLVM::InlineFunction(*cast<CallBase>(result), IFI, nullptr, false);
          I.eraseFromParent();
        }
      };
      switch (intrinsicID) {
      case Intrinsic::trunc: {
        lowerIntrinsicWithFunc(&LLVM3DBuilder<>::CreateRoundZ);
        break;
      }
      case Intrinsic::floor: {
        lowerIntrinsicWithFunc(&LLVM3DBuilder<>::CreateFloor);
        break;
      }
      case Intrinsic::ceil: {
        lowerIntrinsicWithFunc(&LLVM3DBuilder<>::CreateCeil);
        break;
      }
      default: {
        IGC_ASSERT_MESSAGE(0, "Incorrect intrinsic");
        break;
      }
      }
    }
  } break;
  case Intrinsic::copysign: {
    Value *const src0 = I.getArgOperand(0);
    Value *const src1 = I.getArgOperand(1);
    Type *const srcType = src0->getType();

    IGC_ASSERT(nullptr != srcType);
    IGC_ASSERT_MESSAGE(srcType->getScalarType()->isFloatingPointTy(),
                       "llvm.copysign supports only floating-point type");

    auto cpySign = [&Builder](Value *const src0, Value *const src1) {
      Type *const srcType = src0->getType();
      const unsigned int srcTypeSize = (const unsigned int)srcType->getPrimitiveSizeInBits();
      const uint64_t signMask = (uint64_t)0x1 << (srcTypeSize - 1);

      Value *const src0Int = Builder.CreateBitCast(src0, Builder.getIntNTy(srcTypeSize));
      Value *const src1Int = Builder.CreateBitCast(src1, Builder.getIntNTy(srcTypeSize));

      Value *const src0NoSign = Builder.CreateAnd(src0Int, Builder.getIntN(srcTypeSize, ~signMask));
      Value *const src1Sign = Builder.CreateAnd(src1Int, Builder.getIntN(srcTypeSize, signMask));

      Value *newValue = static_cast<Value *>(Builder.CreateOr(src0NoSign, src1Sign));
      newValue = Builder.CreateBitCast(newValue, srcType);
      return newValue;
    };

    Value *newValue = nullptr;
    if (srcType->isVectorTy()) {
      auto sourceVT = cast<IGCLLVM::FixedVectorType>(srcType);
      const unsigned int numElements = (uint32_t)sourceVT->getNumElements();
      Value *dstVec = UndefValue::get(srcType);
      for (unsigned int i = 0; i < numElements; ++i) {
        Value *const src0Scalar = Builder.CreateExtractElement(src0, i);
        Value *const src1Scalar = Builder.CreateExtractElement(src1, i);
        auto newValue = cpySign(src0Scalar, src1Scalar);
        dstVec = Builder.CreateInsertElement(dstVec, newValue, i);
      }
      newValue = dstVec;
    } else {
      newValue = cpySign(src0, src1);
    }
    IGC_ASSERT(nullptr != newValue);
    I.replaceAllUsesWith(newValue);
    I.eraseFromParent();
  } break;
  case Intrinsic::bitreverse: {
    Value *src0 = I.getArgOperand(0);
    Type *instrType = I.getType();
    IGC_ASSERT(nullptr != instrType);
    IGC_ASSERT_MESSAGE(instrType->isIntOrIntVectorTy(), "Unsupported type");
    Type *const intType = instrType->getScalarType();
    unsigned BitWidth = intType->getIntegerBitWidth();
    unsigned Elems = 1;
    if (instrType->isVectorTy())
      Elems = (unsigned)cast<IGCLLVM::FixedVectorType>(instrType)->getNumElements();

    Function *bfrevFunc =
        GenISAIntrinsic::getDeclaration(m_ctx->getModule(), GenISAIntrinsic::GenISA_bfrev, Builder.getInt32Ty());

    Value *newValue = Elems == 1 ? nullptr : UndefValue::get(instrType);
    for (unsigned i = 0; i < Elems; i++) {
      Value *ElVal = src0;
      Value *ElRes = nullptr;
      if (Elems > 1) {
        ElVal = Builder.CreateExtractElement(src0, ConstantInt::get(Type::getInt32Ty(I.getContext()), i));
      }
      if (BitWidth > 32 && BitWidth <= 64) {
        if (BitWidth < 64)
          ElVal = Builder.CreateZExt(ElVal, Builder.getInt64Ty());
        auto *Lo = Builder.CreateTrunc(ElVal, Builder.getInt32Ty());
        auto *Hi64 = Builder.CreateLShr(ElVal, 32);
        auto *Hi = Builder.CreateTrunc(Hi64, Builder.getInt32Ty());
        auto *RevLo = Builder.CreateCall(bfrevFunc, Lo);
        auto *RevHi = Builder.CreateCall(bfrevFunc, Hi);
        auto *NewVal = Builder.CreateZExt(RevLo, Builder.getInt64Ty());
        auto *ShlVal = Builder.CreateShl(NewVal, BitWidth - 32);
        auto *RevHiExt = Builder.CreateZExt(RevHi, Builder.getInt64Ty());
        if (BitWidth < 64)
          RevHiExt = Builder.CreateLShr(RevHiExt, 64 - BitWidth);
        ElRes = Builder.CreateOr(ShlVal, RevHiExt);
        if (BitWidth < 64)
          ElRes = Builder.CreateTrunc(ElRes, intType);
      } else {
        IGC_ASSERT_MESSAGE(BitWidth <= 32, "Unexpected type");
        if (BitWidth < 32)
          ElVal = Builder.CreateZExt(ElVal, Builder.getInt32Ty());
        auto *Call = Builder.CreateCall(bfrevFunc, ElVal);
        if (BitWidth < 32) {
          auto *LShr = Builder.CreateLShr(Call, 32 - BitWidth);
          ElRes = Builder.CreateTrunc(LShr, intType);
        } else
          ElRes = Call;
      }
      if (Elems > 1) {
        newValue = Builder.CreateInsertElement(newValue, ElRes, Builder.getInt32(i));
      } else
        newValue = ElRes;
    }
    IGC_ASSERT(nullptr != newValue);
    I.replaceAllUsesWith(newValue);
    I.eraseFromParent();
  } break;
  default:
    break;
  }
}

void Legalization::visitBasicBlock(llvm::BasicBlock &BB) { fpMap.clear(); }

void Legalization::PromoteFp16ToFp32OnGenSampleCall(llvm::CallInst &I) {
  llvm::SmallVector<llvm::Value *, 16> args(I.arg_begin(), I.arg_end());
  GenIntrinsicInst *CI = llvm::dyn_cast<GenIntrinsicInst>(&I);

  llvm::SmallVector<Type *, 5> types;

  llvm::Value *pairedTexture = nullptr;
  llvm::Value *texture = nullptr;
  llvm::Value *sampler = nullptr;

  if (SampleIntrinsic *inst = llvm::dyn_cast<SampleIntrinsic>(&I)) {
    pairedTexture = inst->getPairedTextureValue();
    texture = inst->getTextureValue();
    sampler = inst->getSamplerValue();
  } else if (SamplerGatherIntrinsic *inst = llvm::dyn_cast<SamplerGatherIntrinsic>(&I)) {
    pairedTexture = inst->getPairedTextureValue();
    texture = inst->getTextureValue();
    sampler = inst->getSamplerValue();
  }
  if (pairedTexture && pairedTexture->getType()->isPointerTy() && texture && texture->getType()->isPointerTy()) {
    types.resize(5);
    types[2] = pairedTexture->getType();
    types[3] = texture->getType();
    types[4] = sampler->getType();
  } else if (texture && texture->getType()->isPointerTy()) {
    types.resize(4);
    types[2] = texture->getType();
    types[3] = sampler->getType();
  } else {
    types.resize(2);
  }
  types[0] = I.getType();
  types[1] = Type::getFloatTy(I.getContext());

  for (size_t index = 0; index < args.size(); index++) {
    Value *input = I.getOperand(index);
    if (input->getType()->isHalfTy()) {
      m_builder->SetInsertPoint(&I);
      if (fpMap.find(input) == fpMap.end()) {
        args[index] = m_builder->CreateFPExt(input, Type::getFloatTy(I.getContext()), "");
        fpMap[input] = args[index];
      } else {
        args[index] = fpMap[input];
      }
    } else {
      args[index] = input;
    }
  }

  llvm::Function *f0 = GenISAIntrinsic::getDeclaration(m_ctx->getModule(), CI->getIntrinsicID(), types);
  llvm::CallInst *I0 = GenIntrinsicInst::Create(f0, args);
  I0->setDebugLoc(I.getDebugLoc());
  llvm::ReplaceInstWithInst(&I, I0);
}

void Legalization::visitTruncInst(llvm::TruncInst &I) {
  // Legalize
  //
  //  (trunc (bitcast <3 x i16> to i48) i32)
  //
  // into
  //
  //  (or (extract-element <3 x i16> 0)
  //      (shl (extract-element <3 x i16> 1) 16))
  //
  // Or, legalize
  //
  //  (trunc (lshr (bitcast <3 x i16> to i48) 32)
  //
  // into
  //
  //  (or (extract-element <3 x i16> 2) 0)
  //

  Type *DstTy = I.getDestTy();
  if (!DstTy->isIntegerTy(32))
    return;
  if (!I.getSrcTy()->isIntegerTy(48))
    return;

  unsigned Idx = 0; // By default, extract from the 0th element.

  Value *Src = I.getOperand(0);
  BitCastInst *BC = dyn_cast<BitCastInst>(Src);
  if (!BC) {
    // Check (lshr ...)
    BinaryOperator *BO = dyn_cast<BinaryOperator>(Src);
    if (!BO)
      return;
    if (BO->getOpcode() != Instruction::LShr)
      return;
    // The shift amount must be constant.
    ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));
    if (!CI)
      return;
    if (CI->equalsInt(16))
      Idx = 1;
    else if (CI->equalsInt(32))
      Idx = 2;
    else // Bail out if the shift amount is not a mutiplication of 16.
      return;

    BC = dyn_cast<BitCastInst>(BO->getOperand(0));
    if (!BC)
      return;
  }

  Src = BC->getOperand(0);
  IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Src->getType());
  // Bail out if it's not bitcasted from <3 x i16>
  if (!VTy || VTy->getNumElements() != 3 || !VTy->getElementType()->isIntegerTy(16))
    return;

  m_builder->SetInsertPoint(&I);

  IGC_ASSERT_MESSAGE(Idx < 3, "The initial index is out of range!");

  Value *NewVal = m_builder->CreateZExt(m_builder->CreateExtractElement(Src, m_builder->getInt32(Idx)), DstTy);
  if (++Idx < 3) {
    Value *Hi = m_builder->CreateZExt(m_builder->CreateExtractElement(Src, m_builder->getInt32(Idx)), DstTy);
    NewVal = m_builder->CreateOr(m_builder->CreateShl(Hi, 16), NewVal);
  }

  I.replaceAllUsesWith(NewVal);
  I.eraseFromParent();
}

void Legalization::visitAddrSpaceCastInst(llvm::AddrSpaceCastInst &I) {
  if (m_ctx->type != ShaderType::OPENCL_SHADER)
    return;

  Value *Src = I.getOperand(0);
  PointerType *SrcPtrTy = cast<PointerType>(Src->getType());
  if (SrcPtrTy->getAddressSpace() != ADDRESS_SPACE_LOCAL)
    return;

  PointerType *DstPtrTy = cast<PointerType>(I.getType());
  unsigned AS = DstPtrTy->getAddressSpace();
  if (AS != ADDRESS_SPACE_GENERIC) {
    if (!AS) // FIXME: Skip nullify on default AS as it's still used in VA builtins.
      return;
    Value *Null = Constant::getNullValue(DstPtrTy);
    auto tempInst = dyn_cast<Instruction>(Null);
    if (tempInst)
      tempInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(Null);
    I.eraseFromParent();
    return;
  }

  // Check for null pointer casting
  // Currently check is only handling specific scnario
  //%n = addrspacecast i32 addrspace(3)* null to i32 addrspace(4)*
  // this will be replaced with "null", and the %n where is used will be replaced with null
  // This issue was exposed with LLVM 4.0 because of a patch
  // github.com/llvm-mirror/llvm/commit/bca8aba44a2f414a25b55a3ba37f718113315f5f#diff-11765a284352f0be6fc81f5d6a8ddcbc
  // This patch made sure, that above instructions are not replaced with null
  // Consequently Legalization pass had to appropriately handle it.
  // However current fix will not handle complex scenariod such as
  // local pointer casted to different address spaces in dynamic flow
  if (isa<ConstantPointerNull>(I.getPointerOperand())) {
    Constant *Null = Constant::getNullValue(I.getType());
    auto tempInst = dyn_cast<Instruction>(Null);
    if (tempInst)
      tempInst->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(Null);
    I.eraseFromParent();
    return;
  }

  Function *F = I.getParent()->getParent();
  ImplicitArgs implicitArgs = ImplicitArgs(*F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
  Argument *SLM = implicitArgs.getImplicitArg(*F, ImplicitArg::LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS);
  if (!SLM)
    return;

  m_builder->SetInsertPoint(&I);

  unsigned PtrSz = m_DL->getPointerSizeInBits(cast<PointerType>(SLM->getType())->getAddressSpace());
  Type *Int16Ty = m_builder->getInt16Ty();
  Type *IntPtrTy = m_builder->getIntNTy(PtrSz);
  Value *Offset = m_builder->CreateZExt(m_builder->CreatePtrToInt(Src, Int16Ty), IntPtrTy);
  Value *Start = m_builder->CreatePtrToInt(SLM, IntPtrTy);
  Value *GASPtr = m_builder->CreateIntToPtr(m_builder->CreateAdd(Start, Offset), DstPtrTy);
  I.replaceAllUsesWith(GASPtr);
  I.eraseFromParent();
}

namespace {

/// Match and legalize IR that IGC does not handle correctly or efficiently; run
/// after some llvm optimization pass.
class GenOptLegalizer : public FunctionPass, public InstVisitor<GenOptLegalizer> {
public:
  static char ID;
  GenOptLegalizer();
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  void visitBitCastInst(BitCastInst &I);
  void visitLoadInst(LoadInst &I);
  void visitStoreInst(StoreInst &I);

private:
  const DataLayout *m_DL;
  IRBuilder<> *m_Builder;
  bool m_Changed;
  std::vector<llvm::Instruction *> m_InstructionsToRemove;
};

} // namespace

namespace IGC {

FunctionPass *createGenOptLegalizer() { return new GenOptLegalizer(); }

} // namespace IGC

IGC_INITIALIZE_PASS_BEGIN(GenOptLegalizer, "GenOptLegalizer", "GenOptLegalizer", false, false)
IGC_INITIALIZE_PASS_END(GenOptLegalizer, "GenOptLegalizer", "GenOptLegalizer", false, false)

char GenOptLegalizer::ID = 0;
GenOptLegalizer::GenOptLegalizer() : FunctionPass(ID), m_DL(nullptr), m_Builder(nullptr), m_Changed(false) {
  initializeGenOptLegalizerPass(*PassRegistry::getPassRegistry());
}

bool GenOptLegalizer::runOnFunction(Function &F) {
  IRBuilder<> Builder(F.getContext());
  m_Builder = &Builder;
  m_DL = &F.getParent()->getDataLayout();
  m_Changed = false;
  m_InstructionsToRemove.clear();
  visit(F);
  for (auto I : m_InstructionsToRemove)
    I->eraseFromParent();
  m_InstructionsToRemove.clear();
  return m_Changed;
}

void GenOptLegalizer::visitBitCastInst(BitCastInst &I) {
  m_Changed |= LegalizeGVNBitCastPattern(m_Builder, m_DL, I, nullptr);
}

void GenOptLegalizer::visitLoadInst(LoadInst &I) {
  if (I.getType()->isIntegerTy(24)) {
    if (!I.hasOneUse())
      return;
    auto *ZEI = dyn_cast<ZExtInst>(*I.user_begin());
    if (!ZEI || !ZEI->getType()->isIntegerTy(32))
      return;
    // Transforms the following sequence
    //
    // %0 = load i24, i24* %ptr
    // %1 = zext i24 %0 to i32
    //
    // into
    //
    // %newptr = bitcast i24* %ptr to <3 x i8>*
    // %0 = load <3 x i8>, <3 x i8>* %newptr
    // %1 = shufflevector <3 x i8> %0, <3 x i8> zeroinitializer, <i32 0, i32 1, i32 2, i32 3>
    // %2 = bitcast <4 x i8> %1 to i32
    // (RAUW)
    //
    m_Builder->SetInsertPoint(&I);
    Type *I8x3Ty = IGCLLVM::FixedVectorType::get(m_Builder->getInt8Ty(), 3);
    Type *I8x3PtrTy = PointerType::get(I8x3Ty, I.getPointerAddressSpace());
    Value *NewPtr = m_Builder->CreateBitCast(I.getPointerOperand(), I8x3PtrTy);
    Value *NewLD = IGC::cloneLoad(&I, I8x3Ty, NewPtr);
    Type *NewTy = ZEI->getType();
    Value *NewVal = Constant::getNullValue(NewTy);
    Value *L0 = m_Builder->CreateExtractElement(NewLD, uint64_t(0));
    NewVal = m_Builder->CreateOr(NewVal, m_Builder->CreateShl(m_Builder->CreateZExt(L0, NewTy), uint64_t(0)));
    Value *L1 = m_Builder->CreateExtractElement(NewLD, uint64_t(1));
    NewVal = m_Builder->CreateOr(NewVal, m_Builder->CreateShl(m_Builder->CreateZExt(L1, NewTy), uint64_t(8)));
    Value *L2 = m_Builder->CreateExtractElement(NewLD, uint64_t(2));
    m_Builder->SetCurrentDebugLocation(ZEI->getDebugLoc());
    NewVal = m_Builder->CreateOr(NewVal, m_Builder->CreateShl(m_Builder->CreateZExt(L2, NewTy), uint64_t(16)));
    ZEI->replaceAllUsesWith(NewVal);
    m_InstructionsToRemove.push_back(ZEI);
    m_InstructionsToRemove.push_back(&I);
    m_Changed = true;
  }
}

void GenOptLegalizer::visitStoreInst(StoreInst &I) {
  Value *V = I.getValueOperand();
  if (V->getType()->isIntegerTy(24)) {
    if (!V->hasOneUse())
      return;
    if (LoadInst *LD = dyn_cast<LoadInst>(V)) {
      // Transforms the following sequence
      //
      // %0 = load i24, i24* %src
      // %1 = store i24 %0, i24* %dst
      //
      // into
      //
      // %newsrc = bitcast i24* %src to <3 x i8>*
      // %0 = load <3 x i8>, <3 x i8>* %newsrc
      // %newdst = bitcast i24* %dst to <3 x i8>*
      // %1 = store <3 x i8> %0, <3 x i8>* %newdst
      //
      Type *I8x3Ty = IGCLLVM::FixedVectorType::get(m_Builder->getInt8Ty(), 3);
      Type *I8x3PtrTy = PointerType::get(I8x3Ty, LD->getPointerAddressSpace());
      // Replace load of i24 to load of <3 x i8>
      m_Builder->SetInsertPoint(LD);
      Value *NewPtr = m_Builder->CreateBitCast(LD->getPointerOperand(), I8x3PtrTy);
      Value *NewLD = IGC::cloneLoad(LD, I8x3Ty, NewPtr);
      // Replace store of i24 to load of <3 x i8>
      m_Builder->SetInsertPoint(&I);
      I8x3PtrTy = PointerType::get(I8x3Ty, I.getPointerAddressSpace());
      NewPtr = m_Builder->CreateBitCast(I.getPointerOperand(), I8x3PtrTy);
      IGC::cloneStore(&I, NewLD, NewPtr);
      // Remove original LD and ST.
      m_InstructionsToRemove.push_back(&I);
      m_InstructionsToRemove.push_back(LD);
      m_Changed = true;
    } else {
      TruncInst *SV = dyn_cast<TruncInst>(I.getValueOperand());
      BitCastInst *SP = dyn_cast<BitCastInst>(I.getPointerOperand());
      if (SV && SP) {
        // Transforms the following sequence
        //
        // %0 = bitcast i8* %ptr to i24*
        // %1 = trunc i32 %src to i24
        // store i24 %1, i24 addrspace(1)* %0
        //
        // into
        //
        // %0 = bitcast i8* %ptr to <3 x i8>*
        // %1 = bitcast i32 %src to <4 x i8>
        // %2 = shufflevector <4 x i8> %1, <4 x i8> undef, <i32 0, i32 1, i32 2>
        // store <3 x i8> %2, <3 x i8>* %0
        //
        m_Builder->SetInsertPoint(&I);
        Type *I8x3Ty = IGCLLVM::FixedVectorType::get(m_Builder->getInt8Ty(), 3);
        Type *I8x3PtrTy = PointerType::get(I8x3Ty, I.getPointerAddressSpace());

        // Convert i32 to <4 x i8>
        Type *SrcTy = SV->getOperand(0)->getType();
        unsigned numElements = (unsigned int)SrcTy->getPrimitiveSizeInBits() / 8;
        Type *NewVecTy = IGCLLVM::FixedVectorType::get(m_Builder->getInt8Ty(), numElements);
        Value *NewVec = m_Builder->CreateBitCast(SV->getOperand(0), NewVecTy);
        // Create shufflevector to select elements for <3 x i8>
        SmallVector<uint32_t, 3> maskVals = {0, 1, 2};
        Value *pMask = ConstantDataVector::get(I.getContext(), maskVals);
        auto *NewVal = new ShuffleVectorInst(NewVec, UndefValue::get(NewVecTy), pMask);
        NewVal->insertBefore(&I);
        // Bitcast src pointer to <3 x i8>* instead of i24*
        Value *NewPtr = m_Builder->CreateBitCast(SP->getOperand(0), I8x3PtrTy);
        // Create new store
        IGC::cloneStore(&I, NewVal, NewPtr);

        m_InstructionsToRemove.push_back(&I);
        m_InstructionsToRemove.push_back(SV);
        m_InstructionsToRemove.push_back(SP);
        m_Changed = true;
      }
    }
  }
}

static bool isCandidateFDiv(Instruction *Inst) {
  if (Inst->use_empty())
    return false;

  Type *Ty = Inst->getType();
  if (!Ty->isFloatTy() && !Ty->isHalfTy() && !IGCLLVM::isBFloatTy(Ty))
    return false;

  auto Op = dyn_cast<FPMathOperator>(Inst);
  if (Op && Op->getOpcode() == Instruction::FDiv) {
    Value *Src0 = Op->getOperand(0);
    if (auto CFP = dyn_cast<ConstantFP>(Src0))
      return !CFP->isExactlyValue(1.0);
    return true;
  }
  return false;
}

// Check if a scaling factor is needed for a constant denominator.
static bool needsNoScaling(Value *Val) {
  auto FP = dyn_cast<ConstantFP>(Val);
  if (!FP || !FP->getType()->isFloatTy())
    return false;

  union {
    uint32_t u32;
    float f32;
  } U;

  float FVal = FP->getValueAPF().convertToFloat();
  U.f32 = FVal;

  uint32_t UVal = U.u32;
  UVal &= 0x7f800000;
  return (UVal > 0) && (UVal < (200U << 23));
}

// Expand fdiv(x, y) into rcp(y * S) * x * S
// where S = 2^32 if exp(y) == 0,
//       S = 2^(-32) if exp(y) >= 200,
//       S = 1.0f otherwise
//
bool IGC::expandFDIVInstructions(llvm::Function &F, ShaderType ShaderTy) {
  bool Changed = false;
  for (auto &BB : F) {
    for (auto Iter = BB.begin(); Iter != BB.end();) {
      Instruction *Inst = &*Iter++;
      if (!isCandidateFDiv(Inst))
        continue;

      IRBuilder<> Builder(Inst);
      Builder.setFastMathFlags(Inst->getFastMathFlags());

      auto &Ctx = Inst->getContext();
      Value *X = Inst->getOperand(0);
      Value *Y = Inst->getOperand(1);
      Value *V = nullptr;

      if (Inst->getType()->isHalfTy() || IGCLLVM::isBFloatTy(Inst->getType())) {
        if (Inst->hasAllowReciprocal()) {
          APFloat Val(1.0f);
          bool ignored;
          Val.convert((Inst->getType()->isHalfTy()) ? APFloat::IEEEhalf() : APFloat::BFloat(), APFloat::rmTowardZero,
                      &ignored);
          ConstantFP *C1 = ConstantFP::get(Ctx, Val);
          Y = Builder.CreateFDiv(C1, Y);
          V = Builder.CreateFMul(Y, X);
        } else {
          // Up cast to float, and down cast to half / bfloat.
          // div as float with additional checks for better precision and special cases like Inf, NaN. to be spec
          // conformant.
          Y = Builder.CreateFPExt(Y, Builder.getFloatTy());
          X = Builder.CreateFPExt(X, Builder.getFloatTy());
          V = Builder.CreateFDiv(X, Y);
          // Iterator at the begining of the loop is already at the next instruction,
          // so we want to set it back to handle this fdiv as normal one.
          Iter = BasicBlock::iterator(dyn_cast<Instruction>(V));

          V = Builder.CreateFPTrunc(V, Inst->getType());
        }
      } else if (Inst->hasAllowReciprocal() || needsNoScaling(Y)) {
        Y = Builder.CreateFDiv(ConstantFP::get(Ctx, APFloat(1.0f)), Y);
        V = Builder.CreateFMul(Y, X);
      } else {
        Value *YAsInt32 = Builder.CreateBitCast(Y, Builder.getInt32Ty());
        Value *YExp = Builder.CreateAnd(YAsInt32, Builder.getInt32(0x7f800000));

        float S32 = uint64_t(1) << 32;
        ConstantFP *C0 = ConstantFP::get(Ctx, APFloat(S32));
        ConstantFP *C1 = ConstantFP::get(Ctx, APFloat(1.0f));
        ConstantFP *C2 = ConstantFP::get(Ctx, APFloat(1.0f / S32));

        // Determine the appropriate scale based on Y's exponent.
        Value *ScaleUp = Builder.CreateSelect(Builder.CreateICmpEQ(YExp, Builder.getInt32(0)), C0, C1);
        Value *Scale = Builder.CreateSelect(Builder.CreateICmpUGE(YExp, Builder.getInt32(200 << 23)), C2, ScaleUp);

        // Compute rcp(y * S) * x * S
        Value *ScaledY = Builder.CreateFMul(Y, Scale);
        ScaledY = Builder.CreateFDiv(C1, ScaledY);
        V = Builder.CreateFMul(ScaledY, X);
        V = Builder.CreateFMul(V, Scale);

        // In case of OpenCL kernels, create comparisons to check if X or Y is +/-0, +/-Inf, +/-NaN,
        // or subnormal. If x == y and y is a normal number, select 1.0f as a result for better precision.
        if (ShaderTy == ShaderType::OPENCL_SHADER) {
          Value *CmpXY = Builder.CreateFCmpOEQ(X, Y);
          Value *YMantissa = Builder.CreateAnd(YAsInt32, Builder.getInt32(0x007fffff));
          Value *CmpYExpZero = Builder.CreateICmpEQ(YExp, Builder.getInt32(0));
          Value *CmpYMantissaZero = Builder.CreateICmpEQ(YMantissa, Builder.getInt32(0));
          Value *CmpYIsZeroOrSubnormal = Builder.CreateOr(CmpYExpZero, CmpYMantissaZero);
          Value *CmpYIsNotZeroOrSubnormal = Builder.CreateNot(CmpYIsZeroOrSubnormal);
          V = Builder.CreateSelect(Builder.CreateAnd(CmpXY, CmpYIsNotZeroOrSubnormal),
                                   ConstantFP::get(Ctx, APFloat(1.0f)), V);
        }
      }

      Inst->replaceAllUsesWith(V);
      Inst->eraseFromParent();
      Changed = true;
    }
  }

  return Changed;
}

namespace IGC {

class GenFDIVEmulation : public FunctionPass {
public:
  static char ID;
  GenFDIVEmulation();
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }
};

FunctionPass *createGenFDIVEmulation() { return new GenFDIVEmulation; }

} // namespace IGC

IGC_INITIALIZE_PASS_BEGIN(GenFDIVEmulation, "GenFDIVEmulation", "GenFDIVEmulation", false, false)
IGC_INITIALIZE_PASS_END(GenFDIVEmulation, "GenFDIVEmulation", "GenFDIVEmulation", false, false)

char GenFDIVEmulation::ID = 0;
GenFDIVEmulation::GenFDIVEmulation() : FunctionPass(ID) {
  initializeGenFDIVEmulationPass(*PassRegistry::getPassRegistry());
}

bool GenFDIVEmulation::runOnFunction(Function &F) {
  IGC::CodeGenContext *m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  // Always emulate fdiv instructions.
  return expandFDIVInstructions(F, m_ctx->type);
}
