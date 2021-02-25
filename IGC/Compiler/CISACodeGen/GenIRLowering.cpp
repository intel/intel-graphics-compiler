/*========================== begin_copyright_notice ============================

Copyright (c) 2014-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// vim:ts=2:sw=2:et:
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/GenIRLowering.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Analysis/TargetFolder.h>
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvmWrapper/IR/Intrinsics.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "common/IGCIRBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using IGCLLVM::FixedVectorType;

namespace {
    class GenIRLowering : public FunctionPass {
        using BuilderTy = IGCIRBuilder<TargetFolder>;
        BuilderTy* Builder = nullptr;
    public:
        static char ID;

        GenIRLowering() : FunctionPass(ID) {
            initializeGenIRLoweringPass(*PassRegistry::getPassRegistry());
        }

        StringRef getPassName() const override { return "GenIR Lowering"; }

        bool runOnFunction(Function& F) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

    private:
        // Helpers
        Value* rearrangeAdd(Value*, Loop*) const;

        bool combineFMaxFMin(CallInst* GII, BasicBlock::iterator& BBI) const;
        bool combineSelectInst(SelectInst* Sel, BasicBlock::iterator& BBI) const;

        bool constantFoldFMaxFMin(CallInst* GII,
            BasicBlock::iterator& BBI) const;
    };

    char GenIRLowering::ID = 0;

    // Pattern match helpers.

    template <typename LHS_t, typename RHS_t, typename Pred_t>
    struct FMaxMinCast_match {
        unsigned& CastOpcode;
        LHS_t L;
        RHS_t R;

        FMaxMinCast_match(unsigned& Opcode, const LHS_t& LHS, const RHS_t& RHS)
            : CastOpcode(Opcode), L(LHS), R(RHS) {}

        bool isEqualOrCasted(Value* V, Value* Orig, unsigned Opcode) const {
            if (V == Orig)
                return true;
            // Check V is casted from Orig.
            CastInst* Cast = dyn_cast<CastInst>(V);
            if (Cast && Cast->getOpcode() == Opcode && Cast->getOperand(0) == Orig)
                return true;
            if (Constant * C = dyn_cast<Constant>(Orig)) {
                if (!CastInst::castIsValid(Instruction::CastOps(Opcode), C, V->getType()))
                    return false;
                // TODO: Need to check isExact for FPToSI/FPToUI.
                Constant* Casted = ConstantExpr::getCast(Opcode, C, V->getType());
                if (V == Casted)
                    return true;
            }
            return false;
        }

        template <typename OpTy> bool match(OpTy* V) {
            SelectInst* SI = dyn_cast<SelectInst>(V);
            if (!SI)
                return false;
            FCmpInst* Cmp = dyn_cast<FCmpInst>(SI->getCondition());
            if (!Cmp)
                return false;
            Value* TVal = SI->getTrueValue();
            Value* FVal = SI->getFalseValue();

            // Check cast op if any. If both operands use cast op, they should match.
            unsigned Opcode = Instruction::UserOp1;
            if (CastInst * Cast = dyn_cast<CastInst>(TVal))
                Opcode = Cast->getOpcode();
            if (CastInst * Cast = dyn_cast<CastInst>(FVal)) {
                unsigned Op = Cast->getOpcode();
                if (Opcode != Instruction::UserOp1 && Opcode != Op)
                    return false;
                Opcode = Op;
            }

            Value* LHS = Cmp->getOperand(0);
            Value* RHS = Cmp->getOperand(1);
            if ((!isEqualOrCasted(TVal, LHS, Opcode) ||
                !isEqualOrCasted(FVal, RHS, Opcode)) &&
                (!isEqualOrCasted(TVal, RHS, Opcode) ||
                    !isEqualOrCasted(FVal, LHS, Opcode)))
                return false;

            FCmpInst::Predicate Pred = Cmp->getPredicate();
            if (!isEqualOrCasted(TVal, LHS, Opcode)) {
                Pred = Cmp->getSwappedPredicate();
                std::swap(TVal, FVal);
            }
            if (!Pred_t::match(Pred))
                return false;

            if (L.match(LHS) && R.match(RHS)) {
                CastOpcode = Opcode;
                return true;
            }
            return false;
        }
    };

    template <typename LHS, typename RHS>
    inline FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmax_pred_ty>
        m_OrdFMaxCast(unsigned& Opcode, const LHS& L, const RHS& R) {
        return FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmax_pred_ty>(Opcode,
            L, R);
    }

    template <typename LHS, typename RHS>
    inline FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmin_pred_ty>
        m_OrdFMinCast(unsigned& Opcode, const LHS& L, const RHS& R) {
        return FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmin_pred_ty>(Opcode,
            L, R);
    }

    template <typename Op_t, typename ConstTy> struct ClampWithConstants_match {
        typedef ConstTy* ConstPtrTy;

        Op_t Op;
        ConstPtrTy& CMin, & CMax;

        ClampWithConstants_match(const Op_t& OpMatch, ConstPtrTy& Min,
            ConstPtrTy& Max)
            : Op(OpMatch), CMin(Min), CMax(Max) {}

        template <typename OpTy> bool match(OpTy* V) {
            CallInst* GII = dyn_cast<CallInst>(V);
            if (!GII)
                return false;

            EOPCODE GIID = GetOpCode(GII);
            if (GIID != llvm_max && GIID != llvm_min)
                return false;

            Value* X = GII->getOperand(0);
            Value* C = GII->getOperand(1);
            if (isa<ConstTy>(X))
                std::swap(X, C);

            ConstPtrTy C0 = dyn_cast<ConstTy>(C);
            if (!C0)
                return false;

            CallInst* GII2 = dyn_cast<CallInst>(X);
            if (!GII2)
                return false;

            EOPCODE GIID2 = GetOpCode(GII2);
            if (!(GIID == llvm_min && GIID2 == llvm_max) &&
                !(GIID == llvm_max && GIID2 == llvm_min))
                return false;

            X = GII2->getOperand(0);
            C = GII2->getOperand(1);
            if (isa<ConstTy>(X))
                std::swap(X, C);

            ConstPtrTy C1 = dyn_cast<ConstTy>(C);
            if (!C1)
                return false;

            if (!Op.match(X))
                return false;

            CMin = (GIID2 == llvm_min) ? C0 : C1;
            CMax = (GIID2 == llvm_min) ? C1 : C0;
            return true;
        }
    };

    template <typename OpTy, typename ConstTy>
    inline ClampWithConstants_match<OpTy, ConstTy>
        m_ClampWithConstants(const OpTy& Op, ConstTy*& Min, ConstTy*& Max) {
        return ClampWithConstants_match<OpTy, ConstTy>(Op, Min, Max);
    }

    // This pass lowers GEP into primitive ones (i.e. addition and/or
    // multiplication, converted to shift if applicable) to expose address
    // calculation to LLVM optimizations, such as CSE, LICM, and etc.
    //
    class GEPLowering : public FunctionPass {
        const DataLayout* DL = nullptr;
        CodeGenContext* m_ctx = nullptr;
        using BuilderTy = IGCIRBuilder<TargetFolder>;
        BuilderTy* Builder = nullptr;
        llvm::LoopInfo* m_LI = nullptr;
        ModuleMetaData* modMD = nullptr;
    public:
        static char ID;

        GEPLowering() : FunctionPass(ID) {
            initializeGEPLoweringPass(*PassRegistry::getPassRegistry());
        }

        StringRef getPassName() const override { return "GEP Lowering"; }

        bool runOnFunction(Function& F) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

    private:
        // Helpers
        Value* getSExtOrTrunc(Value*, Type*) const;
        Value* truncExpr(Value*, Type*) const;

        bool lowerGetElementPtrInst(GetElementPtrInst* GEP) const;
    };

    char GEPLowering::ID = 0;

} // End anonymous namespace

bool GenIRLowering::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    auto &DL = F.getParent()->getDataLayout();

    BuilderTy TheBuilder(F.getContext(), TargetFolder(DL));
    Builder = &TheBuilder;

    bool Changed = false;

    // Replace SLM PtrToInt by the assigned immed offset
    // Later optimization (InstCombine) can fold away some address computation

    FunctionMetaData* funcMD = &modMD->FuncMD[&F];

    for (auto localOffetsItr = funcMD->localOffsets.begin(), localOffsetsEnd = funcMD->localOffsets.end();
        localOffetsItr != localOffsetsEnd;
        ++localOffetsItr)
    {
        LocalOffsetMD localOffset = *localOffetsItr;

        // look up the value-to-offset mapping
        Value* V = localOffset.m_Var;
        unsigned Offset = localOffset.m_Offset;

        // Skip non-pointer values.
        if (!V->getType()->isPointerTy())
            continue;
        // Skip non-local pointers.
        unsigned AS = V->getType()->getPointerAddressSpace();
        if (AS != ADDRESS_SPACE_LOCAL)
            continue;

        // It is possible that a global (slm) is used in more than one kernels
        // and each kernel might have a different offset for this global. Thus,
        // we can only replace the uses within this kernel function. We will check
        // instructions only as the constant expressions have been broken up
        // before this pass.
        PointerType* PTy = cast<PointerType>(V->getType());
        Constant* CO = ConstantInt::get(Type::getInt32Ty(F.getContext()), Offset);
        Constant* NewBase = ConstantExpr::getIntToPtr(CO, PTy);
        auto NI = V->user_begin();
        for (auto I = NI, E = V->user_end(); I != E; I = NI)
        {
            ++NI;
            Instruction* Inst = dyn_cast<Instruction>(*I);
            if (!Inst || Inst->getParent()->getParent() != &F) {
                continue;
            }

            // As constant exprs have been broken up, need to check insts only.
            if (GetElementPtrInst * GEPI = dyn_cast<GetElementPtrInst>(Inst))
            {
                // sanity check
                if (GEPI->getOperand(0) == V) {
                    // operand 0 is pointer operand
                    GEPI->setOperand(0, NewBase);
                    Changed = true;
                }
            }
            else if (PtrToIntInst * PI = dyn_cast<PtrToIntInst>(Inst))
            {
                Value* CI = ConstantInt::get(PI->getType(), Offset);
                PI->replaceAllUsesWith(CI);
                PI->eraseFromParent();
                Changed = true;
            }
            else if (BitCastInst * BCI = dyn_cast<BitCastInst>(Inst))
            {
                BCI->setOperand(0, NewBase);
                Changed = true;
            }
            else if (LoadInst * LI = dyn_cast<LoadInst>(Inst))
            {
                LI->setOperand(0, NewBase);
                Changed = true;
            }
            else if (StoreInst * SI = dyn_cast<StoreInst>(Inst))
            {
                if (SI->getPointerOperand() == V)
                {
                    // pointer operand is operand 1!
                    SI->setOperand(1, NewBase);
                    Changed = true;
                }
            }
        }
    }

    for (auto& BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE;) {
            Instruction* Inst = &(*BI++);
            Builder->SetInsertPoint(Inst);

            switch (Inst->getOpcode()) {
            default: // By default, DO NOTHING
                break;
            case Instruction::Call:
                if (CallInst * GII = dyn_cast<CallInst>(Inst)) {
                    switch (GetOpCode(GII)) {
                    case llvm_max:
                    case llvm_min:
                        Changed |= combineFMaxFMin(GII, BI);
                        break;
                    default:
                        break;
                    }
                }
                break;
            case Instruction::Select:
                // Enable the pattern match only when NaNs can be ignored.
                if (modMD->compOpt.NoNaNs ||
                    modMD->compOpt.FiniteMathOnly)
                {
                    Changed |= combineSelectInst(cast<SelectInst>(Inst), BI);
                }
                break;
            }
        }
    }

    Builder = nullptr;

    return Changed;
}

bool GEPLowering::runOnFunction(Function& F) {
    // Skip non-kernel function.
    modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    m_ctx = pCtxWrapper->getCodeGenContext();

    DL = &F.getParent()->getDataLayout();

    BuilderTy TheBuilder(F.getContext(), TargetFolder(*DL));
    Builder = &TheBuilder;

    bool Changed = false;

    for (auto& BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE;) {
            Instruction* Inst = &(*BI++);
            Builder->SetInsertPoint(Inst);

            switch (Inst->getOpcode()) {
            default: // By default, DO NOTHING
                break;
            // Lower GEPs to inttoptr/ptrtoint with offsets.
            case Instruction::GetElementPtr:
                Changed |=
                    lowerGetElementPtrInst(cast<GetElementPtrInst>(Inst));
                break;
            }
        }
    }

    return Changed;
}


Value* GEPLowering::getSExtOrTrunc(Value* Val, Type* NewTy) const {
    Type* OldTy = Val->getType();
    unsigned OldWidth;
    unsigned NewWidth;

    IGC_ASSERT_MESSAGE(OldTy->isIntOrIntVectorTy(), "Index should be Integer or vector of Integer!");

    if (auto OldVecTy = dyn_cast<IGCLLVM::FixedVectorType>(OldTy)) {
        OldWidth = (unsigned)OldVecTy->getNumElements() * OldVecTy->getElementType()->getIntegerBitWidth();
        NewWidth = (unsigned)OldVecTy->getNumElements() * NewTy->getIntegerBitWidth();
    }
    else {
        OldWidth = OldTy->getIntegerBitWidth();
        NewWidth = NewTy->getIntegerBitWidth();
    }

    if (OldWidth < NewWidth) { // SExt
        return Builder->CreateSExt(Val, NewTy);
    }

    if (OldWidth > NewWidth) { // Trunc
        return truncExpr(Val, NewTy);
    }

    return Val;
}

Value* GEPLowering::truncExpr(Value* Val, Type* NewTy) const {
    // Truncation on Gen could be as cheap as NOP by creating the proper region.
    // Instead of truncating the value itself, try to truncate how it's
    // calculated.
    if (Constant * C = dyn_cast<Constant>(Val))
        return Builder->CreateIntCast(C, NewTy, false);

    if (!isa<Instruction>(Val))
        return Builder->CreateTrunc(Val, NewTy);

    Instruction* I = cast<Instruction>(Val);
    unsigned Opc = I->getOpcode();
    switch (Opc) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor: {
        BinaryOperator* BO = cast<BinaryOperator>(I);
        Value* LHS = truncExpr(BO->getOperand(0), NewTy);
        Value* RHS = truncExpr(BO->getOperand(1), NewTy);
        return Builder->CreateBinOp(BO->getOpcode(), LHS, RHS);
    }
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt: {
        Value* Opnd = I->getOperand(0);
        if (Opnd->getType() == NewTy)
            return Opnd;
        return Builder->CreateIntCast(Opnd, NewTy, Opc == Instruction::SExt);
    }
    case Instruction::Select: {
        Value* TVal = truncExpr(I->getOperand(1), NewTy);
        Value* FVal = truncExpr(I->getOperand(2), NewTy);
        return Builder->CreateSelect(I->getOperand(0), TVal, FVal);
    }
#if 0
                              // TODO: Rewrite truncExpr into iterative one instead of recursive one to
                              // easily found the loop due to phi-node.
    case Instruction::PHI: {
        PHINode* PN = cast<PHINode>(I);
        PHINode* Res = PHINode::Create(NewTy, PN->getNumIncomingValues());
        for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            Value* V = truncExpr(PN->getIncomingValue(i), NewTy);
            Res->addIncoming(V, PN->getIncomingBlock(i));
        }
        return Res;
    }
#endif
    default:
        // Don't know truncate its calculation safely, fall back to the regular
        // way.
        break;
    }

    return Builder->CreateTrunc(Val, NewTy);
}

//
// reassociate chain of address adds so that the loop invariant terms appear on RHS tree
//
Value* GenIRLowering::rearrangeAdd(Value* val, Loop* loop) const
{
    BinaryOperator* binOp = dyn_cast<BinaryOperator>(val);
    if (!binOp || binOp->getOpcode() != Instruction::Add)
    {
        return val;
    }

    Value* LHS = binOp->getOperand(0);
    Value* RHS = binOp->getOperand(1);

    if (loop->isLoopInvariant(LHS))
    {
        Value* newRHS = rearrangeAdd(binOp->getOperand(1), loop);
        if (!loop->isLoopInvariant(newRHS))
        {
            BinaryOperator* RHSBinOp = dyn_cast<BinaryOperator>(newRHS);
            if (RHSBinOp && RHSBinOp->getOpcode() == Instruction::Add)
            {
                // LI + (a + b) --> a + (b + LI)
                Value* LHSofNewRHS = RHSBinOp->getOperand(0);
                Value* RHSofNewRHS = RHSBinOp->getOperand(1);
                return Builder->CreateAdd(LHSofNewRHS, Builder->CreateAdd(RHSofNewRHS, LHS));
            }
        }

        // LI + a --> a + LI
        return Builder->CreateAdd(newRHS, LHS);
    }
    else
    {
        Value* newLHS = rearrangeAdd(LHS, loop);
        BinaryOperator* LHSBinOp = dyn_cast<BinaryOperator>(newLHS);
        if (LHSBinOp && LHSBinOp->getOpcode() == Instruction::Add)
        {
            Value* LHSofLHS = LHSBinOp->getOperand(0);
            Value* RHSofLHS = LHSBinOp->getOperand(1);
            if (loop->isLoopInvariant(RHSofLHS))
            {
                // (a + LI) + b --> a + (b + LI)
                return Builder->CreateAdd(LHSofLHS, rearrangeAdd(Builder->CreateAdd(RHS, RHSofLHS), loop));
            }
        }

        return Builder->CreateAdd(newLHS, rearrangeAdd(RHS, loop));
    }
}

bool GEPLowering::lowerGetElementPtrInst(GetElementPtrInst* GEP) const
{
    Value* const PtrOp = GEP->getPointerOperand();
    IGC_ASSERT(nullptr != PtrOp);
    PointerType* const PtrTy = dyn_cast<PointerType>(PtrOp->getType());
    IGC_ASSERT_MESSAGE(nullptr != PtrTy, "Only accept scalar pointer!");

    unsigned pointerSizeInBits = m_ctx->getRegisterPointerSizeInBits(PtrTy->getAddressSpace());
    unsigned pointerMathSizeInBits = pointerSizeInBits;
    bool reducePointerArith = false;
    bool canReduceNegativeOffset = false;

    // Detect if we can do intermediate pointer arithmetic in 32bits
    if (pointerMathSizeInBits == 64 && GEP->isInBounds()) {
        if (!modMD->compOpt.GreaterThan4GBBufferRequired) {
            bool gepProducesPositivePointer = true;

            // prove that the offset from the base pointer will be positive.  if we cannot
            // prove that all parameters to GEP increase the address of the final calculation
            // we can't fall back to 32bit math
            for (auto U = GEP->idx_begin(), E = GEP->idx_end(); U != E; ++U) {
                Value* Idx = U->get();

                if (Idx != GEP->getPointerOperand()) {
                    gepProducesPositivePointer &= valueIsPositive(Idx, DL);
                }
            }

            if (gepProducesPositivePointer) {
                pointerMathSizeInBits = 32;
                reducePointerArith = true;
            }
        }
        else if (GEP->getAddressSpace() == ADDRESS_SPACE_CONSTANT || !modMD->compOpt.GreaterThan2GBBufferRequired)
        {
            canReduceNegativeOffset = true;
            pointerMathSizeInBits = 32;
            reducePointerArith = true;
        }
    }

    IntegerType* IntPtrTy = IntegerType::get(Builder->getContext(), pointerSizeInBits);
    IntegerType* PtrMathTy =
        IntegerType::get(Builder->getContext(), pointerMathSizeInBits);

    Value* BasePointer = nullptr;
    // Check if the pointer itself is created from IntToPtr.  If it is, and if
    // the int is the same size, we can use the int directly.  Otherwise, we
    // need to add PtrToInt.
    if (IntToPtrInst * I2PI = dyn_cast<IntToPtrInst>(PtrOp)) {
        Value* IntOp = I2PI->getOperand(0);
        if (IntOp->getType() == IntPtrTy) {
            BasePointer = IntOp;
        }
    }
    if (!BasePointer) {
        BasePointer = Builder->CreatePtrToInt(PtrOp, IntPtrTy);
    }
    // This is the value of the pointer, which will ultimately replace
    // getelementptr.
    Value* PointerValue = nullptr;
    if (reducePointerArith)
    {
        // in case the pointer arithmetic is done in lower type postpone adding the base to the end
        PointerValue = ConstantInt::get(PtrMathTy, 0);
    }
    else
    {
        PointerValue = BasePointer;
    }

    Type* Ty = PtrTy;
    gep_type_iterator GTI = gep_type_begin(GEP);
    for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
        Value* Idx = *OI;
        if (StructType * StTy = GTI.getStructTypeOrNull()) {
            unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
            if (Field) {
                uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);
                Value* OffsetValue =
                    Builder->getInt(APInt(pointerMathSizeInBits, Offset));
                PointerValue = Builder->CreateAdd(PointerValue, OffsetValue);
            }
            Ty = StTy->getElementType(Field);
        }
        else {
            Ty = GTI.getIndexedType();

            if (const ConstantInt * CI = dyn_cast<ConstantInt>(Idx)) {
                if (!CI->isZero()) {
                    uint64_t Offset = DL->getTypeAllocSize(Ty) * CI->getSExtValue();
                    Value* OffsetValue =
                        Builder->getInt(APInt(pointerMathSizeInBits, Offset));
                    PointerValue = Builder->CreateAdd(PointerValue, OffsetValue);
                }
            }
            else {
                Value* NewIdx = getSExtOrTrunc(Idx, PtrMathTy);
                APInt ElementSize =
                    APInt(pointerMathSizeInBits, DL->getTypeAllocSize(Ty));

                ConstantInt* COffset = nullptr;
                if (IGC_IS_FLAG_ENABLED(EnableSimplifyGEP) && NewIdx->hasOneUse())
                {
                    // When EnableSimplifyGEP is on, GEP's index can be of V + C
                    // where C is constant. If so, we will continue push C up to
                    // the top so CSE could do better job.

                    //
                    // Replace
                    //   %nswAdd = add nsw i32 %49, 195
                    //   %NewIdx = sext i32 %nswAdd to i64
                    //   %PointerValue = %NewIdx * 4 + %Base
                    // with
                    //   %NewIdx = sext i32 %49
                    //   %PointerValue = (%NewIdx * 4 + %Base) + (4 * 195)
                    // for later CSE.
                    //

                    bool performSExt = false;
                    if (SExtInst * I = dyn_cast<SExtInst>(NewIdx)) {
                        if (OverflowingBinaryOperator * nswAdd = dyn_cast<OverflowingBinaryOperator>(I->getOperand(0)))
                        {
                            if ((nswAdd->getOpcode() == Instruction::Add)
                                && nswAdd->hasNoSignedWrap()
                                && isa<ConstantInt>(nswAdd->getOperand(1)))
                            {
                                performSExt = true;
                                NewIdx = nswAdd;
                            }
                        }
                    }
                    if (Instruction * Inst = dyn_cast<Instruction>(NewIdx))
                    {
                        if (Inst->getOpcode() == Instruction::Add)
                        {
                            COffset = dyn_cast<ConstantInt>(Inst->getOperand(1));
                            if (COffset)
                            {
                                NewIdx = Inst->getOperand(0);
                                int64_t cval = COffset->getSExtValue() * ElementSize.getZExtValue();
                                COffset = ConstantInt::get(PtrMathTy, cval);
                            }
                        }
                    }
                    if (performSExt) {
                        NewIdx = Builder->CreateSExt(NewIdx, PtrMathTy);
                    }
                }

                if (BinaryOperator * binaryOp = dyn_cast<BinaryOperator>(NewIdx))
                {
                    // detect the pattern
                    // GEP base, a + b
                    // where base and a are both loop invariant (but not b), so we could rearrange the lowered code into
                    // (base + (a << shftAmt)) + (b << shftAmt)
                    // For now we only look at one level
                    Loop* loop = m_LI ? m_LI->getLoopFor(binaryOp->getParent()) : nullptr;
                    if (loop != nullptr && loop->isLoopInvariant(PtrOp) && binaryOp->getOpcode() == Instruction::Add)
                    {

                        Value* LHS = binaryOp->getOperand(0);
                        Value* RHS = binaryOp->getOperand(1);
                        bool isLHSLI = loop->isLoopInvariant(LHS);
                        bool isRHSLI = loop->isLoopInvariant(RHS);

                        auto reassociate = [&](Value* invariant, Value* other)
                        {
                            Value* invariantVal = nullptr;
                            if (ElementSize == 1)
                            {
                                invariantVal = invariant;
                            }
                            else if (ElementSize.isPowerOf2())
                            {
                                invariantVal = Builder->CreateShl(invariant, APInt(pointerMathSizeInBits, ElementSize.logBase2()));
                            }
                            else
                            {
                                invariantVal = Builder->CreateMul(invariant, Builder->getInt(ElementSize));
                            }
                            PointerValue = Builder->CreateAdd(PointerValue, invariantVal);
                            NewIdx = other;
                        };
                        if (isLHSLI && !isRHSLI)
                        {
                            reassociate(LHS, RHS);
                        }
                        else if (!isLHSLI && isRHSLI)
                        {
                            reassociate(RHS, LHS);
                        }
                    }
                }
                if (ElementSize == 1) {
                    // DO NOTHING.
                }
                else if (ElementSize.isPowerOf2()) {
                    APInt ShiftAmount =
                        APInt(pointerMathSizeInBits, ElementSize.logBase2());
                    NewIdx = Builder->CreateShl(NewIdx, ShiftAmount);
                }
                else {
                    NewIdx = Builder->CreateMul(NewIdx, Builder->getInt(ElementSize));
                }


                Loop* loop = m_LI ? m_LI->getLoopFor(GEP->getParent()) : nullptr;

                if (loop && loop->isLoopInvariant(PtrOp))
                {
                    // add COffset to Pointer base first so LICM can kick in later
                    // note that PointerValue is guaranteed to be LI since both PtrOp and whatever
                    // we've added to it during reassociation must be LI
                    if (COffset)
                    {
                        PointerValue = Builder->CreateAdd(PointerValue, COffset);
                    }
                    PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
                }
                else
                {
                    if (auto NewIdxVT = dyn_cast<IGCLLVM::FixedVectorType>(NewIdx->getType())) {
                        Value* result = llvm::UndefValue::get(FixedVectorType::get(PtrMathTy, (unsigned)NewIdxVT->getNumElements()));
                        for (uint32_t j = 0; j < (uint32_t)NewIdxVT->getNumElements(); j++) {
                            result = Builder->CreateInsertElement(result, PointerValue, Builder->getInt32(j));
                        }
                        PointerValue = result;
                    }
                    PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
                    if (COffset)
                    {
                        PointerValue = Builder->CreateAdd(PointerValue, COffset);
                    }
                }
            }
        }
    }

    if (reducePointerArith)
    {
        IGC_ASSERT_MESSAGE(GEP->isInBounds(), "we can only do a zext if the GEP is inbound");
        if (!canReduceNegativeOffset)
        {
            PointerValue = Builder->CreateZExt(PointerValue, BasePointer->getType());
        }
        else
        {
            PointerValue = Builder->CreateSExt(PointerValue, BasePointer->getType());
        }
        PointerValue = Builder->CreateAdd(BasePointer, PointerValue);
    }
    PointerValue = Builder->CreateIntToPtr(PointerValue, GEP->getType());
    GEP->replaceAllUsesWith(PointerValue);
    GEP->eraseFromParent();

    return true;
}

bool GenIRLowering::constantFoldFMaxFMin(CallInst* GII,
    BasicBlock::iterator& BBI) const {
    // Constant fold fmax/fmin only.
    EOPCODE GIID = GetOpCode(GII);
    if (GIID != llvm_max && GIID != llvm_min)
        return false;

    // Skip fmax/fmin with non-constant operand.
    ConstantFP* CFP0 = dyn_cast<ConstantFP>(GII->getOperand(0));
    ConstantFP* CFP1 = dyn_cast<ConstantFP>(GII->getOperand(1));
    if (!CFP0 || !CFP1)
        return false;

    // Fold fmax/fmin following OpenCL spec.
    const APFloat& A = CFP0->getValueAPF();
    const APFloat& B = CFP1->getValueAPF();
    APFloat Result =
        (GIID == llvm_min) ? minnum(A, B) : maxnum(A, B);
    Constant* C = ConstantFP::get(GII->getContext(), Result);

    GII->replaceAllUsesWith(C);
    GII->eraseFromParent();

    return true;
}

bool GenIRLowering::combineFMaxFMin(CallInst* GII,
    BasicBlock::iterator& BBI) const {
    using namespace llvm::PatternMatch; // Scoped namespace using.

    // Fold fmax/fmin with all constant operands.
    if (constantFoldFMaxFMin(GII, BBI))
        return true;

    ConstantFP* CMin, * CMax;
    Value* X = nullptr;

    if (!match(GII, m_ClampWithConstants(m_Value(X), CMin, CMax)))
        return false;

    // Optimize chained clamp, i.e. combine
    // (clamp (clamp x, MIN, MAX), MIN, MAX) into
    // (clamp x, MIN, MAX)
    ConstantFP* CMin2, * CMax2;
    Value* X2 = nullptr;
    if (match(X, m_ClampWithConstants(m_Value(X2), CMin2, CMax2)) &&
        CMin == CMin2 && CMax == CMax2) {
        GII->replaceAllUsesWith(X);
        GII->eraseFromParent();

        return true;
    }

    // TODO: The following case should be combined as well
    //  (clamp (clamp x, MIN, MAX), MIN2, MAX2) into
    //  (clamp x, MIN3, MAX3), where
    // MIN3 = max(MIN, MIN2) and MAX3 = min(MAX, MAX2). The above case is just a
    // special case of this general form.

    if (!CMin->isZero() || !CMax->isExactlyValue(1.f))
        return false;

    // TODO: optimize chained fsat, i.e. combine
    // (fsat (fsat x)) into (fsat x)

    auto M = GII->getParent()->getParent()->getParent();
    GenISAIntrinsic::ID IID = GenISAIntrinsic::GenISA_fsat;
    Function* IFunc = GenISAIntrinsic::getDeclaration(M, IID, GII->getType());

    Instruction* I = Builder->CreateCall(IFunc, X);
    GII->replaceAllUsesWith(I);
    GII->eraseFromParent();

    BBI = llvm::BasicBlock::iterator(I);
    ++BBI;

    return true;
}

bool GenIRLowering::combineSelectInst(SelectInst* Sel,
    BasicBlock::iterator& BBI) const {
    using namespace llvm::PatternMatch; // Scoped namespace using.

    Value* LHS = nullptr;
    Value* RHS = nullptr;
    bool IsMax = false;
    unsigned Opcode = Instruction::UserOp1;

    if (Sel->getType()->isIntegerTy()) {
        IsMax = match(Sel, m_OrdFMaxCast(Opcode, m_Value(LHS), m_Value(RHS)));
        if (!IsMax &&
            !match(Sel, m_OrdFMinCast(Opcode, m_Value(LHS), m_Value(RHS))))
            return false;

        switch (Opcode) {
        default:
            return false;
        case Instruction::FPToSI:
        case Instruction::FPToUI:
        case Instruction::BitCast:
            break;
        }
    }
    else {
        IsMax = match(Sel, m_OrdFMax(m_Value(LHS), m_Value(RHS)));
        if (!IsMax && !match(Sel, m_OrdFMin(m_Value(LHS), m_Value(RHS))))
            return false;
    }

    IGCLLVM::Intrinsic IID =
        IsMax ? Intrinsic::maxnum : Intrinsic::minnum;
    Function* IFunc = Intrinsic::getDeclaration(
        Sel->getParent()->getParent()->getParent(), IID, LHS->getType());

    Instruction* I = Builder->CreateCall2(IFunc, LHS, RHS);
    BBI = BasicBlock::iterator(I); // Don't move to the next one. We still need combine for saturation.
    if (Opcode != Instruction::UserOp1) {
        I = cast<Instruction>(Builder->CreateCast(
            static_cast<Instruction::CastOps>(Opcode), I, Sel->getType()));
    }
    Sel->replaceAllUsesWith(I);
    Sel->eraseFromParent();

    return false;
}

FunctionPass* IGC::createGenIRLowerPass() {
    return new GenIRLowering();
}

// Register pass to igc-opt
#define PASS_FLAG "igc-gen-ir-lowering"
#define PASS_DESCRIPTION "Lowers GEP into primitive ones"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenIRLowering, PASS_FLAG, PASS_DESCRIPTION,
    PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(GenIRLowering, PASS_FLAG, PASS_DESCRIPTION,
        PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass* IGC::createGEPLoweringPass() {
    return new GEPLowering();
}

// Register pass to igc-opt
#define PASS_FLAG2 "igc-gep-lowering"
#define PASS_DESCRIPTION2 "Lowers GEP into primitive ones"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(GEPLowering, PASS_FLAG2, PASS_DESCRIPTION2,
    PASS_CFG_ONLY2, PASS_ANALYSIS2)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
    IGC_INITIALIZE_PASS_END(GEPLowering, PASS_FLAG2, PASS_DESCRIPTION2,
        PASS_CFG_ONLY2, PASS_ANALYSIS2)
