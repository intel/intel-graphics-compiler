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

#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/MathExtras.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

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
    class VectorPreProcess : public FunctionPass
    {
    public:
        typedef SmallVector<Instruction*, 32> InstWorkVector;
        typedef SmallVector<Value*, 16> ValVector;
        // map from Vector value to its Component Values
        typedef DenseMap<Value*, ValVector> V2SMap;

        enum {
            // If a vector's size is bigger than VP_SPLIT_SIZE, split it into
            // multiple of VP_SPLIT_SIZE (plus smaller sub-vectors or scalar
            // if any). This means the max elements of a vector after this
            // pass is 32 (<32 x i8>)!
            //
            // VP_SPLIT_SIZE is at least 8 bytes (largest element size) and
            // must be power of 2.
            VP_SPLIT_SIZE=32,       // 32 bytes (must power of 2)
            VP_MAX_VECTOR_SIZE=128  // max vector length
        };

        static char ID; // Pass identification, replacement for typeid
        VectorPreProcess()
          : FunctionPass(ID)
          , m_DL(nullptr)
          , m_C(nullptr)
          , m_WorkList()
          , m_Temps()
        {
            initializeVectorPreProcessPass(*PassRegistry::getPassRegistry());
        }

        virtual bool runOnFunction(Function &F);
        virtual void getAnalysisUsage(AnalysisUsage &AU) const
        {
            AU.setPreservesCFG();
        }

    private:

        void getOrGenScalarValues(
            Function& F, Value* VecVal, Value** scalars, Instruction*& availBeforeInst);
        void replaceAllVectorUsesWithScalars(Instruction *VI,
                                             ValVector& SVals);

        // Return true if V is created by InsertElementInst with const index.
        bool isValueCreatedOnlyByIEI(Value *V, InsertElementInst** IEInsts);
        // Return true if V is only used by ExtractElement with const index.
        bool isValueUsedOnlyByEEI(Value *V, ExtractElementInst** EEInsts);

        // Split load/store that cannot be re-layout or is too big.
        bool splitLoadStore(Instruction *Inst, V2SMap& vecToSubVec);
        bool splitLoad(LoadInst *LI, V2SMap& vecToSubVec);
        bool splitStore(StoreInst *SI, V2SMap& vecToSubVec);
        bool splitVector3LoadStore(Instruction *Inst);
        // Simplify load/store instructions if possible. Return itself if no
        // simplification is performed.
        Instruction* simplifyLoadStore(Instruction *LI);
        void createSplitVectorTypes(
            Type* ETy,
            uint32_t NElts,
            Type** SVTypes,
            uint32_t* SVCounts,
            uint32_t& Len);

     private:
        const DataLayout *m_DL;
        LLVMContext *m_C;
        InstWorkVector m_WorkList;
        ValVector m_Temps;
        InstWorkVector m_Vector3List; // used for keep all 3-element vectors.
    };
}

// Register pass to igc-opt
#define PASS_FLAG "igc-vectorpreprocess"
#define PASS_DESCRIPTION "Split loads/stores of big (or 3-element) vectors into smaller ones."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(VectorPreProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(VectorPreProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char VectorPreProcess::ID = 0;

FunctionPass *IGC::createVectorPreProcessPass()
{
    return new VectorPreProcess();
}

bool VectorPreProcess::isValueCreatedOnlyByIEI(Value *V, InsertElementInst **IEInsts)
{
    Value *ChainVal = V;
    while( !isa<UndefValue>(ChainVal) )
    {
        InsertElementInst *IEI = dyn_cast<InsertElementInst>(ChainVal);
        if( !IEI || !isa<ConstantInt>(IEI->getOperand(2)) )
        {
            return false;
        }
        ConstantInt *CInt = dyn_cast<ConstantInt>(IEI->getOperand(2));
        uint32_t idx = (uint32_t)CInt->getZExtValue();

        // Make sure the last IEI will be recorded if an element is
        // inserted multiple times.
        if( IEInsts[idx] == nullptr )
        {
            IEInsts[idx] = IEI;
        }

        ChainVal = IEI->getOperand(0);
    }
    return true;
}

bool VectorPreProcess::isValueUsedOnlyByEEI(Value *V, ExtractElementInst **EEInsts)
{
    for( Value::user_iterator UI = V->user_begin(), UE = V->user_end();
         UI != UE; ++UI )
    {
        ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(*UI);
        if( !EEI || 
            (EEI->getOperand(0) != V) || 
            !isa<ConstantInt>(EEI->getOperand(1)) )
        {
            return false;
        }
        ConstantInt *CInt = dyn_cast<ConstantInt>(EEI->getOperand(1));
        uint32_t idx = (uint32_t)CInt->getZExtValue();

         // Quit if there are multiple extract from the same index.
        if( EEInsts[idx] != nullptr )
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
void VectorPreProcess::replaceAllVectorUsesWithScalars(Instruction *VI, ValVector& SVals)
{
    SmallVector<Instruction *, 8> ToBeDeleted;
    bool genVec = false;
    for (Value::user_iterator UI = VI->user_begin(), UE = VI->user_end(); UI != UE; ++UI)
    {
        ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(*UI);
        if (!EEI)
        {
            genVec = true;
            continue;
        }
        ConstantInt *CI = dyn_cast<ConstantInt>(EEI->getOperand(1));
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
        Instruction *I;
        if (!isa<PHINode>(VI))
        {
            I = VI;
        }
        else
        {
            I = VI->getParent()->getFirstNonPHI();
        }
        IRBuilder<> Builder(I);
        VectorType *VTy = cast<VectorType>(VI->getType());
        Value *newVec = UndefValue::get(VTy);
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
    Type** SVTypes,
    uint32_t* SVCounts,
    uint32_t& Len)
{
    uint32_t ebytes = ETy->getPrimitiveSizeInBits() / 8;
    if(ETy->isPointerTy())
    {
        ebytes = m_DL->getPointerTypeSize(ETy);
    }
    assert((VP_SPLIT_SIZE % ebytes) == 0 &&
           "Internal Error: Wrong split size!");

    // the number of elements of a new vector
    uint32_t E = VP_SPLIT_SIZE / ebytes;
    // number of vectors
    uint32_t N = NElts / E;
    // remaining number of elements.
    uint32_t R = NElts % E;

    int j = 0;
    if (N > 0)
    {
        SVCounts[0] = N;
        SVTypes[0] = VectorType::get(ETy, E);
        ++j;
    }

    // Sub-vectors are 
    //   1. ebytes >=4, the remaing is a single sub-vector; or
    //   2. ebytes < 4, the remaining is splitted into
    //        one sub-vector of multiple 4xebytes, and
    //        the remaining vector of 3|2|1 elements.
    //
    //        Note that we keep vector 3 here so that we may convert
    //        vector3 to vector4 later when special-handling vector3.
    if (ebytes < 4 && R > 0)
    {
        N = R / 4;
        R = R % 4;
        if (N > 0)
        {
            SVCounts[j] = 1;
            SVTypes[j] = VectorType::get(ETy, 4 * N);
            ++j;
        }
    }

    // remaining sub-vector
    if (R > 0)
    {
        SVCounts[j] = 1;
        SVTypes[j] = (R == 1) ? ETy : VectorType::get(ETy, R);
        ++j;
    }
    Len = j;
 }

bool VectorPreProcess::splitStore(StoreInst *SI, V2SMap& vecToSubVec)
{
    Value *StoredVal = SI->getValueOperand();
    VectorType *VTy = cast<VectorType>(StoredVal->getType());
    Type *ETy = VTy->getElementType();
    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());

    assert(nelts <= VP_MAX_VECTOR_SIZE && "Vector length is too big!");

    Type *tys[6];
    uint32_t tycnts[6];
    uint32_t len;
    // Generate splitted loads and save them in the map
    createSplitVectorTypes(ETy, nelts, tys, tycnts, len);

    // return if no split
    if (len == 1 && tycnts[0] == 1)
    {
        return false;
    }

    ValVector& svals = vecToSubVec[StoredVal];
    if (svals.size() == 0)
    {
        // Need to create splitted values.
        Instruction *insertBeforeInst;
        Value* scalars[VP_MAX_VECTOR_SIZE];
        getOrGenScalarValues(*SI->getParent()->getParent(),
                             StoredVal, scalars, insertBeforeInst);
        insertBeforeInst = insertBeforeInst ? insertBeforeInst : SI;
        IRBuilder<> aBuilder(insertBeforeInst);
        // Now generate svals
        for (uint32_t i = 0, Idx = 0; i < len; ++i)
        {
            VectorType *VTy1 = dyn_cast<VectorType>(tys[i]);
            for (uint32_t j = 0; j < tycnts[i]; ++j)
            {
                Value *subVec;
                if (!VTy1)
                {
                    subVec = scalars[Idx];
                    ++Idx;
                }
                else
                {
                    subVec = UndefValue::get(tys[i]);
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

    IRBuilder<> Builder(SI);
    Value *Addr = SI->getPointerOperand();
    uint32_t Align = SI->getAlignment();
    bool IsVolatile = SI->isVolatile();
    const uint32_t AS = SI->getPointerAddressSpace();
    PointerType* ePTy = PointerType::get(ETy, AS);
    Value* ePtr = Builder.CreateBitCast(Addr, ePTy, "split");
    uint32_t eOffset = 0;
    uint32_t EBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(ETy));
    
    for (uint32_t i = 0, subIdx = 0; i < len; ++i)
    {
        Value *sP =
            (eOffset == 0) ? ePtr : Builder.CreateConstGEP1_32(ePtr, eOffset);
        PointerType* PTy = PointerType::get(tys[i], AS);
        Value* vP = Builder.CreateBitCast(sP, PTy, "split");
        VectorType *VTy1 = dyn_cast<VectorType>(tys[i]);
        for (uint32_t j = 0; j < tycnts[i]; ++j)
        {
            Value *vP1 = (j == 0) ? vP : Builder.CreateConstGEP1_32(vP, j);
            uint32_t vAlign = (uint32_t)MinAlign(Align, eOffset * EBytes);
            StoreInst *newST = 
                Builder.CreateAlignedStore(svals[subIdx], vP1, vAlign, IsVolatile);
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
    Instruction *inst = dyn_cast<Instruction>(StoredVal);
    bool keepLI = inst && isa<LoadInst>(inst) &&
                  (vecToSubVec.find(inst) != vecToSubVec.end());
    while (inst && !keepLI && inst->use_empty())
    {
        Instruction *next = nullptr;
        if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(inst))
        {
            next = dyn_cast<Instruction>(IEI->getOperand(0));
        }

        inst->eraseFromParent();
        inst = next;
        keepLI = inst && isa<LoadInst>(inst) &&
                 (vecToSubVec.find(inst) != vecToSubVec.end());
    }
    return true;
}

bool VectorPreProcess::splitLoad(LoadInst *LI, V2SMap& vecToSubVec)
{
    VectorType *VTy = cast<VectorType>(LI->getType());
    Type *ETy = VTy->getElementType();
    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());

    Type *tys[6];
    uint32_t tycnts[6];
    uint32_t len;
    // Generate splitted loads and save them in the map
    createSplitVectorTypes(ETy, nelts, tys, tycnts, len);

    // return if no split
    if (len == 1 && tycnts[0] == 1)
    {
        return false;
    }

    IRBuilder<> Builder(LI);
    Value *Addr = LI->getPointerOperand();
    uint32_t Align = LI->getAlignment();
    bool IsVolatile = LI->isVolatile();
    const uint32_t AS = LI->getPointerAddressSpace();

    PointerType* ePTy = PointerType::get(ETy, AS);
    Value* ePtr = Builder.CreateBitCast(Addr, ePTy, "split");
    uint32_t eOffset = 0;
    uint32_t EBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(ETy));

    // Create a map entry for LI
    ValVector& svals = vecToSubVec[LI];

    for (uint32_t i = 0; i < len; ++i)
    {
        Value *sP = (eOffset == 0) ? ePtr : Builder.CreateConstGEP1_32(ePtr, eOffset);
        PointerType* PTy = PointerType::get(tys[i], AS);
        VectorType *VTy1 = dyn_cast<VectorType>(tys[i]);
        Value* vP = Builder.CreateBitCast(sP, PTy, "split");
        for (uint32_t j = 0; j < tycnts[i]; ++j)
        {
            Value *vP1 = (j == 0) ? vP : Builder.CreateConstGEP1_32(vP, j);
            uint32_t vAlign = (uint32_t)MinAlign(Align, eOffset * EBytes);
            LoadInst *L = Builder.CreateAlignedLoad(vP1, vAlign, IsVolatile);
            
            eOffset += (VTy1 ? int_cast<uint32_t>(VTy1->getNumElements()) : 1);

            svals.push_back(L);

            // If this is a new 3-element vector, add it into m_Vector3List
            if (VTy1 && VTy1->getNumElements() == 3)
            {
                m_Vector3List.push_back(L);
            }
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

bool VectorPreProcess::splitLoadStore(Instruction *Inst, V2SMap& vecToSubVec)
{
    LoadInst  *LI = dyn_cast<LoadInst>(Inst);
    StoreInst *SI = dyn_cast<StoreInst>(Inst);
    assert((LI || SI) && "Inst should be either load or store");
    Type *Ty = LI ? LI->getType() : SI->getValueOperand()->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
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

    Value *V = LI ? LI : SI->getValueOperand();
    bool isInMap = vecToSubVec.find(V) != vecToSubVec.end();

    // Only LI could be processed already.
    bool processed = LI && isInMap;
    if (processed)
    {
        return false;
    }

    // Do splitting

    // If it is a store and its stored value is from a load that
    // has not been splitted yet, then splitting the load first
    // so that the stored value will be directly from loaded values
    // without adding insert/extract instructions.
    LoadInst *aLI = LI;
    if (SI && !isInMap)
    {
        aLI = dyn_cast<LoadInst>(V);
    }

    if (aLI)
    {
        splitLoad(aLI, vecToSubVec);
    }

    if (SI)
    {
        splitStore(SI, vecToSubVec);
    }
    return true;
}

// For a vector3 whose element size < 4 bytes, will split them into one whose
// size is multiple of DW and one whose size is less than DW; If the size is
// less than DW, make sure it is either 1 Byte or 2 bytes.  After this, for
// vector size < 4, it must be either 1 byte or 2 bytes, never 3  bytes.
bool VectorPreProcess::splitVector3LoadStore(Instruction *Inst)
{
    LoadInst  *LI = dyn_cast<LoadInst>(Inst);
    StoreInst *SI = dyn_cast<StoreInst>(Inst);
    assert((LI || SI) && "Inst should be either load or store");
    Type *Ty = LI ? LI->getType() : SI->getValueOperand()->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
    assert(VTy && VTy->getNumElements() == 3 &&
        "Inst should be a 3-element vector load/store!");

    Type *eTy = VTy->getElementType();
    uint32_t etyBytes = int_cast<unsigned int>(m_DL->getTypeAllocSize(eTy));
    // total size of vector in bytes;
    //uint32_t sz = VTy->getNumElements() * etyBytes;

    Value *Ptr = LI ? LI->getPointerOperand() : SI->getPointerOperand();
    PointerType  *PtrTy = cast<PointerType>(Ptr->getType());

    if (etyBytes == 1 || etyBytes == 2)
    {
        IRBuilder<> Builder(Inst);

        // ptr type to point to the element.
        Type *ePtrTy = PointerType::get(eTy, PtrTy->getPointerAddressSpace());
        if (LI)
        {
            Value *Elt0 = NULL;
            Value *Elt1 = NULL;
            Value *Elt2 = NULL;
            // If alignment is the same as 4-element vector's, it's likely safe
            // to make it 4-element load. (always safe ?)
            if (LI->getAlignment() >= 4 * etyBytes)
            {
                // Make it 4-element load
                Type *newVTy = VectorType::get(eTy, 4);
                Type *newTy = PointerType::get(newVTy, PtrTy->getPointerAddressSpace());
                Value *newPtr = Builder.CreateBitCast(Ptr, newTy, "v3tov4");
                Value *V = Builder.CreateAlignedLoad(newPtr, LI->getAlignment(),
                    LI->isVolatile(), "v4ld");

                Elt0 = Builder.CreateExtractElement(V, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V, Builder.getInt32(1), "elt1");
                Elt2 = Builder.CreateExtractElement(V, Builder.getInt32(2), "elt2");
            }
            else
            {
                // One 2-element vector load + one scalar load
                Type *newVTy = VectorType::get(eTy, 2);
                Type *newTy = PointerType::get(newVTy, PtrTy->getPointerAddressSpace());
                Value *newVPtr = Builder.CreateBitCast(Ptr, newTy, "v3tov2");
                Value *newSPtr = Builder.CreateBitCast(Ptr, ePtrTy, "ePtr");
                newSPtr = Builder.CreateConstGEP1_32(newSPtr, 2);

                Value *V2 = Builder.CreateAlignedLoad(newVPtr, LI->getAlignment(),
                    LI->isVolatile(), "v2ld");
                Elt0 = Builder.CreateExtractElement(V2, Builder.getInt32(0), "elt0");
                Elt1 = Builder.CreateExtractElement(V2, Builder.getInt32(1), "elt1");

                uint32_t newAlign = (uint32_t)MinAlign(LI->getAlignment(), 2 * etyBytes);
                Elt2 = Builder.CreateAlignedLoad(newSPtr, newAlign,
                    LI->isVolatile(), "s1ld");
            }

            // A little optimization here 
            ExtractElementInst *EEInsts[3];
            for (int i = 0; i < 3; ++i)
            {
                EEInsts[i] = nullptr;
            }
            if (isValueUsedOnlyByEEI(LI, EEInsts))
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
                Value *V = Builder.CreateInsertElement(UndefValue::get(VTy), Elt0,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(V, Elt1, Builder.getInt32(1));
                V = Builder.CreateInsertElement(V, Elt2, Builder.getInt32(2));
                LI->replaceAllUsesWith(V);
            }
            LI->eraseFromParent();
        }
        else
        {   // Split 3-element into 2-element + 1 scalar
            Type *newVTy = VectorType::get(eTy, 2);
            Type *newTy = PointerType::get(newVTy, PtrTy->getPointerAddressSpace());
            Value *newVPtr = Builder.CreateBitCast(Ptr, newTy, "v3tov2");
            Value *newSPtr = Builder.CreateBitCast(Ptr, ePtrTy, "ePtr");
            newSPtr = Builder.CreateConstGEP1_32(newSPtr, 2);
            Value *StoredVal = SI->getValueOperand();
            InsertElementInst *IEInsts[3];
            for (int i = 0; i < 3; ++i)
            {
                IEInsts[i] = nullptr;
            }

            // vec3 = vec2 + scalar,  newAlign is an alignment for scalar store.
            uint32_t newAlign = (uint32_t)MinAlign(SI->getAlignment(), 2 * etyBytes);
            Value *UDVal = UndefValue::get(eTy);
            if (isValueCreatedOnlyByIEI(SI, IEInsts))
            {
                // This case should be most frequent, and want
                // to generate a better code by removing dead
                // InsertElementInst.

                // Be ware of partial vector store.
                Value *V = UndefValue::get(newTy);
                V = Builder.CreateInsertElement(
                    V, (IEInsts[0] != nullptr) ? IEInsts[0]->getOperand(1) : UDVal,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(
                    V, (IEInsts[1] != nullptr) ? IEInsts[1]->getOperand(1) : UDVal,
                    Builder.getInt32(1));
                V = Builder.CreateAlignedStore(V, newVPtr,
                    SI->getAlignment(),
                    SI->isVolatile());

                // If IEInsts[2] is undefined, skip scalar store.
                if (IEInsts[2] != nullptr)
                {
                    (void) Builder.CreateAlignedStore(IEInsts[2]->getOperand(1),
                        newSPtr,
                        newAlign,
                        SI->isVolatile());
                }
                SI->eraseFromParent();

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
                Value *Elt0 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(0),
                    "Elt0");
                Value *Elt1 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(1),
                    "Elt1");
                Value *Elt2 = Builder.CreateExtractElement(StoredVal,
                    Builder.getInt32(2),
                    "Elt2");
                Value *V = Builder.CreateInsertElement(UndefValue::get(newVTy),
                    Elt0,
                    Builder.getInt32(0));
                V = Builder.CreateInsertElement(V, Elt1, Builder.getInt32(1));
                Builder.CreateAlignedStore(V, newVPtr,
                    SI->getAlignment(),
                    SI->isVolatile());
                Builder.CreateAlignedStore(Elt2, newSPtr,
                    newAlign,
                    SI->isVolatile());
                SI->eraseFromParent();
            }
        }
        return true;
    }
    return false;
}

// availBeforeInst:
//    Used to indicate that all scalar values of VecVal are available right
//    before the instruction pointed to availBeforeInst.
//    If availBeforeInst is null, it means all scalar values are constants.
void VectorPreProcess::getOrGenScalarValues(
    Function& F, Value* VecVal, Value** scalars, Instruction*& availBeforeInst)
{
    availBeforeInst = nullptr;

    VectorType *VTy = cast<VectorType>(VecVal->getType());
    if (!VTy)
    {
        scalars[0] = VecVal;
        return;
    }

    uint32_t nelts = int_cast<uint32_t>(VTy->getNumElements());
    Type *ETy = VTy->getElementType();
    if (isa<UndefValue>(VecVal))
    {
        Value *udv = UndefValue::get(ETy);
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = udv;
        }       
    }
    else if (ConstantVector* CV = dyn_cast<ConstantVector>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CV->getOperand(i);
        }
    }
    else if (ConstantDataVector* CDV = dyn_cast<ConstantDataVector>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CDV->getElementAsConstant(i);
        }
    }
    else if (ConstantAggregateZero* CAZ = dyn_cast<ConstantAggregateZero>(VecVal))
    {
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = CAZ->getSequentialElement();
        }
    }
    else
    {
        bool genExtract = false;
        Value *V = VecVal;
        for (uint32_t i = 0; i < nelts; ++i)
        {
            scalars[i] = nullptr;
        }
        while (InsertElementInst *IEI = dyn_cast<InsertElementInst>(V))
        {
            Value *ixVal = IEI->getOperand(2);
            ConstantInt *CI = dyn_cast<ConstantInt>(ixVal);
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
        if (Instruction *I = dyn_cast<Instruction>(VecVal))
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
                Value *S;
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
Instruction *VectorPreProcess::simplifyLoadStore(Instruction *Inst)
{
    if (LoadInst *LI = dyn_cast<LoadInst>(Inst))
    {
        if (!LI->getType()->isVectorTy() || LI->getAlignment() < 4)
            return Inst;

        unsigned NBits = LI->getType()->getScalarSizeInBits();
        if (NBits < 32)
            return Inst;

        SmallVector<ExtractElementInst *, 8> ConstEEIUses;
        unsigned MaxIndex = 0;
        for (auto U : LI->users())
        {
            auto EEI = dyn_cast<ExtractElementInst>(U);
            if (!EEI || !isa<ConstantInt>(EEI->getIndexOperand()))
                return Inst;

            auto CI = cast<ConstantInt>(EEI->getIndexOperand());
            ConstEEIUses.push_back(EEI);
            MaxIndex = std::max(MaxIndex, int_cast<unsigned>(CI->getZExtValue()));
        }

        // All uses are constant EEI.
        assert(ConstEEIUses.size() == LI->getNumUses() && "out of sync");

        // FIXME: this is to WA an issue that splitLoadStore does not split
        // vectors of size 5, 6, 7.
        if (MaxIndex + 1 > 4)
            return Inst;

        // If MaxIndex is smaller than <vector_size - 1>, then narrow the size
        // of this vector load to reduce unnecessary memory load.
        //
        // TODO: further optimize this load into a message with channel masks
        // for cases in which use indices are sparse like {0, 2}.
        unsigned N = LI->getType()->getVectorNumElements();
        if (N == MaxIndex + 1)
            return Inst;

        Value *BasePtr = LI->getPointerOperand();
        Type *NewVecTy = VectorType::get(LI->getType()->getVectorElementType(),
                                         MaxIndex + 1);
        Type *NewBaseTy = PointerType::get(
            NewVecTy, BasePtr->getType()->getPointerAddressSpace());

        IRBuilder<> Builder(LI);
        Value *NewBasePtr = Builder.CreateBitCast(BasePtr, NewBaseTy);
        auto NewLI = Builder.CreateAlignedLoad(NewBasePtr, LI->getAlignment(),
                                               LI->isVolatile());

        // Loop and replace all uses.
        SmallVector<Value *, 8> NewEEI(MaxIndex + 1, nullptr);
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
        assert(LI->use_empty() && "out of sync");
        LI->eraseFromParent();
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
    assert(isa<StoreInst>(Inst));
    StoreInst *SI = cast<StoreInst>(Inst);
    Value *Val = SI->getValueOperand();
    if (isa<UndefValue>(Val))
    {
        Inst->eraseFromParent();
        return nullptr;
    }

    if (!Val->getType()->isVectorTy() || SI->getAlignment() < 4)
        return Inst;

    unsigned NBits = Val->getType()->getScalarSizeInBits();
    if (NBits < 32)
        return Inst;

    unsigned N = Val->getType()->getVectorNumElements();
    if (auto CV = dyn_cast<ConstantVector>(Val))
    {
        unsigned MaxIndex = 0;
        for (unsigned i = N - 1; i != 0; --i)
        {
            Constant *Item = CV->getAggregateElement(i);
            if (!isa<UndefValue>(Item))
            {
                MaxIndex = i;
                break;
            }
        }

        if (MaxIndex + 1 == N)
            return Inst;

        SmallVector<Constant *, 8> Data(MaxIndex + 1, nullptr);
        for (unsigned i = 0; i <= MaxIndex; ++i)
        {
            Data[i] = CV->getAggregateElement(i);
        }
        auto SVal = ConstantVector::get(Data);

        IRBuilder<> Builder(SI);
        Value *BasePtr = SI->getPointerOperand();
        Type *NewBaseTy = PointerType::get(
            SVal->getType(), BasePtr->getType()->getPointerAddressSpace());
        Value *NewBasePtr = Builder.CreateBitCast(BasePtr, NewBaseTy);
        auto NewSI = Builder.CreateAlignedStore(
            SVal, NewBasePtr, SI->getAlignment(), SI->isVolatile());
        SI->eraseFromParent();
        return NewSI;
    }

    SmallVector<InsertElementInst *, 8> ConstIEIs(N, nullptr);
    Value *ChainVal = Val;
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
        IRBuilder<> Builder(SI);
        Value *BasePtr = SI->getPointerOperand();
        Type *NewVecTy = VectorType::get(Val->getType()->getVectorElementType(),
                                         MaxIndex + 1);
        Type *NewBaseTy = PointerType::get(
            NewVecTy, BasePtr->getType()->getPointerAddressSpace());
        Value *NewBasePtr = Builder.CreateBitCast(BasePtr, NewBaseTy);
        Value *SVal = UndefValue::get(NewVecTy);
        for (int i = 0; i <= MaxIndex; ++i)
        {
            if (ConstIEIs[i] != nullptr)
            {
                SVal = Builder.CreateInsertElement(SVal,
                                                   ConstIEIs[i]->getOperand(1),
                                                   ConstIEIs[i]->getOperand(2));
            }
        }
        auto NewSI = Builder.CreateAlignedStore(
            SVal, NewBasePtr, SI->getAlignment(), SI->isVolatile());
        SI->eraseFromParent();
        return NewSI;
    }

    return Inst;
}

bool VectorPreProcess::runOnFunction(Function& F)
{
    bool changed = false;
    m_DL = &F.getParent()->getDataLayout();
    m_C  = &F.getContext();

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    {
        Instruction* inst = &*I;
        if (isa<LoadInst>(inst) || isa<StoreInst>(inst))
        {
            m_WorkList.push_back(inst);
        }
    }

    // Simplify loads/stores.
    bool Simplified = false;
    for (unsigned i = 0, n = m_WorkList.size(); i < n; ++i)
    {
        Instruction *Inst = m_WorkList[i];
        Instruction *NewInst = simplifyLoadStore(Inst);
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
                       [](Value *V) -> bool {
			return !V || (!isa<LoadInst>(V) && !isa<StoreInst>(V));
        });
		m_WorkList.erase(new_end, m_WorkList.end());
    }

    // Split vectors
    if (m_WorkList.size() > 0)
    {
        V2SMap vecToSubVec;

        // m_Temps is used to keep loads that needs post-processing.
        m_Temps.clear();

        for (uint32_t i = 0; i < m_WorkList.size(); ++i)
        {
            if (splitLoadStore(m_WorkList[i], vecToSubVec))
            {
                changed = true;
            }
        }

        // Now, do post-processing for the splitted loads
        for (uint32_t i = 0; i < m_Temps.size(); ++i)
        {
            Value *V = m_Temps[i];
            LoadInst *LI = dyn_cast<LoadInst>(V);
            if (!LI)
            {
                continue;
            }
            ValVector& svals = vecToSubVec[V];
            if (!LI->use_empty())
            {
                ValVector Scalars;
                IRBuilder<> Builder(LI);
                for (uint32_t j = 0; j < svals.size(); ++j)
                {
                    Type *Ty1 = svals[j]->getType();
                    VectorType *VTy1 = dyn_cast<VectorType>(Ty1);
                    if (VTy1) {
                        for (uint32_t k = 0; k < VTy1->getNumElements(); ++k)
                        {
                            Value *S = Builder.CreateExtractElement(
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
                        Instruction *tInst = cast<Instruction>(Scalars[j]);
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
                Instruction *tInst = cast<Instruction>(svals[j]);
                if (tInst->use_empty())
                {
                    // If this is a 3-element vector load, remove it
                    // from m_Vector3List as well.
                    if (isa<LoadInst>(tInst) && tInst->getType()->isVectorTy() &&
                        tInst->getType()->getVectorNumElements() == 3)
                    {
                        InstWorkVector::iterator
                            tI = m_Vector3List.begin(),
                            tE = m_Vector3List.end();
                        for (; tI != tE; ++tI)
                        {
                            Instruction *tmp = *tI;
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
