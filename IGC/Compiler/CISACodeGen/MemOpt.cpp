/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/Analysis/InstructionSimplify.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include <llvmWrapper/Analysis/AliasSetTracker.h>
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
#include <optional>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/SLMConstProp.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/CISACodeGen/MemOpt.h"
#include "Probe/Assertion.h"
#include <DebugInfo/DwarfDebug.cpp>
#include "MemOptUtils.h"

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
        // ALoadInst, Offset, MemRefListTy::iterator, LeadingLoad's int2PtrOffset
        typedef SmallVector<std::tuple<Instruction *, int64_t, MemRefListTy::iterator>, 8> MergeVector;

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

        StringRef getPassName() const override { return "MemOpt"; }

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

        bool mergeLoad(ALoadInst& LeadingLoad, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt);
        bool mergeStore(AStoreInst& LeadingStore, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt);
        bool removeRedBlockRead(GenIntrinsicInst* LeadingLoad, MemRefListTy::iterator MI,
            MemRefListTy& MemRefs, TrivialMemRefListTy& ToOpt, unsigned& SimdSize);

        std::optional<unsigned> chainedSelectAndPhis(Instruction* Inst, unsigned depth,
            llvm::DenseMap<Instruction*, unsigned> &depthTracking);

        void removeVectorBlockRead(Instruction* BlockReadToOptimize, Instruction* BlockReadToRemove,
            Value* SgId, llvm::IRBuilder<>& Builder, unsigned& sg_size);
        void removeScalarBlockRead(Instruction* BlockReadToOptimize, Instruction* BlockReadToRemove,
            Value* SgId, llvm::IRBuilder<>& Builder);
        Value* getShuffle(Value* ShflId, Instruction* BlockReadToOptimize,
            Value* SgId, llvm::IRBuilder<>& Builder, unsigned& ToOptSize);

        unsigned getNumElements(Type* Ty) {
            return Ty->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;
        }

        Type* getVectorElementType(Type* Ty) const {
            return isa<VectorType>(Ty) ? cast<VectorType>(Ty)->getElementType() : Ty;
        }

        bool hasSameSize(Type* A, Type* B) const {
            // Shortcut if A is equal to B.
            if (A == B)
                return true;
            return DL->getTypeStoreSize(A) == DL->getTypeStoreSize(B);
        }

        Value* createBitOrPointerCast(Value* V, Type* DestTy,
            IGCIRBuilder<>& Builder) {
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

        /**
         * @brief Creates a new merge value for merged load from a set of predicated loads' merge values.
         *
         * This function constructs a new combined merge value by merging the merge values of multiple predicated load intrinsics.
         * Merge value from each input predicated load is inserted into the appropriate position in the resulting merge vector value,
         * based on its offset and the scalar size. The function handles both scalar and vector merge input values.
         *
         * @param MergeValTy The type of the merged value to be created.
         * @param LoadsToMerge A vector of tuples, each containing a load instruction and its associated offset.
         * @param LdScalarSize The size (in bytes) of the scalar element being loaded in the combined load.
         * @param NumElts Number of elements in the merged value vector.
         * @return Value* The newly created merged value, or nullptr if we are merging generic loads, not predicated.
         */
        Value* CreateNewMergeValue(IGCIRBuilder<>& Builder, Type* MergeValTy,
                                  const MergeVector& LoadsToMerge, unsigned LdScalarSize,
                                  unsigned& NumElts) {
            Value* NewMergeValue = UndefValue::get(MergeValTy);
            unsigned Pos = 0;
            int64_t FirstOffset = std::get<1>(LoadsToMerge.front());

            for (auto& I : LoadsToMerge) {
                PredicatedLoadIntrinsic* PLI = ALoadInst::get(std::get<0>(I))->getPredicatedLoadIntrinsic();
                if (!PLI)
                    return nullptr;

                Value* MergeValue = PLI->getMergeValue();
                unsigned MergeValNumElements = getNumElements(MergeValue->getType());
                Type* MergeValScalarTy = MergeValTy->getScalarType();
                Pos = unsigned((std::get<1>(I) - FirstOffset) / LdScalarSize);

                if (MergeValNumElements == 1) {
                    IGC_ASSERT_MESSAGE(Pos < NumElts, "Index is larger than the number of elements, we cannot update merge value.");
                    MergeValue = createBitOrPointerCast(MergeValue, MergeValScalarTy, Builder);
                    NewMergeValue = Builder.CreateInsertElement(NewMergeValue, MergeValue, Builder.getInt32(Pos));
                    continue;
                }

                IGC_ASSERT_MESSAGE(Pos + MergeValNumElements <= NumElts,
                    "Index is larger than the number of elements, we cannot update merge value.");

                for (unsigned i = 0; i < MergeValNumElements; ++i) {
                    Value* ExtractValue = Builder.CreateExtractElement(MergeValue, Builder.getInt32(i));
                    ExtractValue = createBitOrPointerCast(ExtractValue, MergeValScalarTy, Builder);
                    NewMergeValue = Builder.CreateInsertElement(NewMergeValue, ExtractValue, Builder.getInt32(Pos + i));
                }
            }
            return NewMergeValue;
        }

        bool isSafeToMergeLoad(const ALoadInst& Ld,
            const SmallVectorImpl<Instruction*>& checkList) const;
        bool isSafeToMergeStores(
            const SmallVectorImpl<std::tuple<Instruction*, int64_t, MemRefListTy::iterator>>& Stores,
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
                if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead ||
                    GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_PredicatedLoad ||
                    GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_PredicatedStore){
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
        bool checkAlignmentBeforeMerge(const AccessInstruction& inst,
            SmallVector<std::tuple<Instruction*, int64_t, MemRefListTy::iterator>, 8> & AccessIntrs,
            unsigned& NumElts)
        {
            auto alignment = inst.getAlignmentValue();
            if (alignment == 0)
            {
                // SROA LLVM pass may sometimes set a load/store alignment to 0. It happens when
                // deduced alignment (based on GEP instructions) matches an alignment specified
                // in datalayout for a specific type. It can be problematic as MemOpt merging
                // logic is implemented in a way that a product of merging inherits an alignment
                // from the leading load/store. It results in creating memory instruction with
                // different type, without alignment set, therefore the information about the
                // correct alignment gets lost.
                CGC->EmitWarning("MemOpt expects alignment to be always explicitly set for the leading instruction!");
            }

            if (alignment < 4 && !WI->isUniform(inst.inst()))
            {
                llvm::Type* dataType = inst.getValue()->getType();
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
                    auto AI = AccessInstruction::get(acessInst);
                    accessSize = int64_t(DL->getTypeSizeInBits(AI->getValue()->getType())) / 8;
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
                    if (AccessInstruction::get(std::get<0>(*rit))->getAlignmentValue() >= 4)
                        return false;
                }

                // Need to subtract the last offset by the first offset and add one to
                // get the new size of the vector
                NumElts = unsigned(mergedSize / scalarTypeSizeInBytes);
            }
            return true;
        }

        // This is for enabling the mergeload improvement (comparing GEP's last
        // index instead) as it requires to turn off GEP canonicalization.
        bool EnableCanonicalizeGEP() const {
            IGC_ASSERT(CGC != nullptr);
            // The new mergeload improvement is intended for PVC+ for now.
            if (CGC->platform.getPlatformInfo().eProductFamily != IGFX_PVC &&
                !CGC->platform.isProductChildOf(IGFX_PVC)) {
                // No mergeload improvement
                return true;
            }

            switch (IGC_GET_FLAG_VALUE(MemOptGEPCanon)) {
            case 1:
                return false;
            case 2:
            {
                if (CGC->type == ShaderType::OPENCL_SHADER)
                    return false;
                break;
            }
            default:
                break;
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

        bool getConstantOffset(SymbolicPointer& Other, int64_t& Off);
        static Value* getLinearExpression(Value* Val, APInt& Scale, APInt& Offset,
            ExtensionKind& Extension, unsigned Depth,
            const DataLayout* DL);
        static bool decomposePointer(const Value* Ptr, SymbolicPointer& SymPtr,
            CodeGenContext* DL);

        static const unsigned MaxLookupSearchDepth = 6;

    private:
        void saveTerm(Value* Src, int64_t IndexScale, uint64_t Scale, int64_t IndexOffset,
            ExtensionKind Extension, unsigned int ptrSize);
        bool checkTerms(const Term* T, const Term* OtherT, int64_t& Off) const;
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

    // If LdStCombining is on, no need to do memopt.
    const bool DisableMergeStore =
        (doLdStCombine(CGC) && IGC_IS_FLAG_ENABLED(DisableMergeStore));

    bool Changed = false;

    IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MDU->getFunctionsInfoItem(&F);
    unsigned SimdSize = funcInfoMD->getSubGroupSize()->getSIMDSize();

    for (Function::iterator BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI) {
        // Find all instructions with memory reference. Remember the distance one
        // by one.
        BasicBlock* BB = &*BBI;
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

        if (EnableCanonicalizeGEP()) {
            // Canonicalize 64-bit GEP to help SCEV find constant offset by
            // distributing `zext`/`sext` over safe expressions.
            for (auto& M : MemRefs)
                Changed |= canonicalizeGEP64(M.first);
        }

        for (auto MI = MemRefs.begin(), ME = MemRefs.end(); MI != ME; ++MI) {
            Instruction* I = MI->first;

            // Skip already merged one.
            if (!I)
                continue;

            if (auto ALI = ALoadInst::get(I); ALI.has_value())
                Changed |= mergeLoad(ALI.value(), MI, MemRefs, MemRefsToOptimize);
            else if (auto ASI = AStoreInst::get(I); ASI.has_value()) {
                if (!DisableMergeStore)
                    Changed |= mergeStore(ASI.value(), MI, MemRefs, MemRefsToOptimize);
            }
            else if (EnableRemoveRedBlockreads) {
                if (GenIntrinsicInst* GInst = dyn_cast<GenIntrinsicInst>(I)) {
                    if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead) {
                        Changed |= removeRedBlockRead(GInst, MI, MemRefs, MemRefsToOptimize, SimdSize);
                    }
                }
            }
        }

        if (EnableCanonicalizeGEP()) {
            // Optimize 64-bit GEP to reduce strength by factoring out `zext`/`sext`
            // over safe expressions.
            for (auto I : MemRefsToOptimize)
                Changed |= optimizeGEP64(I);
        }
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

    MemoryLocation LeadingBlockReadMemLoc = getLocation(cast<Instruction>(LeadingBlockRead), TLI);
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
            MemoryLocation WriteInstrMemLoc = getLocation(NextMemRef, TLI);
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
    for (const auto& ITuple : BlockReadToRemove) {
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
std::optional<unsigned> MemOpt::chainedSelectAndPhis(Instruction* Inst , unsigned depth,
    llvm::DenseMap<Instruction*, unsigned> &depthTracking)
{
    //Max depth set to 300
    if (depth >= 300)
    {
        return std::nullopt;
    }

    if (auto I = depthTracking.find(Inst); I != depthTracking.end())
    {
        if ((depth + I->second) >= 300)
            return std::nullopt;

        return I->second;
    }

    unsigned MaxRemDepth = 0;
    for (auto& operand : Inst->operands())
    {
        if (auto* op_inst = dyn_cast<Instruction>(operand))
        {
            if (isa<PHINode>(op_inst) || isa<SelectInst>(op_inst))
            {
                std::optional<unsigned> RemDepth = chainedSelectAndPhis(op_inst, depth + 1, depthTracking);
                if (!RemDepth)
                    return std::nullopt;
                MaxRemDepth = std::max(MaxRemDepth, *RemDepth + 1);
            }
        }
    }

    depthTracking[Inst] = MaxRemDepth;
    return MaxRemDepth;
}

bool MemOpt::mergeLoad(ALoadInst& LeadingLoad,
    MemRefListTy::iterator aMI, MemRefListTy& MemRefs,
    TrivialMemRefListTy& ToOpt)
{
    MemRefListTy::iterator MI = aMI;
    // For cases like the following:
    //   ix0 = sext i32 a0 to i64
    //   addr0 = gep base, i64 ix0
    //
    //   ix1 = sext i32 a1 to i64
    //   addr1 = gep base, i64 ix1
    // Since SCEV does not do well with sext/zext/longer expression on
    // comparing addr0 with addr1, this function compares a0 with a1 instead.
    // In doing so, it skip sext/zext and only on the last index (thus shorter
    // expression). The condition for doing so is that if all indices are
    // identical except the last one.
    //
    // Return value:  byte offset to LeadLastIdx. Return 0 if unknown.
    auto getGEPIdxDiffIfAppliable = [this](const SCEV*& LeadLastIdx,
        ALoadInst& LeadLd, ALoadInst& NextLd)
    {
        // Only handle single-index GEP for now.
        auto LeadGEP = dyn_cast<GetElementPtrInst>(LeadLd.getPointerOperand());
        auto NextGEP = dyn_cast<GetElementPtrInst>(NextLd.getPointerOperand());
        if (LeadGEP && NextGEP &&
            LeadGEP->getPointerOperand() == NextGEP->getPointerOperand() &&
            LeadGEP->getNumIndices() == NextGEP->getNumIndices() &&
            LeadLd.getType() == NextLd.getType() &&
            LeadGEP->getNumIndices() > 0) {
            const int N = LeadGEP->getNumIndices();
            for (int i = 1; i < N; ++i) {
                // GEP  0:base, 1:1st_index, 2:2nd_index, ..., N:Nth_index
                Value* ix0 = LeadGEP->getOperand(i);
                Value* ix1 = NextGEP->getOperand(i);
                if (ix0 == ix1)
                    continue;
                ConstantInt* Cix0 = dyn_cast<ConstantInt>(ix0);
                ConstantInt* Cix1 = dyn_cast<ConstantInt>(ix1);
                if (Cix0 && Cix1 && Cix0->getSExtValue() == Cix1->getSExtValue())
                    continue;
                // don't handle, skip
                return (int64_t)0;
            }

            // Make sure the last index is to the array (indexed type is array
            // element type).
            //   For N = 1, the type is an implicit array of the pointee type
            //   of GEP's pointer operand. But N > 1, need to check as the last
            //   index might be to a struct.
            if (N > 1) {
                // get type of the second index from the last.
                SmallVector<Value*, 4> Indices (LeadGEP->idx_begin(), std::prev(LeadGEP->idx_end()));
                Type* srcEltTy = LeadGEP->getSourceElementType();
                Type* Idx2Ty = GetElementPtrInst::getIndexedType(srcEltTy, Indices);
                if (!Idx2Ty || !Idx2Ty->isArrayTy())
                    return (int64_t)0;
            }

            CastInst* lastIx0 = dyn_cast<CastInst>(LeadGEP->getOperand(N));
            CastInst* lastIx1 = dyn_cast<CastInst>(NextGEP->getOperand(N));
            if (lastIx0 && lastIx1 &&
                lastIx0->getOpcode() == lastIx1->getOpcode() &&
                (isa<SExtInst>(lastIx0) || isa<ZExtInst>(lastIx0)) &&
                lastIx0->getType() == lastIx1->getType() &&
                lastIx0->getSrcTy() == lastIx1->getSrcTy()) {
                if (!LeadLastIdx)
                    LeadLastIdx = SE->getSCEV(lastIx0->getOperand(0));
                const SCEV* NextIdx = SE->getSCEV(lastIx1->getOperand(0));
                auto Diff = dyn_cast<SCEVConstant>(SE->getMinusSCEV(NextIdx, LeadLastIdx));
                if (Diff) {
                    // This returns 16 for <3 x i32>, not 12!
                    uint32_t LoadedBytes = (uint32_t)DL->getTypeStoreSize(NextLd.getType());

                    int64_t eltDiff = Diff->getValue()->getSExtValue();
                    return (int64_t)(eltDiff * LoadedBytes);
                }
            }
        }
        return (int64_t)0;
    };

    // Push the leading load into the list to be optimized (after
    // canonicalization.) It will be swapped with the new one if it's merged.
    ToOpt.push_back(LeadingLoad.inst());

    if (!LeadingLoad.isSimple())
        return false;

    Type* LeadingLoadType = LeadingLoad.getType();
    if (LeadingLoadType->isPointerTy()) {
        unsigned int AS = LeadingLoadType->getPointerAddressSpace();
        if (CGC->getRegisterPointerSizeInBits(AS) != DL->getPointerSizeInBits(AS)) {
            // we cannot coalesce pointers which have been reduced as they are
            // bigger in memory than in register
            return false;
        }
    }

    Type* LeadingLoadScalarType = LeadingLoadType->getScalarType();
    unsigned TypeSizeInBits =
        unsigned(DL->getTypeSizeInBits(LeadingLoadScalarType));
    if (!ProfitVectorLengths.count(TypeSizeInBits))
        return false;
    SmallVector<unsigned, 8> profitVec;
    // FIXME: Enable for OCL shader only as other clients have regressions but
    // there's no way to trace down.
    bool isUniformLoad = (CGC->type == ShaderType::OPENCL_SHADER) && (WI->isUniform(LeadingLoad.inst()));
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

    if (auto* Ptr = dyn_cast<Instruction>(LeadingLoad.getPointerOperand()))
    {
        llvm::DenseMap<Instruction*, unsigned> depthTracking;
        if (!chainedSelectAndPhis(Ptr, 0, depthTracking))
        {
            return false;
        }
    }

    const SCEV* LeadingPtr = SE->getSCEV(LeadingLoad.getPointerOperand());
    if (isa<SCEVCouldNotCompute>(LeadingPtr))
        return false;
    const SCEV* LeadingLastIdx = nullptr; // set on-demand
    bool DoCmpOnLastIdx = false;
    if (!EnableCanonicalizeGEP()) {
        auto aGEP = dyn_cast<GetElementPtrInst>(LeadingLoad.getPointerOperand());
        if (aGEP && aGEP->hasIndices()) {
            // index starts from 1
            Value* ix = aGEP->getOperand(aGEP->getNumIndices());
            DoCmpOnLastIdx = (isa<SExtInst>(ix) || isa<ZExtInst>(ix));
        }
    }

    // ALoadInst, Offset, MemRefListTy::iterator, LeadingLoad's int2PtrOffset
    MergeVector LoadsToMerge;
    LoadsToMerge.push_back(std::make_tuple(LeadingLoad.inst(), 0, MI));

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

        auto NextLoad = ALoadInst::get(NextMemRef);

        // Skip non-load instruction.
        if (!NextLoad.has_value())
            continue;

        // Bail out if that load is not a simple one.
        if (!NextLoad->isSimple())
            break;

        // Skip if that load is from different address spaces.
        if (NextLoad->getPointerAddressSpace() !=
            LeadingLoad.getPointerAddressSpace())
            continue;

        // Skip if predicates are different (for non-predicated load, predicate
        // is nullptr, so this check also filters out combination of predicated
        // and non-predicated loads)
        if (NextLoad->getPredicate() != LeadingLoad.getPredicate())
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
        // If addr cmp fails, try whether index cmp can be applied.
        if (DoCmpOnLastIdx && Offset == nullptr)
            Off = getGEPIdxDiffIfAppliable(LeadingLastIdx, LeadingLoad, NextLoad.value());
        // Skip load with non-constant distance.
        // If Off != 0, it is already a constant via index cmp
        if (Off == 0) {
            if (!Offset) {
                SymbolicPointer LeadingSymPtr;
                SymbolicPointer NextSymPtr;
                if (SymbolicPointer::decomposePointer(LeadingLoad.getPointerOperand(),
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
        if (!isSafeToMergeLoad(NextLoad.value(), CheckList))
            break;

        LoadsToMerge.push_back(std::make_tuple(NextLoad->inst(), Off, MI));
    }

    unsigned s = LoadsToMerge.size();
    if (s < 2)
        return false;

    IGCIRBuilder<> Builder(LeadingLoad.inst());

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

        if (EnableCanonicalizeGEP()) {
            // Guard under the key to distinguish new code (GEPCanon is off) from the old.
            //    Note: not sure about the reason for the following check.
            if (NumElts == 3 && (LeadingLoadScalarType->isIntegerTy(16) || LeadingLoadScalarType->isHalfTy())) {
                return false;
            }
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
    ALoadInst FirstLoad = ALoadInst::get(std::get<0>(LoadsToMerge.front())).value();
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
    Value* Ptr = LeadingLoad.getPointerOperand();
    if (FirstOffset < 0) {
        // If the first load is not the leading load, re-calculate the pointer
        // from the pointer of the leading load.
        IGC_ASSERT(LdScalarSize);
        IGC_ASSERT_MESSAGE(FirstOffset % LdScalarSize == 0, "Remainder is expected to be 0!");

        Value* Idx = Builder.getInt64(FirstOffset / LdScalarSize);
        Type* Ty =
            PointerType::get(LeadingLoadScalarType,
                LeadingLoad.getPointerAddressSpace());
        Ptr = Builder.CreateBitCast(Ptr, Ty);

        GEPOperator* FirstGEP =
            dyn_cast<GEPOperator>(FirstLoad.getPointerOperand());
        if (FirstGEP && FirstGEP->isInBounds())
            Ptr = Builder.CreateInBoundsGEP(LeadingLoadScalarType, Ptr, Idx);
        else
            Ptr = Builder.CreateGEP(LeadingLoadScalarType, Ptr, Idx);
    }

    Type* NewLoadType = IGCLLVM::FixedVectorType::get(LeadingLoadScalarType, NumElts);
    Type* NewPointerType =
        PointerType::get(NewLoadType, LeadingLoad.getPointerAddressSpace());
    Value* NewPointer = Builder.CreateBitCast(Ptr, NewPointerType);

    // Prepare Merge Value if needed:
    Value* NewMergeValue = CreateNewMergeValue(Builder, NewLoadType, LoadsToMerge,
                                               LdScalarSize, NumElts);

    Instruction* NewLoad =
        FirstLoad.CreateAlignedLoad(Builder, NewLoadType, NewPointer, NewMergeValue);
    NewLoad->setDebugLoc(LeadingLoad.inst()->getDebugLoc());

    // Unpack the load value to their uses. For original vector loads, extracting
    // and inserting is necessary to avoid tracking uses of each element in the
    // original vector load value.
    unsigned Pos = 0;
    MDNode* mdLoadInv = nullptr;
    bool allInvariantLoads = true;

    MDNode* nonTempMD = LeadingLoad.inst()->getMetadata("nontemporal");

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
        ALoadInst LD = ALoadInst::get(std::get<0>(I)).value();
        Value* Ptr = LD.getPointerOperand();
        // make sure the load was merged before actually removing it
        if (LD.inst()->use_empty()) {
            LD.inst()->eraseFromParent();
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

bool MemOpt::mergeStore(AStoreInst& LeadingStore,
    MemRefListTy::iterator MI, MemRefListTy& MemRefs,
    TrivialMemRefListTy& ToOpt) {
    // Push the leading store into the list to be optimized (after
    // canonicalization.) It will be swapped with the new one if it's merged.
    ToOpt.push_back(LeadingStore.inst());

    if (!LeadingStore.isSimple())
        return false;

    if (LeadingStore.getValueOperand()->getType()->isPointerTy()) {
        unsigned AS =
            LeadingStore.getValueOperand()->getType()->getPointerAddressSpace();
        if (CGC->getRegisterPointerSizeInBits(AS) != DL->getPointerSizeInBits(AS)) {
            // we cannot coalesce pointers which have been reduced as they are
            // bigger in memory than in register
            return false;
        }
    }
    unsigned NumElts = 0;
    Value* LeadingStoreVal = LeadingStore.getValueOperand();
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

    const SCEV* LeadingPtr = SE->getSCEV(LeadingStore.getPointerOperand());
    if (isa<SCEVCouldNotCompute>(LeadingPtr))
        return false;

    // AStoreInst, Offset, MemRefListTy::iterator, LeadingStore's int2PtrOffset
    SmallVector<std::tuple<Instruction*, int64_t, MemRefListTy::iterator>, 8>
        StoresToMerge;

    StoresToMerge.push_back(std::make_tuple(LeadingStore.inst(), 0, MI));

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

        std::optional<AStoreInst> NextStore = AStoreInst::get(NextMemRef);
        // Skip non-store instruction.
        if (!NextStore.has_value())
            continue;

        // Bail out if that store is not a simple one.
        if (!NextStore->isSimple())
            break;

        // Skip if that store is from different address spaces.
        if (NextStore->getPointerAddressSpace() !=
            LeadingStore.getPointerAddressSpace())
            continue;

        // Skip if it is a predicated store and predicates are different
        // (for non-predicated store, predicate is nullptr, so this check also
        // filters out combination of predicated and non-predicated stores)
        if (NextStore->getPredicate() != LeadingStore.getPredicate())
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
                LeadingStore.getPointerOperand(), LeadingSymPtr, CGC) ||
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

        StoresToMerge.push_back(std::make_tuple(NextStore->inst(), Off, MI));

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
    Instruction* TailingStore = std::get<0>(StoresToMerge.back());
    IGCIRBuilder<> Builder(TailingStore);

    // Start to merge stores.
    NumElts = 0;
    for (auto& I : StoresToMerge) {
        Type* Ty = AStoreInst::get(std::get<0>(I))->getValueOperand()->getType();
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
            Type* Ty = AStoreInst::get(std::get<0>(StoresToMerge[--s]))->getValueOperand()->getType();
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
    AStoreInst FirstStore = AStoreInst::get(std::get<0>(StoresToMerge.front())).value();

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
        Value* Val = AStoreInst::get(std::get<0>(I))->getValueOperand();
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
        PointerType::get(NewStoreType, LeadingStore.getPointerAddressSpace());
    Value* NewPointer =
        Builder.CreateBitCast(FirstStore.getPointerOperand(), NewPointerType);
    Instruction* NewStore = FirstStore.CreateAlignedStore(Builder, NewStoreVal, NewPointer);
    NewStore->setDebugLoc(TailingStore->getDebugLoc());

    // Transfer !nontemporal metadata to the new store
    if (NonTempMD)
        NewStore->setMetadata("nontemporal", NonTempMD);

    // Clone metadata
    llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 4> MDs;
    TailingStore->getAllMetadata(MDs);
    for (llvm::SmallVectorImpl<std::pair<unsigned, llvm::MDNode*> >::iterator
        MI = MDs.begin(), ME = MDs.end(); MI != ME; ++MI)
    {
        NewStore->setMetadata(MI->first, MI->second);
    }

    // Replace the list to be optimized with the new store.
    Instruction* NewOne = NewStore;
    std::swap(ToOpt.back(), NewOne);

    for (auto& I : StoresToMerge) {
        AStoreInst ST = AStoreInst::get(std::get<0>(I)).value();
        Value* Ptr = ST.getPointerOperand();
        // Stores merged in the previous iterations can get merged again, so we need
        // to update ToOpt vector to avoid null instruction in there
        ToOpt.erase(std::remove(ToOpt.begin(), ToOpt.end(), ST.inst()), ToOpt.end());
        ST.inst()->eraseFromParent();
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
bool MemOpt::isSafeToMergeLoad(const ALoadInst& Ld,
    const SmallVectorImpl<Instruction*>& CheckList) const {
    MemoryLocation A = getLocation(Ld.inst(), TLI);

    for (auto* I : CheckList) {
        // Skip instructions never writing to memory.
        if (!I->mayWriteToMemory())
            continue;

        MemoryLocation B = getLocation(I, TLI);

        if (!A.Ptr || !B.Ptr || AA->alias(A, B))
            return false;
    }

    return true;
}

/// isSafeToMergeStores() - checks whether there is any alias from the
/// specified store set to any one in the check list, which may read/write to
/// that location.
bool MemOpt::isSafeToMergeStores(
    const SmallVectorImpl<std::tuple<Instruction*, int64_t, MemRefListTy::iterator> >& Stores,
    const SmallVectorImpl<Instruction*>& CheckList) const {
    // Arrange CheckList as the outer loop to favor the case where there are
    // back-to-back stores only.
    for (auto* I : CheckList) {
        if (I->getMetadata(LLVMContext::MD_invariant_load))
            continue;

        MemoryLocation A = getLocation(I, TLI);

        for (auto& S : Stores) {
            MemoryLocation B = getLocation(std::get<0>(S), TLI);

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
    if (auto ALI = ALoadInst::get(I); ALI.has_value())
        Ptr = ALI->getPointerOperand();
    else if (auto ASI = AStoreInst::get(I); ASI.has_value())
        Ptr = ASI->getPointerOperand();

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
    if (auto ALI = ALoadInst::get(I); ALI.has_value())
        Ptr = ALI->getPointerOperand();
    else if (auto ASI = AStoreInst::get(I); ASI.has_value())
        Ptr = ASI->getPointerOperand();

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

// getConstantOffset - Return the constant offset between two memory
// locations.
bool SymbolicPointer::getConstantOffset(SymbolicPointer& Other, int64_t& Off) {
    Term* DiffTerm = nullptr;
    Term* DiffOtherTerm = nullptr;

    // Find how many differences there are between the two vectors of terms.
    auto findDifferences = [&](SmallVector<Term, 8>& Terms1, SmallVector<Term, 8>& Terms2) -> int {
        int DiffCount = 0;
        for (unsigned i = 0, e = Terms1.size(); i != e; ++i) {
            bool Found = false;
            for (unsigned j = 0, f = Terms2.size(); !Found && j != f; ++j)
                if (Terms1[i] == Terms2[j])
                    Found = true;

            if (!Found) {
                DiffCount++;
                if (DiffCount > 1)
                    break;

                DiffTerm = &Terms1[i];
            }
        }

        // If there are no differences, no need to check further.
        if (DiffCount == 0)
            return DiffCount;

        for (unsigned i = 0, e = Terms2.size(); i != e; ++i) {
            bool Found = false;
            for (unsigned j = 0, f = Terms1.size(); !Found && j != f; ++j)
                if (Terms2[i] == Terms1[j])
                    Found = true;

            if (!Found) {
                DiffOtherTerm = &Terms2[i];
                break;
            }
        }

        return DiffCount;
    };

    if (!BasePtr || !Other.BasePtr)
        return true;

    if (BasePtr != Other.BasePtr &&
        (!isa<ConstantPointerNull>(BasePtr) ||
            !isa<ConstantPointerNull>(Other.BasePtr)))
        return true;

    if (Terms.size() != Other.Terms.size())
        return true;

    int DiffCount = findDifferences(Terms, Other.Terms);

    if (DiffCount > 1)
        return true;

    Off = Offset - Other.Offset;

    if (DiffCount == 0)
        return false;

    if (checkTerms(DiffTerm, DiffOtherTerm, Off))
        return true;

    return false;
}

// Try to match the pattern that can't be processed by the current decomposePointer algorithm.
//   First chain:
//   %145 = add nsw i32 %102, 1
//   %146 = sub nsw i32 %145, %const_reg_dword18
//
//   Second chain:
//   %176 = add nsw i32 %102, 2
//   %177 = sub nsw i32 %176, %const_reg_dword18
bool SymbolicPointer::checkTerms(const Term* T, const Term* OtherT, int64_t& Off) const {
    bool IsPositive = true;
    size_t OpNum = 0;

    // Check that the instructions are add or sub with nsw flag.
    auto checkInstructions = [&](const BinaryOperator* Inst0, const BinaryOperator* Inst1) -> bool {
        if (!Inst0 || !Inst1)
            return true;

        if (Inst1->getOpcode() != Inst0->getOpcode())
            return true;

        if (Inst0->getOpcode() != Instruction::Add && Inst0->getOpcode() != Instruction::Sub)
            return true;

        if (!Inst0->hasNoSignedWrap() || !Inst1->hasNoSignedWrap())
            return true;

        if (Inst0->getOperand(0) != Inst1->getOperand(0) &&
            Inst0->getOperand(1) != Inst1->getOperand(1))
            return true;

        if (Inst0->getOperand(0) == Inst1->getOperand(0) &&
            Inst0->getOperand(1) == Inst1->getOperand(1)) {
            OpNum = 3;
            return false;
        }

        if (Inst0->getOperand(0) == Inst1->getOperand(0)) {
            OpNum = 1;
        } else {
            OpNum = 0;
        }

        if (Inst0->getOpcode() == Instruction::Sub)
            if (Inst0->getOperand(0) == Inst1->getOperand(0))
                IsPositive = !IsPositive;

        return false;
    };

    if (!T || !OtherT)
        return true;

    auto* Inst = dyn_cast<BinaryOperator>(T->Idx.getPointer());
    auto* OtherInst = dyn_cast<BinaryOperator>(OtherT->Idx.getPointer());
    if (checkInstructions(Inst, OtherInst))
        return true;

    auto InstOp0 = dyn_cast<BinaryOperator>(Inst->getOperand(OpNum));
    auto OtherInstOp0 = dyn_cast<BinaryOperator>(OtherInst->getOperand(OpNum));
    if (checkInstructions(InstOp0, OtherInstOp0))
        return true;

    if (OpNum == 3)
        return false;

    auto ConstInt  = dyn_cast<ConstantInt>(InstOp0->getOperand(OpNum));
    auto OtherConstInt = dyn_cast<ConstantInt>(OtherInstOp0->getOperand(OpNum));
    if (!ConstInt || !OtherConstInt)
        return true;

    int64_t NewScale = T->Scale;
    int64_t NewOtherScale = OtherT->Scale;
    if (!IsPositive) {
        NewScale = -NewScale;
        NewOtherScale = -NewOtherScale;
    }

    Off += ConstInt->getSExtValue() * NewScale - OtherConstInt->getSExtValue() * NewOtherScale;
    return false;
}

// Save Term in the vector of terms.
 void SymbolicPointer::saveTerm(Value* Src, int64_t IndexScale, uint64_t Scale, int64_t IndexOffset, ExtensionKind Extension, unsigned int ptrSize) {;
    this->Offset += IndexOffset * Scale;
    Scale *= IndexScale;

    SymbolicIndex Idx(Src, Extension);

    // If we already had an occurrence of this index variable, merge this
    // scale into it.  For example, we want to handle:
    //   A[x][x] -> x*16 + x*4 -> x*20
    // This also ensures that 'x' only appears in the index list once.
    for (unsigned i = 0, e = this->Terms.size(); i != e; ++i) {
        if (this->Terms[i].Idx == Idx) {
            Scale += this->Terms[i].Scale;
            this->Terms.erase(this->Terms.begin() + i);
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
        this->Terms.push_back(Entry);
    }
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
                SymPtr.saveTerm(Src, IndexScale.getSExtValue(), Scale, IndexOffset.getSExtValue(), Extension, ptrSize);

                Ptr = BasePtr;
            }

            SymPtr.BasePtr = Ptr;
            return false;
        }

        // Don't attempt to analyze GEPs over unsized objects.
        if (!GEPOp->getSourceElementType()->isSized()) {
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
                SymPtr.saveTerm(new_Ind, IndexScale.getSExtValue(), Scale, IndexOffset.getSExtValue(), Extension, ptrSize);
            }
        }

        // Analyze the base pointer next.
        Ptr = GEPOp->getOperand(0);
    } while (--MaxLookup);

    return true;
}


// Debugging
//#define _LDST_DEBUG 1
#undef _LDST_DEBUG
#if defined(_LDST_DEBUG)
static int _bundleid = 0;
#endif

namespace {
    enum class AddressModel {
        BTS, A32, SLM, A64
    };

    class LdStInfo {
        // Load (or load intrinsic) for loadCombine().
        // store (or store intrinsic) for storeCombine.
        Instruction* Inst;
        // Byte offset of 'Inst'->getPointerOperand() relative to
        // that of the leading load/store inst.
        int64_t      ByteOffset;
        bool IsStore;

    public:
        LdStInfo(Instruction* aI, int64_t aBO) : Inst(aI), ByteOffset(aBO) {
            auto ASI = AStoreInst::get(aI);
            IsStore = ASI.has_value();
        }

        Type* getLdStType() const;
        uint32_t getAlignment() const;
        AddressModel getAddressModel(CodeGenContext* Ctx) const;
        Value* getValueOperand() const;
        bool isStore() const { return IsStore; }
        int64_t getByteOffset() const { return ByteOffset; }
        Instruction* getInst() const { return Inst; }
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

        void print(raw_ostream& O, int BundleID = 0) const;
        void dump() const;
    };

    typedef SmallVector<uint32_t, 8> BundleSize_t;

    enum class LdStKind { IS_STORE, IS_LOAD };

    // BundleConfig:
    //    To tell what vector size is legit. It may need GEN platform as input.
    class BundleConfig {
    public:
        enum {
            STORE_DEFAULT_BYTES_PER_LANE = 16, // 4 DW for non-uniform
            LOAD_DEFAULT_BYTES_PER_LANE = 16   // 4 DW for non-uniform
        };

        BundleConfig(LdStKind K, int ByteAlign, bool Uniform,
            const AddressModel AddrModel, CodeGenContext* Ctx)
        {
            uint32_t maxBytes = 0;
            if (K == LdStKind::IS_STORE)
                maxBytes = getMaxStoreBytes(Ctx);
            else
                maxBytes = getMaxLoadBytes(Ctx);

            auto calculateSize = [this, maxBytes](bool Uniform)
            {
                int sz = (int)m_currVecSizeVar->size();
                if (Uniform) {
                    return (uint32_t)sz;
                }
                int ix = 0;
                for (; ix < sz; ++ix) {
                    uint32_t currBytes = (*m_currVecSizeVar)[ix] * m_eltSizeInBytes;
                    if (currBytes > maxBytes) {
                        break;
                    }
                }
                return (uint32_t)(ix > 0 ? ix : 1);
            };

            if (Ctx->platform.LSCEnabled()) {
                if (ByteAlign >= 8) {
                    m_currVecSizeVar =
                        Uniform ? &m_d64VecSizes_u : &m_d64VecSizes;
                    m_eltSizeInBytes = 8;
                    m_actualSize = calculateSize(Uniform);
                }
                else if (ByteAlign == 4) {
                    m_currVecSizeVar =
                        Uniform ? &m_d32VecSizes_u : &m_d32VecSizes;
                    m_eltSizeInBytes = 4;
                    m_actualSize = calculateSize(Uniform);
                }
                else {
                    m_currVecSizeVar =
                        Uniform ? &m_d8VecSizes_u : &m_d8VecSizes;
                    m_eltSizeInBytes = 1;
                    m_actualSize = (uint32_t)m_currVecSizeVar->size();
                }
            }
            else {
                m_currVecSizeVar = &m_vecSizeVar;
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
                    m_actualSize = (uint32_t)m_vecSizeVar.size();
                }
                else {
                    if (ByteAlign >= 8 && AddrModel == AddressModel::A64) {
                        m_vecSizeVar = { 2, 4 };  // QW scattered read/write
                        m_eltSizeInBytes = 8;
                        m_actualSize = calculateSize(Uniform);
                    }
                    else if (ByteAlign < 4) {
                        m_vecSizeVar = { 2, 4 };  // Byte scattered read/write
                        m_eltSizeInBytes = 1;
                        m_actualSize = m_vecSizeVar.size();
                    }
                    else {
                        m_vecSizeVar = { 2, 3, 4 };  // untyped read/write
                        m_eltSizeInBytes = 4;
                        m_actualSize = calculateSize(Uniform);
                    }
                }
            }
            m_currIndex = 0;
        }

        uint32_t getAndUpdateVecSizeInBytes(uint32_t Bytes) {
            const BundleSize_t& Var = *m_currVecSizeVar;
            int sz = (int)getSize();
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
            return Var[getSize()-1] * m_eltSizeInBytes;
        }

        uint32_t getCurrVecSize() const {
            const BundleSize_t& Var = *m_currVecSizeVar;
            IGC_ASSERT(0 <= m_currIndex && (int)getSize() > m_currIndex);
            return Var[m_currIndex];
        }

        uint32_t getSize() const { return m_actualSize; }

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
        // m_currIndex is initialized to zero.
        // m_actualSize is the actual size of BundleSize variable to use
        //   and it is no larger than the variable's capacity.
        int                 m_currIndex;
        int                 m_actualSize;
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
        TargetLibraryInfo* m_TLI;

    public:
        static char ID;

        LdStCombine()
            : FunctionPass(ID)
            , m_DL(nullptr), m_AA(nullptr), m_WI(nullptr), m_CGC(nullptr)
            , m_F(nullptr), m_TLI(nullptr), m_hasLoadCombined(false), m_hasStoreCombined(false)
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

        //
        // Caching
        //
        // If true, IGC needs to emulate I64.
        bool m_hasI64Emu = false;

        //
        // Temporary reused for each BB.
        //
        // Inst order within a BB.
        DenseMap<const Instruction*, int> m_instOrder;
        // Per-BB: all insts that have been combined and will be deleted.
        DenseMap<const Instruction*, int> m_combinedInsts;
        // All root instructions (ie their uses are empty, including stores)
        // that are to be deleted at the end of each BB.
        SmallVector<Instruction*, 16> m_toBeDeleted;
        void appendToBeDeleted(Instruction* I) {
            if (I != nullptr)
                m_toBeDeleted.push_back(I);
        }
        // Control the way that a load/store is handled.
        // [more for future improvement]
        DenseMap<const Instruction*, int> m_visited;

        // a bundle : a group of loads or a group of store.
        // Each bundle will be combined into a single load or single store.
        std::list<BundleInfo> m_bundles;

        void init(BasicBlock* BB) {
            m_visited.clear();
            m_instOrder.clear();
            m_combinedInsts.clear();
        }
        void setInstOrder(BasicBlock* BB);
        void setVisited(Instruction* I) { m_visited[I] = 1; }
        bool isVisited(const Instruction* I) const {
            return m_visited.count(I) > 0;
        }

        // store combining top function
        void combineStores();

        // load combining top function
        void combineLoads();

        void createBundles(BasicBlock* BB, InstAndOffsetPairs& Stores);

        // Actually combining stores.
        void createCombinedStores(BasicBlock* BB);

        // Actually combining loads.
        void createCombinedLoads(BasicBlock* BB);

        // If V is vector, get all its elements (may generate extractElement
        //   insts; if V is not vector, just V itself.
        void getOrCreateElements(Value* V,
            SmallVector<Value*, 16>& EltV, Instruction* InsertBefore);
        // Return true if V is vector and splitting is beneficial.
        bool splitVectorType(Value* V, LdStKind K) const;
        bool splitVectorTypeForGather(Value* V) const {
            return splitVectorType(V, LdStKind::IS_STORE);
        }
        bool splitVectorTypeForScatter(Value* V) const {
            return splitVectorType(V, LdStKind::IS_LOAD);
        }

        void AllowDummyLoadCoalescing(const InstAndOffsetPairs& Loads);

        // GatherCopy:
        //   copy multiple values (arg: Vals) into a single Dst (return value)
        //   (It's a packed copy, thus size(all Vals) = size(Dst).
        Value* gatherCopy(
          const uint32_t DstEltBytes,
          int NElts,
          SmallVector<Value*, 16>& Vals,
          Instruction* InsertBefore);

        // scatterCopy:
        //   copy components of a single value (arg: CompositeVal) into
        //   multiple values (arg: Vals)
        void scatterCopy(
            SmallVector<Value*, 16>& Vals,
            int DstEltBytes,
            int NElts,
            Value* CompositeVal,
            Instruction* InsertBefore);

        // StructToVec:
        //   convert a struct type to a vector type.
        //     structVal -> <nelts x eltBytes>
        Value* structToVec(IGCIRBuilder<>* irBuilder, BasicBlock* BB, Value* structVal, unsigned eltBytes, unsigned nelts);

        // Helper functions
        bool hasAlias(AliasSetTracker& AST, MemoryLocation& MemLoc);

        // Create unique identified struct type
        StructType* getOrCreateUniqueIdentifiedStructType(
            ArrayRef<Type*> EltTys, bool IsSOA, bool IsPacked = true);

        uint32_t getNumElements(Type* Ty) const {
            return Ty->isVectorTy()
                ? (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;
        }

        Type* generateLoadType(SmallVector<Value*, 16>& Vals,
            uint32_t ValEltBytes, uint32_t ValNElts);

        // For layout struct (at most 2 level), given the current member
        // position specified my Indices, advance Indices to the next member.
        // Return value:
        //   false : if the current member is already the last;
        //    true : otherwise.
        bool advanceStructIndices(SmallVector<uint32_t, 2>& Indices,
            StructType* StTy);

        // Skip counting those insts as no code shall be emitted for them.
        bool skipCounting(Instruction* I) {
            if (auto* IntrinsicI = dyn_cast<llvm::IntrinsicInst>(I)) {
                if (IntrinsicI->getIntrinsicID() == Intrinsic::assume)
                    return true;
            }
            return isDbgIntrinsic(I) || isa<BitCastInst>(I);
        }

        // For generating better code
        bool getVecEltIfConstExtract(Value* V, SmallVector<Value*, 8>& EltV);
        void mergeConstElements(
            SmallVector<Value*, 4>& EltVals, uint32_t MaxMergeBytes);

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

bool IGC::doLdStCombine(const CodeGenContext* CGC) {
    if (CGC->type == ShaderType::OPENCL_SHADER) {
       auto oclCtx = (const OpenCLProgramContext*)CGC;
       // internal flag overrides IGC key
       switch (oclCtx->m_InternalOptions.LdStCombine) {
       default:
           break;
       case 0:
           return false;
       case 1:
           return CGC->platform.LSCEnabled();
       case 2:
           return true;
       }
    }
    uint32_t keyval = IGC_GET_FLAG_VALUE(EnableLdStCombine);
    if ((keyval & 0x3) == 1 && !CGC->platform.LSCEnabled())
        return false;
    return ((keyval & 0x3) || (keyval & 0x4));
}

uint32_t IGC::getMaxStoreBytes(const CodeGenContext* CGC) {
    if (CGC->type == ShaderType::OPENCL_SHADER) {
       auto oclCtx = (const OpenCLProgramContext*)CGC;
       // internal flag overrides IGC key
       if (oclCtx->m_InternalOptions.MaxStoreBytes != 0)
           return oclCtx->m_InternalOptions.MaxStoreBytes;
    }

    uint32_t bytes = IGC_GET_FLAG_VALUE(MaxStoreVectorSizeInBytes);
    if (bytes == 0 &&
        (IGC_IS_FLAG_ENABLED(EnableVector8LoadStore) ||
         CGC->type == ShaderType::RAYTRACING_SHADER ||
         CGC->hasSyncRTCalls()) &&
        CGC->platform.supports8DWLSCMessage()) {
        // MaxStoreVectorSizeInBytes isn't set and it is RT
        // EnableVector8LoadStore from memopt is supported as well
        bytes = 32; // 8 DW
    }
    else if (!(bytes >= 4 && bytes <= 32 && isPowerOf2_32(bytes))) {
        // Use default if bytes from the key is not set or invalid
        bytes = BundleConfig::STORE_DEFAULT_BYTES_PER_LANE;
    }
    return bytes;
}

uint32_t IGC::getMaxLoadBytes(const CodeGenContext* CGC) {
    if (CGC->type == ShaderType::OPENCL_SHADER) {
        auto oclCtx = (const OpenCLProgramContext*)CGC;
        // internal flag overrides IGC key
        if (oclCtx->m_InternalOptions.MaxLoadBytes != 0)
            return oclCtx->m_InternalOptions.MaxLoadBytes;
    }

    uint32_t bytes = IGC_GET_FLAG_VALUE(MaxLoadVectorSizeInBytes);
    if (bytes == 0 &&
        (IGC_IS_FLAG_ENABLED(EnableVector8LoadStore) ||
         CGC->type == ShaderType::RAYTRACING_SHADER ||
         CGC->hasSyncRTCalls()) &&
        CGC->platform.supports8DWLSCMessage()) {
        // MaxLoadVectorSizeInBytes isn't set and it is RT
        // EnableVector8LoadStore from memopt is supported as well
        bytes = 32; // 8 DW
    }
    // Use default if bytes from the key is not set or invalid
    else if (!(bytes >= 4 && bytes <= 32 && isPowerOf2_32(bytes))) {
        // Use default if bytes from the key is not set or invalid
        bytes = BundleConfig::LOAD_DEFAULT_BYTES_PER_LANE;
    }
    return bytes;
}

FunctionPass* IGC::createLdStCombinePass() {
    return new LdStCombine();
}

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#undef DEBUG_TYPE

#define DEBUG_TYPE "LdStCombine"
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
    if (!isStore())
        return ALoadInst::get(Inst)->getType();

    return AStoreInst::get(Inst)->getValueOperand()->getType();
}

uint32_t LdStInfo::getAlignment() const
{
    if (!isStore())
        return (uint32_t)ALoadInst::get(Inst)->getAlignmentValue();

    return (uint32_t)AStoreInst::get(Inst)->getAlignmentValue();
}

Value* LdStInfo::getValueOperand() const
{
    if (!isStore())
        return Inst;

    return AStoreInst::get(Inst)->getValueOperand();
}

AddressModel LdStInfo::getAddressModel(CodeGenContext* Ctx) const
{
    Value* Ptr = nullptr;
    if (!isStore())
        Ptr = ALoadInst::get(Inst)->getPointerOperand();
    else
        Ptr = AStoreInst::get(Inst)->getPointerOperand();

    PointerType* PTy = cast<PointerType>(Ptr->getType());
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
        if (AS.isForwardingAliasSet())
            continue;
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

bool LdStCombine::advanceStructIndices(
    SmallVector<uint32_t, 2>& Indices, StructType* StTy)
{
    IGC_ASSERT_MESSAGE(Indices[0] < StTy->getNumElements(),
        "Indices should be valid on entry to this function!");
    Type* Ty1 = StTy->getElementType(Indices[0]);
    if (Ty1->isStructTy()) {
        StructType* subStTy = cast<StructType>(Ty1);
        uint32_t nextIdx = Indices[1] + 1;
        if (nextIdx == subStTy->getNumElements()) {
            nextIdx = 0;
            Indices[0] += 1;
        }
        Indices[1] = nextIdx;
    }
    else {
        Indices[0] += 1;
    }
    return Indices[0] < StTy->getNumElements();
}

bool LdStCombine::runOnFunction(Function& F)
{
    m_CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

    // If EnableLdStCombine = 2, do it for both lsc and legacy messages.
    // The plan is to do it for LSC message only, ie, EnableLdStCombine=1.
    uint32_t keyval = IGC_GET_FLAG_VALUE(EnableLdStCombine);
    if (F.hasOptNone() ||
        ((keyval & 0x1) == 1 && !m_CGC->platform.LSCEnabled()))
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

    // i64Emu: mimic Emu64Ops's enabling condition. Seems conservative
    //         but can be improved in the future if needed.
    m_hasI64Emu = (m_CGC->platform.need64BitEmulation() &&
            (IGC_GET_FLAG_VALUE(Enable64BitEmulation) ||
             IGC_GET_FLAG_VALUE(Enable64BitEmulationOnSelectedPlatform)));

    combineStores();

    combineLoads();

    bool changed = (m_hasLoadCombined || m_hasStoreCombined);
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

    const int32_t nelts = getNumElements(VTy);
    EltV.resize(nelts, UndefValue::get(VTy->getScalarType()));
    Value* ChainVal = V;
    while (!isa<Constant>(ChainVal)) {
        InsertElementInst* IEI = dyn_cast<InsertElementInst>(ChainVal);
        if (!IEI || !isa<ConstantInt>(IEI->getOperand(2))) {
            break;
        }
        ConstantInt* CInt = cast<ConstantInt>(IEI->getOperand(2));
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
            Value* v = builder.CreateExtractElement(V, builder.getInt32(i));
            EltV[i] = v;
        }
    }
}

// Return value:
//   true:
//     if V is a vector and it is only used in ExtractElement with const index.
//     'EltV has all its elements.
//   false: otherwise.  'EltV' has 'V' only.
// Note: unused elements are returned as UndefValue.
bool LdStCombine::getVecEltIfConstExtract(Value* V, SmallVector<Value*, 8>& EltV)
{
    auto useOrigVector = [&EltV, V]() {
        EltV.clear();
        EltV.push_back(V);
    };

    Type* Ty = V->getType();
    VectorType* VTy = dyn_cast<VectorType>(Ty);
    if (!VTy) {
        useOrigVector();
        return false;
    }

    uint32_t N = getNumElements(VTy);
    Value* undef = UndefValue::get(Ty->getScalarType());
    EltV.assign(N, undef);
    for (auto UI : V->users()) {
        ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(UI);
        if (!EEI) {
            useOrigVector();
            return false;
        }
        ConstantInt *CI = dyn_cast<ConstantInt>(EEI->getIndexOperand());
        if (!CI) {
            useOrigVector();
            return false;
        }
        uint32_t ix = (uint32_t)CI->getZExtValue();
        if (!isa<UndefValue>(EltV[ix])) {
            useOrigVector();
            return false;
        }
        EltV[ix] = EEI;
    }
    return true;
}

void LdStCombine::combineStores()
{
    // All store candidates with addr = common-base + const-offset
    //   All stores have the same common-base but different const-offset.
    InstAndOffsetPairs Stores;

    auto isStoreCandidate = [&](Instruction* I)
    {
        if (std::optional<AStoreInst> SI = AStoreInst::get(I); SI.has_value())
        {
            // Sanity check
            Type* eTy = SI->getValueOperand()->getType()->getScalarType();
            if (!isPowerOf2_32((uint32_t)m_DL->getTypeStoreSize(eTy))) {
                return false;
            }

            // Only original, not-yet-visited store can be candidates.
            const bool isOrigSt = (m_instOrder.size() == 0 ||
                                   m_instOrder.count(I) > 0);
            uint32_t eBytes = (uint32_t)m_DL->getTypeStoreSize(eTy);
            const bool legitSize = isPowerOf2_32(eBytes);
            return (isOrigSt && !isVisited(I) && SI->isSimple());
        }
        return false;
    };

    // If all Stores can move down across I, return true;
    // otherwise, return false.
    auto canCombineStoresAcross = [this](AliasSetTracker& aAST, Instruction* I)
    {
        // Can't combine for non-debug fence like instructions
        if (I->isFenceLike() && !IsDebugInst(I))
            return false;

        if (ALoadInst::get(I).has_value() ||
            AStoreInst::get(I).has_value() ||
            I->mayReadOrWriteMemory()) {
            MemoryLocation memloc = getLocation(I, m_TLI);
            return !hasAlias(aAST, memloc);
        }
        return true;
    };

    // If 'aI' with offset 'aStart' overlaps with any store in aStores,
    // return true; otherwise, return false.
    // Note: once we know the offset is constant, this checking is precise
    //       and better than using alias analysis (basicaa).
    auto hasOverlap = [this](InstAndOffsetPairs& aStores,
        Instruction* aI, int64_t aStart) {
        std::optional<AStoreInst> aSI = AStoreInst::get(aI);
        if (!aSI.has_value())
            return true;
        Type* Ty = aSI->getValueOperand()->getType();
        uint32_t TyBytes = (uint32_t)m_DL->getTypeStoreSize(Ty);
        int64_t aEnd = aStart + TyBytes;
        // 'aSI' byte range [aStart, aEnd)
        for (auto& lsinfo : aStores) {
            IGC_ASSERT(lsinfo.isStore());
            Type* aTy = lsinfo.getLdStType();
            uint32_t aTyBytes = (uint32_t)m_DL->getTypeStoreSize(aTy);
            // 'lsinfo' byte range: [thisStart, thisEnd)
            int64_t thisStart = lsinfo.getByteOffset();
            int64_t thisEnd = thisStart + aTyBytes;
            if ((aStart >= thisStart && aStart < thisEnd) ||
                (thisStart >= aStart && thisStart < aEnd))
                return true;
        }
        return false;
    };

    // Only handle stores within the given instruction window.
    constexpr uint32_t WINDOWSIZE = 150;
    m_hasStoreCombined = false;
    for (auto& BB : *m_F)
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

            uint32_t numInsts = 1;
            Stores.push_back(LdStInfo(base, 0));

            // Keep store candidates for checking alias to see if those
            // stores can be moved to the place of the last store.
            auto batchAARes = IGCLLVM::AliasAnalysis::createAAresults(m_AA);
            AliasSetTracker AST = IGCLLVM::createAliasSetTracker(batchAARes);

            AST.add(base);
            for (auto JI = std::next(II); JI != IE; ++JI) {
                Instruction* I = &*JI;
                if (!skipCounting(I))
                    ++numInsts;
                if (numInsts > WINDOWSIZE)
                    break;

                // Check if any store in AST may be aliased to I
                bool mayAlias = (!canCombineStoresAcross(AST, I));

                int64_t offset;
                if (isStoreCandidate(I) &&
                    getAddressDiffIfConstant(base, I, offset, m_symEval)) {
                    // If both mayAlias and hasOverlap are true, stop
                    if (mayAlias && hasOverlap(Stores, I, offset))
                        break;

                    // if predicates are different - stop
                    if (AStoreInst::get(base)->getPredicate() !=
                        AStoreInst::get(I)->getPredicate())
                        break;

                    Stores.push_back(LdStInfo(I, offset));
                    AST.add(I);
                }
                else if (mayAlias) {
                    break;
                }
            }

            // Create bundles from those stores.
            //   Note: createBundles() will markt all stores as visited when
            //         it is returend, meaning each store is considered only
            //         once. For example,
            //     store a
            //     store b
            //     store c        // alias to store a
            //     store d
            //   As 'store c' aliases to 'store a', candidate 'Stores' stop
            //   growing at 'store c', giving the first set {a, b} for
            //   combining. Even if {a, b} cannot be combined, but {b, c, d}
            //   can; it will go on with the next candidate set {c, d},  not
            //   {b, c, d}; missing opportunity to combine {b, c, d}.
            //   So far, this is fine as this case isn't important.
            createBundles(&BB, Stores);
        }

        // Actually combining them.
        createCombinedStores(&BB);
    }
}

void LdStCombine::combineLoads()
{
    // this check's for testing, and to be removed when stable.
    if ((IGC_GET_FLAG_VALUE(EnableLdStCombine) & 0x4) == 0)
        return;

    if (m_CGC->type != ShaderType::OPENCL_SHADER)
    {
        if (!m_CGC->getModuleMetaData()->compOpt.EnableLdStCombineforLoad)
        {
            return;
        }
    }

    // All load candidates with addr = common-base + const-offset
    InstAndOffsetPairs Loads;

    auto isLoadCandidate = [&](Instruction* I)
    {
        if (auto LI = ALoadInst::get(I); LI.has_value())
        {
            // Sanity check
            Type* eTy = LI->getType()->getScalarType();
            if (!isPowerOf2_32((uint32_t)m_DL->getTypeStoreSize(eTy))) {
                return false;
            }

            // Only original, not-yet-visited load can be candidates.
            bool isOrigLd = (m_instOrder.size() == 0 ||
                m_instOrder.count(I) > 0);
            return (isOrigLd && !isVisited(I) && LI->isSimple());
        }
        return false;
    };

    // If 'I' can be moved up accross all inst in aAST, return true.
    auto canMoveUp = [this](AliasSetTracker& aAST, Instruction* I)
    {
        if (ALoadInst::get(I).has_value()) {
            MemoryLocation memloc = getLocation(I, m_TLI);
            return !hasAlias(aAST, memloc);
        }
        return true;
    };

    // Only handle loads within the given instruction window.
    constexpr uint32_t LDWINDOWSIZE = 150;
    m_hasLoadCombined = false;
    for (auto& BB : *m_F)
    {
        LLVM_DEBUG(dbgs() << "Process BB: " << BB.getName() << "\n");
        init(&BB);

        auto IE = BB.end();
        for (auto II = BB.begin(); II != IE; ++II)
        {
            Instruction* base = &*II;
            LLVM_DEBUG(dbgs() << "- Process base inst: " << *base << "\n");

            if (!isLoadCandidate(base)) {
                continue;
            }

            uint32_t numInsts = 1;
            Loads.push_back(LdStInfo(base, 0));
            LLVM_DEBUG(dbgs() << "- Added to Loads\n");

            // Keep store/maywritemem/fence insts for checking alias to see if those
            // stores block load candidates from moving to the first (leading) load.
            auto batchAARes = IGCLLVM::AliasAnalysis::createAAresults(m_AA);
            AliasSetTracker AST = IGCLLVM::createAliasSetTracker(batchAARes);

            for (auto JI = std::next(II); JI != IE; ++JI) {
                Instruction* I = &*JI;
                LLVM_DEBUG(dbgs() << "- - Process inst: " << *I << "\n");

                if (!skipCounting(I))
                    ++numInsts;

                // cannot merge beyond fence or window limit
                if ((I->isFenceLike() && !isa<PredicatedLoadIntrinsic>(I) && !isa<PredicatedStoreIntrinsic>(I)) || numInsts > LDWINDOWSIZE) {
                    LLVM_DEBUG(dbgs() << "- - Stop at fence or window limit\n");
                    break;
                }

                if (AStoreInst::get(I).has_value() || I->mayWriteToMemory()) {
                    AST.add(I);
                    LLVM_DEBUG(dbgs() << "- - Added to AST. Continue to next instruction\n");
                    continue;
                }

                if (isLoadCandidate(I)) {
                    LLVM_DEBUG(dbgs() << "- - It is load candidate\n");
                    int64_t offset;
                    if (getAddressDiffIfConstant(base, I, offset, m_symEval)) {
                        LLVM_DEBUG(dbgs() << "- - Found offset: " << offset << "\n");
                        if (canMoveUp(AST, I)) {
                            LLVM_DEBUG(dbgs() << "- - Can move up\n");

                            // if predicates are different - stop
                            if (ALoadInst::get(base)->getPredicate() !=
                                ALoadInst::get(I)->getPredicate()) {
                                LLVM_DEBUG(dbgs() << "- - Predicates are different. Stop\n");
                                break;
                            }

                            Loads.push_back(LdStInfo(I, offset));
                            LLVM_DEBUG(dbgs() << "- - Added to Loads\n");
                        } else {
                            // If it cannot be moved up, either keep going or
                            // stopping.  Choose stop for now.
                            LLVM_DEBUG(dbgs() << "- - Cannot move up. Stop\n");
                            break;
                        }
                    }
                }
            }

            //Experiment: If its the last element of the load and does not fit the DWORD alignment,
            //It creates a dummy load with the same alignment type as the previous load
            if (m_CGC->type != ShaderType::OPENCL_SHADER)
            {
                if (m_CGC->getModuleMetaData()->compOpt.EnableLdStCombinewithDummyLoad) {
                    LLVM_DEBUG(dbgs() << "- - Allow dummy load coalescing\n");
                    AllowDummyLoadCoalescing(Loads);
                }
            }

            //   Note: For now, each load is considered once. For example,
            //     load a
            //       store x : alias to load c
            //     load b
            //     load c
            //     load d
            //   As 'load c' aliases to 'store x', candidate 'Loads' stop
            //   growing at 'load b', giving the first set {a, b}. Even
            //   though {a, b} cannot be combined, 'load b' will not be
            //   reconsidered for a potential merging of {b, c, d}.
            //
            // This is controlled by setting visited. A better way of setting
            // visited can overcome the above issue.
            createBundles(&BB, Loads);
        }

        // Actually combining them.
        createCombinedLoads(&BB);
    }
}

void LdStCombine::createBundles(BasicBlock* BB, InstAndOffsetPairs& LoadStores)
{
    LLVM_DEBUG(dbgs() << "Create bundles for " << LoadStores.size() << " instructions\n");
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
            // sanity check
            if (!(eTy->isIntOrPtrTy() || eTy->isFloatingPointTy()))
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

        void save() {
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

    auto markVisited = [this](InstAndOffsetPairs& LoadStores) {
        int32_t SZ = (int)LoadStores.size();
        for (int i = 0; i < SZ; ++i)
        {
            const LdStInfo* lsi = &LoadStores[i];
            setVisited(lsi->getInst());
        }
        LoadStores.clear();
    };

    int32_t SZ = (int)LoadStores.size();
    if (SZ <= 1) {
        markVisited(LoadStores);
        return;
    }

    auto isBundled = [](const LdStInfo* LSI, DenseMap<const Instruction*, int>& L) {
        return (L.count(LSI->getInst()) > 0);
    };
    auto setBundled = [&isBundled](LdStInfo* LSI,
        DenseMap<const Instruction*, int>& L) {
        if (!isBundled(LSI, L)) {
            L[LSI->getInst()] = 1;
        }
    };

    setInstOrder(BB);

    // Sort loads/stores in the order of increasing ByteOffset
    std::sort(LoadStores.begin(), LoadStores.end(),
        [](const LdStInfo& A, const LdStInfo& B) {
            return A.getByteOffset() < B.getByteOffset();
        });

    const LdStInfo* lsi0 = &LoadStores[0];
    auto LI = ALoadInst::get(lsi0->getInst());
    auto SI = AStoreInst::get(lsi0->getInst());
    LdStKind Kind = LI.has_value() ? LdStKind::IS_LOAD : LdStKind::IS_STORE;
    bool isUniform = false;
    if (m_WI)
    {
        isUniform = m_WI->isUniform(
            LI.has_value() ? LI->getPointerOperand() : SI->getPointerOperand());
    }
    const AddressModel AddrModel = lsi0->getAddressModel(m_CGC);

    // Starting from the largest alignment (favor larger alignment)
    const uint32_t bundleAlign[] = { 8, 4, 1 };
    const uint32_t aligns = (int)(sizeof(bundleAlign)/sizeof(bundleAlign[0]));
    // keep track of the number of unmerged loads
    uint32_t numRemainingLdSt = SZ;
    for (int ix = 0; ix < aligns && numRemainingLdSt > 1; ++ix)
    {
        const uint32_t theAlign = bundleAlign[ix];

        // If i64 insts are not supported, don't do D64 as it might
        // require i64 mov in codegen emit (I64 Emu only handles 1-level
        // insertvalue and extractvalue so far).
        if (m_hasI64Emu && theAlign > 4)
            continue;

        // Use alignment as element size, which maps to gen load/store element
        // size as follows:
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
            const LdStInfo* leadLSI = &LoadStores[i];
            LLVM_DEBUG(llvm::dbgs() << "Try leading LdSt: " << *leadLSI->getInst() << "\n");
            if (isBundled(leadLSI, m_combinedInsts) ||
                (i+1) == SZ) /* skip for last one */ {
                ++i;
                continue;
            }

            if (m_WI && isUniform &&
                !m_WI->isUniform(leadLSI->getValueOperand())) {
                LLVM_DEBUG(llvm::dbgs() << "No combining for *uniform-ptr = non-uniform value\n");
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

            BundleConfig  BC(Kind, theAlign, isUniform, AddrModel, m_CGC);
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
                const LdStInfo* LSI = &LoadStores[j];
                LLVM_DEBUG(llvm::dbgs() << "Try to make bundle with: " << *LSI->getInst() << "\n");
                if (isBundled(LSI, m_combinedInsts) ||
                    (leadLSI->getByteOffset() + totalBytes) != LSI->getByteOffset())
                {
                    // stop as remaining stores are not contiguous
                    break;
                }
                if (m_WI && isUniform &&
                    !m_WI->isUniform(LSI->getValueOperand())) {
                    LLVM_DEBUG(llvm::dbgs() << "No combining for *uniform-ptr = non-uniform value\n");
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

                if (m_hasI64Emu && vecEltBytes == 1 && nextBytes == 8) {
                    // If I64 emu is on, skip D64 as I64 emu would result
                    // in inefficient code.
                    continue;
                }

                if (totalBytes == nextBytes &&
                    !D32OrD64.skip(vecEltBytes, BC.getCurrVecSize())) {
                    e = j;
                    vecSize = BC.getCurrVecSize();

                    D32OrD64.save();
                }
            }

            // If any ldst has byte element, skip D64 to avoid byte mov
            // with dst-stride = 8.
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
                    LdStInfo& tlsi = LoadStores[k];
                    newBundle.LoadStores.push_back(tlsi);
                    setBundled(&tlsi, m_combinedInsts);
                    if (tlsi.isStore()) {
                        appendToBeDeleted(tlsi.getInst());
                    }
                    setVisited(tlsi.getInst());
                }
                i = e + 1;
                numRemainingLdSt -= bundle_nelts;
                if (numRemainingLdSt < 2) {
                    // No enough loads/stores to merge
                    break;
                }
            }
            else {
                ++i;
            }
        }
    }

    markVisited(LoadStores);
}

void LdStCombine::AllowDummyLoadCoalescing(const InstAndOffsetPairs& Loads)
{
    // Currently supports only this pattern.
    // % 164 = add i32 % 114, 1020
    // % 165 = and i32 % 164, 1020
    // % 166 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 165
    // %167 = load half, half addrspace(3) * %166, align 8
    // % 168 = or i32 % 165, 1
    // % 169 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 168
    // % 170 = load half, half addrspace(3) * %169, align 2
    // % 171 = or i32 % 165, 2
    // % 172 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 171
    // % 173 = load half, half addrspace(3) * %172, align 4
    // to
    // % 164 = add i32 % 114, 1020
    // % 165 = and i32 % 164, 1020
    // % 166 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 165
    // %167 = load half, half addrspace(3) * %166, align 8
    // % 168 = or i32 % 165, 1
    // % 169 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 168
    // % 170 = load half, half addrspace(3) * %169, align 2
    // % 171 = or i32 % 165, 2
    // % 172 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 171
    // % 173 = load half, half addrspace(3) * %172, align 4
    // % 174 = add i32 % 165, 3
    // % 175 = getelementptr[1024 x half], [1024 x half] addrspace(3) * null, i32 0, i32 % 174
    // % 176 = load half, half addrspace(3) * %175, align 2
    int size = Loads.size();
    LdStInfo LastLoad = Loads[size - 1];
    uint32_t LastLoadSize = (uint32_t)m_DL->getTypeStoreSize(LastLoad.getInst()->getType());
    uint32_t currLoadSize = LastLoadSize + LastLoad.getByteOffset();
    if (currLoadSize % 4)
    {
        //Replicating the last load to make it DWORD aligned
        uint32_t newLoadSize = LastLoadSize;
        if (!((currLoadSize + newLoadSize) % 4))
        {
            ALoadInst lead = ALoadInst::get(LastLoad.getInst()).value();
            Value* ldPtr = lead.getPointerOperand();
            if (auto gep = dyn_cast<GetElementPtrInst>(ldPtr))
            {
                if ((gep->getNumOperands() == 3) && (isa<ConstantPointerNull>(gep->getPointerOperand())))
                {
                    IGCIRBuilder<> irBuilder(LastLoad.getInst());
                    Value* AddInst = irBuilder.CreateAdd(gep->getOperand(2), irBuilder.getInt32(1));
                    Value* gepArg[] = { gep->getOperand(1), AddInst };
                    Value* Addr = irBuilder.CreateInBoundsGEP(gep->getSourceElementType(),
                        gep->getOperand(0), gepArg);

                    // Create a dummy merge value:
                    Type* Ty = cast<GetElementPtrInst>(Addr)->getResultElementType();
                    Value *mergeValue = nullptr;
                    if (IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
                        mergeValue = ConstantAggregateZero::get(Ty);
                    else
                        mergeValue = Constant::getNullValue(Ty);

                    lead.CreateLoad(irBuilder, Ty, Addr, mergeValue);
                }
            }
        }
    }
    return;
}

// A member of layout struct can be a vector type. This function will decide
// if the vector type or a sequence of its elements' types shall be used as
// the layout struct's member types. If spliting a vector type into a sequence
// of its elements' types is beneficial (ie, likely results in less mov
// instructions), return true; otherwise, return false.
//
// For example:
//
//        Not split <2xi32>       split <2xi32>
//        -----------------       -------------
//     struct SOA {                struct SOA {
//       <2 x i32> x;                i32 x0;
//                                   i32 x1;
//       float     y;                float y;
//       struct AOS {                struct AOS {
//          i16 a, i16 b} z;           i16 a, i16 b} z;
//     }                           }
//
// args:
//   V : value to be checked
//   K : indicate if V is a stored value or a loaded value.
//   (special case: return false if V is null or V is scalar.)
bool LdStCombine::splitVectorType(Value* V, LdStKind K) const
{
    if (V == nullptr) {
        return false;
    }

    Type* Ty = V->getType();
    // Not vector, always return false;
    if (!Ty->isVectorTy()) {
        return false;
    }

    // If vector size isn't packed (store size != alloc size), must split to
    // avoid holes in the layout struct.
    //   For example, alloc size(<3 x i32>) = 16B, not 12B
    //       struct { <3xi32>, float }      : size = 20 Bytes
    //       struct { i32, i32, i32, float} : size = 16 bytes.
    if (!isa<Constant>(V) &&
        m_DL->getTypeStoreSize(Ty) != m_DL->getTypeAllocSize(Ty)) {
        return true;
    }

    Value* val = V;
    if (K == LdStKind::IS_STORE) {
        while (auto IEI = dyn_cast<InsertElementInst>(val)) {
            if (!isa<Constant>(IEI->getOperand(2))) {
                return false;
            }
            val = IEI->getOperand(0);
        }
        if (isa<Constant>(val)) {
            return true;
        }
    }
    else {
        for (auto U : val->users()) {
            Value* user = U;
            if (auto EEI = dyn_cast<ExtractElementInst>(user)) {
                if (isa<Constant>(EEI->getIndexOperand())) {
                    continue;
                }
            }
            return false;
        }
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

    auto isValidConst = [](Value* v){
        return isa<ConstantInt>(v) || isa<ConstantFP>(v) ||
               isa<ConstantPointerNull>(v);
    };

    // Check if it has two consecutive constants, skip if not.
    // This is a quick check to skip for majority of cases.
    bool isCandidate = false;
    for (int i = 0, sz = (int)EltVals.size() - 1; i < sz; ++i) {
        if (isValidConst(EltVals[i]) && isValidConst(EltVals[i + 1])) {
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
            if (!C0 || !C1 || (sz0 + sz1) != b ||
                !isValidConst(C0) || !isValidConst(C1)) {
                currOff += sz0;
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

// This is to make sure to reuse the layout types. Two identified structs have
// the same layout if
//   1. both are SOA or both are AOS; and
//   2. both are packed; and
//   3, element types are matched in order.
StructType* LdStCombine::getOrCreateUniqueIdentifiedStructType(
    ArrayRef<Type*> EltTys, bool IsSOA, bool IsPacked)
{
    auto& layoutStructTypes = m_CGC->getLayoutStructTypes();
    for (auto II : layoutStructTypes) {
        StructType* stTy = II;
        if (IsPacked == stTy->isPacked() &&
            IsSOA == isLayoutStructTypeSOA(stTy) &&
            stTy->elements() == EltTys)
            return stTy;
    }

    // Create one
    StructType* StTy = StructType::create(EltTys,
        IsSOA ? getStructNameForSOALayout() : getStructNameForAOSLayout(),
        IsPacked);
    layoutStructTypes.push_back(StTy);
    return StTy;
}

// gatherCopy():
//   Generate the final value by coalescing given values. The final value is
//   of either struct type or vector type.
// Arguments:
//   DstEltBytes:  size of vector element if the final value is a vector.
//                 If the final value is a struct,  its struct member size
//                 must be the same as DstEltBytes.
//   DstNElts:     the num of elements if the final value is a vector or
//                 the num of direct members if the final value is a struct.
//   Vals:         a list of values to be coalesced into the final value.
//   InsertBefore: inserting pos for new instructions.
//
// DstEltTy: not an argument, but used often in this function and comments.
//     It is the element type if the final value is a vector; or int type if
//     the final value is a struct. For a struct, it could be
//       1.  int64:  DstEltBytes == 8; or  // D64
//       2.  int32:  DstEltBytes == 4, or  // D32
//       3.  int16:  DstEltBytes == 2.     // D16U32
//
// Examples:
//   1. vector type;
//      given DstEltBytes=4 and DstNElts=4
//      Vals = { i32 a, i64 b, int c }
//
//      'b' is split into two i32, the final value is a vector and DstEltTy
//      is i32.
//
//      <4xi32> returnVal = {
//        a,
//        extractElement bitcast (i64 b to <2xi32>), 0
//        extractElement bitcast (i64 b to <2xi32>), 1
//        c
//      };
//   2. struct type (multiple of 4 bytes)
//      given DstNElts x DstEltBytes = 8x4B.
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
//      As DstEltBytes == 4,  DstEltTy is i32.
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
    const uint32_t DstEltBytes,
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

    // remainingBytes:
    //   initialize to be the size of DstEltTy. It is the size of each
    //   member of the struct or vector.
    uint remainingBytes = DstEltBytes;
    while (!worklist.empty()) {
        Value* v = worklist.front();
        worklist.pop_front();

        if (v->getType()->isVectorTy())
        {
            IGC_ASSERT((v->getType()->getScalarSizeInBits() % 8) == 0);
            uint32_t eBytes = (v->getType()->getScalarSizeInBits() / 8);
            uint32_t n = getNumElements(v->getType());

            // true if v is a legal vector at level 1
            bool isLvl1 = (remainingBytes == DstEltBytes && eBytes == DstEltBytes);
            // true if v is a legal vector at level 2
            bool isLvl2 = (remainingBytes >= (eBytes * n));
            bool keepVector = !splitVectorTypeForGather(v);
            if (isLvl1 && keepVector)
            {   // case 1
                // 1st level vector member
                eltVals.push_back(v);
                allEltVals.push_back(eltVals);

                eltVals.clear();
            }
            else if (isLvl2 && keepVector)
            {   // case 2
                // 2nd level vector member
                eltVals.push_back(v);
                remainingBytes -= (eBytes * n);

                if (remainingBytes == 0) {
                    mergeConstElements(eltVals, DstEltBytes);

                    allEltVals.push_back(eltVals);

                    // Initialization for the next element
                    eltVals.clear();
                    remainingBytes = DstEltBytes;
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
            (uint32_t)MinAlign(DstEltBytes, DstEltBytes - remainingBytes);
        if (currAlign < eBytes) {
            // Two cases:
            //   1. DstEltBytes = 4
            //      store i32 p
            //      store i32 p+4
            //      store i64 p+8  <- v : i64
            //     Need to split i64 by casting i64 --> 2xi32
            //   2. DstEltBytes = 4, packed struct
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
                Value* v = irBuilder.CreateExtractElement(new_v, irBuilder.getInt32(i));
                worklist.insert(insPos, v);
            }
            continue;
        }

        // v should fit into this remainingByts as v is element-size aligned.
        IGC_ASSERT(remainingBytes >= eBytes);
        eltVals.push_back(v);
        remainingBytes -= eBytes;
        if (remainingBytes == 0) {
            // Found one element of size DstEltBytes.
            mergeConstElements(eltVals, DstEltBytes);

            allEltVals.push_back(eltVals);

            // Initialization for the next element
            eltVals.clear();
            remainingBytes = DstEltBytes;
        }
    }

    IGC_ASSERT(eltVals.empty());
    Type* DstEltTy = nullptr;

    // A new coalesced value could be one of two types
    //   1 a vector type  < DstNElts x DstEltTy >
    //     If all elements are of the same type (which is DstEltTy). It
    //     could be a float or integer type.
    //   2 a struct type
    //     An integer type is used as DstEltTy whose size is DstEltBytes.
    //     All its members, include struct members, must be this same size.
    //     The struct nesting is at most 2 levels.
    //
    //    More examples:
    //     1) vector type (i64 as element)
    //           store i64 a, p; store i64 b, p+8; store i64 c, p+16
    //        -->
    //           store <3xi64> <a, b, c>,  p
    //
    //        Another example,
    //          store float a, p; store float b, p+4
    //        -->
    //          store <2xfloat> <a,b>, p
    //     2)struct type (i32 as element type)
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
    //          val = call <4xi32> bitcastfromstruct( __StructSOALayout__ %stVal)
    //          store <4xi32> %val, p
    //
    //        The "bitcastfromstruct" is no-op intrinsic (by dessa).
    //
    //        Another example:
    //             store float a, p; store i32 b, p+4
    //          -->
    //             store __StructSOALayout__ <{float a, int b}>, p
    //          Note in this case, we can do
    //             store <2xi32> <bitcast(float a to i32), b>, p
    //          but this will introduce additional bitcast. And struct is
    //          preferred.
    //
    auto isLvl2Vecmember = [this, DstEltBytes](Type* ty) {
        uint32_t n = (uint32_t)m_DL->getTypeStoreSize(ty->getScalarType());
        return ty->isVectorTy() && n < DstEltBytes;
    };

    bool hasStructMember = false;
    bool hasVecMember = false;
    const int32_t sz = (int)allEltVals.size();
    SmallVector<Type*, 16> StructTys;
    for (int i = 0; i < sz; ++i) {
        SmallVector<Value*, 4>& subElts = allEltVals[i];
        int nelts = (int)subElts.size();
        Type* ty = subElts[0]->getType();
        uint32_t eBytes = (uint32_t)m_DL->getTypeStoreSize(ty->getScalarType());
        if (nelts == 1 && !isLvl2Vecmember(ty)) {
            IGC_ASSERT(eBytes == DstEltBytes);
            StructTys.push_back(ty);
            hasVecMember = (hasVecMember || ty->isVectorTy());
        }
        else {
            SmallVector<Type*, 4> subEltTys;
            for (auto II : subElts) {
                Value* elt = II;
                subEltTys.push_back(elt->getType());
                hasVecMember = (hasVecMember || elt->getType()->isVectorTy());
            }

            // create a member of a packed and identified struct type
            // whose size = DstEltBytes. Use AOS layout.
            Type* eltStTy =
                getOrCreateUniqueIdentifiedStructType(subEltTys, false, true);
            StructTys.push_back(eltStTy);

            hasStructMember = true;
        }
    }

    // Check if a vector is preferred for the final value.
    // (For reducing the number of struct types created, and also vector
    //  is better supported in codegen.)
    if (!hasStructMember && !hasVecMember) {
        // Set initial value for DstEltTy.
        // Skip any const as it can be taken as either float or int.
        int i = 0;
        for (; i < sz; ++i) {
            SmallVector<Value*, 4>& subElts = allEltVals[i];
            int nelts = (int)subElts.size();
            IGC_ASSERT(nelts == 1);
            if (!isa<Constant>(subElts[0])) {
                DstEltTy = subElts[0]->getType();
                break;
            }
        }

        if (DstEltTy != nullptr) {
            for (++i; i < sz; ++i) {
                SmallVector<Value*, 4>& subElts = allEltVals[i];
                int nelts = (int)subElts.size();
                IGC_ASSERT(nelts == 1);
                Type* ty = subElts[0]->getType();
                const bool isConst = isa<Constant>(subElts[0]);
                if (!isConst && DstEltTy != ty) {
                    // Use struct is better
                    DstEltTy = nullptr;
                    break;
                }
            }
        }
        else {
            DstEltTy = Type::getIntNTy(m_F->getContext(), DstEltBytes * 8);
        }
    }

    // If DstEltTy != null, use vector; otherwise, use struct as
    // the struct will likely have less mov instructions.
    Type* structTy;
    Value* retVal;
    if (DstEltTy != nullptr)
    {   // case 1
        if (DstNElts == 1) {
            // Constant store values are combined into a single constant
            // for D16U32, D32, D64
            SmallVector<Value*, 4>& eltVals = allEltVals[0];
            IGC_ASSERT(eltVals.size() == 1);
            retVal = eltVals[0];
        }
        else {
            // normal vector
            VectorType* newTy = VectorType::get(DstEltTy, DstNElts, false);
            retVal = UndefValue::get(newTy);
            for (int i = 0; i < sz; ++i) {
                SmallVector<Value*, 4>& eltVals = allEltVals[i];
                Value* tV = irBuilder.CreateBitCast(eltVals[0], DstEltTy);
                retVal = irBuilder.CreateInsertElement(retVal, tV, irBuilder.getInt32(i));
            }
        }
    }
    else
    {   // case 2
        // Packed and named identified struct. Prefix "__" make sure it won't
        // collide with any user types.  Use SOA layout.
        structTy =
            getOrCreateUniqueIdentifiedStructType(StructTys, true, true);

        // Create a value
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
    return retVal;
}

// Given a list of values in order (arg: Vals), return a new packed type
// that is composed of Vals' types. This new type is one of the following:
//   0. if all Vals have the same size of element, the new type will be
//      a vector type with element size = that same size. This is to take
//      advantage of extensive vector optimization in IGC; or
//   1. a vector type with element size = ValEltBytes and the number of
//      elements = ValNElts; or
//   2. a struct type whose direct members are all the same size and are
//      equal to ValEltBytes and the number of direct members = ValNElts.
// Note: this is for load combining as a type is needed before generating
//       component values (store combining does not use this as component
//       values are known before the type).
Type* LdStCombine::generateLoadType(
    SmallVector<Value*, 16>& Vals,
    uint32_t ValEltBytes, uint32_t ValNElts)
{
    // case 0: Optimization
    //   For now, use vector if elements of all Vals are the same size.
    //   Prefer using vector as vector has been well optimized.
    const bool OptimPreferVec = true;
    if (OptimPreferVec && Vals.size() > 1) {
        Type* ETy = Vals[0]->getType()->getScalarType();
        int eBytes = (int)m_DL->getTypeStoreSize(ETy);
        bool isSameEltSize = true;
        for (int i = 1, sz = (int)Vals.size(); i < sz; ++i) {
            Type* ty = Vals[i]->getType()->getScalarType();
            int tBytes = (int)m_DL->getTypeStoreSize(ty);
            if (eBytes != tBytes) {
                isSameEltSize = false;
                break;
            }
        }

        if (isSameEltSize) {
            Type* newETy = Type::getIntNTy(m_F->getContext(), eBytes * 8);
            uint32_t nElts = (ValNElts * ValEltBytes) / eBytes;
            Type* retTy = VectorType::get(newETy, nElts, false);
            return retTy;
        }
    }

    // case 1 and 2
    bool isStructTy = false;
    SmallVector<Type*, 16> tys;
    SmallVector<Type*, 16> subEltTys;
    uint32_t remainingBytes = ValEltBytes;
    std::list<Type*> worklist;
    for (int i = 0, sz = (int)Vals.size(); i < sz; ++i)
    {
        Value* V = Vals[i];
        Type* Ty = V->getType();
        worklist.push_back(Ty);
        while (!worklist.empty())
        {
            Type* Ty = worklist.front();
            worklist.pop_front();
            Type* eTy = Ty->getScalarType();
            uint32_t nElts = getNumElements(Ty);
            uint32_t eBytes = (uint32_t)m_DL->getTypeStoreSize(eTy);

            // true if v is either a vector or a scalar at level 1
            bool isLvl1 = (remainingBytes == ValEltBytes && eBytes == ValEltBytes);
            // true if v is a vector or scalar at level 2
            bool isLvl2 = (remainingBytes >= (eBytes * nElts));
            // It's ok not to split if V == nullptr (not original one from Vals)
            // or if V is one from Vals and splitVectorTypeForScatter() returns
            // false.
            const bool noSplitOK = !splitVectorTypeForScatter(V);

            if (noSplitOK && isLvl1) {
                tys.push_back(Ty);
            }
            else if (noSplitOK && isLvl2) {
                subEltTys.push_back(Ty);
                remainingBytes -= (eBytes * nElts);
                if (remainingBytes == 0) {
                    // struct member.
                    Type* eltStTy =
                        getOrCreateUniqueIdentifiedStructType(subEltTys, false, true);
                    tys.push_back(eltStTy);
                    subEltTys.clear();
                    isStructTy = true;
                    remainingBytes = ValEltBytes;
                }
            }
            else {
                // Split Ty into smaller types if:
                //   1. eBytes > ValEltBytes; or
                //   2. eTy isn't aligned at this offset (cannot generate mov inst)
                //      Ty must be split into a list of smaller types that are aligned.
                // Element size is assumed to be minimum alignment for a type.
                uint32_t currAlign =
                    (uint32_t)MinAlign(ValEltBytes, ValEltBytes - remainingBytes);

                if (currAlign < eBytes) {
                    IGC_ASSERT((eBytes % currAlign) == 0);
                    int n = (eBytes / currAlign) * nElts;
                    Type* newETy = IntegerType::get(m_F->getContext(), currAlign * 8);
                    worklist.insert(worklist.begin(), n, newETy);
                }
                else {
                    worklist.insert(worklist.begin(), nElts, eTy);
                }
                // For next iteration of while, it is for sub-part of V,
                // so set V to nullptr.
                V = nullptr;
            }
        }
    }
    IGC_ASSERT(remainingBytes == ValEltBytes);

    Type* retTy;
    if (isStructTy) {
        retTy = getOrCreateUniqueIdentifiedStructType(tys, true, true);
    } else {
        Type* newEltTy = IntegerType::get(m_F->getContext(), ValEltBytes * 8);
        retTy = VectorType::get(newEltTy, ValNElts, false);
    }
    return retTy;
}

// todo: re-do desc
// Given a list of values (arg: Vals), create a composite type (either
// struct type or vector type). A value of this composite type is loaded,
// and this value is futhter decomposed to the given list of values.
//
void LdStCombine::scatterCopy(
    SmallVector<Value*, 16>& Vals,
    int LoadedValEBytes,
    int LoadedValNElts,
    Value* LoadedVecVal,
    Instruction* InsertBefore)
{
    // To split loadedVal, figure out its type first.
    //   1. Try to use a vector type, if not possible, use a struct type.
    //   2. for each V in Vals, its replacement value is created by mapping
    //    corresponding components of LoadedVal to itself.
    IRBuilder<> irBuilder(InsertBefore);
    Type* LoadedValTy = generateLoadType(Vals, LoadedValEBytes, LoadedValNElts);
    {
        int newTyBytes = (int)m_DL->getTypeStoreSize(LoadedValTy);
        IGC_ASSERT(newTyBytes == (LoadedValNElts * LoadedValEBytes));
    }
    Value* LoadedVal = LoadedVecVal;

    if (LoadedValTy->isStructTy()) {
        // Set loadedVal's name to "StructV" so that both load/store
        // will have names start with "StructV" for layout struct.
        LoadedVal->setName("StructV");
        Type* ITys[2] = { LoadedValTy, LoadedVal->getType() };
        Function* IntrDcl = GenISAIntrinsic::getDeclaration(
            m_F->getParent(), GenISAIntrinsic::ID::GenISA_bitcasttostruct, ITys);
        LoadedVal = irBuilder.CreateCall(IntrDcl, LoadedVal);
    } else if (LoadedValTy != LoadedVal->getType()) {
        LoadedVal = irBuilder.CreateBitCast(LoadedVal, LoadedValTy);
    }

    auto createValueFromElements = [this, &irBuilder] (
        SmallVector<Value*, 8>& Elts, Type* ValueTy)
    {
        IGC_ASSERT(!Elts.empty());
        Value* V0 = Elts[0];
        Type* eTy = V0->getType();
        uint32_t n = (uint32_t)Elts.size();
#if defined(_DEBUG)
        {
            IGC_ASSERT(!Elts.empty());
            Value* V0 = Elts[0];
            for (uint32_t i = 1; i < n; ++i) {
                Value* V = Elts[i];
                if (V0->getType() != V->getType()) {
                    IGC_ASSERT(false);
                }
            }
            uint32_t EltsBytes = (uint32_t)m_DL->getTypeStoreSize(V0->getType());
            EltsBytes *= n;
            IGC_ASSERT(m_DL->getTypeStoreSize(ValueTy) == EltsBytes);
        }
#endif
        Value* retVal;
        if (n == 1) {
            retVal = Elts[0];
            if (eTy != ValueTy) {
                retVal = irBuilder.CreateBitCast(retVal, ValueTy);
            }
        }
        else {
            VectorType* nTy = VectorType::get(eTy, n, false);
            Value* nV = UndefValue::get(nTy);
            for (uint32_t i = 0; i < n; ++i) {
                nV = irBuilder.CreateInsertElement(nV, Elts[i], i);
            }
            retVal = irBuilder.CreateBitCast(nV, ValueTy);
        }
        return retVal;
    };

    // Copy component values from LoadedVal to the original values.
    if (LoadedValTy->isStructTy()) {
        StructType* StTy = cast<StructType>(LoadedValTy);
        SmallVector<uint32_t, 2> Idx = { 0, 0 };

        auto getCurrMemberTy = [StTy, &Idx]() {
            Type* Ty0 = StTy->getElementType(Idx[0]);
            if (StructType* stTy0 = dyn_cast<StructType>(Ty0))
                return stTy0->getElementType(Idx[1]);
            return Ty0;
        };

        auto getValueFromStruct = [&] (Type* Ty)
        {
            uint32_t TyBytes = (uint32_t)m_DL->getTypeStoreSize(Ty);
            Type* Ty0 = StTy->getElementType(Idx[0]);
            StructType* stTy0 = dyn_cast<StructType>(Ty0);
            Type* Ty1 = stTy0 ? stTy0->getElementType(Idx[1]) : nullptr;
            if (!stTy0 && (Ty0 == Ty || m_DL->getTypeStoreSize(Ty0) == TyBytes))
            {
                IGC_ASSERT(Idx[1] == 0);
                Value* V = irBuilder.CreateExtractValue(LoadedVal, Idx[0]);
                if (Ty0 != Ty) {
                    V = irBuilder.CreateBitCast(V, Ty);
                }
                (void)advanceStructIndices(Idx, StTy);
                return V;
            }
            if (stTy0 && (Ty1 == Ty || m_DL->getTypeStoreSize(Ty1) == TyBytes))
            {
                Value* V = irBuilder.CreateExtractValue(LoadedVal, Idx);
                if (Ty1 != Ty) {
                    V = irBuilder.CreateBitCast(V, Ty);
                }
                (void)advanceStructIndices(Idx, StTy);
                return V;
            }

            // Original scalar type (if the original is a vector, it's its
            // element type) could be split into smaller same-typed scalars.
            Type* eTy = Ty->getScalarType();
            uint32_t nelts = getNumElements(Ty);
            uint32_t ebytes = (uint32_t)m_DL->getTypeStoreSize(eTy);
            SmallVector<Value*, 8> vecElts;
            for (uint32_t i = 0; i < nelts; ++i) {
                int eltRemainingBytes = (int)ebytes;
                SmallVector<Value*, 8> subElts;
                do {
                    // Ty0 is type at Idx[0]
                    // stTy0 is dyn_cast<StructType>(Ty0).
                    Value* V;
                    uint32_t currBytes;
                    // type of matching struct member
                    Type* mTy;
                    if (stTy0) {
                        V = irBuilder.CreateExtractValue(LoadedVal, Idx);
                        mTy = stTy0->getElementType(Idx[1]);
                    }
                    else {
                        V = irBuilder.CreateExtractValue(LoadedVal, Idx[0]);
                        mTy = Ty0;
                    }
                    currBytes = (uint32_t)m_DL->getTypeStoreSize(mTy);
                    IGC_ASSERT_MESSAGE(currBytes <= ebytes,
                        "member should't be larger than the element size of load!");
                    eltRemainingBytes -= (int)currBytes;
                    subElts.push_back(V);
                    if (eltRemainingBytes < 0) {
                        IGC_ASSERT_UNREACHABLE();
                        break;
                    }
                    if (!advanceStructIndices(Idx, StTy)) {
                        // already last element
                        break;
                    }
                    // update Ty0/stTy0
                    Ty0 = StTy->getElementType(Idx[0]);
                    stTy0 = dyn_cast<StructType>(Ty0);
                } while (eltRemainingBytes > 0);
                IGC_ASSERT(eltRemainingBytes == 0);
                Value* V = createValueFromElements(subElts, eTy);
                vecElts.push_back(V);
            }
            Value* retVal = createValueFromElements(vecElts, Ty);
            return retVal;
        };

        // Given mTy = type of the next member in the layout struct, and Ty is
        // the type of one of all merged loads that are combined as this layout
        // struct, the algorithm gurantees:
        //   1. if mTy is a vector, Ty must be the same vector,
        //   2. if mTy is a scalar, Ty can be either a vector or scalar, and
        //      size(mTy) <= size(Ty's element type)
        for (auto& V : Vals) {
            Type* memTy = getCurrMemberTy();
            SmallVector<Value*, 8> allUses;
            if (memTy->isVectorTy()) {
                IGC_ASSERT(memTy == V->getType());
                allUses.push_back(V);
            }
            else {
                // Optimization: If V's elements are available, use them.
                getVecEltIfConstExtract(V, allUses);
            }
            for (auto& nV : allUses) {
                Type* aTy = nV->getType();
                Value* newV = getValueFromStruct(aTy);
                if (isa<UndefValue>(nV)) {
                    appendToBeDeleted(dyn_cast<Instruction>(newV));
                }
                else {
                    nV->replaceAllUsesWith(newV);
                    appendToBeDeleted(dyn_cast<Instruction>(nV));
                }
            }
        }
    } else {
        // vector type or scalar type
        uint32_t Idx = 0;
        Type* LoadedEltTy = LoadedValTy->getScalarType();
        uint32_t LoadedEltBytes = (uint32_t)m_DL->getTypeStoreSize(LoadedEltTy);

        // Return a value of type Ty at the given Idx and advance Idx.
        //   If Ty is larger than the element type of LoadedVal, it means to
        //   form a value of Ty by merging several values of LoadedVal
        //   starting at Idx, and those merged values are guaranteed to be
        //   same-typed values.
        auto collectValueFromVector = [&](Type* Ty)
        {
            uint32_t TyBytes = (uint32_t)m_DL->getTypeStoreSize(Ty);
            IGC_ASSERT(TyBytes >= LoadedEltBytes);
            int n = TyBytes / LoadedEltBytes;
            IGC_ASSERT((TyBytes % LoadedEltBytes) == 0);
            Value* retVal;
            if (n == 1) {
                retVal = irBuilder.CreateExtractElement(LoadedVal, Idx);
                if (LoadedEltTy != Ty) {
                    retVal = irBuilder.CreateBitCast(retVal, Ty);
                }
                ++Idx;
            } else {
                VectorType* vTy = VectorType::get(LoadedEltTy, n, false);
                Value* nV = UndefValue::get(vTy);
                for (int i = 0; i < n; ++i) {
                    Value* V = irBuilder.CreateExtractElement(LoadedVal, Idx);
                    nV = irBuilder.CreateInsertElement(nV, V, i);
                    ++Idx;
                }
                retVal = irBuilder.CreateBitCast(nV, Ty);
            }
            return retVal;
        };

        // Given ty = V's type, the algorithm gurantees that size of ty's
        // element is no smaller than LoadedValEBytes
        for (auto& V : Vals) {
            SmallVector<Value*, 8> allUses;
            getVecEltIfConstExtract(V, allUses);
            for (auto& nV : allUses) {
                Type* aTy = nV->getType();
                Type* eTy = aTy->getScalarType();
                uint32_t nelts = getNumElements(aTy);

                IGC_ASSERT(m_DL->getTypeStoreSize(eTy) >= LoadedEltBytes);
                SmallVector<Value*, 8> vecElts;
                for (uint32_t i = 0; i < nelts; ++i) {
                    Value* V = collectValueFromVector(eTy);
                    vecElts.push_back(V);
                }
                Value* newV = createValueFromElements(vecElts, aTy);
                if (isa<UndefValue>(nV)) {
                    appendToBeDeleted(dyn_cast<Instruction>(newV));
                }
                else {
                    nV->replaceAllUsesWith(newV);
                    appendToBeDeleted(dyn_cast<Instruction>(nV));
                }
            }
        }
    }
}

Value* LdStCombine::structToVec(IGCIRBuilder<>* irBuilder, BasicBlock* BB, Value* structVal, unsigned eltBytes, unsigned nelts) {
    uint32_t totalBytes = eltBytes * nelts;
    Type* eltTy;

    // Use special bitcast from struct to int vec to use vector emit.
    if (totalBytes < 4)
        eltTy = Type::getIntNTy(BB->getContext(), totalBytes * 8); // <{i8, i8}>, use i16, not 2xi8
    else
        eltTy = Type::getIntNTy(BB->getContext(), eltBytes * 8);

    // Use an int vector type as VTy
    Type* VTy = (nelts == 1 || totalBytes < 4) ? eltTy : VectorType::get(eltTy, nelts, false);
    Type* ITys[2] = { VTy, structVal->getType() };
    Function* IntrDcl = GenISAIntrinsic::getDeclaration(BB->getParent()->getParent(),
        GenISAIntrinsic::ID::GenISA_bitcastfromstruct, ITys);
    return irBuilder->CreateCall(IntrDcl, structVal);
}

void LdStCombine::createCombinedStores(BasicBlock* BB)
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
        AStoreInst leadStore = AStoreInst::get(Stores[0].getInst()).value();
        SmallVector<Value*, 16> storedValues;
        storedValues.push_back(leadStore.getValueOperand());
        Instruction* anchorStore = leadStore.inst();
        int n = m_instOrder[anchorStore];
        // insts are assigned order number starting from 0. Anchor store is
        // one with the largest inst order number.
        for (int i = 1, sz = (int)bundle.LoadStores.size(); i < sz; ++i)
        {
            AStoreInst SI = AStoreInst::get(Stores[i].getInst()).value();
            int SI_no = m_instOrder[SI.inst()];
            if (SI_no > n)
            {
                n = SI_no;
                anchorStore = SI.inst();
            }
            storedValues.push_back(SI.getValueOperand());
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
            else if (nelts == 2) {
                // <2xi8>,  D16U32
                eltBytes = 2;
                nelts = 1;
            }
            else {
                IGC_ASSERT(false);
            }
        }

        // Generate the coalesced value.
        Value* nV = gatherCopy(eltBytes, nelts, storedValues, anchorStore);
        Type* VTy = nV->getType();

        IGCIRBuilder<> irBuilder(anchorStore);
        if (VTy->isStructTy()) {
            nV = structToVec(&irBuilder, BB, nV, eltBytes, nelts);
            VTy = nV->getType();
        }

        Value* Addr = leadStore.getPointerOperand();
        PointerType* PTy = cast<PointerType>(Addr->getType());
        PointerType* nPTy = PointerType::get(VTy, PTy->getAddressSpace());
        Value* nAddr = irBuilder.CreateBitCast(Addr, nPTy);
        Instruction* finalStore = leadStore.CreateAlignedStore(irBuilder, nV,
            nAddr, leadStore.isVolatile());
        finalStore->setDebugLoc(anchorStore->getDebugLoc());

        // Only keep metadata from leadStore.
        // (If each store has a different metadata, should they be merged
        //  in the first place?)
        //
        //   Special case:
        //     1. set nontemporal if any merged store has it (make sense?)
        SmallVector<std::pair<unsigned, llvm::MDNode*>, 4> MDs;
        leadStore.inst()->getAllMetadata(MDs);
        for (const auto& MII : MDs) {
            finalStore->setMetadata(MII.first, MII.second);
        }

        if (finalStore->getMetadata("nontemporal") == nullptr) {
            for (int i = 1, sz = (int)bundle.LoadStores.size(); i < sz; ++i) {
                if (MDNode* N = Stores[i].getInst()->getMetadata("nontemporal")) {
                    finalStore->setMetadata("nontemporal", N);
                    break;
                }
            }
        }
    }

    // Delete stores that have been combined.
    eraseDeadInsts();

    m_hasStoreCombined = (!m_bundles.empty());

    m_bundles.clear();
}

void LdStCombine::createCombinedLoads(BasicBlock* BB)
{
    LLVM_DEBUG(dbgs() << "LdStCombine::createCombinedLoads for BB: " << BB->getName() << "\n");

    for (auto& bundle : m_bundles)
    {
        InstAndOffsetPairs& Loads = bundle.LoadStores;
        IGC_ASSERT(bundle.LoadStores.size() >= 2);
#if defined(_LDST_DEBUG)
        {
            BundleInfo* pBundle = &bundle;
            pBundle->print(dbgs(), _bundleid);
            ++_bundleid;
        }
#endif
        // The new load will be inserted at the place of the first load in the
        // program order in this bundle, called the anchor load. The lead load
        // is the load with the smallest offset in the bundle.
        ALoadInst leadLoad = ALoadInst::get(Loads[0].getInst()).value();
        SmallVector<Value*, 16> loadedValues;
        loadedValues.push_back(leadLoad.inst());

        // find anchor load.
        Instruction* anchorLoad = leadLoad.inst();
        const int leadLoadNum = m_instOrder[leadLoad.inst()];
        const int leadOffset = (int)Loads[0].getByteOffset();
        int anchorOffset = leadOffset;
        int n = leadLoadNum;
        // insts are assigned order number starting from 0. Anchor load is
        // one with the smallest inst order number.
        for (int i = 1, sz = (int)bundle.LoadStores.size(); i < sz; ++i) {
            Instruction* LI = Loads[i].getInst();
            int LI_no = m_instOrder[LI];
            if (LI_no < n)
            {
                n = LI_no;
                anchorLoad = LI;
                anchorOffset = (int)Loads[i].getByteOffset();
            }
            loadedValues.push_back(LI);
        }
        const int anchorLoadNum = n;

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

        // Create the new vector type for these combined loads.
        Type* eltTy = Type::getIntNTy(BB->getContext(), eltBytes * 8);
        Type* VTy = (nelts == 1 ? eltTy : VectorType::get(eltTy, nelts, false));

        IGCIRBuilder<> irBuilder(anchorLoad);
        Value* Addr = leadLoad.getPointerOperand();
        // If leadLoad is different from anchorLoad and leadLoad's addr is
        // an instruction after anchorLoad, need to re-generate the address
        // of LeadLoad at anchorLoad place.
        if (anchorLoad != leadLoad.inst() && isa<Instruction>(Addr)) {
            Instruction* aI = cast<Instruction>(Addr);
            auto MI = m_instOrder.find(aI);
            if (MI != m_instOrder.end() && MI->second > anchorLoadNum)
            {
                Value* anchorAddr = ALoadInst::get(anchorLoad)->getPointerOperand();
                Type* bTy = Type::getInt8Ty(leadLoad.inst()->getContext());
                Type* nTy = PointerType::get(bTy, leadLoad.getPointerAddressSpace());
                Value* nAddr = irBuilder.CreateBitCast(anchorAddr, nTy);
                Value* aIdx = irBuilder.getInt64(leadOffset - anchorOffset);
                GEPOperator* aGEP = dyn_cast<GEPOperator>(anchorAddr);
                if (aGEP && aGEP->isInBounds()) {
                    Addr = irBuilder.CreateInBoundsGEP(bTy, nAddr, aIdx, "anchorLoad");
                }
                else {
                    Addr = irBuilder.CreateGEP(bTy, nAddr, aIdx, "anchorLoad");
                }
            };
        }
        PointerType* PTy = cast<PointerType>(Addr->getType());
        PointerType* nPTy = PointerType::get(VTy, PTy->getAddressSpace());
        Value* nAddr = irBuilder.CreateBitCast(Addr, nPTy);

        // Merge "merge values" of each predicated load in loadedValues to use in a new load.
        SmallVector<Value*, 16> mergeValues;
        for (auto load : loadedValues)
        {
            PredicatedLoadIntrinsic *PLI = ALoadInst::get(load)->getPredicatedLoadIntrinsic();
            if (!PLI)
                break; // not a predicated load, no merge values
            mergeValues.push_back(PLI->getMergeValue());
        }

        Value* mergeVal = mergeValues.empty() ? nullptr : gatherCopy(eltBytes, nelts, mergeValues, anchorLoad);
        if (mergeVal && mergeVal->getType()->isStructTy())
            mergeVal = structToVec(&irBuilder, BB, mergeVal, eltBytes, nelts);

        Instruction *finalLoad = leadLoad.CreateAlignedLoad(irBuilder, VTy, nAddr, mergeVal,
            leadLoad.isVolatile());
        finalLoad->setDebugLoc(anchorLoad->getDebugLoc());

        // Split loaded value and replace original loads with them.
        scatterCopy(loadedValues, eltBytes, nelts, finalLoad, anchorLoad);

        // Keep metadata
        auto STII = std::find_if_not(
            bundle.LoadStores.begin(), bundle.LoadStores.end(),
            [](LdStInfo& LSI) {
                auto md = LSI.getInst()->getMetadata(LLVMContext::MD_invariant_load);
                return md != nullptr;
            });
        if (STII == bundle.LoadStores.end()) {
            MDNode* md = anchorLoad->getMetadata(LLVMContext::MD_invariant_load);
            IGC_ASSERT(md != nullptr);
            finalLoad->setMetadata(LLVMContext::MD_invariant_load, md);
        }
        MDNode* nonTempMD = nullptr;
        std::for_each(bundle.LoadStores.begin(), bundle.LoadStores.end(),
            [&nonTempMD](LdStInfo& LSI) {
                if (auto md = LSI.getInst()->getMetadata("nontemporal"))
                    nonTempMD = MDNode::concatenate(md, nonTempMD);
            });

        if (nonTempMD) {
            finalLoad->setMetadata("nontemporal", nonTempMD);
        }
    }

    // Delete stores that have been combined.
    eraseDeadInsts();

    m_hasLoadCombined = (!m_bundles.empty());

    m_bundles.clear();
}

void LdStCombine::eraseDeadInsts()
{
    RecursivelyDeleteDeadInstructions(m_toBeDeleted);
    m_toBeDeleted.clear();
}

void BundleInfo::print(raw_ostream& O, int BundleID) const
{
    O << "\nBundle Info " << BundleID << "\n"
      << "  Element bytes = " << bundle_eltBytes << "    "
      << "num of elements = " << bundle_numElts << "    "
      << "useD64 = " << (useD64 ? "true" : "false") << "\n\n";

    for (const auto& II : LoadStores) {
        const LdStInfo& LSI = II;
        O << "  (" << format_decimal(LSI.getByteOffset(), 3) << ")   ";
        O << *LSI.getInst() << "\n";
    }
    O << "\n";
}

void BundleInfo::dump() const
{
    print(dbgs());
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

bool isLayoutStructTypeSOA(const StructType* StTy)
{
    return isLayoutStructType(StTy) && !isLayoutStructTypeAOS(StTy);
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
                // C_i is scalar of int, fp or null pointer
                IGC_ASSERT(isa<ConstantInt>(C_i) || isa<ConstantFP>(C_i) ||
                    isa<ConstantPointerNull>(C_i));
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
    if (isa<UndefValue>(C) || isa<ConstantPointerNull>(C)) {
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

static void searchForDefinedMembers(const ConstantAggregate* S, const std::vector<unsigned>& currentIndices, SmallVectorImpl<std::vector<unsigned>>& fieldsTBC)
{
    for (unsigned i = 0; i < S->getNumOperands(); i++)
    {
        auto indices = currentIndices;
        indices.push_back(i);
        auto* E = S->getAggregateElement(i);
        if (isa<UndefValue>(E))
            continue;

        if (auto* SE = dyn_cast<ConstantAggregate>(E))
        {
            searchForDefinedMembers(SE, indices, fieldsTBC);
        }
        else
        {
            fieldsTBC.push_back(indices);
        }

    }
}

void getAllDefinedMembers (const Value* IVI,
    SmallVectorImpl<std::vector<unsigned>>& fieldsTBC)
{
    IGC_ASSERT(IVI != nullptr);
    const Value* V = IVI;
    while (isa<InsertValueInst>(V))
    {
        const InsertValueInst* I = cast<const InsertValueInst>(V);
        fieldsTBC.push_back(I->getIndices().vec());
        V = I->getOperand(0);
    }

    // at the end we may have a constant struct like this:
    // % 28 = insertvalue % __StructSOALayout_ < { i32 194816, i32 undef, i32 undef, <1 x float> undef } > , i32 % 17, 1
    // we should traverse it and find the indices pointing to the constant values
    if (auto* S = dyn_cast<ConstantAggregate>(V))
    {
        std::vector<unsigned> indices = {};
        searchForDefinedMembers(S, indices, fieldsTBC);
    }

    // reverse the vector to get the ascending order of indices
    std::reverse(fieldsTBC.begin(), fieldsTBC.end());
}
}
