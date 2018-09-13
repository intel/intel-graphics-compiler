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
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "AdaptorOCL/OCL/KernelAnnotations.hpp"
#include "common/allocator.h"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "common/SystemThread.h"
#include "common/secure_mem.h"

#include <iStdLib/utility.h>

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/ADT/StringExtras.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/DebugInfo/VISADebugEmitter.hpp"

/***********************************************************************************
This file contains the code specific to opencl kernels
************************************************************************************/

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace IGC
{

COpenCLKernel::COpenCLKernel(const OpenCLProgramContext* ctx, Function* pFunc, CShaderProgram* pProgram) : 
CShader(pFunc, pProgram)
{
    m_HasTID                = false;
    m_HasGlobalSize         = false;
    m_disableMidThreadPreemption = false;
    m_perWIPrivateMemSize   = 0;
    m_Context               = const_cast<OpenCLProgramContext*>(ctx);
    m_localOffsetsMap.clear();
    m_pBtiLayout            = &(ctx->btiLayout);
    m_Platform              = &(ctx->platform);
    m_DriverInfo            = &(ctx->m_DriverInfo);

    // Prefer to not memset a struct - initializers are safer!
    memset(&m_kernelInfo.m_kernelProgram, 0, sizeof(SKernelProgram));
}

COpenCLKernel::~COpenCLKernel()
{
    ClearKernelInfo();
    m_simdProgram.Destroy();
}

void COpenCLKernel::ClearKernelInfo() 
{
    // Global pointer arguments
    for(auto iter = m_kernelInfo.m_pointerArgument.begin();
        iter != m_kernelInfo.m_pointerArgument.end();
        ++iter)
    {
        iOpenCL::PointerArgumentAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_pointerArgument.clear();

    // Non-argument pointer inputs
    for(auto iter = m_kernelInfo.m_pointerInput.begin();
        iter != m_kernelInfo.m_pointerInput.end();
        ++iter)
    {
        iOpenCL::PointerInputAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_pointerInput.clear();

    // Local pointer arguments
    for(auto iter = m_kernelInfo.m_localPointerArgument.begin();
        iter != m_kernelInfo.m_localPointerArgument.end();
        ++iter)
    {
        iOpenCL::LocalArgumentAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_localPointerArgument.clear();

    // Sampler inputs
    for(auto iter = m_kernelInfo.m_samplerInput.begin();
        iter != m_kernelInfo.m_samplerInput.end();
        ++iter)
    {
        iOpenCL::SamplerInputAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_samplerInput.clear();

    // Sampler arguments
    for(auto iter = m_kernelInfo.m_samplerArgument.begin();
        iter != m_kernelInfo.m_samplerArgument.end();
        ++iter)
    {
        iOpenCL::SamplerArgumentAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_samplerArgument.clear();

    // Scalar inputs
    for(auto iter = m_kernelInfo.m_constantInputAnnotation.begin();
        iter != m_kernelInfo.m_constantInputAnnotation.end();
        ++iter)
    {
        iOpenCL::ConstantInputAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_constantInputAnnotation.clear();

    // Scalar arguments
    for(auto iter = m_kernelInfo.m_constantArgumentAnnotation.begin();
        iter != m_kernelInfo.m_constantArgumentAnnotation.end();
        ++iter)
    {
        iOpenCL::ConstantArgumentAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_constantArgumentAnnotation.clear();

    // Image arguments
    for(auto iter = m_kernelInfo.m_imageInputAnnotations.begin();
        iter != m_kernelInfo.m_imageInputAnnotations.end();
        ++iter)
    {
        iOpenCL::ImageArgumentAnnotation* annotation = *iter;
        delete annotation;
    }
    m_kernelInfo.m_imageInputAnnotations.clear();

    // Kernel Arg Reflection Info
    for(auto iter = m_kernelInfo.m_kernelArgInfo.begin();
        iter != m_kernelInfo.m_kernelArgInfo.end();
        ++iter)
    {
        delete *iter;
    }
    m_kernelInfo.m_kernelArgInfo.clear();

    // Printf strings
    for(auto iter = m_kernelInfo.m_printfStringAnnotations.begin();
        iter != m_kernelInfo.m_printfStringAnnotations.end();
        ++iter)
    {
        iOpenCL::PrintfStringAnnotation* annotation = *iter;

        delete[] annotation->StringData;
        delete annotation;
    }
    m_kernelInfo.m_printfStringAnnotations.clear();

    // Printf Buffer Annotation
    if(m_kernelInfo.m_printfBufferAnnotation != nullptr)
    {
        delete m_kernelInfo.m_printfBufferAnnotation;
        m_kernelInfo.m_printfBufferAnnotation = nullptr;
    }

    // StartGASAnnotationAnnotation
    if(m_kernelInfo.m_startGAS != nullptr)
    {
        delete m_kernelInfo.m_startGAS;
        m_kernelInfo.m_startGAS = nullptr;
    }

     // WindowSizeGASAnnotation Annotation
    if(m_kernelInfo.m_WindowSizeGAS != nullptr)
    {
        delete m_kernelInfo.m_WindowSizeGAS;
        m_kernelInfo.m_WindowSizeGAS = nullptr;
    }

     // PrivateMemSizeAnnotation Annotation
    if(m_kernelInfo.m_PrivateMemSize != nullptr)
    {
        delete m_kernelInfo.m_PrivateMemSize;
        m_kernelInfo.m_PrivateMemSize = nullptr;
    }

    // Argument to BTI/Sampler index map
    m_kernelInfo.m_argIndexMap.clear();
}

void COpenCLKernel::PreCompile()
{
    ClearKernelInfo();
    CreateImplicitArgs();
    //We explicitly want this to be GRF-sized, without relation to simd width

    RecomputeBTLayout();

    // Initialize the table of offsets for GlobalVariables representing locals
    auto funcIter = m_pMdUtils->findFunctionsInfoItem(entry);
    if (funcIter != m_pMdUtils->end_FunctionsInfo())
    {
        auto loIter = funcIter->second->begin_LocalOffsets();
        auto loEnd = funcIter->second->end_LocalOffsets();
        for (; loIter != loEnd; ++loIter)
        {
            LocalOffsetMetaDataHandle loHandle = *loIter;
            m_localOffsetsMap[loHandle->getVar()] = loHandle->getOffset();
        }
    }
}

SOpenCLKernelInfo::SResourceInfo COpenCLKernel::getResourceInfo(int argNo)
{
    SOpenCLKernelInfo::SResourceInfo resInfo;
    ResourceAllocMetaDataHandle resAllocMD = m_pMdUtils->getFunctionsInfoItem(entry)->getResourceAlloc();
    assert(resAllocMD->hasValue() && "Resource Allocation Information not present");
    ArgAllocMetaDataHandle argInfo = resAllocMD->getArgAllocsItem(argNo);
    ResourceTypeEnum type = (ResourceTypeEnum)argInfo->getType();

    if (type == ResourceTypeEnum::UAVResourceType ||
        type == ResourceTypeEnum::BindlessUAVResourceType)
    {
        resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_UAV;
    }
    else if (type == ResourceTypeEnum::SRVResourceType)
    {
        resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_SRV;
    }
    else
    {
        resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_OTHER;
    }
    resInfo.Index = argInfo->getIndex();
    return resInfo;
}

ResourceExtensionTypeEnum COpenCLKernel::getExtensionInfo(int argNo)
{
    ResourceAllocMetaDataHandle resAllocMD = m_pMdUtils->getFunctionsInfoItem(entry)->getResourceAlloc();
    assert(resAllocMD->hasValue() && "Resource Allocation Information not present");
    ArgAllocMetaDataHandle argInfo = resAllocMD->getArgAllocsItem(argNo);
    return (ResourceExtensionTypeEnum)argInfo->getExtenstionType();
}

void COpenCLKernel::CreateInlineSamplerAnnotations()
{
    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);
    ResourceAllocMetaDataHandle resAllocMD = funcInfoMD->getResourceAlloc();
    if (!resAllocMD->hasValue())
    {
        return;
    }

    for (auto i = resAllocMD->begin_InlineSamplers(), e = resAllocMD->end_InlineSamplers(); i != e; ++i)
    {
        InlineSamplerMetaDataHandle inlineSamplerMD = *i;
        iOpenCL::SamplerInputAnnotation* samplerInput = new iOpenCL::SamplerInputAnnotation();
        
        samplerInput->AnnotationSize              = sizeof(samplerInput);
        samplerInput->SamplerType                 = iOpenCL::SAMPLER_OBJECT_TEXTURE;
        samplerInput->SamplerTableIndex           = inlineSamplerMD->getIndex();
        
        samplerInput->TCXAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD->getTCXAddressMode());
        samplerInput->TCYAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD->getTCYAddressMode());
        samplerInput->TCZAddressMode = iOpenCL::SAMPLER_TEXTURE_ADDRESS_MODE(inlineSamplerMD->getTCZAddressMode());
        samplerInput->NormalizedCoords = inlineSamplerMD->getNormalizedCoords() != 0 ? true : false;
        samplerInput->MagFilterType = iOpenCL::SAMPLER_MAPFILTER_TYPE(inlineSamplerMD->getMagFilterType());
        samplerInput->MinFilterType = iOpenCL::SAMPLER_MAPFILTER_TYPE(inlineSamplerMD->getMinFilterType());
        samplerInput->MipFilterType = iOpenCL::SAMPLER_MIPFILTER_TYPE(inlineSamplerMD->getMipFilterType());

        samplerInput->CompareFunc = iOpenCL::SAMPLER_COMPARE_FUNC_TYPE(inlineSamplerMD->getCompareFunc());

        samplerInput->BorderColorR = inlineSamplerMD->getBorderColorR();
        samplerInput->BorderColorG = inlineSamplerMD->getBorderColorG();
        samplerInput->BorderColorB = inlineSamplerMD->getBorderColorB();
        samplerInput->BorderColorA = inlineSamplerMD->getBorderColorA();

        m_kernelInfo.m_samplerInput.push_back(samplerInput);
    }

    {
        auto *MD = m_Context->getModuleMetaData();
        auto   I = MD->FuncMD.find(entry);
        if (I != MD->FuncMD.end())
        {
            m_kernelInfo.m_HasInlineVmeSamplers = I->second.hasInlineVmeSamplers;
        }
    }
}

void COpenCLKernel::CreateKernelArgInfo()
{
    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

    uint count = funcInfoMD->size_OpenCLArgAccessQualifiers();

    for (uint i = 0; i < count; ++i)
    {
        iOpenCL::KernelArgumentInfoAnnotation* kernelArgInfo = new iOpenCL::KernelArgumentInfoAnnotation();
        
        // Format the strings the way the OpenCL runtime expects them
        
        // The access qualifier is expected to have a "__" prefix, 
        // or an upper-case "NONE" if there is no qualifier
        kernelArgInfo->AccessQualifier = funcInfoMD->getOpenCLArgAccessQualifiersItem(i);
        if (kernelArgInfo->AccessQualifier == "none" || kernelArgInfo->AccessQualifier == "")
        {
            kernelArgInfo->AccessQualifier = "NONE";
        }
        else if (kernelArgInfo->AccessQualifier[0] != '_')
        {
            kernelArgInfo->AccessQualifier = "__" + kernelArgInfo->AccessQualifier;
        }

        // The address space is expected to have a __ prefix
        switch (funcInfoMD->getOpenCLArgAddressSpacesItem(i))
        {
        case ADDRESS_SPACE_CONSTANT:
            kernelArgInfo->AddressQualifier = "__constant";
            break;
        case ADDRESS_SPACE_GLOBAL:
            kernelArgInfo->AddressQualifier = "__global";
            break;
        case ADDRESS_SPACE_LOCAL:
            kernelArgInfo->AddressQualifier = "__local";
            break;
        case ADDRESS_SPACE_PRIVATE:
            kernelArgInfo->AddressQualifier = "__private";
            break;
        default:
            assert(0 && "Unexpected address space");
            break;
        }

        // ArgNames is not guaranteed to be present if -cl-kernel-arg-info
        // is not passed in.
        if (funcInfoMD->isOpenCLArgNamesHasValue())
        {
            kernelArgInfo->ArgumentName = funcInfoMD->getOpenCLArgNamesItem(i);
        }

        // The type name is expected to also have the type size, appended after a ";"
        kernelArgInfo->TypeName = funcInfoMD->getOpenCLArgTypesItem(i) + ";";

        // Unfortunately, unlike SPIR, legacy OCL uses an ABI that has byval pointers.
        // So, if the parameter is a byval pointer, look at the contained type
        {
            Function::arg_iterator argumentIter = entry->arg_begin();
            std::advance(argumentIter, i);

            Type* argType = entry->getFunctionType()->getParamType(i);
            if (argumentIter->hasByValAttr())
            {
                argType = argType->getContainedType(0);
            }

            kernelArgInfo->TypeName += utostr(m_DL->getTypeAllocSize(argType));
        }

        // If there are no type qualifiers, "NONE" is expected
        kernelArgInfo->TypeQualifier = funcInfoMD->getOpenCLArgTypeQualifiersItem(i);
        if (kernelArgInfo->TypeQualifier == "")
        {
            kernelArgInfo->TypeQualifier = "NONE";
        }

        m_kernelInfo.m_kernelArgInfo.push_back(kernelArgInfo);
    }
}

void COpenCLKernel::CreateKernelAttributeInfo()
{
    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);
    
    // We need to concatenate 2 things:
    // (a) LLVM attributes, except nounwind. Why? Because that's how IGIL does it.
    // (b) The attributes that get translated into SPIR metadata:
    //     (*) vec_type_hint
    //     (*) reqd_work_group_size
    //     (*) work_group_size_hint
    //

    // Get LLVM function attributes, and erase "nounwind" if necessary
    m_kernelInfo.m_kernelAttributeInfo = entry->getAttributes().getAsString(-1);
    size_t nounwindLoc = m_kernelInfo.m_kernelAttributeInfo.find("nounwind");
    if (nounwindLoc != std::string::npos)
    {
        //8 is the length of "nounwind".
        //If this is not the first attribute, it has a leading space, which we also want to delete.
        int eraseLen = 8;
        if (nounwindLoc != 0)
        {
            nounwindLoc--;
            eraseLen++;
        }
        m_kernelInfo.m_kernelAttributeInfo.erase(nounwindLoc, eraseLen);
    }

    // Now fill in the special OCL attributes from the MD
    VectorTypeHintMetaDataHandle vecTypeHintInfo = funcInfoMD->getOpenCLVectorTypeHint();
    if (vecTypeHintInfo->hasValue())
    {
        m_kernelInfo.m_kernelAttributeInfo += " " + getVecTypeHintString(vecTypeHintInfo);
    }
    SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
    if (subGroupSize->hasValue())
    {
        m_kernelInfo.m_kernelAttributeInfo += " " + getSubGroupSizeString(subGroupSize);
    }
	WorkgroupWalkOrderMetaDataHandle workgroupWalkOrder = funcInfoMD->getWorkgroupWalkOrder();
	if (workgroupWalkOrder->hasValue())
	{
		m_kernelInfo.m_kernelAttributeInfo += " " + getWorkgroupWalkOrderString(workgroupWalkOrder);
	}
    ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
    if (threadGroupSize->hasValue())
    {
        m_kernelInfo.m_kernelAttributeInfo += " " + getThreadGroupSizeString(threadGroupSize, false);
    }

    ThreadGroupSizeMetaDataHandle threadGroupSizeHint = funcInfoMD->getThreadGroupSizeHint();
    if (threadGroupSizeHint->hasValue())
    {
        m_kernelInfo.m_kernelAttributeInfo += " " + getThreadGroupSizeString(threadGroupSizeHint, true);
    }
}

std::string COpenCLKernel::getThreadGroupSizeString(ThreadGroupSizeMetaDataHandle& threadGroupSize, bool isHint)
{
    std::string threadGroupSizeString = "";
    if (isHint)
    {
        threadGroupSizeString = "work_group_size_hint(";
    }
    else
    {
        threadGroupSizeString = "reqd_work_group_size(";
    }

    threadGroupSizeString += utostr(threadGroupSize->getXDim()) + ",";
    threadGroupSizeString += utostr(threadGroupSize->getYDim()) + ",";
    threadGroupSizeString += utostr(threadGroupSize->getZDim());

    threadGroupSizeString += ")";
    return threadGroupSizeString;
}
std::string COpenCLKernel::getSubGroupSizeString(SubGroupSizeMetaDataHandle& subGroupSize)
{
    std::string subTypeString = "intel_reqd_sub_group_size(";
    subTypeString += utostr(subGroupSize->getSIMD_size());
    subTypeString += ")";
    return subTypeString;
}
std::string COpenCLKernel::getWorkgroupWalkOrderString(WorkgroupWalkOrderMetaDataHandle& workgroupWalkOrder)
{
	std::string subTypeString = "intel_reqd_workgroup_walk_order(";
	subTypeString += utostr(workgroupWalkOrder->getDim0()) + ",";
	subTypeString += utostr(workgroupWalkOrder->getDim1()) + ",";
	subTypeString += utostr(workgroupWalkOrder->getDim2()) + ",";
	subTypeString += ")";
	return subTypeString;
}
std::string COpenCLKernel::getVecTypeHintString(VectorTypeHintMetaDataHandle& vecTypeHintInfo)
{
    std::string vecTypeString = "vec_type_hint(";

    // Get the information about the type
    Type* baseType = vecTypeHintInfo->getVecType()->getType();
    unsigned int numElements = 1;
    if (baseType->isVectorTy())
    {
        numElements = baseType->getVectorNumElements();
        baseType = baseType->getVectorElementType();
    }

    // Integer types need to be qualified with a "u" if they are unsigned
    if (baseType->isIntegerTy())
    {
        std::string signString = vecTypeHintInfo->getSign() ? "" : "u";
        vecTypeString += signString;
    }

    switch (baseType->getTypeID())
    {
    case Type::IntegerTyID:
        switch (baseType->getIntegerBitWidth())
        {
        case 8:
            vecTypeString += "char";
            break;
        case 16:
            vecTypeString += "short";
            break;
        case 32:
            vecTypeString += "int";
            break;
        case 64:
            vecTypeString += "long";
            break;
        default:
            assert(0 && "Unexpected data type in vec_type_hint");
            break;
        }
        break;
    case Type::DoubleTyID:
        vecTypeString += "double";
        break;
    case Type::FloatTyID:
        vecTypeString += "float";
        break;
    case Type::HalfTyID:
        vecTypeString += "half";
        break;
    default:
        assert(0 && "Unexpected data type in vec_type_hint");
        break;
    }

    if (numElements != 1)
    {
        vecTypeString += utostr(numElements);
    }

    vecTypeString += ")";

    return vecTypeString;
}

void COpenCLKernel::CreatePrintfStringAnnotations()
{
    std::string MDNodeName = "printf.strings";
    NamedMDNode *printfMDNode = entry->getParent()->getOrInsertNamedMetadata(MDNodeName);

    for (uint i = 0, NumStrings = printfMDNode->getNumOperands();
              i < NumStrings;
              i++)
    {
        iOpenCL::PrintfStringAnnotation* printfAnnotation = new iOpenCL::PrintfStringAnnotation();

        llvm::MDNode      *argMDNode    = printfMDNode->getOperand(i);

        llvm::ConstantInt *indexOpndVal = 
            mdconst::dyn_extract<llvm::ConstantInt>(argMDNode->getOperand(0));
        llvm::Metadata    *stringOpnd    = argMDNode->getOperand(1);
        llvm::MDString    *stringOpndVal = dyn_cast<llvm::MDString>(stringOpnd);
        
        llvm::StringRef stringData(stringOpndVal->getString());

        printfAnnotation->AnnotationSize          = sizeof(printfAnnotation);
        printfAnnotation->Index                   = int_cast<unsigned int>(indexOpndVal->getZExtValue());
        printfAnnotation->StringSize              = stringData.size() + 1;
        printfAnnotation->StringData              = new char[printfAnnotation->StringSize+1];

        memcpy_s(printfAnnotation->StringData, printfAnnotation->StringSize, stringData.data(), printfAnnotation->StringSize);
        printfAnnotation->StringData[printfAnnotation->StringSize - 1] = '\0';

        m_kernelInfo.m_printfStringAnnotations.push_back(printfAnnotation);
    }
}

void COpenCLKernel::CreateAnnotations(KernelArg* kernelArg, uint payloadPosition)
{
    KernelArg::ArgType type = kernelArg->getArgType();
    
    DWORD constantType                          = iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN;
    iOpenCL::IMAGE_MEMORY_OBJECT_TYPE imageType = iOpenCL::IMAGE_MEMORY_OBJECT_INVALID;
    iOpenCL::POINTER_ADDRESS_SPACE addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID;
    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

    static const DWORD DEFAULT_ARG_NUM = 0;
    const llvm::Argument* arg = kernelArg->getArg();

    switch (type) {
    
    case KernelArg::ArgType::IMPLICIT_R0:
        for (Value::const_user_iterator U = arg->user_begin(), UE = arg->user_end(); U != UE; ++U)
        {
            const ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*U);

            if (EEI)
            {
                const ConstantInt* index = dyn_cast<ConstantInt>(EEI->getIndexOperand());
                if (index)
                {
                    uint64_t value = index->getZExtValue();
                    if (value == 1 || value == 6 || value == 7)
                    {
                        // group ids x/y/z
                        if (funcInfoMD->isGroupIDPresentHasValue() && funcInfoMD->getGroupIDPresent() == 1)
                        {
                            m_kernelInfo.m_threadPayload.HasGroupID = true;
                        }
                        break;
                    }
                }
            }
        }
        break;

    case KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER:
        // PayloadHeader contains global work offset x,y,z and local size x,y,z -->
        // total of 6 annotations, 3 of each type
        for(int i = 0; i < 6; ++i)
        {
            iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation();

            DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;

            constInput->AnnotationSize      = sizeof(constInput);
            constInput->ConstantType        = (i < 3 ? 
                                                iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET : 
                                                iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE);
            constInput->Offset              = (i % 3) * sizeInBytes;
            constInput->PayloadPosition     = payloadPosition;
            constInput->PayloadSizeInBytes  = sizeInBytes;
            constInput->ArgumentNumber      = DEFAULT_ARG_NUM;
            m_kernelInfo.m_constantInputAnnotation.push_back(constInput);

            payloadPosition += sizeInBytes;
        }

        for (Value::const_user_iterator U = arg->user_begin(), UE = arg->user_end(); U != UE; ++U)
        { 
            const ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*U);

            if (EEI)
            {
                const ConstantInt* index = dyn_cast<ConstantInt>(EEI->getIndexOperand());
                if (index)
                {
                    uint64_t value = index->getZExtValue();
                    if (value == 0 || value == 1 || value == 2)
                    {
                        // global offset x/y/z
                        if (funcInfoMD->isGlobalOffsetPresentHasValue() && funcInfoMD->getGlobalOffsetPresent() == 1)
                        {
                            m_kernelInfo.m_threadPayload.HasGlobalIDOffset = true;
                        }
                        break;
                    }
                }
            }
        }
        break;
    
    case KernelArg::ArgType::PTR_GLOBAL:
        if (addressSpace == iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID) {
            addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
        }
        // Fall through until PTR_CONSTANT
    case KernelArg::ArgType::PTR_CONSTANT:
        if (addressSpace == iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID) {
            addressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_CONSTANT;
        }
        // may reach here from PTR_GLOBAL, PTR_CONSTANT
        assert(addressSpace != iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_INVALID);

        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);
    
            iOpenCL::PointerArgumentAnnotation *ptrAnnotation = new iOpenCL::PointerArgumentAnnotation();

			IGCMD::ArgAllocMetaDataHandle argInfo = funcInfoMD->getResourceAlloc()->getArgAllocsItem(argNo);
			if (argInfo->getType() == IGCMD::ResourceTypeEnum::BindlessUAVResourceType)
			{
				ptrAnnotation->IsStateless = false;
				ptrAnnotation->IsBindlessAccess = true;
			}
			else
			{
				ptrAnnotation->IsStateless = true;
				ptrAnnotation->IsBindlessAccess = false;
			}

            ptrAnnotation->AddressSpace         = addressSpace;
            ptrAnnotation->AnnotationSize       = sizeof(ptrAnnotation);
            ptrAnnotation->ArgumentNumber       = argNo;
            ptrAnnotation->BindingTableIndex    = getBTI(resInfo);
            ptrAnnotation->PayloadPosition      = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes   = kernelArg->getAllocateSize();
            ptrAnnotation->LocationIndex        = kernelArg->getLocationIndex();
            ptrAnnotation->LocationCount        = kernelArg->getLocationCount();
            ptrAnnotation->IsEmulationArgument  = kernelArg->isEmulationArgument();
            m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation );
        }
        break;

    case KernelArg::ArgType::PTR_LOCAL:
        {
            iOpenCL::LocalArgumentAnnotation *locAnnotation = new iOpenCL::LocalArgumentAnnotation();

            locAnnotation->AnnotationSize     = sizeof(locAnnotation);
            locAnnotation->Alignment          = (DWORD)kernelArg->getAlignment();
            locAnnotation->PayloadPosition    = payloadPosition;
            locAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            locAnnotation->ArgumentNumber     = kernelArg->getAssociatedArgNo();
            m_kernelInfo.m_localPointerArgument.push_back(locAnnotation);
        }
        break;

    case KernelArg::ArgType::PTR_DEVICE_QUEUE:
        {
            m_kernelInfo.m_executionEnivronment.HasDeviceEnqueue = true;
            unsigned int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PointerArgumentAnnotation *ptrAnnotation = new iOpenCL::PointerArgumentAnnotation();

            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_DEVICE_QUEUE;
            ptrAnnotation->AnnotationSize = sizeof(ptrAnnotation);
            ptrAnnotation->ArgumentNumber = argNo;
            ptrAnnotation->BindingTableIndex = getBTI(resInfo);
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            m_kernelInfo.m_pointerArgument.push_back(ptrAnnotation);
        }
        break;
    case KernelArg::ArgType::CONSTANT_REG:
        {
            uint sourceOffsetBase = 0;

            // aggregate arguments may have additional source offsets
            if (kernelArg->getStructArgOffset() != -1)
            {
                sourceOffsetBase = kernelArg->getStructArgOffset();
            }

            iOpenCL::ConstantArgumentAnnotation* constInput = new iOpenCL::ConstantArgumentAnnotation();

            DWORD sizeInBytes = kernelArg->getAllocateSize();
                
            constInput->AnnotationSize      = sizeof(constInput);
            constInput->Offset              = sourceOffsetBase;
            constInput->PayloadPosition     = payloadPosition;
            constInput->PayloadSizeInBytes  = sizeInBytes;
            constInput->ArgumentNumber      = kernelArg->getAssociatedArgNo();
            constInput->LocationIndex       = kernelArg->getLocationIndex();
            constInput->LocationCount       = kernelArg->getLocationCount();
            constInput->IsEmulationArgument = kernelArg->isEmulationArgument();
            m_kernelInfo.m_constantArgumentAnnotation.push_back(constInput);

            payloadPosition += sizeInBytes;
        }
        break;

    case KernelArg::ArgType::IMPLICIT_CONSTANT_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PointerInputAnnotation *ptrAnnotation = new iOpenCL::PointerInputAnnotation();
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_CONSTANT;
            ptrAnnotation->BindingTableIndex = 0xffffffff;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
        }
        break;

    case KernelArg::ArgType::IMPLICIT_GLOBAL_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PointerInputAnnotation *ptrAnnotation = new iOpenCL::PointerInputAnnotation();
            ptrAnnotation->AddressSpace = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_GLOBAL;
            ptrAnnotation->BindingTableIndex = 0xffffffff;
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
        }
        break;
    
    case KernelArg::ArgType::IMPLICIT_PRIVATE_BASE:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);
    
            iOpenCL::PrivateInputAnnotation *ptrAnnotation = new iOpenCL::PrivateInputAnnotation();

            ptrAnnotation->AddressSpace                 = iOpenCL::KERNEL_ARGUMENT_ADDRESS_SPACE_PRIVATE;
            ptrAnnotation->AnnotationSize               = sizeof(ptrAnnotation);
            ptrAnnotation->ArgumentNumber               = argNo;
            // PerThreadPrivateMemorySize is defined as "Total private memory requirements for each OpenCL work-item."
            ptrAnnotation->PerThreadPrivateMemorySize   = m_perWIPrivateMemSize;
            ptrAnnotation->BindingTableIndex            = getBTI(resInfo);
            ptrAnnotation->IsStateless                  = true;
            ptrAnnotation->PayloadPosition              = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes           = kernelArg->getAllocateSize();
            m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
        }
        break;

    case KernelArg::ArgType::IMPLICIT_NUM_GROUPS:
    case KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE:
    case KernelArg::ArgType::IMPLICIT_LOCAL_SIZE:
    case KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE:
    case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN:
    case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE:

        constantType = kernelArg->getDataParamToken();
        assert(constantType != iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN);
        
        for (int i = 0; i < 3; ++i)
        {
            iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation();
            
            DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;
            
            constInput->AnnotationSize      = sizeof(constInput);
            constInput->ConstantType        = constantType;
            constInput->Offset              = i * sizeInBytes;
            constInput->PayloadPosition     = payloadPosition;
            constInput->PayloadSizeInBytes  = sizeInBytes;
            constInput->ArgumentNumber      = DEFAULT_ARG_NUM;
            m_kernelInfo.m_constantInputAnnotation.push_back(constInput);

            payloadPosition += sizeInBytes;
        }

        if (type == KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN)
            m_kernelInfo.m_threadPayload.HasStageInGridOrigin = true;
        else if (type == KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE)
            m_kernelInfo.m_threadPayload.HasStageInGridSize = true;

        break;

    case KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT:
    case KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH:
    case KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH:
    case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS:
    case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE:
    case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER:
    case KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER:
    case KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE:
    case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES:
    case KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS:
    case KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED:
    case KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA:
    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DATA_PARAMETER_OBJECT_ID:
    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DISPATCHER_SIMD_SIZE:
    case KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET:
        constantType = kernelArg->getDataParamToken();
        assert(constantType != iOpenCL::DATA_PARAMETER_TOKEN_UNKNOWN);
        {
            iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation();

            constInput->AnnotationSize      = sizeof(constInput);
            constInput->ConstantType        = constantType;
            constInput->Offset              =  0;
            constInput->PayloadPosition     = payloadPosition;
            constInput->PayloadSizeInBytes  = iOpenCL::DATA_PARAMETER_DATA_SIZE;
            constInput->ArgumentNumber      = kernelArg->getAssociatedArgNo();
            m_kernelInfo.m_constantInputAnnotation.push_back(constInput);
        }
        break;

    case KernelArg::ArgType::IMAGE_1D:
    case KernelArg::ArgType::BINDLESS_IMAGE_1D:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_1D;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_1D_BUFFER:
    case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_BUFFER;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_2D:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            if (getExtensionInfo(kernelArg->getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceType) {
                imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA;
            }
            else if (getExtensionInfo(kernelArg->getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceBlockType) {
                imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK;
            }
            else {
                imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D;
            }
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_3D:
    case KernelArg::ArgType::BINDLESS_IMAGE_3D:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_3D;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_CUBE:
    case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
    case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            // Use regular cube texture for depth:
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_1D_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_1D_ARRAY;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_2D_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_2D_DEPTH:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_DEPTH;
        }
      // Fall through
    case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_DEPTH;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_2D_MSAA:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA_DEPTH;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA_DEPTH;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;
        }
        // Fall through
    case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
    case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY:
        if (imageType == iOpenCL::IMAGE_MEMORY_OBJECT_INVALID) {
            // Use regular cube texture array for depth:
            imageType = iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;
        }
        // may reach here from IMAGE_1D, IMAGE_2D, IMAGE_3D, MSAA, DEPTH, and IMAGE ARRAYS
        assert(imageType != iOpenCL::IMAGE_MEMORY_OBJECT_INVALID);
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::ImageArgumentAnnotation* imageInput = new iOpenCL::ImageArgumentAnnotation();

            imageInput->AnnotationSize              = sizeof(imageInput);
            imageInput->ArgumentNumber              = argNo;
            imageInput->IsFixedBindingTableIndex    = true;
            imageInput->BindingTableIndex           = getBTI(resInfo);
            imageInput->ImageType                   = imageType;
            imageInput->LocationIndex = kernelArg->getLocationIndex();
            imageInput->LocationCount = kernelArg->getLocationCount();
            imageInput->IsEmulationArgument = kernelArg->isEmulationArgument();

            imageInput->AccessedByFloatCoords = kernelArg->getImgAccessedFloatCoords();
            imageInput->AccessedByIntCoords   = kernelArg->getImgAccessedIntCoords();
			imageInput->IsBindlessAccess = kernelArg->needsAllocation();
			imageInput->PayloadPosition = payloadPosition;

            switch (resInfo.Type)
            {
            case SOpenCLKernelInfo::SResourceInfo::RES_UAV:
                if(kernelArg->getAccessQual() == IGC::KernelArg::AccessQual::READ_ONLY)
                    imageInput->Writeable = false;
                else
                    imageInput->Writeable = true;
                break;
            case SOpenCLKernelInfo::SResourceInfo::RES_SRV:
                imageInput->Writeable = false;
                break;
            default:
                assert(0 && "Unknown resource type");
            }
            m_kernelInfo.m_imageInputAnnotations.push_back(imageInput);

            if(kernelArg->getAccessQual() == IGC::KernelArg::AccessQual::READ_WRITE)
            {
                m_kernelInfo.m_executionEnivronment.HasReadWriteImages = true;
            }
        }
        break;

    case KernelArg::ArgType::SAMPLER:
    case KernelArg::ArgType::BINDLESS_SAMPLER:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = resInfo.Index;

            iOpenCL::SAMPLER_OBJECT_TYPE samplerType;
            if (getExtensionInfo(argNo) == ResourceExtensionTypeEnum::MediaSamplerType) {
                samplerType = iOpenCL::SAMPLER_OBJECT_VME;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeConvolve ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_2DCONVOLVE;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeErode ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_ERODE;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeDilate ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_DILATE;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeMinMaxFilter ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAXFILTER;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeMinMax ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAX;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeCentroid ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_CENTROID;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeBoolCentroid ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_CENTROID;
            }
            else if( getExtensionInfo( argNo ) == ResourceExtensionTypeEnum::MediaSamplerTypeBoolSum ) {
                samplerType = iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_SUM;
            }
            else {
                samplerType = iOpenCL::SAMPLER_OBJECT_TEXTURE;
            }

            iOpenCL::SamplerArgumentAnnotation* samplerArg  = new iOpenCL::SamplerArgumentAnnotation();
            samplerArg->AnnotationSize                      = sizeof(samplerArg);
            samplerArg->SamplerType                         = samplerType;
            samplerArg->ArgumentNumber                      = argNo;
            samplerArg->SamplerTableIndex                   = resInfo.Index;
            samplerArg->LocationIndex                       = kernelArg->getLocationIndex();
            samplerArg->LocationCount                       = kernelArg->getLocationCount();
			samplerArg->IsBindlessAccess					= kernelArg->needsAllocation();
            samplerArg->IsEmulationArgument                 = kernelArg->isEmulationArgument();
			samplerArg->PayloadPosition = payloadPosition;

            m_kernelInfo.m_samplerArgument.push_back(samplerArg);
        }
        break;

    case KernelArg::ArgType::IMPLICIT_LOCAL_IDS:
        m_kernelInfo.m_threadPayload.HasLocalIDx    = true;
        m_kernelInfo.m_threadPayload.HasLocalIDy    = true;
        m_kernelInfo.m_threadPayload.HasLocalIDz    = true;
        if (funcInfoMD->isLocalIDPresentHasValue() && funcInfoMD->getLocalIDPresent() == 1)
        {
            m_kernelInfo.m_threadPayload.HasLocalID = true;
        }
        break;

    case KernelArg::ArgType::R1:
        m_kernelInfo.m_threadPayload.UnusedPerThreadConstantPresent = true;
        break;

    case KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER:
        {
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PrintfBufferAnnotation  *printfBuffer = new iOpenCL::PrintfBufferAnnotation();

            printfBuffer->AnnotationSize  = sizeof(printfBuffer);
            printfBuffer->ArgumentNumber  = argNo;
            printfBuffer->PayloadPosition = payloadPosition;
            printfBuffer->DataSize        = kernelArg->getAllocateSize();
            printfBuffer->Index           = 0; // This value is not used by Runtime.
    
            m_kernelInfo.m_printfBufferAnnotation = printfBuffer;
        }
        break;

    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE:
        {
            m_kernelInfo.m_executionEnivronment.HasDeviceEnqueue = true;
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PointerInputAnnotation *ptrAnnotation = new iOpenCL::PointerInputAnnotation();

            ptrAnnotation->AddressSpace = iOpenCL::ADDRESS_SPACE_INTERNAL_DEFAULT_DEVICE_QUEUE;
            ptrAnnotation->BindingTableIndex = getBTI(resInfo);
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
        }
        break;

    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL:
        {
            m_kernelInfo.m_executionEnivronment.HasDeviceEnqueue = true;
            int argNo = kernelArg->getAssociatedArgNo();
            SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(argNo);
            m_kernelInfo.m_argIndexMap[argNo] = getBTI(resInfo);

            iOpenCL::PointerInputAnnotation *ptrAnnotation = new iOpenCL::PointerInputAnnotation();
            ptrAnnotation->AddressSpace = iOpenCL::ADDRESS_SPACE_INTERNAL_EVENT_POOL;
            ptrAnnotation->BindingTableIndex = getBTI(resInfo);
            ptrAnnotation->IsStateless = true;
            ptrAnnotation->PayloadPosition = payloadPosition;
            ptrAnnotation->PayloadSizeInBytes = kernelArg->getAllocateSize();
            ptrAnnotation->ArgumentNumber = argNo;
            m_kernelInfo.m_pointerInput.push_back(ptrAnnotation);
        }
        break;

    case KernelArg::ArgType::IMPLICIT_WORK_DIM:
    case KernelArg::ArgType::IMPLICIT_VME_MB_BLOCK_TYPE:
    case KernelArg::ArgType::IMPLICIT_VME_SUBPIXEL_MODE:
    case KernelArg::ArgType::IMPLICIT_VME_SAD_ADJUST_MODE:
    case KernelArg::ArgType::IMPLICIT_VME_SEARCH_PATH_TYPE:
    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE:
    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PARENT_EVENT:
    case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE:
        constantType = kernelArg->getDataParamToken();
        {
            iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation();
            
            DWORD sizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;
            
            constInput->AnnotationSize      = sizeof(constInput);
            constInput->ConstantType        = constantType;
            constInput->Offset              = 0;
            constInput->PayloadPosition     = payloadPosition;
            constInput->PayloadSizeInBytes  = sizeInBytes;
            constInput->ArgumentNumber      = DEFAULT_ARG_NUM;
            m_kernelInfo.m_constantInputAnnotation.push_back(constInput);

            payloadPosition += sizeInBytes;
        }
        break;
    case KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS:
        {
            iOpenCL::StartGASAnnotation* GASStart = new iOpenCL::StartGASAnnotation();
            GASStart->Offset = payloadPosition;
            GASStart->gpuPointerSizeInBytes = kernelArg->getAllocateSize();
            m_kernelInfo.m_startGAS = GASStart;
        }
        break;
    case KernelArg::ArgType::IMPLICIT_LOCAL_MEMORY_STATELESS_WINDOW_SIZE:
        {
        iOpenCL::WindowSizeGASAnnotation* winSizeGAS = new iOpenCL::WindowSizeGASAnnotation();
   
        winSizeGAS->Offset = payloadPosition;
        m_kernelInfo.m_WindowSizeGAS = winSizeGAS;
        }
        break;
    case KernelArg::ArgType::IMPLICIT_PRIVATE_MEMORY_STATELESS_SIZE:
        {
        iOpenCL::PrivateMemSizeAnnotation* privateMemSize = new iOpenCL::PrivateMemSizeAnnotation();

        privateMemSize->Offset = payloadPosition;
        m_kernelInfo.m_PrivateMemSize = privateMemSize;
        }
        break;
    default:
        // OCLTODO: What about PTR_LOCAL? no annotation?
        // Do nothing
        break;
    }

    // todo: get it from per-kernel data.
    m_kernelInfo.m_executionEnivronment.NumGRFRequired = m_Context->getNumGRFPerThread();

    // DATA_PARAMETER_BUFFER_STATEFUL
    //   ( SPatchDataParameterBuffer for this token only uses one field: ArgumentNumber )
    //   Used to indicate that all memory references via a gobal/constant ptr argument are
    //   converted to stateful (by StatelessToStateful optimization). Thus, the ptr itself
    //   is no longer referenced at all.
    //
    if (IGC_IS_FLAG_ENABLED(EnableStatelessToStatefull) &&
        IGC_IS_FLAG_ENABLED(EnableStatefulToken) &&
        m_DriverInfo->SupportStatefulToken() &&
        arg && arg->use_empty() &&
        (type == KernelArg::ArgType::PTR_GLOBAL ||
         type == KernelArg::ArgType::PTR_CONSTANT))
    {
        iOpenCL::ConstantInputAnnotation* constInput = new iOpenCL::ConstantInputAnnotation();

        constInput->AnnotationSize = sizeof(constInput);
        constInput->ConstantType = iOpenCL::DATA_PARAMETER_BUFFER_STATEFUL;
        constInput->Offset = 0;
        constInput->PayloadPosition = payloadPosition;
        constInput->PayloadSizeInBytes = iOpenCL::DATA_PARAMETER_DATA_SIZE;
        constInput->ArgumentNumber = kernelArg->getAssociatedArgNo(); // used only for this token.
        m_kernelInfo.m_constantInputAnnotation.push_back(constInput);
    }
}

void COpenCLKernel::ParseShaderSpecificOpcode( llvm::Instruction* inst )
{
    // Currently we see data corruption when we have IEEE macros and midthread preemption enabled.
    // Adding a temporary work around to disable mid thread preemption when we see IEEE Macros.
    switch( inst->getOpcode() )
    {
    case Instruction::FDiv:
        if(inst->getType()->isDoubleTy())
        {
            SetDisableMidthreadPreemption();
        }
        break;
    case Instruction::Call:
        if(inst->getType()->isDoubleTy())
        {
            if (GetOpCode(inst) == llvm_sqrt)
            {
                SetDisableMidthreadPreemption();
            }
        }
        break;
    default:
        break;
    }
}

void COpenCLKernel::AllocatePayload()
{
    assert(m_Context);

    bool loadThreadPayload = false;


    // SKL defaults to indirect thread payload storage.
    // BDW needs CURBE payload. Spec says:
    // "CURBE should be used for the payload when using indirect dispatch rather than indirect payload".
    m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
    if(IGC_IS_FLAG_ENABLED(DisableGPGPUIndirectPayload) || 
       m_Context->platform.getWATable().WaDisableIndirectDataForIndirectDispatch)
    {
        m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = false;
    }
    if (loadThreadPayload)
    {
        m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
    }
    m_kernelInfo.m_threadPayload.HasFlattenedLocalID = false;
    m_kernelInfo.m_threadPayload.HasLocalIDx = false;
    m_kernelInfo.m_threadPayload.HasLocalIDy = false;
    m_kernelInfo.m_threadPayload.HasLocalIDz = false;
    m_kernelInfo.m_threadPayload.HasGlobalIDOffset = false;
    m_kernelInfo.m_threadPayload.HasGroupID = false;
    m_kernelInfo.m_threadPayload.HasLocalID = false;
    m_kernelInfo.m_threadPayload.UnusedPerThreadConstantPresent = false;
    m_kernelInfo.m_printfBufferAnnotation = nullptr;
    m_kernelInfo.m_threadPayload.HasStageInGridOrigin = false;
    m_kernelInfo.m_threadPayload.HasStageInGridSize = false;

    m_kernelInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = 0;
    m_kernelInfo.m_threadPayload.PassInlineData = false;

    // Set the amount of the private memory used by the kernel
    // Set only if the private memory metadata actually exists and we don't use
    // scratch space for private memory.
    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);
    bool noScratchSpacePrivMem = !m_Context->getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory;
    if (noScratchSpacePrivMem && funcInfoMD->isPrivateMemoryPerWIHasValue()) 
    {
        m_perWIPrivateMemSize = funcInfoMD->getPrivateMemoryPerWI();
    }

    m_ConstantBufferLength  = 0;
    m_NOSBufferSize         = 0;

    uint offset                 = 0;
    
    uint constantBufferStart    = 0;
    uint constantBufferEnd      = 0;

    bool constantBufferStartSet = false;
    bool constantBufferEndSet   = false;
    
    KernelArgsOrder::InputType layout = 
            m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage ?
            KernelArgsOrder::InputType::INDIRECT :
            KernelArgsOrder::InputType::CURBE;

    KernelArgs kernelArgs(*entry, m_DL, m_pMdUtils, layout, m_ModuleMetadata);

    if (layout == KernelArgsOrder::InputType::INDIRECT && !loadThreadPayload)
    {
        kernelArgs.checkForZeroPerThreadData();
    }

    for (KernelArgs::const_iterator i = kernelArgs.begin(), e = kernelArgs.end(); i != e; ++i) {
        KernelArg arg = *i;

		// For now, only check BUFFER_OFFSET arguments (may move it into KernelArg class)
		bool IsUnusedArg = (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET &&
			                IGC_IS_FLAG_ENABLED(EnableOptionalBufferOffset) &&
			                arg.getArg()->use_empty());

        if (!constantBufferStartSet && arg.isConstantBuf()) {
            constantBufferStart = offset;
            AllocateNOSConstants(offset);
            constantBufferStartSet = true;
        }

        // Local IDs are non-uniform and may have two instances in SIMD32 mode
        int numAllocInstances = arg.getArgType() == KernelArg::ArgType::IMPLICIT_LOCAL_IDS ? m_numberInstance : 1;

        if (arg.needsAllocation() && !IsUnusedArg)
        {
            // Align on the desired alignment for this argument
            offset = iSTD::Align(offset, arg.getAlignment());

            // Arguments larger than a GRF must be at least GRF-aligned.
            // Arguments smaller than a GRF may not cross GRF boundaries.
            // This means that arguments that cross a GRF boundary
            // must be GRF aligned.
            // Note that this is done AFTER we align on the base alignment,
            // because of edge cases where aligning on the base alignment
            // is what causes the "overflow".
            unsigned int startGRF = offset / SIZE_GRF;
            unsigned int endGRF = (offset + arg.getAllocateSize() - 1) / SIZE_GRF;
            if (startGRF != endGRF)
            {
                offset = iSTD::Align(offset, SIZE_GRF);
            }

            // And now actually tell vISA we need this space.
            // (Except for r0, which is a predefined variable, and should never be allocated as input!)
            const llvm::Argument * A = arg.getArg();
            if(A != nullptr && arg.getArgType() != KernelArg::ArgType::IMPLICIT_R0) 
            {
                CVariable* var = GetSymbol(const_cast<Argument*>(A));
                for (int i = 0; i < numAllocInstances; ++i)
                {
                    AllocateInput(var, offset + (arg.getAllocateSize() * i), i);
                }
            }
            // or else we would just need to increase an offset
        }

        // Create annotations for the kernel argument
		// If an arg is unused, don't generate patch token for it.
		if (!IsUnusedArg)
		{
			CreateAnnotations(&arg, offset - constantBufferStart);
		}

        if (arg.needsAllocation() && !IsUnusedArg)
        {
            for (int i = 0; i < numAllocInstances; ++i)
            {
                offset += arg.getAllocateSize();
            }
        }
    }

    if (!constantBufferEndSet && constantBufferStartSet) {
        constantBufferEnd = offset;
    }

    // ToDo: we should avoid passing all three dimensions of local id
    if (m_kernelInfo.m_threadPayload.HasLocalIDx || 
        m_kernelInfo.m_threadPayload.HasLocalIDy ||
        m_kernelInfo.m_threadPayload.HasLocalIDz)
    {    
        if (loadThreadPayload)
        {
            uint perThreadInputSize = SIZE_GRF * 3 * m_numberInstance;
            encoder.GetVISAKernel()->AddKernelAttribute("perThreadInputSize", sizeof(uint16_t), &perThreadInputSize);
        }
    }
    
    assert(constantBufferEnd >= constantBufferStart && "Constant buffer size should be non negative!");
    m_ConstantBufferLength = constantBufferEnd - constantBufferStart;
    m_ConstantBufferLength = iSTD::Align(m_ConstantBufferLength, SIZE_GRF);

    CreateInlineSamplerAnnotations();

    // Handle kernel reflection
    CreateKernelArgInfo();
    CreateKernelAttributeInfo();

    // Create annotations for printf string.
    CreatePrintfStringAnnotations();
}

unsigned int COpenCLKernel::GetGlobalMappingValue(llvm::Value* c)
{
    unsigned int val = 0;
    auto localIter = m_localOffsetsMap.find(c);
    if (localIter != m_localOffsetsMap.end())
    {
        val = localIter->second;
    }
    else
    {
        assert(0 && "Trying to access a GlobalVariable not in locals map");
    }
    return val;
}

CVariable* COpenCLKernel::GetGlobalMapping(llvm::Value* c)
{
    unsigned int val = GetGlobalMappingValue(c);
    
    VISA_Type type = GetType(c->getType());
    return ImmToVariable(val, type); 
}

unsigned int COpenCLKernel::getSumFixedTGSMSizes(Function* F)
{
    // Find whether we have size information for this kernel.
    // If not, then the total TGSM is 0, otherwise pull it from the MD
    auto i = m_pMdUtils->findFunctionsInfoItem(F);
    if (i == m_pMdUtils->end_FunctionsInfo())
    {
        return 0;
    }
    return i->second->getLocalSize();
}

void COpenCLKernel::FillKernel()
{
    FillGTPinRequest(&(m_kernelInfo.m_kernelProgram));
    m_kernelInfo.m_executionEnivronment.PerThreadSpillFillSize = ProgramOutput()->m_scratchSpaceUsedBySpills;
    m_kernelInfo.m_executionEnivronment.PerThreadScratchSpace = ProgramOutput()->m_scratchSpaceUsedByShader;
    m_kernelInfo.m_kernelProgram.NOSBufferSize = m_NOSBufferSize / SIZE_GRF; // in 256 bits
    m_kernelInfo.m_kernelProgram.ConstantBufferLength = m_ConstantBufferLength / SIZE_GRF; // in 256 bits
    m_kernelInfo.m_kernelProgram.MaxNumberOfThreads = m_Platform->getMaxGPGPUShaderThreads();

    m_kernelInfo.m_executionEnivronment.SumFixedTGSMSizes = getSumFixedTGSMSizes(entry);
    m_kernelInfo.m_executionEnivronment.HasBarriers = this->GetHasBarrier();
    m_kernelInfo.m_executionEnivronment.DisableMidThreadPreemption = GetDisableMidThreadPreemption();
    m_kernelInfo.m_executionEnivronment.SubgroupIndependentForwardProgressRequired = 
        m_Context->getModuleMetaData()->compOpt.SubgroupIndependentForwardProgressRequired;
    m_kernelInfo.m_executionEnivronment.CompiledForGreaterThan4GBBuffers = 
        m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired;

    assert(gatherMap.size() == 0);
    m_kernelInfo.m_kernelProgram.gatherMapSize = 0;
    m_kernelInfo.m_kernelProgram.bindingTableEntryCount = 0;

    m_kernelInfo.m_executionEnivronment.HasDeviceEnqueue = false;
    m_kernelInfo.m_executionEnivronment.IsSingleProgramFlow = false;
    m_kernelInfo.m_executionEnivronment.PerSIMDLanePrivateMemorySize = m_perWIPrivateMemSize;
    m_kernelInfo.m_executionEnivronment.HasFixedWorkGroupSize = false;
    m_kernelInfo.m_kernelName = entry->getName();
    m_kernelInfo.m_ShaderHashCode = m_Context->hash.getAsmHash();

    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);
    ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
    SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
	WorkgroupWalkOrderMetaDataHandle workgroupWalkOrder = funcInfoMD->getWorkgroupWalkOrder();
    if(threadGroupSize->hasValue())
    {
        m_kernelInfo.m_executionEnivronment.HasFixedWorkGroupSize = true;
        m_kernelInfo.m_executionEnivronment.FixedWorkgroupSize[0] = threadGroupSize->getXDim();
        m_kernelInfo.m_executionEnivronment.FixedWorkgroupSize[1] = threadGroupSize->getYDim();
        m_kernelInfo.m_executionEnivronment.FixedWorkgroupSize[2] = threadGroupSize->getZDim();
    }
    if(subGroupSize->hasValue())
    {
        m_kernelInfo.m_executionEnivronment.CompiledSIMDSize = subGroupSize->getSIMD_size();
    }

	if (workgroupWalkOrder->hasValue())
	{
		m_kernelInfo.m_executionEnivronment.WorkgroupWalkOrder[0] = workgroupWalkOrder->getDim0();
		m_kernelInfo.m_executionEnivronment.WorkgroupWalkOrder[1] = workgroupWalkOrder->getDim1();
		m_kernelInfo.m_executionEnivronment.WorkgroupWalkOrder[2] = workgroupWalkOrder->getDim2();
	}
 
    auto &FuncMap = m_Context->getModuleMetaData()->FuncMD;
    auto FuncIter = FuncMap.find(entry);
    if (FuncIter != FuncMap.end())
    {
        auto &FuncInfo = FuncIter->second;

        m_kernelInfo.m_executionEnivronment.IsInitializer = FuncInfo.IsInitializer;
        m_kernelInfo.m_executionEnivronment.IsFinalizer   = FuncInfo.IsFinalizer;

        m_kernelInfo.m_executionEnivronment.CompiledSubGroupsNumber =
            FuncInfo.CompiledSubGroupsNumber;
    }
}

void COpenCLKernel::RecomputeBTLayout()
{
    ResourceAllocMetaDataHandle resourceAlloc = m_pMdUtils->getFunctionsInfoItem(entry)->getResourceAlloc();
    assert(resourceAlloc->hasValue() && "Resource Allocation information not present");
    // Get the number of UAVs and Resources from MD.
    int numUAVs = resourceAlloc->getUAVsNum();
    int numResources = resourceAlloc->getSRVsNum();

    // Now, update the layout information
    USC::SShaderStageBTLayout* layout = ((COCLBTILayout *)m_pBtiLayout)->getModifiableLayout();

    // The BT layout contains the minimum and the maximum number BTI for each kind
    // of resource. E.g. UAVs may be mapped to BTIs 0..3, SRVs to 4..5, and the scratch
    // surface to 6.
    // Note that the names are somewhat misleading. They are used for the sake of consistency 
    // with the ICBE sources.

    // Some fields are always 0 for OCL.
    layout->resourceNullBoundOffset = 0;
    layout->immediateConstantBufferOffset = 0;
    layout->interfaceConstantBufferOffset = 0;
    layout->constantBufferNullBoundOffset = 0;
    layout->JournalIdx = 0;
    layout->JournalCounterIdx = 0;

    // And TGSM (aka SLM) is always 254.
    layout->TGSMIdx = 254;

    int index = 0;

    // First, allocate BTI for debug surface
    if (m_Context->m_InternalOptions.KernelDebugEnable)
    {
        layout->systemThreadIdx = index++;
    }

    // Now, allocate BTIs for all the SRVs.
    layout->minResourceIdx = index;
    if (numResources)
    {
        index += numResources - 1;
        layout->maxResourceIdx = index++;
    }
    else
    {
        layout->maxResourceIdx = index;
    }

    // Now, ConstantBuffers - used as a placeholder for the inline constants, if present.
    layout->minConstantBufferIdx = index;
    layout->maxConstantBufferIdx = index;

    // Now, the UAVs
    layout->minUAVIdx = index;
    if (numUAVs)
    {
        index += numUAVs - 1;
        layout->maxUAVIdx = index++;
    }
    else
    {
        layout->maxUAVIdx = index;
    }

    // And finally, the scratch surface
    layout->surfaceScratchIdx = index++;
    
    // Overall number of used BT entries, not including TGSM.
    layout->maxBTsize = index;
}

bool COpenCLKernel::HasFullDispatchMask()
{
    unsigned int groupSize = IGCMetaDataHelper::getThreadGroupSize(*m_pMdUtils, entry);
    if(groupSize != 0)
    {
        if(groupSize % numLanes(m_dispatchSize) == 0)
        {
            return true;
        }
    }
    return false;
}

unsigned int COpenCLKernel::getBTI(SOpenCLKernelInfo::SResourceInfo& resInfo)
{
    switch (resInfo.Type)
    {
        case SOpenCLKernelInfo::SResourceInfo::RES_UAV:
            return m_pBtiLayout->GetUavIndex(resInfo.Index);
        case SOpenCLKernelInfo::SResourceInfo::RES_SRV:
            return m_pBtiLayout->GetTextureIndex(resInfo.Index);
        default:
            return 0xffffffff;
    }
}

void CollectProgramInfo(OpenCLProgramContext* ctx)
{    
    MetaDataUtils mdUtils(ctx->getModule());
	ModuleMetaData *modMD = ctx->getModuleMetaData();

    if (!modMD->inlineConstantBuffers.empty())
    {
        auto ipsbMDHandle = modMD->inlineConstantBuffers[0];
        std::unique_ptr<iOpenCL::InitConstantAnnotation> initConstant(new iOpenCL::InitConstantAnnotation());
        initConstant->Alignment = ipsbMDHandle.alignment;

        size_t bufferSize = (ipsbMDHandle.Buffer).size();
        initConstant->InlineData.resize(bufferSize);
        for (size_t i = 0; i < bufferSize; ++i)
        {
            initConstant->InlineData[i] = (ipsbMDHandle.Buffer)[i];
        }

        ctx->m_programInfo.m_initConstantAnnotation.push_back(std::move(initConstant));
    }

    if (!modMD->inlineGlobalBuffers.empty())
    {
        auto ipsbMDHandle = modMD->inlineGlobalBuffers[0];

        std::unique_ptr<iOpenCL::InitGlobalAnnotation> initGlobal(new iOpenCL::InitGlobalAnnotation());
        initGlobal->Alignment = ipsbMDHandle.alignment;

        size_t bufferSize = (ipsbMDHandle.Buffer).size();
        initGlobal->InlineData.resize(bufferSize);
        for (size_t i = 0; i < bufferSize; ++i)
        {
            initGlobal->InlineData[i] = (ipsbMDHandle.Buffer)[i];
        }

        ctx->m_programInfo.m_initGlobalAnnotation.push_back(std::move(initGlobal));
    }

	{
        auto &FuncMap = ctx->getModuleMetaData()->FuncMD;
        for (auto i : FuncMap)
        {
            std::unique_ptr<iOpenCL::KernelTypeProgramBinaryInfo> initConstant(new iOpenCL::KernelTypeProgramBinaryInfo());
            initConstant->KernelName = i.first->getName();
            if (i.second.IsFinalizer)
            {
                
                initConstant->Type = iOpenCL::PROGRAM_SCOPE_KERNEL_DESTRUCTOR;
                ctx->m_programInfo.m_initKernelTypeAnnotation.push_back(std::move(initConstant));
            }
            else if (i.second.IsInitializer)
            {
                initConstant->Type = iOpenCL::PROGRAM_SCOPE_KERNEL_CONSTRUCTOR;
                ctx->m_programInfo.m_initKernelTypeAnnotation.push_back(std::move(initConstant));
            }

        } 
    }

    for (auto iter = modMD->GlobalPointerProgramBinaryInfos.begin();
         iter != modMD->GlobalPointerProgramBinaryInfos.end();
         iter++)
    {
        std::unique_ptr<iOpenCL::GlobalPointerAnnotation> initGlobalPointer(new iOpenCL::GlobalPointerAnnotation());
		initGlobalPointer->PointeeAddressSpace = iter->PointeeAddressSpace;
		initGlobalPointer->PointeeBufferIndex  = iter->PointeeBufferIndex;
		initGlobalPointer->PointerBufferIndex  = iter->PointerBufferIndex; 
		initGlobalPointer->PointerOffset       = iter->PointerOffset;
        ctx->m_programInfo.m_initGlobalPointerAnnotation.push_back(std::move(initGlobalPointer));
    }

    for (auto iter = modMD->ConstantPointerProgramBinaryInfos.begin();
         iter != modMD->ConstantPointerProgramBinaryInfos.end();
         iter++)
    {
        std::unique_ptr<iOpenCL::ConstantPointerAnnotation> initConstantPointer(new iOpenCL::ConstantPointerAnnotation());
		initConstantPointer->PointeeAddressSpace = iter->PointeeAddressSpace;
		initConstantPointer->PointeeBufferIndex  = iter->PointeeBufferIndex;
		initConstantPointer->PointerBufferIndex  = iter->PointerBufferIndex;
		initConstantPointer->PointerOffset       = iter->PointerOffset;

        ctx->m_programInfo.m_initConstantPointerAnnotation.push_back(std::move(initConstantPointer));
    }
}

void GatherDataForDriver(OpenCLProgramContext* ctx, COpenCLKernel* pShader, CShaderProgram *pKernel, Function* pFunc, MetaDataUtils *pMdUtils)
{
    assert(pShader != nullptr);
    pShader->FillKernel();
    SProgramOutput* pOutput = pShader->ProgramOutput();

    //  Need a better heuristic for NoRetry
    FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(pFunc);
    int subGrpSize = funcInfoMD->getSubGroupSize()->getSIMD_size();
    bool noRetry = ((subGrpSize > 0 || pOutput->m_scratchSpaceUsedBySpills < 1000) &&
        ctx->m_instrTypes.mayHaveIndirectOperands);

    bool fullDebugInfo = false;
    if (ctx->m_instrTypes.hasDebugInfo)
    {
        IF_DEBUG_INFO(bool hasLineNumber = false;)
            IF_DEBUG_INFO(IGC::DebugMetadataInfo::hasAnyDebugInfo(ctx, fullDebugInfo, hasLineNumber);)
    }

    if (pOutput->m_scratchSpaceUsedBySpills == 0 ||
        noRetry ||
        ctx->m_retryManager.IsLastTry() ||
        fullDebugInfo)
    {
        // Save the shader program to the state processor to be handled later
        if (ctx->m_programOutput.m_ShaderProgramList.size() == 0 ||
            ctx->m_programOutput.m_ShaderProgramList.back() != pKernel)
        {
            ctx->m_programOutput.m_ShaderProgramList.push_back(pKernel);
        }
        COMPILER_SHADER_STATS_PRINT(pKernel->m_shaderStats, ShaderType::OPENCL_SHADER, ctx->hash, ctx->GetStr(pFunc));
        COMPILER_SHADER_STATS_SUM(ctx->m_sumShaderStats, pKernel->m_shaderStats, ShaderType::OPENCL_SHADER);
        COMPILER_SHADER_STATS_DEL(pKernel->m_shaderStats);
    }
    else
    {
        ctx->m_retryManager.kernelSet.insert(pShader->m_kernelInfo.m_kernelName);
    }
}

static bool SetKernelProgram(COpenCLKernel* shader, DWORD simdMode)
{
    if (shader && shader->ProgramOutput()->m_programSize > 0)
    {
        if (simdMode == 32)
        {
            shader->m_kernelInfo.m_executionEnivronment.PerThreadSpillFillSize =
                shader->ProgramOutput()->m_scratchSpaceUsedBySpills;
            shader->m_kernelInfo.m_kernelProgram.simd32 = *shader->ProgramOutput();
        }
        else if (simdMode == 16)
        {
            shader->m_kernelInfo.m_kernelProgram.simd16 = *shader->ProgramOutput();
        }
        else if (simdMode == 8)
        {
            shader->m_kernelInfo.m_kernelProgram.simd8 = *shader->ProgramOutput();
        }   
        shader->m_kernelInfo.m_executionEnivronment.CompiledSIMDSize = simdMode;
        return true;
    }
    return false;
}

void CodeGen(OpenCLProgramContext* ctx)
{
    // Do program-wide code generation.
    // Currently, this just creates the program-scope patch stream.
    if (ctx->m_retryManager.IsFirstTry())
    {
        CollectProgramInfo(ctx);
        ctx->m_programOutput.CreateProgramScopePatchStream(ctx->m_programInfo);
    }

    MetaDataUtils *pMdUtils = ctx->getMetaDataUtils();

    //Clear spill parameters of retry manager in the very begining of code gen
	ctx->m_retryManager.ClearSpillParams();

    CShaderProgram::KernelShaderMap shaders;
    CodeGen(ctx, shaders);

    if (ctx->m_programOutput.m_pSystemThreadKernelOutput == nullptr)
    {
        const auto options = ctx->m_InternalOptions;
        if (options.IncludeSIPCSR         ||
            options.IncludeSIPKernelDebug ||
            options.IncludeSIPKernelDebugWithLocalMemory ||
            options.KernelDebugEnable)
        {
            DWORD systemThreadMode = 0;

            if (options.IncludeSIPCSR)
            {
                systemThreadMode |= USC::SYSTEM_THREAD_MODE_CSR;
            }

            if (options.KernelDebugEnable ||
                options.IncludeSIPKernelDebug)
            {
                systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG;
            }

            if (options.IncludeSIPKernelDebugWithLocalMemory)
            {
                systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG_LOCAL;
            }

            SIP::CSystemThread::CreateSystemThreadKernel(
                ctx->platform,
                (USC::SYSTEM_THREAD_MODE)systemThreadMode,
                ctx->m_programOutput.m_pSystemThreadKernelOutput);
        }
    }

    ctx->m_retryManager.kernelSet.clear();

    // gather data to send back to the driver
    for (auto k : shaders)
    {
        Function* pFunc = k.first;
        CShaderProgram *pKernel = static_cast<CShaderProgram*>(k.second);
        COpenCLKernel* simd8Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD8));
        COpenCLKernel* simd16Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD16));
        COpenCLKernel* simd32Shader = static_cast<COpenCLKernel*>(pKernel->GetShader(SIMDMode::SIMD32));

        if (ctx->m_DriverInfo.sendMultipleSIMDModes())
        {
            //Gather the kernel binary for each compiled kernel
            if (SetKernelProgram(simd32Shader, 32))
                GatherDataForDriver(ctx, simd32Shader, pKernel, pFunc, pMdUtils);
            if (SetKernelProgram(simd16Shader, 16))
                GatherDataForDriver(ctx, simd16Shader, pKernel, pFunc, pMdUtils);
            if (SetKernelProgram(simd8Shader, 8))
                GatherDataForDriver(ctx, simd8Shader, pKernel, pFunc, pMdUtils);
        }
        else
        {
            //Gather the kernel binary only for 1 SIMD mode of the kernel
            if (SetKernelProgram(simd32Shader, 32))
                GatherDataForDriver(ctx, simd32Shader, pKernel, pFunc, pMdUtils);
            else if (SetKernelProgram(simd16Shader, 16))
                GatherDataForDriver(ctx, simd16Shader, pKernel, pFunc, pMdUtils);
            else if (SetKernelProgram(simd8Shader, 8))
                GatherDataForDriver(ctx, simd8Shader, pKernel, pFunc, pMdUtils);
        }
    }
}

bool COpenCLKernel::hasReadWriteImage(llvm::Function &F)
{
    if (!isEntryFunc(m_pMdUtils, &F))
    {
        // Ignore read/write flags for subroutines for now.
        // TODO: get access types for subroutines without using kernel args
        return false;
    }

    KernelArgs kernelArgs(F, m_DL, m_pMdUtils, KernelArgsOrder::InputType::INDEPENDENT, m_ModuleMetadata);
    for (auto KA : kernelArgs)
    {
        // RenderScript annotation sets "read_write" qualifier 
        // for any applicable kernel argument, not only for kernel arguments 
        // that are images, so we should check if kernel argument is an image.
        if(KA.getAccessQual() == KernelArg::AccessQual::READ_WRITE &&
           KA.getArgType() >= KernelArg::ArgType::IMAGE_1D &&
           KA.getArgType() <= KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY)
        {
            return true;
        }
    }
    return false;
}

bool COpenCLKernel::CompileSIMDSize(SIMDMode simdMode, EmitPass &EP, llvm::Function &F)
{
    //If the driver has forced a specific SIMD mode, then:
    // 1. compile only that SIMD mode and nothing else
    // 2. Compille that SIMD mode even if it is not profitable, i.e. even if compileThisSIMD = false for it.
    //    Don't bother checking profitability for it  
    if (m_Context->getModuleMetaData()->csInfo.forcedSIMDModeFromDriver != 0)
    {
        //Entered here means driver has requested a specific SIMD mode
        if ((simdMode == SIMDMode::SIMD8 && m_Context->getModuleMetaData()->csInfo.forcedSIMDModeFromDriver == 8) ||
            (simdMode == SIMDMode::SIMD16 && m_Context->getModuleMetaData()->csInfo.forcedSIMDModeFromDriver == 16) ||
            (simdMode == SIMDMode::SIMD32 && m_Context->getModuleMetaData()->csInfo.forcedSIMDModeFromDriver == 32))
        {
            m_Context->setDefaultSIMDMode(simdMode);
            return true;
        }
        return false;
    }

    bool compileThisSIMD = CompileThisSIMD(simdMode, EP, F);
    
    SIMDMode origSIMDMode = m_Context->getDefaultSIMDMode();

    //if compilation SIMD mode is true then we are guaranteed to compile this mode.
    //Hence set the default compile SIMD mode to the new SIMD configuration
    //Note: This mode is now the default, if compileThisSIMD is false, then we 
    //will try to compile it if multiple simd mode is enabled but we will not set 
    //it as a default mode. That is why this check has to happen immediately and 
    //cannot be moved.
    if (compileThisSIMD)
    {
        if (simdMode > origSIMDMode)
            m_Context->setDefaultSIMDMode(simdMode);
    }

    //Even if compile SIMD mode fails we want to check if multiple SIMD mode is enabled
    if (!compileThisSIMD)
    {
        //if even SIMD8 fails, then we compile that. So our defaultShader will be SIMD8
        if (simdMode == SIMDMode::SIMD8 && m_Context->getDefaultSIMDMode() == SIMDMode::BEGIN)
            m_Context->setDefaultSIMDMode(simdMode);

        if(m_Context->m_DriverInfo.sendMultipleSIMDModes()) {
            compileThisSIMD = true; //in this case continue to compile unless below condition is observed
        }
    }
    
    //check if we want to proceed further based on whether any existing shader has spilled
    //we also want to make sure there is another retry that will be attempted
    //Note for this check to work the order must he ascending. Becuase in descending
    //order that is simd32 -> simd16--> simd8 we still have chance that one of hte lower
    //simd modes will compile.
    if (compileThisSIMD && m_Context->m_DriverInfo.sendMultipleSIMDModes()) {
        //Here are some of the assumptions
        //1. if m_Context->m_DriverInfo.sendMultipleSIMDModes() is true that means the order is always ascending.
        //   This will be cleaned up as a separate interface in CodeGen for compute path.
        //2. origSIMDMode will always have a valid value 8, 16 or 32
        //3. For SIMD8 VISA always returns a shader, and if it has spill GetLastSpillSize will be set
        //4. For SIM16 if ForceOCLSIMDWidth is set to 16  then VISA will compile without abort, hence
        //   we need to evaluate LastSpillSize
        if(origSIMDMode  ==  SIMDMode::SIMD8 && m_Context->m_retryManager.GetLastSpillSize())
        {
            //if any one of these are set, we need to try compiling SIMD
           if ((simdMode == SIMDMode::SIMD32 && IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) != 32) ||
               (simdMode == SIMDMode::SIMD16 && IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) != 16))
           {
               compileThisSIMD = false;
           }
        }
        else if(simdMode == SIMDMode::SIMD32 && IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) != 32) {
            //SIMD32 if not forced must see if SIMD16 has been generated without force or without spill
            auto simd16Shader = m_parent->GetShader(SIMDMode::SIMD16);
            bool hasSIMD16 = simd16Shader && simd16Shader->ProgramOutput()->m_programSize > 0;
            if(!hasSIMD16 ||
               (IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) == 16 && m_Context->m_retryManager.GetLastSpillSize()))
           {
                compileThisSIMD = false;
           }
        }
        if(!compileThisSIMD)
            m_Context->setDefaultSIMDMode(origSIMDMode);
    }
    return compileThisSIMD;
}

bool COpenCLKernel::CompileThisSIMD(SIMDMode simdMode, EmitPass &EP, llvm::Function &F)
{
    CShader* simd8Program = m_parent->GetShader(SIMDMode::SIMD8);
    CShader* simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
    CShader* simd32Program = m_parent->GetShader(SIMDMode::SIMD32);
    CodeGenContext *pCtx = GetContext();

    // Here we see if we have compiled a size for this shader already
    if((simd8Program && simd8Program->ProgramOutput()->m_programSize > 0) ||
        (simd16Program && simd16Program->ProgramOutput()->m_programSize > 0) || 
         (simd32Program && simd32Program->ProgramOutput()->m_programSize > 0))
        
    {
        if(!pCtx->m_DriverInfo.sendMultipleSIMDModes())
            return false;
    }

    // Scratch space allocated per-thread needs to be less than 2 MB.
    if (m_ScratchSpaceSize > pCtx->m_DriverInfo.maxPerThreadScratchSpace())
    {
      return false;
    }

    // Next we check if there is a required sub group size specified
    MetaDataUtils* pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData* modMD = pCtx->getModuleMetaData();
    FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
    int simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();
    uint32_t groupSize = 0;
    if (modMD->csInfo.maxWorkGroupSize)
    {
        groupSize = modMD->csInfo.maxWorkGroupSize;
    }
    else
    {
        groupSize = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, &F);
    }
     
    if (groupSize == 0)
    {
        groupSize = IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, &F);
    }

    if (simd_size)
    {
        switch (simd_size)
        {
            case 8:
                if (simdMode == SIMDMode::SIMD16 || simdMode == SIMDMode::SIMD32)
                {
                    return false;
                }
                break;
            case 16:
                if (simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD32)
                {
                    return false;
                }
                EP.m_canAbortOnSpill = false;
                break;
            case 32:
                if (simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16)
                {
                    return false;
                }
                EP.m_canAbortOnSpill = false;
                break;
            default:
                assert(0 && "Unsupported required sub group size");
        }
    }
    else
    {
        // Checking registry/flag here. Note that if ForceOCLSIMDWidth is set to
        // 8/16/32, only corresponding EnableOCLSIMD<N> is set to true. Therefore,
        // if any of EnableOCLSIMD<N> is disabled, ForceOCLSIMDWidth must set to
        // a value other than <N> if set. See igc_regkeys.cpp for detail.
        if ((simdMode == SIMDMode::SIMD32 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD32)) ||
            (simdMode == SIMDMode::SIMD16 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD16)))
        {
            return false;
        }

        // Check if we force code generation for the current SIMD size.
        // Note that for SIMD8, we always force it!
        if (numLanes(simdMode) == IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) ||
            simdMode == SIMDMode::SIMD8)
        {
            return true;
        }

        if (groupSize != 0 && groupSize <= 16)
        {
            if (simdMode == SIMDMode::SIMD32 ||
                (groupSize <= 8 && simdMode != SIMDMode::SIMD8))
            {
                return false;
            }
        }

        // Here we check profitablility, etc.
        if (simdMode == SIMDMode::SIMD16)
        {
            if (pCtx->m_instrTypes.hasDebugInfo)
            {
                bool hasFullDebugInfo = false;
                IF_DEBUG_INFO(bool hasLineNumbersOnly = false;)
                IF_DEBUG_INFO(DebugMetadataInfo::hasAnyDebugInfo(pCtx, hasFullDebugInfo, hasLineNumbersOnly);)

                if (hasFullDebugInfo)
                {
                    return false;
                }
            }

            // bail out of SIMD16 if it's not profitable.
            Simd32ProfitabilityAnalysis &PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
            if (!PA.isSimd16Profitable())
            {
                return false;
            }
        }
        if (simdMode == SIMDMode::SIMD32)
        {
            if (pCtx->m_instrTypes.hasDebugInfo)
            {
                bool hasFullDebugInfo = false;
                IF_DEBUG_INFO(bool hasLineNumbersOnly = false;)
                IF_DEBUG_INFO(DebugMetadataInfo::hasAnyDebugInfo(pCtx, hasFullDebugInfo, hasLineNumbersOnly);)

                if (hasFullDebugInfo)
                {
                    return false;
                }
            }
            // bail out of SIMD32 if it's not profitable.
            Simd32ProfitabilityAnalysis &PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
            if (!PA.isSimd32Profitable())
            {
                return false;
            }
        }
    }

    return true;
}

}
