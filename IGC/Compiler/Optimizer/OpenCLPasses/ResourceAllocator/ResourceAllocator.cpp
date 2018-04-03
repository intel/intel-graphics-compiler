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
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataApi/IGCMetaDataDefs.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-resource-allocator"
#define PASS_DESCRIPTION "Allocates UAV and SRV numbers to kernel arguments"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResourceAllocator, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(ExtensionArgAnalysis)
IGC_INITIALIZE_PASS_END(ResourceAllocator, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResourceAllocator::ID = 0;

ResourceAllocator::ResourceAllocator() : ModulePass(ID)
{
    initializeResourceAllocatorPass(*PassRegistry::getPassRegistry());
}

bool ResourceAllocator::runOnModule(Module &M)
{
    // There are two places resources can come from:
    // 1) Images and samplers passed as kernel arguments.
    // 2) Samplers declared inline in kernel scope or program scope.
    
    // This allocates indices only for the arguments.
    // Indices for inline samplers are allocated in the OCL BI Conveter,
    // since finding all inline samplers requires going through the 
    // actual calls.
    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    // FunctionsInfo contains kernels only.
    for( auto i = pMdUtils->begin_FunctionsInfo(), e = pMdUtils->end_FunctionsInfo(); i != e; ++i )
    {
        // Bypass instrumented functions. Instrumented functions should not have Sampler or Image arguments
        // so it is fine to bypass them here. Otherwise the instrumented function will failed in 
        // getOpenCLArgAccessQualifiersItem that try to get non-existed info from FunctionInfoMetaData
        if (i->first->hasFnAttribute("InstrumentedFunc"))
            continue;
        runOnFunction(*(i->first));
    }

    return true;
}

bool ResourceAllocator::runOnFunction(llvm::Function &F)
{
    // This does two things:
    // * Count the number of UAVs/SRVs/Samplers used by the kernels
    // * Allocate a UAV/SRV/Sampler number to each argument, to be compatible with DX.
    // This is then written to the metadata.

    KernelArgs kernelArgs(F, &(F.getParent()->getDataLayout()), getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    ExtensionArgAnalysis& EAA = getAnalysis<ExtensionArgAnalysis>(F);
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    assert(ctx);

    // Go over all of the kernel args.
    // For each kernel arg, if it represents an explicit image or buffer argument, 
    // add appropriate metadata.
    auto defParam = ArgAllocMetaDataHandle(ArgAllocMetaData::get());
    defParam->setType(ResourceTypeEnum::OtherResourceType);
    std::vector<ArgAllocMetaDataHandle> paramAllocations(F.arg_size(), defParam);
    int numUAVs = 0, numResources = 0, numSamplers = 0;

    // Since bindless textures/samplers could potentially be promoted to bindful later, we allocate these
    // first to minimize "holes" in the BT index
    for (auto arg : kernelArgs)
    {
        auto argAlloc = ArgAllocMetaDataHandle(ArgAllocMetaData::get());

        switch (arg.getArgType())
        {
        case KernelArg::ArgType::BINDLESS_IMAGE_1D:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_3D:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY:
        {
            const llvm::Argument *pArg = arg.getArg();
            assert(pArg->getType()->isPointerTy() && "Expected a pointer type for address space decoded samplers");

            if (IGC_IS_FLAG_ENABLED(EnableFallbackToBindless) && ctx->platform.supportBindless())
            {
                argAlloc->setType(ResourceTypeEnum::BindlessUAVResourceType);
                argAlloc->setIndex(numUAVs);
            }
            else
            {
                unsigned int address_space = pArg->getType()->getPointerAddressSpace();

                // This is a buffer. Try to decode this
                bool directIdx = false;
                unsigned int bufId = 0;
                DecodeAS4GFXResource(address_space, directIdx, bufId);
                assert(directIdx == true && "Expected a direct index for address space decoded images");

                argAlloc->setType(ResourceTypeEnum::UAVResourceType);
                argAlloc->setIndex(bufId);
            }
            numUAVs++;
        }
        break;

        case KernelArg::ArgType::BINDLESS_SAMPLER:
        {
            const llvm::Argument *pArg = arg.getArg();
            assert(pArg->getType()->isPointerTy() && "Expected a pointer type for address space decoded samplers");

            if (IGC_IS_FLAG_ENABLED(EnableFallbackToBindless) && ctx->platform.supportBindless())
            {
                argAlloc->setType(ResourceTypeEnum::BindlessSamplerResourceType);
                argAlloc->setIndex(numSamplers);
            }
            else
            {
                unsigned int address_space = pArg->getType()->getPointerAddressSpace();

                // This is a buffer. Try to decode this
                bool directIdx = false;
                unsigned int bufId = 0;
                DecodeAS4GFXResource(address_space, directIdx, bufId);
                assert(directIdx == true && "Expected a direct index for address space decoded sampler");

                argAlloc->setType(ResourceTypeEnum::SamplerResourceType);
                argAlloc->setIndex(bufId);
            }
            numSamplers++;
        }
        break;

        default:
            continue;
        }

        // We want the location to be arg.getArgNo() and not i, because
        // this is eventually accessed by the state processor. The SP
        // aware of the KernelArgs array, it only knows each argument's
        // original arg number. 
        paramAllocations[arg.getAssociatedArgNo()] = argAlloc;
    }

    for( auto arg : kernelArgs )
    {
        auto argAlloc = ArgAllocMetaDataHandle(ArgAllocMetaData::get());

        switch( arg.getArgType() )
        {

        case KernelArg::ArgType::IMAGE_1D:
        case KernelArg::ArgType::IMAGE_1D_BUFFER:
        case KernelArg::ArgType::IMAGE_2D:
        case KernelArg::ArgType::IMAGE_2D_DEPTH:
        case KernelArg::ArgType::IMAGE_2D_MSAA:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
        case KernelArg::ArgType::IMAGE_3D:
        case KernelArg::ArgType::IMAGE_CUBE:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
        case KernelArg::ArgType::IMAGE_1D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
        case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
        case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
            if( arg.getAccessQual() == KernelArg::WRITE_ONLY ||
                arg.getAccessQual() == KernelArg::READ_WRITE )
            {
                argAlloc->setType(ResourceTypeEnum::UAVResourceType);
                argAlloc->setIndex(numUAVs);
                numUAVs++;
            }
            else
            {
                argAlloc->setType(ResourceTypeEnum::SRVResourceType);
                argAlloc->setIndex(numResources);
                numResources++;
            }
            
            assert( !(EAA.isMediaArg( arg.getArg( ) ) && EAA.isVaArg( arg.getArg( ) )) );

            if( EAA.isMediaArg( arg.getArg( ) ) || EAA.isVaArg( arg.getArg( ) ) )
            {
                argAlloc->setExtenstionType( ResourceExtensionTypeEnum::MediaResourceType );
            }
            else if( EAA.isMediaBlockArg( arg.getArg( ) ) )
            {
                argAlloc->setExtenstionType( ResourceExtensionTypeEnum::MediaResourceBlockType );
            }
            else
            {
                argAlloc->setExtenstionType( ResourceExtensionTypeEnum::NonExtensionType );
            }
            break;

        case KernelArg::ArgType::PTR_GLOBAL:
        case KernelArg::ArgType::PTR_CONSTANT:
        case KernelArg::ArgType::PTR_DEVICE_QUEUE:
        case KernelArg::ArgType::IMPLICIT_CONSTANT_BASE:
        case KernelArg::ArgType::IMPLICIT_GLOBAL_BASE:
        case KernelArg::ArgType::IMPLICIT_PRIVATE_BASE:
        case KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER:
            argAlloc->setType(ResourceTypeEnum::UAVResourceType);
            argAlloc->setIndex(numUAVs);
            numUAVs++;
            break;
        
        case KernelArg::ArgType::SAMPLER:
            argAlloc->setType(ResourceTypeEnum::SamplerResourceType);

            assert( !(EAA.isMediaSamplerArg( arg.getArg( ) ) && EAA.isVaArg( arg.getArg( ) )) );

            if( EAA.isMediaSamplerArg( arg.getArg( ) ) )
            {
                argAlloc->setExtenstionType( ResourceExtensionTypeEnum::MediaSamplerType );
            }
            else if( EAA.isVaArg( arg.getArg() ) )
            {
                argAlloc->setExtenstionType( EAA.GetExtensionSamplerType() );
            }
            else
            {
                argAlloc->setExtenstionType( ResourceExtensionTypeEnum::NonExtensionType );
            }
            
            argAlloc->setIndex(numSamplers);
            numSamplers++;
            break;

        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_EVENT_POOL:
        case KernelArg::ArgType::IMPLICIT_DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE:
            argAlloc->setType(ResourceTypeEnum::UAVResourceType);
            argAlloc->setIndex(numUAVs);
            numUAVs++;
            break;

        default:
            continue;
        }

        // We want the location to be arg.getArgNo() and not i, because
        // this is eventually accessed by the state processor. The SP
        // aware of the KernelArgs array, it only knows each argument's
        // original arg number. 
        paramAllocations[arg.getAssociatedArgNo()] = argAlloc;
    }
    
    // Param allocations must be inserted to the Metadata Utils in order.
    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto funcResourceAllocInfo = pMdUtils->getFunctionsInfoItem(&F)->getResourceAlloc();
    for( auto i : paramAllocations )
    {
        funcResourceAllocInfo->addArgAllocsItem(i);
    }

    funcResourceAllocInfo->setUAVsNum(numUAVs);
    funcResourceAllocInfo->setSRVsNum(numResources);
    funcResourceAllocInfo->setSamplersNum(numSamplers);

    pMdUtils->save(F.getContext());
    
    return true;
}
