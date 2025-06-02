/*========================== begin_copyright_notice ============================

Copyright (C) 2017 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvmWrapper/Support/Alignment.h>
#include <optional>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include "common/debug/Debug.hpp"

#include <utility>    // std::pair, std::make_pair
#include <sstream>    // std::string, std::stringstream
#include <fstream>    // std::ofstream

using namespace llvm;
using namespace IGC;
using IGCLLVM::FixedVectorType;

//
// Description of VectorPreProcess Pass
//   The purpose is both to legalize vector types and to reduce register
//   presure. Once this pass is done, there is no 3-element vector whose
//   element size < 4 bytes, that is, no <3 x i8>, no <3 x i16>. (But
//   we will have <3xi32> and <3xi64>.)
//
// 1. Split a vector load/stores with a large vector into ones with
//    smaller vectors or scalars; and make sure that the sub-vectors
//    are either multiple of DW, vector3, or their size is less than
//    4 bytes (see details in code).  Vector3 will be specially
//    handled later.
//    For example,
//        <16xi64> ---> four <4xi64>
//        <15xi32> ---> <8xi32>, <7xi32>
//        <13xi32> ---> <8xi32>, <5xi32>
//        <31xi16> ---> <16xi16>, <12xi16>, <3xi16>
//        <19xi16> ---> <16xi16>, <3xi16>
//        <39xi8>  ---> <32xi8>, <4xi8>, <3xi8>
//    Note that splitting keeps the vector element's type without
//    changing it.
//
//    Note that as 6/2020,
//      if size of vector element >= DW, the number of elements of the new vector
//      should be power of 2 except for vector3.  Thus, we should not see 5xi32,
//      7xi32, etc.  This makes code emit easier.
//
// 2. Special processing of 3-element vectors
//    If (vector element's size < 4 bytes)
//    {
//       3-element vector load  --> 2-element vector load + scalar load
//       3-element vector store --> 2-element vector store + scalar store.
//    }
//    Note that 3-element load could be optimized to 4-element load (check
//    details in the code).
//
//    for example,
//      (1) %1 = load <3 x i8> *p
//          converted into
//             %pv = bitcast p to <2 x i8>*  // %pv is type <2 x i8>*
//             %ps = (i8*)p + 2;             // %ps is type i8*
//             %2 = load <2 x i8> *pv
//             %3 = load i8 *ps
//          original vector %1 == (%2, %3)
//
//      (2) store <3 x i16> %1, <3 x i16> *p
//          converted into
//             %pv = bitcast p to <2 x i16>*  // %pv is type <2 x i16>*
//             %ps = (i16*)p + 2;             // %ps is type i16*
//             %new_v = (%1.x, %1.y)
//             store <2 x i16> %new_v, <2 x i16> *pv
//             store i16 %1.z i8 *ps
//
namespace
{
    // AbstractLoadInst and AbstractStoreInst abstract away the differences
    // between ldraw, Load and PredicatedLoad and between storeraw, Store and PredicatedStore.
    // Note on usage: The Value* passed as the ptr paramter to the Create method
    // should be either the result of the getPointerOperand() method or the
    // CreateConstScalarGEP() method. Do not attempt to do arithmetic
    // (or pointer arithmetic) on these values.
    class AbstractLoadInst
    {
        Instruction* const m_inst;
        const DataLayout& DL;
        AbstractLoadInst(LoadInst* LI, const DataLayout& DL) :
            m_inst(LI), DL(DL) {}
        AbstractLoadInst(LdRawIntrinsic* LdRI, const DataLayout& DL) :
            m_inst(LdRI), DL(DL) {}
        AbstractLoadInst(PredicatedLoadIntrinsic* PLI, const DataLayout& DL) :
            m_inst(PLI), DL(DL) {}

        LoadInst* getLoad() const
        {
            return cast<LoadInst>(m_inst);
        }
        LdRawIntrinsic* getLdRaw() const
        {
            return cast<LdRawIntrinsic>(m_inst);
        }
        PredicatedLoadIntrinsic* getPredicatedLoad() const
        {
            return cast<PredicatedLoadIntrinsic>(m_inst);
        }
    public:
        Instruction* getInst() const
        {
            return m_inst;
        }
        alignment_t getAlignment() const
        {
            if (isa<LoadInst>(m_inst))
                return IGCLLVM::getAlignmentValue(getLoad());
            if (isa<LdRawIntrinsic>(m_inst))
                return getLdRaw()->getAlignment();
            return getPredicatedLoad()->getAlignment();
        }
        void setAlignment(alignment_t alignment)
        {
            if (isa<LoadInst>(m_inst))
            {
                getLoad()->setAlignment(IGCLLVM::getCorrectAlign(alignment));
            }
            else if (isa<LdRawIntrinsic>(m_inst))
            {
                getLdRaw()->setAlignment(alignment);
            }
            else
            {
                getPredicatedLoad()->setAlignment(alignment);
            }
        }
        Value* getPointerOperand() const
        {
            if (isa<LoadInst>(m_inst))
                return getLoad()->getPointerOperand();
            if (isa<LdRawIntrinsic>(m_inst))
                return getLdRaw()->getResourceValue();
            return getPredicatedLoad()->getPointerOperand();
        }
        bool getIsVolatile() const
        {
            if (isa<LoadInst>(m_inst))
                return getLoad()->isVolatile();
            if (isa<LdRawIntrinsic>(m_inst))
                return getLdRaw()->isVolatile();
            return getPredicatedLoad()->isVolatile();
        }
        unsigned getPointerAddressSpace() const
        {
            return getPointerOperand()->getType()->getPointerAddressSpace();
        }
        Value *getMergeValue() const
        {
            if(isa<PredicatedLoadIntrinsic>(m_inst))
                return getPredicatedLoad()->getMergeValue();
            return nullptr;
        }
        Instruction* Create(Type* returnType, Value* mergeValue = nullptr)
        {
            return Create(returnType, getPointerOperand(), getAlignment(), getIsVolatile(), mergeValue);
        }
        Instruction* Create(Type* returnType, Value* ptr, alignment_t alignment, bool isVolatile, Value* mergeValue = nullptr)
        {
            IGCLLVM::IRBuilder<> builder(m_inst);
            if (isa<LoadInst>(m_inst))
            {
                Type* newPtrType = PointerType::get(returnType, ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, newPtrType);
                LoadInst* newLI = builder.CreateAlignedLoad(returnType, ptr, IGCLLVM::getAlign(alignment), isVolatile);
                if (MDNode* lscMetadata = m_inst->getMetadata("lsc.cache.ctrl"))
                {
                    newLI->setMetadata("lsc.cache.ctrl", lscMetadata);
                }
                return newLI;
            }
            if (isa<LdRawIntrinsic>(m_inst))
            {
                LdRawIntrinsic* ldraw = getLdRaw();
                bool hasComputedOffset = ptr != ldraw->getResourceValue();
                Value* offsetVal = hasComputedOffset ? ptr : ldraw->getOffsetValue();
                ptr = ldraw->getResourceValue();
                Type* types[2] = { returnType , ptr->getType() };
                Value* args[4] = { ptr, offsetVal, builder.getInt32((uint32_t)alignment), builder.getInt1(isVolatile) };
                Function* newLdRawFunction =
                    GenISAIntrinsic::getDeclaration(ldraw->getModule(), ldraw->getIntrinsicID(), types);
                return builder.CreateCall(newLdRawFunction, args);
            }

            IGC_ASSERT(isa<PredicatedLoadIntrinsic>(m_inst));
            IGC_ASSERT(mergeValue);
            IGC_ASSERT(mergeValue->getType() == returnType);

            PredicatedLoadIntrinsic* PLI = getPredicatedLoad();
            Type* newPtrType = PointerType::get(returnType, ptr->getType()->getPointerAddressSpace());
            ptr = builder.CreateBitCast(ptr, newPtrType);
            Type* types[3] = { returnType, ptr->getType(), returnType };
            Function *predLoadFunc = GenISAIntrinsic::getDeclaration(m_inst->getModule(), PLI->getIntrinsicID(), types);
            Value *args[4] = { ptr, builder.getInt64((uint64_t)alignment), PLI->getPredicate(), mergeValue };
            Instruction *PredLoad = builder.CreateCall(predLoadFunc, args);
            if (MDNode* lscMetadata = m_inst->getMetadata("lsc.cache.ctrl"))
                PredLoad->setMetadata("lsc.cache.ctrl", lscMetadata);
            return PredLoad;
        }
        // Emulates a GEP on a pointer of the scalar type of returnType.
        Value* CreateConstScalarGEP(Type* returnType, Value* ptr, uint32_t offset)
        {
            IGCLLVM::IRBuilder<> builder(m_inst);
            if (isa<LoadInst>(m_inst) || isa<PredicatedLoadIntrinsic>(m_inst))
            {
                Type* ePtrType = PointerType::get(returnType->getScalarType(), ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, ePtrType);
                return builder.CreateConstGEP1_32(returnType->getScalarType(), ptr, offset);
            }
            else
            {
                uint32_t sizeInBytes =
                    int_cast<uint32_t>(DL.getTypeSizeInBits(returnType->getScalarType()) / 8);
                Value* offsetInBytes = builder.getInt32(offset * sizeInBytes);
                return builder.CreateAdd(offsetInBytes, getLdRaw()->getOffsetValue());
            }
        }
        static std::optional<AbstractLoadInst> get(llvm::Value* value, const DataLayout &DL)
        {
            if (LoadInst * LI = dyn_cast<LoadInst>(value))
            {
                return AbstractLoadInst{ LI, DL };
            }
            else if (LdRawIntrinsic * LdRI = dyn_cast<LdRawIntrinsic>(value))
            {
                return AbstractLoadInst{ LdRI, DL };
            }
            else if (PredicatedLoadIntrinsic *PLI = dyn_cast<PredicatedLoadIntrinsic>(value))
            {
                return AbstractLoadInst{ PLI, DL };
            }
            else
            {
                return std::nullopt;
            }
        }
    };
    static bool isAbstractLoadInst(llvm::Value* value)
    {
        return isa<LoadInst>(value) || isa<LdRawIntrinsic>(value) || isa<PredicatedLoadIntrinsic>(value);
    }

    class AbstractStoreInst
    {
        Instruction* const m_inst;
        const DataLayout& DL;
        AbstractStoreInst(StoreInst* SI, const DataLayout& DL) :
            m_inst(SI), DL(DL) {}
        AbstractStoreInst(StoreRawIntrinsic* SRI, const DataLayout& DL) :
            m_inst(SRI), DL(DL) {}
        AbstractStoreInst(PredicatedStoreIntrinsic* PSI, const DataLayout& DL) :
            m_inst(PSI), DL(DL) {}

        StoreInst* getStore() const
        {
            return cast<StoreInst>(m_inst);
        }
        StoreRawIntrinsic* getStoreRaw() const
        {
            return cast<StoreRawIntrinsic>(m_inst);
        }
        PredicatedStoreIntrinsic* getPredicatedStore() const
        {
            return cast<PredicatedStoreIntrinsic>(m_inst);
        }
    public:
        Instruction* getInst() const
        {
            return m_inst;
        }
        alignment_t getAlignment() const
        {
            if(isa<StoreInst>(m_inst))
                return IGCLLVM::getAlignmentValue(getStore());
            if (isa<StoreRawIntrinsic>(m_inst))
                return getStoreRaw()->getAlignment();
            return getPredicatedStore()->getAlignment();
        }
        void setAlignment(alignment_t alignment)
        {
            if (isa<StoreInst>(m_inst))
            {
                getStore()->setAlignment(IGCLLVM::getCorrectAlign(alignment));
            }
            else if (isa<PredicatedStoreIntrinsic>(m_inst))
            {
                getPredicatedStore()->setAlignment(alignment);
            }
        }
        Value* getValueOperand() const
        {
            if (isa<StoreInst>(m_inst))
                return getStore()->getValueOperand();
            if (isa<StoreRawIntrinsic>(m_inst))
                return getStoreRaw()->getArgOperand(2);
            return getPredicatedStore()->getValueOperand();
        }
        Value* getPointerOperand() const
        {
            if(isa<StoreInst>(m_inst))
                return getStore()->getPointerOperand();
            if (isa<StoreRawIntrinsic>(m_inst))
                return getStoreRaw()->getArgOperand(0);
            return getPredicatedStore()->getPointerOperand();
        }
        bool getIsVolatile() const
        {
            if(isa<StoreInst>(m_inst))
                return getStore()->isVolatile();
            if (isa<PredicatedLoadIntrinsic>(m_inst))
                return getPredicatedStore()->isVolatile();
            return false;
        }
        unsigned getPointerAddressSpace() const
        {
            return getPointerOperand()->getType()->getPointerAddressSpace();
        }
        Instruction* Create(Value* storedValue, Value* ptr, alignment_t alignment, bool isVolatile)
        {
            IRBuilder<> builder(m_inst);
            Type* newType = storedValue->getType();
            if (isa<StoreInst>(m_inst))
            {
                Type* newPtrType = PointerType::get(newType, ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, newPtrType);
                return alignment ?
                    builder.CreateAlignedStore(storedValue, ptr, IGCLLVM::getAlign(alignment), isVolatile) :
                    builder.CreateStore(storedValue, ptr, isVolatile);
            }
            if (isa<StoreRawIntrinsic>(m_inst))
            {
                bool hasComputedOffset = ptr != getPointerOperand();
                Value* offset = hasComputedOffset ? ptr : getStoreRaw()->getArgOperand(1);
                ptr = getPointerOperand();
                Type* types[2] = { ptr->getType(), newType };
                Value* args[5] = { ptr, offset, storedValue, builder.getInt32((uint32_t)alignment), builder.getInt1(isVolatile) };
                Function* newStoreRawFunction =
                    GenISAIntrinsic::getDeclaration(getStoreRaw()->getModule(), getStoreRaw()->getIntrinsicID(), types);
                return builder.CreateCall(newStoreRawFunction, args);
            }

            Type *newPtrType = PointerType::get(newType, ptr->getType()->getPointerAddressSpace());
            ptr = builder.CreateBitCast(ptr, newPtrType);
            Type *types[2] = {ptr->getType(), newType};
            Function *predStoreFunc = GenISAIntrinsic::getDeclaration(getPredicatedStore()->getModule(), getPredicatedStore()->getIntrinsicID(), types);
            Value *args[4] = {ptr, storedValue, builder.getInt64((uint64_t)alignment), getPredicatedStore()->getPredicate()};
            return builder.CreateCall(predStoreFunc, args);
        }
        Instruction* Create(Value* storedValue)
        {
            return Create(storedValue, getPointerOperand(), getAlignment(), getIsVolatile());
        }
        // Emulates a GEP on a pointer of the scalar type of storedType.
        Value* CreateConstScalarGEP(Type* storedType, Value* ptr, uint32_t offset)
        {
            IGCLLVM::IRBuilder<> builder(m_inst);
            if (isa<StoreInst>(m_inst) || isa<PredicatedStoreIntrinsic>(m_inst))
            {
                Type* ePtrType = PointerType::get(storedType->getScalarType(), ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, ePtrType);
                return builder.CreateConstGEP1_32(storedType->getScalarType(), ptr, offset);
            }
            else
            {
                uint32_t sizeInBytes =
                    int_cast<uint32_t>(DL.getTypeSizeInBits(storedType->getScalarType()) / 8);
                Value* offsetInBytes = builder.getInt32(offset * sizeInBytes);
                return builder.CreateAdd(offsetInBytes, getStoreRaw()->getArgOperand(1));
            }
        }
        static std::optional<AbstractStoreInst> get(llvm::Value* value, const DataLayout &DL)
        {
            if (StoreInst * SI = dyn_cast<StoreInst>(value))
            {
                return AbstractStoreInst{ SI, DL };
            }
            if (StoreRawIntrinsic* SRI = dyn_cast<StoreRawIntrinsic>(value))
            {
                return AbstractStoreInst{ SRI, DL };
            }
            if (PredicatedStoreIntrinsic* PSI = dyn_cast<PredicatedStoreIntrinsic>(value))
            {
                return AbstractStoreInst{ PSI, DL };
            }
            return std::nullopt;
        }
    };

    static bool isAbstractStoreInst(llvm::Value* value)
    {
        GenIntrinsicInst* II = dyn_cast<GenIntrinsicInst>(value);
        return isa<StoreInst>(value) ||
               (II && (II->getIntrinsicID() == GenISAIntrinsic::GenISA_storeraw_indexed ||
                       II->getIntrinsicID() == GenISAIntrinsic::GenISA_storerawvector_indexed ||
                       II->getIntrinsicID() == GenISAIntrinsic::GenISA_PredicatedStore));
    }

    class VectorPreProcess : public FunctionPass
    {
    public:
        typedef SmallVector<Instruction*, 32> InstWorkVector;
        typedef SmallVector<Value*, 16> ValVector;

        // vector value -> (split size in bytes -> vector's component values)
        typedef DenseMap<Value*, DenseMap<uint32_t, ValVector>> V2SMap;

        enum class VPConst {
            // If a vector's size is bigger than SPLIT_SIZE, split it into multiple
            // of SPLIT_SIZE (plus smaller sub-vectors or scalar if any).
            // With SPLIT_SIZE=32, we have the max vectors as below after this pass:
            //     <32 x i8>, 16xi16, 8xi32, or 4xi64!
            SPLIT_SIZE = 32,              // default, 32 bytes
            LSC_D64_UNIFORM_SPLIT_SIZE = 512, // LSC transpose 64 x D64
            LSC_D32_UNIFORM_SPLIT_SIZE = 256, // LSC transpose 64 x D32
            RAW_SPLIT_SIZE = 16
        };

        static char ID; // Pass identification, replacement for typeid
        VectorPreProcess()
            : FunctionPass(ID)
            , m_DL(nullptr)
            , m_C(nullptr)
            , m_WorkList()
            , m_Temps()
            , m_CGCtx(nullptr)
        {
            initializeVectorPreProcessPass(*PassRegistry::getPassRegistry());
        }

        StringRef getPassName() const override { return "VectorPreProcess"; }
        bool runOnFunction(Function& F) override;
        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<PostDominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

    private:

        void getOrGenScalarValues(
            Function& F, Value* VecVal, ValVector& scalars, Instruction*& availBeforeInst);
        void replaceAllVectorUsesWithScalars(Instruction* VI,
            ValVector& SVals);

        // Return true if V is created by InsertElementInst with const index.
        bool isValueCreatedOnlyByIEI(Value* V, InsertElementInst** IEInsts);
        // Return true if V is only used by ExtractElement with const index.
        bool isValueUsedOnlyByEEI(Value* V, ExtractElementInst** EEInsts);

        // Split load/store that cannot be re-layout or is too big.
        uint32_t getSplitByteSize(Instruction* I, WIAnalysisRunner& WI) const;
        bool splitLoadStore(Instruction* Inst, V2SMap& vecToSubVec, WIAnalysisRunner& WI);
        bool splitLoad(AbstractLoadInst& LI, V2SMap& vecToSubVec, WIAnalysisRunner& WI);
        bool splitStore(AbstractStoreInst& SI, V2SMap& vecToSubVec, WIAnalysisRunner& WI);
        bool splitVector3LoadStore(Instruction* Inst);
        // Simplify load/store instructions if possible. Return itself if no
        // simplification is performed.
        Instruction* simplifyLoadStore(Instruction* LI);
        void createSplitVectorTypes(
            Type* ETy,
            uint32_t NElts,
            uint32_t SplitSize,
            SmallVector<std::pair<Type*, uint32_t>, 8>& SplitInfo);
        // If predicated loads are split, we also need to split merge values
        void createSplitMergeValues(
            Instruction* Inst,
            Value* OrigMergeVal,
            const SmallVector<std::pair<Type*, uint32_t>, 8>& SplitInfo,
            ValVector& NewMergeVals) const;
        bool processScalarLoadStore(Function& F);

    private:
        const DataLayout* m_DL;
        LLVMContext* m_C;
        InstWorkVector m_WorkList;
        ValVector m_Temps;
        InstWorkVector m_Vector3List; // used for keep all 3-element vectors.
        IGC::CodeGenContext* m_CGCtx;
    };
}

// Register pass to igc-opt
#define PASS_FLAG "igc-vectorpreprocess"
#define PASS_DESCRIPTION "Split loads/stores of big (or 3-element) vectors into smaller ones."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(VectorPreProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(VectorPreProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char VectorPreProcess::ID = 0;

FunctionPass* IGC::createVectorPreProcessPass()
{
    return new VectorPreProcess();
}

bool VectorPreProcess::isValueCreatedOnlyByIEI(Value* V, InsertElementInst** IEInsts)
{
    Value* ChainVal = V;
    while (!isa<UndefValue>(ChainVal))
    {
        InsertElementInst* IEI = dyn_cast<InsertElementInst>(ChainVal);
        if (!IEI || !isa<ConstantInt>(IEI->getOperand(2)))
        {
            return false;
        }
        ConstantInt* CInt = cast<ConstantInt>(IEI->getOperand(2));
        uint32_t idx = (uint32_t)CInt->getZExtValue();

        // Make sure the last IEI will be recorded if an element is
        // inserted multiple times.
        if (IEInsts[idx] == nullptr)
        {
            IEInsts[idx] = IEI;
        }

        ChainVal = IEI->getOperand(0);
    }
    return true;
}

bool VectorPreProcess::isValueUsedOnlyByEEI(Value* V, ExtractElementInst** EEInsts)
{
    for (Value::user_iterator UI = V->user_begin(), UE = V->user_end();
        UI != UE; ++UI)
    {
        ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*UI);
        if (!EEI ||
            (EEI->getOperand(0) != V) ||
            !isa<ConstantInt>(EEI->getOperand(1)))
        {
            return false;
        }
        ConstantInt* CInt = cast<ConstantInt>(EEI->getOperand(1));
        uint32_t idx = (uint32_t)CInt->getZExtValue();

        // Quit if there are multiple extract from the same index.
        if (EEInsts[idx] != nullptr)
        {
            return false;
        }
        EEInsts[idx] = EEI;
    }
    return true;
}

// SVals[0:NumElements] has all scalar elements of vector VI. This function
// tries to replace all uses of VI with SVals[...] if possible, If not
// possible, re-generate the vector from SVals at the BB of VI.
//
// This function also erase VI.
void VectorPreProcess::replaceAllVectorUsesWithScalars(Instruction* VI, ValVector& SVals)
{
    SmallVector<Instruction*, 8> ToBeDeleted;
    bool genVec = false;
    for (Value::user_iterator UI = VI->user_begin(), UE = VI->user_end(); UI != UE; ++UI)
    {
        ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*UI);
        if (!EEI)
        {
            genVec = true;
            continue;
        }
        ConstantInt* CI = dyn_cast<ConstantInt>(EEI->getOperand(1));
        if (!CI)
        {
            genVec = true;
            continue;
        }
        uint32_t ix = (uint32_t)CI->getZExtValue();
        EEI->replaceAllUsesWith(SVals[ix]);
        ToBeDeleted.push_back(EEI);
    }
    if (genVec)
    {
        Instruction* I;
        if (!isa<PHINode>(VI))
        {
            I = VI;
        }
        else
        {
            I = VI->getParent()->getFirstNonPHI();
        }
        IRBuilder<> Builder(I);
        IGCLLVM::FixedVectorType* VTy = cast<IGCLLVM::FixedVectorType>(VI->getType());
        Value* newVec = UndefValue::get(VTy);
        for (uint32_t i = 0, e = int_cast<uint32_t>(VTy->getNumElements()); i < e; ++i)
        {
            newVec = Builder.CreateInsertElement(
                newVec,
                SVals[i],
                Builder.getInt32(i),
                "scalarize");
        }
        // Replace old instruction with new one
        VI->replaceAllUsesWith(newVec);
    }
    for (uint32_t i = 0; i < ToBeDeleted.size(); ++i)
    {
        ToBeDeleted[i]->eraseFromParent();
    }
}

void VectorPreProcess::createSplitVectorTypes(
    Type* ETy,
    uint32_t NElts,
    uint32_t SplitSize,
    SmallVector<std::pair<Type*, uint32_t>, 8>& SplitInfo)
{
    uint32_t ebytes = int_cast<uint32_t>(m_DL->getTypeSizeInBits(ETy) / 8);

    // todo: generalize splitting for cases whose element size is bigger than splitsize!
    if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
    {
        if (ebytes > SplitSize)
        {
            IGC_ASSERT(SplitSize);
            uint32_t M = NElts * ebytes / SplitSize;
            Type* Ty = IntegerType::get(ETy->getContext(), SplitSize * 8);
            SplitInfo.push_back(std::make_pair(Ty, M));
            return;
        }
    }

    // Both SplitSize and ebytes shall be a power of 2
    IGC_ASSERT(ebytes);
    IGC_ASSERT_MESSAGE((SplitSize % ebytes) == 0, "Internal Error: Wrong split size!");

    uint32_t E = SplitSize / ebytes; // split size in elements
    uint32_t N = NElts;  // the number of elements to be split

    IGC_ASSERT(E);

    // 1. Make sure splitting it by SplitSize as required
    uint32_t M = N / E;  // the number of subvectors for split size E
    if (M > 0)
    {
        Type* Ty = (E == 1) ? ETy : FixedVectorType::get(ETy, E);
        SplitInfo.push_back(std::make_pair(Ty, M));
    }
    N = N % E;
    E = E / 2;  // next split size

    // 2. The remaining elts are splitted if not power of 2 until N <= 4.
    while (N > 4)
    {
        IGC_ASSERT(E);

        M = N / E;  // the number of subvectors for split size E
        if (M > 0)
        {
            SplitInfo.push_back(std::make_pair(FixedVectorType::get(ETy, E), M));
        }
        // The remaining elts are to be split for next iteration.
        N = N % E;
        E = E / 2;  // next split size
    }

    // 3. A vector of 1|2|3|4 elements. No further splitting!
    if (N > 0)
    {
        Type* Ty = (N == 1) ? ETy : FixedVectorType::get(ETy, N);
        SplitInfo.push_back(std::make_pair(Ty, 1));
    }
}

void VectorPreProcess::createSplitMergeValues(Instruction* Inst,
                                              Value* OrigMergeVal,
                                              const SmallVector<std::pair<Type*, uint32_t>, 8>& SplitInfo,
                                              ValVector& NewMergeVals) const
{
    // if OrigMergeVal is a zeroinitializer, undef, or poison value, we just need to fill
    // NewMergeVals with the same based on SplitInfo and return.
    if (isa<ConstantAggregateZero>(OrigMergeVal) ||
        isa<UndefValue>(OrigMergeVal) ||
        isa<PoisonValue>(OrigMergeVal))
    {
        for (auto &SI : SplitInfo)
        {
            Type *Ty = SI.first;
            IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
            uint32_t N = SI.second;
            for (uint32_t i = 0; i < N; ++i)
            {
                Value *NewMergeVal = nullptr;
                if (isa<ConstantAggregateZero>(OrigMergeVal)) {
                    if (VTy)
                        NewMergeVal = ConstantAggregateZero::get(VTy);
                    else
                        NewMergeVal = Constant::getNullValue(Ty);
                } else if (isa<PoisonValue>(OrigMergeVal)) {
                    NewMergeVal = PoisonValue::get(SI.first);
                } else {
                    NewMergeVal = UndefValue::get(SI.first);
                }
                NewMergeVals.push_back(NewMergeVal);
            }
        }

        return;
    }

    IRBuilder<> Builder(Inst);

    // Case when we split vector merge value into subvectors. Element type is the same.
    // Just one big vector is being split into subvectors.
    if (IGCLLVM::FixedVectorType *OrigVTy = dyn_cast<IGCLLVM::FixedVectorType>(OrigMergeVal->getType()))
    {
        unsigned OrigVTyNEl = OrigVTy->getNumElements();
        uint32_t idx = 0; // index counting elements of the the original vector merge value

        // Split the merge value into subvectors
        for (auto &SI : SplitInfo)
        {
            Type *Ty = SI.first;
            IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
            uint32_t N = SI.second;
            for (uint32_t i = 0; i < N; ++i)
            {
                Value *NewMergeVal = UndefValue::get(Ty);
                if (VTy)
                {
                    for (uint32_t j = 0, e = int_cast<uint32_t>(VTy->getNumElements()); j < e; ++j)
                    {
                        Value *Elt = (idx < OrigVTyNEl) ? Builder.CreateExtractElement(OrigMergeVal, Builder.getInt32(idx++)) :
                                                          Constant::getNullValue(VTy->getElementType());
                        NewMergeVal = Builder.CreateInsertElement(NewMergeVal, Elt, Builder.getInt32(j));
                    }
                }
                else
                {
                    NewMergeVal = Builder.CreateExtractElement(OrigMergeVal, Builder.getInt32(idx++));
                }
                NewMergeVals.push_back(NewMergeVal);
            }
        }

        return;
    }

    // Case when we change scalar value into vector with smaller element type.
    IGC_ASSERT_MESSAGE(SplitInfo.size() == 1, "Unexpected split info!");
    IGC_ASSERT_MESSAGE(SplitInfo[0].second == 1, "Unexpected split info!");
    Value *NewMergeVal = Builder.CreateBitCast(OrigMergeVal, SplitInfo[0].first);
    NewMergeVals.push_back(NewMergeVal);
}

uint32_t VectorPreProcess::getSplitByteSize(Instruction* I, WIAnalysisRunner& WI) const
{
    uint32_t bytes = 0;
    std::optional<AbstractLoadInst> ALI = AbstractLoadInst::get(I, *m_DL);
    std::optional<AbstractStoreInst> ASI = AbstractStoreInst::get(I, *m_DL);

    if (isa<LoadInst>(I) || isa<PredicatedLoadIntrinsic>(I))
    {
        IGC_ASSERT(ALI.has_value());
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
        if (WI.isUniform(ALI->getPointerOperand()) &&
            (m_CGCtx->platform.LSCEnabled() || IGC_GET_FLAG_VALUE(UniformMemOpt4OW)))
        {
            if (ALI->getAlignment() >= 8)
                bytes = (uint32_t)VPConst::LSC_D64_UNIFORM_SPLIT_SIZE;
            else if (ALI->getAlignment() >= 4)
                bytes = (uint32_t)VPConst::LSC_D32_UNIFORM_SPLIT_SIZE;
        }
    }
    else if (isa<StoreInst>(I) || isa<PredicatedStoreIntrinsic>(I))
    {
        IGC_ASSERT(ASI.has_value());
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
        Value* Addr = ASI->getPointerOperand();
        Value* Data = ASI->getValueOperand();
        if (m_CGCtx->platform.LSCEnabled() && WI.isUniform(Addr) && WI.isUniform(Data))
        {
            if (ASI->getAlignment() >= 8)
                bytes = (uint32_t)VPConst::LSC_D64_UNIFORM_SPLIT_SIZE;
            else if (ASI->getAlignment() >= 4)
                bytes = (uint32_t)VPConst::LSC_D32_UNIFORM_SPLIT_SIZE;
        }
    }
    else if (isa<LdRawIntrinsic>(I) || isa<StoreRawIntrinsic>(I))
    {
        uint32_t alignment = isa<LdRawIntrinsic>(I) ?
            cast<LdRawIntrinsic>(I)->getAlignment() : cast<StoreRawIntrinsic>(I)->getAlignment();
        Value* bufferAddr = isa<LdRawIntrinsic>(I) ?
            cast<LdRawIntrinsic>(I)->getResourceValue() : cast<StoreRawIntrinsic>(I)->getResourceValue();
        Value* offset = isa<LdRawIntrinsic>(I) ?
            cast<LdRawIntrinsic>(I)->getOffsetValue() : cast<StoreRawIntrinsic>(I)->getOffsetValue();
        Value* data = isa<LdRawIntrinsic>(I) ?
            nullptr : cast<StoreRawIntrinsic>(I)->getStoreValue();
        bytes = (uint32_t)VPConst::RAW_SPLIT_SIZE;
        if (EmitPass::shouldGenerateLSCQuery(*m_CGCtx, I) == Tristate::True)
        {
            if (WI.isUniform(bufferAddr) && WI.isUniform(offset) &&
                (data == nullptr || WI.isUniform(data)))
            {
                if (alignment >= 8)
                {
                    bytes = (uint32_t)VPConst::LSC_D64_UNIFORM_SPLIT_SIZE;
                }
                else if (alignment >= 4)
                {
                    bytes = (uint32_t)VPConst::LSC_D32_UNIFORM_SPLIT_SIZE;
                }
            }
            else
            {
                bytes = (uint32_t)VPConst::SPLIT_SIZE;
            }
        }
        else
        {
            Type* ValueTy = nullptr;
            if (StoreRawIntrinsic* SRI = dyn_cast<StoreRawIntrinsic>(I))
            {
                ValueTy = SRI->getStoreValue()->getType();
            }
            else
            {
                ValueTy = I->getType();
            }
            IGCLLVM::FixedVectorType* vecType = dyn_cast_or_null<IGCLLVM::FixedVectorType>(ValueTy);
            if (vecType && m_DL->getTypeSizeInBits(vecType->getScalarType()) == 64)
            {
                bytes = 8;  // use QW load/store
            }
        }
    }
    else
    {
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
    }

    if ((isa<LoadInst>(I) || isa<StoreInst>(I) ||
         isa<PredicatedLoadIntrinsic>(I) || isa<PredicatedStoreIntrinsic>(I))
         && WI.isUniform(I))
    {
        auto Alignment = ALI.has_value() ? ALI->getAlignment()
                                          : ASI->getAlignment();
        if (Alignment >= 16) {
            Type* ETy = ALI.has_value() ?
                cast<VectorType>(I->getType())->getElementType() :
                cast<VectorType>(ASI->getValueOperand()->getType())->getElementType();

            Value *Ptr = ALI.has_value() ? ALI->getPointerOperand() : ASI->getPointerOperand();
            bool SLM = Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL;
            uint32_t ebytes = int_cast<uint32_t>(m_DL->getTypeSizeInBits(ETy) / 8);
            // Limit to DW and QW element types to avoid generating vectors that
            // are too large (ideally, should be <= 32 elements currently).
            if (ebytes == 4 || ebytes == 8)
            {
                bytes = std::max(bytes, m_CGCtx->platform.getMaxBlockMsgSize(SLM));
            }
        }
    }
    return bytes;
}

bool VectorPreProcess::splitStore(
    AbstractStoreInst& ASI, V2SMap& vecToSubVec, WIAnalysisRunner& WI)
{
    Instruction* SI = ASI.getInst();
    Value* StoredVal = ASI.getValueOperand();
    IGCLLVM::FixedVectorType* VTy = cast<IGCLLVM::FixedVectorType>(StoredVal->getType());
    Type* ETy = VTy->getElementType();
    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());

    // splitInfo: Keep track of all pairs of (sub-vec type, #sub-vec).
    SmallVector<std::pair<Type*, uint32_t>, 8> splitInfo;
    bool isStoreInst = isa<StoreInst>(SI) || isa<PredicatedStoreIntrinsic>(SI);
    uint32_t splitSize = getSplitByteSize(SI, WI);
    if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
    {
        // byte and word-aligned stores can only store a dword at a time.
        auto alignment = ASI.getAlignment();
        if (isStoreInst && alignment < 4)
        {
            alignment_t newAlign = (alignment_t)IGCLLVM::getAlignmentValue(getKnownAlignment(ASI.getPointerOperand(), *m_DL));
            if (newAlign > alignment)
            {
                // For the same reason as Load, use DW-aligned for OCL stateful.
                if (newAlign > 4 && isStatefulAddrSpace(ASI.getPointerAddressSpace()))
                {
                    newAlign = 4;
                }
                ASI.setAlignment(newAlign);
            }
        }
        bool needsDWordSplit =
            (!isStoreInst ||
                m_CGCtx->m_DriverInfo.splitUnalignedVectors() ||
                !WI.isUniform(ASI.getInst()))
            && ASI.getAlignment() < 4;
        if (needsDWordSplit)
        {
            splitSize = 4;
        }
    }
    createSplitVectorTypes(ETy, nelts, splitSize, splitInfo);

    // return if no split
    uint32_t len = splitInfo.size();
    if (len == 1 && splitInfo[0].second == 1)
    {
        return false;
    }

    // Create a new value in the map for store
    ValVector& svals = vecToSubVec[SI][splitSize];
    if (svals.size() == 0)
    {
        // Need to create splitted values.
        Instruction* insertBeforeInst = nullptr;
        ValVector scalars(nelts, nullptr);
        getOrGenScalarValues(*SI->getParent()->getParent(),
            StoredVal, scalars, insertBeforeInst);
        insertBeforeInst = insertBeforeInst ? insertBeforeInst : SI;
        IRBuilder<> aBuilder(insertBeforeInst);

        Type* Ty1 = splitInfo[0].first;
        if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
        {
            if (m_DL->getTypeSizeInBits(ETy) > m_DL->getTypeSizeInBits(Ty1->getScalarType()))
            {
                std::vector<Value*> splitScalars;
                IGC_ASSERT(m_DL->getTypeSizeInBits(Ty1->getScalarType()));
                const uint32_t vectorSize =
                    (unsigned int)m_DL->getTypeSizeInBits(ETy) /
                    (unsigned int)m_DL->getTypeSizeInBits(Ty1->getScalarType());
                Type* splitType = FixedVectorType::get(Ty1, vectorSize);
                for (uint32_t i = 0; i < nelts; i++)
                {
                    Value* splitInst = aBuilder.CreateBitCast(scalars[i], splitType);
                    for (uint32_t j = 0; j < vectorSize; j++)
                    {
                        splitScalars.push_back(aBuilder.CreateExtractElement(splitInst, j));
                    }
                }
                scalars.resize(splitScalars.size());
                for (uint32_t i = 0; i < splitScalars.size(); i++)
                {
                    scalars[i] = splitScalars[i];
                }
            }
        }

        // Now generate svals
        for (uint32_t i = 0, Idx = 0; i < len; ++i)
        {
            Type* Ty1 = splitInfo[i].first;
            uint32_t len1 = splitInfo[i].second;
            IGCLLVM::FixedVectorType* VTy1 = dyn_cast<IGCLLVM::FixedVectorType>(Ty1);
            for (uint32_t j = 0; j < len1; ++j)
            {
                Value* subVec;
                if (!VTy1)
                {
                    subVec = scalars[Idx];
                    ++Idx;
                }
                else
                {
                    subVec = UndefValue::get(Ty1);
                    uint32_t n1 = int_cast<uint32_t>(VTy1->getNumElements());
                    for (uint32_t k = 0; k < n1; ++k)
                    {
                        subVec = aBuilder.CreateInsertElement(
                            subVec,
                            scalars[Idx],
                            aBuilder.getInt32(k));
                        ++Idx;
                    }
                }
                svals.push_back(subVec);
            }
        }
    }

    Value* Addr = ASI.getPointerOperand();
    auto Align = ASI.getAlignment();
    bool IsVolatile = ASI.getIsVolatile();
    uint32_t eOffset = 0;
    uint32_t EBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(ETy));

    for (uint32_t i = 0, subIdx = 0; i < len; ++i)
    {
        Type* Ty1 = splitInfo[i].first;
        uint32_t len1 = splitInfo[i].second;
        IGCLLVM::FixedVectorType* VTy1 = dyn_cast<IGCLLVM::FixedVectorType>(Ty1);
        for (uint32_t j = 0; j < len1; ++j)
        {
            alignment_t vAlign = (alignment_t)MinAlign(Align, (alignment_t)eOffset * EBytes);
            Value* offsetAddr = ASI.CreateConstScalarGEP(svals[subIdx]->getType(), Addr, eOffset);
            Instruction* newST = ASI.Create(svals[subIdx], offsetAddr, vAlign, IsVolatile);
            eOffset += (VTy1 ? int_cast<uint32_t>(VTy1->getNumElements()) : 1);
            ++subIdx;

            // If this is a new 3-element vector, add it into m_Vector3List
            if (VTy1 && VTy1->getNumElements() == 3)
            {
                m_Vector3List.push_back(newST);
            }
        }
    }

    // Stores don't require post processing, so remove it as soon as we finish splitting
    vecToSubVec.erase(SI);
    SI->eraseFromParent();

    // Since Load is processed later, stop optimizing if inst is Load.
    Instruction* inst = dyn_cast<Instruction>(StoredVal);
    bool keepLI = inst && isAbstractLoadInst(inst) &&
        (vecToSubVec.find(inst) != vecToSubVec.end());
    while (inst && !keepLI && inst->use_empty())
    {
        Instruction* next = nullptr;
        if (InsertElementInst * IEI = dyn_cast<InsertElementInst>(inst))
        {
            next = dyn_cast<Instruction>(IEI->getOperand(0));
        }

        inst->eraseFromParent();
        inst = next;
        keepLI = inst && isAbstractLoadInst(inst) &&
            (vecToSubVec.find(inst) != vecToSubVec.end());
    }
    return true;
}

bool VectorPreProcess::splitLoad(
    AbstractLoadInst& ALI, V2SMap& vecToSubVec, WIAnalysisRunner& WI)
{
    Instruction* LI = ALI.getInst();
    bool isLdRaw = isa<LdRawIntrinsic>(LI);
    bool isPredLd = isa<PredicatedLoadIntrinsic>(LI);
    IGCLLVM::FixedVectorType* VTy = cast<IGCLLVM::FixedVectorType>(LI->getType());
    Type* ETy = VTy->getElementType();
    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());

    // Split a vector type into multiple sub-types:
    //       'len0' number of sub-vectors of type 'vecTy0'
    //       'len1' number of sub-vectors of type 'vecTy1'
    //       ...
    // SplitInfo : all pairs, each of which is (sub-vector's type, #sub-vectors).
    SmallVector< std::pair<Type*, uint32_t>, 8 > splitInfo;
    uint32_t splitSize = getSplitByteSize(LI, WI);
    if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
    {
        // byte and word-aligned loads can only load a dword at a time.
        auto alignment = ALI.getAlignment();
        if (!isLdRaw && alignment < 4)
        {
            alignment_t newAlign = (alignment_t)IGCLLVM::getAlignmentValue(getKnownAlignment(ALI.getPointerOperand(), *m_DL));
            if (newAlign > alignment)
            {
                //  For OCL stateful, the base can be as little as DW-aligned. To be safe,
                //  need to use DW-aligned. For example,
                //       % 0 = add i32 0, 16
                //       % 4 = inttoptr i32 % 0 to <8 x i16> addrspace(131073) *
                //       %5 = load <8 x i16>, <8 x i16> addrspace(131073) * %4, align 2
                //  newAlign from getKnownAlignment() is 16. But we can only set align to 4 as
                //  the base of this stateful could be just DW-aligned.
                if (newAlign > 4 && isStatefulAddrSpace(ALI.getPointerAddressSpace()))
                {
                    newAlign = 4;
                }
                ALI.setAlignment(newAlign);
            }
        }

        if ((isLdRaw || !WI.isUniform(ALI.getInst())) && ALI.getAlignment() < 4)
            splitSize = 4;
    }
    createSplitVectorTypes(ETy, nelts, splitSize, splitInfo);

    // return if no split
    uint32_t len = splitInfo.size();
    if (len == 1 &&  splitInfo[0].second == 1)
    {
        return false;
    }

    ValVector splitMergeValues;
    if (isPredLd)
        createSplitMergeValues(LI, cast<PredicatedLoadIntrinsic>(LI)->getMergeValue(), splitInfo, splitMergeValues);

    Value* Addr = ALI.getPointerOperand();
    auto Align = ALI.getAlignment();
    bool IsVolatile = ALI.getIsVolatile();

    uint32_t eOffset = 0;
    uint32_t EBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(ETy));
    uint32_t mergeValueIdx = 0;

    // Create a map entry for LI
    ValVector& svals = vecToSubVec[LI][splitSize];

    for (uint32_t i = 0; i < len; ++i)
    {
        Type* Ty1 = splitInfo[i].first;
        uint32_t len1 = splitInfo[i].second;
        IGCLLVM::FixedVectorType* VTy1 = dyn_cast<IGCLLVM::FixedVectorType>(Ty1);
        for (uint32_t j = 0; j < len1; ++j)
        {
            alignment_t vAlign = (alignment_t)MinAlign(Align, (alignment_t)eOffset * EBytes);
            Value* offsetAddr = ALI.CreateConstScalarGEP(Ty1, Addr, eOffset);
            Value *MergeV = isPredLd ? splitMergeValues[mergeValueIdx++] : nullptr;
            Instruction* I = ALI.Create(Ty1, offsetAddr, vAlign, IsVolatile, MergeV);
            eOffset += (VTy1 ? int_cast<uint32_t>(VTy1->getNumElements()) : 1);

            svals.push_back(I);

            // If this is a new 3-element vector, add it into m_Vector3List
            if (VTy1 && VTy1->getNumElements() == 3)
            {
                m_Vector3List.push_back(I);
            }
        }
    }

    if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
    {
        if (m_DL->getTypeSizeInBits(svals[0]->getType()) < m_DL->getTypeSizeInBits(ETy))
        {
            const unsigned int denominator = (unsigned int)m_DL->getTypeSizeInBits(svals[0]->getType());
            IGC_ASSERT(0 < denominator);

            const uint32_t scalarsPerElement = (unsigned int)m_DL->getTypeSizeInBits(ETy) / denominator;
            IGC_ASSERT(1 < scalarsPerElement);
            IGC_ASSERT((svals.size() % scalarsPerElement) == 0);

            ValVector mergedScalars;
            IRBuilder<> builder(LI->getParent());
            Instruction* nextInst = LI->getNextNode();
            if (nextInst)
            {
                builder.SetInsertPoint(nextInst);
            }
            Value* undef = UndefValue::get(FixedVectorType::get(svals[0]->getType(), scalarsPerElement));
            for (uint32_t i = 0; i < svals.size() / scalarsPerElement; i++)
            {
                Value* newElement = undef;
                for (uint32_t j = 0; j < scalarsPerElement; j++)
                {
                    newElement = builder.CreateInsertElement(newElement, svals[i * scalarsPerElement + j], j);
                }
                mergedScalars.push_back(builder.CreateBitCast(newElement, ETy));
            }
            svals.clear();
            svals.append(mergedScalars.begin(), mergedScalars.end());
        }
    }
    // Put LI in m_Temps for post-processing.
    //
    // LI may be used only in store. If so, no need to re-generate the original
    // vector as load and store will use the same set of sub-vectors. So, we delay
    // generating the original vector until all stores are processed. Doing so,
    // we re-generate the original vector only if it is necessary and thus avoid
    // unnecesary insert/extract instructions.
    m_Temps.push_back(LI);
    return true;
}

bool VectorPreProcess::splitLoadStore(
    Instruction* Inst, V2SMap& vecToSubVec, WIAnalysisRunner& WI)
{
    std::optional<AbstractLoadInst> ALI = AbstractLoadInst::get(Inst, *m_DL);
    std::optional<AbstractStoreInst> ASI = AbstractStoreInst::get(Inst, *m_DL);
    IGC_ASSERT_MESSAGE((ALI || ASI), "Inst should be either load or store");
    Type* Ty = ALI ? ALI->getInst()->getType() : ASI->getValueOperand()->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    if (!VTy)
    {
        return false;
    }

    if (VTy->getNumElements() == 3)
    {
        // Handle 3-element vector later.
        m_Vector3List.push_back(Inst);
        return false;
    }

    Value* V = ALI ? ALI->getInst() : ASI->getInst();

    auto InMap = [&vecToSubVec](Value* V)
    {
        return vecToSubVec.find(V) != vecToSubVec.end();
    };

    // Only LI could be processed already.
    bool processed = ALI && InMap(V);
    if (processed)
    {
        return false;
    }

    // Do splitting

    // If it is a store and its stored value is from a load that
    // has not been splitted yet, then splitting the load first
    // so that the stored value will be directly from loaded values
    // without adding insert/extract instructions.
    std::optional<AbstractLoadInst> aALI = ASI && !InMap(ASI->getValueOperand()) ?
        AbstractLoadInst::get(ASI->getValueOperand(), *m_DL) : std::move(ALI);

    if (aALI)
    {
        auto aALIValue = aALI.value();
        splitLoad(aALIValue, vecToSubVec, WI);
    }

    if (ASI)
    {
        auto ASIValue = ASI.value();
        splitStore(ASIValue, vecToSubVec, WI);
    }

    return true;
}

// For a vector3 whose element size < 4 bytes, will split them into one whose
// size is multiple of DW and one whose size is less than DW; If the size is
// less than DW, make sure it is either 1 Byte or 2 bytes.  After this, for
// vector size < 4, it must be either 1 byte or 2 bytes, never 3  bytes.
// This function also splits vector3s with an element size of 8 bytes if
// ldraw or storeraw is being used since neither of those messages support
// payloads larger than 4 DW.
bool VectorPreProcess::splitVector3LoadStore(Instruction* Inst)
{
    std::optional<AbstractLoadInst> optionalALI = AbstractLoadInst::get(Inst, *m_DL);
    AbstractLoadInst* ALI = optionalALI.has_value() ? &optionalALI.value() : nullptr;
    std::optional<AbstractStoreInst> optionalASI = AbstractStoreInst::get(Inst, *m_DL);
    AbstractStoreInst* ASI = optionalASI.has_value() ? &optionalASI.value() : nullptr;

    std::optional<int> a;
    IGC_ASSERT_MESSAGE((optionalALI || optionalASI), "Inst should be either load or store");
    Type* Ty = ALI ? ALI->getInst()->getType() : ASI->getValueOperand()->getType();
    IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    IGC_ASSERT_MESSAGE(nullptr != VTy, "Inst should be a 3-element vector load/store!");
    IGC_ASSERT_MESSAGE(VTy->getNumElements() == 3, "Inst should be a 3-element vector load/store!");

    Type* eTy = VTy->getElementType();
    uint32_t etyBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(eTy));
    // total size of vector in bytes;
    //uint32_t sz = VTy->getNumElements() * etyBytes;
    GenIntrinsicInst* II = dyn_cast<GenIntrinsicInst>(Inst);
    bool isStoreRaw = II &&
        (II->getIntrinsicID() == GenISAIntrinsic::GenISA_storerawvector_indexed ||
            II->getIntrinsicID() == GenISAIntrinsic::GenISA_storeraw_indexed);
    bool isPredLoad = isa<PredicatedLoadIntrinsic>(Inst);

    if (etyBytes == 1 || etyBytes == 2 ||
        (etyBytes == 8 && (isa<LdRawIntrinsic>(Inst) || isStoreRaw)))
    {
        IRBuilder<> Builder(Inst);
        if (optionalALI)
        {
            Value* Elt0 = NULL;
            Value* Elt1 = NULL;
            Value* Elt2 = NULL;
            bool UseLegacyLdRawMessage = isa<LdRawIntrinsic>(Inst) &&
                EmitPass::shouldGenerateLSCQuery(*m_CGCtx, Inst) != Tristate::True;
            // If alignment is the same as 4-element vector's, it's likely safe
            // to make it 4-element load. (always safe ?)
            if (ALI->getAlignment() >= 4 * etyBytes &&
                // Legacy ldraw message doesn't support 32-byte payloads
                !(UseLegacyLdRawMessage && etyBytes == 8))
            {
                // Make it 4-element load
                Type* newVTy = FixedVectorType::get(eTy, 4);

                ValVector splitMergeValues;
                if (isPredLoad)
                    createSplitMergeValues(Inst, cast<PredicatedLoadIntrinsic>(Inst)->getMergeValue(),
                                           { {newVTy, 1} }, splitMergeValues);

                Value* V = ALI->Create(newVTy, isPredLoad ? splitMergeValues[0] : nullptr);

                Elt0 = Builder.CreateExtractElement(V, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V, Builder.getInt32(1), "elt1");
                Elt2 = Builder.CreateExtractElement(V, Builder.getInt32(2), "elt2");
            }
            else
            {
                // One 2-element vector load + one scalar load
                Type* newVTy = FixedVectorType::get(eTy, 2);
                Value* offsetAddr = ALI->CreateConstScalarGEP(eTy, ALI->getPointerOperand(), 2);

                ValVector splitMergeValues;
                if (isPredLoad)
                    createSplitMergeValues(Inst, cast<PredicatedLoadIntrinsic>(Inst)->getMergeValue(),
                                           { {newVTy, 1}, {eTy, 1} }, splitMergeValues);

                Value* V2 = ALI->Create(newVTy, isPredLoad ? splitMergeValues[0] : nullptr);
                Elt0 = Builder.CreateExtractElement(V2, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V2, Builder.getInt32(1), "elt1");

                uint32_t newAlign = (uint32_t)MinAlign(ALI->getAlignment(), 2 * etyBytes);
                Elt2 = ALI->Create(eTy, offsetAddr, newAlign, ALI->getIsVolatile(), isPredLoad ? splitMergeValues[1] : nullptr);
            }

            // A little optimization here
            ExtractElementInst* EEInsts[3];
            for (int i = 0; i < 3; ++i)
            {
                EEInsts[i] = nullptr;
            }
            if (isValueUsedOnlyByEEI(ALI->getInst(), EEInsts))
            {
                if (EEInsts[0] != nullptr)
                {
                    EEInsts[0]->replaceAllUsesWith(Elt0);
                    EEInsts[0]->eraseFromParent();
                }
                if (EEInsts[1] != nullptr)
                {
                    EEInsts[1]->replaceAllUsesWith(Elt1);
                    EEInsts[1]->eraseFromParent();
                }
                if (EEInsts[2] != nullptr)
                {
                    EEInsts[2]->replaceAllUsesWith(Elt2);
                    EEInsts[2]->eraseFromParent();
                }
            }
            else
            {
                Value* V = Builder.CreateInsertElement(UndefValue::get(VTy), Elt0,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(V, Elt1, Builder.getInt32(1));
                V = Builder.CreateInsertElement(V, Elt2, Builder.getInt32(2));
                ALI->getInst()->replaceAllUsesWith(V);
            }
            ALI->getInst()->eraseFromParent();
        }
        else
        {
            Value* Ptr = ASI->getPointerOperand();
            // Split 3-element into 2-element + 1 scalar
            Type* newVTy = FixedVectorType::get(eTy, 2);
            Value* StoredVal = ASI->getValueOperand();
            Value* offsetAddr = ASI->CreateConstScalarGEP(StoredVal->getType(), Ptr, 2);
            InsertElementInst* IEInsts[3];
            for (int i = 0; i < 3; ++i)
            {
                IEInsts[i] = nullptr;
            }

            // vec3 = vec2 + scalar,  newAlign is an alignment for scalar store.
            uint32_t newAlign = (uint32_t)MinAlign(ASI->getAlignment(), 2 * etyBytes);
            Value* UDVal = UndefValue::get(eTy);
            if (isValueCreatedOnlyByIEI(ASI->getInst(), IEInsts))
            {
                // This case should be most frequent, and want
                // to generate a better code by removing dead
                // InsertElementInst.

                // Be ware of partial vector store.
                Value* V = UndefValue::get(newVTy);
                V = Builder.CreateInsertElement(
                    V, (IEInsts[0] != nullptr) ? IEInsts[0]->getOperand(1) : UDVal,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(
                    V, (IEInsts[1] != nullptr) ? IEInsts[1]->getOperand(1) : UDVal,
                    Builder.getInt32(1));
                V = ASI->Create(V);

                // If IEInsts[2] is undefined, skip scalar store.
                if (IEInsts[2] != nullptr)
                {
                    (void)ASI->Create(IEInsts[2]->getOperand(1), offsetAddr, newAlign, ASI->getIsVolatile());
                }
                ASI->getInst()->eraseFromParent();

                // Remove all InsertElementInst if possible
                bool change = true;
                while (change)
                {
                    change = false;
                    for (int i = 0; i < 3; ++i)
                    {
                        if (IEInsts[i] && IEInsts[i]->use_empty())
                        {
                            IEInsts[i]->eraseFromParent();
                            IEInsts[i] = nullptr;
                            change = true;
                        }
                    }
                }
            }
            else
            {
                // Get a 2-element vector and a scalar from the
                // 3-element vector and store them respectively.
                // Shuffle isn't handled in Emit, use extract/insert instead
                Value* Elt0 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(0),
                    "Elt0");
                Value* Elt1 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(1),
                    "Elt1");
                Value* Elt2 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(2),
                    "Elt2");
                Value* V = Builder.CreateInsertElement(UndefValue::get(newVTy),
                    Elt0,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(V, Elt1, Builder.getInt32(1));
                ASI->Create(V);
                ASI->Create(Elt2, offsetAddr, newAlign, ASI->getIsVolatile());
                ASI->getInst()->eraseFromParent();
            }
        }
        return true;
    }
    return false;
}

// availBeforeInst:
//    Indicate that all scalar values of VecVal are available right before
//    instruction 'availBeforeInst'. If availBeforeInst is null, it means
//    all scalar values are constants.
void VectorPreProcess::getOrGenScalarValues(
    Function& F, Value* VecVal, ValVector& scalars, Instruction*& availBeforeInst)
{
    availBeforeInst = nullptr;

    IGCLLVM::FixedVectorType* VTy = cast<IGCLLVM::FixedVectorType>(VecVal->getType());
    if (!VTy)
    {
        scalars[0] = VecVal;
        return;
    }

    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());
    Type* ETy = VTy->getElementType();
    if (isa<UndefValue>(VecVal))
    {
        Value* udv = UndefValue::get(ETy);
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = udv;
        }
    }
    else if (ConstantVector * CV = dyn_cast<ConstantVector>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CV->getOperand(i);
        }
    }
    else if (ConstantDataVector * CDV = dyn_cast<ConstantDataVector>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CDV->getElementAsConstant(i);
        }
    }
    else if (ConstantAggregateZero * CAZ = dyn_cast<ConstantAggregateZero>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CAZ->getSequentialElement();
        }
    }
    else
    {
        bool genExtract = false;
        Value* V = VecVal;
        IGC_ASSERT(scalars.size() == nelts);
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = nullptr;
        }
        uint32_t numEltsFound = 0;
        while (InsertElementInst * IEI = dyn_cast<InsertElementInst>(V))
        {
            Value* ixVal = IEI->getOperand(2);
            ConstantInt* CI = dyn_cast<ConstantInt>(ixVal);
            if (!CI)
            {
                genExtract = true;
                break;
            }
            uint32_t ix = int_cast<unsigned int>(CI->getZExtValue());
            if (scalars[ix] == nullptr)
            {
                scalars[ix] = IEI->getOperand(1);
                ++numEltsFound;
            }
            if (numEltsFound == nelts)
            {
                break;
            }
            V = IEI->getOperand(0);
        }
        // Generate extractelement instructions if not all elements were found.
        if (!isa<UndefValue>(V) && numEltsFound != nelts)
        {
            genExtract = true;
        }

        BasicBlock::iterator inst_b;
        if (Instruction * I = dyn_cast<Instruction>(VecVal))
        {
            if (auto phi = dyn_cast<PHINode>(I))
            {
                // avoid inserting between phis
                inst_b = phi->getParent()->getFirstInsertionPt();
            }
            else
            {
                inst_b = BasicBlock::iterator(I);
                ++inst_b;
            }
        }
        else
        {
            // VecVal is an argument or constant
            inst_b = F.begin()->getFirstInsertionPt();
        }

        IRBuilder<> Builder(&(*inst_b));
        for (uint32_t i = 0; i < nelts; ++i)
        {
            if (scalars[i] == nullptr)
            {
                Value* S;
                if (genExtract)
                {
                    S = Builder.CreateExtractElement(V, Builder.getInt32(i));
                }
                else
                {
                    S = UndefValue::get(ETy);
                }
                scalars[i] = S;
            }
        }

        availBeforeInst = &(*inst_b);
    }
}

// Perform LoadInst/StoreInst simplification. E.g. The following vector load is
// only used by three extractelements with constants indices, so we can narrow
// the load width to 3.
//
// %34 = load <4 x float> addrspace(1)* %33, align 16
// %scalar35 = extractelement <4 x float> %34, i32 0
// %scalar36 = extractelement <4 x float> %34, i32 1
// %scalar47 = extractelement <4 x float> %34, i32 2
//
// %40 = bitcast <4 x float> addrspace(1)* %33 to <3 x float> addrspace(1)*
// %41 = load <3 x float> addrspace(1)* %40, align 16 (keep alignment!)
// %scalar42 = extractelement <3 x float> %41, i32 0
// %scalar43 = extractelement <3 x float> %41, i32 1
// %scalar44 = extractelement <3 x float> %41, i32 2
//
Instruction* VectorPreProcess::simplifyLoadStore(Instruction* Inst)
{
    if (std::optional<AbstractLoadInst> optionalALI = AbstractLoadInst::get(Inst, *m_DL))
    {
        bool optReportEnabled = IGC_IS_FLAG_ENABLED(EnableOptReportLoadNarrowing);
        auto emitOptReport = [&](std::string report, Instruction* from, Instruction* to)
        {
            std::string strFrom;
            llvm::raw_string_ostream rsoFrom(strFrom);
            from->print(rsoFrom);

            std::string strTo;
            llvm::raw_string_ostream rsoTo(strTo);
            to->print(rsoTo);

            std::stringstream optReportFile;
            optReportFile << IGC::Debug::GetShaderOutputFolder() << "LoadNarrowing.opt";

            std::ofstream optReportStream;
            optReportStream.open(optReportFile.str(), std::ios::app);
            optReportStream << IGC::Debug::GetShaderOutputName() << ": "
                << report << std::endl
                << rsoFrom.str() << " ->" << std::endl
                << rsoTo.str() << std::endl;
        };

        auto ALI = optionalALI.value();
        if (!Inst->getType()->isVectorTy() || ALI.getAlignment() < 4)
            return Inst;

        unsigned NBits = int_cast<unsigned>(
            m_DL->getTypeSizeInBits(Inst->getType()->getScalarType()));
        if (NBits < 32)
            return Inst;

        BitCastInst* BC = nullptr;
        Type* DstEltTy = nullptr;
        // Handle bitcasts patterns like:
        //
        // %41 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p1v4f32(...)
        // %bc = bitcast <4 x i32> %41 to <4 x float>
        // %42 = extractelement <4 x float> %bc, i32 0
        if (Inst->hasOneUse())
        {
            BC = dyn_cast<BitCastInst>(*Inst->users().begin());
            if (BC)
            {
                IGCLLVM::FixedVectorType* DstVTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getType());
                IGCLLVM::FixedVectorType* SrcVTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getOperand(0)->getType());
                if (IGC_IS_FLAG_DISABLED(EnableBitcastedLoadNarrowing) ||
                    !DstVTy || !SrcVTy || DstVTy->getNumElements() != SrcVTy->getNumElements())
                {
                    BC = nullptr;
                }
                else
                {
                    DstEltTy = DstVTy->getElementType();
                }
            }
        }

        SmallVector<ExtractElementInst*, 8> ConstEEIUses;
        unsigned MaxIndex = 0;
        for (auto U : (BC ? BC : Inst)->users())
        {
            auto EEI = dyn_cast<ExtractElementInst>(U);
            if (!EEI || !isa<ConstantInt>(EEI->getIndexOperand()))
                return Inst;

            auto CI = cast<ConstantInt>(EEI->getIndexOperand());
            ConstEEIUses.push_back(EEI);
            MaxIndex = std::max(MaxIndex, int_cast<unsigned>(CI->getZExtValue()));
        }

        // All uses are constant EEI.
        IGC_ASSERT_MESSAGE((BC ? BC : Inst)->hasNUses(ConstEEIUses.size()), "out of sync");

        // FIXME: this is to WA an issue that splitLoadStore does not split
        // vectors of size 5, 6, 7.
        if (MaxIndex + 1 > 4)
            return Inst;

        // If MaxIndex is smaller than <vector_size - 1>, then narrow the size
        // of this vector load to reduce unnecessary memory load.
        //
        // TODO: further optimize this load into a message with channel masks
        // for cases in which use indices are sparse like {0, 2}.
        unsigned N = (unsigned)cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
        if (N == MaxIndex + 1)
            return Inst;

        //Check if we can turn a ldrawvector into a ldraw
        Instruction* NewLI = nullptr;
        IRBuilder<> Builder(Inst);
        auto ldrawvec = dyn_cast<LdRawIntrinsic>(Inst);
        bool canSimplifyOneUse = ldrawvec && isa<VectorType>(ldrawvec->getType()) &&
            (BC ? BC : Inst)->hasOneUse() &&
            !ConstEEIUses.empty();

        bool canSimplifyOneUseZeroIndex = canSimplifyOneUse &&
            cast<ConstantInt>(ConstEEIUses.front()->getIndexOperand())->getZExtValue() == 0;

        // There is a known case where narrowing bitcasted ldrawvector to ldraw
        // leads to a corruption. We can still simplify a vector load to
        // a narrow one (e.g. <4 x i32> to <2 x i32> when only 0th elt is used
        // as a float).
        // TODO: remove WA when issue is resolved.
        bool skipSimplifyBitcastedOneUse = canSimplifyOneUse &&
            BC && IGC_IS_FLAG_DISABLED(EnableBitcastedLoadNarrowingToScalar);

        auto simplifyLDVecToLDRaw = [&](bool calc_offset)
        {
            auto EE_user = ConstEEIUses.front();
            auto return_type = cast<VectorType>(ldrawvec->getType())->getElementType();
            auto buffer_ptr = ldrawvec->getResourceValue();
            Value* OffsetVal = ldrawvec->getOffsetValue();
            auto alloc_size = (unsigned)m_DL->getTypeAllocSize(return_type);
            if (calc_offset)
            {
                auto EE_index = (unsigned)cast<ConstantInt>(EE_user->getIndexOperand())->getZExtValue();
                if (isa<ConstantInt>(OffsetVal))
                {
                    // Calculate static offset
                    auto offset = (unsigned)cast<ConstantInt>(OffsetVal)->getZExtValue();
                    auto new_offset = offset + (EE_index * alloc_size);
                    OffsetVal = Builder.getInt32(new_offset);
                }
                else
                {
                    // Calculate runtime offset
                    OffsetVal = Builder.CreateAdd(OffsetVal, Builder.getInt32(EE_index * alloc_size));
                }
            }
            Type* types[2] = { return_type , buffer_ptr->getType() };
            Value* args[4] = { buffer_ptr, OffsetVal,
                Builder.getInt32(alloc_size), Builder.getInt1(ldrawvec->isVolatile()) };
            Function* newLdRawFunction =
                GenISAIntrinsic::getDeclaration(ldrawvec->getModule(), GenISAIntrinsic::GenISA_ldraw_indexed, types);
            NewLI = Builder.CreateCall(newLdRawFunction, args);
            NewLI->setDebugLoc(EE_user->getDebugLoc());

            if (optReportEnabled)
            {
                std::string type;
                llvm::raw_string_ostream rsoType(type);
                Inst->getType()->print(rsoType);

                std::stringstream report;
                report << (BC ? "Bitcasted vector" : "Vector") << " load of " << rsoType.str()
                    << " is transformed to scalar load";
                if (calc_offset)
                {
                    report << (isa<ConstantInt>(ldrawvec->getOffsetValue())
                        ? ", static offset added"
                        : ", runtime offset added");
                }
                report << ":";
                emitOptReport(report.str(), Inst, NewLI);
            }

            Value* NewBC = nullptr;
            if (BC)
            {
                NewBC = Builder.CreateBitCast(NewLI, DstEltTy);
            }
            EE_user->replaceAllUsesWith(BC ? NewBC : NewLI);
            EE_user->eraseFromParent();
            if (BC) {
                BC->eraseFromParent();
            }
        };

        if (canSimplifyOneUseZeroIndex && !skipSimplifyBitcastedOneUse)
        {
            simplifyLDVecToLDRaw(false);
            return NewLI;
        }
        else if (canSimplifyOneUse && !skipSimplifyBitcastedOneUse)
        {
            simplifyLDVecToLDRaw(true);
            return NewLI;
        }
        else
        {
            // WA: Do not narrow a bitcasted vector load to 1 elt vector load,
            // choose at least 2 elts vector.
            if (canSimplifyOneUseZeroIndex && skipSimplifyBitcastedOneUse)
            {
                MaxIndex = 1;
            }

            Type* NewVecTy = FixedVectorType::get(cast<VectorType>(Inst->getType())->getElementType(),
                MaxIndex + 1);

            bool isPredLoad = isa<PredicatedLoadIntrinsic>(Inst);
            ValVector splitMergeValues;
            if (isPredLoad)
                createSplitMergeValues(Inst, cast<PredicatedLoadIntrinsic>(Inst)->getMergeValue(),
                                       { {NewVecTy, 1} }, splitMergeValues);

            NewLI = ALI.Create(NewVecTy, isPredLoad ? splitMergeValues[0] : nullptr);

            if (optReportEnabled)
            {
                std::string type, narrowedType;
                llvm::raw_string_ostream rsoType(type), rsoNarrowedType(narrowedType);
                Inst->getType()->print(rsoType);
                NewVecTy->print(rsoNarrowedType);

                std::stringstream report;
                report << (BC ? "Bitcasted vector" : "Vector") << " load of " << rsoType.str()
                    << " is narrowed to vector load of " << rsoNarrowedType.str();
                if (canSimplifyOneUseZeroIndex && skipSimplifyBitcastedOneUse)
                {
                    report << " (narrowing to scalar load is disabled by WA)";
                }
                report << ":";
                emitOptReport(report.str(), Inst, NewLI);
            }

            // Loop and replace all uses.
            SmallVector<Value*, 8> NewEEI(MaxIndex + 1, nullptr);
            SmallVector<Value*, 8> NewBC(MaxIndex + 1, nullptr);
            for (auto EEI : ConstEEIUses)
            {
                auto CI = cast<ConstantInt>(EEI->getIndexOperand());
                unsigned Idx = int_cast<unsigned>(CI->getZExtValue());
                if (NewEEI[Idx] == nullptr)
                {
                    NewEEI[Idx] = Builder.CreateExtractElement(NewLI, CI);
                    if (BC)
                    {
                        NewBC[Idx] = Builder.CreateBitCast(NewEEI[Idx], DstEltTy);
                        cast<BitCastInst>(NewBC[Idx])->setDebugLoc(BC->getDebugLoc());
                    }
                }
                cast<ExtractElementInst>(NewEEI[Idx])->setDebugLoc(EEI->getDebugLoc());
                EEI->replaceAllUsesWith(BC ? NewBC[Idx] : NewEEI[Idx]);
                EEI->eraseFromParent();
            }
            if (BC) {
                BC->eraseFromParent();
            }
            IGC_ASSERT_MESSAGE(Inst->use_empty(), "out of sync");
            Inst->eraseFromParent();
            return NewLI;
        }
    }

    // %2 = insertelement <4 x float> undef, float 1.000000e+00, i32 0
    // %3 = insertelement <4 x float> %2, float 1.000000e+00, i32 1
    // %4 = insertelement <4 x float> %3, float 1.000000e+00, i32 2
    // store <4 x float> %4, <4 x float>* %1, align 16
    //
    // becomes
    //
    // %5 = bitcast <4 x float>* %1 to <3 x float>*
    // %6 = insertelement <3 x float> undef, float 1.000000e+00, i32 0
    // %7 = insertelement <3 x float> %2, float 1.000000e+00, i32 1
    // %8 = insertelement <3 x float> %3, float 1.000000e+00, i32 2
    // store <3 x float> %8, <3 x float>* %5, align 16
    //
    IGC_ASSERT(isAbstractStoreInst(Inst));
    std::optional<AbstractStoreInst> optionalASI = AbstractStoreInst::get(Inst, *m_DL);
    auto ASI = optionalASI.value();
    Value* Val = ASI.getValueOperand();
    if (isa<UndefValue>(Val))
    {
        Inst->eraseFromParent();
        return nullptr;
    }

    if (!Val->getType()->isVectorTy() || ASI.getAlignment() < 4)
        return Inst;

    unsigned NBits = int_cast<unsigned>(
        m_DL->getTypeSizeInBits(Val->getType()->getScalarType()));
    if (NBits < 32)
        return Inst;

    unsigned N = (unsigned)cast<IGCLLVM::FixedVectorType>(Val->getType())->getNumElements();
    if (auto CV = dyn_cast<ConstantVector>(Val))
    {
        unsigned MaxIndex = 0;
        for (unsigned i = N - 1; i != 0; --i)
        {
            Constant* Item = CV->getAggregateElement(i);
            if (!isa<UndefValue>(Item))
            {
                MaxIndex = i;
                break;
            }
        }

        if (MaxIndex + 1 == N)
            return Inst;

        SmallVector<Constant*, 8> Data(MaxIndex + 1, nullptr);
        for (unsigned i = 0; i <= MaxIndex; ++i)
        {
            Data[i] = CV->getAggregateElement(i);
        }
        auto SVal = ConstantVector::get(Data);
        Instruction* NewSI = ASI.Create(SVal);
        ASI.getInst()->eraseFromParent();
        return NewSI;
    }

    SmallVector<InsertElementInst*, 8> ConstIEIs(N, nullptr);
    Value* ChainVal = Val;
    int MaxIndex = -1;
    while (auto IEI = dyn_cast<InsertElementInst>(ChainVal))
    {
        if (MaxIndex + 1 == (int)N || !isa<ConstantInt>(IEI->getOperand(2)))
        {
            return Inst;
        }

        // Make sure the last IEI will be recorded if an element is
        // inserted multiple times.
        auto CI = cast<ConstantInt>(IEI->getOperand(2));
        int Idx = (int)CI->getZExtValue();
        if (ConstIEIs[Idx] == nullptr)
        {
            ConstIEIs[Idx] = IEI;
        }
        MaxIndex = std::max(MaxIndex, Idx);
        ChainVal = IEI->getOperand(0);
    }

    // FIXME: this is to WA an issue that splitLoadStore does not split
    // vectors of size 5, 6, 7.
    if (MaxIndex + 1 > 4)
        return Inst;

    // Inserted less than N values into Undef.
    if (MaxIndex >= 0 && MaxIndex + 1 < (int)N && isa<UndefValue>(ChainVal))
    {
        IRBuilder<> Builder(ASI.getInst());
        Type* NewVecTy = FixedVectorType::get(cast<VectorType>(Val->getType())->getElementType(),
            MaxIndex + 1);
        Value* SVal = UndefValue::get(NewVecTy);
        for (int i = 0; i <= MaxIndex; ++i)
        {
            if (ConstIEIs[i] != nullptr)
            {
                SVal = Builder.CreateInsertElement(SVal,
                    ConstIEIs[i]->getOperand(1),
                    ConstIEIs[i]->getOperand(2));
            }
        }
        Instruction* NewSI = ASI.Create(SVal);
        ASI.getInst()->eraseFromParent();
        return NewSI;
    }

    return Inst;
}

// Replace store instructions like
// store i24 %1, i24 addrspace(3)* %2, align 4
// or
// store i48 %1, i48 addrspace(3)* %2, align 4
//
// with
// store <3 x i8> %3, <3 x i8> addrspace(3)* %4, align 4
// or
// store <3 x i16> %16, <3 x i16> addrspace(3)* %4, align 4
//
// to be split later in this pass.
// Otherwise later TypeLegalizwe pass replaces these instructions with 3-element store.
// The same is for i24 and i48 load instructions.
//
bool VectorPreProcess::processScalarLoadStore(Function& F)
{
    InstWorkVector list_delete;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    {
        Instruction* inst = &*I;
        if (isa<StoreInst>(inst) || isa<PredicatedStoreIntrinsic>(inst))
        {
            std::optional<AbstractStoreInst> optionalASI = AbstractStoreInst::get(inst, *m_DL);
            auto ASI = optionalASI.value();

            Type* Ty = ASI.getValueOperand()->getType();
            if (Ty->isVectorTy())
                continue;
            unsigned bitSize = int_cast<unsigned>(
                m_DL->getTypeSizeInBits(Ty->getScalarType()));
            if (bitSize != 24 && bitSize != 48)
                continue;
            IRBuilder<> Builder(inst);
            Type* newScalTy = bitSize == 24 ? Type::getInt8Ty(inst->getContext()) : Type::getInt16Ty(inst->getContext());
            Type* newVecTy = IGCLLVM::FixedVectorType::get(newScalTy, 3);
            ASI.Create(Builder.CreateBitCast(ASI.getValueOperand(), newVecTy));
            list_delete.push_back(inst);
        }
        else if (isa<LoadInst>(inst) || isa<PredicatedLoadIntrinsic>(inst))
        {
            std::optional<AbstractLoadInst> optionalALI = AbstractLoadInst::get(inst, *m_DL);
            auto ALI = optionalALI.value();

            Type* Ty = inst->getType();
            if (Ty->isVectorTy())
                continue;
            unsigned bitSize = int_cast<unsigned>(
                m_DL->getTypeSizeInBits(Ty->getScalarType()));
            if (bitSize != 24 && bitSize != 48)
                continue;
            IRBuilder<> Builder(inst);
            Type* newScalTy = bitSize == 24 ? Type::getInt8Ty(inst->getContext()) : Type::getInt16Ty(inst->getContext());
            Type* newVecTy = IGCLLVM::FixedVectorType::get(newScalTy, 3);

            bool isPredLd = isa<PredicatedLoadIntrinsic>(inst);
            ValVector splitMergeValues;
            if (isPredLd)
                createSplitMergeValues(inst, cast<PredicatedLoadIntrinsic>(inst)->getMergeValue(),
                                       { {newVecTy, 1} }, splitMergeValues);
            Value *MergeVal = isPredLd ? splitMergeValues[0] : nullptr;

            Value* newVecVal = ALI.Create(newVecTy, MergeVal);
            Value* newVal = Builder.CreateBitCast(newVecVal, Ty);
            inst->replaceAllUsesWith(newVal);
            list_delete.push_back(inst);
        }
    }

    if (list_delete.empty())
        return false;

    for (auto i : list_delete)
    {
        i->eraseFromParent();
    }
    return true;
}

bool VectorPreProcess::runOnFunction(Function& F)
{
    bool changed = false;
    m_DL = &F.getParent()->getDataLayout();
    m_C = &F.getContext();
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    changed = processScalarLoadStore(F);

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    {
        Instruction* inst = &*I;
        if (isAbstractStoreInst(inst) || isAbstractLoadInst(inst))
        {
            m_WorkList.push_back(inst);
        }
    }

    // Simplify loads/stores.
    bool Simplified = false;
    for (unsigned i = 0, n = m_WorkList.size(); i < n; ++i)
    {
        Instruction* Inst = m_WorkList[i];
        Instruction* NewInst = simplifyLoadStore(Inst);
        if (NewInst != Inst)
        {
            m_WorkList[i] = NewInst;
            Simplified = true;
        }
    }

    // Cleanup work items, only keep load and store instructions.
    if (Simplified)
    {
        changed = true;
        auto new_end = std::remove_if(m_WorkList.begin(), m_WorkList.end(),
            [](Value* V) {
            return !V || (!isAbstractStoreInst(V) && !isAbstractLoadInst(V));
        });
        m_WorkList.erase(new_end, m_WorkList.end());
    }

    // Split vectors
    if (m_WorkList.size() > 0)
    {
        V2SMap vecToSubVec;

        // m_Temps is used to keep loads that needs post-processing.
        m_Temps.clear();

        {
            auto* MDUtils =
                getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            auto* DT =
                &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
            auto* PDT =
                &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
            auto *LI =
                &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
            auto* ModMD =
                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

            TranslationTable TT;
            TT.run(F);
            WIAnalysisRunner WI(&F, LI, DT, PDT, MDUtils, m_CGCtx, ModMD, &TT);
            WI.run();

            for (uint32_t i = 0; i < m_WorkList.size(); ++i)
            {
                if (splitLoadStore(m_WorkList[i], vecToSubVec, WI))
                {
                    changed = true;
                }
            }
        }

        // Now, do post-processing for the splitted loads
        for (uint32_t i = 0; i < m_Temps.size(); ++i)
        {
            Value* V = m_Temps[i];
            std::optional<AbstractLoadInst> ALI = AbstractLoadInst::get(V, *m_DL);
            if (!ALI)
            {
                continue;
            }
            Instruction* LI = ALI.value().getInst();

            for (auto& it : vecToSubVec[LI])
            {
                ValVector& svals = it.second;
                if (!LI->use_empty())
                {
                    ValVector Scalars;
                    IRBuilder<> Builder(LI);
                    for (uint32_t j = 0; j < svals.size(); ++j)
                    {
                        Type* Ty1 = svals[j]->getType();
                        IGCLLVM::FixedVectorType* VTy1 = dyn_cast<IGCLLVM::FixedVectorType>(Ty1);
                        if (VTy1) {
                            for (uint32_t k = 0; k < VTy1->getNumElements(); ++k)
                            {
                                Value* S = Builder.CreateExtractElement(
                                    svals[j], Builder.getInt32(k), "split");
                                Scalars.push_back(S);
                            }
                        }
                        else
                        {
                            Scalars.push_back(svals[j]);

                            // svals[j] will be no long needed, set it to null
                            // to prevent double-deleting later
                            svals[j] = nullptr;
                        }
                    }
                    // Replace LI
                    replaceAllVectorUsesWithScalars(LI, Scalars);

                    // Remove any dead scalars
                    for (uint32_t j = 0; j < Scalars.size(); ++j)
                    {
                        if (Scalars[j]->use_empty())
                        {
                            Instruction* tInst = cast<Instruction>(Scalars[j]);
                            tInst->eraseFromParent();
                        }
                    }
                }

                // Remove any dead sub vectors
                for (uint32_t j = 0; j < svals.size(); ++j)
                {
                    if (svals[j] == nullptr)
                    {
                        continue;
                    }
                    Instruction* tInst = cast<Instruction>(svals[j]);
                    if (tInst->use_empty())
                    {
                        // If this is a 3-element vector load, remove it
                        // from m_Vector3List as well.
                        if (isAbstractLoadInst(tInst) && tInst->getType()->isVectorTy() &&
                            cast<IGCLLVM::FixedVectorType>(tInst->getType())->getNumElements() == 3)
                        {
                            InstWorkVector::iterator
                                tI = m_Vector3List.begin(),
                                tE = m_Vector3List.end();
                            for (; tI != tE; ++tI)
                            {
                                Instruction* tmp = *tI;
                                if (tmp == tInst)
                                {
                                    break;
                                }
                            }
                            if (tI != m_Vector3List.end())
                            {
                                m_Vector3List.erase(tI);
                            }
                        }

                        tInst->eraseFromParent();
                    }
                }
            }

            // Done with load splits, remove the original load inst
            if (LI->use_empty())
            {
                vecToSubVec.erase(LI);
                LI->eraseFromParent();
            }
        }

        // Last, split 3-element vector if necessary
        for (uint32_t i = 0; i < m_Vector3List.size(); ++i)
        {
            if (splitVector3LoadStore(m_Vector3List[i]))
            {
                changed = true;
            }
        }

        vecToSubVec.clear();
        m_Vector3List.clear();
        m_WorkList.clear();
    }
    return changed;
}
