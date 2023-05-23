/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/ADT/STLExtras.h>
#include <llvmWrapper/Analysis/InstructionSimplify.h>
#include <llvmWrapper/Analysis/MemoryLocation.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include "llvm/Analysis/AliasSetTracker.h"
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/GlobalAlias.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvmWrapper/Support/Alignment.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/DebugCounter.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/CommandLine.h"
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/SLMConstProp.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/CISACodeGen/MemOpt.h"
#include "Probe/Assertion.h"
#include <DebugInfo/DwarfDebug.cpp>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

static cl::opt<bool> EnableRemoveRedBlockreads(
    "remove-red-blockreads", cl::init(false), cl::Hidden,
    cl::desc("Enable removal of redundant blockread instructions."));

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
        bool removeRedBlockRead(GenIntrinsicInst* LeadingLoad, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt, unsigned& SimdSize);

        Optional<unsigned> chainedSelectAndPhis(Instruction* Inst, unsigned depth,
            llvm::DenseMap<Instruction*, unsigned> &depthTracking);

        void removeVectorBlockRead(Instruction* BlockReadToOptimize, Instruction* BlockReadToRemove,
            Value* SgId, llvm::IRBuilder<>& Builder, unsigned& sg_size);
        void removeScalarBlockRead(Instruction* BlockReadToOptimize, Instruction* BlockReadToRemove,
            Value* SgId, llvm::IRBuilder<>& Builder);
        Value* getShuffle(Value* ShflId, Instruction* BlockReadToOptimize,
            Value* SgId, llvm::IRBuilder<>& Builder, unsigned& ToOptSize);

        unsigned getNumElements(Type* Ty) const {
            return Ty->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;
        }

        Type* getVectorElementType(Type* Ty) const {
            return isa<VectorType>(Ty) ? cast<VectorType>(Ty)->getElementType() : Ty;
        }

        MemoryLocation getLocation(Instruction* I) const {

            if (LoadInst * LI = dyn_cast<LoadInst>(I))
                return MemoryLocation::get(LI);

            if (StoreInst * SI = dyn_cast<StoreInst>(I))
                return MemoryLocation::get(SI);

            if (isa<LdRawIntrinsic>(I))
                return llvm::MemoryLocation::getForArgument(llvm::cast<llvm::CallInst>(I), 0, TLI);

            if (isa<StoreRawIntrinsic>(I))
                return llvm::MemoryLocation::getForArgument(llvm::cast<llvm::CallInst>(I), 0, TLI);

            if (GenIntrinsicInst* GInst = dyn_cast<GenIntrinsicInst>(I)) {
                if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead) {
                    return llvm::MemoryLocation::getForArgument(llvm::cast<llvm::CallInst>(I), 0, TLI);
                }
            }

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

            if (auto GInst = dyn_cast<GenIntrinsicInst>(I)) {
                if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead) {
                    return shouldSkip(I->getOperand(0));
                }
            }

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
            if (IGCLLVM::getAlignmentValue(inst) < 4 && !WI->isUniform(inst))
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
                    if (IGCLLVM::getAlignmentValue(std::get<0>(*rit)) >= 4)
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

FunctionPass* IGC::createMemOptPass(bool AllowNegativeSymPtrsForLoad, bool AllowVector8LoadStore) {
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

    IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MDU->getFunctionsInfoItem(&F);
    unsigned SimdSize = funcInfoMD->getSubGroupSize()->getSIMD_size();

    for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
        // Find all instructions with memory reference. Remember the distance one
        // by one.
        MemRefListTy MemRefs;
        TrivialMemRefListTy MemRefsToOptimize;
        unsigned Distance = 0;
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI, ++Distance) {
            Instruction* I = &(*BI);

            // Make sure we don't count debug info intrinsincs
            // This is required to keep debug and non-debug optimizations identical
            if (isDbgIntrinsic(I)) {
                Distance--;
                continue;
            }

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
            else if (EnableRemoveRedBlockreads) {
                if (GenIntrinsicInst* GInst = dyn_cast<GenIntrinsicInst>(I)) {
                    if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead) {
                        Changed |= removeRedBlockRead(GInst, MI, MemRefs, MemRefsToOptimize, SimdSize);
                    }
                }
            }
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

//This function removes redundant blockread instructions
//if they read from addresses with the same base.
//It replaces redundant blockread with a set of shuffle instructions.
//
//For example,
//
//before:
// %0 = inttoptr i64 %i64input to i32 addrspace(1)*
// %1 = inttoptr i64 %i64input to i8 addrspace(1)*
// %2 = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* %0)
// %3 = call i8 @llvm.genx.GenISA.simdBlockRead.i8.p1i8(i8 addrspace(1)* %1)
// store i32 %2, i32 addrspace(1)* %i32addr, align 4
// store i8 %3, i8 addrspace(1)* %i8addr, align 1
//
//after:
// %0 = inttoptr i64 %i64input to i32 addrspace(1)*
// %1 = inttoptr i64 %i64input to i8 addrspace(1)*
// %2 = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* %0)
// %3 = call i16 @llvm.genx.GenISA.simdLaneId()
// %4 = zext i16 %3 to i32
// %5 = lshr i32 %4, 2
// %6 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %5, i32 0)
// %7 = and i32 %4, 3
// %8 = mul i32 %7, 8
// %9 = lshr i32 %6, %8
// %10 = trunc i32 %9 to i8
// store i32 %2, i32 addrspace(1)* %i32addr, align 4
// store i8 %10, i8 addrspace(1)* %i8addr, align 1
bool MemOpt::removeRedBlockRead(GenIntrinsicInst* LeadingBlockRead,
    MemRefListTy::iterator aMI, MemRefListTy& MemRefs,
    TrivialMemRefListTy& ToOpt, unsigned& sg_size)
{
    MemRefListTy::iterator MI = aMI;
    const unsigned Limit = IGC_GET_FLAG_VALUE(MemOptWindowSize);
    const unsigned windowEnd = Limit + MI->second;
    auto ME = MemRefs.end();

    MemoryLocation LeadingBlockReadMemLoc = getLocation(cast<Instruction>(LeadingBlockRead));
    Type* LeadingBlockReadType = LeadingBlockRead->getType();
    Value* LeadingBlockReadBase = LeadingBlockRead->getOperand(0)->stripPointerCasts();

    Instruction* BlockReadToOptimize = LeadingBlockRead;
    MemRefListTy::iterator MIToOpt = aMI;

    llvm::SmallVector<std::tuple<Instruction*, MemRefListTy::iterator>, 8> BlockReadToRemove;
    uint64_t MaxBlockReadSize = LeadingBlockReadType->getPrimitiveSizeInBits();

    //Go through MemRefs to collect blockreads that can be removed.
    for (++MI; MI != ME && MI->second <= windowEnd; ++MI) {
        Instruction* NextMemRef = MI->first;
        if (!NextMemRef) {
            continue;
        }

        if (GenIntrinsicInst* GInst = dyn_cast<GenIntrinsicInst>(NextMemRef)) {
            if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead) {
                Type* GInstType = GInst->getType();
                uint64_t NextSize = GInstType->getPrimitiveSizeInBits();
                Value* NextBlockReadBase = NextMemRef->getOperand(0)->stripPointerCasts();

                if (isa<IntToPtrInst>(LeadingBlockReadBase) && isa<IntToPtrInst>(NextBlockReadBase)) {
                    LeadingBlockReadBase = cast<IntToPtrInst>(LeadingBlockReadBase)->getOperand(0);
                    NextBlockReadBase = cast<IntToPtrInst>(NextBlockReadBase)->getOperand(0);
                }

                if (LeadingBlockReadBase == NextBlockReadBase) {
                    if (NextSize > MaxBlockReadSize) {
                        BlockReadToRemove.push_back(std::make_tuple(BlockReadToOptimize, MIToOpt));
                        MaxBlockReadSize = NextSize;
                        BlockReadToOptimize = NextMemRef;
                        MIToOpt = MI;
                    }
                    else {
                        BlockReadToRemove.push_back(std::make_tuple(NextMemRef, MI));
                    }
                }
            }
        }
        else if (NextMemRef->mayWriteToMemory()) {
            MemoryLocation WriteInstrMemLoc = getLocation(NextMemRef);
            if (!WriteInstrMemLoc.Ptr || !LeadingBlockReadMemLoc.Ptr || AA->alias(WriteInstrMemLoc, LeadingBlockReadMemLoc)) {
                break;
            }
        }
    }

    if (BlockReadToRemove.size() == 0) {
        return false;
    }

    IRBuilder<> Builder(LeadingBlockRead);

    //Raise the blockread, which we will not remove, in place of the leading blockread.
    if (BlockReadToOptimize != LeadingBlockRead) {
        Type* ArgType = BlockReadToOptimize->getOperand(0)->getType();
        BlockReadToOptimize->moveBefore(LeadingBlockRead);

        Builder.SetInsertPoint(BlockReadToOptimize);
        Value* BitCast = Builder.CreateBitCast(LeadingBlockRead->getOperand(0), ArgType);

        BlockReadToOptimize->setOperand(0, BitCast);
        aMI->first = BlockReadToOptimize;
    }

    Builder.SetInsertPoint(BlockReadToOptimize->getNextNonDebugInstruction());
    Value* subgroupLocalInvocationId = nullptr;

    //Go through the collected blockreads to replace them with shuffles
    for (auto ITuple : BlockReadToRemove) {
        Instruction* I = std::get<0>(ITuple);

        if (BlockReadToOptimize != I) {
            if (!subgroupLocalInvocationId) {
                Function* simdLaneIdIntrinsic = GenISAIntrinsic::getDeclaration(
                    BlockReadToOptimize->getModule(),
                    GenISAIntrinsic::GenISA_simdLaneId);

                subgroupLocalInvocationId = Builder.CreateZExtOrTrunc(
                    Builder.CreateCall(simdLaneIdIntrinsic),
                    Builder.getInt32Ty());
            }

            //Case when one of blockreads is vector
            if (I->getType()->isVectorTy() || BlockReadToOptimize->getType()->isVectorTy()) {
                MemOpt::removeVectorBlockRead(BlockReadToOptimize, I, subgroupLocalInvocationId, Builder, sg_size);
            } //Case when blockreads are scalars
            else {
                MemOpt::removeScalarBlockRead(BlockReadToOptimize, I, subgroupLocalInvocationId, Builder);
            }

            std::get<1>(ITuple)->first = nullptr;
            I->eraseFromParent();
            Builder.SetInsertPoint(BlockReadToOptimize->getNextNonDebugInstruction());
        }
    }
    aMI->first = BlockReadToOptimize;
    return true;
}

//Removes redundant blockread if both blockreads are scalar.
void MemOpt::removeScalarBlockRead(Instruction* BlockReadToOptimize,
    Instruction* BlockReadToRemove, Value* SgId,
    llvm::IRBuilder<>& Builder)
{
    Type* BlockReadToOptType = BlockReadToOptimize->getType();
    unsigned ToOptSize = (unsigned)(BlockReadToOptType->getPrimitiveSizeInBits());
    Type* BlockReadToRemoveType = BlockReadToRemove->getType();

    int rat = (int)(ToOptSize / (2 * BlockReadToRemoveType->getPrimitiveSizeInBits()));
    Value* LShr = Builder.CreateLShr(SgId, Builder.getInt32(rat));
    Value* shuffle = getShuffle(LShr, BlockReadToOptimize, SgId, Builder, ToOptSize);

    Value* and_instr = Builder.CreateAnd(SgId, Builder.getInt32(rat * 2 - 1));
    Value* shift = Builder.CreateMul(and_instr, Builder.getInt32((int)(BlockReadToRemoveType->getPrimitiveSizeInBits())));
    Value* extr_elem = Builder.CreateLShr(shuffle, Builder.CreateZExtOrTrunc(shift, BlockReadToOptType));
    Value* TypeConvInstr = Builder.CreateTrunc(extr_elem, cast<Type>(BlockReadToRemoveType));

    BlockReadToRemove->replaceAllUsesWith(TypeConvInstr);
}

//Removes redundant blockreads if one of the pair is a vector blockread.
void MemOpt::removeVectorBlockRead(Instruction* BlockReadToOptimize,
    Instruction* BlockReadToRemove, Value* SgId,
    llvm::IRBuilder<>& Builder, unsigned& sg_size)
{
    Type* BlockReadToOptType = BlockReadToOptimize->getType();
    Type* BlockReadToRemoveType = BlockReadToRemove->getType();
    unsigned ToOptSize = BlockReadToOptType->getScalarSizeInBits();

    if (BlockReadToOptType->getScalarSizeInBits() < BlockReadToRemoveType->getScalarSizeInBits()) {
        unsigned step = BlockReadToRemoveType->getScalarSizeInBits() / BlockReadToOptType->getScalarSizeInBits();

        unsigned ToRemoveNumElem = getNumElements(BlockReadToRemoveType);
        Type* ElemType = getVectorElementType(BlockReadToRemoveType);

        Function* shufflefn = GenISAIntrinsic::getDeclaration(
            BlockReadToOptimize->getModule(),
            GenISAIntrinsic::GenISA_WaveShuffleIndex,
            getVectorElementType(BlockReadToOptType));

        unsigned LimitElem = step * ToRemoveNumElem;
        std::vector<Instruction*> ExtractElemInstrVector;
        //Extracting elements from BlockReadToOptimize to use them in shuffles
        for (unsigned i = 0; i < LimitElem; i++) {
            Instruction* ExtrElemInstr = cast<Instruction>(Builder.CreateExtractElement(BlockReadToOptimize, Builder.getInt32(i)));
            ExtractElemInstrVector.push_back(ExtrElemInstr);
        }

        Type* NewType = VectorType::get(getVectorElementType(BlockReadToOptType), LimitElem * sg_size, false);
        std::vector<Instruction*> ShuffleInstrVector;
        Value* CollectedData = nullptr;

        //Generating set of shuffles and collecting them in vector
        for (unsigned index = 0; index < LimitElem; index++) {
            for (unsigned id = 0; id < sg_size; id++) {
                SmallVector<Value*, 3> Args;
                Args.push_back(cast<Value>(ExtractElemInstrVector[index]));
                Args.push_back(Builder.getInt32(id));
                Args.push_back(Builder.getInt32(0));
                if (index == 0 && id == 0) {
                    Value* ShuffleInstr = Builder.CreateCall(shufflefn, Args);
                    Value* InsertIndex = cast<Value>(Builder.getInt64(0));
                    CollectedData = Builder.CreateInsertElement(UndefValue::get(NewType), ShuffleInstr, InsertIndex);
                }
                else {
                    Value* ShuffleInstr = Builder.CreateCall(shufflefn, Args);
                    Value* InsertIndex = cast<Value>(Builder.getInt64(id + index * sg_size));
                    CollectedData = Builder.CreateInsertElement(CollectedData, ShuffleInstr, InsertIndex);
                }
            }
        }

        Value* offset = Builder.CreateMul(SgId, Builder.getInt32(step));
        Type* TypeVectForBitCast = VectorType::get(getVectorElementType(BlockReadToOptType), step, false);
        Value* ResVect = nullptr;

        //Getting the result of a blockread that has been deleted
        for (unsigned k = 0; k < ToRemoveNumElem; k++) {
            Value* VectForBitCast = nullptr;
            Value* Index = Builder.CreateAdd(offset, Builder.getInt32(k * sg_size * step));
            for (unsigned i = 0; i < step; i++) {
                Value* AddInstr = Builder.CreateAdd(Index, Builder.getInt32(i));
                Value* extr_elem = cast<Instruction>(Builder.CreateExtractElement(CollectedData, AddInstr));

                if (i == 0) {
                    VectForBitCast = Builder.CreateInsertElement(UndefValue::get(TypeVectForBitCast), extr_elem, cast<Value>(Builder.getInt64(0)));
                }
                else {
                    VectForBitCast = Builder.CreateInsertElement(VectForBitCast, extr_elem, cast<Value>(Builder.getInt64(i)));
                }
            }

            Value* BitCastInstr = Builder.CreateBitCast(VectForBitCast, ElemType);

            if (BlockReadToRemoveType->isVectorTy()) {
                if (k == 0) {
                    ResVect = Builder.CreateInsertElement(UndefValue::get(BlockReadToRemoveType), BitCastInstr, cast<Value>(Builder.getInt64(0)));
                }
                else {
                    ResVect = Builder.CreateInsertElement(ResVect, BitCastInstr, cast<Value>(Builder.getInt64(k)));
                }
            }
            else {
                ResVect = BitCastInstr;
            }
        }

        BlockReadToRemove->replaceAllUsesWith(ResVect);
    }
    else if (BlockReadToOptType->getScalarSizeInBits() > BlockReadToRemoveType->getScalarSizeInBits()) {
        unsigned step = BlockReadToOptType->getScalarSizeInBits() / BlockReadToRemoveType->getScalarSizeInBits();

        unsigned ToRemoveNumElem = getNumElements(BlockReadToRemoveType);
        Type* IElemType = getVectorElementType(BlockReadToRemoveType);

        unsigned tmp = step;
        int pw = 0;
        while (tmp >>= 1) ++pw;

        Value* SgidDivStep = Builder.CreateLShr(SgId, Builder.getInt32(pw));
        Value* SimdDivStep = Builder.CreateLShr(Builder.getInt32(sg_size), Builder.getInt32(pw));

        unsigned LimitElem = ToRemoveNumElem / step;
        if (ToRemoveNumElem % step) {
            LimitElem++;
        }

        std::vector<Instruction*> ExtractElemInstrVector;
        //Extracting elements from BlockReadToOptimize to use them in shuffles
        for (unsigned i = 0; i < LimitElem; i++) {
            if (BlockReadToOptType->isVectorTy()) {
                Instruction* ExtrElemInstr = cast<Instruction>(Builder.CreateExtractElement(BlockReadToOptimize, Builder.getInt32(i)));
                ExtractElemInstrVector.push_back(ExtrElemInstr);
            }
            else {
                ExtractElemInstrVector.push_back(BlockReadToOptimize);
            }
        }

        std::vector<Instruction*> ShuffleInstrVector;

        unsigned LimitId = step;
        if (ToRemoveNumElem < step) {
            LimitId = ToRemoveNumElem;
        }
        //Generating set of shuffles and collecting them in vector
        for (unsigned k = 0; k < LimitElem; k++) {
            for (unsigned i = 0; i < LimitId; i++) {
                Value* SgIdShfl = Builder.CreateAdd(SgidDivStep, Builder.CreateMul(SimdDivStep, Builder.getInt32(i)));
                Value* shuffle = getShuffle(SgIdShfl, ExtractElemInstrVector[k], SgId, Builder, ToOptSize);
                ShuffleInstrVector.push_back(cast<Instruction>(shuffle));
            }
        }

        unsigned ShufflesNum = LimitElem * LimitId;

        Type* TypeVectForBitCast = VectorType::get(IElemType, step, false);
        Value* ResVect = nullptr;
        //Getting the result of a blockread that has been deleted
        for (unsigned ShfflCnt = 0; ShfflCnt < ShufflesNum; ShfflCnt++) {
            Value* VectBitcast = Builder.CreateBitCast(ShuffleInstrVector[ShfflCnt], TypeVectForBitCast);
            Value* Index = Builder.CreateAnd(SgId, Builder.CreateSub(Builder.getInt32(step), Builder.getInt32(1)));
            Value* Elem = Builder.CreateExtractElement(VectBitcast, Index);

            if (BlockReadToRemoveType->isVectorTy()) {
                if (ShfflCnt == 0) {
                    ResVect = Builder.CreateInsertElement(UndefValue::get(BlockReadToRemoveType), Elem, Builder.getInt32(0));
                }
                else {
                    ResVect = Builder.CreateInsertElement(ResVect, Elem, Builder.getInt32(ShfflCnt));
                }
            }
            else {
                ResVect = Elem;
            }
        }

        BlockReadToRemove->replaceAllUsesWith(ResVect);
    }
    else {
        BlockReadToRemove->replaceAllUsesWith(BlockReadToOptimize);
    }
}

//This function return shuffle instruction(if BlockedToOptimize size < 64)
//or it returns value which is concatenation of two shuffle instructions.
Value* MemOpt::getShuffle(Value* ShflId,
    Instruction* BlockReadToOptimize,
    Value* SgId, llvm::IRBuilder<>&Builder,
    unsigned& ToOptSize)
{
    Value* shuffle = nullptr;
    Type* BlockReadToOptType = BlockReadToOptimize->getType();

    if (ToOptSize < 64) {
        Type* shufflefntype = getVectorElementType(BlockReadToOptType);

        Function* shufflefn = GenISAIntrinsic::getDeclaration(
            BlockReadToOptimize->getModule(),
            GenISAIntrinsic::GenISA_WaveShuffleIndex,
            shufflefntype);

        SmallVector<Value*, 3> Args;
        Args.push_back(cast<Value>(BlockReadToOptimize));
        Args.push_back(ShflId);
        Args.push_back(Builder.getInt32(0));

        shuffle = Builder.CreateCall(shufflefn, Args);
    }
    else if (ToOptSize == 64) {
        Type* NewType = VectorType::get(Builder.getInt32Ty(), 2, false);

        Instruction* BitCastInstr = cast<Instruction>(Builder.CreateBitCast(BlockReadToOptimize, cast<Type>(NewType)));

        Instruction* ExtractElemInstr0 = cast<Instruction>(Builder.CreateExtractElement(BitCastInstr, Builder.getInt32(0)));
        Instruction* ExtractElemInstr1 = cast<Instruction>(Builder.CreateExtractElement(BitCastInstr, Builder.getInt32(1)));

        Function* shufflefn = GenISAIntrinsic::getDeclaration(
            BlockReadToOptimize->getModule(),
            GenISAIntrinsic::GenISA_WaveShuffleIndex,
            Builder.getInt32Ty());

        SmallVector<Value*, 3> Args0;
        Args0.push_back(cast<Value>(ExtractElemInstr0));
        Args0.push_back(ShflId);
        Args0.push_back(Builder.getInt32(0));

        Value* shuffle0 = Builder.CreateCall(shufflefn, Args0);

        SmallVector<Value*, 3> Args1;
        Args1.push_back(cast<Value>(ExtractElemInstr1));
        Args1.push_back(ShflId);
        Args1.push_back(Builder.getInt32(0));

        Value* shuffle1 = Builder.CreateCall(shufflefn, Args1);

        Value* ins_elem0 = Builder.CreateInsertElement(UndefValue::get(NewType), shuffle0, cast<Value>(Builder.getInt64(0)));
        Value* ins_elem1 = Builder.CreateInsertElement(ins_elem0, shuffle1, Builder.getInt64(1));

        shuffle = Builder.CreateBitCast(ins_elem1, BlockReadToOptType);
    }

    return shuffle;
}


// The following function "chainedSelectAndPhis" is designed to avoid going into SCEV in special circumstances
// when the shader has a large set of chained phi nodes and selects. One of the downsides of SCEV is it is a
// recursive approach and can cause a stack overflow when tracing back instructions.
Optional<unsigned> MemOpt::chainedSelectAndPhis(Instruction* Inst , unsigned depth,
    llvm::DenseMap<Instruction*, unsigned> &depthTracking)
{
    //Max depth set to 300
    if (depth >= 300)
    {
        return None;
    }

    if (auto I = depthTracking.find(Inst); I != depthTracking.end())
    {
        if ((depth + I->second) >= 300)
            return None;

        return I->second;
    }

    unsigned MaxRemDepth = 0;
    for (auto& operand : Inst->operands())
    {
        if (auto* op_inst = dyn_cast<Instruction>(operand))
        {
            if (isa<PHINode>(op_inst) || isa<SelectInst>(op_inst))
            {
                Optional<unsigned> RemDepth = chainedSelectAndPhis(op_inst, depth + 1, depthTracking);
                if (!RemDepth)
                    return None;
                MaxRemDepth = std::max(MaxRemDepth, *RemDepth + 1);
            }
        }
    }

    depthTracking[Inst] = MaxRemDepth;
    return MaxRemDepth;
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

    if (auto* Ptr = dyn_cast<Instruction>(LeadingLoad->getPointerOperand()))
    {
        llvm::DenseMap<Instruction*, unsigned> depthTracking;
        if (!chainedSelectAndPhis(Ptr, 0, depthTracking))
        {
            return false;
        }
    }

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
        Builder.CreateAlignedLoad(NewPointer, IGCLLVM::getAlign(*FirstLoad));
    NewLoad->setDebugLoc(LeadingLoad->getDebugLoc());

    // Unpack the load value to their uses. For original vector loads, extracting
    // and inserting is necessary to avoid tracking uses of each element in the
    // original vector load value.
    unsigned Pos = 0;
    MDNode* mdLoadInv = nullptr;
    bool allInvariantLoads = true;

    MDNode* nonTempMD = LeadingLoad->getMetadata("nontemporal");

    for (auto& I : LoadsToMerge) {
        Type* Ty = std::get<0>(I)->getType();
        Type* ScalarTy = Ty->getScalarType();
        IGC_ASSERT(hasSameSize(ScalarTy, LeadingLoadScalarType));

        mdLoadInv = std::get<0>(I)->getMetadata(LLVMContext::MD_invariant_load);
        if (!mdLoadInv)
        {
            allInvariantLoads = false;
        }

        nonTempMD = MDNode::concatenate(std::get<0>(I)->getMetadata("nontemporal"), nonTempMD);

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

    // Transfer !nontemporal metadata to the new load
    if (nonTempMD)
    {
        NewLoad->setMetadata("nontemporal", nonTempMD);
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

    MDNode* NonTempMD = TailingStore->getMetadata("nontemporal");

    // Pack the store value from their original store values. For original vector
    // store values, extracting and inserting is necessary to avoid tracking uses
    // of each element in the original vector store value.
    unsigned Pos = 0;
    for (auto& I : StoresToMerge) {
        Value* Val = std::get<0>(I)->getValueOperand();
        Type* Ty = Val->getType();
        Type* ScalarTy = Ty->getScalarType();
        IGC_ASSERT(hasSameSize(ScalarTy, LeadingStoreScalarType));

        NonTempMD = MDNode::concatenate(std::get<0>(I)->getMetadata("nontemporal"), NonTempMD);

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
            IGCLLVM::getAlign(*FirstStore));
    NewStore->setDebugLoc(TailingStore->getDebugLoc());

    // Transfer !nontemporal metadata to the new store
    if (NonTempMD)
        NewStore->setMetadata("nontemporal", NonTempMD);

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

    ~ExtOperator() = delete;
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

    ~OverflowingAdditiveOperator() = delete;
};

class OrOperator : public ConcreteOperator<BinaryOperator, Instruction::Or>
{
    ~OrOperator() = delete;
};
class BitCastOperator : public ConcreteOperator<Operator, Instruction::BitCast>
{
    ~BitCastOperator() = delete;
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

    bool Changed = false;
    for (auto U = GEPOp->idx_begin(), E = GEPOp->idx_end(); U != E; ++U) {
        Value* Idx = U->get();
        Type* IdxTy = Idx->getType();
        IRBuilder<> Builder(isa<Instruction>(GEPOp) ? cast<Instruction>(GEPOp) : I);

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
            Value* BinOpVal = cast<Value>(BinOp);
            // We want to check if we should create a separate BinOp instruction for this gep instruction.
            bool NeedToChangeBinOp = BinOpVal->hasOneUse();
            if (NeedToChangeBinOp)
                Builder.SetInsertPoint(cast<Instruction>(ExtOp));

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

            if (NeedToChangeBinOp)
                ExtOp->replaceAllUsesWith(BO);

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
    ~IntToPtrOperator() = delete;
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
            // If it's not a GEP, hand it off to simplifyInstruction to see if it
            // can come up with something. This matches what GetUnderlyingObject does.
            if (const Instruction * I = dyn_cast<Instruction>(Ptr))
                // TODO: Get a DominatorTree and use it here.
                if (const Value * Simplified =
                    IGCLLVM::simplifyInstruction(const_cast<Instruction*>(I), *DL)) {
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
        if (!IGCLLVM::getNonOpaquePtrEltTy(GEPOp->getOperand(0)->getType())->isSized()) {
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

namespace {
    enum class AddressModel {
        BTS, A32, SLM, A64
    };

    struct LdStInfo {
        // Load (or load intrinsic) for loadCombine().
        // store (or store intrinsic) for storeCombine.
        Instruction* Inst;
        // Byte offset of 'Inst'->getPointerOperand() relative to
        // that of the leading load/store inst.
        int64_t      ByteOffset;

        LdStInfo(Instruction* aI, int64_t aBO) : Inst(aI), ByteOffset(aBO) {}
        Type* getLdStType() const;
        uint32_t getAlignment() const;
        AddressModel getAddressModel(CodeGenContext* Ctx) const;
    };

    typedef SmallVector<LdStInfo, 8> InstAndOffsetPairs;

    // A bundle: a group of consecutive loads or a group of consecutive stores.
    // Each bundle maps to a single GEN load or store.
    struct BundleInfo {
        InstAndOffsetPairs LoadStores;
        int bundle_eltBytes;    // 1, 4, 8
        int bundle_numElts;
        // Valid for bundle_eltBytes = 1. It indicates whether D64 or
        // D32(including D8U32 and D16U32) is used as data size.
        bool useD64;
    };

    typedef SmallVector<uint32_t, 8> BundleSize_t;

    // BundleConfig:
    //    To tell what vector size is legit. It may need GEN platform as input.
    class BundleConfig {
    public:
        BundleConfig(int ByteAlign, bool Uniform,
            const AddressModel AddrModel, CodeGenContext* Ctx) {
            if (Ctx->platform.LSCEnabled()) {
                if (ByteAlign >= 8) {
                    m_currVecSizeVar =
                        Uniform ? &m_d64VecSizes_u : &m_d64VecSizes;
                    m_eltSizeInBytes = 8;
                }
                else if (ByteAlign == 4) {
                    m_currVecSizeVar =
                        Uniform ? &m_d32VecSizes_u : &m_d32VecSizes;
                    m_eltSizeInBytes = 4;
                }
                else {
                    m_currVecSizeVar =
                        Uniform ? &m_d8VecSizes_u : &m_d8VecSizes;
                    m_eltSizeInBytes = 1;
                }
            }
            else {
                if (Uniform) {
                    // Limit to simd8 (reasonable?), scattered read/write
                    if (ByteAlign >= 4) {
                        m_vecSizeVar = { 2, 4, 8 };
                        m_eltSizeInBytes = (ByteAlign >= 8 ? 8 : 4);
                    }
                    else {
                        m_vecSizeVar = { 2, 4, 8, 16, 32 };
                        m_eltSizeInBytes = 1;
                    }
                }
                else {
                    if (ByteAlign >= 8 && AddrModel == AddressModel::A64) {
                        m_vecSizeVar = { 2, 4 };  // QW scattered read/write
                        m_eltSizeInBytes = 8;
                    }
                    else if (ByteAlign < 4) {
                        m_vecSizeVar = { 2, 4 };  // Byte scattered read/write
                        m_eltSizeInBytes = 1;
                    }
                    else {
                        m_vecSizeVar = { 2, 3, 4 };  // untyped read/write
                        m_eltSizeInBytes = 4;
                    }
                }
                m_currVecSizeVar = &m_vecSizeVar;
            }
            m_currIndex = 0;
        }

        uint32_t getAndUpdateVecSizeInBytes(uint32_t Bytes) {
            const BundleSize_t& Var = *m_currVecSizeVar;
            int sz = (int)Var.size();
            int i;
            uint32_t total = 0;
            for (i = m_currIndex; i < sz; ++i) {
                uint32_t vecsize = Var[i];
                total = vecsize * m_eltSizeInBytes;
                if (total >= Bytes) {
                    break;
                }
            }

            if (i >= sz) {
                m_currIndex = 0;
                return 0;
            }
            // update index
            m_currIndex = i;
            return total;
        }

        uint32_t getMaxVecSizeInBytes() const {
            const BundleSize_t& Var = *m_currVecSizeVar;
            return Var.back() * m_eltSizeInBytes;
        }

        uint32_t getCurrVecSize() const {
            const BundleSize_t& Var = *m_currVecSizeVar;
            IGC_ASSERT(0 <= m_currIndex && (int)Var.size() > m_currIndex);
            return Var[m_currIndex];
        }

        //
        // Legal vector sizes for load/store
        //
        // 64bit aligned, 64bit element (D64)
        static const BundleSize_t m_d64VecSizes;
        // 32bit aligned, 32bit element (D32)
        static const BundleSize_t m_d32VecSizes;
        // 8bit aligned, 8bit element (D16U32, D32, D64)
        static const BundleSize_t m_d8VecSizes;

        //
        // uniform
        //
        // 64bit aligned, 64bit element (D64)
        static const BundleSize_t m_d64VecSizes_u;
        // 32bit aligned, 32bit element (D32)
        static const BundleSize_t m_d32VecSizes_u;
        //  8bit aligned, 8bit element (D16U32, D32, D64)
        static const BundleSize_t m_d8VecSizes_u;

    private:
        // Special vecSize, used for pre-LSC platform.
        BundleSize_t        m_vecSizeVar;

        const BundleSize_t* m_currVecSizeVar;
        uint32_t            m_eltSizeInBytes;
        int                 m_currIndex;
    };

    //
    // Load and Store combine pass:
    //   combines consecutive loads/stores into a single load/store.
    // It is based on a simple integer symbolic evaluation.
    //    1. It can combine loads/stores of different element size; and
    //    2. It does clean up to remove dead code after combining, thus
    //       no need to run DCE after this.
    class LdStCombine : public FunctionPass
    {
        const DataLayout* m_DL;
        AliasAnalysis* m_AA;
        WIAnalysis* m_WI;
        CodeGenContext* m_CGC;
        Function* m_F;

    public:
        static char ID;

        LdStCombine()
            : FunctionPass(ID)
            , m_DL(nullptr), m_AA(nullptr), m_WI(nullptr), m_CGC(nullptr)
            , m_F(nullptr), m_hasLoadCombined(false), m_hasStoreCombined(false)
        {
            initializeLdStCombinePass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.addRequired<CodeGenContextWrapper>();

            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<AAResultsWrapperPass>();
            AU.addRequired<TargetLibraryInfoWrapperPass>();
            AU.addRequired<WIAnalysis>();
        }

        StringRef getPassName() const override { return "LdStCombine"; }

        void releaseMemory() override { clear(); }

    private:
        SymbolicEvaluation m_symEval;
        bool m_hasLoadCombined;
        bool m_hasStoreCombined;

        // All insts that have been combined and can be deleted.
        SmallVector<Instruction*, 16> m_combinedInsts;

        //
        // Temporary reused for each BB.
        //
        // Inst order within a BB.
        DenseMap<const Instruction*, int> m_instOrder;

        // a bundle : a group of loads or a group of store.
        // Each bundle will be combined into a single load or single store.
        std::list<BundleInfo> m_bundles;

        DenseMap<const Instruction*, int> m_visited;

        void init(BasicBlock* BB) {
            m_visited.clear();
            m_instOrder.clear();
        }
        void setInstOrder(BasicBlock* BB);
        void setVisited(Instruction* I) { m_visited[I] = 1; }
        bool isVisited(const Instruction* I) const {
            return m_visited.count(I) > 0;
        }

        // store combining top function
        void combineStores(Function& F);

        void createBundles(BasicBlock* BB, InstAndOffsetPairs& Stores);

        // Actually combining stores.
        void createCombinedStores(Function& F);

        // If V is vector, get all its elements (may generate extractElement
        //   insts; if V is not vector, just V itself.
        void getOrCreateElements(Value* V,
            SmallVector<Value*, 16>& EltV, Instruction* InsertBefore);
        // If V is constant or created only by IEI with const idx, return true
        bool isSimpleVector(Value* V) const;

        void mergeConstElements(
            SmallVector<Value*, 4>& EltVals,  uint32_t MaxMergeBytes);

        // GatherCopy: copy vals to Dst
        //   It's a packed copy, thus size of vals = size of DstTy.
        Value* gatherCopy(Type* DstTy,
          int NElts,
          SmallVector<Value*, 16>& Vals,
          Instruction* InsertBefore);

        // Helper functions
        bool hasAlias(AliasSetTracker& AST, MemoryLocation& MemLoc);

        // Symbolic difference of two address values
        // return value:
        //   true  if A1 - A0 = constant in bytes, and return that constant as BO.
        //   false if A1 - A0 != constant. BO will be undefined.
        // BO: byte offset
        bool getDiffIfConstant(Value* A0, Value* A1, int64_t& ConstBO);

        // If I0 and I1 are load/store insts, compare their address operands and return
        // the constant difference if it is; return false otherwise.
        bool getAddressDiffIfConstant(Instruction* I0, Instruction* I1, int64_t& ConstBO);

        void eraseDeadInsts();

        void clear()
        {
            m_symEval.clear();
            m_hasLoadCombined = false;
            m_hasStoreCombined = false;
            m_visited.clear();
            m_instOrder.clear();
            m_bundles.clear();
        }
    };
}

const BundleSize_t BundleConfig::m_d64VecSizes = { 2,3,4};
const BundleSize_t BundleConfig::m_d32VecSizes = { 2,3,4,8 };
const BundleSize_t BundleConfig::m_d8VecSizes = { 2,4,8 };
const BundleSize_t BundleConfig::m_d64VecSizes_u = { 2,3,4,8,16,32,64 };
const BundleSize_t BundleConfig::m_d32VecSizes_u = { 2,3,4,8,16,32,64 };
const BundleSize_t BundleConfig::m_d8VecSizes_u = { 2,4,8,16,32 };

FunctionPass* IGC::createLdStCombinePass() {
    return new LdStCombine();
}

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG     "igc-ldstcombine"
#define PASS_DESC     "IGC load/store combine"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LdStCombine, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(LdStCombine, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char LdStCombine::ID = 0;

Type* LdStInfo::getLdStType() const
{
    if (LoadInst* LI = dyn_cast<LoadInst>(Inst))
    {
        return LI->getType();
    }
    else if (StoreInst* SI = dyn_cast<StoreInst>(Inst))
    {
        return SI->getValueOperand()->getType();
    }
    IGC_ASSERT(false);
    return nullptr;
}

uint32_t LdStInfo::getAlignment() const
{
    if (LoadInst* LI = dyn_cast<LoadInst>(Inst))
    {
        return (uint32_t)IGCLLVM::getAlignmentValue(LI);
    }
    else if (StoreInst* SI = dyn_cast<StoreInst>(Inst))
    {
        return (uint32_t)IGCLLVM::getAlignmentValue(SI);
    }
    IGC_ASSERT(false);
    return 1;
}

AddressModel LdStInfo::getAddressModel(CodeGenContext* Ctx) const
{
    Value* Ptr = nullptr;
    if (LoadInst* LI = dyn_cast<LoadInst>(Inst))
    {
        Ptr = LI->getPointerOperand();
    }
    else if (StoreInst* SI = dyn_cast<StoreInst>(Inst)) {
        Ptr = SI->getPointerOperand();
    }
    else {
        IGC_ASSERT_MESSAGE(false, "Not support yet");
    }

    PointerType* PTy = dyn_cast<PointerType>(Ptr->getType());
    IGC_ASSERT(PTy);
    const uint32_t AS = PTy->getPointerAddressSpace();
    uint bufferIndex = 0;
    bool directIndexing = false;

    BufferType BTy = DecodeAS4GFXResource(AS, directIndexing, bufferIndex);

    AddressModel addrModel;
    if (BTy == SLM) {
        addrModel = AddressModel::SLM;
    }
    else if (BTy == ESURFACE_STATELESS) {
        const bool isA32 = !IGC::isA64Ptr(PTy, Ctx);
        addrModel = (isA32 ? AddressModel::A32 : AddressModel::A64);
    }
    else {
        addrModel = AddressModel::BTS;
    }
    return addrModel;
}

bool LdStCombine::hasAlias(AliasSetTracker& AST, MemoryLocation& MemLoc)
{
    for (auto& AS : AST)
    {
        AliasResult aresult = AS.aliasesPointer(MemLoc.Ptr, MemLoc.Size, MemLoc.AATags, AST.getAliasAnalysis());
        if (aresult != AliasResult::NoAlias) {
            return true;
        }
    }
    return false;
}

void LdStCombine::setInstOrder(BasicBlock* BB)
{
    // Lazy initialization. Skip if it's been initialized.
    if (m_instOrder.size() > 0)
        return;

    int i = -1;
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
    {
        Instruction* I = &*II;
        m_instOrder[I] = (++i);
    }
}

bool LdStCombine::getDiffIfConstant(Value* A0, Value* A1, int64_t& constBO)
{
    // Using a simple integer symbolic expression (polynomial) as SCEV
    // does not work well for this.
    SymExpr* S0 = m_symEval.getSymExpr(A0);
    SymExpr* S1 = m_symEval.getSymExpr(A1);
    return m_symEval.isOffByConstant(S0, S1, constBO);
}

bool LdStCombine::getAddressDiffIfConstant(Instruction* I0, Instruction* I1, int64_t& BO)
{
    if (isa<LoadInst>(I0) && isa<LoadInst>(I1))
    {
        LoadInst* LI0 = static_cast<LoadInst*>(I0);
        LoadInst* LI1 = static_cast<LoadInst*>(I1);
        return getDiffIfConstant(LI0->getPointerOperand(), LI1->getPointerOperand(), BO);
    }
    if (isa<StoreInst>(I0) && isa<StoreInst>(I1))
    {
        StoreInst* SI0 = static_cast<StoreInst*>(I0);
        StoreInst* SI1 = static_cast<StoreInst*>(I1);
        return getDiffIfConstant(SI0->getPointerOperand(), SI1->getPointerOperand(), BO);
    }
    return false;
}

bool LdStCombine::runOnFunction(Function& F)
{
    m_CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // Plan for doing it for LSC message only; But if EnableLdStCombine = 2,
    // do it for legacy message as well.
    if (F.hasOptNone() ||
        (IGC_GET_FLAG_VALUE(EnableLdStCombine) == 1 &&
         !m_CGC->platform.LSCEnabled()))
        return false;

    m_DL = &F.getParent()->getDataLayout();
    m_AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    m_WI = &getAnalysis<WIAnalysis>();
    if (IGC_IS_FLAG_ENABLED(DisableUniformAnalysis)) {
        m_WI = nullptr;
    }
    else {
        m_WI = &getAnalysis<WIAnalysis>();
    }
    m_F = &F;

    // Initialize symbolic evaluation
    m_symEval.setDataLayout(m_DL);

    combineStores(F);

    bool changed = (m_hasLoadCombined || m_hasStoreCombined);

    clear();

    return changed;
}

// getElments():
//   Return all valid elements of a given vector V.
//   It may need to insert ExtractElementInst.
void LdStCombine::getOrCreateElements(
    Value* V, SmallVector<Value*, 16>& EltV, Instruction* InsertBefore)
{
    Type* Ty = V->getType();
    VectorType* VTy = dyn_cast<VectorType>(Ty);
    if (!VTy) {
        EltV.push_back(V);
        return;
    }

    IGCLLVM::FixedVectorType* tVTy = cast<IGCLLVM::FixedVectorType>(VTy);
    const int32_t nelts = (int32_t)tVTy->getNumElements();
    EltV.resize(nelts, UndefValue::get(tVTy->getElementType()));
    Value* ChainVal = V;
    while (!isa<Constant>(ChainVal)) {
        InsertElementInst* IEI = dyn_cast<InsertElementInst>(ChainVal);
        if (!IEI || !isa<ConstantInt>(IEI->getOperand(2))) {
            break;
        }
        ConstantInt* CInt = dyn_cast<ConstantInt>(IEI->getOperand(2));
        uint32_t idx = (uint32_t)CInt->getZExtValue();

        // Make sure the last IEI will be recorded if an element is
        // inserted multiple times.
        if (isa<UndefValue>(EltV[idx])) {
            EltV[idx] = IEI->getOperand(1);
        }

        ChainVal = IEI->getOperand(0);
    }

    if (isa<UndefValue>(ChainVal)) {
        // All valid elements known. For example,
        //   v0 = extelt undef, s0, 0
        //   v1 = extelt v0,    s1, 1
        //   v2 = extelt v1,    s2, 2
        //   V  = extelt v2,    s3, 3
        // EltV[] = { s0, s1, s2, s3 }
        return;
    }

    if (ConstantVector* CV = dyn_cast<ConstantVector>(ChainVal)) {
        // Get valid elements from the final constant vector, for example.
        //   v0 = extelt {1, 2, 3, 4}, s0, 0
        //   V  = extelt v0,    s2, 2
        // EltV[] = { s0, 2, s2, 4}
        for (int i = 0; i < nelts; ++i) {
            Value* v = CV->getOperand(i);
            if (isa<UndefValue>(EltV[i]) && !isa<UndefValue>(v)) {
                EltV[i] = v;
            }
        }
        return;
    }

    // Not all elements known, get remaining unknown elements
    //   LV = load
    //   v0 = extelt LV,    s0, 0
    //   V  = extelt v0,    s2, 1
    // EltV[] = {s0, s1, 'extElt LV, 2', 'extElt LV, 3' }
    IRBuilder<> builder(InsertBefore);
    for (int i = 0; i < nelts; ++i) {
        if (isa<UndefValue>(EltV[i])) {
            Value* v = builder.CreateExtractElement(V, i);
            EltV[i] = v;
        }
    }
}

void LdStCombine::combineStores(Function& F)
{
    // All store candidates with addr = common-base + const-offset
    //   All stores have the same common-base and different const-offset.
    InstAndOffsetPairs Stores;

    // Keep store candidates for checking alias to see if those
    // stores can be moved to the place of the last store.
    AliasSetTracker AST(*m_AA);

    auto isStoreCandidate = [&](Instruction* I)
    {
        if (StoreInst* SI = dyn_cast<StoreInst>(I))
        {
            // Only original, not-yet-visited store can be candidates.
            bool isOrigSt = (m_instOrder.size() == 0 ||
                             m_instOrder.count(I) > 0);
            return (isOrigSt && !isVisited(I) &&
                SI->isSimple() && SI->isUnordered());
        }
        return false;
    };

    // If all Stores can move down across I, return true;
    // otherwise, return false.
    auto canCombineStoresAcross = [&](Instruction* I)
    {
        // Can't combine for non-debug fence like instructions
        if (I->isFenceLike() && !IsDebugInst(I))
            return false;

        if (isa<LoadInst>(I) ||
            isa<StoreInst>(I) ||
            I->mayReadOrWriteMemory()) {
            MemoryLocation memloc = MemoryLocation::get(I);
            return !hasAlias(AST, memloc);
        }
        return true;
    };

    m_hasStoreCombined = false;
    for (auto& BB : F)
    {
        init(&BB);

        auto IE = BB.end();
        for (auto II = BB.begin(); II != IE; ++II)
        {
            Instruction* base = &*II;
            if (!isStoreCandidate(base))
            {
                continue;
            }

            Stores.push_back(LdStInfo(base, 0));
            AST.clear();
            AST.add(base);
            for (auto JI = std::next(II); JI != IE; ++JI) {
                Instruction* I = &*JI;
                if (canCombineStoresAcross(I)) {
                    int64_t offset;
                    if (isStoreCandidate(I) &&
                        getAddressDiffIfConstant(base, I, offset)) {
                        Stores.push_back(LdStInfo(I, offset));
                        AST.add(I);
                    }
                    continue;
                }

                // Cannot add more stores, create bundles now.
                //   Note: For now, each store is considered once. For example,
                //     store a
                //     store b
                //     store c        // alias to store a
                //     store d
                //   As 'store c' aliases to 'store a', candidate 'Stores' stop
                //   growing at 'store c', giving the first set {a, b}. Even
                //   though {a, b} cannot combined, 'store b' will not be
                //   reconsidered for a potential merging of {b, c, d}.
                //   This can be changed if needed.
                createBundles(&BB, Stores);
            }

            // last one
            createBundles(&BB, Stores);
        }

        // Actually combining them.
        createCombinedStores(F);
    }
}

void LdStCombine::createBundles(BasicBlock* BB, InstAndOffsetPairs& Stores)
{
    //
    // SelectD32OrD64:
    // a utility class to select whether to use data element D32 or D64 when
    // the alignment is 8 bytes or 1 bytes. Not used when alignment is 4.
    // (Here, data element refers to data element in load/store messages.)
    //   0) Don't use D64 if i64 is not nativaly supported (no Q mov).
    //   1) use D32 if any store in the bundle has byte-element access (either
    //      scalar or element type of a vector), and the store is non-uniform,
    //      as D64 might require stride=8, which is not legit, to merge byte
    //      elements; or
    //   2) use D64 if there are more D64 elements than D32 elements (thus
    //      less move instructions); or
    //   3) use D64 if VecSize = 3 and there is at least one D64 store
    //      (note that V3D64 has no equivalent D32 messages).
    //   4) otherwise, either D32 or D64 based on uniformity and size
    //      (details in useD64()).
    //
    class SelectD32OrD64 {
        uint32_t LastNumD64, LastNumD32;
        uint32_t currNumD64, currNumD32;
        // If byte element is present, save its index.
        int32_t lastStoreIdxWithByteElt;
        // Whether this store is uniform or not.
        const bool isUniform;
        // Do tracking only for 8byte-aligned D64 or 1byte-aligned
        const bool doTracking;

        const CodeGenContext* Ctx;
        const DataLayout* DL;
    public:
        SelectD32OrD64(const CodeGenContext* aCtx,
            const DataLayout* aDL, bool aUniform, uint32_t aAlign)
            : Ctx(aCtx), DL(aDL)
            , LastNumD64(0), LastNumD32(0)
            , currNumD64(0), currNumD32(0)
            , lastStoreIdxWithByteElt(-1)
            , isUniform(aUniform)
            , doTracking(aAlign == 8 || aAlign == 1)
        {}

        // LSI:   the store to be tracked.
        // LSIIdx: this store's index in the bundle.
        // ByteOffset: starting offset of this LSI in the coalesced var.
        void track(const LdStInfo* LSI, int32_t LSIIdx, uint32_t ByteOffset) {
            if (!doTracking)
                return;

            Type* Ty = LSI->getLdStType();
            Type* eTy = Ty->getScalarType();
            if (!(eTy->isIntegerTy() || eTy->isFloatingPointTy()))
                return;

            uint32_t eBytes = (uint32_t)DL->getTypeStoreSize(eTy);
            uint32_t nElts = 1;
            if (VectorType* VTy = dyn_cast<VectorType>(Ty)) {
                auto fVTy = cast<IGCLLVM::FixedVectorType>(VTy);
                nElts = (uint32_t)fVTy->getNumElements();
            }
            // If ByteOffset is odd, need to use byte mov to pack coalesced var
            // (packed struct). so, treat this the same as byte-element access.
            if (eBytes == 1 || (ByteOffset & 1) != 0) {
                lastStoreIdxWithByteElt = LSIIdx;
            }
            else if (eBytes == 4) {
                currNumD32 += nElts;
            }
            else if (eBytes == 8) {
                currNumD64 += nElts;
            }
        }

        void save(uint32_t Align) {
            if (!doTracking)
                return;
            LastNumD32 = currNumD32;
            LastNumD64 = currNumD64;
        }

        bool useD64(uint32_t VecEltBytes, uint32_t VecSizeInElt) {
            if (!doTracking)
                return false;

            if (VecEltBytes == 1) {
                if (hasByteElement()) {
                    if (!isUniform) {
                        IGC_ASSERT(VecSizeInElt <= 4);
                    }
                    return false;
                }
                if (VecSizeInElt == 8 && !isUniform) {
                    return true;
                }
                // Currently, emit uses d32/d64 scatter for uniform store/load
                // and is limited to simd8.
                // Use LastNumD64 as the bundle has been found
                if (isUniform &&
                    (VecSizeInElt > (4 * 8) ||
                     (LastNumD64 > 0 && (2 * LastNumD64 > LastNumD32)))) {
                    return true;
                }
            }
            return false;
        }

        bool hasByteElement() const { return lastStoreIdxWithByteElt >= 0; }
        bool skip(uint32_t VecEltBytes, uint32_t VecSizeInElt) const {
            if (!doTracking)
                return false;

            if (VecEltBytes == 8 ||
                (VecEltBytes == 1 && VecSizeInElt == 8)) {
                if (hasByteElement() && !isUniform) {
                    // case 1: check whether to skip D64.
                    return true;
                }
            }
            if (VecEltBytes == 8) {
                // use currNumD64 during finding the bundle
                if (currNumD64 > 0 && VecSizeInElt == 3) {
                    // case 2, check whether to skip D64.
                    return false;
                }
                if (currNumD64 > 0 && (2 * currNumD64 > currNumD32)) {
                    // case 3: check whether to skip D64.
                    return false;
                }
                // otherwise, skip 8byte-aligned D64
                return true;
            }
            // VecEltBytes == 1; either D32 or D64 is okay, thus no skip.
            // useD64() will select which one to use.
            return false;
        }
    };

    auto markVisited = [this](InstAndOffsetPairs& Stores) {
        int32_t SZ = (int)Stores.size();
        for (int i = 0; i < SZ; ++i)
        {
            const LdStInfo* lsi = &Stores[i];
            setVisited(lsi->Inst);
        }
        Stores.clear();
    };

    int32_t SZ = (int)Stores.size();
    if (SZ <= 1) {
        markVisited(Stores);
        return;
    }

    auto isBundled = [](const LdStInfo* LSI, SmallVector<Instruction*, 16>& L) {
        return (std::find(L.begin(), L.end(), LSI->Inst) != L.end());
    };
    auto setBundled = [&isBundled](LdStInfo* LSI,
        SmallVector<Instruction*, 16>& L) {
        if (!isBundled(LSI, L)) {
            L.push_back(LSI->Inst);
        }
    };

    setInstOrder(BB);

    // Sort stores in the order of increasing ByteOffset
    std::sort(Stores.begin(), Stores.end(),
        [](const LdStInfo& A, const LdStInfo& B) {
            return A.ByteOffset < B.ByteOffset;
        });

    const LdStInfo* lsi0 = &Stores[0];
    LoadInst* LI = dyn_cast<LoadInst>(lsi0->Inst);
    StoreInst* SI = dyn_cast<StoreInst>(lsi0->Inst);
    bool isUniform = false;
    if (m_WI)
    {
        isUniform = m_WI->isUniform(
            LI ? LI->getPointerOperand() : SI->getPointerOperand());
    }
    const AddressModel AddrModel = lsi0->getAddressModel(m_CGC);

    // Starting from the largest alignment.
    const uint32_t bundleAlign[] = { 8, 4, 1 };
    const uint32_t aligns = (int)(sizeof(bundleAlign)/sizeof(bundleAlign[0]));
    for (int ix = 0; ix < aligns; ++ix)
    {
        const uint32_t theAlign = bundleAlign[ix];

        // If i64 insts are not supported, don't do D64 as it might
        // require i64 mov (I64 Emu only handles 1-level insertvalue
        // and extractvalue so far), etc in codegen emit.
        //
        // i64Emu: mimic Emu64Ops's enabling condition. Seems conservative
        //         and be improved in the future if needed.
        const bool hasI64Emu =
            (m_CGC->platform.need64BitEmulation() &&
                (IGC_GET_FLAG_VALUE(Enable64BitEmulation) ||
                 IGC_GET_FLAG_VALUE(Enable64BitEmulationOnSelectedPlatform)));
        if (hasI64Emu && theAlign > 4)
            continue;

        // Element size is the same as the alignment.
        //   1) For byte-aligned, use vecEltBytes = 1 with different
        //      number of vector elements to map D16U32, D32, D64. The final
        //      store's type would be <2xi8> or i16 for D16U32, i32 for D32,
        //      and i64 for D64.  For uniform, multiple of D32/D64 can be
        //      merged and store's type would be <n x i32> or <n x i64>.
        //   2) 4-byte aligned D32. vecEltBytes = 4.
        //      The final store's type is <n x i32>
        //   3) 8-byte aligned D64. vecEltBytes = 8.
        //      The final store's type is <n x i64>
        const uint32_t vecEltBytes = theAlign;
        int32_t i = 0;
        while (i < SZ)
        {
            // 1. The first one is the leading store.
            const LdStInfo* leadLSI = &Stores[i];
            if (isBundled(leadLSI, m_combinedInsts) ||
                (i+1) == SZ) /* skip for last one */ {
                ++i;
                continue;
            }

            Type* leadTy = leadLSI->getLdStType();
            Type* eltTy = leadTy->getScalarType();
            uint32_t eltBytes = (uint32_t)(m_DL->getTypeStoreSize(eltTy));
            const uint32_t align = leadLSI->getAlignment();
            // Skip if align is less than the current alignment. Also, avoid
            // merging non-uniform stores whose size >= 4 bytes when checking
            // byte-aligned bundling.
            if (align < theAlign ||
                (theAlign == 1 && eltBytes >= 4 && !isUniform)) {
                ++i;
                continue;
            }

            BundleConfig  BC(theAlign, isUniform, AddrModel, m_CGC);
            const uint32_t maxBytes = BC.getMaxVecSizeInBytes();
            uint32_t totalBytes = (uint32_t)m_DL->getTypeStoreSize(leadTy);

            SelectD32OrD64 D32OrD64(m_CGC, m_DL, isUniform, theAlign);
            D32OrD64.track(leadLSI, i, 0);

            if (totalBytes >= maxBytes) {
                ++i;
                continue;
            }

            // 2. grow this bundle as much as possible
            // [i, e]: the range of stores form a legit bundle (e > i).
            int e = -1;
            uint32_t vecSize = -1;
            for (int j = i + 1; j < SZ; ++j) {
                const LdStInfo* LSI = &Stores[j];
                if (isBundled(LSI, m_combinedInsts) ||
                    (leadLSI->ByteOffset + totalBytes) != LSI->ByteOffset)
                {
                    // stop as remaining stores are not contiguous
                    break;
                }
                Type* aTy = LSI->getLdStType();
                uint32_t currByteOffset = totalBytes;
                totalBytes += (uint32_t)m_DL->getTypeStoreSize(aTy);
                if (totalBytes > maxBytes) {
                    break;
                }

                D32OrD64.track(LSI, j, currByteOffset);

                int nextBytes = BC.getAndUpdateVecSizeInBytes(totalBytes);
                if (totalBytes == nextBytes &&
                    !D32OrD64.skip(vecEltBytes, BC.getCurrVecSize())) {
                    e = j;
                    vecSize = BC.getCurrVecSize();

                    D32OrD64.save(theAlign);
                }
            }

            // If any store has byte element, skip D64 entirely to avoid
            // forming a partial 8B-aligned D64 stores. Hopefully, this
            // would result in a bigger 4B-aligned D32 store.
            if (vecEltBytes == 8 && D32OrD64.hasByteElement()) {
                // go to next iteration with D32.
                break;
            }

            const int bundle_nelts = e - i + 1;
            if (e >= 0 && bundle_nelts > 1) {
                // Have a bundle, save it.
                m_bundles.emplace_back(BundleInfo());
                BundleInfo& newBundle = m_bundles.back();
                newBundle.bundle_eltBytes = vecEltBytes;
                newBundle.bundle_numElts = vecSize;
                newBundle.useD64 =
                    (theAlign == 1)
                    ? D32OrD64.useD64(vecEltBytes, vecSize)
                    : false;
                for (int k = i; k <= e; ++k)
                {
                    LdStInfo& tlsi = Stores[k];
                    newBundle.LoadStores.push_back(tlsi);
                    setBundled(&tlsi, m_combinedInsts);
                    setVisited(tlsi.Inst);
                }
                i = e + 1;
            }
            else {
                ++i;
            }
        }
    }

    markVisited(Stores);
}

// If V is constant or created only by IEI with const idx, return true.
bool LdStCombine::isSimpleVector(Value* V) const
{
    Value* val = V;
    while (auto IEI = dyn_cast<InsertElementInst>(val)) {
        if (!isa<Constant>(IEI->getOperand(2))) {
            return false;
        }
        val = IEI->getOperand(0);
    }
    if (isa<Constant>(val)) {
        return true;
    }
    return false;
}

// mergeConstElements
//   If EltVals has constant elements consecutively, merge them if possible.
//   The merged constant's size is no more than MaxMergeByte.
void LdStCombine::mergeConstElements(
    SmallVector<Value*, 4>& EltVals, uint32_t MaxMergeBytes)
{
    // If all sub values are constants, coalescing them into a single
    // constant of type DstEltTy.
    //
    // Merge goes with 2 bytes, 4 bytes, up to EltBytes (DstEltTy).
    // For example: DstEltTy = i64
    //     {i8 1, i8 2, i16 0x77, i8 3, i8 4, i8 5, i8 %y}
    //  b = 2:
    //     { i16 0x201, i16 0x77, i16 0x403, i8 5, i8 %y}
    //  b = 4:
    //     { i32 0x770201, i16 0x403, i8 5, i8 %y}
    //  b = 8 :
    //     no change.

    // Check if it has two consecutive constants, skip if not.
    // This is a quick check to skip for majority of cases.
    bool isCandidate = false;
    for (int i = 0, sz = (int)EltVals.size() - 1; i < sz; ++i) {
        if (isa<Constant>(EltVals[i]) && isa<Constant>(EltVals[i + 1])) {
            isCandidate = true;
            break;
        }
    }
    if (!isCandidate) {
        return;
    }

    // If there is a vector constant, expand it with its components
    bool hasMerged = false;
    std::list<Value*> mergedElts(EltVals.begin(), EltVals.end());
    // b : the number of bytes of the merged value.
    for (uint32_t b = 2; b <= MaxMergeBytes; b *= 2) {
        int currOff = 0;
        auto NI = mergedElts.begin();
        for (auto II = NI; II != mergedElts.end(); II = NI) {
            ++NI;
            if (NI == mergedElts.end()) {
                break;
            }

            // Try to merge (II, NI)
            Value* elt0 = *II;
            Type* ty0 = elt0->getType();
            const uint32_t sz0 = (uint32_t)m_DL->getTypeStoreSize(ty0);
            // Merged value shall be naturally aligned.
            if ((currOff % b) != 0 || sz0 >= b) {
                currOff += sz0;
                continue;
            }
            Value* elt1 = *NI;
            Type* ty1 = elt1->getType();
            const uint32_t sz1 = (uint32_t)m_DL->getTypeStoreSize(ty1);
            Constant* C0 = dyn_cast<Constant>(elt0);
            Constant* C1 = dyn_cast<Constant>(elt1);
            if (!C0 || !C1 || (sz0 + sz1) != b) {
                continue;
            }
            IGC_ASSERT_MESSAGE(!C0->getType()->isVectorTy() &&
                !C1->getType()->isVectorTy(), "Vector Constant not supported!");
            uint64_t imm0 = GetImmediateVal(C0);
            uint64_t imm1 = GetImmediateVal(C1);
            imm0 &= maxUIntN(sz0 * 8);
            imm1 &= maxUIntN(sz1 * 8);
            uint64_t imm = ((imm1 << (sz0 * 8)) | imm0);
            Type* ty = IntegerType::get(ty0->getContext(), (sz0 + sz1) * 8);
            Constant* nC = ConstantInt::get(ty, imm, false);

            mergedElts.insert(II, nC);
            auto tII = NI;
            ++NI;
            mergedElts.erase(II);
            mergedElts.erase(tII);
            hasMerged = true;
        }
    }

    if (!hasMerged) {
        return;
    }

    EltVals.clear();
    EltVals.insert(EltVals.end(), mergedElts.begin(), mergedElts.end());
}

// gatherCopy():
//   Generate the coalesed values of either struct type or vector type.
// Arguments:
//   DstEltTy:     type for vector element. If the final value is a struct,
//                 its struct member does not need to be DstEltTy, but its
//                 size must be the same as DstEltTy's size.
//   DstNElts:     num of elements (vector) or num of direct members (struct)
//   Vals:         a list of values to be coalesced
//   InsertBefore: inserting pos for new instructions.
//
// Examples:
//   1. vector type;
//      given DstEltTy=i32 and DstNElts=4
//      Vals = { i32 a, i64 b, float c }
//
//      <4xi32> returnVal = {
//        a,
//        extractElement bitcast (i64 b to <2xi32>), 0
//        extractElement bitcast (i64 b to <2xi32>), 1
//        bitcast (float c to i32)
//      };
//   2. struct type
//      given DstNElts x DstEltTy = 8xi32
//      Vals = { 4xi8 a, i64 b,  4xfloat c, i16 d, i8 e, i8 f}
//
//      This function generates a val of struct type:
//
//      struct {
//        struct {   // indexes
//          i8 d0;   // (0, 0): extElt <4xi8> a,  0
//          i8 d1;   // (0, 1): extElt <4xi8> a,  1
//          i8 d2;   // (0, 2): extElt <4xi8> a,  2
//          i8 d3;   // (0, 3): extElt <4xi8> a,  3
//        } E0;
//        i32   E1;    // (1): extElt bitcast (i64 b to <2xi32>), 0
//        i32   E2;    // (2): extElt bitcast (i64 b to <2xi32>), 1
//        float E3;    // (3): extElt <4xfloat> c,  0
//        float E4;    // (4): extElt <4xfloat> c,  1
//        float E5;    // (5): extElt <4xfloat> c,  2
//        float E6;    // (6): extElt <4xfloat> c,  3
//        struct {
//          i16 d0;  // (7, 0): d
//          i8  d1;  // (7, 1): e
//          i8  d2;  // (7, 2): f
//        } E7;
//      } returnVal;
//
//  The struct layout:
//    The direct members are in SOA. If its direct members are struct, those
//    struct members (their size is either 32bit or 64bit) are in AOS. This
//    is consistent with viewing struct as a vector < DstNElts x DstEltTy >
//    from layout point of view.
//
//    To distinguish the struct generated here from other structs, the struct
//    generated here are identified with reserved names, returned by
//    getStructNameForSOALayout() or getStructNameForAOSLayout().
//
Value* LdStCombine::gatherCopy(
    Type* DstEltTy,
    int   DstNElts,
    SmallVector<Value*, 16>& Vals,
    Instruction* InsertBefore)
{
    // AllEltVals:
    //   each entry is one direct member of struct or vector. If an entry has
    //   more than one elements, it is either D32 or D64 in size, and likely a
    //   member of type struct.
    // The final value is either a struct or a vector. Its total size and its
    // GRF layout is the same as vector type <DstNElts x DstEltTy>.
    SmallVector<SmallVector<Value*, 4>, 16> allEltVals;

    // eltVals:
    //   Pending values that are going to form a single element in allEltVals.
    //   Once all pending values is complete, save it into allEltVals.
    SmallVector<Value*, 4> eltVals;

    // worklist:
    //   initialized to all input values in this bundle. Its values are
    //   gradually moved to AllEltVals one by one until it is empty.
    std::list<Value*> worklist(Vals.begin(), Vals.end());
    IRBuilder<> irBuilder(InsertBefore);
    const uint32_t EltBytes = (DstEltTy->getScalarSizeInBits() / 8);

    // remainingBytes:
    //   initialize to be the size of DstEltTy. It is the size of each
    //   member of the struct or vector.
    uint remainingBytes = EltBytes;
    while (!worklist.empty()) {
        Value* v = worklist.front();
        worklist.pop_front();

        if (v->getType()->isVectorTy())
        {
            IGC_ASSERT((v->getType()->getScalarSizeInBits() % 8) == 0);
            uint32_t eBytes = (v->getType()->getScalarSizeInBits() / 8);
            auto iVTy = cast<IGCLLVM::FixedVectorType>(v->getType());
            uint32_t n = (uint32_t)iVTy->getNumElements();

            // true if v is a legal vector at level 1
            bool isLvl1 = (remainingBytes == EltBytes && eBytes == EltBytes);
            // true if v is a legal vector at level 2
            bool isLvl2 = (remainingBytes >= (eBytes * n));
            // v is a simple vector if v is either a constant or
            // all its components are created by IEI.
            bool isSimpleVec = isSimpleVector(v);
            if (isLvl1 && !isSimpleVec)
            {   // case 1
                // 1st level vector member
                eltVals.push_back(v);
                allEltVals.push_back(eltVals);

                eltVals.clear();
            }
            else if (isLvl2 && !isSimpleVec)
            {   // case 2
                // 2nd level vector member
                eltVals.push_back(v);
                remainingBytes -= (eBytes * n);

                if (remainingBytes == 0) {
                    mergeConstElements(eltVals, EltBytes);

                    allEltVals.push_back(eltVals);

                    // Initialization for the next element
                    eltVals.clear();
                    remainingBytes = EltBytes;
                }
            }
            else
            {   // case 3
                SmallVector<Value*, 16> elts;
                getOrCreateElements(v, elts, InsertBefore);
                worklist.insert(worklist.begin(), elts.begin(), elts.end());
            }
            continue;
        }

        Type* eTy = v->getType();
        const uint32_t eBytes = (uint32_t)m_DL->getTypeStoreSize(eTy);
        if (eTy->isPointerTy()) {
            // need ptrtoint cast as bitcast does not work
            IGC_ASSERT(eBytes == 8 || eBytes == 4 || eBytes == 2);
            eTy = IntegerType::get(eTy->getContext(), eBytes * 8);
            v = irBuilder.CreateCast(Instruction::PtrToInt, v, eTy);
        }

        // If v isn't element-size aligned in GRF at this offset, cannot
        // generate a mov instruction. v must be split into small chunks
        // that are aligned for mov to work.
        uint32_t currAlign =
            (uint32_t)MinAlign(EltBytes, EltBytes - remainingBytes);
        if (currAlign < eBytes) {
            // Two cases:
            //   1. EltBytes = 4
            //      store i32 p
            //      store i32 p+4
            //      store i64 p+8  <- v : i64
            //     Need to split i64 by casting i64 --> 2xi32
            //   2. EltBytes = 4, packed struct
            //      store  i8 p
            //      store i16 p+1    <- v : i16
            //      store  i8 p+2
            //     Need to split i16 into 2xi8
            IGC_ASSERT((eBytes % currAlign) == 0);
            int n = eBytes / currAlign;
            Type* newETy = IntegerType::get(m_F->getContext(), currAlign * 8);
            VectorType* nVTy = VectorType::get(newETy, n, false);
            Value* new_v = irBuilder.CreateCast(Instruction::BitCast, v, nVTy);
            auto insPos = worklist.begin();
            for (int i = 0; i < n; ++i) {
                Value* v = irBuilder.CreateExtractElement(new_v, i);
                worklist.insert(insPos, v);
            }
            continue;
        }

        // v should fit into this remainingByts as v is element-size aligned.
        IGC_ASSERT(remainingBytes >= eBytes);
        eltVals.push_back(v);
        remainingBytes -= eBytes;
        if (remainingBytes == 0) {
            // Found one element of size EltBytes.
            mergeConstElements(eltVals, EltBytes);

            allEltVals.push_back(eltVals);

            // Initialization for the next element
            eltVals.clear();
            remainingBytes = EltBytes;
        }
    }

    IGC_ASSERT(eltVals.empty());

    // A new coalesced value could be one of two types
    //   1 a vector type  < DstNElts x DstEltTy >
    //     If The element size of every original stored value is the same as
    //     or multiple of the size of DstEltTy.
    //   2 a struct type
    //     If it cannot be of vector type, it is of a struct type. All the
    //     elements of the struct must have the same size. If its element
    //     is also a struct member, that struct member's size must be the
    //     same as other element's size. And that size is either 32bit or
    //     64bits. And the struct nesting is at most 2 levels.
    //
    //     Those struct or member struct are created as identified struct
    //     types with a special reserved names. The struct's layout in GRF
    //     is the same as that of a vector < DstNElts x DstEltTy >. This
    //     implies that the struct GRF layout is SOA on the first level,
    //     which is consistent with the current rule; for any of its member
    //     that is also of struct type, it is AOS.
    //
    //     Note that the struct with one-level was supported only in IGC
    //     until this change.
    //
    //    For examples:
    //     1) vector type (D64 as element)
    //           store i64 a, p; store i64 b, p+8; store i64 c, p+16
    //        -->
    //           store <3xi64> <a, b, c>,  p
    //     2)struct type (D32 as element)
    //          store i32 a, p; store i32 b, p+4
    //          store i8  c0,  p+8; store i8 c1, p+9;
    //          store i8  c2, p+10; store i8 c3, p+11
    //          store i32 d, p+12
    //      -->
    //          struct __StructSOALayout_ {
    //            i32, i32, struct {i8, i8, i16}, i32}
    //          }
    //          store __StructSOALayout__ <{a, b, <{c0, c1, c2, c3}>, d}>, p
    //
    //        Instead of store on struct type, a vector store is used to take
    //        advantage of the existing vector store of codegen emit.
    //          let stVal = __StructSOALayout__ <{a, b, <{c0, c1, c2, c3}>, d}>
    //
    //          val = bitcast __StructSOALayout__ %stVal to <4xi32>
    //          store <4xi32> %val, p
    //
    //        The "bitcast" is no-op, meaning the layout of stVal is the same as
    //        that of viewing it as a vector.
    //
    auto isLvl2Vecmember = [=](Type* ty) {
        uint32_t n = (uint32_t)m_DL->getTypeStoreSize(ty->getScalarType());
        return ty->isVectorTy() && n < EltBytes;
    };

    bool hasStructMember = false;
    bool hasVecMember = false;
    const int32_t sz = (int)allEltVals.size();
    SmallVector<Type*, 16> StructTys;
    for (int i = 0; i < sz; ++i) {
        SmallVector<Value*, 4>& subElts = allEltVals[i];
        int nelts = (int)subElts.size();
        Type* ty = subElts[0]->getType();
        hasVecMember = (hasVecMember || ty->isVectorTy());
        uint32_t eBytes = (uint32_t)m_DL->getTypeStoreSize(ty->getScalarType());
        if (nelts == 1 && !isLvl2Vecmember(ty)) {
            IGC_ASSERT(eBytes == EltBytes);
            StructTys.push_back(ty);
        }
        else {
            SmallVector<Type*, 4> subEltTys;
            for (auto II : subElts) {
                Value* elt = II;
                hasVecMember = (hasVecMember || elt->getType()->isVectorTy());
                subEltTys.push_back(elt->getType());
            }

            // create a member of a packed and identified struct type
            // whose size = EltBytes. Use AOS layout.
            Type* eltStTy = StructType::create(subEltTys,
                getStructNameForAOSLayout(), true);
            StructTys.push_back(eltStTy);

            hasStructMember = true;
        }
    }

    // If only hasVecMember is true, either struct or vector work; but
    // struct is better as it will likely have less mov instructions.
    Type* structTy;
    Value* retVal;
    if (hasStructMember || hasVecMember)
    {
        // Packed and named identified struct. Prefix "__" make sure it won't
        // collide with any user types.  Use SOA layout.
        structTy =
            StructType::create(StructTys, getStructNameForSOALayout(), true);

        // Create a value of type structTy
        retVal = UndefValue::get(structTy);
        for (int i = 0; i < sz; ++i) {
            SmallVector<Value*, 4>& eltVals = allEltVals[i];
            const int sz1 = (int)eltVals.size();
            Type* ty = eltVals[0]->getType();
            if (sz1 == 1 && !isLvl2Vecmember(ty)) {
                retVal = irBuilder.CreateInsertValue(retVal, eltVals[0], i);
            }
            else {
                for (int j = 0; j < sz1; ++j) {
                    uint32_t idxs[2] = { (unsigned)i, (unsigned)j };
                    retVal =
                        irBuilder.CreateInsertValue(retVal, eltVals[j], idxs);
                }
            }
        }
    }
    else if (DstNElts == 1)
    {
        // a <1xDstEltTy>, use scalar type DstEltTy as the final type.
        SmallVector<Value*, 4>& eltVals = allEltVals[0];
        retVal = irBuilder.CreateBitCast(eltVals[0], DstEltTy);
    }
    else
    {
        VectorType* newTy = VectorType::get(DstEltTy, DstNElts, false);
        retVal = UndefValue::get(newTy);
        for (int i = 0; i < sz; ++i) {
            SmallVector<Value*, 4>& eltVals = allEltVals[i];
            Value* tV = irBuilder.CreateBitCast(eltVals[0], DstEltTy);
            retVal = irBuilder.CreateInsertElement(retVal, tV, i);
        }
    }
    return retVal;
}

void LdStCombine::createCombinedStores(Function& F)
{
    for (auto& bundle : m_bundles)
    {
        InstAndOffsetPairs& Stores = bundle.LoadStores;
        IGC_ASSERT(bundle.LoadStores.size() >= 2);

        // The new store will be inserted at the place of the last store,
        // called anchor store, in the bundle. The lead store is the first
        // store in the bundle.
        // (Lead store, amaong all stores in the bundle, does not necessarily
        //  appear first in the BB; and the last store does not necessarily
        //  have the largest offset in the bundle.)
        StoreInst* leadStore = static_cast<StoreInst*>(Stores[0].Inst);
        SmallVector<Value*, 16> storedValues;
        storedValues.push_back(leadStore->getValueOperand());
        StoreInst* anchorStore = leadStore;
        int n = m_instOrder[anchorStore];
        // insts are assigned order number starting from 0. Anchor store is
        // one with the largest inst order number.
        for (int i = 1, sz = (int)bundle.LoadStores.size(); i < sz; ++i)
        {
            StoreInst* SI = static_cast<StoreInst*>(Stores[i].Inst);
            int SI_no = m_instOrder[SI];
            if (SI_no > n)
            {
                n = SI_no;
                anchorStore = SI;
            }
            storedValues.push_back(SI->getValueOperand());
        }

        int eltBytes = bundle.bundle_eltBytes;
        int nelts = bundle.bundle_numElts;
        if (eltBytes == 1) { // byte-aligned
            // D64, D32, D16U32
            if ((nelts % 4) == 0) {
                if (bundle.useD64) {
                    // D64
                    IGC_ASSERT((nelts % 8) == 0);
                    eltBytes = 8;
                    nelts = nelts / 8;
                }
                else {
                    // D32
                    eltBytes = 4;
                    nelts = nelts / 4;
                }
            }
            else {
                // <2xi8>,  D16U32
                IGC_ASSERT(nelts == 2);
            }
        }

        // Create the new vector type for these combined stores.
        Type* eltTy = Type::getIntNTy(F.getContext(), eltBytes * 8);
        Type* VTy = (nelts == 1 ? eltTy : VectorType::get(eltTy, nelts, false));

        // Generate the coalesced value.
        Value* nV = gatherCopy(eltTy, nelts, storedValues, anchorStore);

        IRBuilder<> irBuilder(anchorStore);
        Value* storedVal = nV;
        if (nV->getType()->isStructTy()) {
            // Use special bitcast from struct to int vec to use vector emit.
            // This bitcast is a noop and will be dessa'ed with its source.
            Type* ITys[2] = { VTy, nV->getType() };
            Function* IntrDcl = GenISAIntrinsic::getDeclaration(
                F.getParent(), GenISAIntrinsic::ID::GenISA_bitcastfromstruct, ITys);
            storedVal = irBuilder.CreateCall(IntrDcl, nV);
        }

        Value* Addr = leadStore->getPointerOperand();
        PointerType* PTy = cast<PointerType>(Addr->getType());
        PointerType* nPTy = PointerType::get(VTy, PTy->getAddressSpace());
        Value* nAddr = irBuilder.CreateBitCast(Addr, nPTy);
        StoreInst* finalStore = irBuilder.CreateAlignedStore(storedVal,
            nAddr, IGCLLVM::getAlign(*leadStore), leadStore->isVolatile());
        finalStore->setDebugLoc(anchorStore->getDebugLoc());

        // Only keep metadata based on leadStore.
        // (If each store has a different metadata, should they be merged
        //  in the first place?)
        if (MDNode* Node = leadStore->getMetadata("lsc.cache.ctrl"))
            finalStore->setMetadata("lsc.cache.ctrl", Node);
        if (MDNode* Node = leadStore->getMetadata(LLVMContext::MD_nontemporal))
            finalStore->setMetadata(LLVMContext::MD_nontemporal, Node);
    }

    // Delete stores that have been combined.
    eraseDeadInsts();

    m_bundles.clear();
}

void LdStCombine::eraseDeadInsts()
{
    RecursivelyDeleteDeadInstructions(m_combinedInsts);
    m_combinedInsts.clear();
}


namespace IGC
{

bool isLayoutStructType(const StructType* StTy)
{
    if (!StTy || StTy->isLiteral() || !StTy->hasName() || !StTy->isPacked())
        return false;
    StringRef stId = StTy->getName();
    return (stId.startswith(getStructNameForSOALayout()) ||
        stId.startswith(getStructNameForAOSLayout()));
}

bool isLayoutStructTypeAOS(const StructType* StTy)
{
    if (!StTy || StTy->isLiteral() || !StTy->hasName() || !StTy->isPacked())
        return false;
    StringRef stId = StTy->getName();
    return stId.startswith(getStructNameForAOSLayout());
}

uint64_t bitcastToUI64(Constant* C, const DataLayout* DL)
{
    Type* ty = C->getType();
    IGC_ASSERT(DL->getTypeStoreSizeInBits(ty) <= 64);
    IGC_ASSERT(ty->isStructTy() ||
        (ty->isSingleValueType() && !ty->isVectorTy()));

    uint64_t imm = 0;
    if (StructType* sTy = dyn_cast<StructType>(C->getType())) {
        IGC_ASSERT(DL->getTypeStoreSizeInBits(sTy) <= 64);
        IGC_ASSERT(isLayoutStructTypeAOS(sTy));
        const StructLayout* SL = DL->getStructLayout(sTy);
        int N = (int)sTy->getNumElements();
        for (int i = 0; i < N; ++i)
        {
            Constant* C_i = C->getAggregateElement(i);
            if (isa<UndefValue>(C_i)) {
                continue;
            }
            Type* ty_i = sTy->getElementType(i);
            uint32_t offbits = (uint32_t)SL->getElementOffsetInBits(i);
            if (auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(ty_i)) {
                // C_I is vector
                int32_t nelts = (int32_t)iVTy->getNumElements();
                Type* eTy_i = ty_i->getScalarType();
                IGC_ASSERT(eTy_i->isFloatingPointTy() || eTy_i->isIntegerTy());
                uint32_t nbits = (uint32_t)DL->getTypeStoreSizeInBits(eTy_i);
                for (int j = 0; j < nelts; ++j) {
                    Constant* c_ij = C_i->getAggregateElement(j);
                    uint64_t tImm = GetImmediateVal(c_ij);
                    tImm &= maxUIntN(nbits);
                    imm = imm | (tImm << (offbits + j * nbits));
                }
            }
            else {
                // C_i is scalar
                uint32_t nbits = (uint32_t)DL->getTypeStoreSizeInBits(ty_i);
                uint64_t tImm = GetImmediateVal(C_i);
                tImm &= maxUIntN(nbits);
                imm = imm | (tImm << offbits);
            }
        }
        return imm;
    }
    if (isa<ConstantFP>(C) || isa<ConstantInt>(C)) {
        return GetImmediateVal(C);
    }
    if (isa<UndefValue>(C)) {
        return 0;
    }
    IGC_ASSERT_MESSAGE(0, "unsupported Constant!");
    return 0;
}

void getStructMemberByteOffsetAndType_1(const DataLayout* DL,
    StructType* StTy, const ArrayRef<unsigned>& Indices,
    Type*& Ty, uint32_t& ByteOffset)
{
    IGC_ASSERT_MESSAGE(Indices.size() == 1,
        "ICE: nested struct not supported!");
    const StructLayout* aSL = DL->getStructLayout(StTy);
    uint32_t ix = Indices.front();
    ByteOffset = (uint32_t)aSL->getElementOffset(ix);
    Ty = StTy->getElementType(ix);
    return;
};

void getStructMemberOffsetAndType_2(const DataLayout* DL,
    StructType* StTy, const ArrayRef<unsigned>& Indices,
    Type*& Ty0, uint32_t& ByteOffset0,
    Type*& Ty1, uint32_t& ByteOffset1)
{
    uint32_t ix = Indices[0];
    const StructLayout* SL0 = DL->getStructLayout(StTy);
    ByteOffset0 = (uint32_t)SL0->getElementOffset(ix);
    Ty0 = StTy->getElementType(ix);
    ByteOffset1 = 0;
    Ty1 = nullptr;

    if (Indices.size() == 1)
    {
        return;
    }

    IGC_ASSERT(isLayoutStructType(StTy));
    IGC_ASSERT_MESSAGE(Indices.size() <= 2,
        "struct with nesting level > 2 not supported!");
    IGC_ASSERT_MESSAGE((Ty0->isStructTy() &&
        isLayoutStructTypeAOS(cast<StructType>(Ty0))),
        "Only a special AOS layout struct is supported as a member");
    uint32_t ix1 = Indices[1];
    StructType* stTy0 = cast<StructType>(Ty0);
    const StructLayout* SL1 = DL->getStructLayout(stTy0);
    ByteOffset1 = (uint32_t)SL1->getElementOffset(ix1);
    Ty1 = stTy0->getElementType(ix1);
    return;
}

void getAllDefinedMembers (const Value* IVI,
    std::list<ArrayRef<unsigned>>& fieldsTBC)
{
    IGC_ASSERT(IVI != nullptr);
    const Value* V = IVI;
    while (isa<InsertValueInst>(V))
    {
        const InsertValueInst* I = cast<const InsertValueInst>(V);
        fieldsTBC.push_front(I->getIndices());
        V = I->getOperand(0);
    }
    if (!isa<UndefValue>(V))
    {
        // Don't know for sure, assume all have been defined.
        fieldsTBC.clear();
        StructType* stTy = cast<StructType>(IVI->getType());
        fieldsTBC.insert(fieldsTBC.end(), 0, stTy->getNumElements() - 1);
    }
}
}
