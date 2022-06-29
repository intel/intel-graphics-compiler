/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/ADT/STLExtras.h>
#include <llvmWrapper/Analysis/MemoryLocation.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalAlias.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvmWrapper/Support/Alignment.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/DebugCounter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/MemOpt.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

DEBUG_COUNTER(MergeLoadCounter, "memopt-merge-load",
    "Controls count of merged loads");

DEBUG_COUNTER(MergeStoreCounter, "memopt-merge-store",
    "Controls count of merged stores");

namespace {
    // This pass merge consecutive loads/stores within a BB when it's safe:
    // - Two loads (one of them is denoted as the leading load if it happens
    //   before the other one in the program order) are safe to be merged, i.e.
    //   the non-leading load is merged into the leading load, iff there's no
    //   memory dependency between them which may results in different loading
    //   result.
    // - Two stores (one of them is denoted as the tailing store if it happens
    //   after the other one in the program order) are safe to be merged, i.e.
    //   the non-tailing store is merged into the tailing one, iff there's no
    //   memory dependency between them which may results in different result.
    //
    class MemOpt : public FunctionPass {
        const DataLayout* DL;
        AliasAnalysis* AA;
        ScalarEvolution* SE;
        WIAnalysis* WI;

        CodeGenContext* CGC;
        TargetLibraryInfo* TLI;

        bool AllowNegativeSymPtrsForLoad = false;
        bool AllowVector8LoadStore = false;

        // Map of profit vector lengths per scalar type. Each entry specifies the
        // profit vector length of a given scalar type.
        // NOTE: Prepare the profit vector lengths in the *DESCENDING* order.
        typedef DenseMap<unsigned int, SmallVector<unsigned, 4> > ProfitVectorLengthsMap;
        ProfitVectorLengthsMap ProfitVectorLengths;

        // A list of memory references (within a BB) with the distance to the begining of the BB.
        typedef std::vector<std::pair<Instruction*, unsigned> > MemRefListTy;
        typedef std::vector<Instruction*> TrivialMemRefListTy;

    public:
        static char ID;

        MemOpt(bool AllowNegativeSymPtrsForLoad = false, bool AllowVector8LoadStore = false) :
            FunctionPass(ID), DL(nullptr), AA(nullptr), SE(nullptr), WI(nullptr),
            CGC(nullptr), AllowNegativeSymPtrsForLoad(AllowNegativeSymPtrsForLoad),
            AllowVector8LoadStore(AllowVector8LoadStore)
        {
            initializeMemOptPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<AAResultsWrapperPass>();
            AU.addRequired<TargetLibraryInfoWrapperPass>();
            AU.addRequired<ScalarEvolutionWrapperPass>();
            AU.addRequired<WIAnalysis>();
        }

        void buildProfitVectorLengths(Function& F);

        bool mergeLoad(LoadInst* LeadingLoad, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt);
        bool mergeStore(StoreInst* LeadingStore, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt);

        unsigned getNumElements(Type* Ty) const {
            return Ty->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;
        }

        MemoryLocation getLocation(Instruction* I) const {

            if (LoadInst * LI = dyn_cast<LoadInst>(I))
                return MemoryLocation::get(LI);

            if (StoreInst * SI = dyn_cast<StoreInst>(I))
                return MemoryLocation::get(SI);

            if (isa<LdRawIntrinsic>(I))
                return IGCLLVM::MemoryLocation::getForArgument(I, 0, TLI);

            // TODO: Do coarse-grained thing so far. Need better checking for
            // non load or store instructions which may read/write memory.
            return MemoryLocation();
        }

        bool hasSameSize(Type* A, Type* B) const {
            // Shortcut if A is equal to B.
            if (A == B)
                return true;
            return DL->getTypeStoreSize(A) == DL->getTypeStoreSize(B);
        }

        Value* createBitOrPointerCast(Value* V, Type* DestTy,
            IRBuilder<>& Builder) const {
            if (V->getType() == DestTy)
                return V;

            if (V->getType()->isPointerTy() && DestTy->isPointerTy()) {
                PointerType* SrcPtrTy = cast<PointerType>(V->getType());
                PointerType* DstPtrTy = cast<PointerType>(DestTy);
                if (SrcPtrTy->getPointerAddressSpace() !=
                    DstPtrTy->getPointerAddressSpace())
                    return Builder.CreateAddrSpaceCast(V, DestTy);
            }

            if (V->getType()->isPointerTy()) {
                if (DestTy->isIntegerTy()) {
                    return Builder.CreatePtrToInt(V, DestTy);
                }
                else if (DestTy->isFloatingPointTy()) {
                    uint32_t Size = (uint32_t)DestTy->getPrimitiveSizeInBits();
                    Value* Cast = Builder.CreatePtrToInt(
                        V, Builder.getIntNTy(Size));
                    return Builder.CreateBitCast(Cast, DestTy);
                }
            }

            if (DestTy->isPointerTy()) {
                if (V->getType()->isIntegerTy()) {
                    return Builder.CreateIntToPtr(V, DestTy);
                }
                else if (V->getType()->isFloatingPointTy()) {
                    uint32_t Size = (uint32_t)V->getType()->getPrimitiveSizeInBits();
                    Value* Cast = Builder.CreateBitCast(
                        V, Builder.getIntNTy(Size));
                    return Builder.CreateIntToPtr(Cast, DestTy);
                }
            }

            return Builder.CreateBitCast(V, DestTy);
        }

        bool isSafeToMergeLoad(const LoadInst* Ld,
            const SmallVectorImpl<Instruction*>& checkList) const;
        bool isSafeToMergeStores(
            const SmallVectorImpl<std::tuple<StoreInst*, int64_t, MemRefListTy::iterator>>& Stores,
            const SmallVectorImpl<Instruction*>& checkList) const;

        bool shouldSkip(const Value* Ptr) const {
            PointerType* PtrTy = cast<PointerType>(Ptr->getType());
            unsigned AS = PtrTy->getPointerAddressSpace();

            if (PtrTy->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE) {
                if (CGC->type != ShaderType::OPENCL_SHADER) {
                    // For non-OpenCL shader, skip constant buffer accesses.
                    bool DirectIndex = false;
                    unsigned BufID = 0;
                    BufferType BufTy = DecodeAS4GFXResource(AS, DirectIndex, BufID);
                    if (BufTy == CONSTANT_BUFFER &&
                        UsesTypedConstantBuffer(CGC, BufTy))
                        return true;
                }
                return false;
            }

            return false;
        }

        /// Skip irrelevant instructions.
        bool shouldSkip(const Instruction* I) const {
            if (!I->mayReadOrWriteMemory())
                return true;

            if (auto LD = dyn_cast<LoadInst>(I))
                return shouldSkip(LD->getPointerOperand());

            if (auto ST = dyn_cast<StoreInst>(I))
                return shouldSkip(ST->getPointerOperand());

            return false;
        }

        template <typename AccessInstruction>
        bool checkAlignmentBeforeMerge(const AccessInstruction* inst,
            SmallVector<std::tuple<AccessInstruction*, int64_t, MemRefListTy::iterator>, 8> & AccessIntrs,
            unsigned& NumElts)
        {
            if (inst->getAlignment() < 4 && !WI->isUniform(inst))
            {
                llvm::Type* dataType = isa<LoadInst>(inst) ? inst->getType() : inst->getOperand(0)->getType();
                unsigned scalarTypeSizeInBytes = unsigned(DL->getTypeSizeInBits(dataType->getScalarType()) / 8);

                // Need the first offset value (not necessarily zero)
                int64_t firstOffset = std::get<1>(AccessIntrs[0]);
                int64_t mergedSize = 0;
                for (auto rit = AccessIntrs.rbegin(),
                    rie = AccessIntrs.rend(); rit != rie; ++rit)
                {
                    int64_t accessSize = 0;
                    int64_t cur_offset = std::get<1>(*rit);
                    auto acessInst = std::get<0>(*rit);
                    if (isa<LoadInst>(acessInst))
                        accessSize = int64_t(DL->getTypeSizeInBits(acessInst->getType())) / 8;
                    else
                        accessSize = int64_t(DL->getTypeSizeInBits(acessInst->getOperand(0)->getType())) / 8;
                    mergedSize = cur_offset - firstOffset + accessSize;
                    // limit the size of merge when alignment < 4
                    if (mergedSize > 8)
                        AccessIntrs.pop_back();
                    else
                        break;
                }

                if (AccessIntrs.size() < 2)
                    return false;

                for (auto rit = AccessIntrs.rbegin(),
                    rie = AccessIntrs.rend(); rit != rie; ++rit)
                {
                    if (std::get<0>(*rit)->getAlignment() >= 4)
                        return false;
                }

                // Need to subtract the last offset by the first offset and add one to
                // get the new size of the vector
                NumElts = unsigned(mergedSize / scalarTypeSizeInBytes);
            }
            return true;
        }

        /// Canonicalize the calculation of 64-bit pointer by performing the
        /// following transformations to help SCEV to identify the constant offset
        /// between pointers.
        ///
        /// (sext (add.nsw LHS RHS)) => (add.nsw (sext LHS) (sext RHS))
        /// (zext (add.nuw LHS RHS)) => (add.nuw (zext LHS) (zext RHS))
        ///
        /// For SLM (and potentially private) memory, we could ignore `nsw`/`nuw`
        /// as there are only 32 significant bits.
        bool canonicalizeGEP64(Instruction*) const;

        /// Optimize the calculation of 64-bit pointer by performing the following
        /// transformations to reduce instruction strength.
        ///
        /// (add.nsw (sext LHS) (sext RHS)) => (sext (add.nsw LHS RHS))
        /// (add.nuw (zext LHS) (zext RHS)) => (zext (add.nuw LHS RHS))
        ///
        /// In fact, this's the reverse operation of 64-bit pointer
        /// canonicalization, which helps SCEV analysis but increases instruction
        /// strength on 64-bit integer operations.
        bool optimizeGEP64(Instruction*) const;
    };

    template<int M>
    struct less_tuple {
        template <typename T> bool operator()(const T& LHS, const T& RHS) const {
            return std::get<M>(LHS) < std::get<M>(RHS);
        }
    };

    // SymbolicPtr represents how a pointer is calculated from the following
    // equation:
    //
    //  Ptr := BasePtr + \sum_i Scale_i * Index_i + Offset
    //
    // where Scale_i and Offset are constants.
    //

    enum ExtensionKind {
        EK_NotExtended,
        EK_SignExt,
        EK_ZeroExt,
    };

    typedef PointerIntPair<Value*, 2, ExtensionKind> SymbolicIndex;
    struct Term {
        SymbolicIndex Idx;
        int64_t Scale;

        bool operator==(const Term& Other) const {
            return Idx == Other.Idx && Scale == Other.Scale;
        }

        bool operator!=(const Term& Other) const {
            return !operator==(Other);
        }
    };

    struct SymbolicPointer {
        const Value* BasePtr;
        int64_t Offset;
        SmallVector<Term, 8> Terms;

        // getConstantOffset - Return the constant offset between two memory
        // locations.
        bool getConstantOffset(const SymbolicPointer& Other, int64_t& Off) {
            if (!BasePtr || !Other.BasePtr)
                return true;

            if (BasePtr != Other.BasePtr &&
                (!isa<ConstantPointerNull>(BasePtr) ||
                    !isa<ConstantPointerNull>(Other.BasePtr)))
                return true;

            if (Terms.size() != Other.Terms.size())
                return true;

            // Check each term has occurrence in Other. Since, they have the same
            // number of terms, it's safe to say they are equal if all terms are
            // found in Other.
            // TODO: Replace this check with a non-quadratic one.
            for (unsigned i = 0, e = Terms.size(); i != e; ++i) {
                bool Found = false;
                for (unsigned j = 0, f = Other.Terms.size(); !Found && j != f; ++j) {
                    if (Terms[i] == Other.Terms[j])
                        Found = true;
                }
                if (!Found)
                    return true;
            }

            Off = Offset - Other.Offset;
            return false;
        }

        static Value* getLinearExpression(Value* Val, APInt& Scale, APInt& Offset,
            ExtensionKind& Extension, unsigned Depth,
            const DataLayout* DL);
        static bool decomposePointer(const Value* Ptr, SymbolicPointer& SymPtr,
            CodeGenContext* DL);

        static const unsigned MaxLookupSearchDepth = 6;
    };
}

FunctionPass* createMemOptPass(bool AllowNegativeSymPtrsForLoad, bool AllowVector8LoadStore) {
    return new MemOpt(AllowNegativeSymPtrsForLoad, AllowVector8LoadStore);
}

#define PASS_FLAG     "igc-memopt"
#define PASS_DESC     "IGC Memory Optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(MemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char MemOpt::ID = 0;

void MemOpt::buildProfitVectorLengths(Function& F) {
    ProfitVectorLengths.clear();
    if (AllowVector8LoadStore)
    {
        ProfitVectorLengths[64].push_back(4);
        ProfitVectorLengths[32].push_back(8);
    }

    // 64-bit integer
    ProfitVectorLengths[64].push_back(2);

    // 32-bit integer and Float
    ProfitVectorLengths[32].push_back(4);
    ProfitVectorLengths[32].push_back(3);
    ProfitVectorLengths[32].push_back(2);

    // 16-bit integer and Hald
    ProfitVectorLengths[16].push_back(8);
    ProfitVectorLengths[16].push_back(6);
    ProfitVectorLengths[16].push_back(4);
    ProfitVectorLengths[16].push_back(2);

    // 8-bit integer
    ProfitVectorLengths[8].push_back(16);
    ProfitVectorLengths[8].push_back(12);
    ProfitVectorLengths[8].push_back(8);
    ProfitVectorLengths[8].push_back(4);
    ProfitVectorLengths[8].push_back(2);
}

bool MemOpt::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    DL = &F.getParent()->getDataLayout();
    AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    WI = &getAnalysis<WIAnalysis>();

    CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

    if (ProfitVectorLengths.empty())
        buildProfitVectorLengths(F);

    bool Changed = false;

    for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
        // Find all instructions with memory reference. Remember the distance one
        // by one.
        MemRefListTy MemRefs;
        TrivialMemRefListTy MemRefsToOptimize;
        unsigned Distance = 0;
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI, ++Distance) {
            Instruction* I = &(*BI);
            // Skip irrelevant instructions.
            if (shouldSkip(I))
                continue;
            MemRefs.push_back(std::make_pair(I, Distance));
        }

        // Skip BB with no more than 2 loads/stores.
        if (MemRefs.size() < 2)
            continue;

        // Canonicalize 64-bit GEP to help SCEV find constant offset by
        // distributing `zext`/`sext` over safe expressions.
        for (auto& M : MemRefs)
            Changed |= canonicalizeGEP64(M.first);

        for (auto MI = MemRefs.begin(), ME = MemRefs.end(); MI != ME; ++MI) {
            Instruction* I = MI->first;

            // Skip already merged one.
            if (!I)
                continue;

            if (LoadInst * LI = dyn_cast<LoadInst>(I))
                Changed |= mergeLoad(LI, MI, MemRefs, MemRefsToOptimize);
            else if (StoreInst * SI = dyn_cast<StoreInst>(I))
                Changed |= mergeStore(SI, MI, MemRefs, MemRefsToOptimize);
        }

        // Optimize 64-bit GEP to reduce strength by factoring out `zext`/`sext`
        // over safe expressions.
        for (auto I : MemRefsToOptimize)
            Changed |= optimizeGEP64(I);
    }

    DL = nullptr;
    AA = nullptr;
    SE = nullptr;

    return Changed;
}

bool MemOpt::mergeLoad(LoadInst* LeadingLoad,
    MemRefListTy::iterator aMI, MemRefListTy& MemRefs,
    TrivialMemRefListTy& ToOpt)
{
    MemRefListTy::iterator MI = aMI;

    // Push the leading load into the list to be optimized (after
    // canonicalization.) It will be swapped with the new one if it's merged.
    ToOpt.push_back(LeadingLoad);

    if (!LeadingLoad->isSimple())
        return false;

    if (!LeadingLoad->isUnordered())
        return false;

    if (LeadingLoad->getType()->isPointerTy()) {
        unsigned int AS = LeadingLoad->getType()->getPointerAddressSpace();
        if (CGC->getRegisterPointerSizeInBits(AS) != DL->getPointerSizeInBits(AS)) {
            // we cannot coalesce pointers which have been reduced as they are
            // bigger in memory than in register
            return false;
        }
    }

    Type* LeadingLoadType = LeadingLoad->getType();
    Type* LeadingLoadScalarType = LeadingLoadType->getScalarType();
    unsigned TypeSizeInBits =
        unsigned(DL->getTypeSizeInBits(LeadingLoadScalarType));
    if (!ProfitVectorLengths.count(TypeSizeInBits))
        return false;
    SmallVector<unsigned, 8> profitVec;
    // FIXME: Enable for OCL shader only as other clients have regressions but
    // there's no way to trace down.
    bool isUniformLoad = (CGC->type == ShaderType::OPENCL_SHADER) && (WI->isUniform(LeadingLoad));
    if (isUniformLoad) {
        unsigned C = IGC_GET_FLAG_VALUE(UniformMemOpt4OW);
        C = (C == 1) ? 512 : 256;
        C /= TypeSizeInBits;
        for (; C >= 2; --C)
            profitVec.push_back(C);
    }
    else {
        SmallVector<unsigned, 4> & Vec = ProfitVectorLengths[TypeSizeInBits];
        profitVec.append(Vec.begin(), Vec.end());
    }

    unsigned LdSize = unsigned(DL->getTypeStoreSize(LeadingLoadType));
    unsigned LdScalarSize = unsigned(DL->getTypeStoreSize(LeadingLoadScalarType));

    // NumElts: num of elts if all candidates are actually merged.
    unsigned NumElts = getNumElements(LeadingLoadType);
    if (NumElts > profitVec[0])
        return false;

    // The following function "chainedSelectAndPhis" is designed to avoid going into SCEV in special circumstances
    // when the shader has a large set of chained phi nodes and selects. One of the downsides of SCEV is it is a
    // recursive approach and can cause a stack overflow when tracing back instructions.
    bool chainTooLarge = false;
    std::function<void(Instruction*, unsigned)> chainedSelectAndPhis = [&](Instruction* Inst, unsigned depth)
    {
        for (auto& operand : Inst->operands())
        {
            if (chainTooLarge)
                return;
            if (auto op_inst = dyn_cast<Instruction>(operand))
            {
                if (depth == 300) //I have hit 300 chained Phi/Select instructions time to bail
                {
                    chainTooLarge = true;
                    return;
                }
                else if (isa<PHINode>(op_inst) || isa<SelectInst>(op_inst))
                {
                    chainedSelectAndPhis(op_inst, ++depth);
                }
            }
        }
    };

    if(isa<Instruction>(LeadingLoad->getPointerOperand()))
        chainedSelectAndPhis(cast<Instruction>(LeadingLoad->getPointerOperand()), 0);

    if (chainTooLarge)
        return false;

    const SCEV* LeadingPtr = SE->getSCEV(LeadingLoad->getPointerOperand());
    if (isa<SCEVCouldNotCompute>(LeadingPtr))
        return false;

    // LoadInst, Offset, MemRefListTy::iterator, LeadingLoad's int2PtrOffset
    SmallVector<std::tuple<LoadInst*, int64_t, MemRefListTy::iterator>, 8>
        LoadsToMerge;
    LoadsToMerge.push_back(std::make_tuple(LeadingLoad, 0, MI));

    // Loads to be merged is scanned in the program order and will be merged into
    // the leading load. So two edges of that consecutive region are checked
    // against the leading load, i.e.
    // - the left-side edge, the leading load to the first load (mergable load
    //   with the minimal offset)
    // - the right-side edge, the last load (mergable load with the maximal
    //   offset) to the leading load.
    //
    // A check list is maintained from the leading load to the current
    // instruction as the list of instrucitons which may read or write memory but
    // is not able to be merged into that leading load. Since we merge
    // consecutive loads into the leading load, that check list is accumulated
    // and each consecutive load needs to check against that accumulated check
    // list.

    // Two edges of the region where loads are merged into.
    int64_t HighestOffset = LdSize;
    int64_t LowestOffset = 0;

    // List of instructions need dependency check.
    SmallVector<Instruction*, 8> CheckList;

    const unsigned Limit = IGC_GET_FLAG_VALUE(MemOptWindowSize);
    // Given the Start position of the Window is MI->second,
    // the End postion of the Window is "limit + Windows' start".
    const unsigned windowEnd = Limit + MI->second;
    auto ME = MemRefs.end();
    for (++MI; MI != ME && MI->second <= windowEnd; ++MI) {
        Instruction* NextMemRef = MI->first;
        // Skip already merged one.
        if (!NextMemRef)
            continue;

        CheckList.push_back(NextMemRef);

        LoadInst* NextLoad = dyn_cast<LoadInst>(NextMemRef);

        // Skip non-load instruction.
        if (!NextLoad)
            continue;

        // Bail out if that load is not a simple one.
        if (!NextLoad->isSimple())
            break;

        // If we get an ordered load (such as a cst_seq atomic load/store) dont
        // merge.
        if (!NextLoad->isUnordered())
            break;

        // Skip if that load is from different address spaces.
        if (NextLoad->getPointerAddressSpace() !=
            LeadingLoad->getPointerAddressSpace())
            continue;

        Type* NextLoadType = NextLoad->getType();

        // Skip if they have different sizes.
        if (!hasSameSize(NextLoadType->getScalarType(), LeadingLoadScalarType))
            continue;

        const SCEV* NextPtr = SE->getSCEV(NextLoad->getPointerOperand());
        if (isa<SCEVCouldNotCompute>(NextPtr))
            continue;

        int64_t Off = 0;
        const SCEVConstant* Offset
            = dyn_cast<SCEVConstant>(SE->getMinusSCEV(NextPtr, LeadingPtr));
        // Skip load with non-constant distance.
        if (!Offset) {

            SymbolicPointer LeadingSymPtr;
            SymbolicPointer NextSymPtr;
            if (SymbolicPointer::decomposePointer(LeadingLoad->getPointerOperand(),
                LeadingSymPtr, CGC) ||
                SymbolicPointer::decomposePointer(NextLoad->getPointerOperand(),
                    NextSymPtr, CGC) ||
                NextSymPtr.getConstantOffset(LeadingSymPtr, Off)) {
                continue;
            }
            else {
                if (!AllowNegativeSymPtrsForLoad && LeadingSymPtr.Offset < 0)
                    continue;
            }
        }
        else {
            Off = Offset->getValue()->getSExtValue();
        }

        unsigned NextLoadSize = unsigned(DL->getTypeStoreSize(NextLoadType));

        // By assuming dead load elimination always works correctly, if the load on
        // the same location is observed again, that is probably because there is
        // an instruction with global effect between them. Bail out directly.
        if (Off == 0 && LdSize == NextLoadSize)
            break;

        int64_t newHighestOffset = std::max(Off + NextLoadSize, HighestOffset);
        int64_t newLowestOffset = std::min(Off, LowestOffset);
        uint64_t newNumElts = uint64_t((newHighestOffset - newLowestOffset) /
            LdScalarSize);

        // Ensure that the total size read evenly divides the element type.
        // For example, we could have a packed struct <{i64, i32, i64}> that
        // would compute a size of 20 but, without this guard, would set
        // 'NumElts' to 2 as if the i32 wasn't present.
        if (uint64_t(newHighestOffset - newLowestOffset) % LdScalarSize != 0)
            continue;

        // Bail out if the resulting vector load is already not profitable.
        if (newNumElts > profitVec[0])
            continue;

        HighestOffset = newHighestOffset;
        LowestOffset = newLowestOffset;

        NumElts = static_cast<unsigned>(newNumElts);

        // This load is to be merged. Remove it from check list.
        CheckList.pop_back();

        // If the candidate load cannot be safely merged, merge mergable loads
        // currently found.
        if (!isSafeToMergeLoad(NextLoad, CheckList))
            break;

        LoadsToMerge.push_back(std::make_tuple(NextLoad, Off, MI));
    }

    unsigned s = LoadsToMerge.size();
    if (s < 2)
        return false;

    IGCLLVM::IRBuilder<> Builder(LeadingLoad);

    // Start to merge loads.
    IGC_ASSERT_MESSAGE(1 < NumElts, "It's expected to merge into at least 2-element vector!");

    // Sort loads based on their offsets (to the leading load) from the smallest to the largest.
    // And then try to find the profitable vector length first.
    std::sort(LoadsToMerge.begin(), LoadsToMerge.end(), less_tuple<1>());
    unsigned MaxElts = profitVec[0];
    for (unsigned k = 1, e = profitVec.size();
        NumElts != MaxElts && k != e && s != 1;) {
        // Try next legal vector length.
        while (NumElts < MaxElts && k != e) {
            MaxElts = profitVec[k++];
        }

        if (NumElts == 3 && (LeadingLoadScalarType->isIntegerTy(16) || LeadingLoadScalarType->isHalfTy())) {
            return false;
        }

        // Try remove loads to be merged.
        while (NumElts > MaxElts && s != 1) {
            Type* Ty = std::get<0>(LoadsToMerge[--s])->getType();
            NumElts -= getNumElements(Ty);
        }
    }

    if (NumElts != MaxElts || s < 2)
        return false;
    LoadsToMerge.resize(s);

    // Loads to be merged will be merged into the leading load. However, the
    // pointer from the first load (with the minimal offset) will be used as the
    // new pointer.
    LoadInst* FirstLoad = std::get<0>(LoadsToMerge.front());
    int64_t FirstOffset = std::get<1>(LoadsToMerge.front());
    IGC_ASSERT_MESSAGE(FirstOffset <= 0, "The 1st load should be either the leading load or load with smaller offset!");

    // Next we need to check alignment
    if (!checkAlignmentBeforeMerge(FirstLoad, LoadsToMerge, NumElts))
        return false;

    if (!DebugCounter::shouldExecute(MergeLoadCounter))
        return false;

    // Calculate the new pointer. If the leading load is not the first load,
    // re-calculate it from the leading pointer.
    // Alternatively, we could schedule instructions calculating the first
    // pointer ahead the leading load. But it's much simpler to re-calculate
    // it due to the constant offset.
    Value* Ptr = LeadingLoad->getPointerOperand();
    if (FirstOffset < 0) {
        // If the first load is not the leading load, re-calculate the pointer
        // from the pointer of the leading load.
        IGC_ASSERT(LdScalarSize);
        IGC_ASSERT_MESSAGE(FirstOffset % LdScalarSize == 0, "Remainder is expected to be 0!");

        Value* Idx = Builder.getInt64(FirstOffset / LdScalarSize);
        Type* Ty =
            PointerType::get(LeadingLoadScalarType,
                LeadingLoad->getPointerAddressSpace());
        Ptr = Builder.CreateBitCast(Ptr, Ty);

        GEPOperator* FirstGEP =
            dyn_cast<GEPOperator>(FirstLoad->getPointerOperand());
        if (FirstGEP && FirstGEP->isInBounds())
            Ptr = Builder.CreateInBoundsGEP(Ptr, Idx);
        else
            Ptr = Builder.CreateGEP(Ptr, Idx);
    }

    Type* NewLoadType = IGCLLVM::FixedVectorType::get(LeadingLoadScalarType, NumElts);
    Type* NewPointerType =
        PointerType::get(NewLoadType, LeadingLoad->getPointerAddressSpace());
    Value* NewPointer = Builder.CreateBitCast(Ptr, NewPointerType);
    LoadInst* NewLoad =
        Builder.CreateAlignedLoad(NewPointer, IGCLLVM::getAlign(FirstLoad->getAlignment()));
    NewLoad->setDebugLoc(LeadingLoad->getDebugLoc());

    // Unpack the load value to their uses. For original vector loads, extracting
    // and inserting is necessary to avoid tracking uses of each element in the
    // original vector load value.
    unsigned Pos = 0;
    MDNode* mdLoadInv = nullptr;
    bool allInvariantLoads = true;

    for (auto& I : LoadsToMerge) {
        Type* Ty = std::get<0>(I)->getType();
        Type* ScalarTy = Ty->getScalarType();
        IGC_ASSERT(hasSameSize(ScalarTy, LeadingLoadScalarType));

        mdLoadInv = std::get<0>(I)->getMetadata(LLVMContext::MD_invariant_load);
        if (!mdLoadInv)
        {
            allInvariantLoads = false;
        }

        Pos = unsigned((std::get<1>(I) - FirstOffset) / LdScalarSize);

        if (Ty->isVectorTy()) {
            if (Pos + cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() > NumElts) {
                // This implies we're trying to extract an element from our new load
                // with an index > the size of the new load.  If this happens,
                // we'll generate correct code if it does since we don't remove the
                // original load for this element.
                continue;
            }
            Value* Val = UndefValue::get(Ty);
            for (unsigned i = 0, e = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements(); i != e; ++i) {
                Value* Ex = Builder.CreateExtractElement(NewLoad, Builder.getInt32(Pos + i));
                Ex = createBitOrPointerCast(Ex, ScalarTy, Builder);
                Val = Builder.CreateInsertElement(Val, Ex, Builder.getInt32(i));
            }
            std::get<0>(I)->replaceAllUsesWith(Val);
        }
        else {
            if (Pos + 1 > NumElts) {
                continue;
            }
            Value* Val = Builder.CreateExtractElement(NewLoad,
                Builder.getInt32(Pos));
            Val = createBitOrPointerCast(Val, ScalarTy, Builder);
            std::get<0>(I)->replaceAllUsesWith(Val);
        }
    }

    if (allInvariantLoads)
    {
        NewLoad->setMetadata(LLVMContext::MD_invariant_load, mdLoadInv);
    }

    // Replace the list to be optimized with the new load.
    Instruction* NewOne = NewLoad;
    std::swap(ToOpt.back(), NewOne);

    for (auto& I : LoadsToMerge) {
        LoadInst* LD = cast<LoadInst>(std::get<0>(I));
        Value* Ptr = LD->getPointerOperand();
        // make sure the load was merged before actually removing it
        if (LD->use_empty()) {
            LD->eraseFromParent();
        }
        RecursivelyDeleteTriviallyDeadInstructions(Ptr);
        // Mark it as already merged.
        // Also, skip updating distance as the Window size is just a heuristic.
        std::get<2>(I)->first = nullptr;
    }

    // Add merged load into the leading load position in MemRefListTy
    // so that MemRefList is still valid and can be reused.
    aMI->first = NewOne;

    return true;
}

bool MemOpt::mergeStore(StoreInst* LeadingStore,
    MemRefListTy::iterator MI, MemRefListTy& MemRefs,
    TrivialMemRefListTy& ToOpt) {
    // Push the leading store into the list to be optimized (after
    // canonicalization.) It will be swapped with the new one if it's merged.
    ToOpt.push_back(LeadingStore);

    if (!LeadingStore->isSimple())
        return false;

    if (!LeadingStore->isUnordered())
        return false;

    if (LeadingStore->getValueOperand()->getType()->isPointerTy()) {
        unsigned AS =
            LeadingStore->getValueOperand()->getType()->getPointerAddressSpace();
        if (CGC->getRegisterPointerSizeInBits(AS) != DL->getPointerSizeInBits(AS)) {
            // we cannot coalesce pointers which have been reduced as they are
            // bigger in memory than in register
            return false;
        }
    }
    unsigned NumElts = 0;
    Value* LeadingStoreVal = LeadingStore->getValueOperand();
    Type* LeadingStoreType = LeadingStoreVal->getType();
    Type* LeadingStoreScalarType = LeadingStoreType->getScalarType();
    unsigned StSize = unsigned(DL->getTypeStoreSize(LeadingStoreType));
    unsigned typeSizeInBits =
        unsigned(DL->getTypeSizeInBits(LeadingStoreScalarType));
    if (!ProfitVectorLengths.count(typeSizeInBits))
        return false;
    SmallVector<unsigned, 4 > & profitVec = ProfitVectorLengths[typeSizeInBits];

    NumElts += getNumElements(LeadingStoreType);
    if (NumElts >= profitVec[0])
        return false;

    const SCEV* LeadingPtr = SE->getSCEV(LeadingStore->getPointerOperand());
    if (isa<SCEVCouldNotCompute>(LeadingPtr))
        return false;

    // StoreInst, Offset, MemRefListTy::iterator, LeadingStore's int2PtrOffset
    SmallVector<std::tuple<StoreInst*, int64_t, MemRefListTy::iterator>, 8>
        StoresToMerge;

    StoresToMerge.push_back(std::make_tuple(LeadingStore, 0, MI));

    // Stores to be merged are scanned in the program order from the leading store
    // but need to be merged into the tailing store. So two edges of that
    // consecutive region are checked against the leading store, i.e.
    // - the left-side edge, the leading store to the first store (mergable store
    //   with the minimal offset)
    // - the right-side edge, the last store (mergable store with the maximal
    //   offset) to the leading store.
    //
    // A check list is maintained from a previous tailing mergable store to the
    // new tailing store instruction because all those stores will be merged into
    // the new tailing store. That is, we need to check all mergable stores each
    // time a "new" tailing store is found. However, that check list needs not
    // accumulating as we already check that all stores to be merged are safe to
    // be merged into the "previous" tailing store.

    // Two edges of the region where stores are merged into.
    int64_t LastToLeading = StSize, LastToLeading4Transpose = 0;
    int64_t LeadingToFirst = 0;

    // List of instructions need dependency check.
    SmallVector<Instruction*, 8> CheckList;

    const unsigned Limit = IGC_GET_FLAG_VALUE(MemOptWindowSize);
    // Given the Start position of the Window is MI->second,
    // the End postion of the Window is "limit + Windows' start".
    const unsigned windowEnd = Limit + MI->second;
    auto ME = MemRefs.end();
    for (++MI; MI != ME && MI->second <= windowEnd; ++MI) {
        Instruction* NextMemRef = MI->first;
        // Skip already merged one.
        if (!NextMemRef)
            continue;

        CheckList.push_back(NextMemRef);

        StoreInst* NextStore = dyn_cast<StoreInst>(NextMemRef);
        // Skip non-store instruction.
        if (!NextStore)
            continue;

        // Bail out if that store is not a simple one.
        if (!NextStore->isSimple())
            break;

        // If we get an ordered store (such as a cst_seq atomic load/store) dont
        // merge.
        if (!NextStore->isUnordered())
            break;

        // Skip if that store is from different address spaces.
        if (NextStore->getPointerAddressSpace() !=
            LeadingStore->getPointerAddressSpace())
            continue;

        Value* NextStoreVal = NextStore->getValueOperand();
        Type* NextStoreType = NextStoreVal->getType();

        // Skip if they have different sizes.
        if (!hasSameSize(NextStoreType->getScalarType(), LeadingStoreScalarType))
            continue;

        const SCEV* NextPtr = SE->getSCEV(NextStore->getPointerOperand());
        if (isa<SCEVCouldNotCompute>(NextPtr))
            continue;

        int64_t Off = 0;
        const SCEVConstant* Offset
            = dyn_cast<SCEVConstant>(SE->getMinusSCEV(NextPtr, LeadingPtr));
        // Skip store with non-constant distance.
        if (!Offset) {

            SymbolicPointer LeadingSymPtr;
            SymbolicPointer NextSymPtr;
            if (SymbolicPointer::decomposePointer(
                LeadingStore->getPointerOperand(), LeadingSymPtr, CGC) ||
                SymbolicPointer::decomposePointer(NextStore->getPointerOperand(),
                    NextSymPtr, CGC) ||
                NextSymPtr.getConstantOffset(LeadingSymPtr, Off))
                continue;
        }
        else
            Off = Offset->getValue()->getSExtValue();

        // By assuming dead store elimination always works correctly, if the store
        // on the same location is observed again, that is probably because there
        // is an instruction with global effect between them. Bail out directly.
        if (Off == 0)
            break;

        unsigned NextStoreSize = unsigned(DL->getTypeStoreSize(NextStoreType));

        if ((Off > 0 && Off != LastToLeading) ||
            (Off < 0 && (-Off) != (LeadingToFirst + NextStoreSize)))
            // Check it's consecutive to the current stores to be merged.
            continue;

        NumElts += getNumElements(NextStoreType);
        // Bail out if the resulting vector store is already not profitable.
        if (NumElts > profitVec[0])
            break;

        // This store is to be merged. Remove it from check list.
        CheckList.pop_back();

        // If the candidate store cannot be safely merged, merge mergable stores
        // currently found.
        if (!isSafeToMergeStores(StoresToMerge, CheckList))
            break;

        // Clear check list.
        CheckList.clear();

        StoresToMerge.push_back(std::make_tuple(NextStore, Off, MI));

        if (Off > 0) {
            LastToLeading = Off + NextStoreSize;
            LastToLeading4Transpose = Off;
        }
        else
            LeadingToFirst = (-Off);

        // Early out if the maximal profitable vector length is reached.
        if (NumElts == profitVec[0])
            break;
    }

    unsigned s = StoresToMerge.size();
    if (s < 2)
        return false;

    // Tailing store is always the last one in the program order.
    StoreInst* TailingStore = std::get<0>(StoresToMerge.back());
    IGCLLVM::IRBuilder<> Builder(TailingStore);

    // Start to merge stores.
    NumElts = 0;
    for (auto& I : StoresToMerge) {
        Type* Ty = std::get<0>(I)->getValueOperand()->getType();
        NumElts += getNumElements(Ty);
    }

    IGC_ASSERT_MESSAGE(1 < NumElts, "It's expected to merge into at least 2-element vector!");

    // Try to find the profitable vector length first.
    unsigned MaxElts = profitVec[0];
    for (unsigned k = 1, e = profitVec.size();
        NumElts != MaxElts && k != e && s != 1;) {
        // Try next legal vector length.
        while (NumElts < MaxElts && k != e)
            MaxElts = profitVec[k++];
        // Try remove stores to be merged.
        while (NumElts > MaxElts && s != 1) {
            Type* Ty = std::get<0>(StoresToMerge[--s])->getValueOperand()->getType();
            NumElts -= getNumElements(Ty);
        }
    }

    if (NumElts != MaxElts || s < 2)
        return false;

    // Resize stores to be merged to the profitable length and sort them based on
    // their offsets to the leading store.
    StoresToMerge.resize(s);
    std::sort(StoresToMerge.begin(), StoresToMerge.end(), less_tuple<1>());

    // Stores to be merged will be merged into the tailing store. However, the
    // pointer from the first store (with the minimal offset) will be used as the
    // new pointer.
    StoreInst* FirstStore = std::get<0>(StoresToMerge.front());

    // Next we need to check alignment
    if (!checkAlignmentBeforeMerge(FirstStore, StoresToMerge, NumElts))
        return false;

    Type* NewStoreType = IGCLLVM::FixedVectorType::get(LeadingStoreScalarType, NumElts);
    Value* NewStoreVal = UndefValue::get(NewStoreType);

    // Pack the store value from their original store values. For original vector
    // store values, extracting and inserting is necessary to avoid tracking uses
    // of each element in the original vector store value.
    unsigned Pos = 0;
    for (auto& I : StoresToMerge) {
        Value* Val = std::get<0>(I)->getValueOperand();
        Type* Ty = Val->getType();
        Type* ScalarTy = Ty->getScalarType();
        IGC_ASSERT(hasSameSize(ScalarTy, LeadingStoreScalarType));

        if (Ty->isVectorTy()) {
            for (unsigned i = 0, e = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements(); i != e; ++i) {
                Value* Ex = Builder.CreateExtractElement(Val, Builder.getInt32(i));
                Ex = createBitOrPointerCast(Ex, LeadingStoreScalarType, Builder);
                NewStoreVal = Builder.CreateInsertElement(NewStoreVal, Ex,
                    Builder.getInt32(Pos++));
            }
        }
        else if (Ty->isPointerTy()) {
            if (ScalarTy != LeadingStoreScalarType) {
                if (LeadingStoreScalarType->isPointerTy()) {
                    Val =
                        Builder.CreatePointerBitCastOrAddrSpaceCast(Val,
                            LeadingStoreScalarType);
                }
                else {
                    Val =
                        Builder.CreatePtrToInt(Val,
                            Type::getIntNTy(Val->getContext(),
                            (unsigned int)LeadingStoreScalarType->getPrimitiveSizeInBits()));
                    // LeadingStoreScalarType may not be an integer type, bitcast it to
                    // the appropiate type.
                    Val = Builder.CreateBitCast(Val, LeadingStoreScalarType);
                }
            }
            NewStoreVal = Builder.CreateInsertElement(NewStoreVal, Val,
                Builder.getInt32(Pos++));
        }
        else {
            Val = createBitOrPointerCast(Val, LeadingStoreScalarType, Builder);
            NewStoreVal = Builder.CreateInsertElement(NewStoreVal, Val,
                Builder.getInt32(Pos++));
        }
    }

    if (!DebugCounter::shouldExecute(MergeStoreCounter))
        return false;

    // We don't need to recalculate the new pointer as we merge stores to the
    // tailing store, which is dominated by all mergable stores' address
    // calculations.
    Type* NewPointerType =
        PointerType::get(NewStoreType, LeadingStore->getPointerAddressSpace());
    Value* NewPointer =
        Builder.CreateBitCast(FirstStore->getPointerOperand(), NewPointerType);
    StoreInst* NewStore =
        Builder.CreateAlignedStore(NewStoreVal, NewPointer,
            IGCLLVM::getAlign(FirstStore->getAlignment()));
    NewStore->setDebugLoc(TailingStore->getDebugLoc());

    // Replace the list to be optimized with the new store.
    Instruction* NewOne = NewStore;
    std::swap(ToOpt.back(), NewOne);

    for (auto& I : StoresToMerge) {
        StoreInst* ST = cast<StoreInst>(std::get<0>(I));
        Value* Ptr = ST->getPointerOperand();
        // Stores merged in the previous iterations can get merged again, so we need
        // to update ToOpt vector to avoid null instruction in there
        ToOpt.erase(std::remove(ToOpt.begin(), ToOpt.end(), ST), ToOpt.end());
        ST->eraseFromParent();
        RecursivelyDeleteTriviallyDeadInstructions(Ptr);

        // Also, skip updating distance as the Window size is just a heuristic.
        if (std::get<2>(I)->first == TailingStore)
            // Writing NewStore to MemRefs for correct isSafeToMergeLoad working.
            // For example if MemRefs contains this sequence: S1, S2, S3, L5, L6, L7, S4, L4
            // after stores merge MemRefs contains : L5, L6, L7, S1234, L4 and loads are
            // merged to L567, final instructions instructions sequence is L567, S1234, L4.
            // Otherwise the sequence could be merged to sequence L4567, S1234 with
            // unordered L4,S4 accesses.
            std::get<2>(I)->first = NewStore;
        else {
            // Mark it as already merged.
            std::get<2>(I)->first = nullptr;
        }

    }
    return true;
}

/// isSafeToMergeLoad() - checks whether there is any alias from the specified
/// load to any one in the check list, which may write to that location.
bool MemOpt::isSafeToMergeLoad(const LoadInst* Ld,
    const SmallVectorImpl<Instruction*>& CheckList) const {
    MemoryLocation A = MemoryLocation::get(Ld);

    for (auto* I : CheckList) {
        // Skip instructions never writing to memory.
        if (!I->mayWriteToMemory())
            continue;

        MemoryLocation B = getLocation(I);

        if (!A.Ptr || !B.Ptr || AA->alias(A, B))
            return false;
    }

    return true;
}

/// isSafeToMergeStores() - checks whether there is any alias from the
/// specified store set to any one in the check list, which may read/write to
/// that location.
bool MemOpt::isSafeToMergeStores(
    const SmallVectorImpl<std::tuple<StoreInst*, int64_t, MemRefListTy::iterator> >& Stores,
    const SmallVectorImpl<Instruction*>& CheckList) const {
    // Arrange CheckList as the outer loop to favor the case where there are
    // back-to-back stores only.
    for (auto* I : CheckList) {
        if (I->getMetadata(LLVMContext::MD_invariant_load))
            continue;

        MemoryLocation A = getLocation(I);

        for (auto& S : Stores) {
            MemoryLocation B = getLocation(std::get<0>(S));

            if (!A.Ptr || !B.Ptr || AA->alias(A, B))
                return false;
        }
    }

    return true;
}

class ExtOperator : public Operator {
public:
    static inline bool classof(const Instruction* I) {
        return I->getOpcode() == Instruction::SExt ||
            I->getOpcode() == Instruction::ZExt;
    }
    static inline bool classof(const ConstantExpr* CE) {
        return CE->getOpcode() == Instruction::SExt ||
            CE->getOpcode() == Instruction::ZExt;
    }
    static inline bool classof(const Value* V) {
        return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
            (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
    }

    bool isZExt() const { return getOpcode() == Instruction::ZExt; }
    bool isSExt() const { return getOpcode() == Instruction::SExt; }

#if LLVM_VERSION_MAJOR >= 7
    ~ExtOperator() = delete;
#endif
};

class OverflowingAdditiveOperator : public Operator {
public:
    static inline bool classof(const Instruction* I) {
        return I->getOpcode() == Instruction::Add ||
            I->getOpcode() == Instruction::Sub;
    }
    static inline bool classof(const ConstantExpr* CE) {
        return CE->getOpcode() == Instruction::Add ||
            CE->getOpcode() == Instruction::Sub;
    }
    static inline bool classof(const Value* V) {
        return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
            (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
    }

    bool hasNoUnsignedWrap() const {
        return cast<OverflowingBinaryOperator>(this)->hasNoUnsignedWrap();
    }
    bool hasNoSignedWrap() const {
        return cast<OverflowingBinaryOperator>(this)->hasNoSignedWrap();
    }

#if LLVM_VERSION_MAJOR >= 7
    ~OverflowingAdditiveOperator() = delete;
#endif
};

class OrOperator : public ConcreteOperator<BinaryOperator, Instruction::Or>
{
#if LLVM_VERSION_MAJOR >= 7
    ~OrOperator() = delete;
#endif
};
class BitCastOperator : public ConcreteOperator<Operator, Instruction::BitCast>
{
#if LLVM_VERSION_MAJOR >= 7
    ~BitCastOperator() = delete;
#endif
};

bool MemOpt::canonicalizeGEP64(Instruction* I) const {
    Value* Ptr = nullptr;
    if (auto LD = dyn_cast<LoadInst>(I))
        Ptr = LD->getPointerOperand();
    else if (auto ST = dyn_cast<StoreInst>(I))
        Ptr = ST->getPointerOperand();

    // Skip non 64-bit or non GEP-based pointers if any.
    if (auto Cast = dyn_cast_or_null<llvm::BitCastOperator>(Ptr))
        Ptr = Cast->getOperand(0);
    GEPOperator* GEPOp = dyn_cast_or_null<GEPOperator>(Ptr);
    if (!GEPOp)
        return false;
    if (CGC->getRegisterPointerSizeInBits(GEPOp->getPointerAddressSpace()) != 64)
        return false;

    IRBuilder<> Builder(isa<Instruction>(GEPOp) ? cast<Instruction>(GEPOp) : I);

    bool Changed = false;
    for (auto U = GEPOp->idx_begin(), E = GEPOp->idx_end(); U != E; ++U) {
        Value* Idx = U->get();
        Type* IdxTy = Idx->getType();
        if (!IdxTy->isIntegerTy(64))
            continue;
        auto ExtOp = dyn_cast<ExtOperator>(Idx);
        if (!ExtOp)
            continue;
        auto CastOpcode = Instruction::CastOps(ExtOp->getOpcode());
        // Distribute `ext` over binary operator with corresponding `nsw`/`nuw`
        // flags.
        auto BinOp = dyn_cast<OverflowingAdditiveOperator>(ExtOp->getOperand(0));
        if (!BinOp) {
            auto OrOp = dyn_cast<OrOperator>(ExtOp->getOperand(0));
            if (!OrOp)
                continue;
            Value* LHS = OrOp->getOperand(0);
            Value* RHS = OrOp->getOperand(1);
            ConstantInt* RHSC = dyn_cast<ConstantInt>(RHS);
            if (!RHSC || !MaskedValueIsZero(LHS, RHSC->getValue(), *DL))
                continue;
            // Treat `or` as `add.nsw` or `add.nuw`.
            LHS = Builder.CreateCast(CastOpcode, LHS, IdxTy);
            RHS = Builder.CreateCast(CastOpcode, RHS, IdxTy);
            bool HasNUW = ExtOp->isZExt();
            bool HasNSW = ExtOp->isSExt();
            U->set(Builder.CreateAdd(LHS, RHS, ".or", HasNUW, HasNSW));
            RecursivelyDeleteTriviallyDeadInstructions(ExtOp);
            Changed = true;
        }
        else if ((ExtOp->isSExt() && BinOp->hasNoSignedWrap()) ||
            (ExtOp->isZExt() && BinOp->hasNoUnsignedWrap())) {
            auto BinOpcode = BinaryOperator::BinaryOps(BinOp->getOpcode());
            Value* LHS = BinOp->getOperand(0);
            Value* RHS = BinOp->getOperand(1);
            LHS = Builder.CreateCast(CastOpcode, LHS, IdxTy);
            RHS = Builder.CreateCast(CastOpcode, RHS, IdxTy);
            auto BO = Builder.CreateBinOp(BinOpcode, LHS, RHS);
            // BO can be a constant if both sides are constants
            if (auto BOP = dyn_cast<BinaryOperator>(BO)) {
                if (BinOp->hasNoUnsignedWrap())
                    BOP->setHasNoUnsignedWrap();
                if (BinOp->hasNoSignedWrap())
                    BOP->setHasNoSignedWrap();
            }
            U->set(BO);
            RecursivelyDeleteTriviallyDeadInstructions(ExtOp);
            Changed = true;
        }
    }

    return Changed;
}

bool MemOpt::optimizeGEP64(Instruction* I) const {
    Value* Ptr = nullptr;
    if (auto LD = dyn_cast<LoadInst>(I))
        Ptr = LD->getPointerOperand();
    else if (auto ST = dyn_cast<StoreInst>(I))
        Ptr = ST->getPointerOperand();

    // Skip non 64-bit or non GEP-based pointers if any.
    if (auto Cast = dyn_cast_or_null<llvm::BitCastOperator>(Ptr))
        Ptr = Cast->getOperand(0);
    GEPOperator* GEPOp = dyn_cast_or_null<GEPOperator>(Ptr);
    if (!GEPOp)
        return false;
    if (CGC->getRegisterPointerSizeInBits(GEPOp->getPointerAddressSpace()) != 64)
        return false;

    IRBuilder<> Builder(isa<Instruction>(GEPOp) ? cast<Instruction>(GEPOp) : I);

    bool Changed = false;
    for (auto U = GEPOp->idx_begin(), E = GEPOp->idx_end(); U != E; ++U) {
        Value* Idx = U->get();
        Type* IdxTy = Idx->getType();
        if (!IdxTy->isIntegerTy(64))
            continue;
        // Factor out `ext` through binary operator with corresponding `nsw`/`nuw`
        // flags.
        auto BinOp = dyn_cast<OverflowingAdditiveOperator>(Idx);
        if (!BinOp)
            continue;
        auto BinOpcode = BinaryOperator::BinaryOps(BinOp->getOpcode());
        Value* LHS = BinOp->getOperand(0);
        Value* RHS = BinOp->getOperand(1);
        auto ExtOp0 = dyn_cast<ExtOperator>(LHS);
        if (!ExtOp0)
            continue;
        auto CastOpcode = Instruction::CastOps(ExtOp0->getOpcode());
        auto ExtOp1 = dyn_cast<ExtOperator>(RHS);
        if (ExtOp1 && ExtOp0->getOpcode() == ExtOp1->getOpcode() &&
            ((ExtOp0->isZExt() && BinOp->hasNoUnsignedWrap()) ||
            (ExtOp0->isSExt() && BinOp->hasNoSignedWrap()))) {
            LHS = ExtOp0->getOperand(0);
            RHS = ExtOp1->getOperand(0);
            unsigned LHSBitWidth = LHS->getType()->getIntegerBitWidth();
            unsigned RHSBitWidth = RHS->getType()->getIntegerBitWidth();
            unsigned BitWidth = std::max(LHSBitWidth, RHSBitWidth);
            // Either LHS or RHS may have smaller integer, extend them before
            // creating `binop` over them.
            if (LHSBitWidth < BitWidth) {
                Type* Ty = Builder.getIntNTy(BitWidth);
                LHS = Builder.CreateCast(CastOpcode, LHS, Ty);
            }
            if (RHSBitWidth < BitWidth) {
                Type* Ty = Builder.getIntNTy(BitWidth);
                RHS = Builder.CreateCast(CastOpcode, RHS, Ty);
            }
        }
        else if (isa<ConstantInt>(RHS)) {
            LHS = ExtOp0->getOperand(0);
            unsigned BitWidth = LHS->getType()->getIntegerBitWidth();
            APInt Val = cast<ConstantInt>(RHS)->getValue();
            if (!((ExtOp0->isZExt() && Val.isIntN(BitWidth)) ||
                (ExtOp0->isSExt() && Val.isSignedIntN(BitWidth))))
                continue;
            if (!((ExtOp0->isZExt() && BinOp->hasNoUnsignedWrap()) ||
                (ExtOp0->isSExt() && BinOp->hasNoSignedWrap())))
                continue;
            LHS = ExtOp0->getOperand(0);
            RHS = Builder.CreateTrunc(RHS, LHS->getType());
        }
        else
            continue;
        auto BO = cast<BinaryOperator>(Builder.CreateBinOp(BinOpcode, LHS, RHS));
        if (BinOp->hasNoUnsignedWrap())
            BO->setHasNoUnsignedWrap();
        if (BinOp->hasNoSignedWrap())
            BO->setHasNoSignedWrap();
        U->set(Builder.CreateCast(CastOpcode, BO, IdxTy));
        RecursivelyDeleteTriviallyDeadInstructions(BinOp);
        Changed = true;
    }

    return Changed;
}

Value*
SymbolicPointer::getLinearExpression(Value* V, APInt& Scale, APInt& Offset,
    ExtensionKind& Extension, unsigned Depth,
    const DataLayout* DL) {
    IGC_ASSERT(nullptr != V);
    IGC_ASSERT(nullptr != V->getType());
    IGC_ASSERT_MESSAGE(V->getType()->isIntegerTy(), "Not an integer value");

    // Limit our recursion depth.
    if (Depth == 16) {
        Scale = 1;
        Offset = 0;
        return V;
    }

    if (BinaryOperator * BOp = dyn_cast<BinaryOperator>(V)) {
        if (ConstantInt * RHSC = dyn_cast<ConstantInt>(BOp->getOperand(1))) {
            switch (BOp->getOpcode()) {
            default: break;
            case Instruction::Or:
                // X|C == X+C if all the bits in C are unset in X.  Otherwise we can't
                // analyze it.
                if (!MaskedValueIsZero(BOp->getOperand(0), RHSC->getValue(), *DL))
                    break;
                // FALL THROUGH.
            case Instruction::Add:
                V = getLinearExpression(BOp->getOperand(0), Scale, Offset, Extension,
                    Depth + 1, DL);
                Offset += RHSC->getValue();
                return V;
            case Instruction::Mul:
                V = getLinearExpression(BOp->getOperand(0), Scale, Offset, Extension,
                    Depth + 1, DL);
                Offset *= RHSC->getValue();
                Scale *= RHSC->getValue();
                return V;
            case Instruction::Shl:
                V = getLinearExpression(BOp->getOperand(0), Scale, Offset, Extension,
                    Depth + 1, DL);
                Offset <<= unsigned(RHSC->getValue().getLimitedValue());
                Scale <<= unsigned(RHSC->getValue().getLimitedValue());
                return V;
            }
        }
    }

    // Since GEP indices are sign extended anyway, we don't care about the high
    // bits of a sign or zero extended value - just scales and offsets.  The
    // extensions have to be consistent though.
    if ((isa<SExtInst>(V) && Extension != EK_ZeroExt) ||
        (isa<ZExtInst>(V) && Extension != EK_SignExt)) {
        Value* CastOp = cast<CastInst>(V)->getOperand(0);
        unsigned OldWidth = Scale.getBitWidth();
        unsigned SmallWidth = (unsigned int)CastOp->getType()->getPrimitiveSizeInBits();
        Scale = Scale.trunc(SmallWidth);
        Offset = Offset.trunc(SmallWidth);
        Extension = isa<SExtInst>(V) ? EK_SignExt : EK_ZeroExt;

        Value* Result = getLinearExpression(CastOp, Scale, Offset, Extension,
            Depth + 1, DL);
        Scale = Scale.zext(OldWidth);
        if (Extension == EK_SignExt)
            Offset = Offset.sext(OldWidth);
        else
            Offset = Offset.zext(OldWidth);

        return Result;
    }

    Scale = 1;
    Offset = 0;
    return V;
}

class IntToPtrOperator :
    public ConcreteOperator<Operator, Instruction::IntToPtr>
{
#if LLVM_VERSION_MAJOR >= 7
    ~IntToPtrOperator() = delete;
#endif
};

bool
SymbolicPointer::decomposePointer(const Value* Ptr, SymbolicPointer& SymPtr,
    CodeGenContext* pContext) {
    unsigned MaxLookup = MaxLookupSearchDepth;
    const DataLayout* DL = &pContext->getModule()->getDataLayout();
    SymPtr.Offset = 0;
    SymPtr.BasePtr = nullptr;
    do {
        const Operator* Op = dyn_cast<Operator>(Ptr);
        if (!Op) {
            // The only non-operator case we can handle are GlobalAliases.
            if (const GlobalAlias * GA = dyn_cast<GlobalAlias>(Ptr)) {
                if (!GA->isInterposable()) {
                    Ptr = GA->getAliasee();
                    continue;
                }
            }
            SymPtr.BasePtr = Ptr;
            return false;
        }

        if (Op->getOpcode() == Instruction::BitCast || Op->getOpcode() == Instruction::AddrSpaceCast) {
            Ptr = Op->getOperand(0);
            continue;
        }

        const GEPOperator* GEPOp = dyn_cast<GEPOperator>(Op);
        if (!GEPOp) {
            // If it's not a GEP, hand it off to SimplifyInstruction to see if it
            // can come up with something. This matches what GetUnderlyingObject does.
            if (const Instruction * I = dyn_cast<Instruction>(Ptr))
                // TODO: Get a DominatorTree and use it here.
                if (const Value * Simplified =
                    SimplifyInstruction(const_cast<Instruction*>(I), *DL)) {
                    Ptr = Simplified;
                    continue;
                }

            // IntToPtr is treated like gep(i8* 0, Src).
            // TODO: Unify the common handling of IntToPtr & GEP into a single
            // routine.
            if (const IntToPtrOperator * I2POp = dyn_cast<IntToPtrOperator>(Op)) {
                PointerType* PtrTy = cast<PointerType>(I2POp->getType());
                unsigned int ptrSize = pContext->getRegisterPointerSizeInBits(PtrTy->getAddressSpace());
                Value* Src = I2POp->getOperand(0);
                Value* BasePtr = ConstantPointerNull::get(PtrTy);

                // Constant pointer.
                if (ConstantInt * CI = dyn_cast<ConstantInt>(Src)) {
                    SymPtr.Offset += CI->getSExtValue();
                    SymPtr.BasePtr = BasePtr;
                    return false;
                }

                // Treat that like (inttoptr (add (base offset)))
                if (AddOperator * Add = dyn_cast<AddOperator>(Src)) {
                    // Note that we always assume LHS as the base and RHS as the offset.
                    // That's why GEP is invented in LLVM IR as the pointer arithmetic in
                    // C is always in form of (base + offset). By designating the base
                    // pointer, we won't run into the case where both operands are
                    // symmetric in `add` instruction.
                    if (!isa<ConstantInt>(Add->getOperand(1))) {
                        BasePtr = Add->getOperand(0);
                        Src = Add->getOperand(1);
                    }
                }

                uint64_t Scale = 1;
                ExtensionKind Extension = EK_NotExtended;
                unsigned Width = Src->getType()->getIntegerBitWidth();
                if (ptrSize > Width)
                    Extension = EK_SignExt;

                APInt IndexScale(Width, 0), IndexOffset(Width, 0);
                Src = getLinearExpression(Src, IndexScale, IndexOffset, Extension,
                    0U, DL);
                SymPtr.Offset += IndexOffset.getSExtValue() * Scale;
                Scale *= IndexScale.getSExtValue();

                SymbolicIndex Idx(Src, Extension);

                // If we already had an occurrence of this index variable, merge this
                // scale into it.  For example, we want to handle:
                //   A[x][x] -> x*16 + x*4 -> x*20
                // This also ensures that 'x' only appears in the index list once.
                for (unsigned i = 0, e = SymPtr.Terms.size(); i != e; ++i) {
                    if (SymPtr.Terms[i].Idx == Idx) {
                        Scale += SymPtr.Terms[i].Scale;
                        SymPtr.Terms.erase(SymPtr.Terms.begin() + i);
                        break;
                    }
                }

                // Make sure that we have a scale that makes sense for this target's
                // pointer size.
                if (unsigned ShiftBits = 64 - ptrSize) {
                    Scale <<= ShiftBits;
                    Scale = (int64_t)Scale >> ShiftBits;
                }

                if (Scale) {
                    Term Entry = { Idx, int64_t(Scale) };
                    SymPtr.Terms.push_back(Entry);
                }

                Ptr = BasePtr;
            }

            SymPtr.BasePtr = Ptr;
            return false;
        }

        // Don't attempt to analyze GEPs over unsized objects.
        if (!GEPOp->getOperand(0)->getType()->getPointerElementType()->isSized()) {
            SymPtr.BasePtr = Ptr;
            return false;
        }

        // If we are lacking DataLayout information, we can't compute the offets of
        // elements computed by GEPs.  However, we can handle bitcast equivalent
        // GEPs.
        if (!DL) {
            if (!GEPOp->hasAllZeroIndices()) {
                SymPtr.BasePtr = Ptr;
                return false;
            }
            Ptr = GEPOp->getOperand(0);
            continue;
        }

        unsigned int ptrSize =
            pContext->getRegisterPointerSizeInBits(GEPOp->getPointerAddressSpace());
        // Walk the indices of the GEP, accumulating them into BaseOff/VarIndices.
        gep_type_iterator GTI = gep_type_begin(GEPOp);
        for (User::const_op_iterator I = GEPOp->op_begin() + 1,
            E = GEPOp->op_end(); I != E; ++I, ++GTI) {
            Value* Index = *I;
            // Compute the (potentially symbolic) offset in bytes for this index.
            if (StructType * STy = GTI.getStructTypeOrNull()) {
                // For a struct, add the member offset.
                unsigned FieldNo = unsigned(cast<ConstantInt>(Index)->getZExtValue());
                if (FieldNo == 0) continue;

                SymPtr.Offset += DL->getStructLayout(STy)->getElementOffset(FieldNo);
                continue;
            }

            // For an array/pointer, add the element offset, explicitly scaled.
            if (ConstantInt * CIdx = dyn_cast<ConstantInt>(Index)) {
                if (CIdx->isZero()) continue;
                SymPtr.Offset += DL->getTypeAllocSize(GTI.getIndexedType()) * CIdx->getSExtValue();
                continue;
            }

            // In some cases the GEP might have indices that don't directly have a baseoffset
            // we need to dig deeper to find these
            std::vector<Value*> terms = { Index };
            if (BinaryOperator * BOp = dyn_cast<BinaryOperator>(Index))
            {
                if (!(dyn_cast<ConstantInt>(BOp->getOperand(1))) &&
                    BOp->getOpcode() == Instruction::Add)
                {
                    terms.clear();
                    terms.push_back(BOp->getOperand(0));
                    terms.push_back(BOp->getOperand(1));
                }
            }

            for (auto Ind : terms)
            {
                uint64_t Scale = DL->getTypeAllocSize(GTI.getIndexedType());
                ExtensionKind Extension = EK_NotExtended;

                // If the integer type is smaller than the pointer size, it is implicitly
                // sign extended to pointer size.
                unsigned Width = Index->getType()->getIntegerBitWidth();
                if (ptrSize > Width)
                    Extension = EK_SignExt;

                // Use getLinearExpression to decompose the index into a C1*V+C2 form.
                APInt IndexScale(Width, 0), IndexOffset(Width, 0);
                Value* new_Ind = getLinearExpression(Ind, IndexScale, IndexOffset, Extension,
                    0U, DL);

                // The GEP index scale ("Scale") scales C1*V+C2, yielding (C1*V+C2)*Scale.
                // This gives us an aggregate computation of (C1*Scale)*V + C2*Scale.
                SymPtr.Offset += IndexOffset.getSExtValue() * Scale;
                Scale *= IndexScale.getSExtValue();

                SymbolicIndex Idx(new_Ind, Extension);

                // If we already had an occurrence of this index variable, merge this
                // scale into it.  For example, we want to handle:
                //   A[x][x] -> x*16 + x*4 -> x*20
                // This also ensures that 'x' only appears in the index list once.
                for (unsigned i = 0, e = SymPtr.Terms.size(); i != e; ++i) {
                    if (SymPtr.Terms[i].Idx == Idx) {
                        Scale += SymPtr.Terms[i].Scale;
                        SymPtr.Terms.erase(SymPtr.Terms.begin() + i);
                        break;
                    }
                }

                // Make sure that we have a scale that makes sense for this target's
                // pointer size.
                if (unsigned ShiftBits = 64 - ptrSize) {
                    Scale <<= ShiftBits;
                    Scale = (int64_t)Scale >> ShiftBits;
                }

                if (Scale) {
                    Term Entry = { Idx, int64_t(Scale) };
                    SymPtr.Terms.push_back(Entry);
                }
            }
        }

        // Analyze the base pointer next.
        Ptr = GEPOp->getOperand(0);
    } while (--MaxLookup);

    return true;
}
