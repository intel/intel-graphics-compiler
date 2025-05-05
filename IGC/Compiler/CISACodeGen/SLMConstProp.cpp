/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/SLMConstProp.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Support/MathExtras.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include <vector>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGCMD;

namespace
{
    class SLMConstProp : public FunctionPass
    {
    public:
        static char ID; // Pass identification, replacement for typeid

        enum PatternType {
            PATTERN_UNDEFINED,

            // Given a constant stride and offset, this pattern
            // refers to the locations:
            //    offset + 0*stride
            //    offset + 1*stride
            //    offset + 2*stride
            //    ......
            //    offset + n*stride
            // and all the locations, if defined, have the same constant.
            PATTERN_REPEAT_WITH_STRIDE
        };

        struct PatternInfo {
            PatternType  Type;
            int          offset;
            int          stride;
            Constant* storedVal;
        };

        // Detailed info about a store
        struct StoreInfo {
            PatternType  ty;  // Pattern type if this store is part of
                              // a pattern.
            int  ix{};          // ix into m_storeInsts vector
            SymExpr* SE = nullptr;      // Store's address
            Constant* Val = nullptr;    // Store's value if it is constant.
            int offset{};       // offset relative to a particular SymExpr. Its meaning
                              // is based on the context in which it is used.

            StoreInfo() : ty(PATTERN_UNDEFINED) {}
        };

        SLMConstProp() : FunctionPass(ID)
        {
            initializeSLMConstPropPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.setPreservesCFG();
        }

    private:
        CodeGenContext* m_Ctx = nullptr;
        Function* m_F = nullptr;
        LoopInfo* m_LI = nullptr;
        DominatorTree* m_DT = nullptr;
        const DataLayout* m_DL = nullptr;
        SymbolicEvaluation m_SymEval;

        // All SLM loads
        SmallVector<LoadInst*, 16> m_loadInsts;

        // To keep detailed info about stores.
        std::vector<StoreInfo> m_storeInfos;

        // All SLM stores
        SmallVector<StoreInst*, 16> m_storeInsts;

        // Pattern info
        SmallVector<PatternInfo, 4> m_patternInfos;

        // Return true if constant stores can be handled
        bool analyzeConstantStores();

        // Return true if constant propagation is performed.
        bool performConstProp();

        bool find_PATTERN_REPEAT_WITH_STRIDE(PatternInfo& aPattern);
        bool perform_PATTERN_REPEAT_WITH_STRIDE(PatternInfo& aPattern);

        // Helper functions
        bool isEqual(Constant* C0, Constant* C1);
        bool isLinearFuncOfLocalIds(SymExpr* SE);
        bool isFloatType(Type* Ty);
        Constant* convertIfNeeded(Type* Ty, Constant* C);
    };
}  // namespace

#if defined (_DEBUG)

void SymbolicEvaluation::dump_symbols()
{
    IGC_ASSERT_MESSAGE((int)m_symInfos.size() <= m_nextValueID,
        "ValueInfoMap has the incorrect number of entries!");

    // for sorting symbols in increasing ID.
    std::vector<const Value*> Vals(m_nextValueID);
    for (auto II : m_symInfos) {
        const Value* V = II.first;
        ValueSymInfo* VSI = II.second;
        IGC_ASSERT(nullptr != VSI);
        IGC_ASSERT_MESSAGE(VSI->ID < m_nextValueID, "Incorrect value ID!");
        Vals[VSI->ID] = V;
    }

    dbgs() << "\nSymbols used\n\n";
    for (int i = 0; i < m_nextValueID; ++i) {
        const Value* V = Vals[i];
        // V should not be nullptr, but check it for safety.
        IGC_ASSERT(nullptr != V);
        if (V) {
            ValueSymInfo* VSI = getSymInfo(V);
            SymCastInfo SCI = (SymCastInfo)VSI->CastInfo;
            dbgs() << "  V" << i;
            switch (SCI) {
            case SYMCAST_ZEXT:
                dbgs() << " [zext] :    " << *V << "\n";
                break;
            case SYMCAST_SEXT:
                dbgs() << " [sext] :    " << *V << "\n";
                break;
            default:
                dbgs() << " :    " << *V << "\n";
            }
        }
    }
    dbgs() << "\n\n";
}

void SymbolicEvaluation::print(raw_ostream& OS, const SymProd* P)
{
    bool hasError = false;
    for (int i = 0; i < (int)P->Prod.size(); ++i) {
        const Value* V = P->Prod[i];
        if (ValueSymInfo * VSI = getSymInfo(V)) {
            OS << (i == 0 ? "V" : " * V")
                << VSI->ID;
        }
        else {
            OS << (i == 0 ? "Ve" : " * Ve");
            hasError = true;
        }
    }
    if (hasError) {
        OS << "  Error:  a used value is not in value info map!\n\n";
    }
}

void SymbolicEvaluation::print_varMapping(raw_ostream& OS, const SymProd* P)
{
    std::map<const Value*, int> varMap;
    const int psz = (int)P->Prod.size();
    for (int i = 0; i < psz; ++i) {
        const Value* V = P->Prod[i];
        if (ValueSymInfo* VSI = getSymInfo(V)) {
            varMap[V] = VSI->ID;
        }
    }

    if (!varMap.empty())
    {
        OS << "\n";
        for (auto MI : varMap)
        {
            const Value* V = MI.first;
            int VID = MI.second;
            OS << "    V" << VID << ": " << *V << "\n";
        }
        OS << "\n";
    }
}

void SymbolicEvaluation::print(raw_ostream& OS, const SymTerm* T)
{
    if (T->Term->Prod.size() > 0) {
        if (T->Coeff != 1) {
            OS << T->Coeff;
            OS << " * ";
        }
        print(OS, T->Term);
    }
    else {
        OS << T->Coeff;
    }
}

void SymbolicEvaluation::print_varMapping(raw_ostream& OS, const SymTerm* T)
{
    print_varMapping(OS, T->Term);
}

void SymbolicEvaluation::print(raw_ostream& OS, const SymExpr* SE)
{
    int e = (int)SE->SymTerms.size();
    for (int i = 0; i < e; ++i) {
        if (i > 0) {
            OS << " + ";
        }
        print(OS, SE->SymTerms[i]);
    }
    if (SE->ConstTerm != 0)
    {
        if (e > 0) {
            OS << " + ";
        }
        OS << SE->ConstTerm << "\n";
    }
}

void SymbolicEvaluation::print_varMapping(raw_ostream& OS, const SymExpr* SE)
{
    std::map<const Value*, int> varMap;
    const int e = (int)SE->SymTerms.size();
    for (int i = 0; i < e; ++i) {
        const SymProd* P = SE->SymTerms[i]->Term;
        const int psz = (int)P->Prod.size();
        for (int i = 0; i < psz; ++i) {
            const Value* V = P->Prod[i];
            if (ValueSymInfo* VSI = getSymInfo(V)) {
                varMap[V] = VSI->ID;
            }
        }
    }

    if (!varMap.empty())
    {
        OS << "\n";
        for (auto MI : varMap)
        {
            const Value* V = MI.first;
            int VID = MI.second;
            OS << "    V" << VID << ": " << *V << "\n";
        }
        OS << "\n";
    }
}

void SymbolicEvaluation::print(raw_ostream& OS, const Value* V)
{
    ValueSymInfo* VSI = getSymInfo(V);
    OS << "   Symbolic expression for\n";
    OS << "       " << *V << "\n\n";

    if (VSI)
    {
        print(OS, VSI->symExpr);
    }
    else
    {
        OS << "    No symbolic expression available\n\n";
    }
}

void SymbolicEvaluation::print_varMapping(raw_ostream& OS, const Value* V)
{
    ValueSymInfo* VSI = getSymInfo(V);
    if (VSI) {
        print_varMapping(OS, VSI->symExpr);
    }
}

void SymbolicEvaluation::dump(const SymProd* P)
{
    dbgs() << "\n";
    print(dbgs(), P);
    print_varMapping(dbgs(), P);
    dbgs() << "\n";
}

void SymbolicEvaluation::dump(SymProd* P)
{
    dump((const SymProd*)P);
}

void SymbolicEvaluation::dump(const SymTerm* T)
{
    dbgs() << "\n";
    print(dbgs(), T);
    print_varMapping(dbgs(), T);
    dbgs() << "\n";
}

void SymbolicEvaluation::dump(SymTerm* T)
{
    dump((const SymTerm*)T);
}


void SymbolicEvaluation::dump(const SymExpr* SE)
{
    dbgs() << "\n";
    print(dbgs(), SE);
    print_varMapping(dbgs(), SE);
    dbgs() << "\n";
}

void SymbolicEvaluation::dump(SymExpr* SE)
{
    dump((const SymExpr*)SE);
}

void SymbolicEvaluation::dump(const Value* V)
{
    dbgs() << "\n";
    print(dbgs(), V);
    print_varMapping(dbgs(), V);
    dbgs() << "\n";
}

void SymbolicEvaluation::dump(Value* V)
{
    dump((const Value*)V);
}

#endif

SymExpr* SymbolicEvaluation::getSymExpr(const Value* V)
{
    // Stop if non-Pointer type or non-integer type.
    // Since systemValue is prototyped as to return a float,
    // the float type is accepted here.
    Type* Ty = V->getType();
    if (!Ty->isPointerTy() && !Ty->isIntegerTy() &&
        !Ty->isFloatTy())
    {
        return nullptr;
    }

    if (ValueSymInfo * VSI = getSymInfo(V))
    {
        return VSI->symExpr;
    }

    int64_t coeff = 0;
    SymExpr* expr = nullptr;
    getSymExprOrConstant(V, expr, coeff);

    if (expr == nullptr)
    {
        // Create a SymExpr (it is a constant)
        SymExpr* E = new (m_symExprAllocator.Allocate()) SymExpr();
        E->ConstTerm = coeff;
        setSymInfo(V, E);

        expr = E;
    }
    return expr;
}

// Return : S if S != nullptr;
//          C if S == nullptr;
void SymbolicEvaluation::getSymExprOrConstant(const Value* V, SymExpr*& S, int64_t& C)
{
    S = nullptr;
    if (ValueSymInfo * VSI = getSymInfo(V))
    {
        S = VSI->symExpr;
        return;
    }

    bool isEvaluable = true;
    // Expect scalar type. Stop evaluation if not scalar type.
    if (isa<Constant>(V)) {
        const ConstantInt* CI = dyn_cast<const ConstantInt>(V);
        if (CI && V->getType()->isSingleValueType() && !V->getType()->isVectorTy())
        {
            C = CI->getSExtValue();
            return;
        }
        isEvaluable = false;
    }

    // Used for nomalizing shift amount.
    //   For example, i64, its type mask is 03F(63 = 64 - 1).
    auto getTypeMask = [](const Type* Ty) -> uint32_t {
        // For simplicity, only handle type whose size is power of 2.
        uint32_t nbits = Ty->getScalarSizeInBits();
        if (nbits > 0 && isPowerOf2_32(nbits))
            return (nbits - 1);
        return 0;
    };

    // Instructions/Operators handled for now:
    //   GEP
    //   bitcast (inttoptr, ptrtoint, etc)
    //   mul, add, sub.
    SymExpr* S0;
    SymExpr* S1;
    int64_t  C0, C1;

    auto BinOp = dyn_cast<OverflowingBinaryOperator>(V);
    const Operator* Op = dyn_cast<Operator>(V);
    const bool canOverflow = (considerOverflow() && BinOp &&
        !(BinOp->hasNoSignedWrap() || BinOp->hasNoUnsignedWrap()));
    // Skip if Op can overflow.
    //   Hope we don't need to handle cases if considerOverflow() is true;
    //   otherwise, the code needs more tuning.
    if (Op && isEvaluable && !canOverflow && !exceedMaxValues())
    {
        unsigned opc = Op->getOpcode();
        switch (opc) {
        case Instruction::GetElementPtr:
        {
            const GEPOperator* GEPOp = static_cast<const GEPOperator*>(Op);
            const Value* V0 = GEPOp->getPointerOperand();
            if (GEPOp->hasAllZeroIndices())
            {
                // V is the same as V0.
                getSymExprOrConstant(V0, S, C);
                if (S == nullptr)
                {
                    // V is constant, return the constant
                    return;
                }
            }
            else  if (GEPOp->getSourceElementType()->isSized() &&
                !V0->getType()->isVectorTy() &&
                m_DL != nullptr)
            {
                //
                // 1. Traverse all indexes to accumulate offset;
                // 2. Add the pointer operand at the last.
                //
                int64_t ByteOffset = 0;  // holding the constant part of this GEP
                S0 = nullptr;            // If not null, S0 is GEP's symbolic expression.
                for (gep_type_iterator GII = gep_type_begin(GEPOp), GIE = gep_type_end(GEPOp); GII != GIE; ++GII)
                {
                    Value* Idx = GII.getOperand();
                    ConstantInt* CIdx = dyn_cast<ConstantInt>(Idx);
                    if (StructType* STy = GII.getStructTypeOrNull())
                    {
                        // struct needs special handling. For struct, idx must be a constant.
                        IGC_ASSERT(CIdx != nullptr);
                        uint32_t EltIdx = (uint32_t)CIdx->getZExtValue();
                        const StructLayout* SL = m_DL->getStructLayout(STy);
                        ByteOffset += SL->getElementOffset(EltIdx);
                        continue;
                    }

                    int64_t eltByteSize = m_DL->getTypeAllocSize(GII.getIndexedType());
                    if (CIdx)
                    {
                        ByteOffset += (CIdx->getSExtValue() * eltByteSize);
                        continue;
                    }

                    getSymExprOrConstant(Idx, S1, C1);
                    if (!S1)
                    {
                        C1 = C1 * eltByteSize;
                        ByteOffset += C1;
                    }
                    else
                    {
                        SymExpr* T = mul(S1, eltByteSize);
                        S0 = S0 ? add(S0, T, false) : T;
                    }
                }

                // Get symExpr for pointer operand.
                getSymExprOrConstant(GEPOp->getPointerOperand(), S1, C1);
                if (S0 && S1)
                {
                    S = add(S0, S1, false);
                    if (ByteOffset != 0)
                    {
                        S = add(S, ByteOffset);
                    }
                }
                else if (S0 || S1)
                {
                    int64_t tC = S1 ? ByteOffset : (ByteOffset + C1);
                    S = S1 ? S1 : S0;
                    if (tC != 0)
                    {
                        S = add(S, tC);
                    }
                }
                else
                {
                    C = ByteOffset + C1;
                    return;
                }
            }
            break;
        }
        case Instruction::Sub:
        case Instruction::Add:
        {
            const Value* V0 = Op->getOperand(0);
            const Value* V1 = Op->getOperand(1);
            getSymExprOrConstant(V0, S0, C0);
            getSymExprOrConstant(V1, S1, C1);

            bool negateS1 = (opc == Instruction::Sub);
            if (!S0 && !S1)
            {
                if (negateS1)
                {
                    C = C0 - C1;
                }
                else
                {
                    C = C0 + C1;
                }
                return;
            }
            if (S0 && S1)
            {
                S = add(S0, S1, negateS1);

            }
            else if (S0)
            {
                S = add(S0, negateS1 ? -C1 : C1);
            }
            else
            {
                // S1 isn't null
                if (negateS1) {
                    S = mul(S1, -1);
                    S->ConstTerm += C0;
                }
                else {
                    S = add(S1, C0);
                }
            }
            break;
        }
        case Instruction::Or:
        {
            // Check if it is actually an add.
            //
            //   %mul = shl nuw nsw i64 %v, 1
            //   %add = or i64 %mul, 1
            //     -->  %add = add %mul, 1
            const Value* V0 = Op->getOperand(0);
            const Value* V1 = Op->getOperand(1);
            getSymExprOrConstant(V0, S0, C0);
            getSymExprOrConstant(V1, S1, C1);
            if (!S0 && !S1) {
                C = C0 | C1;
                return;
            }

            // Case: 'or V0  Const' or 'or const  V1'
            if ((S0 && !S1) || (!S0 && S1)) {
                const Value* tV = (S0 ? V0 : V1);
                const uint64_t tC = (uint64_t)(S0 ? C1 : C0);
                uint32_t bits = (uint32_t)tV->getType()->getPrimitiveSizeInBits();
                if (bits != 0 && MaskedValueIsZero(tV, APInt(bits, tC), *m_DL))
                    S = add(S0 ? S0 : S1, tC);
            }
            break;
        }
        case Instruction::Mul:
        {
            const Value* V0 = Op->getOperand(0);
            const Value* V1 = Op->getOperand(1);
            getSymExprOrConstant(V0, S0, C0);
            getSymExprOrConstant(V1, S1, C1);
            if (!S0 && !S1)
            {
                C = C0 * C1;
                return;
            }

            // Don't handle Value * Value for now.
            // Only handle Value * const
            if (!S0 || !S1)
            {
                // only one of S0 & S1 is not null.
                bool isS0Valid = (S0 != nullptr);
                S = mul(isS0Valid ? S0 : S1, (isS0Valid) ? C1 : C0);
            }

            break;
        }
        case Instruction::Shl:
        {
            // shl is a mul
            //     shl a,  b, 2
            //  -> mul a, b, (1 << 2)
            const Value* V0 = Op->getOperand(0);
            const Value* V1 = Op->getOperand(1);
            getSymExprOrConstant(V0, S0, C0);
            getSymExprOrConstant(V1, S1, C1);

            uint32_t shtAmtMask = getTypeMask(V->getType());
            if (shtAmtMask == 0) // sanity
                break;

            if (!S1) {
                C1 = (C1 & shtAmtMask);
            }

            if (!S0 && !S1) {
                C = (C0 << C1);
                return;
            }
            if (!S1) {
                uint64_t tC = (1ull << C1);
                S = mul(S0, tC);
            }
            break;
        }
        case Instruction::BitCast:
        case Instruction::IntToPtr:
        case Instruction::PtrToInt:
        {
            Value* V0 = Op->getOperand(0);
            getSymExprOrConstant(V0, S, C);
            if (!S)
            {
                return;
            }
            break;
        }
        case Instruction::SExt:
        case Instruction::ZExt:
        {
            SymCastInfo SCI = (opc == Instruction::SExt ? SYMCAST_SEXT : SYMCAST_ZEXT);
            Value* V0 = Op->getOperand(0);
            ValueSymInfo* VSI = getSymInfo(V0);
            if (!VSI) {
                // first time or it is a constant
                getSymExprOrConstant(V0, S, C);
                if (!S) {
                    // By default, sext is used. If zext, recalculate C.
                    if (SCI == SYMCAST_ZEXT) {
                        uint32_t bits = (uint32_t)V0->getType()->getPrimitiveSizeInBits();
                        uint64_t bitMask = maskTrailingOnes<uint64_t>(bits);
                        C = (C & bitMask);
                    }
                    return;
                }
                VSI = getSymInfo(V0);
                IGC_ASSERT(VSI);
            }

            // If V0 has been sext/zext before and this sext/zext isn't
            // the same as before, bail out so V (input) is inevaluable.
            if (VSI->CastInfo == SYMCAST_NOCAST || VSI->CastInfo == SCI) {
                S = VSI->symExpr;
                if (VSI->CastInfo == SYMCAST_NOCAST)
                    VSI->CastInfo = SCI;
            }
            break;
        }
        default:
            break;
        }
    }

    // Inevaluable for V. Its symExpr is itself. If V is a evaluable constant,
    // the function should have returned already and won't reach this place.
    if (S == nullptr)
    {
        SymProd* P = new (m_symProdAllocator.Allocate()) SymProd();
        SymTerm* T = new (m_symTermAllocator.Allocate()) SymTerm();
        SymExpr* E = new (m_symExprAllocator.Allocate()) SymExpr();
        P->Prod.push_back(V);
        T->Term = P;
        E->SymTerms.push_back(T);
        S = E;
    }

    setSymInfo(V, S);
    return;
}

int SymbolicEvaluation::cmp(const SymProd* T0, const SymProd* T1)
{
    int sz0 = T0->Prod.size();
    int sz1 = T1->Prod.size();

    if (sz0 > sz1) {
        return -1;
    }
    if (sz0 < sz1) {
        return 1;
    }
    for (int i = 0; i < sz0; ++i)
    {
        const Value* V0 = T0->Prod[i];
        const Value* V1 = T1->Prod[i];
        if (V0 == V1) {
            continue;
        }
        auto I0 = m_symInfos.find(V0);
        auto I1 = m_symInfos.find(V1);
        if (I0 != m_symInfos.end() && I1 != m_symInfos.end())
        {
            ValueSymInfo* VSI0 = I0->second;
            ValueSymInfo* VSI1 = I1->second;
            if (VSI0->ID < VSI1->ID) {
                return -1;
            }
            if (VSI0->ID > VSI1->ID)
            {
                return 1;
            }
        }
        else {
            IGC_ASSERT_MESSAGE(0, "V0 and/or V1 should be in the map!");
        }
    }
    return 0;
}

SymExpr* SymbolicEvaluation::add(
    SymExpr* S0, SymExpr* S1, bool negateS1)
{
    SymExpr* Expr = new (m_symExprAllocator.Allocate()) SymExpr();

    int i0 = 0, i1 = 0;
    const int e0 = S0->SymTerms.size();
    const int e1 = S1->SymTerms.size();
    while (i0 < e0 && i1 < e1)
    {
        SymTerm* T = nullptr;
        int64_t Coeff;
        SymTerm* T0 = S0->SymTerms[i0];
        SymTerm* T1 = S1->SymTerms[i1];
        int ret = cmp(T0->Term, T1->Term);
        if (ret < 0) {
            T = T0;
            Coeff = T0->Coeff;
            ++i0;
        }
        else if (ret > 0) {
            T = T1;
            Coeff = T1->Coeff;
            if (negateS1) {
                Coeff = -Coeff;
            }
            ++i1;
        }
        else { // ret == 0
            Coeff =
                negateS1 ? (T0->Coeff - T1->Coeff) : (T0->Coeff + T1->Coeff);
            if (Coeff != 0)
            {
                T = T0;
            }
            ++i0;
            ++i1;
        }

        if (T != nullptr) {
            SymTerm* newT = new (m_symTermAllocator.Allocate()) SymTerm();
            Expr->SymTerms.push_back(newT);
            copy(newT, T);
            newT->Coeff = Coeff;
        }
    }

    while (i0 < e0)
    {
        SymTerm* newT = new (m_symTermAllocator.Allocate()) SymTerm();
        Expr->SymTerms.push_back(newT);
        copy(newT, S0->SymTerms[i0]);
        ++i0;
    }
    while (i1 < e1)
    {
        SymTerm* newT = new (m_symTermAllocator.Allocate()) SymTerm();
        Expr->SymTerms.push_back(newT);
        copy(newT, S1->SymTerms[i1]);
        if (negateS1) {
            newT->Coeff = -newT->Coeff;
        }
        ++i1;
    }

    Expr->ConstTerm = negateS1 ? (S0->ConstTerm - S1->ConstTerm)
        : (S0->ConstTerm + S1->ConstTerm);
    return Expr;
}

SymExpr* SymbolicEvaluation::add(SymExpr* S, int64_t C)
{
    SymExpr* Expr = new (m_symExprAllocator.Allocate()) SymExpr();
    copy(Expr, S);
    Expr->ConstTerm += C;
    return Expr;
}

SymExpr* SymbolicEvaluation::mul(SymExpr* S, int64_t C)
{
    SymExpr* Expr = new (m_symExprAllocator.Allocate()) SymExpr();
    if (C != 0)
    {
        for (int i = 0, e = S->SymTerms.size(); i < e; ++i)
        {
            SymTerm* newT = new (m_symTermAllocator.Allocate()) SymTerm();
            copy(newT, S->SymTerms[i]);
            newT->Coeff *= C;
            Expr->SymTerms.push_back(newT);
        }
    }
    Expr->ConstTerm = S->ConstTerm * C;
    return Expr;
}


// If S1 - S0 is a constant, set that constant as COff
// and return true; otherwise return false.
bool SymbolicEvaluation::isOffByConstant(
    SymExpr* S0, SymExpr* S1, int64_t& COff)
{
    int e = (int)S0->SymTerms.size();
    if (e != (int)S1->SymTerms.size()) {
        return false;
    }

    for (int i = 0; i < e; ++i)
    {
        if (S0->SymTerms[i]->Coeff != S1->SymTerms[i]->Coeff ||
            cmp(S0->SymTerms[i]->Term, S1->SymTerms[i]->Term) != 0) {
            return false;
        }
    }

    COff = S1->ConstTerm - S0->ConstTerm;
    return true;
}


#define PASS_FLAG "igc-slmconstprop"
#define PASS_DESCRIPTION "Special const prop for SLM"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SLMConstProp, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(SLMConstProp, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SLMConstProp::ID = 0;

FunctionPass* IGC::createSLMConstPropPass()
{
    return new SLMConstProp();
}

bool SLMConstProp::isLinearFuncOfLocalIds(SymExpr* SE)
{
    for (int i = 0, e = (int)SE->SymTerms.size(); i < e; ++i)
    {
        SymTerm* aT = SE->SymTerms[i];
        // For now, handle a simple prod
        if (aT->Term->Prod.size() > 1) {
            return false;
        }
        const Value* Val = aT->Term->Prod[0];

        // Check if val is local ids, if not, quit
        const GenIntrinsicInst* sysVal = dyn_cast<const llvm::GenIntrinsicInst>(Val);
        if (!sysVal ||
            sysVal->getIntrinsicID() != GenISAIntrinsic::GenISA_DCL_SystemValue) {
            return false;
        }
        SGVUsage usage = static_cast<SGVUsage>(cast<ConstantInt>(sysVal->getOperand(0))->getZExtValue());
        if (usage != THREAD_ID_IN_GROUP_X && usage != THREAD_ID_IN_GROUP_Y && usage != THREAD_ID_IN_GROUP_Z)
        {
            return false;
        }
    }
    return true;
}

bool SLMConstProp::isEqual(Constant* C0, Constant* C1)
{
    Type* Ty0 = C0->getType();
    Type* Ty1 = C1->getType();

    // safety check, make sure the constant is float type
    if (!isFloatType(Ty0) || !isFloatType(Ty1))
    {
        return false;
    }

    ConstantFP* CF0;
    ConstantFP* CF1;
    if (ConstantDataVector * CDV0 = dyn_cast<ConstantDataVector>(C0))
    {
        CF0 = dyn_cast<ConstantFP>(CDV0->getElementAsConstant(0));
    }
    else
    {
        CF0 = dyn_cast<ConstantFP>(C0);
    }

    if (ConstantDataVector * CDV1 = dyn_cast<ConstantDataVector>(C1))
    {
        CF1 = dyn_cast<ConstantFP>(CDV1->getElementAsConstant(0));
    }
    else
    {
        CF1 = dyn_cast<ConstantFP>(C1);
    }

    if (CF0 && CF1 && CF0->isExactlyValue(CF1->getValueAPF())) {
        return true;
    }

    return false;
}

bool SLMConstProp::isFloatType(Type* Ty)
{
    if (IGCLLVM::FixedVectorType * vTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
    {
        if (vTy->getNumElements() > 1)
        {
            return false;
        }
        Ty = cast<VectorType>(vTy)->getElementType();
    }
    return Ty->isFloatTy();
}

Constant* SLMConstProp::convertIfNeeded(Type* Ty, Constant* C)
{
    if (Ty == C->getType())
    {
        return C;
    }

    // May handle other cases in future.
    return nullptr;
}


bool SLMConstProp::find_PATTERN_REPEAT_WITH_STRIDE(PatternInfo& aPattern)
{
    // Note that m_storeInfos keeps the sorted list of all stores.

    // First, make sure there are no two stores that access the same memory.
    // (Strong requirement, and can be relaxed to cover more cases if we
    //  have more cases.)
    int nStores = (int)m_storeInsts.size();
    for (int i = 1; i < (nStores - 1); ++i)
    {
        int ix = m_storeInfos[i].ix;
        StoreInst* SI = m_storeInsts[ix];
        Type* Ty = SI->getValueOperand()->getType();
        int off = m_storeInfos[i].offset;
        int endOff = off + (int)m_DL->getTypeStoreSize(Ty);
        if (m_storeInfos[i + 1].offset < endOff) {
            return false;
        }
    }

    // Now, no alias among stores. First, get the first two
    // constants that will be used to find out the pattern.
    int ix_1st = -1;
    int ix_2nd = -1;
    for (int i = 0; i < nStores; ++i)
    {
        if (m_storeInfos[i].Val)
        {
            if (ix_1st == -1) {
                ix_1st = i;
            }
            else if (ix_2nd == -1) {
                if (isEqual(m_storeInfos[ix_1st].Val,
                    m_storeInfos[i].Val))
                {
                    ix_2nd = i;
                    break;
                }
            }
        }
    }

    IGC_ASSERT_MESSAGE(-1 != ix_1st, "constant store expected but not found");

    SymExpr* basePtr = m_storeInfos[0].SE;
    StoreInst* lastSI = m_storeInsts[m_storeInfos[nStores - 1].ix];
    Type* lastTy = lastSI->getValueOperand()->getType();

    // Keep track of which stores are in this pattern.
    SmallVector<int, 32> patternStores;
    patternStores.push_back(ix_1st);

    int stride;
    if (ix_2nd == -1)
    {
        // maxSpan: the size of memory those stores may span, in which some
        // location is not necessarily stored.  For example, the first store
        // accesses location 0x1000, and the last 0x10fc. The maxSpan is
        //       (0x10fc + 4 (float) - 0x1000 = 0x100
        // As 'offset' in m_storeInfos is relative to the first store, the
        // last offset plus its stored size should be maxSpan.
        int maxSpan =
            m_storeInfos[nStores - 1].offset + (int)m_DL->getTypeStoreSize(lastTy);
        // Only one constant, set stride to be maxSpan.
        stride = maxSpan;
        if (!m_SymEval.isFactor(basePtr, stride))
        {
            return false;
        }
    }
    else
    {
        patternStores.push_back(ix_2nd);

        // Now, found two consecutive constants at ix_1stConst && ix_2ndConst
        // Check if the stride is a factor of basePtr. If so, we know that
        // only those stores in this pattern (if present, need to check if
        // the pattern exists in the following code) can write to those
        // locations. This is a condition to make sure that if other work-items
        // write to the location written by the current work-item, they must
        // write via those stores in this pattern, therefore, with the same
        // constant.
        stride = m_storeInfos[ix_2nd].offset - m_storeInfos[ix_1st].offset;
        if (!m_SymEval.isFactor(basePtr, stride))
        {
            return false;
        }

        // Now verify that every store at offset that is multiple of "stride",
        // will have the same constant as its stored value.
        Constant* CVal0 = m_storeInfos[ix_1st].Val;
        int offset = m_storeInfos[ix_1st].offset;
        offset = (offset % stride);
        for (int i = 0; i < nStores; ++i)
        {
            int ofst = m_storeInfos[i].offset;
            if (ofst < offset ||                // not in pattern
                (i >= ix_1st && i <= ix_2nd))   // already verified
            {
                continue;
            }
            int diff = ofst - offset;
            if ((diff % stride) == 0)
            {
                // This is a store to a location in the pattern.
                // Make sure its stored value is the same constant.
                Constant* CVal = m_storeInfos[i].Val;
                if (!CVal || !isEqual(CVal0, CVal))
                {
                    return false;
                }
                patternStores.push_back(i);
            }
        }
    }

    // pattern found.
    aPattern.Type = PATTERN_REPEAT_WITH_STRIDE;
    aPattern.stride = stride;
    aPattern.storedVal = m_storeInfos[ix_1st].Val;

    // Normalize the offset to be smaller than stride
    // (They are still the same pattern.)
    int normalized_ofst = m_storeInfos[ix_1st].offset + (int)basePtr->ConstTerm;
    aPattern.offset = (normalized_ofst % stride);

    // Set type for each storeInfos
    for (int i = 0, e = patternStores.size(); i < e; ++i)
    {
        int ix = patternStores[i];
        m_storeInfos[ix].ty = PATTERN_REPEAT_WITH_STRIDE;
    }
    patternStores.clear();

    return true;
}

bool SLMConstProp::perform_PATTERN_REPEAT_WITH_STRIDE(
    PatternInfo& aPattern)
{
    bool mayAccessPattern = false;
    bool changed = false;
    SymExpr* basePtr = nullptr;
    int nLoads = (int)m_loadInsts.size();
    for (int i = 0; i < nLoads; ++i)
    {
        LoadInst* LI = m_loadInsts[i];
        Value* ptrAddr = LI->getPointerOperand();

        // Handle scalar float load only for now.
        if (!isFloatType(LI->getType()))
        {
            mayAccessPattern = true;
            continue;
        }
        SymExpr* aSE = m_SymEval.getSymExpr(ptrAddr);

        if (basePtr == nullptr)
        {
            // To find a first match
            if (!isLinearFuncOfLocalIds(aSE) ||
                !m_SymEval.isFactor(aSE, aPattern.stride))
            {
                mayAccessPattern = true;
                continue;
            }

            int off = (int)aSE->ConstTerm - aPattern.offset;
            if ((off % aPattern.stride) != 0) {
                continue;
            }

            // Found
            basePtr = aSE;
        }
        else {
            // Only do it if it is based on the same base expression
            int64_t diff = 0;
            if (!m_SymEval.isOffByConstant(basePtr, aSE, diff))
            {
                mayAccessPattern = true;
                continue;
            }
            if ((diff % aPattern.stride) != 0) {
                continue;
            }
        }

        // This load is a constant load, replace it with constant.
        Constant* nVal = convertIfNeeded(LI->getType(), aPattern.storedVal);
        if (!nVal)
        {
            // may improve this once we see the test case.
            mayAccessPattern = true;
            continue;
        }
        LI->replaceAllUsesWith(nVal);
        LI->eraseFromParent();
        changed = true;
    }

    // disable this for now
    if (!mayAccessPattern && false)
    {
        // Since all loads to this pattern have been replaced, the stores
        // to this pattern are no longer needed. Delete those stores.
        int nStores = (int)m_storeInfos.size();
        for (int i = 0; i < nStores; ++i)
        {
            if (m_storeInfos[i].ty != PATTERN_REPEAT_WITH_STRIDE)
            {
                continue;
            }
            int ix = m_storeInfos[i].ix;
            StoreInst* SI = m_storeInsts[ix];
            SI->eraseFromParent();
        }
    }

    return changed;
}

bool SLMConstProp::analyzeConstantStores()
{
    // Check if all constant stores are fromed in a pattern.
    // To check this, all stores must be checked.

    int nStores = (int)m_storeInsts.size();
    m_storeInfos.resize(nStores);

    // Initially, use storeInfos[0]'s SE as the base SE.
    SymExpr* basePtr = nullptr;
    for (int i = 0; i < nStores; ++i)
    {
        int64_t diff = 0;
        StoreInst* SI = m_storeInsts[i];
        Value* ptrAddr = SI->getPointerOperand();
        Value* storedVal = SI->getValueOperand();

        // Handle scalar float store only for now.
        if (!isFloatType(storedVal->getType()))
        {
            return false;
        }
        m_storeInfos[i].ix = i;
        SymExpr* aSE = m_SymEval.getSymExpr(ptrAddr);
        m_storeInfos[i].SE = aSE;
        m_storeInfos[i].Val = dyn_cast<Constant>(storedVal);

        if (i == 0) {
            m_storeInfos[i].offset = 0;
            basePtr = aSE;
        }
        else if (m_SymEval.isOffByConstant(basePtr, aSE, diff))
        {
            m_storeInfos[i].offset = (int)diff;
        }
        else {
            // Don't handle it for now, quit
            return false;
        }
    }

    // Sort storeInfos in terms of increasing offset.
    // Return true if a goes before b.
    auto compareFunc = [](const StoreInfo& a, const StoreInfo& b) {
        return a.offset < b.offset;
    };
    std::sort(m_storeInfos.begin(), m_storeInfos.end(), compareFunc);

    // Readjust the offset based on the new baseptr
    int adjustAmt = m_storeInfos[0].offset;
    if (adjustAmt != 0)
    {
        for (int i = 0; i < nStores; ++i) {
            m_storeInfos[i].offset -= adjustAmt;
        }
    }
    // Re-set basePtr.
    // And at this point, m_storeInfos[0].offset == 0
    basePtr = m_storeInfos[0].SE;

    // Check if the base is based on local ids (or integer value)
    if (!isLinearFuncOfLocalIds(basePtr)) {
        return false;
    }

    //
    // Now, all stores are analzable, meaning their address is base + const-offset.
    // Next, find a pattern that we want to handle.
    //
    // For pattern type : PATTERN_REPEAT_WITH_STRIDE,  need to make sure that locations
    //   stride*0, stride*1, stride*2, ......, stride*n  (every stride bytes)
    // have the same constant.  Note that if there is no store to location at stride*k
    // (undefined), it is ignored (can safely assume it has the same constant);  if it is
    // defined multiple times with the same constant, it is okay as well.
    //
    PatternInfo aPattern;
    if (!find_PATTERN_REPEAT_WITH_STRIDE(aPattern)) {
        return false;
    }

    m_patternInfos.push_back(aPattern);
    return true;
}


bool SLMConstProp::performConstProp()
{
    bool changed = false;
    for (int i = 0, e = m_patternInfos.size(); i < e; ++i)
    {
        switch (m_patternInfos[i].Type) {
        case PATTERN_REPEAT_WITH_STRIDE:
            changed = perform_PATTERN_REPEAT_WITH_STRIDE(m_patternInfos[i]);
            break;

        default:
            break;
        }
    }
    return changed;
}

bool SLMConstProp::runOnFunction(Function& F)
{
#if 0
    {
        CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
        m_Ctx = pCtxWrapper->getCodeGenContext();

        //COMPILER_TIME_END(cgCtx, TIME_CG_PreCodeEmit);
        DumpLLVMIR(m_Ctx, "beforeSLMConstProp");
    }
#endif

    // Do this only for the top function
    MetaDataUtils* pMdUtils = nullptr;
    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }

    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    m_Ctx = pCtxWrapper->getCodeGenContext();

    m_F = &F;


    // Idea:
    //   1. Handle the case that all writes to SLM is done before a barrier
    //      AND all reads to SLM is done after the barrier; AND all addresses
    //      will be based on local ids so their access pattern is known. Also,
    //      all addresses are off by a constant so that we don't need to check
    //      aliases among different stores. And
    //   2. the same constant is written to the SLM location in a clear pattern
    //      For now,  we will check the pattern:
    //        PATTERN_REPEAT_WITH_STRIDE (see PatternType)
    //           The constant is written every <fixed amount> bytes, where
    //           <fixed amount> is stride in bytes. For this case, we know which
    //           location of SLM will have this constant.
    //      It could handle other patterns in the future.
    //
    // Algorithm:
    //    1. Collect all SLM loads and stores; if no loads or no stores, quit.
    //       If no constant stores, quit.
    //    2. Find barrier, if not present, quit.
    //    3. Make sure all stores dominate the barrier, and the barrier
    //       dominate all loads. If not, quit.
    //    4. Analyze all stores,  their addresses must be based on local ids or
    //       constant. If not, quit.
    //    5. Check if constant stores have a clear pattern, if not, quit.
    //    6. Check all loads, if any of loads matches the pattern, replace it with
    //       the constant.
    //
    //

    // Collect all loads and stores
    //    When collecing loads/stores, keep their BBs, which is convenient
    //    for checking that all stores dominate barrier and barrier dominates
    //    all loads.
    SmallPtrSet<BasicBlock*, 4> loadBBs;
    SmallPtrSet<BasicBlock*, 4> storeBBs;
    Instruction* barrierInst = nullptr;
    bool hasConstantStore = false;

    // Used to Make sure that all SLM accesses are within  main func.
    bool mayHaveUserFuncCalls = false;

    // Used to make sure SLM is only accessed via ld/st instruction.
    bool usedByNonLDST = false;

    for (Function::iterator FI = m_F->begin(), FE = m_F->end(); FI != FE; ++FI)
    {
        BasicBlock* BB = &*FI;
        for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
        {
            Instruction* I = &*BI;
            if (LoadInst * LI = dyn_cast<LoadInst>(I)) {
                if (LI->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL) {
                    m_loadInsts.push_back(LI);
                    (void)loadBBs.insert(BB);
                }
            }
            else if (StoreInst * SI = dyn_cast<StoreInst>(I)) {
                if (SI->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
                {
                    m_storeInsts.push_back(SI);
                    (void)storeBBs.insert(BB);
                    if (dyn_cast<Constant>(SI->getValueOperand()))
                    {
                        hasConstantStore = true;
                    }
                }
            }
            else if (CallInst * CallI = dyn_cast<CallInst>(I)) {
                // Skip if a user func is invoked.
                Function* Callee = CallI->getCalledFunction();
                if ((!Callee && !CallI->isInlineAsm()) || (Callee && !Callee->isDeclaration())) {
                    mayHaveUserFuncCalls = true;
                    break;
                }

                bool checkSLMArg = true;
                if (GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(CallI)) {
                    GenISAIntrinsic::ID id = GII->getIntrinsicID();
                    if (!barrierInst &&
                        id == GenISAIntrinsic::GenISA_threadgroupbarrier) {
                        // Use the first threadgroupbarrier
                        barrierInst = I;
                        checkSLMArg = false; // skip checking this intrinsic.
                    }
                }

                if (checkSLMArg) {
                    // Make sure those intrinsic does not use ptr to SLM
                    // (for example, SLM atomic, etc.
                    for (int i = 0, e = (int)IGCLLVM::getNumArgOperands(CallI);
                        i < e; ++i) {
                        Type* Ty = CallI->getArgOperand(i)->getType();
                        if (PointerType * PTy = dyn_cast<PointerType>(Ty)) {
                            if (PTy->getAddressSpace() == ADDRESS_SPACE_LOCAL)
                            {
                                usedByNonLDST = true;
                                break;
                            }
                        }
                    }
                    if (usedByNonLDST) {
                        break;
                    }
                }
            }
            else if (AddrSpaceCastInst * ASC = dyn_cast<AddrSpaceCastInst>(I)) {
                PointerType* DstPTy = cast<PointerType>(ASC->getDestTy());
                PointerType* SrcPTy = cast<PointerType>(ASC->getSrcTy());
                if (DstPTy->getAddressSpace() == ADDRESS_SPACE_LOCAL ||
                    SrcPTy->getAddressSpace() == ADDRESS_SPACE_LOCAL) {
                    usedByNonLDST = true;
                    break;
                }
            }
        }

        if (mayHaveUserFuncCalls || usedByNonLDST) {
            break;
        }
    }

    if (mayHaveUserFuncCalls || usedByNonLDST ||
        !hasConstantStore || !barrierInst || m_loadInsts.size() == 0) {
        return false;
    }

    // Check if all stores dominates barrier and
    // the barrier dominates all loads; if not, quit.
    m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    // set it to true if load/store are in the same BB as barrier.
    bool sameBBAsBarrier = false;

    BasicBlock* barrierBB = barrierInst->getParent();
    for (auto Iter : storeBBs)
    {
        BasicBlock* tBB = Iter;
        if (tBB == barrierBB) {
            sameBBAsBarrier = true;
        }
        else if (!m_DT->dominates(tBB, barrierBB)) {
            return false;
        }
    }
    if (sameBBAsBarrier)
    {
        // Make sure there is no store after barrier instruction
        BasicBlock::iterator II(barrierInst);
        BasicBlock::iterator IE = barrierBB->end();
        for (++II; II != IE; ++II)
        {
            Instruction* Inst = &*II;
            StoreInst* SI = dyn_cast<StoreInst>(Inst);
            if (SI && SI->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL) {
                return false;
            }
        }
    }

    // Make sure all loads are dominated by the barrier
    sameBBAsBarrier = false;
    for (auto Iter : loadBBs)
    {
        BasicBlock* tBB = Iter;
        if (tBB == barrierBB) {
            sameBBAsBarrier = true;
        }
        else if (!m_DT->dominates(barrierBB, tBB)) {
            return false;
        }
    }
    if (sameBBAsBarrier)
    {
        // Make sure there is no load before barrier instruction
        BasicBlock::iterator IE(barrierInst);
        for (BasicBlock::iterator II = barrierBB->begin(); II != IE; ++II)
        {
            Instruction* Inst = &*II;
            LoadInst* LI = dyn_cast<LoadInst>(Inst);
            if (LI && LI->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL) {
                return false;
            }
        }
    }

    Module* M = m_F->getParent();
    m_DL = &M->getDataLayout();
    m_SymEval.setDataLayout(m_DL);

    // Analyze the constant stores and see if they have a particular pattern
    if (!analyzeConstantStores()) {
        return false;
    }

    // Analyze the loads and apply const prop if available.
    if (!performConstProp()) {
        return false;
    }

#if 0
    {
        CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
        m_Ctx = pCtxWrapper->getCodeGenContext();

        //COMPILER_TIME_END(cgCtx, TIME_CG_PreCodeEmit);
        DumpLLVMIR(m_Ctx, "afterSLMConstProp");
    }
#endif
    return true;
}
