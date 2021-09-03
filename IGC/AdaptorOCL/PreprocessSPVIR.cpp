/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PreprocessSPVIR.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include "common/LLVMWarningsPop.hpp"

#include <regex>
#include <iostream>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-preprocess-spvir"
#define PASS_DESCRIPTION "Adjust SPV-IR produced by Khronos SPIRV-LLVM Translator to be consumable by IGC BiFModule"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PreprocessSPVIR::ID = 0;

PreprocessSPVIR::PreprocessSPVIR() : ModulePass(ID)
{
    initializePreprocessSPVIRPass(*PassRegistry::getPassRegistry());
}

uint64_t PreprocessSPVIR::parseSampledImageTy(StructType* sampledImageTy)
{
    std::string Name = sampledImageTy->getName().str();
    std::smatch match;
    std::regex reg("spirv.SampledImage._void_([0-6])_([0-2])_([0-1])_([0-1])_([0-2])_([0-9]+)_([0-2])");

    if (std::regex_match(Name, match, reg))
    {
        IGC_ASSERT(match.size() == 8);
        uint64_t ImageType = 0;

        uint64_t Dim = stoi(match[1]);
        ImageType |= Dim << 59;
        uint64_t Depth = stoi(match[2]);
        ImageType |= Depth << 58;
        uint64_t Arrayed = stoi(match[3]);
        ImageType |= Arrayed << 57;
        uint64_t MS = stoi(match[4]);
        ImageType |= MS << 56;
        uint64_t Sampled = stoi(match[5]);
        ImageType |= Sampled << 62;
        uint64_t Access = stoi(match[6]);
        ImageType |= Access << 54;

        return ImageType;
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "SampledImage opaque type is incorrect!");
        return 0;
    }
}

Value* PreprocessSPVIR::getWidenImageCoordsArg(Value* Coords)
{
    Type* coordsType = Coords->getType();
    Value* newCoords = nullptr;
    if (!isa<IGCLLVM::FixedVectorType>(coordsType))
    {
        Value* undef = UndefValue::get(IGCLLVM::FixedVectorType::get(coordsType, 4));
        newCoords = m_Builder->CreateInsertElement(undef, Coords, ConstantInt::get(m_Builder->getInt32Ty(), 0));
    }
    else if (cast<IGCLLVM::FixedVectorType>(coordsType)->getNumElements() < 4)
    {
        SmallVector<Constant*, 4> shuffleIdx;
        for (uint64_t i = 0; i < cast<IGCLLVM::FixedVectorType>(coordsType)->getNumElements(); i++)
            shuffleIdx.push_back(ConstantInt::get(m_Builder->getInt32Ty(), i));

        for (uint64_t i = cast<IGCLLVM::FixedVectorType>(coordsType)->getNumElements(); i < 4; i++)
            shuffleIdx.push_back(ConstantInt::get(m_Builder->getInt32Ty(), 0));

        newCoords = m_Builder->CreateShuffleVector(Coords, UndefValue::get(coordsType), ConstantVector::get(shuffleIdx));
    }
    return newCoords;
}

void PreprocessSPVIR::createCallAndReplace(CallInst& oldCallInst, StringRef newFuncName, std::vector<Value*>& args )
{
    Function* F = oldCallInst.getCalledFunction();
    IGC_ASSERT(F);

    std::vector<Type*> argTypes;
    for (auto arg : args)
        argTypes.push_back(arg->getType());

    FunctionType* FT = FunctionType::get(oldCallInst.getType(), argTypes, false);
    FunctionCallee newFunction = m_Module->getOrInsertFunction(newFuncName, FT, oldCallInst.getAttributes());
    CallInst* newCall = m_Builder->CreateCall(newFunction, args);
    oldCallInst.replaceAllUsesWith(newCall);
    oldCallInst.eraseFromParent();
}

// Transform:
// %SampledImage = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImage14ocl_image2d_ro11ocl_sampler(
//                      %opencl.image2d_ro_t addrspace(1)* %image, %opencl.sampler_t addrspace(2)* %sampler)
// call spir_func <4 x float> @_Z38__spirv_ImageSampleExplicitLod_Rfloat4PU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0Dv2_fif(
//                      %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)*% SampledImage, <2 x float> %coords, i32 2, float 0.000000e+00)
//
// ---->
//
// %0 = ptrtoint %opencl.image2d_ro_t addrspace(1)* %image to i64
// %1 = ptrtoint % opencl.sampler_t addrspace(2) * %sampler to i64
// %2 = insertelement <3 x i64> undef, i64 %0, i32 0
// %3 = insertelement <3 x i64> %2, i64 576460752303423488, i32 1
// %4 = insertelement <3 x i64> %3, i64 %1, i32 2
// %5 = shufflevector <2 x float> %coords, <2 x float> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
// %6 = call <4 x float> @__intel_sample_image_lod_fcoords_Rfloat4(<3 x i64> %4, <4 x float> %5, i32 2, float 0.000000e+00)
void PreprocessSPVIR::visitImageSampleExplicitLod(CallInst& CI)
{
    m_Builder->SetInsertPoint(&CI);

    enum {
        SAMPLED_IMAGE_INDEX,
        COORDS_INDEX,
        IMAGE_OPERANDS_INDEX,
        LOD_INDEX,
        DX_INDEX = LOD_INDEX,
        DY_INDEX
    };

    auto callSampledImage = cast<CallInst>(CI.getArgOperand(0));
    auto sampledImagePtr = cast<PointerType>(callSampledImage->getType());
    auto sampledImageTy = cast<StructType>(sampledImagePtr->getElementType());

    std::vector<Value*> args;
    for (size_t i = 0; i < CI.getNumArgOperands(); i++)
        args.push_back(CI.getArgOperand(i));

    // Khronos SPIRV-LLVM Translator represents OpTypeSampledImage as a global pointer to opaque type:
    //  %spirv.SampledImage._{SampledType}_{Dim}_{Depth}_{Arrayed}_{MS}_{Sampled}_{Format}_{Access}.
    // Such representation cannot be optimally handled in BiFModule itself, since it would require to temporary
    // allocate global memory and then clear this up once image/sampler intrinsics are resolved.
    // It's easier to transform it into <3 x i64> vector containing:
    //   -image,
    //   -image type which comes from parsing opaque type
    //   -sampler.
    Value* unifiedSampledImage = UndefValue::get(IGCLLVM::FixedVectorType::get(m_Builder->getInt64Ty(), 3));

    Value* imageAsInt = m_Builder->CreatePtrToInt(callSampledImage->getArgOperand(0), m_Builder->getInt64Ty());
    Value* samplerAsInt = m_Builder->CreatePtrToInt(callSampledImage->getArgOperand(1), m_Builder->getInt64Ty());

    unifiedSampledImage = m_Builder->CreateInsertElement(unifiedSampledImage, imageAsInt, m_Builder->getInt32(0));
    uint64_t imageType = parseSampledImageTy(sampledImageTy);
    unifiedSampledImage = m_Builder->CreateInsertElement(unifiedSampledImage, m_Builder->getInt64(imageType), m_Builder->getInt32(1));
    unifiedSampledImage = m_Builder->CreateInsertElement(unifiedSampledImage, samplerAsInt, m_Builder->getInt32(2));

    args[SAMPLED_IMAGE_INDEX] = unifiedSampledImage;

    // Widen coords argument, since __intel_sample_image always takes coordinates as 4-elements vector type
    Value* coords = getWidenImageCoordsArg(args[COORDS_INDEX]);
    args[COORDS_INDEX] = coords;

    // Prepare a new builtin name
    std::string unifiedImageSampleName = "__intel_sample_image";
    auto imageOperand = cast<ConstantInt>(args[IMAGE_OPERANDS_INDEX]);
    switch (imageOperand->getZExtValue())
    {
    case 2: // Lod
    {
        unifiedImageSampleName += "_lod";

        Type* scalarCoordsType = coords->getType()->getScalarType();
        if (scalarCoordsType->isFloatTy())
            unifiedImageSampleName += "_fcoords";
        else if (scalarCoordsType->isIntegerTy())
            unifiedImageSampleName += "_icoords";
        else
            IGC_ASSERT_MESSAGE(0, "Unsupported coordinates type of ImageSampleExplicitLod builtin.");
    }
    break;
    case 4: // Grad
    {
        unifiedImageSampleName += "_grad";
        Type* dxType = args[DX_INDEX]->getType();
        Type* dyType = args[DY_INDEX]->getType();
        IGC_ASSERT(dxType == dyType);

        Type* scalarDxType = dxType->getScalarType();
        if (scalarDxType->isFloatTy())
            unifiedImageSampleName += "_float";
        else if(scalarDxType->isIntegerTy())
            unifiedImageSampleName += "_uint";
        else
            IGC_ASSERT_MESSAGE(0, "Unsupported dx/dy types of ImageSampleExplicitLod builtin.");

        if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(dxType))
        {
            unifiedImageSampleName += std::to_string(VT->getNumElements());
        }
    }
    break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unsupported image operand!");
        break;
    }

   Type* retType = CI.getType();
   auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(retType);
   if (VT && VT->getNumElements() == 4)
   {
        Type* retScalarType = retType->getScalarType();
        if (retScalarType->isIntegerTy())
            unifiedImageSampleName += "_Ruint4";
        else if (retScalarType->isFloatTy())
            unifiedImageSampleName += "_Rfloat4";
        else if (retScalarType->isHalfTy())
            unifiedImageSampleName += "_Rhalf4";
        else
            IGC_ASSERT_MESSAGE(0, "Unsupported return type of ImageSampleExplicitLod builtin.");
    }
    else
        IGC_ASSERT_MESSAGE(0, "ImageSampleExplicitLod builtin must return vector 4-elements type.");

    // Create function call to __intel_sample_image_{Lod|Grad}_{i|f}coords_R{returnType}
    createCallAndReplace(CI, unifiedImageSampleName, args);

    if (callSampledImage->use_empty())
        callSampledImage->eraseFromParent();

    m_changed = true;
}

// Replace functions like:
//  i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*)
//  i32 @_Z18__spirv_ocl_printfPU3AS2ci(i8 addrspace(2)*, i32)
// With:
//  i32 @printf(i8 addrspace(2)*, ...)
//
// Khronos SPV-IR represents printf function as a non-variadic one. Since
// IGC supports clang-consistent representation of printf (which is unmangled,
// variadic function), all printf calls must get replaced.
void PreprocessSPVIR::visitOpenCLEISPrintf(llvm::CallInst& CI)
{
    FunctionType* FT = FunctionType::get(CI.getType(), Type::getInt8PtrTy(m_Module->getContext(), 2), true);
    FunctionCallee newPrintf = m_Module->getOrInsertFunction("printf", FT);
    CI.setCalledFunction(newPrintf);

    m_changed = true;
}

bool PreprocessSPVIR::isSPVIR(StringRef funcName)
{
    std::regex patternRegular("_Z[0-9]+__spirv_[A-Z].*");
    // Pattern for SPV-IR produced from "OpenCL Extended Instruction Set" instruction
    std::regex patternEIS("_Z[0-9]+__spirv_ocl_[a-z].*");
    std::string name = funcName.str();

    return std::regex_search(name, patternRegular) || std::regex_search(name, patternEIS);
}

void PreprocessSPVIR::visitCallInst(CallInst& CI)
{
    Function* F = CI.getCalledFunction();
    if (!F) return;

    StringRef Name = F->getName();
    if (!isSPVIR(Name)) return;

    if (Name.contains("ImageSampleExplicitLod"))
    {
        visitImageSampleExplicitLod(CI);
    }
    else if (Name.contains("printf"))
    {
        visitOpenCLEISPrintf(CI);
    }
}

bool PreprocessSPVIR::runOnModule(Module& M) {
    m_Module = &M;
    IRBuilder<> builder(M.getContext());
    m_Builder = &builder;
    visit(M);
    return m_changed;
}
