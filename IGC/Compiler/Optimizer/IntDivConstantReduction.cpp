/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/Optimizer/IntDivConstantReduction.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/igc_regkeys.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include <cmath>
#include <limits>
#include <type_traits>

using namespace llvm;

// This pass reduces division and remainder instructions with constant
// divisors (and moduli) to simpler multiplies.  This shows benefits even
// against hardware division.
//
// Constant divisors with types smaller than 32 bits with extend to 32b
// and then truncate back only if the divisor is not a power of two
// (the multiply-by-reciprocal case).
//
// This uses classic algorithms from Hackers Delight by Warren.
// While these algorithms exist already in LLVM, they are buried in the
// SelectionDAG/TargetLowering part and we don't reach that path.
// Furthermore, we tweak the algorithms for additional speed by taking
// advantage of GPU hardware behavior and idioms.
//
// We also deal with many simplistic reduction cases that other passes such as
// instruction combining or early CSE will probably remove, but we cover them
// here just in case this pass gets moved around.
struct IntDivConstantReduction : public FunctionPass
{
    static char ID;

    IntDivConstantReduction();

    /// @brief  Provides name of pass
    virtual StringRef getPassName() const override {
        return "IntDivConstantReductionPass";
    }

    bool isSignedPosNegPowerOf2(const APInt &d) {
        if (d.isNonNegative()) {
            return d.isPowerOf2();
        } else if (d.isMinSignedValue()) {
            return true;
        } else {
            return (-d).isPowerOf2();
        }
        return false;
    }

    virtual bool runOnFunction(Function& F) override {
        SmallVector<BinaryOperator*,4> divRems;

        for (auto ii = inst_begin(F), ie = inst_end(F); ii != ie; ii++) {
            Instruction *I = &*ii;
            //
            switch (I->getOpcode()) {
            case Instruction::SDiv:
            case Instruction::UDiv:
            case Instruction::SRem:
            case Instruction::URem:
                if (ConstantInt *divisor =
                        dyn_cast<ConstantInt>(I->getOperand(1)))
                {
                    divRems.push_back(cast<BinaryOperator>(I));
                }
                break;
            default: break;
            }
        }

        for (BinaryOperator *BO : divRems) {
            expandDivModByConst(F, BO, cast<ConstantInt>(BO->getOperand(1)));
        }

        return !divRems.empty();
    } // runOnFunction

    void expandDivModByConst(
        Function& F,
        BinaryOperator *divRem,
        ConstantInt *divisor)
    {
        IRBuilder<> Builder(divRem);

        Value *dividend = divRem->getOperand(0);

        bool isMod =
            divRem->getOpcode() == Instruction::SRem ||
            divRem->getOpcode() == Instruction::URem;
        bool isSigned =
            divRem->getOpcode() == Instruction::SDiv ||
            divRem->getOpcode() == Instruction::SRem;
        const uint64_t uVal = divisor->getZExtValue();

        APInt divisorValue = divisor->getValue();

        Value *result = nullptr;
        Value *zero = isSigned ?
            getConstantSInt(Builder, divisorValue.getBitWidth(), 0) :
            getConstantUInt(Builder, divisorValue.getBitWidth(), 0);

        if (uVal == 1) {
            // all bit sizes x signed and unsigned
            if (isMod) // X%1 == 0
                result = zero;
            else // X/1 == X
                result = dividend;
        } else if (isSigned && divisorValue.isAllOnesValue()) {
            if (isMod) // X%-1 == 0
                result = zero;
            else // X/-1 == -X
                result = Builder.CreateNeg(dividend);
        } else if (isSigned && isSignedPosNegPowerOf2(divisorValue)) {
            // signed power of two (positive/negative); includes minval
            result = expandPowerOf2Signed(
                Builder, isMod, divRem, dividend, divisorValue);
        } else if (!isSigned && divisorValue.isPowerOf2()) {
            result = expandPowerOf2Unsigned(
                Builder, dividend, divisorValue, isMod);
        } else {
            // non-power of twos require multiplication by a shifted reciprocal
            result = expandNonPowerOf2(
                F, Builder, dividend, divisor, isSigned, isMod);
        }

        divRem->replaceAllUsesWith(result);
        divRem->dropAllReferences();
        divRem->eraseFromParent();
    }

    Value *expandPowerOf2Unsigned(
        IRBuilder<> &Builder,
        Value *dividend,
        const APInt &divisor,
        bool isMod)
    {
        // unsigned power of two is a simple shift or mask
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        int shiftAmt = divisor.logBase2();
        Value *result;
        if (isMod) {
            if (shiftAmt == bitSize) { // X % MAX_VAL = X
                result = dividend;
            } else { // X % 2^K = (X & ((1<<K)-1))
                result = Builder.CreateAnd(dividend, ((1ull << shiftAmt)-1));
            }
        } else {
            result = Builder.CreateLShr(dividend, shiftAmt);
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Support for powers of two (unsigned is handled in the main case)
    Value *expandPowerOf2Signed(
        IRBuilder<> &Builder,
        bool isMod,
        BinaryOperator *divRem,
        Value *dividend,
        const APInt &divisor)
    {
        // Signed power of two use a shift/add sequence.
        // (It's a little harder because of negative values.)
        //
        // c.f. Hacker's Delight 10-1
        //
        //    %t0 = ashr %n,  (k-1)
        //    %t1 = lshr %t0, (BITS_SIZE-k)
        //    %t2 = add  %t1, n
        //    %q  = ashr %t2, k
        //  [for negative divisors]
        //    %q  = neg %q
        //
        // c.f. Hacker's Delight 10-2
        //    for the remainder/modulus algorithm
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        int shiftAmt;
        int64_t twoToTheKminus1;
        if (divisor.isNonNegative()) {
            shiftAmt = divisor.logBase2();
            twoToTheKminus1 = ((1LL << shiftAmt) - 1);
        } else { // negative divisor (including MIN_VAL)
            shiftAmt = (-divisor).logBase2();
            twoToTheKminus1 = ((1LL << shiftAmt) - 1);
        }

        ///////////////////////////////////////////////////////////////////////
        // Compute 2^k - 1 (to fixup negative dividends)
        Value *result = getPowerOf2SignedFixupDividend(
            divRem, Builder, dividend, shiftAmt, twoToTheKminus1, isMod);
        if (isMod) {
            // C.f. Hacker's Delight 10-2
            // faster than using the re-multiply quotient and subtract
            result = Builder.CreateAnd(result, -(1LL << shiftAmt));
            result = Builder.CreateSub(dividend, result, "rem");
        } else {
            if (divisor.isNegative()) {
                result = Builder.CreateAShr(result, shiftAmt, "neg_qot");
                result = Builder.CreateNeg(result, "qot");
            } else {
                result = Builder.CreateAShr(result, shiftAmt, "qot");
            }
        }

        return result;
    }

    // get's the fixed up signed dividend
    Value *getPowerOf2SignedFixupDividend(
        BinaryOperator *divRem,
        IRBuilder<> &Builder,
        Value *dividend,
        int shiftAmt,
        int64_t twoToTheKminus1,
        bool isMod)
    {
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        auto algorithm = IGC_GET_FLAG_VALUE(EnableConstIntDivReduction);
        if (algorithm == 3) {
            // shift-only version of this:
            //   %t0 = %ashr  %dividend, K-1
            //   %t1 = %lshr  %t0, BIT_SIZE-K
            //   %d2 = add %dividend, %t1
            //   ... as before ...
            Value *t0 = Builder.CreateAShr(dividend, shiftAmt - 1);
            Value *t1 = Builder.CreateLShr(t0, bitSize - shiftAmt);
            return Builder.CreateAdd(t1, dividend);
        } else if (algorithm == 2) {
            // the select() approach
            //   %pr = icmp %n, 0
            //   %t1 = select %pr, (2^k-1), 0
            //   %d2 = add %dividend, %t1
            //   ... as before ...
            Value *twoToTheKMinusOne =
                getConstantSInt(Builder, bitSize, twoToTheKminus1);
            Value *zero = getConstantSInt(Builder, bitSize, 0);
            Value *p = Builder.CreateICmpSLT(dividend, zero);
            Value *t1 = Builder.CreateSelect(p, twoToTheKMinusOne, zero);
            return Builder.CreateAdd(t1, dividend);
        } else {
            // a conditional add version (the default approach)
            //
            // since we don't have an intrinsic to conditionally add, we must
            // emulate with a short branch (which will get simplified later)
            //
            //   %neg = icmp %dividend, 0
            //   br %neg, %fix-dividend %dividend-fixed
            //
            //   fix-dividend:
            //   %fixed-divisor = add %dividend, (2^K-1)
            //
            //   dividend-fixed:
            //   %fixed_dividend = phi [], []
            //   ashr ...
            //
            // NOTE: we rely on the later phase to simplify to GEN ISA:
            //          cmp  (f0.0)lt  null:d num:d       0:d
            //   (f0.0) add            num:d  num:d (2^K-1):d
            //          asr ...
            // otherwise this is probably a losing strategy and select
            // is better
            BasicBlock *originalBlock = Builder.GetInsertBlock();
            Value *zero = getConstantSInt(Builder, bitSize, 0);
            Value *isNegative = Builder.CreateICmpSLT(dividend, zero, "neg");
            //
            auto *t = SplitBlockAndInsertIfThen(isNegative, divRem, false);
            BasicBlock *pow2NegDividend = t->getParent();
            pow2NegDividend->setName(
                isMod ? "srem_power_of_2_negative" : "sdiv_power_of_2_negative");
            BasicBlock *afterFixed = t->getSuccessor(0);
            afterFixed->setName(isMod ? "srem_power_of_2" : "sdiv_power_of_2");
            // resets the builder to just before the next block
            // if (afterFixed->empty())
            //     Builder.SetInsertPoint(afterFixed);
            // else
            //    Builder.SetInsertPoint(&afterFixed->front());
            //
            IRBuilder<> addBuilder(t);
            addBuilder.SetInstDebugLocation(divRem);
            Value *twoKM1 = getConstantSInt(Builder, bitSize, twoToTheKminus1);
            Value *fixedDivisor = addBuilder.CreateAdd(
                dividend, twoKM1, "fixed_dividend");
            //
            Builder.SetInsertPoint(divRem);
            PHINode *phi =
                Builder.CreatePHI(dividend->getType(), 2, "fixed_dividend");
            phi->addIncoming(dividend, originalBlock);
            phi->addIncoming(fixedDivisor, pow2NegDividend);
            //
            return phi;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Support for non-powers of two
    Value *expandNonPowerOf2(
        Function &F,
        IRBuilder<> &Builder,
        Value *dividend,
        ConstantInt *divisor,
        bool isSigned,
        bool isMod)
    {
        APInt divisorValue = divisor->getValue();

        Value *result;
        if (divisorValue.getBitWidth() < 32) {
            // if bitsize < 32, widen and perform the multiplication by
            // pseudo-inverse as 32b.
            //
            // NOTE: we could apply the same gimmick as with 32-64 bit,
            // if it were important enough.  One just has to compute the
            // magic value table etc...
            Value *dividend32;
            ConstantInt *divisor32;
            if (isSigned) {
                dividend32 = Builder.CreateSExt(
                    dividend, Builder.getInt32Ty(), "dividend32");
                divisor32 = Builder.getInt32(
                    (uint32_t)divisorValue.getSExtValue());
            } else {
                dividend32 = Builder.CreateZExt(
                    dividend, Builder.getInt32Ty(), "dividend32");
                divisor32 = Builder.getInt32(
                    (uint32_t)divisorValue.getZExtValue());
            }
            //
            result = expandNonPowerOf2Divide(
                F, Builder, dividend32, divisor32, isSigned);
            result = Builder.CreateTrunc(result, dividend->getType(), "quot");
        } else {
            // either 32b or 64b
            result = expandNonPowerOf2Divide(
                F, Builder, dividend, divisor, isSigned);
        }

        if (isMod)
            result = expandModFromQuotient(Builder, dividend, divisor, result);

        return result;
    }

    Value *expandNonPowerOf2Divide(
        Function &F,
        IRBuilder<> &Builder,
        Value *dividend,
        ConstantInt *divisor,
        bool isSigned)
    {
        return isSigned ?
            expandNonPowerOf2SignedDivide(
                F, Builder, dividend, divisor->getValue()) :
            expandNonPowerOf2UnsignedDivide(
                F, Builder, dividend, divisor->getValue());
    }

    Value *expandNonPowerOf2SignedDivide(
        Function &F,
        IRBuilder<> &Builder,
        Value *dividend,
        const APInt &divisor)
    {
        // C.f. Hacker's Delight 10-4 and 10-5
        //
        //   Use static lookup of approximate reciprocal R with shift S.
        //
        // %qAppx0 = mulh R,        %n      -- retain the high 32b of mul
        // [positive divisor]
        //   %qAppx1 = add  %qApx0, %n
        // [negative divisor]
        //   %qAppx1 = sub  %qApx0, %n
        // %qAppx2 = ashr %qApx1, S
        // [positive divisor]
        //   %sgnBit = lshr %n,     31       -- add 1 if n negative
        // [negative divisor]
        //   %sgnBit = lshr %qApx2, 31       -- add 1 if q negative (N positive)
        // %q = add  %qApx2, %sgnBit
        //
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        //
        APInt::ms appxRecip = divisor.magic();
        //
        ConstantInt *appxRcp = getConstantSInt(
            Builder, bitSize, appxRecip.m.getSExtValue());
        Value *appxQ =
            CreateMulh(F, Builder, true, dividend, appxRcp);
        if (divisor.isStrictlyPositive() && appxRecip.m.isNegative()) {
            appxQ = Builder.CreateAdd(appxQ, dividend, "q_appx");
        }
        if (divisor.isNegative() && appxRecip.m.isStrictlyPositive()) {
            appxQ = Builder.CreateSub(appxQ, dividend, "q_appx");
        }
        if (appxRecip.s > 0) {
            ConstantInt *shift =
                getConstantSInt(Builder, bitSize, appxRecip.s);
            appxQ = Builder.CreateAShr(appxQ, shift, "q_appx");
        }
        //
        // Extract the sign bit and add it to the quotient
        ConstantInt *shiftSignBit =
            getConstantSInt(Builder, bitSize, bitSize - 1);
        Value *sign = Builder.CreateLShr(appxQ, shiftSignBit, "q_sign");
        appxQ = Builder.CreateAdd(appxQ, sign, "q");
        //
        return appxQ;
    }

    Value *expandNonPowerOf2UnsignedDivide(
        Function &F,
        IRBuilder<> &Builder,
        Value *dividend,
        const APInt &divisor)
    {
        //////////////////////////////////////////////////
        // C.f. Hacker's Delight 10-8
        APInt::mu appxRecip = divisor.magicu();
        //
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        //
        // even divisors can pre-shift the dividend to avoid
        // extra work at the end.
        Value *shiftedDividend = dividend;
        if (appxRecip.a && !divisor[0]) {
            unsigned s = divisor.countTrailingZeros();
            shiftedDividend = Builder.CreateLShr(shiftedDividend, s);
            appxRecip = divisor.lshr(s).magicu(s);
            assert(!appxRecip.a && "expected to subtract now");
            assert(appxRecip.s < divisor.getBitWidth() && "undefined shift");
        }
        //
        ConstantInt *appxRcp = getConstantUInt(
            Builder, bitSize, appxRecip.m.getZExtValue());
        Value *appxQ =
            CreateMulh(F, Builder, false, shiftedDividend, appxRcp);
        //
        if (!appxRecip.a) {
            appxQ = Builder.CreateLShr(appxQ, appxRecip.s, "q_appx");
        } else {
            Value *fixup = Builder.CreateSub(dividend, appxQ, "q_appx");
            fixup = Builder.CreateLShr(fixup, 1);
            appxQ = Builder.CreateAdd(fixup, appxQ, "q_appx");
            appxQ = Builder.CreateLShr(appxQ, appxRecip.s - 1, "q_appx");
        }
        return appxQ;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Helpers
    Value *expandModFromQuotient(
        IRBuilder<> &Builder,
        Value *dividend,
        ConstantInt *divisor,
        Value *quotient)
    {
        //   r = n - (n/d)*d
        Value *qd = Builder.CreateMul(quotient, divisor, "q_times_d");
        return Builder.CreateSub(dividend, qd, "rem");
    }

    Value *CreateMulh(
        Function &F,
        IRBuilder<> &Builder,
        bool isSigned,
        Value *u,
        Value *v)
    {
        int bitWidth = u->getType()->getIntegerBitWidth();
        if (bitWidth == 32) {
            // we have a dedicated machine instruction for 32b
            SmallVector<Value*,2> imulhArgs;
            imulhArgs.push_back(u);
            imulhArgs.push_back(v);
            auto intrinsic = isSigned ?
                GenISAIntrinsic::GenISA_imulH :
                GenISAIntrinsic::GenISA_umulH;
            Function *iMulhDecl = llvm::GenISAIntrinsic::getDeclaration(
                F.getParent(),
                intrinsic,
                v->getType());
            return Builder.CreateCall(iMulhDecl, imulhArgs, "q_appx");
        } else if (bitWidth == 64) {
            // emulate via 64b arithmetic
            if (isSigned) {
                return CreateMulhS64(Builder, u, v);
            } else {
                return CreateMulhU64(Builder, u, v);
            }
        } else {
            assert(0 && "CreateMulH must be 32 or 64");
            return nullptr;
        }
    }

    Value *CreateMulhS64(IRBuilder<> &Builder, Value *u, Value *v) const {
        // This comes from Hacker's Delight 8-2.
        // Think of this as elementry schoole multiplication, but base 2^32.
        ConstantInt *loMask = getConstantSInt(Builder, 64, 0xFFFFFFFFll);
        ConstantInt *hiShift = getConstantSInt(Builder, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF; s64 u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF; s64 v1 = v >> 32;
        Value *u0 = Builder.CreateAnd(u, loMask, "u.lo32");
        Value *u1 = Builder.CreateAShr(u, hiShift, "u.hi32");
        Value *v0 = Builder.CreateAnd(v,  loMask, "v.lo32");
        Value *v1 = Builder.CreateAShr(v, hiShift, "v.hi32");
        //
        // w = u0*v0
        Value *w0 = Builder.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value *tLHS = Builder.CreateMul(u1, v0);
        Value *tRHS = Builder.CreateLShr(w0, hiShift, "w0.lo32");
        Value *t = Builder.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value *u0v1 = Builder.CreateMul(u0, v1);
        Value *tLO32 = Builder.CreateAnd(t, loMask, "t.lo32");
        Value *w1 = Builder.CreateAdd(u0v1, tLO32, "w1");
        //
        // return u0*v1 + (t >> 32) + (w1 >> 32)
        Value *u1v1 = Builder.CreateMul(u1, v1);
        Value *tHI32 = Builder.CreateAShr(t, hiShift, "t.hi32");
        Value *rLHS = Builder.CreateAdd(u1v1, tHI32);
        Value *rRHS = Builder.CreateAShr(w1, hiShift, "w1.lo32");
        Value *r = Builder.CreateAdd(rLHS, rRHS, "uv");
        //
        return r;
    }

    Value *CreateMulhU64(IRBuilder<> &Builder, Value *u, Value *v) const {
        // This is the same as CreateMulhS64, but with all logical shifts.
        ConstantInt *loMask = getConstantUInt(Builder, 64, 0xFFFFFFFFull);
        ConstantInt *hiShift = getConstantUInt(Builder, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF, u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF, v1 = v >> 32;
        Value *u0 = Builder.CreateAnd(u, loMask, "u.lo32");
        Value *u1 = Builder.CreateLShr(u, hiShift, "u.hi32");
        Value *v0 = Builder.CreateAnd(v, loMask, "v.lo32");
        Value *v1 = Builder.CreateLShr(v, hiShift, "v.hi32");
        //
        // w0 = u0*v0
        Value *w0 = Builder.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value *tLHS = Builder.CreateMul(u1, v0);
        Value *tRHS = Builder.CreateLShr(w0, hiShift, "w0.lo32");
        Value *t = Builder.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value *u0v1 = Builder.CreateMul(u0, v1);
        Value *tLO32 = Builder.CreateAnd(t, loMask, "t.lo32");
        Value *w1 = Builder.CreateAdd(u0v1, tLO32, "w1");
        //
        // w1 = u0*v1 + (t >> 32) + (w1 >> 32)
        Value *u1v1 = Builder.CreateMul(u1, v1);
        Value *tHI32 = Builder.CreateLShr(t, hiShift, "t.hi32");
        Value *rLHS = Builder.CreateAdd(u1v1, tHI32);
        Value *rRHS = Builder.CreateLShr(w1, hiShift, "w1.lo32");
        Value *r = Builder.CreateAdd(rLHS, rRHS, "uv");
        //
        return r;
    }

    ConstantInt *getConstantSInt(
        IRBuilder<> &Builder, int bitSize, int64_t val) const
    {
        switch (bitSize) {
        case 8: return Builder.getInt8((uint8_t)val);
        case 16: return Builder.getInt16((uint16_t)val);
        case 32: return Builder.getInt32((uint32_t)val);
        case 64: return Builder.getInt64((uint64_t)val);
        default:
            assert(false && "invalid bitsize");
            return nullptr;
        }
    }
    ConstantInt *getConstantUInt(
        IRBuilder<> &Builder, int bitSize, uint64_t val) const
    {
        switch (bitSize) {
        case 8: return Builder.getInt8((uint8_t)val);
        case 16: return Builder.getInt16((uint16_t)val);
        case 32: return Builder.getInt32((uint32_t)val);
        case 64: return Builder.getInt64(val);
        default:
            assert(false && "invalid bitsize");
            return nullptr;
        }
    }
};

char IntDivConstantReduction::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-intdiv-red"
#define PASS_DESCRIPTION "Integer Division Constant Reduction"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(IntDivConstantReduction,
    PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(IntDivConstantReduction,
        PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

IntDivConstantReduction::IntDivConstantReduction() : FunctionPass(ID) {
    initializeIntDivConstantReductionPass(*PassRegistry::getPassRegistry());
}

llvm::FunctionPass* IGC::createIntDivConstantReductionPass()
{
    return new IntDivConstantReduction();
}


