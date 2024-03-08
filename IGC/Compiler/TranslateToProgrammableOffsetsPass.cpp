/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TranslateToProgrammableOffsetsPass.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/igc_resourceDimTypes.h"


using namespace llvm;
using namespace IGC;


class TranslateToProgrammableOffsetsPass : public llvm::FunctionPass
{
public:
    static char ID;

    TranslateToProgrammableOffsetsPass();

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<CodeGenContextWrapper>();
        AU.setPreservesCFG();
    }

    StringRef getPassName() const { return "TranslateToProgrammableOffsetsPass"; }

    bool runOnFunction(Function& F);
};

char TranslateToProgrammableOffsetsPass::ID = 0;

#define PASS_FLAG     "igc-TranslateToProgrammableOffsetsPass"
#define PASS_DESC     "IGC runs translation of samples and gathers with non const"\
                      "immediate offset intrinsics to corresponding PO versions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TranslateToProgrammableOffsetsPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TranslateToProgrammableOffsetsPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)


namespace IGC
{
    llvm::FunctionPass* createTranslateToProgrammableOffsetsPass()
    {
        return new TranslateToProgrammableOffsetsPass();
    }
}

namespace
{

template <GenISAIntrinsic::ID T>
struct dependent_false : std::false_type {};

template <typename T>
class HasIsApplicableMethod
{
private:
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C> static YesType& test(decltype(&C::IsApplicable));
    template <typename C> static NoType& test(...);


public:
    enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
};

} // namespace anonymous


TranslateToProgrammableOffsetsPass::TranslateToProgrammableOffsetsPass()
    :FunctionPass(ID)
{
    initializeTranslateToProgrammableOffsetsPassPass(*PassRegistry::getPassRegistry());
}


template<GenISAIntrinsic::ID Intrinsic>
struct TranslateIntrinsicImpl
{
    static_assert(dependent_false<Intrinsic>::value, "Target translation is not supported.");

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        return nullptr;
    }
};

struct TranslateIntrinsicBase
{
    template<class Traits, class SampleOrGatherInstrinsic>
    static llvm::Function* GetTargetFunction(SampleOrGatherInstrinsic* sampleOrGatherIntr)
    {
        llvm::Type* funcTypes[] =
        {
            sampleOrGatherIntr->getType(),
            sampleOrGatherIntr->getOperand(1)->getType(),
            sampleOrGatherIntr->getPairedTextureValue()->getType(),
            sampleOrGatherIntr->getTextureValue()->getType(),
            sampleOrGatherIntr->getSamplerValue()->getType()
        };

        Function* newFunc = GenISAIntrinsic::getDeclaration(
            sampleOrGatherIntr->getModule(),
            Traits::TargetIntrinsic,
            funcTypes);

        return newFunc;
    }

    template<class Traits>
    static llvm::Function* GetTargetFunction(SamplerLoadIntrinsic* loadIntr)
    {
        return loadIntr->getCalledFunction();
    }

    template<class Traits, class SampleOrGatherInstrinsic>
    static llvm::CallInst* CreateTranslatedCall(SampleOrGatherInstrinsic* sampleOrGatherIntr,
        const llvm::SmallVector<llvm::Value*, 10>& args)
    {
        IRBuilder<> builder(sampleOrGatherIntr->getContext());
        builder.SetInsertPoint(sampleOrGatherIntr);
        llvm::Function* newFunc = GetTargetFunction<Traits>(sampleOrGatherIntr);
        llvm::CallInst* newSample = builder.CreateCall(newFunc, args, "");
        return newSample;
    }

    template<class SampleOrGatherInstrinsic>
    static void ZeroImmediateOffsets(SampleOrGatherInstrinsic* sampleOrGatherIntr)
    {
        IRBuilder<> builder(sampleOrGatherIntr->getContext());
        builder.SetInsertPoint(sampleOrGatherIntr);

        uint immOffsetSourceIndex = sampleOrGatherIntr->getImmediateOffsetsIndex();
        sampleOrGatherIntr->setOperand(immOffsetSourceIndex, builder.getInt32(0));
        sampleOrGatherIntr->setOperand(immOffsetSourceIndex + 1, builder.getInt32(0));
        sampleOrGatherIntr->setOperand(immOffsetSourceIndex + 2, builder.getInt32(0));
    }
};

template<GenISAIntrinsic::ID TTranslateIntrinsic>
struct TranslateIntrinsic : TranslateIntrinsicImpl<TTranslateIntrinsic>
{
    using ImplType = TranslateIntrinsicImpl<TTranslateIntrinsic>;

    static bool IsValidImmediateOffset(llvm::Value* offset)
    {
        constexpr int64_t cMinImmediateOffset = -8;
        constexpr int64_t cMaxImmediateOffset =  7;
        llvm::ConstantInt* imm = llvm::dyn_cast<llvm::ConstantInt>(offset);
        if (imm &&
            imm->getSExtValue() >= cMinImmediateOffset &&
            imm->getSExtValue() <= cMaxImmediateOffset)
        {
            return true;
        }
        return false;
    }

    template<class SampleOrGatherInstrinsic>
    static bool IsApplicable(SampleOrGatherInstrinsic* sampleOrGatherIntr)
    {
        uint immOffsetSourceIndex = sampleOrGatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = sampleOrGatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = sampleOrGatherIntr->getOperand(immOffsetSourceIndex + 1);
        llvm::Value* offsetR = sampleOrGatherIntr->getOperand(immOffsetSourceIndex + 2);

        bool applicable =
            !IsValidImmediateOffset(offsetU) ||
            !IsValidImmediateOffset(offsetV) ||
            !IsValidImmediateOffset(offsetR);

        applicable &= IsApplicableDerived<ImplType>(sampleOrGatherIntr);

        return applicable;
    }


    template<class SampleOrGatherInstrinsic>
    static llvm::CallInst* Translate(SampleOrGatherInstrinsic* sampleOrGatherIntr)
    {
        if (!IsApplicable(sampleOrGatherIntr))
        {
            return nullptr;
        }

        return ImplType::TranslateDetail(sampleOrGatherIntr);
    }

private:
    template<typename T, class SampleOrGatherInstrinsic>
    static typename std::enable_if<HasIsApplicableMethod<T>::value, bool>::type
        IsApplicableDerived(SampleOrGatherInstrinsic* sampleOrGatherIntr)
    {
        return ImplType::IsApplicable(sampleOrGatherIntr);
    }

    template<typename T, class SampleOrGatherInstrinsic>
    static typename std::enable_if<!HasIsApplicableMethod<T>::value, bool>::type
        IsApplicableDerived(SampleOrGatherInstrinsic*)
    {
        return true;
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePOptr;

    static llvm::Value* PackOffsetUVR(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        uint immOffsetSourceIndex = sampleIntr->getNumOperands() - 4;
        llvm::Value* offsetU = sampleIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = sampleIntr->getOperand(immOffsetSourceIndex + 1);
        llvm::Value* offsetR = sampleIntr->getOperand(immOffsetSourceIndex + 2);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);
        llvm::Value* clampedOffsetR = builder.CreateAnd(offsetR, 0xf);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 4));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetR, 8));
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint aiOffset = 3;

        llvm::Value* packedOffsetUVR = PackOffsetUVR(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == aiOffset)
            {
                args.push_back(packedOffsetUVR);
                continue;
            }

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleBptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePOBptr;

    static llvm::Value* PackBiasOffsetUVR(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        uint immOffsetSourceIndex = sampleIntr->getNumOperands() - 4;
        llvm::Value* offsetU = sampleIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = sampleIntr->getOperand(immOffsetSourceIndex + 1);
        llvm::Value* offsetR = sampleIntr->getOperand(immOffsetSourceIndex + 2);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);
        llvm::Value* clampedOffsetR = builder.CreateAnd(offsetR, 0xf);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 4));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetR, 8));

        llvm::Value* bias = sampleIntr->getBiasValue();
        llvm::Value* clampedBias = builder.CreateAnd(
            builder.CreateBitCast(bias, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedBias);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint aiOffset = 4;
        uint biasOffset = 0;

        llvm::Value* packedBiasOffsetUVR = PackBiasOffsetUVR(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == biasOffset)
            {
                args.push_back(packedBiasOffsetUVR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleLptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePOLptr;

    static llvm::Value* PackLodOffsetUVR(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        uint immOffsetSourceIndex = sampleIntr->getNumOperands() - 4;
        llvm::Value* offsetU = sampleIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = sampleIntr->getOperand(immOffsetSourceIndex + 1);
        llvm::Value* offsetR = sampleIntr->getOperand(immOffsetSourceIndex + 2);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);
        llvm::Value* clampedOffsetR = builder.CreateAnd(offsetR, 0xf);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 4));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetR, 8));

        llvm::Value* lod = sampleIntr->getLodValue();
        llvm::Value* clampedLod = builder.CreateAnd(
            builder.CreateBitCast(lod, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedLod);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint aiOffset = 4;
        uint lodOffset = 0;

        llvm::Value* packedLodOffsetUVR = PackLodOffsetUVR(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == lodOffset)
            {
                args.push_back(packedLodOffsetUVR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleCptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePOCptr;

    static llvm::Value* PackOffsetUVCoordR(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        llvm::Value* offsetU = sampleIntr->getImmediateOffsetsValue(0);
        llvm::Value* offsetV = sampleIntr->getImmediateOffsetsValue(1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);

        llvm::Value* aux = builder.getInt32(0);
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetU, 12));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 16));

        llvm::Value* r = sampleIntr->getOperand(3);
        aux = CombineSampleOrGather4Params(builder, r, aux, 12, "", "");

        return builder.CreateBitCast(aux, builder.getFloatTy());
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint rOffset = 3;
        uint aiOffset = 4;

        llvm::Value* packedOffUVCoordR = PackOffsetUVCoordR(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == rOffset)
            {
                args.push_back(packedOffUVCoordR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    };
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleDptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePODptr;

    static bool IsApplicable(SampleIntrinsic* sampleIntr)
    {
        // Don't translate sample_d with 3D texture.
        // If the offset is non const it will be translated in EmitPass::emulateSampleD().
        Type* textureType = IGCLLVM::getNonOpaquePtrEltTy(sampleIntr->getTextureValue()->getType());
        Type* volumeTextureType = GetResourceDimensionType(*sampleIntr->getModule(),
            RESOURCE_DIMENSION_TYPE::DIM_3D_TYPE);
        return textureType != volumeTextureType;
    }

    static llvm::Value* PackOffsetUVRCoordR(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        llvm::Value* offsetU = sampleIntr->getImmediateOffsetsValue(0);
        llvm::Value* offsetV = sampleIntr->getImmediateOffsetsValue(1);
        llvm::Value* offsetR = sampleIntr->getImmediateOffsetsValue(2);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);
        llvm::Value* clampedOffsetR = builder.CreateAnd(offsetR, 0xf);

        llvm::Value* aux = builder.getInt32(0);
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetU, 12));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 16));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetR, 20));

        llvm::Value* r = sampleIntr->getOperand(6);
        aux = CombineSampleOrGather4Params(builder, r, aux, 12, "", "");

        return builder.CreateBitCast(aux, builder.getFloatTy());
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint rOffset = 6;
        uint aiOffset = 9;

        llvm::Value* packedOffUVRCoordR = PackOffsetUVRCoordR(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == rOffset)
            {
                args.push_back(packedOffUVRCoordR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_sampleLCptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_samplePOLCptr;

    static llvm::Value* PackLodOffsetUV(SampleIntrinsic* sampleIntr)
    {
        IRBuilder<> builder(sampleIntr->getContext());
        builder.SetInsertPoint(sampleIntr);

        llvm::Value* offsetU = sampleIntr->getImmediateOffsetsValue(0);
        llvm::Value* offsetV = sampleIntr->getImmediateOffsetsValue(1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0xf);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0xf);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 4));

        llvm::Value* lod = sampleIntr->getOperand(1);
        llvm::Value* clampedLod = builder.CreateAnd(
            builder.CreateBitCast(lod, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedLod);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SampleIntrinsic* sampleIntr)
    {
        uint aiOffset = 5;
        uint lodOffset = 1;

        llvm::Value* packedLodOffsetUV = PackLodOffsetUV(sampleIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(sampleIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(sampleIntr); i++)
        {
            if (i == lodOffset)
            {
                args.push_back(packedLodOffsetUV);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(sampleIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(sampleIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4ptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedptr;

    static llvm::Value* PackOffsetUV(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        uint immOffsetSourceIndex = gatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = gatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = gatherIntr->getOperand(immOffsetSourceIndex + 1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 6));
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint aiOffset = 3;

        llvm::Value* packedOffsetUV = PackOffsetUV(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == aiOffset)
            {
                args.push_back(packedOffsetUV);
                continue;
            }

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4Lptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedLptr;

    static llvm::Value* PackLodOffsetUV(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        uint immOffsetSourceIndex = gatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = gatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = gatherIntr->getOperand(immOffsetSourceIndex + 1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 6));

        llvm::Value* lod = gatherIntr->getLodValue();
        llvm::Value* clampedLod = builder.CreateAnd(
            builder.CreateBitCast(lod, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedLod);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint aiOffset = 4;
        uint lodOffset = gatherIntr->getLodIndex();

        llvm::Value* packedLodOffsetUV = PackLodOffsetUV(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == lodOffset)
            {
                args.push_back(packedLodOffsetUV);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4Bptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedBptr;

    static llvm::Value* PackBiasOffsetUV(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        uint immOffsetSourceIndex = gatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = gatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = gatherIntr->getOperand(immOffsetSourceIndex + 1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 6));

        llvm::Value* bias = gatherIntr->getBiasValue();
        llvm::Value* clampedBias = builder.CreateAnd(
            builder.CreateBitCast(bias, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedBias);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint aiOffset = 4;
        uint biasOffset = gatherIntr->getBiasIndex();

        llvm::Value* packedBiasOffsetUVR = PackBiasOffsetUV(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == biasOffset)
            {
                args.push_back(packedBiasOffsetUVR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4Iptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedIptr;

    static llvm::Value* PackOffsetUV(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        uint immOffsetSourceIndex = gatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = gatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = gatherIntr->getOperand(immOffsetSourceIndex + 1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 6));
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint aiOffset = 3;

        llvm::Value* packedOffsetUV = PackOffsetUV(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == aiOffset)
            {
                args.push_back(packedOffsetUV);
                continue;
            }

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4Cptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedCptr;

    static llvm::Value* PackOffsetUVCoordR(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        llvm::Value* offsetU = gatherIntr->getImmediateOffsetsValue(0);
        llvm::Value* offsetV = gatherIntr->getImmediateOffsetsValue(1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = builder.getInt32(0);
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetU, 12));
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 18));

        llvm::Value* r = gatherIntr->getOperand(3);
        aux = CombineSampleOrGather4Params(builder, r, aux, 12, "", "");

        return builder.CreateBitCast(aux, builder.getFloatTy());
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint rOffset = 3;
        uint aiOffset = 4;

        llvm::Value* packedOffUVCoordR = PackOffsetUVCoordR(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == rOffset)
            {
                args.push_back(packedOffUVCoordR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    };
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4ICptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedICptr;


    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint rOffset = 3;
        uint aiOffset = 4;

        llvm::Value* packedOffUVCoordR =
            TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4Cptr>::PackOffsetUVCoordR(gatherIntr);

        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == rOffset)
            {
                args.push_back(packedOffUVCoordR);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    };
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_gather4LCptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_gather4POPackedLCptr;

    static llvm::Value* PackLodOffsetUV(SamplerGatherIntrinsic* gatherIntr)
    {
        IRBuilder<> builder(gatherIntr->getContext());
        builder.SetInsertPoint(gatherIntr);

        uint immOffsetSourceIndex = gatherIntr->getImmediateOffsetsIndex();
        llvm::Value* offsetU = gatherIntr->getOperand(immOffsetSourceIndex);
        llvm::Value* offsetV = gatherIntr->getOperand(immOffsetSourceIndex + 1);

        llvm::Value* clampedOffsetU = builder.CreateAnd(offsetU, 0x3f);
        llvm::Value* clampedOffsetV = builder.CreateAnd(offsetV, 0x3f);

        llvm::Value* aux = clampedOffsetU;
        aux = builder.CreateOr(aux, builder.CreateShl(clampedOffsetV, 6));

        llvm::Value* lod = gatherIntr->getLodValue();
        llvm::Value* clampedLod = builder.CreateAnd(
            builder.CreateBitCast(lod, builder.getInt32Ty()),
            0xfffff000);
        aux = builder.CreateOr(aux, clampedLod);
        aux = builder.CreateBitCast(aux, builder.getFloatTy());

        return aux;
    }

    static llvm::CallInst* TranslateDetail(SamplerGatherIntrinsic* gatherIntr)
    {
        uint aiOffset = 5;
        uint lodOffset = gatherIntr->getLodIndex();

        llvm::Value* packedLodOffsetUV = PackLodOffsetUV(gatherIntr);
        TranslateIntrinsicBase::ZeroImmediateOffsets(gatherIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(gatherIntr); i++)
        {
            if (i == lodOffset)
            {
                args.push_back(packedLodOffsetUV);
                continue;
            }

            if (i == aiOffset)
                continue;

            args.push_back(gatherIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(gatherIntr, args);
    }
};

template<>
struct TranslateIntrinsicImpl<GenISAIntrinsic::GenISA_ldptr>
{
    static const GenISAIntrinsic::ID TargetIntrinsic = GenISAIntrinsic::GenISA_ldptr;

    static llvm::CallInst* TranslateDetail(SamplerLoadIntrinsic* loadIntr)
    {
        IRBuilder<> builder(loadIntr->getContext());
        builder.SetInsertPoint(loadIntr);

        uint immOffsetIndex = loadIntr->getImmediateOffsetsIndex();
        uint coordIndex = loadIntr->getCoordinateIndex(0);

        // Clamp the offsets and add them to the coordinates.
        for (uint i = 0; i < 3; i++)
        {
            llvm::Value* offset = loadIntr->getArgOperand(immOffsetIndex + i);
            llvm::Value* clampedOffset = builder.CreateAShr(builder.CreateShl(offset, 28), 28);
            llvm::Value* newCoord = builder.CreateAdd(
                loadIntr->getArgOperand(coordIndex + i),
                clampedOffset
            );
            loadIntr->setArgOperand(coordIndex + i, newCoord);
        }

        TranslateIntrinsicBase::ZeroImmediateOffsets(loadIntr);

        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(loadIntr); i++)
        {
            args.push_back(loadIntr->getArgOperand(i));
        }

        return TranslateIntrinsicBase::CreateTranslatedCall<TranslateIntrinsicImpl>(loadIntr, args);
    }
};

bool TranslateToProgrammableOffsetsPass::runOnFunction(Function& F)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!ctx->platform.supportsProgrammableOffsets())
    {
        return false;
    }

    bool changed = false;

    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        auto II = BI->begin();
        while (II != BI->end())
        {
            llvm::CallInst* newInst = nullptr;

            if (SampleIntrinsic* sampleInst = dyn_cast<SampleIntrinsic>(II))
            {
                switch (sampleInst->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_sampleptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleptr>::Translate(sampleInst);
                    break;
                case GenISAIntrinsic::GenISA_sampleBptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleBptr>::Translate(sampleInst);
                    break;
                case GenISAIntrinsic::GenISA_sampleLptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleLptr>::Translate(sampleInst);
                    break;
                case GenISAIntrinsic::GenISA_sampleCptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleCptr>::Translate(sampleInst);
                    break;
                case GenISAIntrinsic::GenISA_sampleDptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleDptr>::Translate(sampleInst);
                    break;
                case GenISAIntrinsic::GenISA_sampleLCptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_sampleLCptr>::Translate(sampleInst);
                    break;
                default:
                    break;
                }
            }

            if (SamplerGatherIntrinsic* gatherInst = dyn_cast<SamplerGatherIntrinsic>(II))
            {
                switch (gatherInst->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_gather4ptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4ptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4Lptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4Lptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4Bptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4Bptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4Iptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4Iptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4Cptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4Cptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4ICptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4ICptr>::Translate(gatherInst);
                    break;
                case GenISAIntrinsic::GenISA_gather4LCptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_gather4LCptr>::Translate(gatherInst);
                    break;
                default:
                    break;
                }
            }

            if (SamplerLoadIntrinsic* loadInst = dyn_cast<SamplerLoadIntrinsic>(II))
            {
                switch (loadInst->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_ldptr:
                    newInst = TranslateIntrinsic<GenISAIntrinsic::GenISA_ldptr>::Translate(loadInst);
                    break;
                default:
                    break;
                }
            }

            // swap with new intrinsic
            if (newInst)
            {
                newInst->setDebugLoc(II->getDebugLoc());
                II->replaceAllUsesWith(newInst);
                II = II->eraseFromParent();

                changed = true;
            }

            if (!newInst)
            {
                II++;
            }
        }
    }

    return changed;
}
