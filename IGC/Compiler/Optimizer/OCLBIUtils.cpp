/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "Probe/Assertion.h"
#include <llvm/Support/Casting.h>
#include "IGC/Compiler/CISACodeGen/messageEncoding.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

void CCommand::execute(CallInst* Inst, CodeGenContext* CodeGenContext)
{
    init(Inst, CodeGenContext);
    createIntrinsic();
}

void CCommand::init(CallInst* Inst, CodeGenContext* CodeGenContext)
{
    m_pCallInst = Inst;
    m_pFunc = m_pCallInst->getParent()->getParent();
    m_pCtx = &(m_pFunc->getContext());
    m_pCodeGenContext = CodeGenContext;
    m_pFloatType = Type::getFloatTy(*m_pCtx);
    m_pIntType = Type::getInt32Ty(*m_pCtx);
    m_pFloatZero = Constant::getNullValue(m_pFloatType);
    m_pIntZero = Constant::getNullValue(m_pIntType);
    m_pIntOne = ConstantInt::get(m_pIntType, 1);
    m_DL = m_pCallInst->getDebugLoc();
    m_args.clear();
}

Function* CCommand::getFunctionDeclaration(GenISAIntrinsic::ID id, ArrayRef<Type*> Tys)
{
    return GenISAIntrinsic::getDeclaration(m_pFunc->getParent(), id, Tys);
}

Function* CCommand::getFunctionDeclaration(IGCLLVM::Intrinsic id, ArrayRef<Type*> Tys)
{
    return Intrinsic::getDeclaration(m_pFunc->getParent(), id, Tys);
}

void CCommand::replaceCallInst(IGCLLVM::Intrinsic intrinsicName, ArrayRef<Type*> Tys)
{
    Function* func = getFunctionDeclaration(intrinsicName, Tys);
    Instruction* newCall = CallInst::Create(func, m_args, m_pCallInst->getName(), m_pCallInst);

    if (isa<FPMathOperator>(m_pCallInst)) {
        if (auto II = dyn_cast<IntrinsicInst>(newCall)) {
            II->copyFastMathFlags(m_pCallInst->getFastMathFlags());
        }
    }

    newCall->setDebugLoc(m_DL);
    m_pCallInst->replaceAllUsesWith(newCall);
}

void CCommand::replaceGenISACallInst(GenISAIntrinsic::ID intrinsicName, ArrayRef<Type*> Tys)
{
    Function* func = getFunctionDeclaration(intrinsicName, Tys);
    Instruction* newCall = CallInst::Create(func, m_args, m_pCallInst->getName(), m_pCallInst);
    newCall->setDebugLoc(m_DL);
    m_pCallInst->replaceAllUsesWith(newCall);
}

void CCommand::emitError(const char *ErrorStr, const Value *Context)
{
    m_pCodeGenContext->EmitError(ErrorStr, Context);
    m_bHasError = true;
}

void CImagesBI::prepareZeroOffsets()
{
    m_args.push_back(m_pIntZero); // offsetU
    m_args.push_back(m_pIntZero); // offsetV
    m_args.push_back(m_pIntZero); // offsetW
}

void CImagesBI::prepareCoords(Dimension Dim, Value* Coord, Value* Zero)
{
    CoordX = Coord;
    CoordY = Zero;
    CoordZ = Zero;
    Instruction* tmp;
    switch (Dim)
    {
    case DIM_3D:
    case DIM_2D_ARRAY:
        tmp = ExtractElementInst::Create(
            Coord,
            ConstantInt::get(m_pIntType, COORD_Z),
            "CoordZ",
            m_pCallInst); // z
        tmp->setDebugLoc(m_DL);
        CoordZ = tmp;
        // fall through
    case DIM_2D:
    case DIM_1D_ARRAY:
        tmp = ExtractElementInst::Create(
            Coord,
            ConstantInt::get(m_pIntType, COORD_X),
            "CoordX",
            m_pCallInst); // x
        tmp->setDebugLoc(m_DL);
        CoordX = tmp;
        tmp = ExtractElementInst::Create(
            Coord,
            ConstantInt::get(m_pIntType, COORD_Y),
            "CoordY",
            m_pCallInst); // y
        tmp->setDebugLoc(m_DL);
        CoordY = tmp;
        // fall through
    case DIM_1D:
        //  no need to extract since in 1 Dim Coord isn't a vector
        break;
    case DIM_UNKNOWN:
        emitError("Unexpected dimension", NULL);
        break;
    }
}

void CImagesBI::CreateInlineSamplerAnnotations(Module* M, InlineSamplersMD& inlineSamplerMD, int samplerValue)
{
    inlineSamplerMD.m_Value = samplerValue;
    if (llvm::StringRef(M->getTargetTriple()).startswith("igil") || llvm::StringRef(M->getTargetTriple()).startswith("gpu_64"))
    {
        inlineSamplerMD.addressMode = samplerValue & LEGACY_SAMPLER_ADDRESS_MASK;
        switch (samplerValue & LEGACY_SAMPLER_ADDRESS_MASK)
        {
        case LEGACY_CLK_ADDRESS_NONE:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case LEGACY_CLK_ADDRESS_CLAMP:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER;
            break;
        case LEGACY_CLK_ADDRESS_CLAMP_TO_EDGE:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case LEGACY_CLK_ADDRESS_REPEAT:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        case LEGACY_CLK_ADDRESS_MIRRORED_REPEAT:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR;
            break;
#if defined ( _DEBUG ) || defined ( _INTERNAL )
        case LEGACY_CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL:
            inlineSamplerMD.TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101;
            inlineSamplerMD.TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101;
            inlineSamplerMD.TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101;
            break;
#endif // RELEASE INTERNAL||DEBUG

        default:
            IGC_ASSERT_MESSAGE(0, "Invalid sampler type");
            break;
        }

        inlineSamplerMD.NormalizedCoords = (samplerValue & LEGACY_SAMPLER_NORMALIZED_MASK);

        switch (samplerValue & LEGACY_SAMPLER_FILTER_MASK)
        {
        case LEGACY_CLK_FILTER_NEAREST:
            inlineSamplerMD.MagFilterType = (iOpenCL::SAMPLER_MAPFILTER_POINT);
            inlineSamplerMD.MinFilterType = (iOpenCL::SAMPLER_MAPFILTER_POINT);
            break;
        case LEGACY_CLK_FILTER_LINEAR:
            inlineSamplerMD.MagFilterType = (iOpenCL::SAMPLER_MAPFILTER_LINEAR);
            inlineSamplerMD.MinFilterType = (iOpenCL::SAMPLER_MAPFILTER_LINEAR);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Filter Type must have value");
            break;
        }

        // Border color should always be transparent black:
        inlineSamplerMD.BorderColorR = (0.0f);
        inlineSamplerMD.BorderColorG = (0.0f);
        inlineSamplerMD.BorderColorB = (0.0f);
        inlineSamplerMD.BorderColorA = (0.0f);
    }
    else if (llvm::StringRef(M->getTargetTriple()).startswith("spir"))
    {
        switch (samplerValue & SPIR_SAMPLER_ADDRESS_MASK)
        {
        case SPIR_CLK_ADDRESS_NONE:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.addressMode = (LEGACY_CLK_ADDRESS_NONE);
            break;
        case SPIR_CLK_ADDRESS_CLAMP:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_BORDER);
            inlineSamplerMD.addressMode = (LEGACY_CLK_ADDRESS_CLAMP);
            break;
        case SPIR_CLK_ADDRESS_CLAMP_TO_EDGE:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP);
            inlineSamplerMD.addressMode = (LEGACY_CLK_ADDRESS_CLAMP_TO_EDGE);
            break;
        case SPIR_CLK_ADDRESS_REPEAT:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_WRAP);
            inlineSamplerMD.addressMode = (LEGACY_CLK_ADDRESS_REPEAT);
            break;
        case SPIR_CLK_ADDRESS_MIRRORED_REPEAT:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR);
            inlineSamplerMD.addressMode = (LEGACY_CLK_ADDRESS_MIRRORED_REPEAT);
            break;
#if defined ( _DEBUG ) || defined ( _INTERNAL )
        case SPIR_CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL:
            inlineSamplerMD.TCXAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101);
            inlineSamplerMD.TCYAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101);
            inlineSamplerMD.TCZAddressMode = (iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101);
            break;
#endif // RELEASE INTERNAL||DEBUG

        default:
            IGC_ASSERT_MESSAGE(0, "Invalid sampler type");
            break;
        }

        switch (samplerValue & SPIR_SAMPLER_NORMALIZED_MASK)
        {
        case SPIR_CLK_NORMALIZED_COORDS_TRUE:
            inlineSamplerMD.NormalizedCoords = (LEGACY_CLK_NORMALIZED_COORDS_TRUE);
            break;
        case SPIR_CLK_NORMALIZED_COORDS_FALSE:
            inlineSamplerMD.NormalizedCoords = (LEGACY_CLK_NORMALIZED_COORDS_FALSE);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Invalid normalized coords");
            break;
        }


        switch (samplerValue & SPIR_SAMPLER_FILTER_MASK)
        {
        case SPIR_CLK_FILTER_NEAREST:
            inlineSamplerMD.MagFilterType = (iOpenCL::SAMPLER_MAPFILTER_POINT);
            inlineSamplerMD.MinFilterType = (iOpenCL::SAMPLER_MAPFILTER_POINT);
            break;
        case SPIR_CLK_FILTER_LINEAR:
            inlineSamplerMD.MagFilterType = (iOpenCL::SAMPLER_MAPFILTER_LINEAR);
            inlineSamplerMD.MinFilterType = (iOpenCL::SAMPLER_MAPFILTER_LINEAR);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Filter Type must have value");
            break;
        }

        // Border color should always be transparent black:
        inlineSamplerMD.BorderColorR = (0.0f);
        inlineSamplerMD.BorderColorG = (0.0f);
        inlineSamplerMD.BorderColorB = (0.0f);
        inlineSamplerMD.BorderColorA = (0.0f);
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Input IR version must be OCL or SPIR");
    }
}

void CImagesBI::prepareColor(Value* Color)
{
    Value* TmpColor = Color;
    auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Color->getType());
    if (Color->getType()->getScalarType() != m_pFloatType)
    {
        // GenISA_typedwrite intrinsic expect to get the color as float,
        // therefore we do bitcast that should disappear in the final code.
        Type *CastTy = VTy ? IGCLLVM::FixedVectorType::get(m_pFloatType, static_cast<unsigned>(VTy->getNumElements())) : m_pFloatType;
        Instruction* tmp = BitCastInst::Create(
            Instruction::BitCast,
            Color,
            CastTy,
            "floatColor",
            m_pCallInst);
        tmp->setDebugLoc(m_DL);
        TmpColor = tmp;
    }

    Value* ColorX = VTy ? ExtractElementInst::Create(TmpColor, ConstantInt::get(m_pIntType, COORD_X), "ColorX", m_pCallInst) : TmpColor; // color x
    if (VTy)
    {
        cast<Instruction>(ColorX)->setDebugLoc(m_DL);
    }

    Value* ColorY = nullptr;
    Value* ColorZ = nullptr;
    Value* ColorW = nullptr;
    if (VTy)
    {
        unsigned NumElts = static_cast<unsigned>(VTy->getNumElements());
        IGC_ASSERT(NumElts == 2 || NumElts == 3 || NumElts == 4);
        if (NumElts >= 2)
        {
            ColorY = ExtractElementInst::Create(TmpColor, ConstantInt::get(m_pIntType, COORD_Y), "ColorY", m_pCallInst); // color y
            cast<Instruction>(ColorY)->setDebugLoc(m_DL);
        }
        if (NumElts >= 3)
        {
            ColorZ = ExtractElementInst::Create(TmpColor, ConstantInt::get(m_pIntType, COORD_Z), "ColorZ", m_pCallInst); // color z
            cast<Instruction>(ColorZ)->setDebugLoc(m_DL);
        }
        if (NumElts == 4)
        {
            ColorW = ExtractElementInst::Create(TmpColor, ConstantInt::get(m_pIntType, COORD_W), "ColorW", m_pCallInst); // color w
            cast<Instruction>(ColorW)->setDebugLoc(m_DL);
        }
    }

    Value *Undef = UndefValue::get(m_pFloatType);
    m_args.push_back(ColorX);
    m_args.push_back(ColorY ? ColorY : Undef);
    m_args.push_back(ColorZ ? ColorZ : Undef);
    m_args.push_back(ColorW ? ColorW : Undef);
}

void CImagesBI::prepareImageBTI()
{
    Value* pImg = nullptr;
    ConstantInt* imageIndex = CImagesUtils::getImageIndex(m_pParamMap, m_pCallInst, 0, pImg);
    IGC_ASSERT(isa<Argument>(pImg) || isa<LoadInst>(pImg));
    unsigned int addrSpace = EncodeAS4GFXResource(*imageIndex, RESOURCE);
    Value* img = ConstantPointerNull::get(PointerType::get(pImg->getType(), addrSpace));
    m_args.push_back(img); // BTI

    // prepareImageBTI() is called for standard image reads.  If there is an extension already tagged on
    // this argument then we have an inconsistent usage of the image and should fail compilation.
    if (m_pParamMap->count(pImg) != 0)
    {
        if ((*m_pParamMap)[pImg].extension != ResourceExtensionTypeEnum::NonExtensionType)
        {
            m_IncorrectBti = true;
        }
    }
}

void CImagesBI::preparePairedResource()
{
    if (m_pCodeGenContext->getModuleMetaData()->UseBindlessImage)
    {
        ConstantInt* bindlessIndex = ConstantInt::get(m_pIntType, BINDLESS_BTI);

        uint32_t addrSpace = EncodeAS4GFXResource(*bindlessIndex, RESOURCE);
        Type* ptrTy = llvm::PointerType::get(m_pFloatType, addrSpace);
        Value* pairedResource = UndefValue::get(ptrTy);
        m_args.push_back(pairedResource);
        return;
    }

    Value* pImg = nullptr;
    ConstantInt* imageIndex = CImagesUtils::getImageIndex(m_pParamMap, m_pCallInst, 0, pImg);
    IGC_ASSERT(isa<Argument>(pImg) || isa<LoadInst>(pImg));
    unsigned int addrSpace = EncodeAS4GFXResource(*imageIndex, RESOURCE);
    Type* ptrTy = llvm::PointerType::get(m_pFloatType, addrSpace);
    Value* pairedResource = UndefValue::get(ptrTy);
    m_args.push_back(pairedResource);
}

void CImagesBI::verifyCommand()
{
    if (m_IncorrectBti)
    {
        emitError("Inconsistent use of image!", NULL);
    }
}

Argument* CImagesBI::CImagesUtils::findImageFromBufferPtr(const MetaDataUtils& MdUtils, Function* F, BufferType bufType, uint64_t idx, const IGC::ModuleMetaData* modMD)
{
    if (modMD->FuncMD.find(F) != modMD->FuncMD.end())
    {
        FunctionMetaData funcMD = modMD->FuncMD.find(F)->second;
        ResourceAllocMD resAllocMD = funcMD.resAllocMD;
        IGC_ASSERT_MESSAGE(resAllocMD.argAllocMDList.size() > 0, "ArgAllocMDList is empty.");
        for (auto& arg : F->args())
        {
            unsigned argNo = arg.getArgNo();
            ArgAllocMD argAlloc = resAllocMD.argAllocMDList[argNo];
            ResourceTypeEnum argType = (ResourceTypeEnum)argAlloc.type;
            if (ResourceTypeMap(argType) == bufType &&
                argAlloc.indexType == idx)
                return &arg;
        }
    }

    return nullptr;
}

static bool isSYCLBindlessImageLoad(Value *v)
{
    auto *load = dyn_cast<LoadInst>(v);
    if (!load)
        return false;

    auto *ptrOp = load->getPointerOperand()->stripPointerCasts();
    if (!isa<Argument>(ptrOp) && !isa<GetElementPtrInst>(ptrOp) && !isa<AllocaInst>(ptrOp) && !isa<CallInst>(ptrOp))
        return false;

    return load->getType()->isIntegerTy(64);
};

ConstantInt* CImagesBI::CImagesUtils::getImageIndex(
    ParamMap* pParamMap,
    CallInst* pCallInst,
    unsigned int paramIndex,
    Value*& imageParam)
{
    ConstantInt* imageIndex = nullptr;

    imageParam = ValueTracker::track(pCallInst, paramIndex, nullptr, nullptr, isSYCLBindlessImageLoad);
    IGC_ASSERT(imageParam);
    IGC_ASSERT(isa<Argument>(imageParam) || isa<LoadInst>(imageParam));
    int i = (*pParamMap)[imageParam].index;
    imageIndex = ConstantInt::get(Type::getInt32Ty(pCallInst->getContext()), i);
    return imageIndex;
}

BufferType CImagesBI::CImagesUtils::getImageType(ParamMap* pParamMap, CallInst* pCallInst, unsigned int paramIndex)
{
    Value *imageParam = ValueTracker::track(pCallInst, paramIndex, nullptr, nullptr, isSYCLBindlessImageLoad);
    IGC_ASSERT(imageParam);
    IGC_ASSERT(isa<Argument>(imageParam) || isa<LoadInst>(imageParam));
    return isa<LoadInst>(imageParam) ? BufferType::BINDLESS : (*pParamMap)[imageParam].type;
}

void CImagesBI::createGetBufferPtr()
{
    //@TODO: Remove the part related to bindless once bindless
    //       is fully implemented outside this pass.
    if (m_pCodeGenContext->getModuleMetaData()->UseBindlessImage)
    {
        // If bindless image is preferred, don't create GetBufferPtr, instead just map the bindless pointer
        Value* pImg = m_pCallInst->getOperand(0);
        ConstantInt* bindlessIndex = ConstantInt::get(m_pIntType, BINDLESS_BTI);
        uint32_t addrSpace = EncodeAS4GFXResource(*bindlessIndex, BINDLESS);
        Type* ptrTy = llvm::PointerType::get(m_pFloatType, addrSpace);

        Value* basePointer = isa<IntegerType>(pImg->getType()) ?
            BitCastInst::CreateBitOrPointerCast(pImg, ptrTy, "bindless_img", m_pCallInst) :
            BitCastInst::CreatePointerCast(pImg, ptrTy, "bindless_img", m_pCallInst);
        m_args.push_back(basePointer);
        return;
    }

    Value* pImg = nullptr;
    ConstantInt* imageIndex = CImagesUtils::getImageIndex(m_pParamMap, m_pCallInst, 0, pImg);
    BufferType bufType = CImagesUtils::getImageType(m_pParamMap, m_pCallInst, 0);
    unsigned int addressSpace = IGC::EncodeAS4GFXResource(*imageIndex, bufType);
    Type* ptrTy = llvm::PointerType::get(m_pFloatType, addressSpace);

    Function* pFuncGetBufferPtr = getFunctionDeclaration(GenISAIntrinsic::GenISA_GetBufferPtr, ptrTy);

    //%base_ptr = call float* @llvm.GenISA.GetBufferPtr(i32 %bufIdx, i32 %type)

    // preparing the argumant list for the function
    llvm::SmallVector<Value*, 2> getBufferPtrArgs;
    getBufferPtrArgs.push_back(imageIndex);
    getBufferPtrArgs.push_back(ConstantInt::get(m_pIntType, bufType));

    CallInst* pDstBuffer = CallInst::Create(
        pFuncGetBufferPtr,
        getBufferPtrArgs,
        m_pCallInst->getName(),
        m_pCallInst);
    pDstBuffer->setDebugLoc(m_DL);
    m_args.push_back(pDstBuffer);

    // createGetBufferPtr() is called for standard writes reads.  If there is an extension already tagged on
    // this argument then we have an inconsistent usage of the image and should fail compilation.
    if (m_pParamMap->count(pImg) != 0)
    {
        if ((*m_pParamMap)[pImg].extension != ResourceExtensionTypeEnum::NonExtensionType)
        {
            m_IncorrectBti = true;
        }
    }
}

inline bool  isInteger(APFloat& val)
{
    // This could be made more efficient; I'm going for obviously correct.
    if (!val.isFinite())
        return false;
    APFloat truncated = val;
    truncated.roundToIntegral(APFloat::rmTowardZero);
    return val.compare(truncated) == APFloat::cmpEqual;
}

bool CImagesBI::derivedFromInt(const Value* pVal)
{
    SmallPtrSet<const Value*, 16> visitedPHIs;

    std::function<bool(const Value*)> go = [&](const Value* val)
    {
        if (val->getType()->getScalarType()->isIntegerTy() || isa<UndefValue>(val))
        {
            return true;
        }
        else if (auto * constVal = dyn_cast<ConstantFP>(val))
        {
            APFloat val = constVal->getValueAPF();
            return isInteger(val);
        }
        else if (auto * inst = dyn_cast<Instruction>(val))
        {
            switch (inst->getOpcode())
            {
            case Instruction::PHI:
            {
                // return "true" if we have visited this PHI before.
                for (const Value* phi : visitedPHIs)
                {
                    if (phi == inst)
                        return true;
                }
                visitedPHIs.insert(inst);
                for (uint i = 0, numOpnds = inst->getNumOperands(); i < numOpnds; i++)
                {
                    if (!go(inst->getOperand(i)))
                        return false;
                }
                visitedPHIs.erase(inst);
                return true;
            }

            case Instruction::Select:
            {
                auto* sel = cast<SelectInst>(inst);
                return (go(sel->getTrueValue()) && go(sel->getFalseValue()));
            }

            case Instruction::InsertElement:
            {
                return (go(inst->getOperand(0)) && go(inst->getOperand(1)));
            }

            case Instruction::ExtractElement:
            {
                auto* extrElem = cast<ExtractElementInst>(inst);
                return (go(extrElem->getVectorOperand()));
            }

            case Instruction::UIToFP:
            case Instruction::SIToFP:
                return true;

            default:
                return false;
            }
        }
        else
        {
            return false;
        }
    };

    return go(pVal);
}

void CImagesBI::prepareTypedReadArgs()
{
    createGetBufferPtr();
    m_args.push_back(CoordX); // u
    m_args.push_back(CoordY); // v
    m_args.push_back(CoordZ); // r
    m_args.push_back(m_pCallInst->getOperand(2)); // LOD
}

void CImagesBI::replaceGenISATypedRead()
{
    GenISAIntrinsic::ID intrinsicName = GenISAIntrinsic::GenISA_typedread;
    Type* Tys[] = {m_args[0]->getType()};

    Function* func = getFunctionDeclaration(intrinsicName, Tys);
    Instruction* newCall = CallInst::Create(func, m_args, m_pCallInst->getName(), m_pCallInst);
    if (m_pCallInst->getType()->getScalarType() != m_pFloatType)
    {
        // GenISA_typedread intrinsic returns <4 x float>
        // therefore we do bitcast that should disappear in the final code.
        Type *CastTy = m_pCallInst->getType();
        Instruction* tmp = BitCastInst::Create(
            Instruction::BitCast,
            newCall,
            CastTy,
            "",
            m_pCallInst);
        tmp->setDebugLoc(m_DL);
        newCall = tmp;
    }
    newCall->setDebugLoc(m_DL);
    m_pCallInst->replaceAllUsesWith(newCall);
}

class COCL_sample : public CImagesBI
{
public:
    COCL_sample(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim, MetaDataUtils* pMdUtils, ModuleMetaData* modMD) : CImagesBI(paramMap, inlineMap, nextSampler, Dim), m_pMdUtils(pMdUtils), m_modMD(modMD) {}

    Value* getSamplerValue(void)
    {
        auto modMD = m_pCodeGenContext->getModuleMetaData();
        //@TODO: Remove the part related to bindless once bindless
        //       is fully implemented outside this pass.
        if (modMD->UseBindlessImage)
        {
            Value* sampler;
            if (modMD->UseBindlessImageWithSamplerTracking)
            {
                Value* samplerValue = ValueTracker::track(m_pCallInst, 1, m_pMdUtils, m_modMD);
                int samplerConstantVal = int_cast<int>(cast<ConstantInt>(samplerValue)->getZExtValue());

                // Bindless inline sampler is passed via implicit kernel argument.
                ImplicitArgs implicitArgs(*m_pFunc, m_pMdUtils);
                sampler = implicitArgs.getNumberedImplicitArg(*m_pFunc, ImplicitArg::INLINE_SAMPLER, samplerConstantVal);
            }
            else
            {
                sampler = m_pCallInst->getArgOperand(1);
            }

            // Map the bindless pointer.
            ConstantInt* bindlessIndex = ConstantInt::get(m_pIntType, BINDLESS_BTI);
            unsigned int addressSpace = IGC::EncodeAS4GFXResource(*bindlessIndex, BufferType::BINDLESS_SAMPLER);
            Type* ptrTy = llvm::PointerType::get(m_pFloatType, addressSpace);
            Value* bindlessSampler = isa<IntegerType>(sampler->getType()) ?
                BitCastInst::CreateBitOrPointerCast(sampler, ptrTy, "bindless_sampler", m_pCallInst) :
                BitCastInst::CreatePointerCast(sampler, ptrTy, "bindless_sampler", m_pCallInst);
            return bindlessSampler;
        }
        ConstantInt* samplerIndex = nullptr;
        auto isBindlessSampler = [](Value *v)
        {
            // Bindless sampler is computed in ResolveSampledImageBuiltins pass.
            if (auto *I = dyn_cast<BinaryOperator>(v))
            {
                if (I->getOpcode() != BinaryOperator::Or || !I->getType()->isIntegerTy(64))
                    return false;
                if (auto *C = dyn_cast<ConstantInt>(I->getOperand(1)))
                {
                    return C->isOne();
                }
            }
            return false;
        };

        Value* samplerValue = ValueTracker::track(m_pCallInst, 1, m_pMdUtils, m_modMD, isBindlessSampler);
        if (!samplerValue) {
            emitError("There are instructions that use a sampler, but no sampler found in the kernel!", m_pCallInst);
            return nullptr;
        }

        auto addToInlineSamplersMD = [&](int samplerConstantVal)
        {
            // Is this sampler already allocated?
            auto iter = m_pInlineMap->find(samplerConstantVal);
            if (iter != m_pInlineMap->end())
            {
                return ConstantInt::get(m_pIntType, iter->second);
            }

            // No, allocate it.
            int currSamplerIdx = (*m_pNextSampler)++;
            (*m_pInlineMap)[samplerConstantVal] = currSamplerIdx;
            samplerIndex = ConstantInt::get(m_pIntType, currSamplerIdx);

            // Push this information into the metadata, for the state processor's benefit
            FunctionMetaData& funcMD = m_modMD->FuncMD[m_pFunc];
            ResourceAllocMD& resAllocMD = funcMD.resAllocMD;
            InlineSamplersMD inlineSamplerMD;
            CreateInlineSamplerAnnotations(m_pCallInst->getModule(), inlineSamplerMD, samplerConstantVal);
            inlineSamplerMD.index = currSamplerIdx;
            resAllocMD.inlineSamplersMD.push_back(inlineSamplerMD);
            m_pMdUtils->save(*m_pCtx);
            return samplerIndex;
        };

        // Argument samplers are looked up in the parameter map
        if (isa<Argument>(samplerValue))
        {
            int i = (*m_pParamMap)[samplerValue].index;
            samplerIndex = ConstantInt::get(m_pIntType, i);
            return samplerIndex;
        }

        // The sampler is not an argument, make sure it's a constant
        IGC_ASSERT_MESSAGE(isa<ConstantInt>(samplerValue), "Sampler must be a global variable or a constant");
        auto *samplerConstant = cast<ConstantInt>(samplerValue);
        int samplerConstantVal = int_cast<int>(samplerConstant->getZExtValue());

        return addToInlineSamplersMD(samplerConstantVal);
    }

    bool prepareSamplerValue()
    {
        Value* samplerValue = getSamplerValue();
        if (!samplerValue) return false;
        if (isa<ConstantInt>(samplerValue))
        {
            unsigned int addrSpace = EncodeAS4GFXResource(*samplerValue, SAMPLER);
            samplerValue = ConstantPointerNull::get(PointerType::get(samplerValue->getType(), addrSpace));
        }

        m_args.push_back(samplerValue);
        return true;
    }

    void prepareGradients(Dimension Dim, Value* gradX, Value* gradY)
    {
        m_gradXY = m_gradXZ = m_gradYY = m_gradYZ = m_pFloatZero;
        m_gradXX = gradX;
        m_gradYX = gradY;
        Instruction* tmp;
        switch (Dim)
        {
        case DIM_3D:
            tmp = ExtractElementInst::Create(
                gradX,
                ConstantInt::get(m_pIntType, COORD_Z),
                "gradXZ",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradXZ = tmp;
            tmp = ExtractElementInst::Create(
                gradY,
                ConstantInt::get(m_pIntType, COORD_Z),
                "gradYZ",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradYZ = tmp;
            // fall through
        case DIM_2D:
        case DIM_2D_ARRAY:
            tmp = ExtractElementInst::Create(
                gradX,
                ConstantInt::get(m_pIntType, COORD_Y),
                "gradXY",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradXY = tmp;
            tmp = ExtractElementInst::Create(
                gradY,
                ConstantInt::get(m_pIntType, COORD_Y),
                "gradYY",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradYY = tmp;
            tmp = ExtractElementInst::Create(
                gradX,
                ConstantInt::get(m_pIntType, COORD_X),
                "gradXX",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradXX = tmp;
            tmp = ExtractElementInst::Create(
                gradY,
                ConstantInt::get(m_pIntType, COORD_X),
                "gradYX",
                m_pCallInst);
            tmp->setDebugLoc(m_DL);
            m_gradYX = tmp;
            // fall through
        case DIM_1D:
        case DIM_1D_ARRAY:
            //  no need to extract since in 1 Dim gradient is not a vector.
            break;
        case DIM_UNKNOWN:
            emitError("Unexpected dimension", NULL);
            break;
        }
    }

protected:
    MetaDataUtils* m_pMdUtils = nullptr;
    ModuleMetaData* m_modMD = nullptr;
    Value* m_gradXX = nullptr;
    Value* m_gradXY = nullptr;
    Value* m_gradXZ = nullptr;
    Value* m_gradYX = nullptr;
    Value* m_gradYY = nullptr;
    Value* m_gradYZ = nullptr;
};

class COCL_sample_l : public COCL_sample
{
public:
    COCL_sample_l(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim, MetaDataUtils* pMdUtils, ModuleMetaData* modMD) : COCL_sample(paramMap, inlineMap, nextSampler, Dim, pMdUtils, modMD) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(2);
        m_args.push_back(m_pCallInst->getOperand(3)); // lod
        prepareCoords(m_dim, Coord, m_pFloatZero);
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_pFloatZero); // ai (?)
        preparePairedResource();
        createGetBufferPtr();
        bool samplerValueFound = prepareSamplerValue();
        if (!samplerValueFound) return;

        prepareZeroOffsets();
        Type* types[] = {
            m_pCallInst->getType(),
            m_pFloatType,
            m_args[5]->getType(),
            m_args[6]->getType(),
            m_args[7]->getType(),
        };
        replaceGenISACallInst(GenISAIntrinsic::GenISA_sampleLptr, types);
    }
};

class COCL_sample_d : public COCL_sample
{
public:
    COCL_sample_d(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim, MetaDataUtils* pMdUtils, ModuleMetaData* modMD) : COCL_sample(paramMap, inlineMap, nextSampler, Dim, pMdUtils, modMD) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(2);
        prepareCoords(m_dim, Coord, m_pFloatZero);
        prepareGradients(m_dim, m_pCallInst->getOperand(3), m_pCallInst->getOperand(4));
        m_args.push_back(CoordX);
        m_args.push_back(m_gradXX);
        m_args.push_back(m_gradYX);
        m_args.push_back(CoordY);
        m_args.push_back(m_gradXY);
        m_args.push_back(m_gradYY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_gradXZ);
        m_args.push_back(m_gradYZ);
        m_args.push_back(m_pFloatZero); // ai (?)
        m_args.push_back(m_pFloatZero); // minLOD (?)
        preparePairedResource();
        prepareImageBTI();
        prepareSamplerValue();
        prepareZeroOffsets();
        Type* types[] = {
            m_pCallInst->getType(),
            m_pFloatType,
            m_args[11]->getType(),
            m_args[12]->getType(),
            m_args[13]->getType(),
        };
        replaceGenISACallInst(GenISAIntrinsic::GenISA_sampleDptr, types);
    }
};

class COCL_ldui : public CImagesBI
{
public:
    COCL_ldui(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(m_pCallInst->getOperand(2)); // LOD
        m_args.push_back(CoordZ);
        preparePairedResource();
        createGetBufferPtr();
        prepareZeroOffsets();
        Type* types[] = {
            m_pCallInst->getType(),
            m_args[4]->getType(),
            m_args[5]->getType(),
        };
        replaceGenISACallInst(GenISAIntrinsic::GenISA_ldptr, types);
    }
};

class COCL_ldui_rw : public CImagesBI
{
public:
    COCL_ldui_rw(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        prepareTypedReadArgs();
        replaceGenISATypedRead();
    }
};

class COCL_ld : public CImagesBI
{
public:
    COCL_ld(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(m_pCallInst->getOperand(2)); // LOD
        m_args.push_back(CoordZ);
        preparePairedResource();
        createGetBufferPtr();
        prepareZeroOffsets();
        Type* types[] = {
            IGCLLVM::FixedVectorType::get(m_pFloatType, 4),
            m_args[4]->getType(),
            m_args[5]->getType(),
        };
        replaceGenISACallInst(GenISAIntrinsic::GenISA_ldptr, types);
    }
};

class COCL_ld_rw : public CImagesBI
{
public:
    COCL_ld_rw(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        prepareTypedReadArgs();
        replaceGenISATypedRead();
    }
};

class COCL_ldmcs : public CImagesBI
{
public:
    COCL_ldmcs(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_pIntZero);   // LOD
        prepareImageBTI();
        prepareZeroOffsets();
        Type* types[] = { IGCLLVM::FixedVectorType::get(m_pFloatType, 4), m_pIntType, m_args[7]->getType() };
        replaceGenISACallInst(GenISAIntrinsic::GenISA_ldmcsptr, types);
    }
};

class COCL_ld2dms_base : public CImagesBI
{
public:
    COCL_ld2dms_base(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

protected:

    // adds mcs information to the arguments.
    // mcs is received as a float vector, from which the 2 lower element need
    // to be extracted, bitcast to i32, and then pushed to the arguments list.
    void prepareMCS(Value* mcs)
    {
        Instruction* fmcsl = ExtractElementInst::Create(mcs, m_pIntZero, "mcsl", m_pCallInst);
        Instruction* fmcsh = ExtractElementInst::Create(mcs, m_pIntOne, "mcsh", m_pCallInst);
        Instruction* imcsl = BitCastInst::Create(Instruction::BitCast, fmcsl, m_pIntType, "imcsl", m_pCallInst);
        Instruction* imcsh = BitCastInst::Create(Instruction::BitCast, fmcsh, m_pIntType, "imcsh", m_pCallInst);
        fmcsl->setDebugLoc(m_DL);
        fmcsh->setDebugLoc(m_DL);
        imcsl->setDebugLoc(m_DL);
        imcsh->setDebugLoc(m_DL);

        m_args.push_back(imcsl);
        m_args.push_back(imcsh);
    }
};

class COCL_ld2dms : public COCL_ld2dms_base
{
public:
    COCL_ld2dms(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : COCL_ld2dms_base(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(m_pCallInst->getOperand(2));   // sample index
        prepareMCS(m_pCallInst->getOperand(3));
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_pIntZero); // LOD
        prepareImageBTI();
        prepareZeroOffsets();
        replaceGenISACallInst(GenISAIntrinsic::GenISA_ldmsptr, { IGCLLVM::FixedVectorType::get(m_pIntType, 4), m_args[7]->getType() });
    }
};

class COCL_ld2dmsui : public COCL_ld2dms_base
{
public:
    COCL_ld2dmsui(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : COCL_ld2dms_base(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(m_pCallInst->getOperand(2));   // sample index
        prepareMCS(m_pCallInst->getOperand(3));
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_pIntZero); // LOD
        prepareImageBTI();
        prepareZeroOffsets();
        replaceGenISACallInst(GenISAIntrinsic::GenISA_ldmsptr, { IGCLLVM::FixedVectorType::get(m_pIntType, 4), m_args[7]->getType() });
    }
};

class CWrite : public CImagesBI
{
public:
    CWrite(ParamMap* paramMap, InlineMap* inlineMap, int* nextSampler, Dimension Dim) : CImagesBI(paramMap, inlineMap, nextSampler, Dim) {}

    void createIntrinsic()
    {
        Value* Coord = m_pCallInst->getOperand(1);
        Value* Color = m_pCallInst->getOperand(2);
        createGetBufferPtr();
        prepareCoords(m_dim, Coord, m_pIntZero);
        m_args.push_back(CoordX);
        m_args.push_back(CoordY);
        m_args.push_back(CoordZ);
        m_args.push_back(m_pCallInst->getOperand(3)); // LOD
        prepareColor(Color);
        replaceGenISACallInst(GenISAIntrinsic::GenISA_typedwrite, llvm::ArrayRef<llvm::Type*>(m_args[0]->getType()));
    }
};

class CGetImageProperty : public CImagesBI
{
    // Image property that needs to be recieved
    const ImageProperty m_Prop;
    CGetImageProperty() = delete;
    CGetImageProperty(ParamMap* paramMap, ImageProperty prop) : CImagesBI(paramMap), m_Prop(prop) {}

public:
    static std::unique_ptr<CGetImageProperty> create(ParamMap* paramMap, ImageProperty prop)
    {
        return std::unique_ptr<CGetImageProperty>(new CGetImageProperty(paramMap, prop));
    }

    void createIntrinsic()
    {
        createGetBufferPtr();
        LLVM3DBuilder<> builder(*m_pCtx, m_pCodeGenContext->platform.getPlatformInfo());
        builder.SetInsertPoint(m_pCallInst);
        Value* info = builder.Create_resinfo(builder.getInt32(0), m_args[0]);
        Value* prop = nullptr;
        switch (m_Prop)
        {
        case WIDTH:
            prop = builder.CreateExtractElement(info, builder.getInt32(0));
            break;
        case HEIGHT:
        case ARRAY_SIZE_1D:
            prop = builder.CreateExtractElement(info, builder.getInt32(1));
            break;
        case DEPTH:
        case ARRAY_SIZE_2D:
            prop = builder.CreateExtractElement(info, builder.getInt32(2));
            break;
        default:
            emitError("Unsupported bindless image property requested.", NULL);
        }
        m_pCallInst->replaceAllUsesWith(prop);
    }
};

class CSimpleIntrinMapping : public CCommand
{
    // id - ID of the intrinsic to be replaced by the call
    const GenISAIntrinsic::ID isaId;

    const IGCLLVM::Intrinsic id;
    // isOverloadable - true if the mapped intrinsic is over-loadable
    const bool isOverloadable;
protected:
    void createIntrinsicType(const CallInst* pCI, ArrayRef<Type*> overloadTypes)
    {
        m_args.append(pCI->op_begin(), pCI->op_begin() + IGCLLVM::getNumArgOperands(pCI));
        IGC_ASSERT_MESSAGE(!(id != Intrinsic::num_intrinsics && isaId != GenISAIntrinsic::ID::num_genisa_intrinsics), "Both intrinsic id's cannot be valid at the same time");

        // GenISA intrinsics ID start after llvm intrinsics
        if (isaId != GenISAIntrinsic::num_genisa_intrinsics && isaId > static_cast<int>(Intrinsic::num_intrinsics))
        {
            replaceGenISACallInst(isaId, this->isOverloadable ? overloadTypes : llvm::ArrayRef<Type*>());
        }
        else
        {
            replaceCallInst(id, this->isOverloadable ? overloadTypes : llvm::ArrayRef<Type*>());
        }
    }
public:
    CSimpleIntrinMapping(GenISAIntrinsic::ID intrinsicId, bool isOverloadable)
        : isaId(intrinsicId), id(Intrinsic::num_intrinsics), isOverloadable(isOverloadable) {}

    CSimpleIntrinMapping(IGCLLVM::Intrinsic intrinsicId, bool isOverloadable)
        : isaId(GenISAIntrinsic::ID::num_genisa_intrinsics), id(intrinsicId), isOverloadable(isOverloadable)
    {}

    static std::unique_ptr<CSimpleIntrinMapping> create(IGCLLVM::Intrinsic intrinsicId, bool isOverloadable = true)
    {
        return std::unique_ptr<CSimpleIntrinMapping>(new CSimpleIntrinMapping(intrinsicId, isOverloadable));
    }

    static std::unique_ptr<CSimpleIntrinMapping> create(GenISAIntrinsic::ID intrinsicId, bool isOverloadable = true)
    {
        return std::unique_ptr<CSimpleIntrinMapping>(new CSimpleIntrinMapping(intrinsicId, isOverloadable));
    }

    void createIntrinsic()
    {
        IGC_ASSERT_MESSAGE(!(this->isOverloadable && IGCLLVM::getNumArgOperands(m_pCallInst) == 0), "Cannot create an overloadable with no args");
        llvm::Type* tys[2];
        switch (isaId)
        {
        case GenISAIntrinsic::GenISA_ftoi_rtn:
        case GenISAIntrinsic::GenISA_ftoi_rtp:
        case GenISAIntrinsic::GenISA_ftoi_rte:
        case GenISAIntrinsic::GenISA_ftoui_rtn:
        case GenISAIntrinsic::GenISA_ftoui_rtp:
        case GenISAIntrinsic::GenISA_ftoui_rte:
        case GenISAIntrinsic::GenISA_ftof_rtn:
        case GenISAIntrinsic::GenISA_ftof_rtp:
        case GenISAIntrinsic::GenISA_ftof_rtz:
        case GenISAIntrinsic::GenISA_uitof_rtn:
        case GenISAIntrinsic::GenISA_uitof_rtp:
        case GenISAIntrinsic::GenISA_uitof_rtz:
        case GenISAIntrinsic::GenISA_itof_rtn:
        case GenISAIntrinsic::GenISA_itof_rtp:
        case GenISAIntrinsic::GenISA_itof_rtz:
            tys[0] = m_pCallInst->getCalledFunction()->getReturnType();
            tys[1] = m_pCallInst->getArgOperand(0)->getType();
            m_args.append(m_pCallInst->op_begin(), m_pCallInst->op_begin() + IGCLLVM::getNumArgOperands(m_pCallInst));
            IGC_ASSERT_MESSAGE(!(id != Intrinsic::num_intrinsics && isaId != GenISAIntrinsic::ID::num_genisa_intrinsics), "Both intrinsic id's cannot be valid at the same time");
            replaceGenISACallInst(isaId, llvm::ArrayRef<llvm::Type*>(tys));
            break;
        case GenISAIntrinsic::GenISA_simdGetMessagePhase:
        case GenISAIntrinsic::GenISA_broadcastMessagePhase:
        case GenISAIntrinsic::GenISA_createMessagePhasesV:
        case GenISAIntrinsic::GenISA_createMessagePhasesNoInitV:
        case GenISAIntrinsic::GenISA_getMessagePhaseX:
        case GenISAIntrinsic::GenISA_setMessagePhaseV:
            createIntrinsicType(m_pCallInst, m_pCallInst->getCalledFunction()->getReturnType());
            break;
        case GenISAIntrinsic::GenISA_simdSetMessagePhase:
        case GenISAIntrinsic::GenISA_setMessagePhaseX:
        case GenISAIntrinsic::GenISA_setMessagePhaseX_legacy:
            createIntrinsicType(m_pCallInst, m_pCallInst->getArgOperand(IGCLLVM::getNumArgOperands(m_pCallInst) - 1)->getType());
            break;
        case GenISAIntrinsic::GenISA_broadcastMessagePhaseV:
        case GenISAIntrinsic::GenISA_simdGetMessagePhaseV:
        case GenISAIntrinsic::GenISA_getMessagePhaseXV:
        {
            Type* overloadTypes[] =
            {
                m_pCallInst->getCalledFunction()->getReturnType(),
                m_pCallInst->getArgOperand(0)->getType()
            };
            createIntrinsicType(m_pCallInst, overloadTypes);
            break;
        }
        case GenISAIntrinsic::GenISA_simdSetMessagePhaseV:
        case GenISAIntrinsic::GenISA_setMessagePhaseXV:
        {
            Type* overloadTypes[] =
            {
                m_pCallInst->getCalledFunction()->getReturnType(),
                m_pCallInst->getArgOperand(IGCLLVM::getNumArgOperands(m_pCallInst) - 1)->getType()
            };
            createIntrinsicType(m_pCallInst, overloadTypes);
            break;
        }
        default:
        {
            auto* pOverloadType = (!this->isOverloadable) ?
                nullptr :
                m_pCallInst->getArgOperand(0)->getType();
            createIntrinsicType(m_pCallInst, pOverloadType);
            break;
        }
        }
    }
};

class CWaveBallotIntrinsic : public CCommand
{
private:
    // id - ID of the intrinsic to be replaced by the call
    const GenISAIntrinsic::ID isaId;
    const IGCLLVM::Intrinsic id;

public:
    CWaveBallotIntrinsic(GenISAIntrinsic::ID intrinsicId)
        : isaId(intrinsicId), id(Intrinsic::num_intrinsics) {}

    CWaveBallotIntrinsic(IGCLLVM::Intrinsic intrinsicId, bool isOverloadable)
        : isaId(GenISAIntrinsic::ID::num_genisa_intrinsics), id(intrinsicId) {}

    static std::unique_ptr<CWaveBallotIntrinsic> create(GenISAIntrinsic::ID intrinsicId)
    {
        return std::unique_ptr<CWaveBallotIntrinsic>(new CWaveBallotIntrinsic(intrinsicId));
    }

    void createIntrinsic()
    {
        IGCLLVM::IRBuilder<> IRB(m_pCallInst);

        Value* pSrc = m_pCallInst->getArgOperand(0);
        Value* truncInst = IRB.CreateTrunc(pSrc, IRB.getInt1Ty());

        if (llvm::Instruction * inst = dyn_cast<llvm::Instruction>(truncInst))
        {
            inst->setDebugLoc(m_DL);
        }

        m_args.push_back(truncInst);

        if (isaId == GenISAIntrinsic::GenISA_WaveClusteredBallot)
        {
            m_args.push_back(m_pCallInst->getArgOperand(1));
        }

        m_args.push_back(IRB.getInt32(0));
        replaceGenISACallInst(isaId);
    }
};

class CVMESend : public CCommand
{
    // id - ID of the intrinsic to be replaced by the call
    const GenISAIntrinsic::ID id;

    // m_pParamMap - maps image and sampler kernel parameters to BTIs
    //               and sampler array indexes, respecitvely
    CImagesBI::ParamMap* m_pParamMap;

    const uint num_images;

public:
    CVMESend(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap, uint num_images)
        : id(intrinsicId), m_pParamMap(paramMap), num_images(num_images) {}

    static std::unique_ptr<CVMESend> create(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap, uint num_images = 2)
    {
        return std::unique_ptr<CVMESend>(new CVMESend(intrinsicId, paramMap, num_images));
    }

    void createIntrinsic()
    {
        // First 3 params
        m_args.append(m_pCallInst->op_begin(), m_pCallInst->op_begin() + 3);

        // Push images like src image, fwd ref image, and bwd ref image.
        IGC_ASSERT((num_images >= 2) && (num_images <= 3));
        for (uint i = 0; i < num_images; i++) {
            Value* pImg = nullptr;
            m_args.push_back(CImagesBI::CImagesUtils::getImageIndex(m_pParamMap, m_pCallInst, i + 3, pImg));
        }

        // Rest of the params
        m_args.append(m_pCallInst->op_begin() + 3 + num_images, m_pCallInst->op_begin() + IGCLLVM::getNumArgOperands(m_pCallInst));

        replaceGenISACallInst(id);
    }
};

class CNewVMESend : public CCommand
{
    // id - ID of the intrinsic to be replaced by the call
    const GenISAIntrinsic::ID id;

    // m_pParamMap - maps image and sampler kernel parameters to BTIs
    //               and sampler array indexes, respectively
    CImagesBI::ParamMap* m_pParamMap;

    // Number of image arguments
    const int m_numImgArgs;
    const bool m_IsOverLoaded;

    CodeGenContext* m_Ctx;

public:
    CNewVMESend(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap, CodeGenContext* pCtx, bool isOverloaded, int numImgArgs)
        : id(intrinsicId), m_pParamMap(paramMap), m_numImgArgs(numImgArgs), m_IsOverLoaded(isOverloaded), m_Ctx(pCtx) {}

    static std::unique_ptr<CNewVMESend> create(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap, CodeGenContext* pCtx, bool isOverloaded, int numImgArgs = 3)
    {
        return std::unique_ptr<CNewVMESend>(new CNewVMESend(intrinsicId, paramMap, pCtx, isOverloaded, numImgArgs));
    }

    void createIntrinsic()
    {
        // First 2 params
        m_args.append(m_pCallInst->op_begin(), m_pCallInst->op_begin() + 1);

        // Push images like src image, fwd ref image, and bwd ref image.
        for (int i = 0; i < m_numImgArgs; i++) {
            Value* pImg = nullptr;
            m_args.push_back(CImagesBI::CImagesUtils::getImageIndex(m_pParamMap, m_pCallInst, i + 1, pImg));
        }

        // Rest of the params except for the accelerator sampler.
        m_args.append(m_pCallInst->op_begin() + 1 + m_numImgArgs + 1, m_pCallInst->op_begin() + IGCLLVM::getNumArgOperands(m_pCallInst));

        // Device-side VME using the CNewVMESend always use inline samplers.
        IGC::ModuleMetaData* MD = m_Ctx->getModuleMetaData();
        auto& funcInfo = MD->FuncMD[m_pCallInst->getParent()->getParent()];
        funcInfo.hasInlineVmeSamplers = true;

        switch (id)
        {
        case GenISAIntrinsic::GenISA_vmeSendIME2:
        {
            Type* Tys[] =
            {
                m_pCallInst->getCalledFunction()->getReturnType(),
                m_pCallInst->getArgOperand(0)->getType()
            };
            replaceGenISACallInst(id, Tys);
            break;
        }
        default:
            replaceGenISACallInst(id, m_IsOverLoaded ? ArrayRef<Type*>(m_pCallInst->getArgOperand(0)->getType()) : llvm::ArrayRef<Type*>());
            break;
        }
    }
};

class CVASend : public CCommand
{
    // m_id - ID of the intrinsic to be replaced by the call
    const GenISAIntrinsic::ID m_id;

    // m_pParamMap - maps image and sampler kernel parameters to BTIs
    //               and sampler array indexes, respecitvely
    CImagesBI::ParamMap* m_pParamMap;

public:
    CVASend(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap) : m_id(intrinsicId), m_pParamMap(paramMap) {}

    static std::unique_ptr<CVASend> create(GenISAIntrinsic::ID intrinsicId, CImagesBI::ParamMap* paramMap)
    {
        return std::unique_ptr<CVASend>(new CVASend(intrinsicId, paramMap));
    }

    void createIntrinsic()
    {
        m_args.append(m_pCallInst->op_begin(), m_pCallInst->op_end() - 1);

        replaceGenISACallInst(m_id);
    }
};

class CMemCpy : public CCommand
{
public:
    static std::unique_ptr<CMemCpy> create()
    {
        return std::unique_ptr<CMemCpy>(new CMemCpy());
    }

    void createIntrinsic()
    {
        Value* pDst = m_pCallInst->getArgOperand(0);
        Value* pSrc = m_pCallInst->getArgOperand(1);
        Value* pNumBytes = m_pCallInst->getArgOperand(2);
        uint64_t Align = cast<ConstantInt>(m_pCallInst->getArgOperand(3))->getZExtValue();

        IGCLLVM::IRBuilder<> IRB(m_pCallInst);

        Value* pDsti8 = IRB.CreateBitCast(
            pDst, IRB.getInt8PtrTy(cast<PointerType>(pDst->getType())->getAddressSpace()));

        Value* pSrci8 = IRB.CreateBitCast(
            pSrc, IRB.getInt8PtrTy(cast<PointerType>(pSrc->getType())->getAddressSpace()));

        CallInst* pCall = IRB.CreateMemCpy(pDsti8, pSrci8, pNumBytes, int_cast<unsigned int>(Align));
        pCall->setDebugLoc(m_DL);
        m_pCallInst->replaceAllUsesWith(pCall);
    }
};

class CSamplePos : public CCommand
{
public:
    static std::unique_ptr<CSamplePos> create()
    {
        return std::unique_ptr<CSamplePos>(new CSamplePos());
    }

    void createIntrinsic()
    {
        // WA, platform is not really needed in that case and we cannot access it
        PLATFORM platform;
        LLVM3DBuilder<> builder(m_pCallInst->getContext(), platform);
        builder.SetInsertPoint(m_pCallInst);
        Value* textureOperand = m_pCallInst->getOperand(0);
        Value* sampleIndex = m_pCallInst->getOperand(1);
        Value* samplePosPackedValues = builder.Create_SamplePos(textureOperand, sampleIndex);
        cast<llvm::Instruction>(samplePosPackedValues)->setDebugLoc(m_DL);
        m_pCallInst->replaceAllUsesWith(samplePosPackedValues);
    }
};

template <typename T>
std::unique_ptr<T> initImageClass(CImagesBI::ParamMap* paramMap, CImagesBI::InlineMap* inlineMap,
    int* nextSampler, CImagesBI::Dimension dim)
{
    return std::unique_ptr<T>(new T(paramMap, inlineMap, nextSampler, dim));
}

template <typename T>
std::unique_ptr<T> initSamplerClass(CImagesBI::ParamMap* paramMap, CImagesBI::InlineMap* inlineMap,
    int* nextSampler, CImagesBI::Dimension dim, MetaDataUtils* pMdUtils, ModuleMetaData* modMD)
{
    return std::unique_ptr<T>(new T(paramMap, inlineMap, nextSampler, dim, pMdUtils, modMD));
}

CBuiltinsResolver::CBuiltinsResolver(CImagesBI::ParamMap* paramMap, CImagesBI::InlineMap* inlineMap, int* nextSampler, CodeGenContext* ctx) : m_CodeGenContext(ctx)
{
    MetaDataUtils* pMdUtils = ctx->getMetaDataUtils();
    ModuleMetaData* modMD = ctx->getModuleMetaData();
    // Images Built-ins
    // Read_Image builtins
    m_CommandMap["__builtin_IB_OCL_1d_ldui"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_OCL_1darr_ldui"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ldui"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ldui"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_3d_ldui"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_OCL_1d_ld"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_OCL_1darr_ld"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ld"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ld"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_3d_ld"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);

    m_CommandMap["__builtin_IB_OCL_1d_ldui_ro"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_OCL_1darr_ldui_ro"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ldui_ro"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ldui_ro"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_3d_ldui_ro"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_OCL_1d_ld_ro"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_OCL_1darr_ld_ro"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ld_ro"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ld_ro"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_3d_ld_ro"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);

    if (m_CodeGenContext->platform.supportsBGRATypedRead())
    {
        m_CommandMap["__builtin_IB_OCL_1d_ldui_rw"] = initImageClass<COCL_ldui_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
        m_CommandMap["__builtin_IB_OCL_1darr_ldui_rw"] = initImageClass<COCL_ldui_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_2d_ldui_rw"] = initImageClass<COCL_ldui_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
        m_CommandMap["__builtin_IB_OCL_2darr_ldui_rw"] = initImageClass<COCL_ldui_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_3d_ldui_rw"] = initImageClass<COCL_ldui_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
        m_CommandMap["__builtin_IB_OCL_1d_ld_rw"] = initImageClass<COCL_ld_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
        m_CommandMap["__builtin_IB_OCL_1darr_ld_rw"] = initImageClass<COCL_ld_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_2d_ld_rw"] = initImageClass<COCL_ld_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
        m_CommandMap["__builtin_IB_OCL_2darr_ld_rw"] = initImageClass<COCL_ld_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_3d_ld_rw"] = initImageClass<COCL_ld_rw>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    }
    else
    {
        // Some image formats are not supported for typed reads, must route through sampler instead.
        m_CommandMap["__builtin_IB_OCL_1d_ldui_rw"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
        m_CommandMap["__builtin_IB_OCL_1darr_ldui_rw"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_2d_ldui_rw"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
        m_CommandMap["__builtin_IB_OCL_2darr_ldui_rw"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_3d_ldui_rw"] = initImageClass<COCL_ldui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
        m_CommandMap["__builtin_IB_OCL_1d_ld_rw"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
        m_CommandMap["__builtin_IB_OCL_1darr_ld_rw"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_2d_ld_rw"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
        m_CommandMap["__builtin_IB_OCL_2darr_ld_rw"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
        m_CommandMap["__builtin_IB_OCL_3d_ld_rw"] = initImageClass<COCL_ld>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    }

    m_CommandMap["__builtin_IB_OCL_2d_ldmcs"] = initImageClass<COCL_ldmcs>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ldmcs"] = initImageClass<COCL_ldmcs>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ld2dms"] = initImageClass<COCL_ld2dms>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ld2dms"] = initImageClass<COCL_ld2dms>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_2d_ld2dmsui"] = initImageClass<COCL_ld2dmsui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_OCL_2darr_ld2dmsui"] = initImageClass<COCL_ld2dmsui>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_OCL_1d_sample_l"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1darr_sample_l"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2d_sample_l"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2darr_sample_l"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_3d_sample_l"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1d_sample_d"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1darr_sample_d"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2d_sample_d"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2darr_sample_d"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_3d_sample_d"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1d_sample_lui"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1darr_sample_lui"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2d_sample_lui"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2darr_sample_lui"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_3d_sample_lui"] = initSamplerClass<COCL_sample_l>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1d_sample_dui"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_1darr_sample_dui"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2d_sample_dui"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_2darr_sample_dui"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY, pMdUtils, modMD);
    m_CommandMap["__builtin_IB_OCL_3d_sample_dui"] = initSamplerClass<COCL_sample_d>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D, pMdUtils, modMD);

    // Write Image
    m_CommandMap["__builtin_IB_write_1d_u1i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_write_1d_u2i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_write_1d_u3i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_write_1d_u4i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D);
    m_CommandMap["__builtin_IB_write_1darr_u1i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_write_1darr_u2i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_write_1darr_u3i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_write_1darr_u4i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_1D_ARRAY);
    m_CommandMap["__builtin_IB_write_2d_u1i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_write_2d_u2i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_write_2d_u3i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_write_2d_u4i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_write_2darr_u1i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_write_2darr_u2i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_write_2darr_u3i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_write_2darr_u4i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);
    m_CommandMap["__builtin_IB_write_3d_u1i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_write_3d_u2i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_write_3d_u3i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_write_3d_u4i"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_3D);
    m_CommandMap["__builtin_IB_write_2d_f"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D);
    m_CommandMap["__builtin_IB_write_2darr_f"] = initImageClass<CWrite>(paramMap, inlineMap, nextSampler, CImagesBI::Dimension::DIM_2D_ARRAY);

    // Get Image properties only supported here for bindless mode
    if (modMD->UseBindlessImage){
        m_CommandMap["__builtin_IB_get_image_width"] = CGetImageProperty::create(paramMap, CImagesBI::ImageProperty::WIDTH);
        m_CommandMap["__builtin_IB_get_image_height"] = CGetImageProperty::create(paramMap, CImagesBI::ImageProperty::HEIGHT);
        m_CommandMap["__builtin_IB_get_image_depth"] = CGetImageProperty::create(paramMap, CImagesBI::ImageProperty::DEPTH);
        m_CommandMap["__builtin_IB_get_image1d_array_size"] = CGetImageProperty::create(paramMap, CImagesBI::ImageProperty::ARRAY_SIZE_1D);
        m_CommandMap["__builtin_IB_get_image2d_array_size"] = CGetImageProperty::create(paramMap, CImagesBI::ImageProperty::ARRAY_SIZE_2D);
    }

    //convert Built-ins
    m_CommandMap["__builtin_IB_frnd_ne"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ROUNDNE, false);
    m_CommandMap["__builtin_IB_ftoh_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtn, true);
    m_CommandMap["__builtin_IB_ftoh_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtp, true);
    m_CommandMap["__builtin_IB_ftoh_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtz, true);
    m_CommandMap["__builtin_IB_dtoh_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtn, true);
    m_CommandMap["__builtin_IB_dtoh_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtp, true);
    m_CommandMap["__builtin_IB_dtoh_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtz, true);
    m_CommandMap["__builtin_IB_dtof_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtn, true);
    m_CommandMap["__builtin_IB_dtof_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtp, true);
    m_CommandMap["__builtin_IB_dtof_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftof_rtz, true);

    m_CommandMap["__builtin_IB_dtoi8_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtn, true);
    m_CommandMap["__builtin_IB_dtoi8_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtp, true);
    m_CommandMap["__builtin_IB_dtoi8_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rte, true);
    m_CommandMap["__builtin_IB_dtoi16_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtn, true);
    m_CommandMap["__builtin_IB_dtoi16_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtp, true);
    m_CommandMap["__builtin_IB_dtoi16_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rte, true);
    m_CommandMap["__builtin_IB_dtoi32_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtn, true);
    m_CommandMap["__builtin_IB_dtoi32_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtp, true);
    m_CommandMap["__builtin_IB_dtoi32_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rte, true);
    m_CommandMap["__builtin_IB_dtoi64_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtn, true);
    m_CommandMap["__builtin_IB_dtoi64_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rtp, true);
    m_CommandMap["__builtin_IB_dtoi64_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoi_rte, true);

    m_CommandMap["__builtin_IB_dtoui8_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtn, true);
    m_CommandMap["__builtin_IB_dtoui8_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtp, true);
    m_CommandMap["__builtin_IB_dtoui8_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rte, true);
    m_CommandMap["__builtin_IB_dtoui16_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtn, true);
    m_CommandMap["__builtin_IB_dtoui16_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtp, true);
    m_CommandMap["__builtin_IB_dtoui16_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rte, true);
    m_CommandMap["__builtin_IB_dtoui32_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtn, true);
    m_CommandMap["__builtin_IB_dtoui32_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtp, true);
    m_CommandMap["__builtin_IB_dtoui32_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rte, true);
    m_CommandMap["__builtin_IB_dtoui64_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtn, true);
    m_CommandMap["__builtin_IB_dtoui64_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rtp, true);
    m_CommandMap["__builtin_IB_dtoui64_rte"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ftoui_rte, true);

    m_CommandMap["__builtin_IB_itof_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtn, true);
    m_CommandMap["__builtin_IB_itof_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtp, true);
    m_CommandMap["__builtin_IB_itof_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtz, true);
    m_CommandMap["__builtin_IB_uitof_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtn, true);
    m_CommandMap["__builtin_IB_uitof_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtp, true);
    m_CommandMap["__builtin_IB_uitof_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtz, true);

    m_CommandMap["__builtin_IB_itofp64_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtn, true);
    m_CommandMap["__builtin_IB_itofp64_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtp, true);
    m_CommandMap["__builtin_IB_itofp64_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_itof_rtz, true);
    m_CommandMap["__builtin_IB_uitofp64_rtn"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtn, true);
    m_CommandMap["__builtin_IB_uitofp64_rtp"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtp, true);
    m_CommandMap["__builtin_IB_uitofp64_rtz"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_uitof_rtz, true);

    //Internal Debug Built-Ins
    m_CommandMap["__builtin_IB_read_cycle_counter"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_cycleCounter, false);
    m_CommandMap["__builtin_IB_source_value"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_source_value, false);
    m_CommandMap["__builtin_IB_set_dbg_register"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_SetDebugReg, false);
    m_CommandMap["__builtin_IB_movflag"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_movflag, false);
    m_CommandMap["__builtin_IB_movcr"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_movcr, false);
    m_CommandMap["__builtin_IB_hw_thread_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_hw_thread_id, false);
    m_CommandMap["__builtin_IB_slice_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_slice_id, false);
    m_CommandMap["__builtin_IB_subslice_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_subslice_id, false);
    m_CommandMap["__builtin_IB_logical_subslice_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_logical_subslice_id, false);
    m_CommandMap["__builtin_IB_dual_subslice_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_dual_subslice_id, false);
    m_CommandMap["__builtin_IB_eu_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_eu_id, false);
    m_CommandMap["__builtin_IB_get_sr0"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getSR0, false);
    m_CommandMap["__builtin_IB_set_sr0"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setSR0, false);
    m_CommandMap["__builtin_IB_eu_thread_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_eu_thread_id, false);
    m_CommandMap["__builtin_IB_eu_thread_pause"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_eu_thread_pause, false);

    //math Built-ins
    m_CommandMap["__builtin_IB_frnd_pi"] = CSimpleIntrinMapping::create(Intrinsic::ceil);
    m_CommandMap["__builtin_IB_frnd_ni"] = CSimpleIntrinMapping::create(Intrinsic::floor);
    m_CommandMap["__builtin_IB_frnd_zi"] = CSimpleIntrinMapping::create(Intrinsic::trunc);
    m_CommandMap["__builtin_IB_native_cosf"] = CSimpleIntrinMapping::create(Intrinsic::cos);
    m_CommandMap["__builtin_IB_native_cosh"] = CSimpleIntrinMapping::create(Intrinsic::cos);
    m_CommandMap["__builtin_IB_native_sinf"] = CSimpleIntrinMapping::create(Intrinsic::sin);
    m_CommandMap["__builtin_IB_native_sinh"] = CSimpleIntrinMapping::create(Intrinsic::sin);
    m_CommandMap["__builtin_IB_native_exp2f"] = CSimpleIntrinMapping::create(Intrinsic::exp2);
    m_CommandMap["__builtin_IB_native_exp2h"] = CSimpleIntrinMapping::create(Intrinsic::exp2);
    m_CommandMap["__builtin_IB_native_log2f"] = CSimpleIntrinMapping::create(Intrinsic::log2);
    m_CommandMap["__builtin_IB_native_log2h"] = CSimpleIntrinMapping::create(Intrinsic::log2);
    m_CommandMap["__builtin_IB_native_sqrtf"] = CSimpleIntrinMapping::create(Intrinsic::sqrt);
    m_CommandMap["__builtin_IB_native_sqrth"] = CSimpleIntrinMapping::create(Intrinsic::sqrt);
    m_CommandMap["__builtin_IB_native_sqrtd"] = CSimpleIntrinMapping::create(Intrinsic::sqrt);
    m_CommandMap["__builtin_IB_popcount_1u32"] = CSimpleIntrinMapping::create(Intrinsic::ctpop);
    m_CommandMap["__builtin_IB_popcount_1u16"] = CSimpleIntrinMapping::create(Intrinsic::ctpop);
    m_CommandMap["__builtin_IB_popcount_1u8"] = CSimpleIntrinMapping::create(Intrinsic::ctpop);
    m_CommandMap["__builtin_IB_native_powrf"] = CSimpleIntrinMapping::create(Intrinsic::pow);
    m_CommandMap["__builtin_IB_fma"] = CSimpleIntrinMapping::create(Intrinsic::fma);
    m_CommandMap["__builtin_IB_fmah"] = CSimpleIntrinMapping::create(Intrinsic::fma);
    m_CommandMap["__builtin_IB_bfi"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_bfi, false);
    m_CommandMap["__builtin_IB_ibfe"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ibfe, false);
    m_CommandMap["__builtin_IB_ubfe"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_ubfe, false);
    m_CommandMap["__builtin_IB_bfrev"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_bfrev, false);
    m_CommandMap["__builtin_IB_fmax"] = CSimpleIntrinMapping::create(Intrinsic::maxnum);
    m_CommandMap["__builtin_IB_fmin"] = CSimpleIntrinMapping::create(Intrinsic::minnum);
    m_CommandMap["__builtin_IB_HMAX"] = CSimpleIntrinMapping::create(Intrinsic::maxnum);
    m_CommandMap["__builtin_IB_HMIN"] = CSimpleIntrinMapping::create(Intrinsic::minnum);
    m_CommandMap["__builtin_IB_dmin"] = CSimpleIntrinMapping::create(Intrinsic::minnum);
    m_CommandMap["__builtin_IB_dmax"] = CSimpleIntrinMapping::create(Intrinsic::maxnum);
    m_CommandMap["__builtin_IB_mul_rtz_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_mul_rtz);
    m_CommandMap["__builtin_IB_mul_rtz_f32"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_mul_rtz);
    m_CommandMap["__builtin_IB_fma_rtz_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_fma_rtz);
    m_CommandMap["__builtin_IB_fma_rtz_f32"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_fma_rtz);
    m_CommandMap["__builtin_IB_add_rtz_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_add_rtz);
    m_CommandMap["__builtin_IB_add_rtz_f32"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_add_rtz);
    m_CommandMap["__builtin_IB_fma_rtp_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_fma_rtp);
    m_CommandMap["__builtin_IB_fma_rtn_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_fma_rtn);

    //Sync built-ins
    m_CommandMap["__builtin_IB_thread_group_barrier"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_threadgroupbarrier, false);
    m_CommandMap["__builtin_IB_thread_group_barrier_signal"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_threadgroupbarrier_signal, false);
    m_CommandMap["__builtin_IB_thread_group_barrier_wait"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_threadgroupbarrier_wait, false);
    m_CommandMap["__builtin_IB_memfence"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_memoryfence, false);
    m_CommandMap["__builtin_IB_flush_sampler_cache"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_flushsampler, false);
    m_CommandMap["__builtin_IB_typedmemfence"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_typedmemoryfence, false);
    m_CommandMap["__builtin_IB_system_memfence"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_systemmemoryfence, false);

    // surface compression - HDC flat CCS builtins
    m_CommandMap["__builtin_IB_hdc_uncompressed_write_uchar"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_HDCuncompressedwrite, true);
    // internal hint builtin
    m_CommandMap["__builtin_IB_assume_uniform"] = CSimpleIntrinMapping::create( GenISAIntrinsic::GenISA_assume_uniform, true );

    // helper built-ins
    m_CommandMap["__builtin_IB_simd_lane_id"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdLaneId, false);

    // vme builtins
    m_CommandMap["__builtin_IB_vme_send_ime"] = CVMESend::create(GenISAIntrinsic::GenISA_vmeSendIME, paramMap);
    m_CommandMap["__builtin_IB_vme_send_fbr"] = CVMESend::create(GenISAIntrinsic::GenISA_vmeSendFBR, paramMap);
    m_CommandMap["__builtin_IB_vme_send_sic"] = CVMESend::create(GenISAIntrinsic::GenISA_vmeSendSIC, paramMap, 3);

    m_CommandMap["__builtin_IB_vme_send_fbr_new"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendFBR2, paramMap, ctx, false);
    m_CommandMap["__builtin_IB_vme_send_sic_new"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendSIC2, paramMap, ctx, false);
    m_CommandMap["__builtin_IB_vme_send_ime_new_uint4_uint8"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendIME2, paramMap, ctx, true);
    m_CommandMap["__builtin_IB_vme_send_ime_new_uint8_uint8"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendIME2, paramMap, ctx, true);
    m_CommandMap["__builtin_IB_vme_send_ime_new_uint4_uint4"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendIME2, paramMap, ctx, true);
    m_CommandMap["__builtin_IB_vme_send_ime_new_uint8_uint4"] = CNewVMESend::create(GenISAIntrinsic::GenISA_vmeSendIME2, paramMap, ctx, true);

    m_CommandMap["__builtin_IB_set_message_phase_legacy_dw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX_legacy, true);
    m_CommandMap["__builtin_IB_set_message_phase_legacy_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX_legacy, true);
    m_CommandMap["__builtin_IB_set_message_phase_legacy_ub"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX_legacy, true);

    m_CommandMap["__builtin_IB_set_message_phase_legacy"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhase_legacy, false);

    m_CommandMap["__builtin_IB_create_message_phases"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhases, false);
    m_CommandMap["__builtin_IB_create_message_phases_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesV, true);
    m_CommandMap["__builtin_IB_create_message_phases_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesV, true);
    m_CommandMap["__builtin_IB_create_message_phases_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesV, true);

    m_CommandMap["__builtin_IB_create_message_phases_no_init"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesNoInit, false);
    m_CommandMap["__builtin_IB_create_message_phases_no_init_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesNoInitV, true);
    m_CommandMap["__builtin_IB_create_message_phases_no_init_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesNoInitV, true);
    m_CommandMap["__builtin_IB_create_message_phases_no_init_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_createMessagePhasesNoInitV, true);

    m_CommandMap["__builtin_IB_get_message_phase_dw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseX, true);
    m_CommandMap["__builtin_IB_get_message_phase_dw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_dw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_dw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_get_message_phase_uq"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseX, true);
    m_CommandMap["__builtin_IB_get_message_phase_uq_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uq_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uq_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_set_message_phase_dw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX, true);
    m_CommandMap["__builtin_IB_set_message_phase_dw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_dw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_dw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_get_message_phase_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseX, true);
    m_CommandMap["__builtin_IB_get_message_phase_uw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_set_message_phase_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX, true);
    m_CommandMap["__builtin_IB_set_message_phase_uw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_uw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_uw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_get_message_phase_ub"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseX, true);
    m_CommandMap["__builtin_IB_get_message_phase_ub_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_ub_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_get_message_phase_ub_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_set_message_phase_ub"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseX, true);
    m_CommandMap["__builtin_IB_set_message_phase_ub_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_ub_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);
    m_CommandMap["__builtin_IB_set_message_phase_ub_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseXV, true);

    m_CommandMap["__builtin_IB_get_message_phase"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhase, false);
    m_CommandMap["__builtin_IB_get_message_phase_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseV, true);
    m_CommandMap["__builtin_IB_get_message_phase_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getMessagePhaseV, true);

    m_CommandMap["__builtin_IB_set_message_phase"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhase, false);
    m_CommandMap["__builtin_IB_set_message_phase_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseV, true);
    m_CommandMap["__builtin_IB_set_message_phase_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseV, true);
    m_CommandMap["__builtin_IB_set_message_phase_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_setMessagePhaseV, true);

    m_CommandMap["__builtin_IB_broadcast_message_phase_ub"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhase, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_ub_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_ub_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_ub_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);

    m_CommandMap["__builtin_IB_broadcast_message_phase_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhase, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);

    m_CommandMap["__builtin_IB_broadcast_message_phase_dw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhase, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_dw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_dw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_dw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);

    m_CommandMap["__builtin_IB_broadcast_message_phase_uq"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhase, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uq_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uq_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);
    m_CommandMap["__builtin_IB_broadcast_message_phase_uq_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_broadcastMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_set_message_phase_ub"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_ub_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_ub_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_ub_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_set_message_phase_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_set_message_phase_dw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_dw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_dw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_dw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_set_message_phase_uq"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uq_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uq_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_set_message_phase_uq_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdSetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_get_message_phase_uw"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uw_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uw_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uw_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_simd_get_message_phase_uq"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhase, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uq_uint2"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uq_uint4"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);
    m_CommandMap["__builtin_IB_simd_get_message_phase_uq_uint8"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdGetMessagePhaseV, true);

    m_CommandMap["__builtin_IB_extract_mv_and_sad"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_extractMVAndSAD, false);
    m_CommandMap["__builtin_IB_cmp_sads"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_cmpSADs, false);

    m_CommandMap["__builtin_IB_simdMediaRegionCopy"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_simdMediaRegionCopy, false);

    // builtin to emit a memcpy intrinsic
    m_CommandMap["__builtin_IB_memcpy_global_to_private"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_constant_to_private"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_local_to_private"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_private_to_private"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_generic_to_private"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_private_to_global"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_private_to_constant"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_private_to_local"] = CMemCpy::create();
    m_CommandMap["__builtin_IB_memcpy_private_to_generic"] = CMemCpy::create();

    // Correctly Rounded built ins
    m_CommandMap["__builtin_IB_ieee_sqrt"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_IEEE_Sqrt, false);
    m_CommandMap["__builtin_IB_ieee_divide"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_IEEE_Divide, true);
    m_CommandMap["__builtin_IB_ieee_divide_f64"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_IEEE_Divide, true);

    // VA built-ins
    m_CommandMap["__builtin_IB_va_erode_64x4"] = CVASend::create(GenISAIntrinsic::GenISA_vaErode, paramMap);
    m_CommandMap["__builtin_IB_va_dilate_64x4"] = CVASend::create(GenISAIntrinsic::GenISA_vaDilate, paramMap);
    m_CommandMap["__builtin_IB_va_minmaxfilter_16x4_SLM"] = CVASend::create(GenISAIntrinsic::GenISA_vaMinMaxFilter, paramMap);
    m_CommandMap["__builtin_IB_va_convolve_16x4_SLM"] = CVASend::create(GenISAIntrinsic::GenISA_vaConvolve, paramMap);
    m_CommandMap["__builtin_IB_va_convolve_16x1"] = CVASend::create(GenISAIntrinsic::GenISA_vaConvolveGRF_16x1, paramMap);
    m_CommandMap["__builtin_IB_va_convolve_16x4"] = CVASend::create(GenISAIntrinsic::GenISA_vaConvolveGRF_16x4, paramMap);
    m_CommandMap["__builtin_IB_va_minmax"] = CVASend::create(GenISAIntrinsic::GenISA_vaMinMax, paramMap);
    m_CommandMap["__builtin_IB_va_centroid"] = CVASend::create(GenISAIntrinsic::GenISA_vaCentroid, paramMap);
    m_CommandMap["__builtin_IB_va_boolcentroid"] = CVASend::create(GenISAIntrinsic::GenISA_vaBoolCentroid, paramMap);
    m_CommandMap["__builtin_IB_va_boolsum"] = CVASend::create(GenISAIntrinsic::GenISA_vaBoolSum, paramMap);

    // Ballot builtins
    m_CommandMap["__builtin_IB_WaveBallot"] = CWaveBallotIntrinsic::create(GenISAIntrinsic::GenISA_WaveBallot);
    m_CommandMap["__builtin_IB_clustered_WaveBallot"] = CWaveBallotIntrinsic::create(GenISAIntrinsic::GenISA_WaveClusteredBallot);

    m_CommandMap[StringRef("__builtin_IB_samplepos")] = CSamplePos::create();

    m_CommandMap["__builtin_IB_bfn_i16"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_bfn);
    m_CommandMap["__builtin_IB_bfn_i32"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_bfn);

    // `dp4a` built-ins
    m_CommandMap["__builtin_IB_dp4a_ss"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_dp4a_ss, false);
    m_CommandMap["__builtin_IB_dp4a_uu"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_dp4a_uu, false);
    m_CommandMap["__builtin_IB_dp4a_su"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_dp4a_su, false);
    m_CommandMap["__builtin_IB_dp4a_us"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_dp4a_us, false);

    m_CommandMap["__builtin_IB_software_exception"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_software_exception, false);
    m_CommandMap["__builtin_IB_enable_ieee_exception_trap"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_enable_ieee_exception_trap, false);
    m_CommandMap["__builtin_IB_disable_ieee_exception_trap"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_disable_ieee_exception_trap, false);
    m_CommandMap["__builtin_IB_get_stack_pointer"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getStackPointer, false);
    m_CommandMap["__builtin_IB_get_stack_size_per_thread"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_getStackSizePerThread, false);

    m_CommandMap["__devicelib_exit"] = CSimpleIntrinMapping::create(GenISAIntrinsic::GenISA_thread_exit, false);

    m_KnownBuiltins = {
        //resolved later in GenericAddressDynamicResolution pass:
        "__builtin_IB_to_local",
        "__builtin_IB_to_private"
    };
}

bool CBuiltinsResolver::resolveBI(CallInst* Inst)
{
    Function* callee = Inst->getCalledFunction();
    StringRef calleeName = callee->getName();
    //Check if it exists in the map.
    if (!m_CommandMap.count(calleeName))
    {
        return false;
    }
    m_CommandMap[calleeName]->execute(Inst, m_CodeGenContext);
    m_CommandMap[calleeName]->verifyCommand();

    return !m_CommandMap[calleeName]->hasError();
}

llvm::CallInst* IGC::CallMemoryFenceWorkgroup(llvm::Instruction* pInsertBefore)
{
    IGCLLVM::IRBuilder<> builder(pInsertBefore);
    llvm::Module* pM = pInsertBefore->getModule();

    Value* trueValue = builder.getInt1(true);
    Value* falseValue = builder.getInt1(false);
    Value* groupScopeValue = builder.getInt32(LSC_SCOPE_GROUP);

    // Add memory fence for workgroup
    return  GenIntrinsicInst::Create(
        GenISAIntrinsic::getDeclaration(pM, GenISAIntrinsic::GenISA_memoryfence),
        {
            trueValue,  // bool commitEnable
            falseValue, // bool flushRW
            falseValue, // bool flushConstant
            falseValue, // bool flushTexture
            falseValue, // bool flushIcache
            falseValue,   // bool isGlobal
            falseValue, // bool invalidateL1
            falseValue, // bool evictL1
            groupScopeValue // int memory scope
        },
        "",
        pInsertBefore);
}
