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
#include "Probe/Assertion.h"

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
    // for 64b this enables a compare and 32b path
    // enable this if we expect dividends to be small in pratice
    // (e.g. get_global_id(X) is often 32b if multiple dimensions are used)
    static const bool DYNAMIC64b_AS_32b = true;

    static char ID;

    BinaryOperator *currDivRem = nullptr;

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
            currDivRem = BO;
            expandDivModByConst(F, BO, cast<ConstantInt>(BO->getOperand(1)));
        }
        currDivRem = nullptr;

        return !divRems.empty();
    } // runOnFunction

    void expandDivModByConst(
        Function &F,
        BinaryOperator *divRem,
        ConstantInt *divisor)
    {
        IRBuilder<> B(divRem);

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
            getConstantSInt(B, divisorValue.getBitWidth(), 0) :
            getConstantUInt(B, divisorValue.getBitWidth(), 0);

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
                result = B.CreateNeg(dividend);
        } else if (isSigned && isSignedPosNegPowerOf2(divisorValue)) {
            // signed power of two (positive/negative); includes minval
            result = expandPowerOf2Signed(
                B, isMod, divRem, dividend, divisorValue);
        } else if (!isSigned && divisorValue.isPowerOf2()) {
            result = expandPowerOf2Unsigned(
                B, dividend, divisorValue, isMod);
        } else {
            // non-power of twos require multiplication by a shifted reciprocal
            result = expandNonPowerOf2(
                F, divRem, B, dividend, divisor, isSigned, isMod);
        }

        divRem->replaceAllUsesWith(result);
        divRem->dropAllReferences();
        divRem->eraseFromParent();
    }

    Value *expandPowerOf2Unsigned(
        IRBuilder<> &B,
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
                result = B.CreateAnd(dividend, ((1ull << shiftAmt)-1));
            }
        } else {
            result = B.CreateLShr(dividend, shiftAmt);
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Support for powers of two (unsigned is handled in the main case)
    Value *expandPowerOf2Signed(
        IRBuilder<> &B,
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
            B, dividend, shiftAmt, twoToTheKminus1, isMod);
        if (isMod) {
            // C.f. Hacker's Delight 10-2
            // faster than using the re-multiply quotient and subtract
            result = B.CreateAnd(result, -(1LL << shiftAmt));
            result = B.CreateSub(dividend, result, "rem");
        } else {
            if (divisor.isNegative()) {
                result = B.CreateAShr(result, shiftAmt, "neg_qot");
                result = B.CreateNeg(result, "qot");
            } else {
                result = B.CreateAShr(result, shiftAmt, "qot");
            }
        }

        return result;
    }

    // gets the fixed up signed dividend for power of two
    // this selects between different methods of doing that.
    // The value of EnableConstIntDivReduction controls this
    //   either via shifts (3), compare and select (2),
    //   or a via a predicated add (1 or anything else)
    Value *getPowerOf2SignedFixupDividend(
        IRBuilder<> &B,
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
            Value *t0 = B.CreateAShr(dividend, shiftAmt - 1);
            Value *t1 = B.CreateLShr(t0, bitSize - shiftAmt);
            return B.CreateAdd(t1, dividend);
        } else {
            // this handles both the conditional add and select version.
            //
            // (1) a conditional add version (the default approach)
            //
            // NOTE: we rely on the later phase to simplify to GEN ISA:
            //          cmp  (f0.0)lt  null:d num:d       0:d
            //   (f0.0) add            num:d  num:d (2^K-1):d
            //          asr ...
            // otherwise this is probably a losing strategy and select
            // is better
            //
            // (2) using a select
            //          cmp    (f0.0)lt  null:d num:d       0:d
            //          sel.f0.0         rounding:d   (2^K-1):d   0:d
            //          add              num:d   num:d  rounding:d
            Value *zero = getConstantSInt(B, bitSize, 0);
            Value *isNegative = B.CreateICmpSLT(dividend, zero, "is-neg");
            ConstantInt *twoKm1 = getConstantSInt(B, bitSize, twoToTheKminus1);
            return CreatePredicatedAdd(
                bitSize, B, currDivRem, isNegative, dividend, twoKm1);
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    // Support for non-powers of two
    Value *expandNonPowerOf2(
        Function &F,
        BinaryOperator *divRem,
        IRBuilder<> &B,
        Value *dividend,
        ConstantInt *divisor,
        bool isSigned,
        bool isMod)
    {
        const APInt divisorValue = divisor->getValue();
        if (DYNAMIC64b_AS_32b &&
            divisorValue.getBitWidth() == 64 &&
            valueFitsIn32b(divisorValue, isSigned))
        {
            // if the value is 64b and the divisor is small enough to
            // fit into 32b, then we can possibly use 32b division
            // (if the dividend fits in 32b)
            return expand64bNonPowerOf2(
                F, divRem, B, dividend, divisor, isSigned, isMod);
        }

        Value *result;
        if (divisorValue.getBitWidth() < 32) {
            // if bitsize < 32, widen and perform the multiplication by
            // pseudo-inverse as 32b.
            //
            // NOTE: we could apply the same gimmick as with 32-64 bit,
            // if it were important enough.  We just have to implement
            // mulh for 16b
            Value *dividend32;
            ConstantInt *divisor32;
            if (isSigned) {
                dividend32 = B.CreateSExt(
                    dividend, B.getInt32Ty(), "dividend32");
                divisor32 = B.getInt32(
                    (uint32_t)divisorValue.getSExtValue());
            } else {
                dividend32 = B.CreateZExt(
                    dividend, B.getInt32Ty(), "dividend32");
                divisor32 = B.getInt32(
                    (uint32_t)divisorValue.getZExtValue());
            }
            //
            result = expandNonPowerOf2Divide(
                F, B, currDivRem, dividend32, divisor32, isSigned);
            result = B.CreateTrunc(result, dividend->getType(), "quot");
        } else {
            // 32b or 64b
            result = expandNonPowerOf2Divide(
                F, B, currDivRem, dividend, divisor, isSigned);
        }

        // if we want the remainder, we use multiplication to get it
        // r = n - q/d
        if (isMod)
            result = expandModFromQuotient(B, dividend, divisor, result);

        return result;
    }

    bool valueFitsIn32b(const APInt &value, bool isSigned) const {
        return isSigned ?
            (value.getSExtValue() >= std::numeric_limits<int32_t>::min() &&
                value.getSExtValue() <= std::numeric_limits<int32_t>::max()) :
            value.getZExtValue() <= std::numeric_limits<uint32_t>::max();
    }

    // this does a conditional check to use 32b if the value is small enough
    Value *expand64bNonPowerOf2(
        Function &F,
        BinaryOperator *divRem,
        IRBuilder<> &B,
        Value *dividend,
        ConstantInt *divisor,
        bool isSigned,
        bool isMod)
    {
        Value *is32b;
        if (isSigned) {
            ConstantInt *min32 =
                B.getInt64((uint64_t)std::numeric_limits<int32_t>::min());
            Value *isGteMin = B.CreateICmpSGE(dividend, min32);
            ConstantInt *max32 =
                B.getInt64((uint64_t)std::numeric_limits<int32_t>::max());
            Value *isLteMax = B.CreateICmpSLE(dividend, max32);
            is32b = B.CreateAnd(isGteMin, isLteMax);
        } else {
            Value *max32 = B.getInt64(
                (uint64_t)std::numeric_limits<uint32_t>::max());
            is32b = B.CreateICmpULE(dividend, max32);
        }

#if LLVM_VERSION_MAJOR <= 7
        // LLVM 7 and lower have the TerminatorInst
        // later versions simplify this to Instruction
        using TermInst = TerminatorInst;
#else
        using TermInst = Instruction;
#endif
        TermInst *thenT = nullptr, *elseT = nullptr;
        SplitBlockAndInsertIfThenElse(is32b, divRem, &thenT, &elseT);

        ///////////////////////////////////////////////////////////////////////
        // narrow to 32b
        thenT->getParent()->setName(
            isSigned ? "sdiv_pow2_64b_as_32b" : "udiv_pow2_64b_as_32b");
        //
        IRBuilder<> B32(thenT);
        B32.SetCurrentDebugLocation(currDivRem->getDebugLoc());
        Value *dividend32 = B32.CreateTrunc(dividend, B32.getInt32Ty());
        ConstantInt *divisor32 =
            B32.getInt32((uint32_t)divisor->getValue().getZExtValue());
        Value *result32 =
            expandNonPowerOf2Divide(
                F,
                B32,
                thenT,
                dividend32,
                divisor32,
                isSigned);
        if (isMod) {
            // if we're after a 64b mod and things fit in 32b, then we can
            // use the expand-from-mod as 32b before widening back
            result32 =
                expandModFromQuotient(B32, dividend32, divisor32, result32);
        }
        // widen back to 64b
        Value *result32_64 = isSigned ?
            B32.CreateSExt(result32, B32.getInt64Ty()) :
            B32.CreateZExt(result32, B32.getInt64Ty());

        ///////////////////////////////////////////////////////////////////////
        // regular 64b path
        elseT->getParent()->setName(
            isSigned ? "sdiv_pow2_64b" : "udiv_pow2_64b");
        //
        IRBuilder<> B64(elseT);
        B64.SetCurrentDebugLocation(currDivRem->getDebugLoc());
        Value *result64 =
            expandNonPowerOf2Divide(F, B64, elseT, dividend, divisor, isSigned);
        if (isMod) {
            result64 =
                expandModFromQuotient(B64, dividend, divisor, result64);
        }

        ///////////////////////////////////////////////////////////////////////
        B.SetInsertPoint(divRem);
        PHINode *phi = B.CreatePHI(dividend->getType(), 2);
        phi->addIncoming(result32_64, thenT->getParent());
        phi->addIncoming(result64, elseT->getParent());

        return phi;
    }

    Value *expandNonPowerOf2Divide(
        Function &F,
        IRBuilder<> &B,
        Instruction *end,
        Value *dividend,
        ConstantInt *divisor,
        bool isSigned)
    {
        return isSigned ?
            expandNonPowerOf2SignedDivide(
                F, B, end, dividend, divisor->getValue()) :
            expandNonPowerOf2UnsignedDivide(
                F, B,      dividend, divisor->getValue());
    }

    Value *expandNonPowerOf2SignedDivide(
        Function &F,
        IRBuilder<> &B,
        Instruction *end,
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
        //   %sgnBit = lshr %qApx2, 31       -- add 1 if q negative (N pos.)
        // %q = add  %qApx2, %sgnBit
        //
        const int bitSize = dividend->getType()->getIntegerBitWidth();
        //
        APInt::ms appxRecip = divisor.magic();
        //
        ConstantInt *appxRcp = getConstantSInt(
            B, bitSize, appxRecip.m.getSExtValue());
        Value *appxQ =
            CreateMulh(F, B, true, dividend, appxRcp);
        if (divisor.isStrictlyPositive() && appxRecip.m.isNegative()) {
            appxQ = B.CreateAdd(appxQ, dividend, "q_appx");
        }
        if (divisor.isNegative() && appxRecip.m.isStrictlyPositive()) {
            appxQ = B.CreateSub(appxQ, dividend, "q_appx");
        }
        if (appxRecip.s > 0) {
            ConstantInt *shift = getConstantSInt(B, bitSize, appxRecip.s);
            appxQ = B.CreateAShr(appxQ, shift, "q_appx");
        }
        //
        // Extract the sign bit and add it to the quotient
        if (IGC_GET_FLAG_VALUE(EnableConstIntDivReduction) == 3) {
            ConstantInt *shiftSignBit =
                getConstantSInt(B, bitSize, bitSize - 1);
            Value *sign = B.CreateLShr(appxQ, shiftSignBit, "q_sign");
            appxQ = B.CreateAdd(appxQ, sign, "q");
        } else {
            ConstantInt *zero = getConstantSInt(B, bitSize, 0);
            ConstantInt *one = getConstantSInt(B, bitSize, 1);
            Value *negative = B.CreateICmpSLT(appxQ, zero);
            appxQ =
                CreatePredicatedAdd(bitSize, B, end, negative, appxQ, one);
        }
        return appxQ;
    }

    Value *expandNonPowerOf2UnsignedDivide(
        Function &F,
        IRBuilder<> &B,
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
            shiftedDividend = B.CreateLShr(shiftedDividend, s);
            appxRecip = divisor.lshr(s).magicu(s);
            IGC_ASSERT_MESSAGE(!appxRecip.a, "expected to subtract now");
            IGC_ASSERT_MESSAGE(appxRecip.s < divisor.getBitWidth(), "undefined shift");
        }
        //
        ConstantInt *appxRcp = getConstantUInt(
            B, bitSize, appxRecip.m.getZExtValue());
        Value *appxQ =
            CreateMulh(F, B, false, shiftedDividend, appxRcp);
        //
        if (!appxRecip.a) {
            appxQ = B.CreateLShr(appxQ, appxRecip.s, "q_appx");
        } else {
            Value *fixup = B.CreateSub(dividend, appxQ, "q_appx");
            fixup = B.CreateLShr(fixup, 1);
            appxQ = B.CreateAdd(fixup, appxQ, "q_appx");
            appxQ = B.CreateLShr(appxQ, appxRecip.s - 1, "q_appx");
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

    ///////////////////////////////////////////////////////////////////////////
    // various builder helpers
    ///////////////////////////////////////////////////////////////////////////

    // a conditional add via either a short branch (predicated add in GEN ISA)
    // or via a select (select in GEN ISA)
    //
    // %x1 = cadd TYPE %p, %x, %addend
    // %x1 is %x if not %p else (%x + %addend)
    Value *CreatePredicatedAdd(
        int bitSize,
        IRBuilder<> &B, // current insert location
        Instruction *end, // join point (a TerminatorInst or the DivRem op)
        Value *pred, Value *x, Value *addend) const
    {
        if (IGC_GET_FLAG_VALUE(EnableConstIntDivReduction) == 2) {
            // use a select:
            //   %addend1 = select %p, %addend, 0
            //   add %x, %addend1
            Value *zero = getConstantSInt(B, bitSize, 0);
            Value *addend1 = B.CreateSelect(pred, addend, zero);
            return B.CreateAdd(x, addend1);
        } else {
            // create a short block:
            //   parent:
            //     ...
            //     %x = ...
            //     %p = icmp ...
            //     br %p, label %cadd, label %done
            //   cadd:
            //     %x1 = add %x, %addend
            //   done:
            //     %r = phi [top,%x], [cadd,%x1]
            //
            // We are counting on this boiling down to
            //  (f0.0)   add ... x, addend
            BasicBlock *parent = B.GetInsertBlock();
            auto *t = SplitBlockAndInsertIfThen(pred, end, false);
            t->getParent()->setName("cond-add");
            BasicBlock *join = t->getSuccessor(0);
            join->setName("cond-add-join");
            //
            IRBuilder<> BA(t);
            BA.SetInstDebugLocation(currDivRem);
            Value *x1 = BA.CreateAdd(x, addend, x->getName());
            //
            B.SetInsertPoint(end); // restore the builder's position
            PHINode *phi = B.CreatePHI(x->getType(), 2, x->getName());
            phi->addIncoming(x, parent);
            phi->addIncoming(x1, t->getParent());
            //
            return phi;
        }
    }

    Value *CreateMulh(
        Function &F,
        IRBuilder<> &B,
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
            return B.CreateCall(iMulhDecl, imulhArgs, "q_appx");
        } else if (bitWidth == 64) {
            // emulate via 64b arithmetic
            if (isSigned) {
                return CreateMulhS64(B, u, v);
            } else {
                return CreateMulhU64(B, u, v);
            }
        } else {
            IGC_ASSERT_MESSAGE(0, "CreateMulH must be 32 or 64");
            return nullptr;
        }
    }

    Value *CreateMulhS64(IRBuilder<> &B, Value *u, Value *v) const {
        // This comes from Hacker's Delight 8-2.
        // Think of this as elementry schoole multiplication, but base 2^32.
        ConstantInt *loMask = getConstantSInt(B, 64, 0xFFFFFFFFll);
        ConstantInt *hiShift = getConstantSInt(B, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF; s64 u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF; s64 v1 = v >> 32;
        Value *u0 = B.CreateAnd(u, loMask, "u.lo32");
        Value *u1 = B.CreateAShr(u, hiShift, "u.hi32");
        Value *v0 = B.CreateAnd(v,  loMask, "v.lo32");
        Value *v1 = B.CreateAShr(v, hiShift, "v.hi32");
        //
        // w = u0*v0
        Value *w0 = B.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value *tLHS = B.CreateMul(u1, v0);
        Value *tRHS = B.CreateLShr(w0, hiShift, "w0.lo32");
        Value *t = B.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value *u0v1 = B.CreateMul(u0, v1);
        Value *tLO32 = B.CreateAnd(t, loMask, "t.lo32");
        Value *w1 = B.CreateAdd(u0v1, tLO32, "w1");
        //
        // return u0*v1 + (t >> 32) + (w1 >> 32)
        Value *u1v1 = B.CreateMul(u1, v1);
        Value *tHI32 = B.CreateAShr(t, hiShift, "t.hi32");
        Value *rLHS = B.CreateAdd(u1v1, tHI32);
        Value *rRHS = B.CreateAShr(w1, hiShift, "w1.lo32");
        Value *r = B.CreateAdd(rLHS, rRHS, "uv");
        //
        return r;
    }

    Value *CreateMulhU64(IRBuilder<> &B, Value *u, Value *v) const {
        // This is the same as CreateMulhS64, but with all logical shifts.
        ConstantInt *loMask = getConstantUInt(B, 64, 0xFFFFFFFFull);
        ConstantInt *hiShift = getConstantUInt(B, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF, u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF, v1 = v >> 32;
        Value *u0 = B.CreateAnd(u, loMask, "u.lo32");
        Value *u1 = B.CreateLShr(u, hiShift, "u.hi32");
        Value *v0 = B.CreateAnd(v, loMask, "v.lo32");
        Value *v1 = B.CreateLShr(v, hiShift, "v.hi32");
        //
        // w0 = u0*v0
        Value *w0 = B.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value *tLHS = B.CreateMul(u1, v0);
        Value *tRHS = B.CreateLShr(w0, hiShift, "w0.lo32");
        Value *t = B.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value *u0v1 = B.CreateMul(u0, v1);
        Value *tLO32 = B.CreateAnd(t, loMask, "t.lo32");
        Value *w1 = B.CreateAdd(u0v1, tLO32, "w1");
        //
        // w1 = u0*v1 + (t >> 32) + (w1 >> 32)
        Value *u1v1 = B.CreateMul(u1, v1);
        Value *tHI32 = B.CreateLShr(t, hiShift, "t.hi32");
        Value *rLHS = B.CreateAdd(u1v1, tHI32);
        Value *rRHS = B.CreateLShr(w1, hiShift, "w1.lo32");
        Value *r = B.CreateAdd(rLHS, rRHS, "uv");
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
            IGC_ASSERT_MESSAGE(0, "invalid bitsize");
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
            IGC_ASSERT_MESSAGE(0, "invalid bitsize");
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


