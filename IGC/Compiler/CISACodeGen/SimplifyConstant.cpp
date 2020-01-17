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
#include "Compiler/CISACodeGen/SimplifyConstant.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

#include "llvmWrapper/IR/Value.h"

#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"

using namespace llvm;
using namespace IGC;

namespace {

    /// \brief Perform by-value simplification of loading constant data.
    ///
    /// Currently this only applies to certain constant data array initializers.
    ///
    class SimplifyConstant : public ModulePass {
    public:
        static char ID; // Pass identification, replacement for typeid
        SimplifyConstant() : ModulePass(ID) {
            initializeSimplifyConstantPass(*PassRegistry::getPassRegistry());
        }
        bool runOnModule(Module& M);

    private:
        bool process(GlobalVariable* GV);
    };

} // namespace

namespace IGC {

    IGC_INITIALIZE_PASS_BEGIN(SimplifyConstant, "SimplifyConstant", "SimplifyConstant", false, false)
        IGC_INITIALIZE_PASS_END(SimplifyConstant, "SimplifyConstant", "SimplifyConstant", false, false)

} // namespace IGC

char SimplifyConstant::ID = 0;
ModulePass* IGC::createSimplifyConstantPass() { return new SimplifyConstant(); }

bool SimplifyConstant::runOnModule(Module& M) {
    bool Changed = false;
    for (auto I = M.global_begin(); I != M.global_end(); /*empty*/) {
        GlobalVariable* GV = &(*I++);
        if (GV->user_empty() || !GV->isConstant() || !GV->hasInitializer() ||
            GV->getType()->getAddressSpace() != ADDRESS_SPACE_CONSTANT)
            continue;
        Changed |= process(GV);
    }
    return Changed;
}

namespace {

    class ConstantLoader {
    public:
        enum CInitKind {
            CK_Unknown,
            // All elements are equal.
            CK_Splat,
            // f(i) = (i % 2) == 0 ? A : B;
            CK_OddEven,
            // f(i) = (i < Pivot) ? A : B
            CK_Ladder2
        };

        explicit ConstantLoader(GlobalVariable* GV)
            : GV(GV), Kind(CK_Unknown), Pivot(0) {
            analyze();
            simplify();
        }

        bool simplified() const { return Kind != CK_Unknown; }

    private:
        void analyze();
        void simplify();

        bool matchSplat(ConstantDataArray* CDA) {
            Constant* V0 = CDA->getElementAsConstant(0);
            for (unsigned i = 1, n = CDA->getNumElements(); i < n; ++i)
                if (CDA->getElementAsConstant(i) != V0)
                    return false;
            Kind = CK_Splat;
            return true;
        }
        bool matchOddEven(ConstantDataArray* CDA) {
            Constant* V0 = CDA->getElementAsConstant(0);
            Constant* V1 = CDA->getElementAsConstant(1);
            for (unsigned i = 2, n = CDA->getNumElements(); i < n; ++i) {
                Constant* Val = CDA->getElementAsConstant(i);
                if ((i % 2 == 0 && Val != V0) || (i % 2 == 1 && Val != V1))
                    return false;
            }
            Kind = CK_OddEven;
            return true;
        }
        bool matchLadder2(ConstantDataArray* CDA) {
            Constant* V = CDA->getElementAsConstant(0);
            unsigned k = 0;
            for (unsigned i = 1, n = CDA->getNumElements(); i < n; ++i) {
                Constant* Val = CDA->getElementAsConstant(i);
                if (Val != V) {
                    if (k > 0)
                        return false;

                    V = Val;
                    k = i;
                }
            }

            Kind = CK_Ladder2;
            Pivot = k;
            return true;
        }

        GlobalVariable* GV;
        CInitKind Kind;
        // Index of the second value if exists.
        unsigned Pivot;
    };

} // namespace

void ConstantLoader::analyze() {
    auto Init = GV->getInitializer();
    auto CDA = dyn_cast<ConstantDataArray>(Init);
    if (!CDA || CDA->isString() || CDA->getNumElements() <= 1)
        return;

    matchSplat(CDA) || matchOddEven(CDA) || matchLadder2(CDA);
}

void ConstantLoader::simplify() {
    if (Kind == CK_Unknown)
        return;

    Constant* Init = GV->getInitializer();
    auto CDA = cast<ConstantDataArray>(Init);
    Constant* V0 = CDA->getElementAsConstant(0);
    Constant* V1 = nullptr;
    if (Kind == CK_OddEven)
        V1 = CDA->getElementAsConstant(1);
    else if (Kind == CK_Ladder2)
        V1 = CDA->getElementAsConstant(Pivot);

    // Replace all GEP + load uses with V0 or a select of V0 and V1.
    for (auto UI = GV->user_begin(); UI != GV->user_end(); /* empty */) {
        auto GEP = dyn_cast<GetElementPtrInst>(*UI++);
        if (!GEP || !GEP->isInBounds())
            continue;

        // Skip optimizations on function with optnone.
        Function* F = GEP->getParent()->getParent();
        if (F->hasFnAttribute(Attribute::OptimizeNone))
            continue;

        for (auto I = GEP->user_begin(); I != GEP->user_end(); /* empty */) {
            LoadInst* LI = dyn_cast<LoadInst>(*I++);
            if (!LI)
                continue;
            assert(GEP->getNumIndices() == 2);
            Value* Index = GEP->getOperand(2);
            Value* Val = V0;
            IRBuilder<> Builder(LI);
            if (Kind == CK_Ladder2) {
                Value* Cmp = Builder.CreateICmpULT(
                    Index, ConstantInt::get(Index->getType(), Pivot));
                Val = Builder.CreateSelect(Cmp, V0, V1);
            }
            else if (Kind == CK_OddEven) {
                Value* Cmp = Builder.CreateTrunc(Index, Builder.getInt1Ty());
                Val = Builder.CreateSelect(Cmp, V1, V0);
            }
            LI->replaceAllUsesWith(Val);
            LI->eraseFromParent();
        }

        if (GEP->use_empty())
            GEP->eraseFromParent();
    }
}

bool SimplifyConstant::process(GlobalVariable* GV) {
    ConstantLoader Loader(GV);
    if (Loader.simplified())
        return true;

    return false;
}

namespace {

    /// \brief Promote small constant data from global to register.
    ///
    class PromoteConstant : public FunctionPass {
    public:
        static char ID; // Pass identification, replacement for typeid
        PromoteConstant() : FunctionPass(ID) {
            initializePromoteConstantPass(*PassRegistry::getPassRegistry());
        }

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addPreserved<LoopInfoWrapperPass>();
        }

        bool runOnFunction(Function& F) override;
    };

} // namespace

namespace IGC {

    IGC_INITIALIZE_PASS_BEGIN(PromoteConstant, "PromoteConstant", "PromoteConstant", false, false)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(PromoteConstant, "PromoteConstant", "PromoteConstant", false, false)

} // namespace IGC

char PromoteConstant::ID = 0;
FunctionPass* IGC::createPromoteConstantPass() { return new PromoteConstant(); }

// Check uses. Within this function, this GV must be used and only used by
// in-bound GEPs each of which is also only used by loads.
//
// We only promote when there is load of GV inside loops.
//
static bool checkUses(GlobalVariable* GV, const Function* F, LoopInfo& LI, const ConstantArray* llvm_used) {
    bool LoadInLoop = false;
    for (auto U : GV->users()) {
        // Because of ProgramScopeConstantAnalysis there might be user in llvm.used
        // let's ignore this one, as it doesn't prevent opt.
        // TODO: is there any other case of Constant to worry about. (?)
        if (auto Const = dyn_cast<Constant>(U))
        {
            bool foundInLLVMUsed = false;
            unsigned numOperands = llvm_used ? llvm_used->getNumOperands() : 0;
            for (unsigned i = 0; i != numOperands; ++i)
            {
                Value* gvi = llvm_used->getOperand(i)->stripPointerCastsNoFollowAliases();
                if (gvi == Const->stripPointerCastsNoFollowAliases())
                {
                    foundInLLVMUsed = true;
                    break;
                }
            }
            if (foundInLLVMUsed)
                continue;
        }

        auto Inst = dyn_cast<Instruction>(U);
        if (!Inst)
            // TODO: this may be a const expr.
            return false;

        if (F != Inst->getParent()->getParent())
            continue;

        auto GEP = dyn_cast<GetElementPtrInst>(Inst);
        if (!GEP || !GEP->isInBounds())
            return false;

        for (auto V : GEP->users()) {
            auto Inst = dyn_cast<LoadInst>(V);
            if (!Inst)
                return false;
            if (LI.getLoopFor(Inst->getParent()) != nullptr)
                LoadInLoop = true;
        }
    }

    if (IGC_IS_FLAG_ENABLED(AllowNonLoopConstantPromotion))
        return true;

    return LoadInLoop;
}

// IGC only allows the following vector sizes:
//
//------------------------------------
//                 name       size
//                         in elements
//------------------------------------
// IGC_IR_VECTOR_TYPE(x1,           1)
// IGC_IR_VECTOR_TYPE(x2,           2)
// IGC_IR_VECTOR_TYPE(x3,           3)
// IGC_IR_VECTOR_TYPE(x4,           4)
// IGC_IR_VECTOR_TYPE(x5,           5)
// IGC_IR_VECTOR_TYPE(x8,           8)
// IGC_IR_VECTOR_TYPE(x10,         10)
// IGC_IR_VECTOR_TYPE(x16,         16)
// IGC_IR_VECTOR_TYPE(x32,         32)
//
// check or return a legal vector size. 0 means illegal.
static unsigned getLegalVectorSize(unsigned N) {
#if 0
    if (N > 32)
        return 0;

    if (N == 6 || N == 7)
        return 8;
    if (N == 9)
        return 10;
    if (N >= 11 && N <= 16)
        return 16;
    if (N >= 17 && N <= 32)
        return 32;
    return N;
#else
    // Only emit power-of-two vector sizes.
    N = 1U << Log2_32_Ceil(N);
    return (N > 32) ? 0 : N;
#endif
}

// Check vector size. We may demote the data type if all values can fit into
// smaller data type.
//
static bool checkSize(GlobalVariable* GV, VectorType*& DataType,
    bool& IsSigned) {
    Constant* Init = GV->getInitializer();
    assert(isa<ArrayType>(Init->getType()));
    ArrayType* ArrayTy = cast<ArrayType>(Init->getType());
    unsigned N = (unsigned)ArrayTy->getArrayNumElements();
    Type* BaseTy = ArrayTy->getArrayElementType();
    unsigned VectorSize = 1;
    if (auto VT = dyn_cast<VectorType>(BaseTy)) {
        BaseTy = VT->getElementType();
        VectorSize = int_cast<unsigned>(VT->getNumElements());
        N *= VectorSize;
    }
    unsigned VS = getLegalVectorSize(N);

    // Allow experimental overriding for bigger than 32 elem. tables
    if (IGC_IS_FLAG_ENABLED(AllowNonLoopConstantPromotion))
        VS = 1U << Log2_32_Ceil(N);

    if (VS == 0)
        return false;

    if (BaseTy->isIntegerTy()) {
        // Cached integer types.
        Type* Int8Ty = IntegerType::get(GV->getContext(), 8);
        Type* Int16Ty = IntegerType::get(GV->getContext(), 16);

        // [i32 -1, i32 1] can be represented as {<i8 -1, i8 1>, Signed}
        // and [i32 200, i32 1] can be represented as {<i8 200, i8 1>, !Signed}
        int64_t Min = INT64_MAX, Max = INT64_MIN;
        for (unsigned i = 0; i < ArrayTy->getArrayNumElements(); ++i) {
            auto Elt = Init->getAggregateElement(i);
            if (isa<UndefValue>(Elt))
                continue;
            if (Elt->getType()->isVectorTy()) {
                for (unsigned j = 0; j < VectorSize; ++j) {
                    auto VecElt = Elt->getAggregateElement(j);
                    int64_t Val = cast<ConstantInt>(VecElt)->getSExtValue();
                    Min = std::min(Min, Val);
                    Max = std::max(Max, Val);
                }
            }
            else {
                int64_t Val = cast<ConstantInt>(Elt)->getSExtValue();
                Min = std::min(Min, Val);
                Max = std::max(Max, Val);
            }
        }

        unsigned BaseSzInBits = (unsigned int)BaseTy->getPrimitiveSizeInBits();
        assert(BaseSzInBits >= 8);
        if (BaseSzInBits > 8 && Min >= 0 && Max <= UINT8_MAX) {
            BaseTy = Int8Ty;
            IsSigned = false;
        }
        else if (BaseSzInBits > 8 && Min >= INT8_MIN && Max <= INT8_MAX) {
            BaseTy = Int8Ty;
            IsSigned = true;
        }
        else if (BaseSzInBits > 16 && Min >= 0 && Max <= UINT16_MAX) {
            BaseTy = Int16Ty;
            IsSigned = false;
        }
        else if (BaseSzInBits > 16 && Min >= INT16_MIN && Max <= INT16_MAX) {
            BaseTy = Int16Ty;
            IsSigned = true;
        }
    }

    // Two GRFs by default.
    unsigned ThresholdInBits = IGC_GET_FLAG_VALUE(ConstantPromotionSize) * 256;
    unsigned TotalSzInBits = (unsigned int)BaseTy->getPrimitiveSizeInBits() * VS;
    if (TotalSzInBits <= ThresholdInBits) {
        DataType = VectorType::get(BaseTy, VS);
        return true;
    }

    return false;
}

// Check if a vector of data could be built out of initializer.
static bool checkType(GlobalVariable* GV) {
    Constant* Init = GV->getInitializer();
    if (auto CDA = dyn_cast<ConstantDataArray>(Init))
        return !CDA->isString();

    // Not simply data. Need to check if all elements have the same type in a
    // constant array.
    if (auto CA = dyn_cast<ConstantArray>(Init)) {
        auto EltTy = CA->getType()->getElementType();
        return EltTy->isFloatingPointTy() || EltTy->isIntegerTy() ||
            EltTy->isVectorTy();
    }

    return false;
}

// Extract N elements from a vector, Idx, ... Idx + N - 1. Return a scalar
// or a vector value depending on N.
static Value* extractNElts(unsigned N, Value* VectorData, Value* Offset,
    IRBuilder<>& IRB) {
    if (N == 1)
        return IRB.CreateExtractElement(VectorData, Offset);

    Type* Ty = VectorData->getType()->getVectorElementType();
    Ty = VectorType::get(Ty, N);
    Value* Result = UndefValue::get(Ty);
    for (unsigned i = 0; i < N; ++i) {
        Value* VectorIdx = ConstantInt::get(Offset->getType(), i);
        if (i == 0) {
            auto Val = IRB.CreateExtractElement(VectorData, Offset);
            Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
        }
        else {
            auto Idx = IRB.CreateAdd(Offset, VectorIdx);
            auto Val = IRB.CreateExtractElement(VectorData, Idx);
            Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
        }
    }

    return Result;
}

static Constant* getConstantVal(Type* VEltTy, Constant* V, bool IsSigned) {
    if (V->getType() == VEltTy)
        return V;
    assert(VEltTy->isIntegerTy());
    int64_t IVal = cast<ConstantInt>(V)->getSExtValue();
    return ConstantInt::get(VEltTy, IVal, IsSigned);
}

static void promote(GlobalVariable* GV, VectorType* AllocaType, bool IsSigned,
    Function* F) {
    // Build the constant vector from constant array.
    unsigned VS = int_cast<unsigned>(AllocaType->getNumElements());
    SmallVector<Constant*, 16> Vals(VS, nullptr);
    Type* VEltTy = AllocaType->getElementType();
    auto Init = GV->getInitializer();

    if (auto CDA = dyn_cast<ConstantDataArray>(Init)) {
        unsigned NElts = CDA->getNumElements();
        for (unsigned i = 0; i < NElts; ++i) {
            Constant* Elt = CDA->getAggregateElement(i);
            assert(Elt && "Null AggregateElement");
            Vals[i] = getConstantVal(VEltTy, Elt, IsSigned);
        }
    }
    else {
        assert(isa<ConstantArray>(Init) && "out of sync");
        ConstantArray* CA = cast<ConstantArray>(Init);
        unsigned NElts = CA->getNumOperands();
        for (unsigned i = 0; i < NElts; ++i) {
            Constant* Elt = CA->getAggregateElement(i);
            if (auto EltTy = dyn_cast<VectorType>(Elt->getType())) {
                unsigned VectorSize = EltTy->getVectorNumElements();
                for (unsigned j = 0; j < VectorSize; ++j) {
                    Constant* V = Elt->getAggregateElement(j);
                    assert(Elt && "Null AggregateElement");
                    Vals[i * VectorSize + j] = getConstantVal(VEltTy, V, IsSigned);
                }
            }
            else
                Vals[i] = getConstantVal(VEltTy, Elt, IsSigned);
        }
    }
    // Fill the missing values with undef, if any.
    for (int i = VS - 1; i >= 0; --i) {
        if (Vals[i] != nullptr)
            break;
        Vals[i] = UndefValue::get(VEltTy);
    }
    Constant* VectorData = ConstantVector::get(Vals);

    // Transform all uses
    for (auto UI = GV->user_begin(); UI != GV->user_end(); /*empty*/) {
        auto GEP = dyn_cast<GetElementPtrInst>(*UI++);
        // might be Constant user in llvm.used
        if (!GEP || GEP->getParent()->getParent() != F)
            continue;
        assert(GEP->getNumIndices() == 2);
        // This is the index to address the array, and the first index is to address
        // the global variable itself.
        Value* Index = GEP->getOperand(2);
        // Demote the index type to int32 to avoid 64 multiplications during vISA
        // emission, e.g. it is illegal to emit Q x W.
        if (Index->getType()->getPrimitiveSizeInBits() > 32) {
            IRBuilder<> Builder(GEP);
            Index = Builder.CreateTrunc(Index, Builder.getInt32Ty());
        }
        for (auto I = GEP->user_begin(); I != GEP->user_end(); /*empty*/) {
            auto LI = dyn_cast<LoadInst>(*I++);
            assert(LI && "nullptr");
            IRBuilder<> Builder(LI);
            Type* Ty = LI->getType();
            unsigned N = 1;
            Value* Offset = Index;
            if (Ty->isVectorTy()) {
                N = Ty->getVectorNumElements();
                Offset = Builder.CreateMul(Offset, ConstantInt::get(Offset->getType(), N));
            }
            Value* Val = extractNElts(N, VectorData, Offset, Builder);
            if (Val->getType() != LI->getType()) {
                assert(Val->getType()->isIntOrIntVectorTy());
                Val = Builder.CreateIntCast(Val, LI->getType(), IsSigned);
            }
            LI->replaceAllUsesWith(Val);
            LI->eraseFromParent();
        }
    }
}

static Constant* getElt(Constant* Init, int k) {
    int n = (int)Init->getType()->getArrayNumElements();
    if (k >= n)
        return UndefValue::get(Init->getType()->getArrayElementType());
    return Init->getAggregateElement(k);
};

// Recursively emit cmp+sel tree.
static Value* getVal(IRBuilder<>& Builder, Constant* Init, Value* Index,
    int Low, int Hi) {
    assert(Hi > Low);
    Type* IdxTy = Index->getType();
    // base case.
    if (Hi == 1 + Low) {
        Value* Cmp = Builder.CreateICmpEQ(Index, ConstantInt::get(IdxTy, Low));
        return Builder.CreateSelect(Cmp, getElt(Init, Low), getElt(Init, Hi));
    }

    // There are more than two elements to be compared.
    int Mid = (Low + Hi + 1) / 2;
    Value* LHS = getVal(Builder, Init, Index, Low, Mid - 1);
    Value* RHS = getVal(Builder, Init, Index, Mid, Hi);
    Value* Cmp = Builder.CreateICmpSLT(Index, ConstantInt::get(IdxTy, Mid));
    return Builder.CreateSelect(Cmp, LHS, RHS);
}

static bool rewriteAsCmpSel(GlobalVariable* GV, Function& F) {
    bool Changed = false;

    Type* Ty = GV->getInitializer()->getType();
    assert(Ty->isArrayTy());
    unsigned NElts = (unsigned)Ty->getArrayNumElements();

    for (auto UI = GV->user_begin(); UI != GV->user_end(); /*empty*/) {
        auto GEP = dyn_cast<GetElementPtrInst>(*UI++);
        // might be Constant user in llvm.used
        if (!GEP || GEP->getParent()->getParent() != &F)
            continue;

        // This is the index to address the array, and the first index is to
        // address the global variable itself.
        assert(GEP->getNumIndices() == 2);
        Value* Index = GEP->getOperand(2);
        if (Index->getType()->getPrimitiveSizeInBits() > 32) {
            IRBuilder<> Builder(GEP);
            Index = Builder.CreateTrunc(Index, Builder.getInt32Ty());
        }
        for (auto I = GEP->user_begin(); I != GEP->user_end(); /*empty*/) {
            auto LI = dyn_cast<LoadInst>(*I++);
            assert(LI && "nullptr");
            IRBuilder<> Builder(LI);

            int n = (int)NextPowerOf2(NElts - 1);
            Value* Val = nullptr;
            if (n > 1)
                Val = getVal(Builder, GV->getInitializer(), Index, 0, n - 1);
            else
                Val = getElt(GV->getInitializer(), 0);
            LI->replaceAllUsesWith(Val);
            LI->eraseFromParent();
            Changed = true;
        }
    }

    return Changed;
}

// Check if it is profitable to emit cmp-sel.
//
// For an array of size N = 2^k, (N - 1) cmp + (N - 1) sel is needed to extract
// a single element. We use the following threshold:
//
// (1) if N <= 4, return true, or
// (2) if N <= 8 and element type is vector, return true, otherwise
// (3) return false.
//
// [4 x float],          6 ops, presumably better than a send
// [4 x <2 x float> ],   6 ops, ditto
// [8 x <2 x float> ],  14 ops, ditto
// [16 x <2 x float> ], 30 ops, close to a send
//
static bool isCmpSelProfitable(GlobalVariable* GV) {
    Constant* Init = GV->getInitializer();
    unsigned NElts = (unsigned)Init->getType()->getArrayNumElements();
    unsigned CmpSelSize = IGC_GET_FLAG_VALUE(ConstantPromotionCmpSelSize);

    if (NElts <= CmpSelSize)
        return true;
    Type* EltTy = Init->getType()->getArrayElementType();
    return EltTy->isVectorTy() && NElts <= CmpSelSize * 2;
}

bool PromoteConstant::runOnFunction(Function& F) {
    if (skipFunction(F))
        return false;

    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    Module* M = F.getParent();
    bool Changed = false;

    auto gv = M->getGlobalVariable("llvm.used");
    ConstantArray* llvm_used = gv ? dyn_cast_or_null<ConstantArray>(gv->getInitializer()) : nullptr;

    for (auto I = M->global_begin(); I != M->global_end(); /*empty*/) {
        GlobalVariable* GV = &(*I++);
        if (GV->user_empty() || !GV->isConstant() || !GV->hasInitializer() ||
            GV->getType()->getAddressSpace() != ADDRESS_SPACE_CONSTANT)
            continue;
        if (!checkType(GV))
            continue;
        if (!checkUses(GV, &F, LI, llvm_used))
            continue;

        // Rewrite as cmp+sel sequence if profitable.
        if (isCmpSelProfitable(GV)) {
            Changed |= rewriteAsCmpSel(GV, F);
            continue;
        }

        // If possible demote the data into smaller type. Uses of value will be
        // promoted back with ZExt or SExt.
        VectorType* AllocaType = nullptr;
        bool IsSigned = false;
        if (!checkSize(GV, AllocaType, IsSigned))
            continue;

        assert(AllocaType);
        promote(GV, AllocaType, IsSigned, &F);
        Changed = true;
    }

    return Changed;
}
