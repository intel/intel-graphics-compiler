/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "type-legalizer"
#include "TypeLegalizerPass.h"
#include "TypeLegalizer.h"
#include "InstLegalChecker.h"
#include "InstPromoter.h"
/*
#include "InstExpander.h"
#include "InstSoftener.h"
#include "InstScalarizer.h"
#include "InstElementizer.h"
*/
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/Transforms/Utils/Local.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Legalizer;

char TypeLegalizer::ID = 0;

TypeLegalizer::TypeLegalizer() : FunctionPass(ID),
                                 DL(nullptr),
                                 ILC(nullptr),
                                 IPromoter(nullptr),
                                 /*
                                 IExpander(nullptr),
                                 ISoftener(nullptr),
                                 IScalarizer(nullptr),
                                 IElementizer(nullptr),
                                 */
                                 TheModule(nullptr),
                                 TheFunction(nullptr) {
    initializeTypeLegalizerPass(*PassRegistry::getPassRegistry());
}

void TypeLegalizer::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
}


FunctionPass* createTypeLegalizerPass() { return new TypeLegalizer(); }

#define PASS_FLAG     "igc-type-legalizer"
#define PASS_DESC     "IGC Type Legalizer"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TypeLegalizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(DataLayout);
IGC_INITIALIZE_PASS_END(TypeLegalizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

// IGC supports type i64 natively or by emulation
const unsigned int MAX_LEGAL_INT_SIZE_IN_BITS = 64;

bool TypeLegalizer::runOnFunction(Function & F) {
    DL = &F.getParent()->getDataLayout();
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    IGC_ASSERT_MESSAGE(DL->isLittleEndian(), "ONLY SUPPORT LITTLE ENDIANNESS!");

    BuilderType TheBuilder(F.getContext(), TargetFolder(*DL));
    IRB = &TheBuilder;

    InstLegalChecker TheLegalChecker(this);
    InstPromoter ThePromoter(this, &TheBuilder);
    /*
    InstExpander TheExpander(this, &TheBuilder);
    InstSoftener TheSoftener(this, &TheBuilder);
    InstScalarizer TheScalarizer(this, &TheBuilder);
    InstElementizer TheElementizer(this, &TheBuilder);
    */

    ILC = &TheLegalChecker;
    IPromoter = &ThePromoter;
    /*
    IExpander = &TheExpander;
    ISoftener = &TheSoftener;
    IScalarizer = &TheScalarizer;
    IElementizer = &TheElementizer;
    */

    TheModule = F.getParent();
    TheFunction = &F;

    TypeMap.clear();
    ValueMap.clear();

    bool Changed = false;

    // Changed |= legalizeArguments(F);
    Changed |= preparePHIs(F);
    Changed |= legalizeInsts(F);
    Changed |= populatePHIs(F);
    // Changed |= legalizeTerminators(F);

    eraseIllegalInsts();

    DL = nullptr;
    DT = nullptr;

    IRB = nullptr;

    ILC = nullptr;
    IPromoter = nullptr;
    /*
    IExpander = nullptr;
    ISoftener = nullptr;
    IScalarizer = nullptr;
    IElementizer = nullptr;
    */
    TheModule = nullptr;
    TheFunction = nullptr;

    return Changed;
}

LegalizeAction
TypeLegalizer::getTypeLegalizeAction(Type* Ty) const {
    // Pointer type is always legal.
    if (Ty->isPointerTy())
        return Legal;

    // Array and Struct are currently skipped, but expected to be treated in Elementize
    if (Ty->isArrayTy() || Ty->isStructTy())
        return Legal;

    // Vector type!
    if (Ty->isVectorTy())
        return getTypeLegalizeAction(cast<VectorType>(Ty)->getElementType());

    // Floating point types always need native support; otherwise, they needs
    // software emulation.
    if (Ty->isFloatingPointTy()) {
        // Only support 'half', 'float', 'bfloat' and 'double'.
        if (Ty->isHalfTy()) return Legal;
        if (Ty->isFloatTy()) return Legal;
        if (Ty->isDoubleTy()) return Legal;
        if (IGCLLVM::isBFloatTy(Ty)) return Legal;

        return SoftenFloat;
    }

    IntegerType* ITy = dyn_cast<IntegerType>(Ty);
    IGC_ASSERT_EXIT_MESSAGE(nullptr != ITy, "DON'T KNOW HOW TO LEGALIZE TYPE!");

    unsigned Width = getTypeSizeInBits(ITy);

    if (Width == 1)
        return Legal;

    if (isLegalInteger(Width))
        return Legal;

    if (DL->fitsInLegalInteger(Width) || Width <= MAX_LEGAL_INT_SIZE_IN_BITS)
        return Promote;

    // Expand action is not supported
    return Legal;
}

LegalizeAction
TypeLegalizer::getLegalizeAction(Value* V) const {
    if (Instruction * I = dyn_cast<Instruction>(V))
        return ILC->check(I);
    return getTypeLegalizeAction(V->getType());
}

LegalizeAction
TypeLegalizer::getLegalizeAction(Instruction* I) const {
    return ILC->check(I);
}

std::pair<TypeSeq*, LegalizeAction>
TypeLegalizer::getLegalizedTypes(Type* Ty, bool legalizeToScalar) {
    LegalizeAction Act = getTypeLegalizeAction(Ty);
    TypeSeq* TySeq = nullptr;

    switch (Act) {
    case Legal:
        break;
    case Promote:
        TySeq = getPromotedTypeSeq(Ty, legalizeToScalar);
        break;
    case SoftenFloat:
        TySeq = getSoftenedTypeSeq(Ty);
        break;
    /*
    case Expand:
        TySeq = getExpandedTypeSeq(Ty);
        break;
    case Scalarize:
        TySeq = getScalarizedTypeSeq(Ty);
        break;
    case Elementize:
        TySeq = getElementizedTypeSeq(Ty);
        break;
    */
    }

    return std::make_pair(TySeq, Act);
}

TypeSeq* TypeLegalizer::getPromotedTypeSeq(Type* Ty, bool legalizeToScalar) {
    IGC_ASSERT(Ty->isIntOrIntVectorTy());
    IGC_ASSERT(getTypeLegalizeAction(Ty) == Promote);

    auto [TMI, New] = TypeMap.insert(std::make_pair(Ty, TypeSeq()));
    if (!New) {
        IGC_ASSERT(TMI->second.size() == 1);
        return &TMI->second;
    }

    Type* PromotedTy =
        DL->getSmallestLegalIntType(Ty->getContext(),
            getTypeSizeInBits(legalizeToScalar ? Ty : Ty->getScalarType()));

    if (!PromotedTy && getTypeSizeInBits(Ty) < MAX_LEGAL_INT_SIZE_IN_BITS)
    {
        PromotedTy = Type::getIntNTy(Ty->getContext(), MAX_LEGAL_INT_SIZE_IN_BITS);
    }

    if (!legalizeToScalar && Ty->isVectorTy())
    {
        unsigned Elems = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
        PromotedTy = IGCLLVM::FixedVectorType::get(PromotedTy, Elems);
    }

    TMI->second.push_back(PromotedTy);

    return &TMI->second;
}

TypeSeq* TypeLegalizer::getSoftenedTypeSeq(Type* Ty) {
    IGC_ASSERT(Ty->isFloatingPointTy());
    IGC_ASSERT(getTypeLegalizeAction(Ty) == SoftenFloat);

    auto [TMI, New] = TypeMap.insert(std::make_pair(Ty, TypeSeq()));
    if (!New) {
        IGC_ASSERT(TMI->second.size() == 1);
        return &TMI->second;
    }

    unsigned Width = 0;
    switch (Ty->getTypeID()) {
    case Type::HalfTyID: Width = 16; break;
    case Type::FloatTyID: Width = 32; break;
    case Type::DoubleTyID: Width = 64; break;
    case Type::X86_FP80TyID: Width = 80; break;
    case Type::FP128TyID: Width = 128; break;
    case Type::PPC_FP128TyID: Width = 128; break;
    default: IGC_ASSERT_EXIT_MESSAGE(0, "INVALID FLOATING POINT TYPE!");
    }

    Type* SoftenedTy = Type::getIntNTy(Ty->getContext(), Width);
    TMI->second.push_back(SoftenedTy);

    return &TMI->second;
}

/*
TypeSeq* TypeLegalizer::getExpandedTypeSeq(Type* Ty) {
    IGC_ASSERT(Ty->isIntegerTy());
    IGC_ASSERT(getTypeLegalizeAction(Ty) == Expand);

    auto [TMI, New] = TypeMap.insert(std::make_pair(Ty, TypeSeq()));
    if (!New)
        return &TMI->second;

    // FIXME: Need backport of DL->getLargestLegalIntTypeSize() instead of hard
    // coding.
    unsigned MaxLegalWidth = 64;
    unsigned Width = getTypeSizeInBits(Ty);
    IGC_ASSERT(MaxLegalWidth < Width);

    for (; MaxLegalWidth < Width; Width -= MaxLegalWidth)
        TMI->second.push_back(Type::getIntNTy(Ty->getContext(), MaxLegalWidth));
    TMI->second.push_back(Type::getIntNTy(Ty->getContext(), Width));

    return &TMI->second;
}
*/

/*
TypeSeq* TypeLegalizer::getScalarizedTypeSeq(Type* Ty) {
    IGC_ASSERT(Ty->isVectorTy());
    IGC_ASSERT(getTypeLegalizeAction(Ty) == Scalarize);

    auto [TMI, New] = TypeMap.insert(std::make_pair(Ty, TypeSeq()));
    if (!New) {
      IGC_ASSERT(TMI->second.size() ==
                 cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements());
      return &TMI->second;
    }

    Type* EltTy = cast<VectorType>(Ty)->getElementType();
    for (unsigned i = 0, e = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements(); i != e; ++i)
        TMI->second.push_back(EltTy);

    return &TMI->second;
}
*/

/*
TypeSeq* TypeLegalizer::getElementizedTypeSeq(Type* Ty) {
    IGC_ASSERT(Ty->isAggregateType());
    IGC_ASSERT(getTypeLegalizeAction(Ty) == Elementize);

    auto [TMI, New] = TypeMap.insert(std::make_pair(Ty, TypeSeq()));
    if (!New)
        return &TMI->second;

    if (Ty->isStructTy()) {
        StructType* STy = cast<StructType>(Ty);
        for (auto EI = STy->element_begin(),
            EE = STy->element_end(); EI != EE; ++EI)
            TMI->second.push_back(*EI);
    }
    else {
        ArrayType* ATy = cast<ArrayType>(Ty);
        Type* EltTy = ATy->getElementType();
        unsigned N = int_cast<unsigned int>(ATy->getNumElements());
        for (unsigned i = 0; i != N; ++i)
            TMI->second.push_back(EltTy);
    }

    return &TMI->second;
}
*/

std::pair<ValueSeq*, LegalizeAction>
TypeLegalizer::getLegalizedValues(Value* V, bool isSigned) {
    LegalizeAction Act = getTypeLegalizeAction(V->getType());

    if (Act == Legal)
        return std::make_pair(nullptr, Legal);

    auto C = dyn_cast<Constant>(V);
    if (C != nullptr)
        ValueMap.erase(V);

    auto [VMI, New] = ValueMap.insert(std::make_pair(V, ValueSeq()));
    if (!New)
        return std::make_pair(&VMI->second, Act);

    if (!C)
        return std::make_pair(nullptr, Act);

    // We visit instructions in topological order and we handle phis and
    // arguments specially, so we shouldn't see a use before a def here except
    // constants.
    TypeSeq* TySeq = nullptr;
    std::tie(TySeq, Act) = getLegalizedTypes(C->getType());

    switch (Act) {
    case Legal:
        IGC_ASSERT_MESSAGE(0, "LEGAL CONSTANT IS BEING LEGALIZED!");
        break;
    case Promote:
        promoteConstant(&VMI->second, TySeq, C, isSigned);
        break;
    case SoftenFloat:
        softenConstant(&VMI->second, TySeq, C);
        break;
    /*
    case Expand:
        expandConstant(&VMI->second, TySeq, C);
        break;
    case Scalarize:
        scalarizeConstant(&VMI->second, TySeq, C);
        break;
    case Elementize:
        elementizeConstant(&VMI->second, TySeq, C);
        break;
    */
    default:
        IGC_ASSERT(0);
        break;
    }

    return std::make_pair(&VMI->second, Act);
}

void
TypeLegalizer::setLegalizedValues(Value* OVal,
    ArrayRef<Value*> LegalizedVals) {
    auto [VMI, New] = ValueMap.insert(std::make_pair(OVal, ValueSeq()));
    IGC_ASSERT(New);

    for (auto* V : LegalizedVals)
        VMI->second.push_back(V);
}

bool
TypeLegalizer::hasLegalizedValues(Value* V) const {
    IGC_ASSERT(!V->getType()->isVoidTy());

    LegalizeAction Act = getTypeLegalizeAction(V->getType());

    if (Act == Legal)
        return true;

    return ValueMap.find(V) != ValueMap.end();
}

void TypeLegalizer::promoteConstant(ValueSeq* ValSeq, TypeSeq* TySeq,
    Constant* C, bool isSigned) {
    IGC_ASSERT(TySeq->size() == 1);

    Type* PromotedTy = TySeq->front();

    auto* ExtValue = isSigned ?
        ConstantExpr::getSExt(C, PromotedTy) :
        ConstantExpr::getZExt(C, PromotedTy);

    ValSeq->push_back(ExtValue);
}

void TypeLegalizer::softenConstant(ValueSeq* ValSeq, TypeSeq* TySeq,
    Constant* C) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");

}

/*
void TypeLegalizer::expandConstant(ValueSeq* ValSeq, TypeSeq* TySeq,
    Constant* C) {
    Type* Ty = C->getType();

    TypeSeq::iterator TI = TySeq->begin();
    TypeSeq::iterator TE = TySeq->end();

    Type* LoTy = *TI;
    ValSeq->push_back(ConstantExpr::getTrunc(C, LoTy));
    for (++TI; TI != TE; ++TI) {
        Constant* ShAmt =
            ConstantInt::get(Ty, DL->getTypeSizeInBits(LoTy), false);
        C = ConstantExpr::getLShr(C, ShAmt);
        LoTy = *TI;
        ValSeq->push_back(ConstantExpr::getTrunc(C, LoTy));
    }
}
*/

/*
void TypeLegalizer::scalarizeConstant(ValueSeq* ValSeq, TypeSeq* TySeq,
    Constant* C) {
    for (unsigned i = 0, e = TySeq->size(); i != e; ++i) {
        Constant* Idx = IRB->getInt32(i);
        ValSeq->push_back(ConstantExpr::getExtractElement(C, Idx));
    }
}
void TypeLegalizer::elementizeConstant(ValueSeq* ValSeq, TypeSeq* TySeq,
    Constant* C) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
}
Value* TypeLegalizer::promoteArgument(Argument* Arg, Type* PromotedTy) {
    IGC_ASSERT_EXIT_MESSAGE(0, "ILLEGAL ARGUMENT IS FOUND TO BE PROMOTED!");
    return nullptr;
}

Value* TypeLegalizer::expandArgument(Argument* Arg,
    Type* ExpandedTy, unsigned Part) {
    IGC_ASSERT_EXIT_MESSAGE(0, "ILLEGAL ARGUMENT IS FOUND TO BE EXPANDED!");
    return nullptr;
}

Value* TypeLegalizer::softenArgument(Argument* Arg, Type* SoftenedTy) {
    return
        IRB->CreateBitCast(Arg, SoftenedTy,
            Twine(Arg->getName(), getSuffix(SoftenFloat)));
}

Value* TypeLegalizer::scalarizeArgument(Argument* Arg,
    Type* ScalarizedTy, unsigned Part) {
    Value* RetVal =
        IRB->CreateExtractElement(Arg, IRB->getInt32(Part),
            Twine(Arg->getName(), getSuffix(Scalarize)) + Twine(Part));
    IGC_ASSERT(RetVal->getType() == ScalarizedTy);
    return RetVal;
}

Value* TypeLegalizer::elementizeArgument(Argument* Arg,
    Type* ElementizedTy, unsigned Part) {
    Value* RetVal =
        IRB->CreateExtractValue(Arg, Part,
            Twine(Arg->getName(), getSuffix(Elementize)) + Twine(Part));
    IGC_ASSERT(RetVal->getType() == ElementizedTy);
    return RetVal;
}

bool TypeLegalizer::legalizeArguments(Function& F) {
    bool Changed = false;
    Instruction* Pos = &F.getEntryBlock().front();
    IRB->SetInsertPoint(Pos);

    // Argument is legalized through special intrinsics
    // - llvm.val.promote.<PromotedTy>(<Ty> <Val>)
    // - llvm.val.expand.<ExpandedTy>(<Ty> <Val>, i32 <Part>)
    // - llvm.val.demote.<OriginalTy>(<PromotedTy> <PromotedVal>)
    // - llvm.val.trim.<OriginalTy>(...)
    for (auto AI = F.arg_begin(), AE = F.arg_end(); AI != AE; ++AI) {
        Argument* Arg = &(*AI);
        Type* Ty = Arg->getType();
        auto [TySeq, Act] = getLegalizedTypes(Ty);

        switch (Act) {
        case Legal:
            continue;
        case Promote: {
            IGC_ASSERT(TySeq->size() == 1);
            Type* PromotedTy = TySeq->front();
            setLegalizedValues(Arg, promoteArgument(Arg, PromotedTy));
            break;
        }
        case Expand: {
            SmallVector<Value*, 8> Expanded;
            unsigned Part = 0;
            for (auto* Ty : *TySeq)
                Expanded.push_back(expandArgument(Arg, Ty, Part++));
            setLegalizedValues(Arg, Expanded);
            break;
        }
        case SoftenFloat: {
            IGC_ASSERT(TySeq->size() == 1);
            Type* SoftenedTy = TySeq->front();
            setLegalizedValues(Arg, softenArgument(Arg, SoftenedTy));
            break;
        }
        case Scalarize: {
            SmallVector<Value*, 8> Scalarized;
            unsigned Part = 0;
            for (auto* Ty : *TySeq)
                Scalarized.push_back(scalarizeArgument(Arg, Ty, Part++));
            setLegalizedValues(Arg, Scalarized);
            break;
        }
        case Elementize: {
            SmallVector<Value*, 8> Elementized;
            unsigned Part = 0;
            for (auto* Ty : *TySeq)
                Elementized.push_back(scalarizeArgument(Arg, Ty, Part++));
            setLegalizedValues(Arg, Elementized);
            break;
        }
        }
        Changed = true;
    }

    return Changed;
}
*/

bool TypeLegalizer::preparePHIs(Function& F) {
    bool Changed = false;

    for (auto& BB : F) {
        for (BasicBlock::iterator BI = BB.begin(); ; ) {
            PHINode* PN = dyn_cast<PHINode>(BI++);
            if (!PN) break;

            Type* Ty = PN->getType();
            auto [TySeq, Act] = getLegalizedTypes(Ty);
            switch (Act) {
            case Legal:
                // Skip legal PHI node.
                continue;
            case Promote:
            case SoftenFloat: {
                IGC_ASSERT(TySeq->size() == 1);
                Type* PromotedTy = TySeq->front();
                StringRef Name = PN->getName();
                PHINode* Promoted =
                    PHINode::Create(PromotedTy, PN->getNumIncomingValues(),
                        Twine(Name, getSuffix(Act)), &(*BI));
                Promoted->setDebugLoc(PN->getDebugLoc());
                replaceAllDbgUsesWith(*PN, *Promoted, *PN, *DT);
                setLegalizedValues(PN, Promoted);
                BI = BasicBlock::iterator(Promoted);
                break;
            }
/*
            case Expand:
            case Scalarize:
            case Elementize: {
                StringRef Name = PN->getName();
                unsigned NumIncomingValues = PN->getNumIncomingValues();
                SmallVector<Value*, 8> Expanded;
                unsigned Part = 0;
                for (auto* Ty : *TySeq) {
                    PHINode* N =
                        PHINode::Create(Ty, NumIncomingValues,
                            Twine(Name, getSuffix(Act)) + Twine(Part++), &(*BI));
                    Expanded.push_back(N);
                }
                setLegalizedValues(PN, Expanded);
                BI = BasicBlock::iterator(cast<Instruction>(Expanded.front()));
                break;
            }
*/
            default: continue;
            }
            Changed = true;
        }
    }

    return Changed;
}

bool TypeLegalizer::populatePromotedPHI(PHINode* PN) {
    ValueSeq* ValSeq = nullptr;
    std::tie(ValSeq, std::ignore) = getLegalizedValues(PN);
    IGC_ASSERT(ValSeq->size() == 1);
    IGC_ASSERT(isa<PHINode>(ValSeq->front()));

    PHINode* Promoted = cast<PHINode>(ValSeq->front());
    IGC_ASSERT(Promoted->getNumIncomingValues() == 0);

    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
        std::tie(ValSeq, std::ignore) =
            getLegalizedValues(PN->getIncomingValue(i));
        IGC_ASSERT(ValSeq->size() == 1);

        Promoted->addIncoming(ValSeq->front(), PN->getIncomingBlock(i));
    }

    return true;
}
/*
bool TypeLegalizer::populateExpandedPHI(PHINode* PN) {
    ValueSeq* Expanded = nullptr;
    std::tie(Expanded, std::ignore) = getLegalizedValues(PN);
    // we should get copy of Expanded here, as next getLegalizedValues call may grow ValueMap object
    // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
    // and previously received ValueSeq objects will become invalid.
    ValueSeq ExpandedCopy(*Expanded);

    for (auto OI = PN->op_begin(), OE = PN->op_end(); OI != OE; ++OI) {
        Value* O = OI->get();
        ValueSeq* ValSeq = nullptr;
        std::tie(ValSeq, std::ignore) = getLegalizedValues(O);
        IGC_ASSERT(ValSeq->size() == ExpandedCopy.size());

        BasicBlock* BB = PN->getIncomingBlock(*OI);

        for (unsigned i = 0, e = ExpandedCopy.size(); i != e; ++i) {
            PHINode* ExpandedPN = cast<PHINode>((ExpandedCopy)[i]);
            IGC_ASSERT(ExpandedPN->getNumIncomingValues() < PN->getNumIncomingValues());
            ExpandedPN->addIncoming((*ValSeq)[i], BB);
        }
    }

    return true;
}
*/

/*
bool TypeLegalizer::populateSoftenedPHI(PHINode* PN) {
    return populatePromotedPHI(PN);
}

bool TypeLegalizer::populateScalarizedPHI(PHINode* PN) {
    return populateExpandedPHI(PN);
}

bool TypeLegalizer::populateElementizedPHI(PHINode* PN) {
    return populateExpandedPHI(PN);
}
*/

bool TypeLegalizer::populatePHIs(Function& F) {
    bool Changed = false;

    for (auto& BB : F) {
        for (BasicBlock::iterator BI = BB.begin(); ; ) {
            PHINode* PN = dyn_cast<PHINode>(BI++);
            if (!PN) break;

            LegalizeAction Act = getLegalizeAction(PN);

            switch (Act) {
            case Legal:
                continue;
            case Promote:
                Changed |= populatePromotedPHI(PN);
                break;
            /*
            case Expand:
                Changed |= populateExpandedPHI(PN);
                break;
            case SoftenFloat:
                Changed |= populateSoftenedPHI(PN);
                break;
            case Scalarize:
                Changed |= populateScalarizedPHI(PN);
                break;
            case Elementize:
                Changed |= populateElementizedPHI(PN);
                break;
            */
            default: continue;
            }
            IllegalInsts.insert(PN);
        }
    }

    return Changed;
}

bool TypeLegalizer::legalizeInsts(Function& F) {
    bool Changed = false;

    // Legalize instructions in a topological order so that the 'def' is always
    // legalized before the legalization of the 'use'.
    ReversePostOrderTraversal<Function*> RPOT(&F);
    for (auto& BB : RPOT) {
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
            Instruction* I = &(*BI);

            // Skip terminator insts which are handled specially.
            if (I->isTerminator())
                continue;

            // Skip PHI nodes which are populated later.
            if (isa<PHINode>(I))
                continue;

            switch (getLegalizeAction(I)) {
            case Legal:
                // Skip legal instructions.
                continue;
            case Promote:
                Changed |= IPromoter->promote(I);
                break;
            /*
            case Expand:
                Changed |= IExpander->expand(I);
                break;
            case SoftenFloat:
                Changed |= ISoftener->soften(I);
                break;
            case Scalarize:
                Changed |= IScalarizer->scalarize(I);
                break;
            case Elementize:
                Changed |= IElementizer->elementize(I);
                break;
            */
            default: continue;
            }

            if ((!hasLegalRetType(I) && !isReservedLegal(I)) || I->use_empty())
                IllegalInsts.insert(I);
        }
    }

    return Changed;
}

/*
bool TypeLegalizer::legalizeTerminators(Function& F) {
    bool Changed = false;

    for (auto& BB : F) {
        IGCLLVM::TerminatorInst* TI = BB.getTerminator();
        IGC_ASSERT(TI);

        ReturnInst* RI = dyn_cast<ReturnInst>(TI);
        if (!RI)
            continue;

        LegalizeAction Act = getLegalizeAction(RI);
        switch (Act) {
        case Legal:
            // Skip legal return.
            continue;
        case Promote:
            Changed |= promoteRet(RI);
            break;
        case Expand:
            Changed |= expandRet(RI);
            break;
        case SoftenFloat:
            Changed |= softenRet(RI);
            break;
        case Scalarize:
            Changed |= scalarizeRet(RI);
            break;
        case Elementize:
            Changed |= elementizeRet(RI);
            break;
        }
    }

    return Changed;
}
*/
/*
bool TypeLegalizer::promoteRet(ReturnInst* RI) {
    IGC_ASSERT_EXIT_MESSAGE(0, "ILLEGAL RETURN VALUE IS FOUND TO BE PROMOTED!");
    return false;
}

bool TypeLegalizer::expandRet(ReturnInst* RI) {
    IGC_ASSERT_EXIT_MESSAGE(0, "ILLEGAL RETURN VALUE IS FOUND TO BE EXPANDED!");
    return false;
}

bool TypeLegalizer::softenRet(ReturnInst* RI) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool TypeLegalizer::scalarizeRet(ReturnInst* RI) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}

bool TypeLegalizer::elementizeRet(ReturnInst* RI) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}
*/

void TypeLegalizer::eraseIllegalInsts() {
    for (auto* I : IllegalInsts) {
        Type* Ty = I->getType();

        if (!Ty->isVoidTy())
            I->replaceAllUsesWith(UndefValue::get(Ty));
        I->eraseFromParent();
    }
    IllegalInsts.clear();
}
