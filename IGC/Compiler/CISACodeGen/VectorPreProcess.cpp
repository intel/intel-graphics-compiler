/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
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
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <utility>    // std::pair, std::make_pair

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
    // between ldraw and Load and between storeraw and Store.
    // Note on usage: The Value* passed as the ptr paramter to the Create method
    // should be either the result of the getPointerOperand() method or the
    // CreateConstScalarGEP() method. Do not attempt to do arithmetic
    // (or pointer arithmetic) on these values.
    class AbstractLoadInst
    {
        Instruction* const m_inst;
        AbstractLoadInst(LoadInst* LI) : m_inst(LI) {}
        AbstractLoadInst(LdRawIntrinsic* LdRI) : m_inst(LdRI) {}

        LoadInst* getLoad() const
        {
            return cast<LoadInst>(m_inst);
        }
        LdRawIntrinsic* getLdRaw() const
        {
            return cast<LdRawIntrinsic>(m_inst);
        }
    public:
        Instruction* getInst() const
        {
            return m_inst;
        }
        unsigned int getAlignment() const
        {
            return isa<LoadInst>(m_inst) ? getLoad()->getAlignment() : getLdRaw()->getAlignment();
        }
        void setAlignment(unsigned int alignment)
        {
            if (isa<LoadInst>(m_inst))
            {
                getLoad()->setAlignment(IGCLLVM::getCorrectAlign(alignment));
            }
            else
            {
                getLdRaw()->setAlignment(alignment);
            }
        }
        Value* getPointerOperand() const
        {
            return isa<LoadInst>(m_inst) ? getLoad()->getPointerOperand() : getLdRaw()->getResourceValue();
        }
        bool getIsVolatile() const
        {
            return isa<LoadInst>(m_inst) ? getLoad()->isVolatile() : getLdRaw()->isVolatile();
        }
        Instruction* Create(Type* returnType)
        {
            return Create(returnType, getPointerOperand(), getAlignment(), getIsVolatile());
        }
        Instruction* Create(Type* returnType, Value* ptr, unsigned int alignment, bool isVolatile)
        {
            IRBuilder<> builder(m_inst);
            if (isa<LoadInst>(m_inst))
            {
                Type* newPtrType = PointerType::get(returnType, ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, newPtrType);
                return builder.CreateAlignedLoad(ptr, IGCLLVM::getAlign(alignment), isVolatile);
            }
            else
            {
                LdRawIntrinsic* ldraw = getLdRaw();
                bool hasComputedOffset = ptr != ldraw->getResourceValue();
                Value* offsetVal = hasComputedOffset ? ptr : ldraw->getOffsetValue();
                ptr = ldraw->getResourceValue();
                Type* types[2] = { returnType , ptr->getType() };
                Value* args[4] = { ptr, offsetVal, builder.getInt32(alignment), builder.getInt1(isVolatile) };
                Function* newLdRawFunction =
                    GenISAIntrinsic::getDeclaration(ldraw->getModule(), ldraw->getIntrinsicID(), types);
                return builder.CreateCall(newLdRawFunction, args);
            }
        }
        // Emulates a GEP on a pointer of the scalar type of returnType.
        Value* CreateConstScalarGEP(Type* returnType, Value* ptr, uint32_t offset)
        {
            IRBuilder<> builder(m_inst);
            if (isa<LoadInst>(m_inst))
            {
                Type* ePtrType = PointerType::get(returnType->getScalarType(), ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, ePtrType);
                return builder.CreateConstGEP1_32(ptr, offset);
            }
            else
            {
                Value* offsetInBytes = builder.getInt32(offset * returnType->getScalarSizeInBits() / 8);
                return builder.CreateAdd(offsetInBytes, getLdRaw()->getOffsetValue());
            }
        }
        static Optional<AbstractLoadInst> get(llvm::Value* value)
        {
            if (LoadInst * LI = dyn_cast<LoadInst>(value))
            {
                return Optional<AbstractLoadInst>(LI);
            }
            else if (LdRawIntrinsic * LdRI = dyn_cast<LdRawIntrinsic>(value))
            {
                return Optional<AbstractLoadInst>(LdRI);
            }
            else
            {
                return Optional<AbstractLoadInst>();
            }
        }
    };
    static bool isAbstractLoadInst(llvm::Value* value)
    {
        return isa<LoadInst>(value) || isa<LdRawIntrinsic>(value);
    }

    class AbstractStoreInst
    {
        Instruction* const m_inst;
        AbstractStoreInst(StoreInst* SI) : m_inst(SI) {}
        AbstractStoreInst(StoreRawIntrinsic* SRI) : m_inst(SRI) {}

        StoreInst* getStore() const
        {
            return cast<StoreInst>(m_inst);
        }
        StoreRawIntrinsic* getStoreRaw() const
        {
            return cast<StoreRawIntrinsic>(m_inst);
        }
    public:
        Instruction* getInst() const
        {
            return m_inst;
        }
        unsigned int getAlignment() const
        {
            return isa<StoreInst>(m_inst) ? getStore()->getAlignment() : getStoreRaw()->getAlignment();
        }
        void setAlignment(unsigned int alignment)
        {
            if (isa<StoreInst>(m_inst))
            {
                getStore()->setAlignment(IGCLLVM::getCorrectAlign(alignment));
            }
        }
        Value* getValueOperand() const
        {
            return isa<StoreInst>(m_inst) ? getStore()->getValueOperand() : getStoreRaw()->getArgOperand(2);
        }
        Value* getPointerOperand() const
        {
            return isa<StoreInst>(m_inst) ? getStore()->getPointerOperand() : getStoreRaw()->getArgOperand(0);
        }
        bool getIsVolatile() const
        {
            return isa<StoreInst>(m_inst) ? getStore()->isVolatile() : false;
        }
        Instruction* Create(Value* storedValue, Value* ptr, unsigned int alignment, bool isVolatile)
        {
            IRBuilder<> builder(m_inst);
            Type* newType = storedValue->getType();
            if (isa<StoreInst>(m_inst))
            {
                Type* newPtrType = PointerType::get(newType, ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, newPtrType);
                return builder.CreateAlignedStore(storedValue, ptr, IGCLLVM::getAlign(alignment), isVolatile);
            }
            else
            {
                bool hasComputedOffset = ptr != getPointerOperand();
                Value* offset = hasComputedOffset ? ptr : getStoreRaw()->getArgOperand(1);
                ptr = getPointerOperand();
                Type* types[2] = { ptr->getType(), newType };
                Value* args[5] = { ptr, offset, storedValue, builder.getInt32(alignment), builder.getInt1(isVolatile) };
                Function* newStoreRawFunction =
                    GenISAIntrinsic::getDeclaration(getStoreRaw()->getModule(), getStoreRaw()->getIntrinsicID(), types);
                return builder.CreateCall(newStoreRawFunction, args);
            }
        }
        Instruction* Create(Value* storedValue)
        {
            return Create(storedValue, getPointerOperand(), getAlignment(), getIsVolatile());
        }
        // Emulates a GEP on a pointer of the scalar type of storedType.
        Value* CreateConstScalarGEP(Type* storedType, Value* ptr, uint32_t offset)
        {
            IRBuilder<> builder(m_inst);
            if (isa<StoreInst>(m_inst))
            {
                Type* ePtrType = PointerType::get(storedType->getScalarType(), ptr->getType()->getPointerAddressSpace());
                ptr = builder.CreateBitCast(ptr, ePtrType);
                return builder.CreateConstGEP1_32(ptr, offset);
            }
            else
            {
                Value* offsetInBytes = builder.getInt32(offset * storedType->getScalarSizeInBits() / 8);
                return builder.CreateAdd(offsetInBytes, getStoreRaw()->getArgOperand(1));
            }
        }
        static Optional<AbstractStoreInst> get(llvm::Value* value)
        {
            if (StoreInst * SI = dyn_cast<StoreInst>(value))
            {
                return Optional<AbstractStoreInst>(SI);
            }
            else if (StoreRawIntrinsic* SRI = dyn_cast<StoreRawIntrinsic>(value))
            {
                return Optional<AbstractStoreInst>(SRI);
            }
            return Optional<AbstractStoreInst>();
        }
    };

    static bool isAbstractStoreInst(llvm::Value* value)
    {
        GenIntrinsicInst* II = dyn_cast<GenIntrinsicInst>(value);
        return isa<StoreInst>(value) || (II && (II->getIntrinsicID() == GenISAIntrinsic::GenISA_storeraw_indexed ||
            II->getIntrinsicID() == GenISAIntrinsic::GenISA_storerawvector_indexed));
    }

    class VectorPreProcess : public FunctionPass
    {
    public:
        typedef SmallVector<Instruction*, 32> InstWorkVector;
        typedef SmallVector<Value*, 16> ValVector;
        // map from Vector value to its Component Values
        typedef DenseMap<Value*, ValVector> V2SMap;

        enum class VPConst {
            // If a vector's size is bigger than SPLIT_SIZE, split it into multiple
            // of SPLIT_SIZE (plus smaller sub-vectors or scalar if any).
            // With SPLIT_SIZE=32, we have the max vectors as below after this pass:
            //     <32 x i8>, 16xi16, 8xi32, or 4xi64!
            SPLIT_SIZE = 32,              // default, 32 bytes
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
        ConstantInt* CInt = dyn_cast<ConstantInt>(IEI->getOperand(2));
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
        ConstantInt* CInt = dyn_cast<ConstantInt>(EEI->getOperand(1));
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

    // May have phi use, need to check if it's empty.
    if (VI->use_empty())
    {
        VI->eraseFromParent();
    }
}

void VectorPreProcess::createSplitVectorTypes(
    Type* ETy,
    uint32_t NElts,
    uint32_t SplitSize,
    SmallVector<std::pair<Type*, uint32_t>, 8>& SplitInfo)
{
    uint32_t ebytes = (unsigned int)ETy->getPrimitiveSizeInBits() / 8;
    if (ETy->isPointerTy())
    {
        ebytes = m_DL->getPointerTypeSize(ETy);
    }

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

uint32_t VectorPreProcess::getSplitByteSize(Instruction* I, WIAnalysisRunner& WI) const
{
    uint32_t bytes = 0;
    if (LoadInst* LI = dyn_cast<LoadInst>(I))
    {
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
    }
    else if (StoreInst* SI = dyn_cast<StoreInst>(I))
    {
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
    }
    else if (isa<LdRawIntrinsic>(I) || isa<StoreRawIntrinsic>(I))
    {
        bytes = (uint32_t)VPConst::RAW_SPLIT_SIZE;
    }
    else
    {
        bytes = (uint32_t)VPConst::SPLIT_SIZE;
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
    bool isStoreInst = isa<StoreInst>(SI);
    uint32_t splitSize = getSplitByteSize(SI, WI);
    if (IGC_IS_FLAG_ENABLED(EnableSplitUnalignedVector))
    {
        // byte and word-aligned stores can only store a dword at a time.
        unsigned int alignment = ASI.getAlignment();
        if (isStoreInst && alignment < 4)
        {
            uint32_t newAlign = (uint32_t)IGCLLVM::getAlignmentValue(getKnownAlignment(ASI.getPointerOperand(), *m_DL));
            if (newAlign > alignment)
            {
                // For the same reason as Load, use DW-aligned for OCL stateful.
                StoreInst* aSI = dyn_cast<StoreInst>(SI);
                if (aSI && newAlign > 4 && isStatefulAddrSpace(aSI->getPointerAddressSpace()))
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

    ValVector& svals = vecToSubVec[StoredVal];
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
            if (ETy->getPrimitiveSizeInBits() > Ty1->getScalarSizeInBits())
            {
                std::vector<Value*> splitScalars;
                IGC_ASSERT(Ty1->getScalarSizeInBits());
                const uint32_t vectorSize = (unsigned int)ETy->getPrimitiveSizeInBits() / Ty1->getScalarSizeInBits();
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
    uint32_t Align = ASI.getAlignment();
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
            uint32_t vAlign = (uint32_t)MinAlign(Align, (uint32_t)eOffset * EBytes);
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
        unsigned int alignment = ALI.getAlignment();
        if (!isLdRaw && alignment < 4)
        {
            uint32_t newAlign = (uint32_t)IGCLLVM::getAlignmentValue(getKnownAlignment(ALI.getPointerOperand(), *m_DL));
            if (newAlign > alignment)
            {
                //  For OCL stateful, the base can be as little as DW-aligned. To be safe,
                //  need to use DW-aligned. For example,
                //       % 0 = add i32 0, 16
                //       % 4 = inttoptr i32 % 0 to <8 x i16> addrspace(131073) *
                //       %5 = load <8 x i16>, <8 x i16> addrspace(131073) * %4, align 2
                //  newAlign from getKnownAlignment() is 16. But we can only set align to 4 as
                //  the base of this stateful could be just DW-aligned.
                LoadInst* aLI = dyn_cast<LoadInst>(LI);
                if (aLI && newAlign > 4 && isStatefulAddrSpace(aLI->getPointerAddressSpace()))
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

    Value* Addr = ALI.getPointerOperand();
    uint32_t Align = ALI.getAlignment();
    bool IsVolatile = ALI.getIsVolatile();

    uint32_t eOffset = 0;
    uint32_t EBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(ETy));

    // Create a map entry for LI
    ValVector& svals = vecToSubVec[LI];

    for (uint32_t i = 0; i < len; ++i)
    {
        Type* Ty1 = splitInfo[i].first;
        uint32_t len1 = splitInfo[i].second;
        IGCLLVM::FixedVectorType* VTy1 = dyn_cast<IGCLLVM::FixedVectorType>(Ty1);
        for (uint32_t j = 0; j < len1; ++j)
        {
            uint32_t vAlign = (uint32_t)MinAlign(Align, eOffset * EBytes);
            Value* offsetAddr = ALI.CreateConstScalarGEP(Ty1, Addr, eOffset);
            Instruction* I = ALI.Create(Ty1, offsetAddr, vAlign, IsVolatile);
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
        if (svals[0]->getType()->getPrimitiveSizeInBits() < ETy->getPrimitiveSizeInBits())
        {
            const unsigned int denominator = (unsigned int)svals[0]->getType()->getPrimitiveSizeInBits();
            IGC_ASSERT(0 < denominator);

            const uint32_t scalarsPerElement = (unsigned int)ETy->getPrimitiveSizeInBits() / denominator;
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
    Optional<AbstractLoadInst> ALI = AbstractLoadInst::get(Inst);
    Optional<AbstractStoreInst> ASI = AbstractStoreInst::get(Inst);
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

    Value* V = ALI ? ALI->getInst() : ASI->getValueOperand();
    bool isInMap = vecToSubVec.find(V) != vecToSubVec.end();

    // Only LI could be processed already.
    bool processed = ALI && isInMap;
    if (processed)
    {
        return false;
    }

    // Do splitting

    // If it is a store and its stored value is from a load that
    // has not been splitted yet, then splitting the load first
    // so that the stored value will be directly from loaded values
    // without adding insert/extract instructions.
    Optional<AbstractLoadInst> aALI = (ASI && !isInMap) ? AbstractLoadInst::get(V) : ALI;

    if (aALI)
    {
        splitLoad(aALI.getValue(), vecToSubVec, WI);
    }

    if (ASI)
    {
        splitStore(ASI.getValue(), vecToSubVec, WI);
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
    Optional<AbstractLoadInst> optionalALI = AbstractLoadInst::get(Inst);
    AbstractLoadInst* ALI = optionalALI ? optionalALI.getPointer() : nullptr;
    Optional<AbstractStoreInst> optionalASI = AbstractStoreInst::get(Inst);
    AbstractStoreInst* ASI = optionalASI ? optionalASI.getPointer() : nullptr;
    IGC_ASSERT_MESSAGE((optionalALI || optionalASI), "Inst should be either load or store");
    Type* Ty = ALI ? ALI->getInst()->getType() : ASI->getValueOperand()->getType();
    VectorType* VTy = dyn_cast<VectorType>(Ty);
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

    if (etyBytes == 1 || etyBytes == 2 ||
        (etyBytes == 8 && (isa<LdRawIntrinsic>(Inst) || isStoreRaw)))
    {
        IRBuilder<> Builder(Inst);
        if (optionalALI)
        {
            Value* Elt0 = NULL;
            Value* Elt1 = NULL;
            Value* Elt2 = NULL;
            // If alignment is the same as 4-element vector's, it's likely safe
            // to make it 4-element load. (always safe ?)
            if (ALI->getAlignment() >= 4 * etyBytes)
            {
                // Make it 4-element load
                Type* newVTy = FixedVectorType::get(eTy, 4);
                Value* V = ALI->Create(newVTy);

                Elt0 = Builder.CreateExtractElement(V, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V, Builder.getInt32(1), "elt1");
                Elt2 = Builder.CreateExtractElement(V, Builder.getInt32(2), "elt2");
            }
            else
            {
                // One 2-element vector load + one scalar load
                Type* newVTy = FixedVectorType::get(eTy, 2);
                Value* offsetAddr = ALI->CreateConstScalarGEP(eTy, ALI->getPointerOperand(), 2);
                Value* V2 = ALI->Create(newVTy);
                Elt0 = Builder.CreateExtractElement(V2, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V2, Builder.getInt32(1), "elt1");

                uint32_t newAlign = (uint32_t)MinAlign(ALI->getAlignment(), 2 * etyBytes);
                Elt2 = ALI->Create(eTy, offsetAddr, newAlign, ALI->getIsVolatile());
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
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = nullptr;
        }
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
            scalars[ix] = IEI->getOperand(1);
            V = IEI->getOperand(0);
        }
        if (!isa<UndefValue>(V))
        {
            genExtract = true;
        }

        BasicBlock::iterator inst_b;
        if (Instruction * I = dyn_cast<Instruction>(VecVal))
        {
            inst_b = BasicBlock::iterator(I);
            ++inst_b;
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
    if (Optional<AbstractLoadInst> optionalALI = AbstractLoadInst::get(Inst))
    {
        AbstractLoadInst& ALI = optionalALI.getValue();
        if (!Inst->getType()->isVectorTy() || ALI.getAlignment() < 4)
            return Inst;

        unsigned NBits = Inst->getType()->getScalarSizeInBits();
        if (NBits < 32)
            return Inst;

        SmallVector<ExtractElementInst*, 8> ConstEEIUses;
        unsigned MaxIndex = 0;
        for (auto U : Inst->users())
        {
            auto EEI = dyn_cast<ExtractElementInst>(U);
            if (!EEI || !isa<ConstantInt>(EEI->getIndexOperand()))
                return Inst;

            auto CI = cast<ConstantInt>(EEI->getIndexOperand());
            ConstEEIUses.push_back(EEI);
            MaxIndex = std::max(MaxIndex, int_cast<unsigned>(CI->getZExtValue()));
        }

        // All uses are constant EEI.
        IGC_ASSERT_MESSAGE(ConstEEIUses.size() == Inst->getNumUses(), "out of sync");

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

        Type* NewVecTy = FixedVectorType::get(cast<VectorType>(Inst->getType())->getElementType(),
            MaxIndex + 1);
        IRBuilder<> Builder(Inst);
        Instruction* NewLI = ALI.Create(NewVecTy);

        // Loop and replace all uses.
        SmallVector<Value*, 8> NewEEI(MaxIndex + 1, nullptr);
        for (auto EEI : ConstEEIUses)
        {
            auto CI = cast<ConstantInt>(EEI->getIndexOperand());
            unsigned Idx = int_cast<unsigned>(CI->getZExtValue());
            if (NewEEI[Idx] == nullptr)
            {
                NewEEI[Idx] = Builder.CreateExtractElement(NewLI, CI);
            }
            EEI->replaceAllUsesWith(NewEEI[Idx]);
            EEI->eraseFromParent();
        }
        IGC_ASSERT_MESSAGE(Inst->use_empty(), "out of sync");
        Inst->eraseFromParent();
        return NewLI;
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
    Optional<AbstractStoreInst> optionalASI = AbstractStoreInst::get(Inst);
    AbstractStoreInst& ASI = optionalASI.getValue();
    Value* Val = ASI.getValueOperand();
    if (isa<UndefValue>(Val))
    {
        Inst->eraseFromParent();
        return nullptr;
    }

    if (!Val->getType()->isVectorTy() || ASI.getAlignment() < 4)
        return Inst;

    unsigned NBits = Val->getType()->getScalarSizeInBits();
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

bool VectorPreProcess::runOnFunction(Function& F)
{
    bool changed = false;
    m_DL = &F.getParent()->getDataLayout();
    m_C = &F.getContext();
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

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
            [](Value* V) -> bool {
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
            auto* ModMD =
                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

            TranslationTable TT;
            TT.run(F);
            WIAnalysisRunner WI(&F, DT, PDT, MDUtils, m_CGCtx, ModMD, &TT);
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
            Optional<AbstractLoadInst> ALI = AbstractLoadInst::get(V);
            if (!ALI)
            {
                continue;
            }
            Instruction* LI = ALI.getValue().getInst();
            ValVector& svals = vecToSubVec[V];
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
                // Replace LI and erase LI.
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
            else
            {
                LI->eraseFromParent();
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
