/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TypeLegalizer.h"
#include "InstExpander.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "type-legalizer"

using namespace llvm;
using namespace IGC::Legalizer;

bool InstExpander::expand(Instruction* I) {
    IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));
    IRB->SetCurrentDebugLocation(I->getDebugLoc());
    Expanded.clear();

    if (!visit(*I))
        return false;

    if (!Expanded.empty())
        TL->setLegalizedValues(I, Expanded);

    return true;
}

// By default, capture all missing instructions!
bool InstExpander::visitInstruction(Instruction& I) {
    LLVM_DEBUG(dbgs() << "EXPAND: " << I << '\n');
    IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN INSTRUCTION IS BEING EXPANDED!");
    return false;
}

/// Terminator instructions
///

bool InstExpander::visitTerminatorInst(IGCLLVM::TerminatorInst& I) {
    // All terminators are handled specially.
    return false;
}

/// Standard binary operators
///

// TODO:
// LLVM intrinsic is not good at large integer calculation and lacks of
// intrinsic like:
//
//  %fsum = call {i32, i1} @full.add(i32 %x, i32 %y, i1 %cin);
//  %sum = extractvalue {i32, i1} %fsum, 0
//  %cout = extractvalue {i32, i1} %fsum, 1
//
// we have to use the following sequence to emulate it:
//
//  %cin32 = zext i1 %cin to i32
//  %psum = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %x, i32 %y)
//  %tsum = extractvalue {i32, i1} %psum, 0
//  %cout1 = extractvalue {i32, i1} %psum, 1
//  %fsum = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %tsum, i32 %cin32)
//  %sum = extractvalue {i32, i1} %fsum, 0
//  %cout2 = extractvalue {i32, i1} %fsum, 1
//  %cout = or i1 %cout1, %cout2
//
bool InstExpander::visitAdd(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitSub(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitMul(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitUDiv(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitSDiv(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitURem(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitSRem(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitShl(BinaryOperator& I) {
    ValueSeq* Ops0;
    TypeSeq* TySeq;
    std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
    std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(I.getType());
    IGC_ASSERT(TySeq->size() == Ops0->size());

    Type* MajorTy = TySeq->front();
    IntegerType* MajorITy = cast<IntegerType>(MajorTy);
    IntegerType* ITy = cast<IntegerType>(I.getType());
    IGC_ASSERT_MESSAGE(ITy->getBitWidth() < MajorITy->getBitMask(), "YOU HAVE A HUGE INTEGER TO BE SHIFTED! Shift amount cannot be encoded in legal integer types!");

    // Arbitrary integer left-shift is implemented as follows:
    //
    // As left-shift with amount greater than 'width(L)' is always 0, shift
    // amount, 'ShAmt', is canonicalized into
    //
    //  ShAmt := umin(ShAmt, width(L));
    //
    // , where 'width(L)' is defined as the bit width of 'L'.
    //
    // Arbitrary large integer 'L' is expanded into several parts, namely
    // P_0, P_1, ..., P_n, i.e.
    //
    //  L := [P_0, P_1, ..., P_n];
    //
    // and 'P_0' is the rightmost part and 'P_n' is the leftmost part.
    //
    // All P_i have the same type except the tailing one, P_n, which needs
    // extending to have the same type. For simplicity, we assume P_n is already
    // extended.
    //
    // Define 'width(P)' as the bit width of each P_i. If 'ShAmt' is greater
    // than 'width(P)', we need to shift parts left first; otherwise, we need to
    // combine bits from adjacent parts, i.e.
    //
    //  while (ShAmt >= width(P)) {
    //    ShAmt -= width(P);
    //    L := [0, P_0, ..., P_{n-1}];
    //  }
    //
    //  RshAmt := width(P) - shAmt;
    //  L:= [                    | (P_0 << ShAmt),
    //       (P_0     >> RshAmt) | (P_1 << ShAmt),
    //       ...
    //       (P_{n-1) >> RshAmt) | (P_n << ShAmt)]
    //
    // That 'while' loop is unrolled as the maximal iteration is known at
    // compilation time because 'ceil(umin(ShAmt, width(L))/width(P))' is always
    // smaller or equal to 'ceil(width(L)/width(P))'.

    unsigned OrigBW = ITy->getBitWidth();
    unsigned MajorBW = MajorITy->getBitWidth();
    unsigned MaxIters = (OrigBW + MajorBW - 1) / MajorBW;

    Value* MajorBitWidth = TL->getIntN(MajorBW, MajorBW);
    Value* Zero = TL->getIntN(MajorBW, 0);
    Value* ShAmt =
        TL->umin(I.getOperand(1), TL->getIntN(OrigBW, OrigBW), MajorTy);

    // Extend all parts into major type.
    SmallVector<Value*, 8> Parts(Ops0->begin(), Ops0->end());
    Value* TailingPart = Parts.pop_back_val();
    Parts.push_back(
        IRB->CreateZExt(TailingPart, MajorTy,
            Twine(TailingPart->getName(), ".zext")));
    Type* MinorTy = TailingPart->getType();

    for (unsigned i = 0; i != MaxIters; ++i) {
        Value* Cond =
            IRB->CreateICmpUGE(ShAmt, MajorBitWidth,
                Twine(ShAmt->getName(), ".cond"));

        // Skip when the shift amount is known to be smaller than the number of
        // bits of the major type.
        if (Cond == IRB->getFalse())
            break;

        Value* NewShAmt =
            IRB->CreateSub(ShAmt, MajorBitWidth,
                Twine(ShAmt->getName(), ".shamt"));

        // Shift in bits of major type if the shift amount is known to be
        // greater than or equal to the number of bits of the major type.
        if (Cond == IRB->getTrue()) {
            ShAmt = NewShAmt;
            Parts.pop_back();
            Parts.insert(Parts.begin(), Zero);
            continue;
        }

        // Otherwise, create 'select' to shift them in bits of major type.
        ShAmt =
            IRB->CreateSelect(Cond, NewShAmt, ShAmt,
                Twine(ShAmt->getName(), ".shamt.sel"));
        SmallVector<Value*, 8> NewParts;
        for (unsigned j = 0; j != Parts.size(); ++j)
            NewParts.push_back(
                IRB->CreateSelect(
                    Cond, (j == 0 ? Zero : Parts[j - 1]), Parts[j],
                    Twine(Parts[j]->getName(), ".sel")));
        std::swap(Parts, NewParts);
    }

    // Now, shift amount is smaller than the bits of major type.
    Value* RshAmt =
        IRB->CreateSub(MajorBitWidth, ShAmt,
            Twine(ShAmt->getName(), ".rshAmt"));
    Expanded.push_back(
        IRB->CreateShl(Parts[0], ShAmt,
            Twine(I.getName(), getSuffix()) + Twine(0)));
    for (unsigned i = 1, e = Parts.size(); i != e; ++i) {
        Value* P0 = Parts[i - 1];
        Value* P1 = Parts[i];
        Value* L = IRB->CreateLShr(P0, RshAmt, Twine(P0->getName(), ".rshl"));
        Value* H = IRB->CreateShl(P1, ShAmt, Twine(P1->getName(), ".shl"));
        Expanded.push_back(
            IRB->CreateOr(L, H, Twine(I.getName(), getSuffix()) + Twine(i)));
    }

    TailingPart = Expanded.back();
    Expanded.pop_back();

    TailingPart =
        IRB->CreateTrunc(TailingPart, MinorTy,
            Twine(TailingPart->getName(), ".trunc"));
    Expanded.push_back(TailingPart);

    return true;
}

bool InstExpander::visitLShr(BinaryOperator& I) {
    ValueSeq* Ops0;
    TypeSeq* TySeq;
    std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
    std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(I.getType());
    IGC_ASSERT(TySeq->size() == Ops0->size());

    Type* MajorTy = TySeq->front();
    IntegerType* MajorITy = cast<IntegerType>(MajorTy);
    IntegerType* ITy = cast<IntegerType>(I.getType());
    IGC_ASSERT_MESSAGE(ITy->getBitWidth() < MajorITy->getBitMask(), "YOU HAVE A HUGE INTEGER TO BE SHIFTED! Shift amount cannot be encoded in legal integer types!");

    // Arbitrary integer logic-right-shift is implemented similar to
    // arbitrary integer left-shift.

    unsigned OrigBW = ITy->getBitWidth();
    unsigned MajorBW = MajorITy->getBitWidth();
    unsigned MaxIters = (OrigBW + MajorBW - 1) / MajorBW;

    Value* MajorBitWidth = TL->getIntN(MajorBW, MajorBW);
    Value* Zero = TL->getIntN(MajorBW, 0);
    Value* ShAmt =
        TL->umin(I.getOperand(1), TL->getIntN(OrigBW, OrigBW), MajorTy);

    // Extend all parts into major type.
    SmallVector<Value*, 8> Parts(Ops0->begin(), Ops0->end());
    Value* TailingPart = Parts.pop_back_val();
    Parts.push_back(
        IRB->CreateZExt(TailingPart, MajorTy,
            Twine(TailingPart->getName(), ".zext")));
    Type* MinorTy = TailingPart->getType();

    for (unsigned i = 0; i != MaxIters; ++i) {
        Value* Cond =
            IRB->CreateICmpUGE(ShAmt, MajorBitWidth,
                Twine(ShAmt->getName(), ".cond"));

        // Skip when the shift amount is known to be smaller than the number of
        // bits of the major type.
        if (Cond == IRB->getFalse())
            break;

        Value* NewShAmt =
            IRB->CreateSub(ShAmt, MajorBitWidth,
                Twine(ShAmt->getName(), ".shamt"));

        // Shift in bits of major type if the shift amount is known to be
        // greater than or equal to the number of bits of the major type.
        if (Cond == IRB->getTrue()) {
            ShAmt = NewShAmt;
            Parts.erase(Parts.begin());
            Parts.push_back(Zero);
            continue;
        }

        // Otherwise, create 'select' to shift them in bits of major type.
        ShAmt =
            IRB->CreateSelect(Cond, NewShAmt, ShAmt,
                Twine(ShAmt->getName(), ".shamt.sel"));
        SmallVector<Value*, 8> NewParts;
        for (unsigned j = 0, e = Parts.size(); j != e; ++j)
            NewParts.push_back(
                IRB->CreateSelect(
                    Cond, ((j == e - 1) ? Zero : Parts[j + 1]), Parts[i],
                    Twine(Parts[j]->getName(), ".sel")));
        std::swap(Parts, NewParts);
    }

    // Now, shift amount is smaller than the bits of major type.
    Value* LshAmt =
        IRB->CreateSub(MajorBitWidth, ShAmt,
            Twine(ShAmt->getName(), ".lshAmt"));
    for (unsigned i = 0, e = Parts.size() - 1; i != e; ++i) {
        Value* P0 = Parts[i];
        Value* P1 = Parts[i + 1];
        Value* L = IRB->CreateLShr(P0, ShAmt, Twine(P0->getName(), ".lshr"));
        Value* H = IRB->CreateShl(P1, LshAmt, Twine(P1->getName(), ".shl"));
        Expanded.push_back(
            IRB->CreateOr(L, H, Twine(I.getName(), getSuffix()) + Twine(i)));
    }

    TailingPart = Parts.back();
    TailingPart =
        IRB->CreateLShr(TailingPart, ShAmt,
            Twine(TailingPart->getName(), ".lshr"));
    Expanded.push_back(
        IRB->CreateTrunc(TailingPart, MinorTy,
            Twine(I.getName(), getSuffix()) + Twine(Parts.size() - 1)));

    return true;
}

bool InstExpander::visitAShr(BinaryOperator& I) {
    // FIXME:
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool InstExpander::visitBinaryOperator(BinaryOperator& I) {
    ValueSeq* Ops0, * Ops1;
    std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
    // we should get copy of Ops0 here, as next getLegalizedValues call may grow ValueMap object
    // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
    // and previously received ValueSeq objects will become invalid.
    ValueSeq Ops0Copy(*Ops0);

    std::tie(Ops1, std::ignore) = TL->getLegalizedValues(I.getOperand(1));
    IGC_ASSERT(Ops0Copy.size() == Ops1->size());

    switch (I.getOpcode()) {
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor: {
        StringRef Name = I.getName();
        for (unsigned i = 0, e = Ops0Copy.size(); i != e; ++i) {
            Value* LHS = (Ops0Copy)[i];
            Value* RHS = (*Ops1)[i];
            Expanded.push_back(
                TL->createBinOpAsGiven(&I, LHS, RHS,
                    Twine(Name, getSuffix()) + Twine(i)));
        }
        break;
    }
    default:
        IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN BINARY OPERATOR IS BEING EXPANDED!");
    }
    return true;
}

/// Memory operators
///

bool InstExpander::visitLoadInst(LoadInst& I) {
    Type* OrigTy = I.getType();

    TypeSeq* TySeq;
    std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(OrigTy);

    unsigned AS = I.getPointerAddressSpace();
    // After expanding, illegal types are represented in a sequence of types
    // of the major type except the last one, which may be a different type.
    Type* MajorTy = TySeq->front();
    Type* MajorPtrTy = PointerType::get(MajorTy, AS);

    Value* OldPtr = I.getPointerOperand();
    Value* NewBasePtr =
        IRB->CreatePointerCast(OldPtr, MajorPtrTy,
            Twine(OldPtr->getName(), getSuffix()) + Twine(0));

    unsigned Part = 0;
    unsigned Off = 0;
    for (auto* Ty : *TySeq) {
        Value* NewPtr =
            TL->getPointerToElt(NewBasePtr, Part, PointerType::get(Ty, AS),
                Twine(OldPtr->getName(), getSuffix()) + Twine(Part));
        LoadInst* NewLd =
            IRB->CreateLoad(NewPtr, Twine(I.getName(), getSuffix()) + Twine(Part));
        TL->dupMemoryAttribute(NewLd, &I, Off);

        Expanded.push_back(NewLd);
        Off += TL->getTypeStoreSize(Ty);
        ++Part;
    }

    return true;
}

bool InstExpander::visitStoreInst(StoreInst& I) {
    Value* OrigVal = I.getValueOperand();
    Type* OrigTy = OrigVal->getType();

    ValueSeq* ValSeq;
    TypeSeq* TySeq;
    std::tie(ValSeq, std::ignore) = TL->getLegalizedValues(OrigVal);
    std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(OrigTy);
    IGC_ASSERT(ValSeq->size() == TySeq->size());

    unsigned AS = I.getPointerAddressSpace();

    // After expanding, illegal types are represented in a sequence of types
    // of the major type except the last one, which may be a different type.
    Type* MajorTy = TySeq->front();
    Type* MajorPtrTy = PointerType::get(MajorTy, AS);

    Value* OldPtr = I.getPointerOperand();
    Value* NewBasePtr =
        IRB->CreatePointerCast(OldPtr, MajorPtrTy,
            Twine(OldPtr->getName(), getSuffix()) + Twine(0));

    unsigned Part = 0;
    unsigned Off = 0;
    for (auto* Ty : *TySeq) {
        Value* NewPtr =
            TL->getPointerToElt(NewBasePtr, Part, PointerType::get(Ty, AS),
                Twine(OldPtr->getName(), getSuffix()) + Twine(Part));
        StoreInst* NewSt = IRB->CreateStore((*ValSeq)[Part], NewPtr);
        TL->dupMemoryAttribute(NewSt, &I, Off);

        Off += TL->getTypeStoreSize(Ty);
        ++Part;
    }

    return true;
}

/// Cast operators
///

bool InstExpander::visitTruncInst(TruncInst& I) {
    ValueSeq* ValSeq;
    std::tie(ValSeq, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
    IGC_ASSERT(ValSeq->size() > 1);

    TypeSeq* TySeq; LegalizeAction Act;
    std::tie(TySeq, Act) = TL->getLegalizedTypes(I.getDestTy());

    if (Act == Legal) {
        Value* Val = ValSeq->front();

        if (Val->getType() == I.getType())
            I.replaceAllUsesWith(Val);
        else
            I.setOperand(0, Val);

        return true;
    }

    IGC_ASSERT(TySeq->size() <= ValSeq->size());

    unsigned Part = 0;
    for (auto* Ty : *TySeq) {
        Value* Val = (*ValSeq)[Part];
        IGC_ASSERT(isa<IntegerType>(Ty));
        IGC_ASSERT(isa<IntegerType>(Val->getType()));
        IGC_ASSERT(cast<IntegerType>(Val->getType())->getBitWidth() >= cast<IntegerType>(Ty)->getBitWidth());

        Expanded.push_back(
            IRB->CreateTrunc(Val, Ty,
                Twine(I.getName(), getSuffix()) + Twine(Part)));
        ++Part;
    }

    return true;
}

bool InstExpander::visitZExtInst(ZExtInst& I) {
    ValueSeq* ValSeq; LegalizeAction Act;
    std::tie(ValSeq, Act) = TL->getLegalizedValues(I.getOperand(0));

    ValueSeq LegalVal;
    switch (Act) {
    case Legal:
        LegalVal.push_back(I.getOperand(0));
        ValSeq = &LegalVal;
        break;
    case Promote:
        LegalVal.push_back(TL->zext(ValSeq->front(), I.getSrcTy()));
        ValSeq = &LegalVal;
        break;
    default:
        break;
    }

    TypeSeq* TySeq;
    std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(I.getDestTy());

    IGC_ASSERT(TySeq->size() >= ValSeq->size());

    unsigned Part = 0;
    for (auto* Ty : *TySeq) {
        Value* Val =
            Part < ValSeq->size() ? (*ValSeq)[Part] : Constant::getNullValue(Ty);
        IGC_ASSERT(isa<IntegerType>(Ty));
        IGC_ASSERT(isa<IntegerType>(Val->getType()));
        IGC_ASSERT(cast<IntegerType>(Val->getType())->getBitWidth() <= cast<IntegerType>(Ty)->getBitWidth());

        if (Ty != Val->getType())
            Expanded.push_back(IRB->CreateZExt(Val, Ty, Twine(Val->getName(), ".zext")));
        else
            Expanded.push_back(Val);
        ++Part;
    }

    return true;
}

bool InstExpander::visitBitCastInst(BitCastInst& I) {
    ValueSeq* ValSeq; LegalizeAction ValAct;
    std::tie(ValSeq, ValAct) = TL->getLegalizedValues(I.getOperand(0));

    ValueSeq LegalVal;
    if (ValAct == Legal) {
        LegalVal.push_back(I.getOperand(0));
        ValSeq = &LegalVal;
    }

    TypeSeq* TySeq; LegalizeAction Act;
    std::tie(TySeq, Act) = TL->getLegalizedTypes(I.getDestTy());

    TypeSeq LegalTy;
    if (Act == Legal) {
        LegalTy.push_back(I.getDestTy());
        TySeq = &LegalTy;
    }

    ValueSeq Repacked;
    TL->repack(&Repacked, *TySeq, *ValSeq, I.getName() + getSuffix());

    if (Act == Legal) {
        IGC_ASSERT(Repacked.size() == 1);

        I.replaceAllUsesWith(Repacked.front());
        return true;
    }

    std::swap(Expanded, Repacked);

    return true;
}
