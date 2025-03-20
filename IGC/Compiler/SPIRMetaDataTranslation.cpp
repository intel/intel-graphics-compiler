/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/SPIRMetaDataTranslation.h"
#include "Compiler/MetaDataApi/SpirMetaDataApi.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/StringSwitch.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-spir-metadata-translation"
#define PASS_DESCRIPTION "SPIR to IGC metadata translator"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SPIRMetaDataTranslation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SPIRMetaDataTranslation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SPIRMetaDataTranslation::ID = 0;

SPIRMetaDataTranslation::SPIRMetaDataTranslation() : ModulePass(ID)
{
    initializeSPIRMetaDataTranslationPass(*PassRegistry::getPassRegistry());
}

bool IGC::isLegalOCLVersion(int major, int minor)
{
    if (major == 1 && minor >= 0 && minor <= 2)
    {
        return true;
    }
    if (major == 2 && minor == 0)
    {
        return true;
    }
    return false;
}

// Khronos SPIRV-LLVM Translator attaches kernel metadata directly to LLVM function, example:
// define spir_kernel void @bar() !reqd_work_group_size !0
// ...
// !0 = !{i32 1, i32 1, i32 1}

// But SPIRMetaDataTranslation pass expects them in the following form:

// !opencl.kernels = !{!0}
// !0 = !{void ()* @bar, !1}
// !1 = !{!"reqd_work_group_size", i32 1, i32 1, i32 1}

// Below function translates the first form into the second one.

// TODO: Reimplement SPIRMetaDataTranslation pass to be able to work directly on Khronos Translator's
// output, without producing opencl.kernels metadata.

void SPIRMetaDataTranslation::translateKernelMetadataIntoOpenCLKernelsMD(Module& M)
{
    llvm::NamedMDNode* opencl_kernels = M.getOrInsertNamedMetadata("opencl.kernels");

    std::set<llvm::Function*> Functions;
    for (uint i = 0; i < opencl_kernels->getNumOperands(); i++)
    {
        if (auto pMdNode = opencl_kernels->getOperand(i))
        {
            if (pMdNode->getOperand(0))
            {
                llvm::Function* opFunc =
                    mdconst::dyn_extract<llvm::Function>(pMdNode->getOperand(0));
                Functions.insert(opFunc);
            }

        }
    }

    for (auto& Func : M)
    {
        if (Functions.find(&Func) == Functions.end() &&
            Func.getCallingConv() == CallingConv::SPIR_KERNEL)
        {
            llvm::SmallVector<StringRef, 8> Names;
            llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 7> Info;
            llvm::SmallVector<Metadata*, 7> Args;
            Func.getContext().getMDKindNames(Names);
            Func.getAllMetadata(Info);
            Args.push_back(ConstantAsMetadata::get(&Func));
            for (const auto &i : Info)
            {
                llvm::SmallVector<Metadata*, 2> Mdvector;
                auto firstElem = MDString::get(M.getContext(), Names[i.first]);
                Mdvector.push_back(firstElem);
                for (uint ops = 0; ops < i.second->getNumOperands(); ops++)
                {
                    Mdvector.push_back(i.second->getOperand(ops));
                }
                Args.push_back(MDTuple::get(M.getContext(), Mdvector));
            }
            opencl_kernels->addOperand(MDTuple::get(M.getContext(), Args));
        }
    }
}

bool SPIRMetaDataTranslation::runOnModule(Module& M)
{
    translateKernelMetadataIntoOpenCLKernelsMD(M);
    MetaDataUtilsWrapper& mduw = getAnalysis<MetaDataUtilsWrapper>();
    MetaDataUtils* pIgcMDUtils = mduw.getMetaDataUtils();
    ModuleMetaData* modMD = mduw.getModuleMetaData();
    SPIRMD::SpirMetaDataUtils spirMDUtils(&M);

    // if no version information is present, check for -std=CLX.X compiler option
    // otherwise, set it to 1.2.
    int oclMajor = 0, oclMinor = 0;
    if (spirMDUtils.isOpenCLVersionsHasValue())
    {
        SPIRMD::VersionMetaDataHandle oclVersion = spirMDUtils.getOpenCLVersionsItem(0);
        oclMajor = oclVersion->getMajor();
        oclMinor = oclVersion->getMinor();
    }
    else
    {
        if (!spirMDUtils.empty_CompilerOptions())
        {
            // check compiler options
            for (auto i = spirMDUtils.getCompilerOptionsItem(0)->begin(), e = spirMDUtils.getCompilerOptionsItem(0)->end(); i != e; ++i)
            {
                if (StringRef(*i).startswith("-cl-std=CL") && i->length() >= 13)
                {
                    oclMajor = i->at(10) - '0';
                    oclMinor = i->at(12) - '0';
                    break;
                }
            }
        }
        // default is 1.2
        if (!isLegalOCLVersion(oclMajor, oclMinor))
        {
            oclMajor = 1;
            oclMinor = 2;
        }
    }

    // Handling Functions
    SPIRMD::SpirMetaDataUtils::KernelsList::const_iterator ki = spirMDUtils.begin_Kernels();
    SPIRMD::SpirMetaDataUtils::KernelsList::const_iterator ke = spirMDUtils.end_Kernels();
    for (; ki != ke; ++ki)
    {
        IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(new IGCMD::FunctionInfoMetaData());
        SPIRMD::KernelMetaDataHandle spirKernel = *ki;
        // TODO: Null metadata entries are only expected in the event of a
        // partial recompilation. Once the retry manager learns to clean
        // up such metadata entries for unneeded kernels, we should simply
        // assert on the function's validity instead.
        if (spirKernel->getFunction() == nullptr)
            continue;
        IGC::FunctionMetaData& funcMD = modMD->FuncMD[spirKernel->getFunction()];
        fHandle->setType(FunctionTypeMD::KernelFunction);

        // Handling Thread Group Size
        SPIRMD::WorkGroupDimensionsMetaDataHandle reqdWorkGroupSize = spirKernel->getRequiredWorkGroupSize();
        if (reqdWorkGroupSize->hasValue())
        {
            IGCMD::ThreadGroupSizeMetaDataHandle tgzHandle = fHandle->getThreadGroupSize();
            tgzHandle->setXDim(reqdWorkGroupSize->getXDim());
            tgzHandle->setYDim(reqdWorkGroupSize->getYDim());
            tgzHandle->setZDim(reqdWorkGroupSize->getZDim());
        }

        // Handling Thread Group Size Hint
        SPIRMD::WorkGroupDimensionsMetaDataHandle workGroupSizeHint = spirKernel->getWorkGroupSizeHint();
        if (workGroupSizeHint->hasValue())
        {
            IGCMD::ThreadGroupSizeMetaDataHandle tgzhHandle = fHandle->getThreadGroupSizeHint();
            tgzhHandle->setXDim(workGroupSizeHint->getXDim());
            tgzhHandle->setYDim(workGroupSizeHint->getYDim());
            tgzhHandle->setZDim(workGroupSizeHint->getZDim());
        }

        // Handling Sub Group Size
        SPIRMD::SubGroupDimensionsMetaDataHandle reqdSubGroupSize = spirKernel->getRequiredSubGroupSize();
        if (reqdSubGroupSize->hasValue())
        {
            IGCMD::SubGroupSizeMetaDataHandle sgHandle = fHandle->getSubGroupSize();
            int simd_size = reqdSubGroupSize->getSIMDSize();
            if (!((simd_size == 8) || (simd_size == 16) || (simd_size == 32)))
            {
                getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError("Unsupported required sub group size", spirKernel->getFunction());
                return false;
            }
            else if (!getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->platform.hasSIMD8Support() && simd_size == 8)
            {
                getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(
                    "Kernel compiled with required subgroup size 8, which is unsupported on this platform", spirKernel->getFunction());
                return false;
            }
            sgHandle->setSIMDSize(simd_size);
        }

        // Handling Sub Group Size
        SPIRMD::WorkgroupWalkOrderMetaDataHandle workgroupWalkOrder = spirKernel->getWorkgroupWalkOrder();
        if (workgroupWalkOrder->hasValue())
        {
            WorkGroupWalkOrderMD& igc_workGroupWalkOrder = funcMD.workGroupWalkOrder;
            int dim0 = workgroupWalkOrder->getDim0();
            int dim1 = workgroupWalkOrder->getDim1();
            int dim2 = workgroupWalkOrder->getDim2();
            igc_workGroupWalkOrder.dim0 = dim0;
            igc_workGroupWalkOrder.dim1 = dim1;
            igc_workGroupWalkOrder.dim2 = dim2;
        }

        // Handling OpenCL Vector Type Hint
        SPIRMD::VectorTypeHintMetaDataHandle vectorTypeHint = spirKernel->getVectorTypeHint();
        if (vectorTypeHint->hasValue())
        {
            IGCMD::VectorTypeHintMetaDataHandle vthHandle = fHandle->getOpenCLVectorTypeHint();
            vthHandle->setVecType(vectorTypeHint->getVecType());
            vthHandle->setSign(vectorTypeHint->getSign());
        }

        // Handling OpenCL Kernel Args Address Spaces
        if (spirKernel->isArgAddressSpacesHasValue())
        {
            SPIRMD::KernelMetaData::ArgAddressSpacesList::const_iterator i = spirKernel->begin_ArgAddressSpaces();
            SPIRMD::KernelMetaData::ArgAddressSpacesList::const_iterator e = spirKernel->end_ArgAddressSpaces();

            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgAddressSpaces.push_back(*i);
            }
        }

        // Handling OpenCL Kernel Args Access Qualifiers
        if (spirKernel->isArgAccessQualifiersHasValue())
        {
            SPIRMD::KernelMetaData::ArgAccessQualifiersList::const_iterator i = spirKernel->begin_ArgAccessQualifiers();
            SPIRMD::KernelMetaData::ArgAccessQualifiersList::const_iterator e = spirKernel->end_ArgAccessQualifiers();
            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgAccessQualifiers.push_back(*i);
            }
        }

        // Handling OpenCL Kernel Args Types
        if (spirKernel->isArgTypesHasValue())
        {
            SPIRMD::KernelMetaData::ArgTypesList::const_iterator i = spirKernel->begin_ArgTypes();
            SPIRMD::KernelMetaData::ArgTypesList::const_iterator e = spirKernel->end_ArgTypes();
            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgTypes.push_back(*i);
            }
        }

        //Handling OpenCL Kernel Args Base Types
        if (spirKernel->isArgBaseTypesHasValue())
        {
            SPIRMD::KernelMetaData::ArgBaseTypesList::const_iterator i = spirKernel->begin_ArgBaseTypes();
            SPIRMD::KernelMetaData::ArgBaseTypesList::const_iterator e = spirKernel->end_ArgBaseTypes();
            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgBaseTypes.push_back(*i);
            }
        }

        //Handling OpenCL Kernel Args Type Qualifiers
        if (spirKernel->isArgTypeQualifiersHasValue())
        {
            SPIRMD::KernelMetaData::ArgTypeQualifiersList::const_iterator i = spirKernel->begin_ArgTypeQualifiers();
            SPIRMD::KernelMetaData::ArgTypeQualifiersList::const_iterator e = spirKernel->end_ArgTypeQualifiers();
            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgTypeQualifiers.push_back(*i);
            }
        }

        //Handling OpenCL Kernel Args Names
        if (spirKernel->isArgNamesHasValue())
        {
            SPIRMD::KernelMetaData::ArgNamesList::const_iterator i = spirKernel->begin_ArgNames();
            SPIRMD::KernelMetaData::ArgNamesList::const_iterator e = spirKernel->end_ArgNames();
            for (; i != e; ++i)
            {
                funcMD.m_OpenCLArgNames.push_back(*i);
            }
        }

        pIgcMDUtils->setFunctionsInfoItem(spirKernel->getFunction(), fHandle);
    }

    // Handling Compiler Options
    // In SPIR, compiler options are represented by a named node with a single item pointing to a list.
    // since the name node is a list, this creates a list of lists where the first item in the outer list
    // is the actual compiler options list.
    if (!spirMDUtils.empty_CompilerOptions())
    {
        SPIRMD::InnerCompilerOptionsMetaDataListHandle compilerOptions = spirMDUtils.getCompilerOptionsItem(0);
        SPIRMD::InnerCompilerOptionsMetaDataList::const_iterator coi = compilerOptions->begin();
        SPIRMD::InnerCompilerOptionsMetaDataList::const_iterator coe = compilerOptions->end();
        for (; coi != coe; ++coi)
        {
            std::string co = *coi;
            // Compiler options that originate from OpenCL/L0 are represented by the same name, without the "-cl"/"-ze" prefixes.
            // L0 supports only "-opt-disable" from the options below
            if (co.find("-cl") == 0 || co.find("-ze") == 0)
            {
                co.erase(0, 3);
            }

            enum OCL_OPTIONS
            {
                DENORM_ARE_ZERO,
                BF_TF_DENORMS_ARE_ZERO,
                CORRECTLY_ROUNDED_SQRT,
                OPT_DISABLE,
                MAD_ENABLE,
                NO_SIGNED_ZERO,
                UNSAFE_MATH,
                FINITE_MATH,
                FAST_RELAXED_MATH,
                DASH_G,
                RELAXED_BUILTINS,
                MATCH_SINCOSPI,
                NONE,
            };
            int igc_compiler_option = llvm::StringSwitch<OCL_OPTIONS>(co)
                .Case("-denorms-are-zero", DENORM_ARE_ZERO)
                .Case("-bf-tf-denorms-are-zero", BF_TF_DENORMS_ARE_ZERO)
                .Case("-fp32-correctly-rounded-divide-sqrt", CORRECTLY_ROUNDED_SQRT)
                .Case("-opt-disable", OPT_DISABLE)
                .Case("-mad-enable", MAD_ENABLE)
                .Case("-no-signed-zeros", NO_SIGNED_ZERO)
                .Case("-unsafe-math-optimizations", UNSAFE_MATH)
                .Case("-finite-math-only", FINITE_MATH)
                .Case("-fast-relaxed-math", FAST_RELAXED_MATH)
                .Case("-g", DASH_G)
                .Case("-relaxed-builtins", RELAXED_BUILTINS)
                .Case("-match-sincospi", MATCH_SINCOSPI)
                .Default(NONE);


            switch (igc_compiler_option)
            {
            case DENORM_ARE_ZERO: modMD->compOpt.DenormsAreZero = true;
                break;
            case BF_TF_DENORMS_ARE_ZERO: modMD->compOpt.BFTFDenormsAreZero = true;
                break;
            case CORRECTLY_ROUNDED_SQRT: modMD->compOpt.CorrectlyRoundedDivSqrt = true;
                break;
            case OPT_DISABLE: modMD->compOpt.OptDisable = true;
                break;
            case MAD_ENABLE:modMD->compOpt.MadEnable = true;
                break;
            case NO_SIGNED_ZERO: modMD->compOpt.NoSignedZeros = true;
                break;
            case UNSAFE_MATH:modMD->compOpt.UnsafeMathOptimizations = true;
                break;
            case FINITE_MATH: modMD->compOpt.FiniteMathOnly = true;
                break;
            case FAST_RELAXED_MATH:
            case RELAXED_BUILTINS:
                modMD->compOpt.FastRelaxedMath = true;
                modMD->compOpt.RelaxedBuiltins = true;
                break;
            case DASH_G: modMD->compOpt.DashGSpecified = true;
                break;
            case MATCH_SINCOSPI: modMD->compOpt.MatchSinCosPi = true;
                break;
            default:
                break;
            }
        }
    }

    // Handling Floating Point Contractions
    if (spirMDUtils.isFloatingPointContractionsHasValue())
    {
        modMD->compOpt.MadEnable = true;
    }

    // Ensure that the extensions listed in the 'igc.spirv.extensions' metadata are
    // reflected within the ModuleMetaData->extensions
    if (!spirMDUtils.empty_SPIRVExtensions())
    {
        auto extensions = spirMDUtils.getSPIRVExtensionsItem(0);
        for (const auto& spirvExtension : *extensions)
        {
            if (spirvExtension == "SPV_INTEL_bindless_images")
            {
                modMD->extensions.spvINTELBindlessImages = true;
            }
        }
    }

    spirMDUtils.deleteMetadata();

    pIgcMDUtils->save(M.getContext());

    return true;
}

