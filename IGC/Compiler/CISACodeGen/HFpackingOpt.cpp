/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PatternMatch.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/HFpackingOpt.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

namespace {

    enum class HFdataType : unsigned char
    {
        HF_STORE,
        HF_LOAD,
        HF_END,
    };

    struct HFdata
    {
        union
        {
            StoreInst* Store;
            LoadInst* Load;
        };
        HFdataType type;
        GetElementPtrInst* GEP;
        uint arraySize;
        uint elmBytes;
        uint offset;
        bool bStoreConstZero;
        uint storePattern;

        HFdata(HFdataType HFtype): type(HFtype) { clear(); }
        void clear()
        {
            GEP = nullptr;
            arraySize = 0;
            elmBytes = 0;
            offset = 0;
            bStoreConstZero = false;
            storePattern = 0;
            if (type == HFdataType::HF_STORE)
                Store = nullptr;
            else
                Load = nullptr;
        }
        bool isValid() { return (GEP && arraySize); }
    };

    class HFpackingOpt : public FunctionPass
    {
    public:
        static char ID;
        HFpackingOpt();

        StringRef getPassName() const override { return "HFpackingOpt"; }

        bool runOnFunction(Function& F) override;
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }
    private:
        CodeGenContext* pContext = nullptr;
        ComputeShaderContext* csCtx = nullptr;
        ModuleMetaData* modMD = nullptr;
        std::map<uint, uint> slmOffsetMap;
        std::map<uint, uint> slmComponentsMap;
        std::map<uint, bool> HFList; // GEPoffset, HF flag
        std::map<uint32_t, uint32_t> HFpackedOffsets; // merged pair: GEP offset #1, GEP offset #2
        std::map<GetElementPtrInst*, Value *> HFpackedGEPs; // mapped old GEP to new GEP
        bool bRemoveFromSlmMap = false;
        bool bRemoveSlmGap = false;

        bool findSRVinfo(GenIntrinsicInst* tex, uint& runtimeV, uint& ptrAddrSpace);
        bool getSRVMap(GenIntrinsicInst* tex, uint& index);
        bool supportHF(ExtractElementInst* EE, const uint gepOffset);
        bool getGEPInfo(GetElementPtrInst* GEP, uint& arraySize, uint& elmBytes, uint& offset);
        ExtractElementInst* decodeStore(StoreInst* pStore, bool& bStoreConstZero, uint &storePattern);
        void getHFPackingCandidate(Function& F);
        void PackHfResources(Function& F);
        void removeRedundantChannels(Function& F);
        bool allowedALUinst(Value* inst);
        bool findStoreSequence(std::vector<Instruction*>& path, std::vector< std::vector<Instruction*>>& allPath);
        bool adjGep(Value* gep0, Value* gep1, Value* gep2, uint32_t offset[3]);
        bool initializeSlmOffsetMap(Function& F);
        void adjOffsetMap(uint32_t offset, uint32_t minusSize, const bool eraseGivenOffset);
        void removeFromSlmMap(GetElementPtrInst* gep);
        void updateSlmOffsetSize(Function& F);
        void removeSlmGap(Function& F);
        uint getGEPcomponent(GetElementPtrInst* GEP);
        bool canMerge(HFdata& prev, GetElementPtrInst* GEP,
            const uint arraySize, const uint elmBytes, const uint offset,
            const uint storePattern, const bool bStoreConstZero, bool &bRemoveSLM);
        void replaceStores(Instruction* startInst, Value* newGEP,
            StoreInst* Store1, StoreInst* Store2,
            const bool bStoreConstZero1, const bool bStoreConstZero2);
        void replaceLoads(Instruction* startInst, Value* newGEP,
            LoadInst* load1, LoadInst* load2);
        bool isPacked(uint offset1, uint offset2);
    };
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-HFpacking"
#define PASS_DESCRIPTION "HF packing Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HFpackingOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HFpackingOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HFpackingOpt::ID = 0;
FunctionPass* IGC::createHFpackingOptPass()
{
    return new HFpackingOpt();
}

HFpackingOpt::HFpackingOpt() : FunctionPass(ID)
{
    initializeHFpackingOptPass(*PassRegistry::getPassRegistry());
    bRemoveFromSlmMap = false;
    bRemoveSlmGap = false;
}

// return runtimeV and ptrAddrSpace for the given GenISA_ldptr
// %27 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p2621447__2D_DIM_Resource.p2621447__2D_DIM_Resource(
//       i32 %.i0568, i32 %.i1569, i32 0, i32 0, %__2D_DIM_Resource addrspace(2621447)* undef,
//       %__2D_DIM_Resource addrspace(2621447)* %t7, i32 0, i32 0, i32 0)
// ptrAddrSpace = 2621447
//
// %4 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 15)
// %t7 = inttoptr i32 % 4 to % __2D_DIM_Resource addrspace(2621447) *
// runtimeV = 15
bool HFpackingOpt::findSRVinfo(GenIntrinsicInst* tex, uint& runtimeV, uint& ptrAddrSpace)
{
    if (!tex)
        return false;

    if (tex->getIntrinsicID() != GenISAIntrinsic::GenISA_ldptr)
        return false;

    ptrAddrSpace = cast<SamplerLoadIntrinsic>(tex)->getTextureValue()->getType()->getPointerAddressSpace();

    Value* src = IGC::TracePointerSource(cast<SamplerLoadIntrinsic>(tex)->getTextureValue());
    if (GetGRFOffsetFromRTV(src, runtimeV))
    {
        return true;
    }
    return false;
}

// lookup SRV mapping entry from GenISA_ldptr intrinsic
// return the 'index' of the found entry
bool HFpackingOpt::getSRVMap(GenIntrinsicInst* texld, uint& index)
{
    uint runtimeV = 0;
    uint ptrAddrSpace = 0;

    if (!texld)
        return false;

    if (findSRVinfo(texld, runtimeV, ptrAddrSpace))
    {
        uint idx = 0;
        for (auto& iter : modMD->SrvMap)
        {
            if (iter.runtimeValue == runtimeV && iter.ptrAddressSpace == ptrAddrSpace)
            {
                index = idx;
                return true;
            }
            idx++;
        }
    }

    return false;
}

void HFpackingOpt::adjOffsetMap(uint32_t offset, uint32_t minusSize, const bool eraseGivenOffset)
{
    auto slmOffsetIter = slmOffsetMap.begin();
    while (slmOffsetIter != slmOffsetMap.end())
    {
        // already removed
        if ( !(slmOffsetIter->second == -1)  &&
            // find the offset to be removed
            (slmOffsetIter->first == offset) )
        {
            bRemoveFromSlmMap = true;
            if (eraseGivenOffset)
            {
                slmOffsetMap[slmOffsetIter->first] = -1;
            }
            csCtx->m_tgsmSize -= minusSize;

            // update the higher offsets
            slmOffsetIter++;
            while (slmOffsetIter != slmOffsetMap.end())
            {
                if (slmOffsetIter->second != -1)
                {
                    IGC_ASSERT(slmOffsetMap[slmOffsetIter->first] >= minusSize);
                    slmOffsetMap[slmOffsetIter->first] -= minusSize;
                }
                slmOffsetIter++;
            }
            break;
        }
        slmOffsetIter++;
    }
}

void HFpackingOpt::removeFromSlmMap(GetElementPtrInst* gep)
{
    uint32_t arraySize = 0, elmBytes = 0, offset = 0;
    getGEPInfo(gep, arraySize, elmBytes, offset);
    adjOffsetMap(offset, arraySize * elmBytes, true);
}

// detect if the given resource supports half float
bool HFpackingOpt::supportHF(ExtractElementInst* EE, const uint gepOffset)
{
    uint index = 0;

    // check cached HFList first
    std::map<uint, bool>::iterator it = HFList.find(gepOffset);
    if (it != HFList.end())
        return it->second;

    if (!EE)
    {
        return false;
    }

    if (!getSRVMap(dyn_cast<GenIntrinsicInst>(EE->getOperand(0)), index))
        return false;

    std::vector<unsigned int> HFRes;
    bool isHF = false;
    // %DepthNormalRoughnessInputTexture_texture_2d = call %dx.types.Handle @dx.op.createHandle(
    //                                                i32 57, i8 0, i32 5, i32 7, i1 false)
    // resourceRangeID = 5
    // indexIntoRange = 7
    for (size_t i = 0; i < pContext->getModuleMetaData()->csInfo.ResForHfPacking.size(); i++)
    {
        HFRes = pContext->getModuleMetaData()->csInfo.ResForHfPacking[i];
        if (modMD->SrvMap[index].resourceRangeID == HFRes[0] &&
            modMD->SrvMap[index].indexIntoRange == HFRes[1])
        {
            isHF = modMD->SrvMap[index].hfCandidate;
            break;
        }
    }

    HFList[gepOffset] = isHF;
    return isHF;
}

// extract offset, arraySize, and elementSize in bytes from GEP
bool HFpackingOpt::getGEPInfo(GetElementPtrInst* GEP, uint& arraySize, uint& elmBytes, uint& offset)
{
    if (!GEP)
        return false;

    // getelementptr[144 x float], [144 x float] addrspace(3) * null, i32 0, i32 % 885
    if (isa<ConstantPointerNull>(GEP->getPointerOperand()))
    {
        offset = 0;
        Type* type = GEP->getSourceElementType();
        if (type->isArrayTy())
        {
            arraySize = (uint)type->getArrayNumElements();
            elmBytes = type->getArrayElementType()->getScalarSizeInBits() / 8;
            return true;
        }
        else
            return false;
    }

    // [144 x float] addrspace(3)* inttoptr (i32 4608 to [144 x float] addrspace(3)*)
    // offset    : 4608
    // arraySize : 144
    // elmBytes  : 4 (size of float)
    if (const ConstantExpr* CE = dyn_cast<ConstantExpr>(GEP->getPointerOperand()))
    {
        const ConstantInt* CI = dyn_cast<ConstantInt>(CE->getOperand(0));

        if (CI && CI->getType()->isIntegerTy())
        {
            offset = (uint)CI->getZExtValue();

            Type* type = GEP->getSourceElementType();
            if (type->isArrayTy())
            {
                arraySize = (uint)type->getArrayNumElements();
                elmBytes = type->getArrayElementType()->getScalarSizeInBits() / 8;
                return true;
            }
            else
                return false;
        }
    }
    return false;
}

void HFpackingOpt::getHFPackingCandidate(Function& F)
{
    for (auto bb = F.begin(); bb != F.end(); ++bb)
    {
        for (auto ii = bb->begin(); ii != bb->end(); ++ii)
        {
            if (StoreInst* pStore = llvm::dyn_cast<StoreInst>(&(*ii)))
            {
                if (pStore->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
                {
                    continue;
                }

                if (ExtractElementInst* ee = dyn_cast<ExtractElementInst>(pStore->getOperand(0)))
                {
                    uint index = 0;
                    if (getSRVMap(dyn_cast<GenIntrinsicInst>(ee->getOperand(0)), index))
                    {
                        // found immediate usage of texture load to local store. Set the hfCandidate to true
                        // UMD will check if the resource is hf and can be packed.
                        modMD->SrvMap[index].hfCandidate = true;
                    }
                }
            }
        }
    }
}

/***** Scenario 1 *****
    %29 = extractelement <4 x float> %27, i32 1
    store float %29, float addrspace(3)* %37, align 4

 ***** Scenario 2 *****
    %50 = extractelement <4 x float> %49, i32 0
    %53 = call fast float @llvm.maxnum.f32(float %50, float 0.000000e+00)
    store float %53, float addrspace(3)* %56, align 4

 ***** Scenario 3 *****
    store float 0.000000e+00, float addrspace(3)* %122, align 4
*/
ExtractElementInst* HFpackingOpt::decodeStore(StoreInst* pStore, bool& bStoreConstZero, uint& storePattern)
{
    bStoreConstZero = false;
    storePattern = 0;

    if (!pStore->getOperand(0)->getType()->isFloatTy())
        return nullptr;

    // scenario 1 - EE is valid here
    ExtractElementInst* EE = dyn_cast<ExtractElementInst>(pStore->getOperand(0));

    if (!EE)
    {
        if (Instruction* inst = dyn_cast<Instruction>(pStore->getOperand(0)))
        {
            // scenario 2
            EE = dyn_cast<ExtractElementInst>(inst->getOperand(0));
            storePattern = 2;
        }
        else if (ConstantFP* fp = dyn_cast<ConstantFP>(pStore->getOperand(0)))
        {
            // scenario 3
            if (fp->isExactlyValue(0))
            {
                bStoreConstZero = true;
                storePattern = 3;
            }
        }
    }
    else
    {
        storePattern = 1;
    }
    return EE;
}

// GEP component 0
// %79 = shl i32 % 78, 2
// %80 = getelementptr[1296 x float], [1296 x float] addrspace(3) *
//       inttoptr(i32 5184 to[1296 x float] addrspace(3)*), i32 0, i32 % 79
//
// GEP component 1
// %81 = or i32 % 79, 1
// %82 = getelementptr[1296 x float], [1296 x float] addrspace(3) *
//       inttoptr(i32 5184 to[1296 x float] addrspace(3)*), i32 0, i32 % 81

uint HFpackingOpt::getGEPcomponent(GetElementPtrInst* GEP)
{
    if (!GEP)
        return 0;

    if (Instruction* inst = dyn_cast<Instruction>(GEP->getOperand(2)))
    {
        if (inst->getOpcode() == Instruction::Or ||
            inst->getOpcode() == Instruction::Add)
        {
            if (ConstantInt* CI = dyn_cast<ConstantInt>(inst->getOperand(1)))
            {
                return (uint)CI->getZExtValue();
            }
        }
    }
    return 0;
}

bool HFpackingOpt::canMerge(HFdata& prev, GetElementPtrInst* GEP,
    const uint arraySize, const uint elmBytes, const uint offset,
    const uint storePattern, const bool bStoreConstZero, bool& removeSLM)
{
    removeSLM = false;

    if (!prev.isValid())
        return false;

    if (!GEP || !prev.GEP)
        return false;

    if ((arraySize != prev.arraySize) || (elmBytes != prev.elmBytes) ||
        (bStoreConstZero != prev.bStoreConstZero))
    {
        return false;
    }

    // For load type, we will only merge the ones which are tracked by stores
    if (prev.type == HFdataType::HF_LOAD)
    {
        return isPacked(prev.offset, offset);
    }

    if (offset != prev.offset)
    {
        if (storePattern != prev.storePattern)
        {
            return false;
        }

        // if offset #2 is already tracked by the merged HF folding offset, we can't merge it
        if (HFpackedOffsets.find(offset) != HFpackedOffsets.end())
        {
            return false;
        }
    }

    /* case 1: Merge GEPs with different offsets, but same indices
      %37 = getelementptr[144 x float], [144 x float] addrspace(3) *
            inttoptr(i32 4608 to[144 x float] addrspace(3)*), i32 0, i32 %23
      %38 = getelementptr[144 x float], [144 x float] addrspace(3) *
            inttoptr(i32 5184 to[144 x float] addrspace(3)*), i32 0, i32 %23
      */
    if ((prev.offset > 0) &&
        (offset == prev.offset + prev.arraySize * prev.elmBytes) &&
        (GEP->getOperand(2) == prev.GEP->getOperand(2)))
    {
        removeSLM = true;
        return true;
    }

    /* case 2: Merge GEPs with the same offset, but different indices
      %80 = getelementptr[1296 x float], [1296 x float] addrspace(3) *
            inttoptr(i32 5184 to[1296 x float] addrspace(3)*), i32 0, i32 %79
      store float% 48, float addrspace(3)*%80, align 32
      %81 = or i32 %79, 1
      %82 = getelementptr[1296 x float], [1296 x float] addrspace(3) *
            inttoptr(i32 5184 to[1296 x float] addrspace(3)*), i32 0, i32 %81
      */
    if ((offset == prev.offset) &&
        (GEP->getOperand(2) != prev.GEP->getOperand(2)))
    {
        uint comp1 = getGEPcomponent(prev.GEP);
        uint comp2 = getGEPcomponent(GEP);
        return (comp2 == comp1 + 1);
    }

    return false;
}

void HFpackingOpt::replaceStores(Instruction* startInst, Value* newGEP,
    StoreInst *Store1, StoreInst* Store2,
    const bool bStoreConstZero1, const bool bStoreConstZero2)
{
    IRBuilder<> builder(startInst);

    if (bStoreConstZero1 && bStoreConstZero2)
    {
        builder.CreateAlignedStore(ConstantInt::get(builder.getInt32Ty(), 0),
            newGEP, IGCLLVM::getAlign(4));
    }
    else
    {
        Value* new1 = nullptr, * new2 = nullptr;
        Function* f32tof16 = GenISAIntrinsic::getDeclaration(startInst->getModule(),
            GenISAIntrinsic::GenISA_f32tof16_rtz);
        new1 = builder.CreateCall(f32tof16, Store1->getOperand(0));
        new1 = builder.CreateBitCast(new1, builder.getInt32Ty());

        new2 = builder.CreateCall(f32tof16, Store2->getOperand(0));
        new2 = builder.CreateBitCast(new2, builder.getInt32Ty());
        new2 = builder.CreateShl(new2, builder.getInt32(16));

        new1 = builder.CreateOr(new1, new2);
        new1 = builder.CreateAlignedStore(new1, newGEP, getLoadStoreAlignment(Store1));
    }
}

void HFpackingOpt::replaceLoads(Instruction *startInst, Value* newGEP,
    LoadInst *load1, LoadInst *load2)
{
    IRBuilder<> builder(startInst);
    Value* new1 = nullptr, * new2 = nullptr;

    new1 = builder.CreateAlignedLoad(builder.getInt32Ty(), newGEP,
        getLoadStoreAlignment(load1));
    new2 = builder.CreateLShr(new1, builder.getInt32(16));
    new1 = builder.CreateAnd(new1, builder.getInt32(0xffff));

    new1 = builder.CreateBitCast(new1, builder.getFloatTy());
    new2 = builder.CreateBitCast(new2, builder.getFloatTy());
    load1->replaceAllUsesWith(new1);
    load2->replaceAllUsesWith(new2);
}

// Checked if these two offsets are tracked by merged HF folding offsets
bool HFpackingOpt::isPacked(uint offset1, uint offset2)
{
    std::map<uint32_t, uint32_t>::iterator it = HFpackedOffsets.find(offset1);
    if (it != HFpackedOffsets.end() && offset2 == it->second)
    {
        return true;
    }
    return false;
}

// Merge 2 f16 stores into 1 i32 store
void HFpackingOpt::PackHfResources(Function& F)
{
    std::vector<Instruction*> RemoveInst;
    HFdata prevStore(HFdataType::HF_STORE);
    HFdata prevLoad(HFdataType::HF_LOAD);

    for (auto bb = F.begin(); bb != F.end(); ++bb)
    {
        for (auto ii = bb->begin(); ii != bb->end(); ++ii)
        {
            /*
            From
                %27 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p2621447__2D_DIM_Resource.p2621447__2D_DIM_Resource(i32 %.i0549, i32 % .i1550, i32 0, i32 0, %__2D_DIM_Resource addrspace(2621447) * undef, % __2D_DIM_Resource addrspace(2621447) * %t7, i32 0, i32 0, i32 0)
                %29 = extractelement <4 x float> %27, i32 1
                %30 = extractelement <4 x float> %27, i32 2
                %37 = getelementptr[144 x float], [144 x float] addrspace(3) * inttoptr(i32 4608 to[144 x float] addrspace(3)*), i32 0, i32 %23
                %38 = getelementptr[144 x float], [144 x float] addrspace(3) * inttoptr(i32 5184 to[144 x float] addrspace(3)*), i32 0, i32 %23
                store float %29, float addrspace(3)* %37, align 4
                store float %30, float addrspace(3)* %38, align 4
            To
                %27 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p2621447__2D_DIM_Resource.p2621447__2D_DIM_Resource(i32 %.i0549, i32 % .i1550, i32 0, i32 0, %__2D_DIM_Resource addrspace(2621447) * undef, % __2D_DIM_Resource addrspace(2621447) * %t7, i32 0, i32 0, i32 0)
                %29 = extractelement <4 x float> %27, i32 1
                %30 = extractelement <4 x float> %27, i32 2
                %37 = getelementptr[144 x i32], [144 x i32] addrspace(3) * inttoptr(i32 4608 to[144 x i32] addrspace(3)*), i32 0, i32 %23
                %38 = call float @llvm.genx.GenISA.f32tof16.rtz(float %29)
                %39 = bitcast float %38 to i32
                %40 = call float @llvm.genx.GenISA.f32tof16.rtz(float% 30)
                %41 = bitcast float %40 to i32
                %42 = shl i32 %41, 16
                %43 = or i32 %39, %42
                store i32 %43, i32 addrspace(3)* %37, align 4
            */
            if (StoreInst* Store = dyn_cast<StoreInst>(&(*ii)))
            {
                if (Store->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
                {
                    continue;
                }

                uint arraySize = 0, elmBytes = 0, offset = 0;
                GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(Store->getOperand(1));
                if (!GEP || !getGEPInfo(GEP, arraySize, elmBytes, offset))
                    continue;

                bool bStoreConstZero = false;
                uint storePattern = 0;
                ExtractElementInst* EE = decodeStore(Store, bStoreConstZero, storePattern);
                if (!supportHF(EE, offset) && !isPacked(prevStore.offset, offset))
                    continue;

                bool bRemoveSLM = false;
                bool bMerge = canMerge(prevStore, GEP, arraySize, elmBytes, offset,
                    storePattern, bStoreConstZero, bRemoveSLM);

                if (!bMerge)
                {
                    prevStore.Store = Store;
                    prevStore.GEP = GEP;
                    prevStore.arraySize = arraySize;
                    prevStore.elmBytes = elmBytes;
                    prevStore.offset = offset;
                    prevStore.bStoreConstZero = bStoreConstZero;
                    prevStore.storePattern = storePattern;
                    continue;
                }

                auto inst = GEP;
                IRBuilder<> builder(inst);

                Value* Int2Ptr = builder.CreateIntToPtr(builder.getInt32(prevStore.offset),
                    PointerType::get(ArrayType::get(builder.getInt32Ty(), prevStore.arraySize),
                        ADDRESS_SPACE_LOCAL));
                Value* gepArg[] = { prevStore.GEP->getOperand(1), prevStore.GEP->getOperand(2) };
                Value* newGEP = builder.CreateGEP(nullptr, Int2Ptr, gepArg);

                replaceStores(GEP, newGEP, prevStore.Store, Store,
                    prevStore.bStoreConstZero, bStoreConstZero);

                // track the GEP offset of the merged stores for quick lookup later
                HFpackedOffsets[prevStore.offset] = offset;
                HFpackedGEPs[prevStore.GEP] = newGEP;
                if (prevStore.offset == offset)
                {
                    bRemoveSlmGap = true;
                }

                RemoveInst.push_back(prevStore.Store);
                RemoveInst.push_back(Store);

                prevStore.clear();

                if (bRemoveSLM)
                    removeFromSlmMap(GEP);
            } // if (StoreInst* Store ...
            /*
            From
                %694 = getelementptr[144 x float], [144 x float] addrspace(3) * inttoptr(i32 4608 to[144 x float] addrspace(3)*), i32 0, i32 % 23
                %695 = getelementptr[144 x float], [144 x float] addrspace(3) * inttoptr(i32 5184 to[144 x float] addrspace(3)*), i32 0, i32 % 23
                %load24 = load float, float addrspace(3) * %694, align 4
                %load26 = load float, float addrspace(3) * %695, align 4
             To
                %694 = getelementptr [144 x i32], [144 x i32] addrspace(3)* inttoptr (i32 4608 to [144 x i32] addrspace(3)*), i32 0, i32 %23
                %695 = load i32, i32 addrspace(3)* %694, align 4
                %696 = lshr i32 %695, 16
                %697 = and i32 %695, 65535
                %698 = bitcast i32 %696 to float
                %699 = bitcast i32 %697 to float
             */
            else if (LoadInst* Load = dyn_cast<LoadInst>(&(*ii)))
            {
                if (Load->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
                {
                    continue;
                }

                uint arraySize = 0, elmBytes = 0, offset = 0;
                GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(Load->getOperand(0));
                if (!GEP || !getGEPInfo(GEP, arraySize, elmBytes, offset))
                    continue;

                bool bDummy = false;
                uint dummyPattern = 0;
                bool bMerge = canMerge(prevLoad, GEP, arraySize, elmBytes, offset,
                    dummyPattern, bDummy, bDummy);

                if (!bMerge)
                {
                    prevLoad.Load = Load;
                    prevLoad.GEP = GEP;
                    prevLoad.arraySize = arraySize;
                    prevLoad.elmBytes = elmBytes;
                    prevLoad.offset = offset;
                    continue;
                }

                Value* newGEP = nullptr;
                Instruction* startInst = nullptr;
                std::map<GetElementPtrInst*, Value*>::iterator iGEP;
                iGEP = HFpackedGEPs.find(prevLoad.GEP);
                if (iGEP != HFpackedGEPs.end())
                {
                    // no need to create new GEP
                    newGEP = iGEP->second;
                    startInst = prevLoad.Load;
                }
                else
                {
                    startInst = GEP;
                    IRBuilder<> builder(startInst);

                    Value* Int2Ptr = builder.CreateIntToPtr(builder.getInt32(prevLoad.offset),
                        PointerType::get(ArrayType::get(builder.getInt32Ty(), prevLoad.arraySize), ADDRESS_SPACE_LOCAL));
                    Value* gepArg[] = { prevLoad.GEP->getOperand(1), prevLoad.GEP->getOperand(2) };
                    newGEP = builder.CreateGEP(nullptr, Int2Ptr, gepArg);
                }

                replaceLoads(startInst, newGEP, prevLoad.Load, Load);

                RemoveInst.push_back(prevLoad.Load);
                RemoveInst.push_back(Load);

                prevLoad.clear();
            } // else if (LoadInst* Load...
        } // for (auto ii ...

        prevStore.clear();
        prevLoad.clear();
    } // for (auto bb ...

    GetElementPtrInst* removeGEP;
    for (size_t i = 0; i < RemoveInst.size(); i++)
    {
        if (LoadInst* Load = dyn_cast<LoadInst>(RemoveInst[i]))
            removeGEP = dyn_cast<GetElementPtrInst>(RemoveInst[i]->getOperand(0));
        else
            removeGEP = dyn_cast<GetElementPtrInst>(RemoveInst[i]->getOperand(1));

        RemoveInst[i]->dropAllReferences();
        RemoveInst[i]->eraseFromParent();

        if (removeGEP && removeGEP->use_empty())
        {
            removeGEP->dropAllReferences();
            removeGEP->eraseFromParent();
        }
    }
}

bool HFpackingOpt::allowedALUinst(Value* inst)
{
    // return true for ALU insts allowed in this opt.
    // can be extended to a larger range of instructions.
    if (inst == nullptr)
        return false;

    if (Instruction* ii = dyn_cast<Instruction>(inst))
    {
        switch (ii->getOpcode()) {
        case Instruction::Add:
        case Instruction::FAdd:
        case Instruction::Sub:
        case Instruction::FSub:
        case Instruction::Mul:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::Shl:
        case Instruction::LShr:
        case Instruction::AShr:
        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor:
            return true;
            break;
        default:
            break;
        }
    }

    if (IntrinsicInst* intrInst = dyn_cast<IntrinsicInst>(inst))
    {
        switch (intrInst->getIntrinsicID()) {
        case Intrinsic::sqrt:
        case Intrinsic::log:
        case Intrinsic::log2:
        case Intrinsic::cos:
        case Intrinsic::sin:
        case Intrinsic::pow:
        case Intrinsic::floor:
        case Intrinsic::ceil:
        case Intrinsic::trunc:
        case Intrinsic::maximum:
        case Intrinsic::minimum:
        case Intrinsic::fabs:
        case Intrinsic::exp:
        case Intrinsic::exp2:
            return true;
            break;
        default:
            break;
        }
    }

    return false;
}

bool HFpackingOpt::findStoreSequence(std::vector<Instruction*>& path, std::vector< std::vector<Instruction*>>& allPath)
{
    // pathIndex=0,1,2 are the store instructions. No need to check them. Start checking pathIndex=3
    // save the sequence that generates the stored value.

    // firstPath is the first set of instructions used to calculate the store to be removed.
    // All sequences going into "path" need to match the same usage pattern as firstPath, which is allPath[0].

    bool firstPath = (allPath.size() == 0 || allPath[0].size() == 0);

    uint pathIndex = 3;
    while (pathIndex < path.size())
    {
        // limit the sequences to no more than 10 instructions
        if (path.size() > 10)
            return false;

        if (!firstPath && allPath[0].size() <= pathIndex)
            return false;

        Instruction* inst = path[pathIndex];

        uint srciCount = inst->getNumOperands();
        if (CallInst* cinst = dyn_cast<CallInst>(inst))
        {
            srciCount = IGCLLVM::getNumArgOperands(cinst);
        }

        for (uint srci = 0; srci < srciCount; srci++)
        {
            // the operands uses are constant
            if (Constant* c = dyn_cast<Constant>(inst->getOperand(srci)))
            {
                // need to match with the constant used in the first set
                if (!firstPath)
                {
                    Constant* c0 = dyn_cast<Constant>(allPath[0][pathIndex]->getOperand(srci));
                    if (!c0 || c != c0)
                        return false;
                }
            }
            // the operands are the stored values from pStore0 and [1].
            else if (inst->getOperand(srci) == path[0]->getOperand(0))
            {
                if (!firstPath)
                {
                    if (allPath[0][pathIndex]->getOperand(srci) != allPath[0][0]->getOperand(0))
                    {
                        return false;
                    }
                }
            }
            else if (inst->getOperand(srci) == path[1]->getOperand(0))
            {
                if (!firstPath)
                {
                    if (allPath[0][pathIndex]->getOperand(srci) != allPath[0][1]->getOperand(0))
                    {
                        return false;
                    }
                }
            }
            else if (allowedALUinst(inst->getOperand(srci)))
            {
                if (!firstPath &&
                    (dyn_cast<Instruction>(inst->getOperand(srci))->getOpcode() !=
                        dyn_cast<Instruction>(allPath[0][pathIndex]->getOperand(srci))->getOpcode()))
                {
                    return false;
                }

                if (std::find(path.begin(), path.end(), inst->getOperand(srci)) == path.end())
                {
                    path.push_back(dyn_cast<Instruction>(inst->getOperand(srci)));
                }
            }
            else
            {
                return false;
            }
        }
        pathIndex++;
    }

    if (!firstPath && allPath[0].size() != path.size())
    {
        return false;
    }

    return true;
}

bool HFpackingOpt::adjGep(Value* gep0, Value* gep1, Value* gep2, uint32_t offset[3])
{
    // check if the GEPs are accessing continuous space.
    uint32_t arraySize[3] = { 0, 0, 0 }, elmBytes[3] = { 0, 0, 0 };
    GetElementPtrInst* GEP[3] = { nullptr, nullptr, nullptr };
    bool skipOpt = false;
    GEP[0] = dyn_cast<GetElementPtrInst>(gep0);
    GEP[1] = dyn_cast<GetElementPtrInst>(gep1);
    GEP[2] = dyn_cast<GetElementPtrInst>(gep2);
    if (GEP[0] && GEP[1] && GEP[2] &&
        GEP[0]->getOperand(2) == GEP[1]->getOperand(2) &&
        GEP[0]->getOperand(2) == GEP[2]->getOperand(2))
    {
        for (int i = 0; i < 3; i++)
        {
            if (!getGEPInfo(GEP[i], arraySize[i], elmBytes[i], offset[i]))
            {
                return false;
            }
        }

        if (skipOpt ||
            arraySize[0] != arraySize[1] || arraySize[0] != arraySize[2] ||
            elmBytes[0] != elmBytes[1] || elmBytes[0] != elmBytes[2])
        {
            return false;
        }
        if (offset[2] != offset[1] + elmBytes[0] * arraySize[0] ||
            offset[1] != offset[0] + elmBytes[0] * arraySize[0])
        {
            return false;
        }
    }
    return true;
}

void HFpackingOpt::removeRedundantChannels(Function& F)
{
    /*
    %32 = fmul fast float %29, %29
    %33 = fmul fast float %30, %30
    %34 = fadd fast float %32, %33
    %35 = fsub fast float 1.000000e+00, %34
    %36 = call fast float @llvm.sqrt.f32(float %35)
    %37 = getelementptr [144 x float], [144 x float] addrspace(3)* inttoptr (i32 4608 to [144 x float] addrspace(3)*), i32 0, i32 %23
    %38 = getelementptr [144 x float], [144 x float] addrspace(3)* inttoptr (i32 5184 to [144 x float] addrspace(3)*), i32 0, i32 %23
    %39 = getelementptr [144 x float], [144 x float] addrspace(3)* inttoptr (i32 5760 to [144 x float] addrspace(3)*), i32 0, i32 %23
    store float %29, float addrspace(3)* %37, align 4
    store float %30, float addrspace(3)* %38, align 4
    store float %36, float addrspace(3)* %39, align 4
    Since %36 is calculated from %29 and %30, we can skip the store/load for %36,
    and calculate %36 when we need to use it.
    In this example, the calculationg includes the fmul->fmul->fadd->fsub->sqrt sequence
    */

    std::map<uint32_t, uint32_t> allGEP; // allGEP[offset, count]
    for (BasicBlock& bb : F)
    {
        for (Instruction& ii : bb)
        {
            if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(&ii))
            {
                uint32_t arraySize, elmBytes, offset;
                if (getGEPInfo(GEP, arraySize, elmBytes, offset))
                {
                    allGEP[offset]++;
                }
            }
        }
    }

    // for both storeGepToRemove and loadGepToRemove, the structure is
    // storeGepToRemove[offset from inttoptr][std::vector of path];
    // where each path is a collection of instructions used to calculate the load/store being removed.
    std::map<uint32_t, std::vector<std::vector<Instruction*>>> storeGepToRemove;
    std::map<uint32_t, std::vector<std::vector<Instruction*>>> loadGepToRemove;
    uint32_t offset[3] = { 0, 0, 0 };
    for (BasicBlock& bb : F)
    {
        for (Instruction& ii : bb)
        {
            LoadInst* pLoad2 = dyn_cast<LoadInst>(&ii);
            if (pLoad2 && pLoad2->getPrevNode())
            {
                LoadInst* pLoad1 = dyn_cast<LoadInst>(pLoad2->getPrevNode());
                if (pLoad1 && pLoad1->getPrevNode())
                {
                    if (LoadInst* pLoad0 = dyn_cast<LoadInst>(pLoad1->getPrevNode()))
                    {
                        bool gepCheck = adjGep(
                            pLoad0->getPointerOperand(),
                            pLoad1->getPointerOperand(),
                            pLoad2->getPointerOperand(),
                            offset);
                        if (gepCheck && !(pLoad2->getNextNode() &&
                            dyn_cast<LoadInst>(pLoad2->getNextNode())) &&
                            dyn_cast<Instruction>(pLoad2->getOperand(0)) != nullptr)
                        {
                            std::vector<Instruction*>allLoads;
                            allLoads.push_back(pLoad0);
                            allLoads.push_back(pLoad1);
                            allLoads.push_back(pLoad2);
                            loadGepToRemove[offset[2]].push_back(allLoads);
                        }
                    }
                }
            }

            // check if the pStore can be generated by other store values.
            StoreInst* pStore2 = dyn_cast<StoreInst>(&ii);
            if (pStore2 && pStore2->getPrevNode())
            {
                StoreInst* pStore1 = dyn_cast<StoreInst>(pStore2->getPrevNode());
                if (pStore1 && pStore1->getPrevNode())
                {
                    if (StoreInst* pStore0 = dyn_cast<StoreInst>(pStore1->getPrevNode()))
                    {
                        if (pStore0->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL ||
                            pStore1->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL ||
                            pStore2->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
                        {
                            continue;
                        }

                        bool gepCheck = adjGep(pStore0->getPointerOperand(), pStore1->getPointerOperand(), pStore2->getPointerOperand(), offset);
                        Instruction* pStore2Src0 = dyn_cast<Instruction>(pStore2->getOperand(0));
                        if (gepCheck && pStore2Src0 && !(pStore2->getNextNode() &&
                            dyn_cast<StoreInst>(pStore2->getNextNode())))
                        {
                            std::vector<Instruction*>path;
                            path.push_back(pStore0);
                            path.push_back(pStore1);
                            path.push_back(pStore2);
                            path.push_back(pStore2Src0);

                            if (findStoreSequence(path, storeGepToRemove[offset[2]]))
                            {
                                storeGepToRemove[offset[2]].push_back(path);
                            }
                        }
                    }
                }
            }
        }
    }

    // if the total number of load/store qualified for the opt is not the same as the total number of all load/store
    // with the same offset, we can not apply the optimization on load/store with this offset.
    // Remove it from storeGepToRemove.
    for (auto iter = storeGepToRemove.cbegin(), next_it = iter; iter != storeGepToRemove.cend(); iter = next_it)
    {
        ++next_it;
        if (allGEP[iter->first] != storeGepToRemove[iter->first].size() + loadGepToRemove[iter->first].size())
        {
            storeGepToRemove.erase(iter);
        }
    }

    if (storeGepToRemove.size() == 0)
        return;

    GetElementPtrInst* removeGEP = nullptr;
    // start removing load/store/gep
    for (auto iter = storeGepToRemove.begin(); iter != storeGepToRemove.end(); iter++)
    {
        llvm::ValueToValueMapTy vmap;
        // create pattern for the load
        std::vector<Instruction*> copyFromSet = storeGepToRemove[iter->first][0];
        for (auto copyToInst : loadGepToRemove[iter->first])
        {
            Instruction* insertPos = copyToInst[2]->getNextNode();
            Instruction* newInst = nullptr;
            for (uint i = copyFromSet.size() - 1; i > 2; i--)
            {
                // copy the pattern
                newInst = copyFromSet[i]->clone();
                vmap[copyFromSet[i]] = newInst;
                newInst->insertBefore(insertPos);
                llvm::RemapInstruction(newInst, vmap,
                    RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

                // set the operand using the other stored values
                for (uint srci = 0; srci < copyFromSet[i]->getNumOperands(); srci++)
                {
                    if (copyFromSet[i]->getOperand(srci) == storeGepToRemove[iter->first][0][0]->getOperand(0))
                    {
                        newInst->setOperand(srci, copyToInst[0]);
                    }
                    if (copyFromSet[i]->getOperand(srci) == storeGepToRemove[iter->first][0][1]->getOperand(0))
                    {
                        newInst->setOperand(srci, copyToInst[1]);
                    }
                }
            }

            // remove load
            IGC_ASSERT(newInst);
            copyToInst[2]->replaceAllUsesWith(newInst);
            copyToInst[2]->eraseFromParent();
        }

        // remove store
        for (uint storeIndex = 0; storeIndex < storeGepToRemove[iter->first].size(); storeIndex++)
        {
            // update the slm map
            removeGEP = cast<GetElementPtrInst>(storeGepToRemove[iter->first][storeIndex][2]->getOperand(1));
            removeFromSlmMap(removeGEP);

            storeGepToRemove[iter->first][storeIndex][2]->dropAllReferences();
            storeGepToRemove[iter->first][storeIndex][2]->eraseFromParent();

            if (removeGEP->use_empty())
                removeGEP->eraseFromParent();
        }

    }
}

bool HFpackingOpt::initializeSlmOffsetMap(Function& F)
{
    uint32_t arraySize = 0, elmBytes = 0, offset = 0;
    slmOffsetMap.clear();
    for (BasicBlock& bb : F)
    {
        for (Instruction& ii : bb)
        {
            if (GetElementPtrInst* GEP = llvm::dyn_cast<GetElementPtrInst>(&(ii)))
            {
                if (GEP->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
                {
                    if (getGEPInfo(GEP, arraySize, elmBytes, offset))
                    {
                        slmOffsetMap[offset] = offset;

                        uint comp = getGEPcomponent(GEP);
                        if (slmComponentsMap.find(offset) == slmComponentsMap.end() ||
                            slmComponentsMap[offset] < comp)
                        {
                            slmComponentsMap[offset] = comp;
                        }
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void HFpackingOpt::updateSlmOffsetSize(Function& F)
{
    uint32_t arraySize = 0, elmBytes = 0, offset = 0;

    if (!bRemoveFromSlmMap)
        return;

    for (BasicBlock& bb : F)
    {
        for (Instruction& ii : bb)
        {
            if (GetElementPtrInst* GEP = llvm::dyn_cast<GetElementPtrInst>(&(ii)))
            {
                if (GEP->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL &&
                    getGEPInfo(GEP, arraySize, elmBytes, offset))
                {
                    if (slmOffsetMap[offset] != -1 && slmOffsetMap[offset] != offset)
                    {
                        PointerType* PTy = cast<PointerType>(GEP->getPointerOperand()->getType());
                        Constant* CO = ConstantInt::get(Type::getInt32Ty(F.getContext()), slmOffsetMap[offset]);
                        GEP->setOperand(0, ConstantExpr::getIntToPtr(CO, PTy));
                    }
                }
            }
        }
    }

    csCtx->m_slmSize = iSTD::RoundPower2(DWORD(csCtx->m_tgsmSize));
}

// If GEP @5184 is 4-component array, we can reduce SLM footprint by half after packing them in pairs
// Before
// %200 = getelementptr[972 x float], [972 x float] addrspace(3) * inttoptr(i32 1296 to .....
// %300 = getelementptr[1296x float], [1296x float] addrspace(3) * inttoptr(i32 5184 to .....
// %400 = getelementptr[486 x float], [486 x float] addrspace(3) * inttoptr(i32 10368 to .....
// After
// %200 = getelementptr[972 x float], [972 x float] addrspace(3) * inttoptr(i32 1296 to .....
// %300 = getelementptr[648 x float], [648 x float] addrspace(3) * inttoptr(i32 5184 to .....
// %400 = getelementptr[486 x float], [486 x float] addrspace(3) * inttoptr(i32 7776 to .....
void HFpackingOpt::removeSlmGap(Function& F)
{
    uint32_t arraySize = 0, elmBytes = 0, offset = 0;
    std::map<uint, uint> savedSlmSpace;
    std::vector<GetElementPtrInst*> RemoveGEPs;

    if (!bRemoveSlmGap)
        return;

    for (BasicBlock& bb : F)
    {
        for (Instruction& ii : bb)
        {
            if (GetElementPtrInst* GEP = llvm::dyn_cast<GetElementPtrInst>(&(ii)))
            {
                if (GEP->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL &&
                    getGEPInfo(GEP, arraySize, elmBytes, offset))
                {
                    if (HFpackedOffsets.find(offset) == HFpackedOffsets.end())
                        continue;
                    else if (HFpackedOffsets[offset] != offset)
                        continue;

                    if (slmComponentsMap.find(offset) != slmComponentsMap.end() &&
                        slmComponentsMap[offset] == 3) // 4-component SLM
                    {
                        if (savedSlmSpace.find(offset) == savedSlmSpace.end())
                        {
                            // cut down SLM space by half
                            // saving (arraySize/2) entries, or 2*arraySize bytes
                            savedSlmSpace[offset] = arraySize << 1; // 2*arraySize
                        }
                        IRBuilder<> builder(GEP);

                        Value* Int2Ptr = builder.CreateIntToPtr(builder.getInt32(offset),
                            PointerType::get(ArrayType::get(builder.getInt32Ty(), arraySize>>1), ADDRESS_SPACE_LOCAL));
                        Value* div2 = builder.CreateLShr(GEP->getOperand(2), builder.getInt32(1));
                        Value* gepArg[] = { GEP->getOperand(1), div2 };
                        Value* newGEP = builder.CreateGEP(nullptr, Int2Ptr, gepArg);
                        GEP->replaceAllUsesWith(newGEP);

                        if (GEP->use_empty())
                            RemoveGEPs.push_back(GEP);
                    }
                }
            }
        }
    }

    for (std::map<uint, uint>::iterator it = savedSlmSpace.begin();
        it != savedSlmSpace.end(); ++it)
    {
        adjOffsetMap(it->first, it->second, false);
    }

    for (size_t i = 0; i < RemoveGEPs.size(); i++)
        RemoveGEPs[i]->eraseFromParent();

    csCtx->m_slmSize = iSTD::RoundPower2(DWORD(csCtx->m_tgsmSize));
}

bool HFpackingOpt::runOnFunction(Function& F)
{
    pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    csCtx = static_cast<ComputeShaderContext*>(pContext);
    modMD = pContext->getModuleMetaData();

    bool allowOpt = initializeSlmOffsetMap(F);
    if (!allowOpt)
        return false;

    removeRedundantChannels(F);

    if (modMD->csInfo.ResForHfPacking.size() == 0)
    {
        getHFPackingCandidate(F);
    }
    else
    {
        PackHfResources(F);
    }

    // update slm offsets and size
    removeSlmGap(F);
    updateSlmOffsetSize(F);

    return false;
}
