/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/BuiltInFuncImport.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/IR/Function.h>
#include <llvmWrapper/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvmWrapper/Transforms/Utils/Cloning.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include "common/LLVMWarningsPop.hpp"
#include <unordered_map>
#include "Probe/Assertion.h"
#include <BiFManager/BiFManagerHandler.hpp>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace CLElfLib;

// Register pass to igc-opt
#define PASS_FLAG "igc-builtin-import"
#define PASS_DESCRIPTION "Built-in function pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BIImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(BIImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BIImport::ID = 0;

BIImport::BIImport() :
    ModulePass(ID)
{
    initializeBIImportPass(*PassRegistry::getPassRegistry());
}


/* We have to run this step of updating mangled SPIR function names
because of SPIR 1.2 specification issue. There are bugs in
Khronos Bugzilla : 16039 and 13597

Also contained in the MangleSubst is the changes made for substitutions in the Clang 4.0
See http://llvm.org/viewvc/llvm-project?view=revision&revision=282059
*/

typedef std::pair<llvm::StringRef, llvm::StringRef> PairTy;
typedef const llvm::SmallVector<PairTy, 54> MangleSubstTy;

static const MangleSubstTy MangleSubst =
{
    { "11ocl_image1d",               "14ocl_image1d" },
    { "16ocl_image1darray",          "20ocl_image1d_array" },
    { "17ocl_image1dbuffer",         "21ocl_image1d_buffer" },
    { "11ocl_image2d",               "14ocl_image2d" },
    { "16ocl_image2darray",          "20ocl_image2d_array" },
    { "11ocl_image3d",               "14ocl_image3d" },
    { "15ocl_image2dmsaa",           "19ocl_image2d_msaa" },
    { "20ocl_image2darraymsaa",      "25ocl_image2d_array_msaa" },
    { "20ocl_image2dmsaadepth",      "25ocl_image2d_msaa_depth" },
    { "25ocl_image2darraymsaadepth", "31ocl_image2d_array_msaa_depth" },
    { "16ocl_image2ddepth",          "20ocl_image2d_depth" },
    { "21ocl_image2darraydepth",     "26ocl_image2d_array_depth" }
};

static std::unordered_map<std::string, std::string> MangleStr =
{
    { "24IGIL_EnqueueKernelShared9ocl_queuejPvjS_S_jS_jiPi9ndrange_tii12ocl_clkevent",
        "24IGIL_EnqueueKernelShared9ocl_queuejPvjS0_S0_jS0_jiPi9ndrange_tii12ocl_clkevent" },
    { "18IGIL_EnqueueKernel9ocl_queuejPvjS_S_jS_j9ndrange_ti12ocl_clkevent",
        "18IGIL_EnqueueKernel9ocl_queuejPvjS0_S0_jS0_j9ndrange_ti12ocl_clkevent" },
    { "33IGIL_EnqueueKernelWithLocalParams9ocl_queueiPijPvjS0_S0_jS0_j9ndrange_ti12ocl_clkevent",
        "33IGIL_EnqueueKernelWithLocalParams9ocl_queueiPijPvjS1_S1_jS1_j9ndrange_ti12ocl_clkevent" },
    { "28IGIL_EnqueueKernelWithEvents9ocl_queuejPvjS_S_jS_jiPi9ndrange_tiiPKU3AS412ocl_clkeventPU3AS412ocl_clkevent12ocl_clkevent",
        "28IGIL_EnqueueKernelWithEvents9ocl_queuejPvjS0_S0_jS0_jiPi9ndrange_tiiPU3AS4K12ocl_clkeventPU3AS4S3_S3_" },
    { "14enqueue_marker9ocl_queuejPKU3AS412ocl_clkeventPU3AS412ocl_clkevent",
        "14enqueue_marker9ocl_queuejPU3AS4K12ocl_clkeventPU3AS4S0_" },
    { "11read_imagef14ocl_image2d_ro11ocl_samplerDv2_fS_S_",
        "11read_imagef14ocl_image2d_ro11ocl_samplerDv2_fS1_S1" },
    { "11read_imagei14ocl_image2d_ro11ocl_samplerDv2_fS_S_",
        "11read_imagei14ocl_image2d_ro11ocl_samplerDv2_fS1_S1_" },
    { "12read_imageui14ocl_image2d_ro11ocl_samplerDv2_fS_S_",
        "12read_imageui14ocl_image2d_ro11ocl_samplerDv2_fS1_S1_" },
    { "11read_imagef20ocl_image2d_depth_ro11ocl_samplerDv2_fS_S_",
        "11read_imagef20ocl_image2d_depth_ro11ocl_samplerDv2_fS1_S1_" },
    { "11read_imagef20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS0_",
        "11read_imagef20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS2_" },
    { "11read_imagei20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS0_",
        "11read_imagei20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS2_" },
    { "12read_imageui20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS0_",
        "12read_imageui20ocl_image2d_array_ro11ocl_samplerDv4_fDv2_fS2_" },
    { "11read_imagef26ocl_image2d_array_depth_ro11ocl_samplerDv4_fDv2_fS0_",
        "11read_imagef26ocl_image2d_array_depth_ro11ocl_samplerDv4_fDv2_fS2_" },
    { "11read_imagef14ocl_image3d_ro11ocl_samplerDv4_fS_S_",
        "11read_imagef14ocl_image3d_ro11ocl_samplerDv4_fS1_S1_" },
    { "11read_imagei14ocl_image3d_ro11ocl_samplerDv4_fS_S_",
        "11read_imagei14ocl_image3d_ro11ocl_samplerDv4_fS1_S1_" },
    { "12read_imageui14ocl_image3d_ro11ocl_samplerDv4_fS_S_",
        "12read_imageui14ocl_image3d_ro11ocl_samplerDv4_fS1_S1_" },
    { "12write_imagei14ocl_image3d_woDv4_iS_", "12write_imagei14ocl_image3d_woDv4_iS0_" },
    { "12write_imagei14ocl_image3d_rwDv4_iS_", "12write_imagei14ocl_image3d_rwDv4_iS0_" },
    { "12write_imagei16ocl_image2darrayDv4_iS_", "12write_imagei20ocl_image2d_array_woDv4_iS0_" },
    { "12write_imagei16ocl_image2d_array_woDv4_iS_", "12write_imagei20ocl_image2d_array_woDv4_iS0_" },
    { "12write_imagei16ocl_image2d_array_rwDv4_iS_", "12write_imagei20ocl_image2d_array_rwDv4_iS0_" },
    { "12write_imagei16ocl_image2d_array_woDv4_iiS_", "12write_imagei20ocl_image2d_array_woDv4_iiS0_" },
    { "12write_imagei14ocl_image3d_woDv4_iiS_", "12write_imagei14ocl_image3d_woDv4_iiS0_" },
    { "29intel_work_group_vme_mb_queryPU3AS3jDv2_iS1_14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "29intel_work_group_vme_mb_queryPU3AS3jDv2_iS1_14ocl_image2d_roS2_11ocl_sampler" },
    { "12DoMultiQueryPU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_ro14ocl_image2d_ro11ocl_samplerj",
        "12DoMultiQueryPU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_roS3_11ocl_samplerj" },
    { "37intel_work_group_vme_mb_multi_query_8PU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "37intel_work_group_vme_mb_multi_query_8PU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_roS3_11ocl_sampler" },
    { "37intel_work_group_vme_mb_multi_query_4PU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "37intel_work_group_vme_mb_multi_query_4PU3AS3jjjDv2_jDv2_iS2_14ocl_image2d_roS3_11ocl_sampler" },
    { "41intel_work_group_vme_mb_multi_check_16x16PU3AS3jjjjDv2_ii14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "41intel_work_group_vme_mb_multi_check_16x16PU3AS3jjjjDv2_ii14ocl_image2d_roS2_S2_11ocl_sampler" },
    { "39intel_work_group_vme_mb_multi_check_8x8PU3AS3jjjjDv2_iDv4_i14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "39intel_work_group_vme_mb_multi_check_8x8PU3AS3jjjjDv2_iDv4_i14ocl_image2d_roS3_S3_11ocl_sampler" },
    { "47intel_work_group_vme_mb_multi_bidir_check_16x16PU3AS3jjjjDv2_ihhi14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "47intel_work_group_vme_mb_multi_bidir_check_16x16PU3AS3jjjjDv2_ihhi14ocl_image2d_roS2_S2_S2_11ocl_sampler" },
    { "45intel_work_group_vme_mb_multi_bidir_check_8x8PU3AS3jjjjDv2_ihhS1_14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler",
        "45intel_work_group_vme_mb_multi_bidir_check_8x8PU3AS3jjjjDv2_ihhS1_14ocl_image2d_roS2_S2_S2_11ocl_sampler" },
    { "52intel_sub_group_avc_ime_evaluate_with_dual_reference14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t",
        "52intel_sub_group_avc_ime_evaluate_with_dual_reference14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_ime_payload_t" },
    { "54intel_sub_group_avc_ime_evaluate_with_single_reference14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t",
        "54intel_sub_group_avc_ime_evaluate_with_single_reference14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_ime_payload_t" },
    { "64intel_sub_group_avc_ime_evaluate_with_single_reference_streamout14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t",
        "64intel_sub_group_avc_ime_evaluate_with_single_reference_streamout14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_ime_payload_t" },
    { "62intel_sub_group_avc_ime_evaluate_with_dual_reference_streamout14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t",
        "62intel_sub_group_avc_ime_evaluate_with_dual_reference_streamout14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_ime_payload_t" },
    { "63intel_sub_group_avc_ime_evaluate_with_single_reference_streamin14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t51intel_sub_group_avc_ime_single_reference_streamin_t",
        "63intel_sub_group_avc_ime_evaluate_with_single_reference_streamin14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_ime_payload_t51intel_sub_group_avc_ime_single_reference_streamin_t" },
    { "61intel_sub_group_avc_ime_evaluate_with_dual_reference_streamin14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t49intel_sub_group_avc_ime_dual_reference_streamin_t",
        "61intel_sub_group_avc_ime_evaluate_with_dual_reference_streamin14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_ime_payload_t49intel_sub_group_avc_ime_dual_reference_streamin_t" },
    { "66intel_sub_group_avc_ime_evaluate_with_single_reference_streaminout14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t51intel_sub_group_avc_ime_single_reference_streamin_t",
        "66intel_sub_group_avc_ime_evaluate_with_single_reference_streaminout14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_ime_payload_t51intel_sub_group_avc_ime_single_reference_streamin_t" },
    { "64intel_sub_group_avc_ime_evaluate_with_dual_reference_streaminout14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ime_payload_t49intel_sub_group_avc_ime_dual_reference_streamin_t",
        "64intel_sub_group_avc_ime_evaluate_with_dual_reference_streaminout14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_ime_payload_t49intel_sub_group_avc_ime_dual_reference_streamin_t" },
    { "52intel_sub_group_avc_ref_evaluate_with_dual_reference14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ref_payload_t",
        "52intel_sub_group_avc_ref_evaluate_with_dual_reference14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_ref_payload_t" },
    { "54intel_sub_group_avc_ref_evaluate_with_single_reference14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_ref_payload_t",
        "54intel_sub_group_avc_ref_evaluate_with_single_reference14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_ref_payload_t" },
    { "54intel_sub_group_avc_sic_evaluate_with_single_reference14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_sic_payload_t",
        "54intel_sub_group_avc_sic_evaluate_with_single_reference14ocl_image2d_roS_11ocl_sampler33intel_sub_group_avc_sic_payload_t" },
    { "52intel_sub_group_avc_sic_evaluate_with_dual_reference14ocl_image2d_ro14ocl_image2d_ro14ocl_image2d_ro11ocl_sampler33intel_sub_group_avc_sic_payload_t",
        "52intel_sub_group_avc_sic_evaluate_with_dual_reference14ocl_image2d_roS_S_11ocl_sampler33intel_sub_group_avc_sic_payload_t" }
};

static bool isMangledImageFn(StringRef FName, const MangleSubstTy& MangleSubst)
{
    bool UpdateMangle = std::any_of(MangleSubst.begin(), MangleSubst.end(),
        [=](PairTy Pair) { return FName.find(Pair.first) != StringRef::npos; });

    return (FName.startswith("_Z") && UpdateMangle);
}

static std::string updatedMangleName(const std::string& FuncName, const std::string& Mangle)
{
    std::string Qual = (FuncName.find("write") != std::string::npos) ? "_wo" : "_ro";
    return Mangle + Qual;
}

static std::string updateSPIRmangleName(StringRef FuncName, const MangleSubstTy& MangleSubst)
{
    std::string NewNameStr = FuncName.str();
    for (const auto& Key : MangleSubst)
    {
        auto Mangle = Key.first.str();
        auto NewMangle = Key.second.str();
        size_t index = 0;
        while ((index = NewNameStr.find(Mangle, index)) != std::string::npos)
        {
            auto UpdatedName = updatedMangleName(NewNameStr, NewMangle);
            NewNameStr.replace(index, Mangle.size(), UpdatedName);
            index += UpdatedName.size();
        }
    }

    return NewNameStr;
}

static std::string updateSPIRmangleName38_to_40(StringRef FuncName, char letter)
{
    std::string new38FuncName;
    for (uint i = 0; i < FuncName.size(); i++)
    {
        if (FuncName[i] == letter)
        {
            if (FuncName.substr(i + 3, 2) == "AS")
            {
                new38FuncName = FuncName.slice(0, i).str() +
                    FuncName.slice(i + 1, i + 6).str() +
                    letter;
                i += 5;
                continue;
            }
        }
        new38FuncName += FuncName[i];
    }
    return new38FuncName;
}

void IGC::BIImport::supportOldManglingSchemes(Module& M)
{
    for (auto& F : M)
    {
        if (F.isDeclaration())
        {
            auto FuncName = F.getName();
            std::string NewFuncName = "";

            std::string ReplaceStr = FuncName.slice(2, FuncName.size()).str();
            if (MangleStr.find(ReplaceStr) != MangleStr.end())
            {
                NewFuncName = "_Z" + MangleStr[ReplaceStr];
            }
            else if (isMangledImageFn(FuncName, MangleSubst))
            {
                NewFuncName = updateSPIRmangleName(FuncName, MangleSubst);
            }
            else
            {
                NewFuncName = FuncName.str();
            }
            // Current workaround to support binaries compiled with < 3.8 clang
            // This is for dealing with constant (K) and volatile (V) types
            if (NewFuncName.find("V") != std::string::npos)
            {
                NewFuncName = updateSPIRmangleName38_to_40(NewFuncName, 'V');
            }
            else if (NewFuncName.find("K") != std::string::npos)
            {
                NewFuncName = updateSPIRmangleName38_to_40(NewFuncName, 'K');
            }
            F.setName(NewFuncName);
        }
    }
}

Function* BIImport::GetBuiltinFunction(llvm::StringRef funcName, llvm::Module* GenericModule)
{
    Function* pFunc = nullptr;
    if ((pFunc = GenericModule->getFunction(funcName)) && !pFunc->isDeclaration())
        return pFunc;
    return nullptr;
}


static bool materialized_use_empty(const Value* v)
{
    return v->materialized_use_begin() == v->use_end();
}

void BIImport::WriteElfHeaderToMap(DenseMap<StringRef, int>& Map, char* pData, size_t dataSize)
{
    //Data from pData is layed out as follows.....
    //First two bytes are the string size
    //Next byte is the start of the function name
    //Last two bytes are the index of function in the elf file

    auto pData2 = (unsigned char*)pData;
    for (uint i = 0; i < dataSize;)
    {
        unsigned short str_size = (unsigned short)pData2[i] | ((unsigned short)pData2[i + 1] << 8);
        StringRef key(&pData[i + 2], str_size);
        unsigned short func_index = (unsigned short)pData2[i + 2 + str_size] | ((unsigned short)pData2[i + 3 + str_size] << 8);
        Map[key] = func_index;
        i += (4 + str_size);
    }

}

std::unique_ptr<llvm::Module> BIImport::Construct(Module& M, CLElfLib::CElfReader* pElfReader, bool hasSizet)
{
    char* pData = NULL;
    size_t dataSize = 0;
    std::string num_line = "";
    DenseMap<StringRef, int> Map(32768);
    pElfReader->GetSectionData(1, pData, dataSize);
    WriteElfHeaderToMap(Map, pData, dataSize);
    if (hasSizet)
    {
        char* pData_sizet = NULL;
        size_t dataSize_sizet = 0;
        if (M.getDataLayout().getPointerSizeInBits() == 32)
        {
            pElfReader->GetSectionData(2, pData_sizet, dataSize_sizet);
        }
        else
        {
            pElfReader->GetSectionData(3, pData_sizet, dataSize_sizet);
        }
        WriteElfHeaderToMap(Map, pData_sizet, dataSize_sizet);
    }

    unsigned numOfHeaders = pElfReader->GetElfHeader()->NumSectionHeaderEntries;
    std::vector<std::unique_ptr<llvm::Module>> elf_index(numOfHeaders);

    std::function<void(Function*)> Explore = [&](Function* pRoot)
    {
        TFunctionsVec calledFuncs;
        GetCalledFunctions(pRoot, calledFuncs);

        for (auto* pCallee : calledFuncs)
        {
            Function* pFunc = nullptr;
            if (pCallee->isDeclaration())
            {
                auto funcName = pCallee->getName();
                if (funcName.str() == "__enqueue_kernel_basic" ||
                    funcName.str() == "__enqueue_kernel_vaargs" ||
                    funcName.str() == "__enqueue_kernel_events_vaargs" ||
                    funcName.str() == "_Z14enqueue_kernel")
                {
                    funcName = StringRef("enqueue_IB_kernel");
                }
                int SectionIndex = Map[funcName];
                if (SectionIndex == 0 || pCallee->isIntrinsic()) continue;
                if (elf_index[SectionIndex] == NULL)
                {
                    char* pData_Func = NULL;
                    size_t SectionSize = 0;
                    pElfReader->GetSectionData(SectionIndex, pData_Func, SectionSize);
                    std::unique_ptr<MemoryBuffer> OutputBuffer =
                        MemoryBuffer::getMemBufferCopy(
                            StringRef(pData_Func, SectionSize));
                    llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                        getOwningLazyBitcodeModule(std::move(OutputBuffer), M.getContext());
                    if (llvm::Error EC = ModuleOrErr.takeError())
                    {
                        IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
                    }
                    elf_index[SectionIndex] = (std::move(*ModuleOrErr));
                }
                auto Generic = elf_index[SectionIndex].get();
                Function* pSrcFunc = GetBuiltinFunction(funcName, Generic);
                pFunc = pSrcFunc;
                if (!pFunc) continue;
            }
            else
            {
                pFunc = pCallee;
            }

            if (pFunc->isMaterializable())
            {
                if (Error Err = pFunc->materialize()) {
                    std::string Msg;
                    handleAllErrors(std::move(Err), [&](ErrorInfoBase& EIB) {
                        errs() << "===> Materialize Failure: " << EIB.message().c_str() << '\n';
                    });
                    IGC_ASSERT_MESSAGE(0, "Failed to materialize Global Variables");
                }
                else {
                    pFunc->addFnAttr("OclBuiltin");
                    Explore(pFunc);
                }
            }
        }
    };

    supportOldManglingSchemes(M);

    for (auto& func : M)
    {
        Explore(&func);
    }

    // nuke the unused functions so we can materializeAll() quickly
    auto CleanUnused = [](Module* Module)
    {
        for (auto I = Module->begin(), E = Module->end(); I != E; )
        {
            auto* F = &(*I++);
            if (F->isDeclaration() || F->isMaterializable())
            {
                if (materialized_use_empty(F))
                {
                    F->eraseFromParent();
                }
            }
        }
    };

    std::unique_ptr<llvm::Module> BIM(new Module("BIF", M.getContext()));
    Linker ld(*BIM);

    for (auto& setIterator : elf_index)
    {
        if (setIterator == NULL)
        {
            continue;
        }
        CleanUnused(setIterator.get());

        if (Error err = setIterator->materializeAll())
        {
            IGC_ASSERT_MESSAGE(0, "materializeAll failed for size_t builtin module");
        }

        if (ld.linkInModule(std::move(setIterator), Linker::OverrideFromSrc))
        {
            IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
        }
    }

    //1)Go through Globals in cleaned BIM
    //2)Check if Global has a user
    //3)link all of the Globals with a user to BIM

    auto& Globals = BIM->getGlobalList();
    for (auto& global_iterator : Globals)
    {
        if (!global_iterator.hasInitializer() && !global_iterator.use_empty())
        {
            int SectionIndex = Map[global_iterator.getName()];
            if (SectionIndex == 0) continue;
            char* pData_Func = NULL;
            size_t SectionSize = 0;
            pElfReader->GetSectionData(SectionIndex, pData_Func, SectionSize);
            std::unique_ptr<MemoryBuffer> OutputBuffer =
                MemoryBuffer::getMemBufferCopy(
                    StringRef(pData_Func, SectionSize));
            llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                getOwningLazyBitcodeModule(std::move(OutputBuffer), M.getContext());
            if (llvm::Error EC = ModuleOrErr.takeError())
            {
                IGC_ASSERT_MESSAGE(0, "Error when LazyLoading global module");
            }
            if (ld.linkInModule(std::move(*ModuleOrErr)))
            {
                IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
            }
        }
    }

    return BIM;
}


// OpenCL C builtins that do not have a corresponding SPIRV specification are represented
// as a regular user functions (OpFunction). Since IGC SPIRV-LLVM Translator promotes i1
// type to i8 type for all user-defined functions, such builtins are also promoted. It leads
// to functions signatures mismatch when trying to link builtins definitions.
// Below function changes i8 types back to i1 to allow for proper linking.
void BIImport::fixSPIRFunctionsReturnType(Module& M)
{
    SmallPtrSet<Function*, 8> funcsToRemove;

    for (auto& F : M)
    {
        if (F.isDeclaration())
        {
            auto FuncName = F.getName();

            if (FuncName.equals("intel_is_traversal_done") ||
                FuncName.equals("intel_get_hit_front_face") ||
                FuncName.equals("intel_has_committed_hit"))
            {
                if (!F.getReturnType()->isIntegerTy(8))
                    continue;

                FunctionType* FT = F.getFunctionType();

                FunctionType* NewFT = FunctionType::get(Type::getInt1Ty(M.getContext()), FT->params(), false);
                auto* NewF = Function::Create(NewFT, F.getLinkage(), FuncName + ".cloned", M);

                SmallPtrSet<CallInst*, 16> Calls;

                for (auto user : F.users())
                    if (CallInst* CI = dyn_cast<CallInst>(user))
                        Calls.insert(CI);

                for (auto CI : Calls)
                {
                    IRBuilder<> builder(CI);

                    SmallVector<Value*, 4> Args;
                    for (auto& Arg : CI->args())
                        Args.push_back(Arg);

                    auto* newCall = builder.CreateCall(NewF, Args);
                    newCall->setCallingConv(CI->getCallingConv());
                    newCall->setAttributes(CI->getAttributes());
                    auto* zext = builder.CreateZExt(newCall, builder.getInt8Ty());

                    CI->replaceAllUsesWith(zext);
                    CI->eraseFromParent();
                }

                std::string originalName = FuncName.str();
                F.setName(FuncName + "_old");
                NewF->setName(originalName);

                funcsToRemove.insert(&F);
            }
        }
    }

    for (auto* F : funcsToRemove)
        F->eraseFromParent();
}

// Older Clang versions generate invalid bitcast instructions for explicit
// C-style casts with specified address space. For example:
//   %0 = bitcast i8 addrspace(1)* %mem to i32 addrspace(4)*
// Bitcasts with address space change are illegal. In the case above, the
// bitcast must be replaced with bitcast + addrspacecast.
void BIImport::fixInvalidBitcasts(llvm::Module &M)
{
    for (auto &F : M)
    {
        if (!F.getName().startswith("__builtin"))
            continue;

        // Collect invalid bitcasts with address space change.
        std::vector<BitCastInst *> Worklist;
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
        {
            BitCastInst *BitCastI = dyn_cast<BitCastInst>(&*I);
            if (!BitCastI || !BitCastI->getType()->isPointerTy() ||
                BitCastI->getSrcTy()->getPointerAddressSpace() ==
                    BitCastI->getType()->getPointerAddressSpace())
                continue;

            Worklist.push_back(BitCastI);
        }

        // Replace the invalid bitcast with bitcast + addrspacecast.
        for (BitCastInst *BitCastI : Worklist)
        {
            PointerType *SrcPtrType =
                dyn_cast<PointerType>(BitCastI->getSrcTy());
            PointerType *DstPtrType =
                dyn_cast<PointerType>(BitCastI->getType());

            // New bitcast
            PointerType *FinalBitCastType =
                IGCLLVM::getWithSamePointeeType(DstPtrType, SrcPtrType->getAddressSpace());
            BitCastInst *NewBitCastI = new BitCastInst(
                BitCastI->getOperand(0), FinalBitCastType, "bcast", BitCastI);

            // New addrspacecast
            AddrSpaceCastInst *NewAddrSpaceCastI = new AddrSpaceCastInst(
                NewBitCastI, DstPtrType, "ascast", BitCastI);

            // Replace all uses of the old invalid bitcast.
            BitCastI->replaceAllUsesWith(NewAddrSpaceCastI);
            BitCastI->eraseFromParent();
        }
    }
}

bool BIImport::runOnModule(Module& M)
{
    fixSPIRFunctionsReturnType(M);

    for (auto& F : M)
    {
        if (F.isDeclaration())
        {
            auto FuncName = F.getName();
            std::string NewFuncName = "";

            std::string ReplaceStr = FuncName.slice(2, FuncName.size()).str();
            if (MangleStr.find(ReplaceStr) != MangleStr.end())
            {
                NewFuncName = "_Z" + MangleStr[ReplaceStr];
            }
            else if (isMangledImageFn(FuncName, MangleSubst))
            {
                NewFuncName = updateSPIRmangleName(FuncName, MangleSubst);
            }
            else
            {
                NewFuncName = FuncName.str();
            }
            // Current workaround to support binaries compiled with < 3.8 clang
            // This is for dealing with constant (K) and volatile (V) types
            if (NewFuncName.find("V") != std::string::npos)
            {
                NewFuncName = updateSPIRmangleName38_to_40(NewFuncName, 'V');
            }
            else if (NewFuncName.find("K") != std::string::npos)
            {
                NewFuncName = updateSPIRmangleName38_to_40(NewFuncName, 'K');
            }
            F.setName(NewFuncName);
        }
    }

#ifdef BIF_LINK_BC
    // Link all needed Bif instr
    {
        auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        BiFManager::FuncTimer startTime =
            [&](COMPILE_TIME_INTERVALS interval) {
            COMPILER_TIME_START(pCtx, interval);
            };
        BiFManager::FuncTimer endTime =
            [&](COMPILE_TIME_INTERVALS interval) {
            COMPILER_TIME_END(pCtx, interval);
            };

        BiFManager::BiFManagerHandler bifLinker(M.getContext());

        bifLinker.SetFuncTimers(&startTime, &endTime);

        bifLinker.LinkBiF(M);
    }
#endif // BIF_LINK_BC

    InitializeBIFlags(M);
    removeFunctionBitcasts(M);
    fixInvalidBitcasts(M);

    std::vector<Instruction*> InstToRemove;
    //temporary work around for sampler types and pipes
    for (auto& func : M)
    {
        auto funcName = func.getName();
        if (funcName.startswith("__builtin_IB_convert_sampler_to_int") ||
            funcName.startswith("__builtin_IB_convert_pipe_ro_to_intel_pipe") ||
            funcName.startswith("__builtin_IB_convert_pipe_wo_to_intel_pipe"))
        {
            for (auto Users : func.users())
            {
                if (auto CI = dyn_cast<CallInst>(Users))
                {
                    IRBuilder<> builder(CI);
                    Value* newV;
                    if (funcName.startswith("__builtin_IB_convert_sampler_to_int"))
                    {
                        Type* CIOpTy = CI->getOperand(0)->getType();
                        if (isa<PointerType>(CIOpTy))
                        {
                            newV = builder.CreatePtrToInt(CI->getOperand(0), CI->getType());
                        }
                        else
                        {
                            CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

                            bool UseBindlessImage = ctx->getModuleMetaData()->UseBindlessImage;
                            ctx->getModuleMetaData()->UseBindlessImageWithSamplerTracking = UseBindlessImage;
                            // i32 to i64
                            newV = builder.CreateZExt(CI->getOperand(0), CI->getType());
                        }
                    }
                    else
                        newV = builder.CreateBitOrPointerCast(CI->getOperand(0), CI->getType());
                    CI->replaceAllUsesWith(newV);
                    InstToRemove.push_back(CI);
                }
            }
        }
    }

    // Builtins and intrinsics to support function pointers
    for (auto& F : M)
    {
        auto funcName = F.getName();
        // Builtin for OCL support for function pointers
        // Gets the function address
        if (funcName.startswith("__builtin_IB_get_function_pointer"))
        {
            for (auto user : F.users())
            {
                if (CallInst* CI = dyn_cast<CallInst>(&*user))
                {
                    // Strip if CI->getArgOperand(0) is ConstExpr bitcast which happens when
                    // Translating with SPIRV-LLVM-Translator >= 16
                    // Before 16 CI->getArgOperand(0) would return GEP instruction
                    Value* arg0 = CI->getArgOperand(0)->stripPointerCasts();
                    GlobalVariable* global = nullptr;

                    if (auto* gepOp = dyn_cast<GEPOperator>(arg0))
                        global = cast<GlobalVariable>(gepOp->getPointerOperand());
                    else
                        global = cast<GlobalVariable>(arg0);

                    auto* gArray = cast<ConstantDataArray>(global->getInitializer());
                    auto* pFunc = GetBuiltinFunction(gArray->getAsCString(), &M);
                    IGC_ASSERT(nullptr != pFunc);

                    pFunc->addFnAttr("IFCALL_BUILTIN");
                    pFunc->setCallingConv(llvm::CallingConv::SPIR_FUNC);
                    // Replace builtin with the actual function pointer
                    IRBuilder<> builder(CI);
                    Value* funcPtr = builder.CreatePointerCast(pFunc, CI->getType());
                    CI->replaceAllUsesWith(funcPtr);
                    InstToRemove.push_back(CI);
                }
            }
        }
        // Builtin for OCL support for function pointers
        // Calls a function address
        else if (funcName.startswith("__builtin_IB_call_function_pointer"))
        {
            for (auto user : F.users())
            {
                if (CallInst* CI = dyn_cast<CallInst>(&*user))
                {
                    IGCLLVM::IRBuilder<> builder(CI);
                    Value* argBuffer = CI->getArgOperand(1);
                    // Function type is always void (i8*)
                    FunctionType* funcTy = FunctionType::get(builder.getVoidTy(), { argBuffer->getType() }, false);
                    Value* funcPtr = builder.CreatePointerCast(CI->getArgOperand(0), PointerType::get(funcTy, 0));
                    // Replace builtin with the function call
                    CallInst* callFunc = builder.CreateCall(funcTy, funcPtr, argBuffer);
                    callFunc->setCallingConv(llvm::CallingConv::SPIR_FUNC);
                    CI->replaceAllUsesWith(callFunc);
                    InstToRemove.push_back(CI);
                }
            }
        }

        // Handles the function pointer SIMD variant functions
        else if (funcName.startswith("__intel_create_simd_variant"))
        {
            // If we encounter this call, we need to enable this flag to indicate we need to compile multiple SIMD
            auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
            pCtx->m_enableSimdVariantCompilation = true;

            for (auto user : F.users())
            {
                if (CallInst* CI = dyn_cast<CallInst>(&*user))
                {
                    Function* calledF = dyn_cast<Function>(CI->getArgOperand(0));
                    IGC_ASSERT(calledF && CI->hasFnAttr("vector-variant"));
                    StringRef VariantName = CI->getFnAttr("vector-variant").getValueAsString();

                    // Parse the variant string, and create a function declaration that represents a variant of the called function.
                    auto [symStr, fName, vecLen] = IGC::ParseVectorVariantFunctionString(VariantName);
                    std::string fDeclName = symStr + fName;
                    Function* pNewFuncDecl = M.getFunction(fDeclName);
                    if (!pNewFuncDecl)
                    {
                        SmallVector<Type*, 8> argTys;
                        for (auto& arg : calledF->args()) argTys.push_back(arg.getType());
                        FunctionType* signature = FunctionType::get(calledF->getReturnType(), argTys, false);
                        pNewFuncDecl = Function::Create(signature, calledF->getLinkage(), fDeclName, &M);
                        pNewFuncDecl->copyAttributesFrom(calledF);
                        pNewFuncDecl->setCallingConv(llvm::CallingConv::SPIR_FUNC);
                    }
                    pNewFuncDecl->addFnAttr("variant-function-decl");
                    calledF->addFnAttr("variant-function-def");
                    calledF->addFnAttr("CompileSIMD" + std::to_string(vecLen));
                    IGCLLVM::IRBuilder<> builder(CI);
                    Value* bitcast = builder.CreateBitCast(pNewFuncDecl, CI->getType());
                    CI->replaceAllUsesWith(bitcast);
                    InstToRemove.push_back(CI);
                }
            }
        }
        // Handles the variant table calls
        // FIXME: This is a temp solution, eventually if we support argument variants etc, it's unlikely
        // we will have all the information for the variant index in this pass, and will have to move it
        // to later passes. For now, since we only require subgroup size, this should suffice.
        else if (funcName.startswith("__intel_indirect_call"))
        {
            MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            for (auto user : F.users())
            {
                if (CallInst* CI = dyn_cast<CallInst>(&*user))
                {
                    unsigned tableIndex = 0;
                    Value* FPTablePtr = CI->getArgOperand(0);
                    IGC_ASSERT(FPTablePtr->getType()->isPointerTy());

                    if (CI->hasFnAttr("vector-variants"))
                    {
                        // Get the list of metadata strings indicating the function variant per index
                        StringRef VariantsStr = CI->getAttributes().getAttributeAtIndex(AttributeList::FunctionIndex, "vector-variants")
                            .getValueAsString();
                        SmallVector<StringRef, 8> VariantsTable;
                        VariantsStr.split(VariantsTable, ',');

                        // Get the caller function
                        Function* callerF = CI->getParent()->getParent();
                        // Assume the caller has explicit subgroup size set, otherwise we cannot determine which variant to use
                        FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(callerF);
                        unsigned subgroup_size = (unsigned)funcInfoMD->getSubGroupSize()->getSIMDSize();
                        IGC_ASSERT(subgroup_size != 0);

                        // Parse each variant string in the table, stop at the first one that matches subgroup_size
                        for (const auto& var : VariantsTable)
                        {
                            // We only need to get the SIMD size from the string
                            auto [symStr, fName, vecLen] = IGC::ParseVectorVariantFunctionString(var);
                            if (vecLen == subgroup_size)
                                break;
                            tableIndex++;
                        }
                        IGC_ASSERT_MESSAGE(tableIndex < VariantsTable.size(), "Subgroup size not found in function variants!");
                    }

                    IGCLLVM::IRBuilder<> builder(CI);
                    IGC_ASSERT(CI->paramHasAttr(0, llvm::Attribute::ByVal));
                    // Load function address from the table index
                    Value* FP = builder.CreateGEP(CI->getParamByValType(0), FPTablePtr, builder.getInt32(tableIndex));
                    FP = builder.CreateLoad(cast<llvm::GetElementPtrInst>(FP)->getResultElementType(), FP);
                    IGC_ASSERT(FP->getType()->isPointerTy());
                    // Call the loaded function address
                    SmallVector<Value*, 8> Args;
                    for (unsigned i = 1; i < IGCLLVM::getNumArgOperands(CI); i++)
                        Args.push_back(CI->getArgOperand(i));
                    CallInst* CallFP = builder.CreateCall(FP, Args);
                    CallFP->setCallingConv(llvm::CallingConv::SPIR_FUNC);

                    CI->replaceAllUsesWith(CallFP);
                    InstToRemove.push_back(CI);
                }
            }
        }
    }

    for (auto I : InstToRemove)
    {
        I->eraseFromParent();
    }

    return true;
}

void BIImport::GetCalledFunctions(const Function* pFunc, TFunctionsVec& calledFuncs)
{
    SmallPtrSet<Function*, 8> visitedSet;
    // Iterate over function instructions and look for call instructions
    for (const_inst_iterator it = inst_begin(pFunc), e = inst_end(pFunc); it != e; ++it)
    {
        const CallInst* pInstCall = dyn_cast<CallInst>(&*it);
        if (!pInstCall) continue;
        Function* pCalledFunc = pInstCall->getCalledFunction();
        if (!pCalledFunc)
        {
            // This case can occur only if CallInst is calling something other than LLVM function.
            // Thus, no need to handle this case - function casting is not allowed (and not expected!)
            continue;
        }
        if (visitedSet.count(pCalledFunc)) continue;

        visitedSet.insert(pCalledFunc);
        calledFuncs.push_back(pCalledFunc);
    }
}

// Returns true when any pointer operand/return type in the call does not match
// the address space of the same position in the callee prototype.
static bool needsPointerASFix(const CallInst *inst, const Function *callee)
{
    const FunctionType *callFTy  = inst->getFunctionType();
    const FunctionType *fProtoTy = callee->getFunctionType();

    if (callFTy->getReturnType()->isPointerTy() &&
        fProtoTy->getReturnType()->isPointerTy() &&
        callFTy->getReturnType()->getPointerAddressSpace() !=
        fProtoTy->getReturnType()->getPointerAddressSpace())
        return true;

    if (callFTy->getNumParams() != fProtoTy->getNumParams())
        return false;

    for (unsigned i = 0, e = callFTy->getNumParams(); i != e; ++i)
    {
        Type *callTy  = callFTy->getParamType(i);
        Type *protoTy = fProtoTy->getParamType(i);

        if (callTy->isPointerTy() && protoTy->isPointerTy() &&
            callTy->getPointerAddressSpace() !=
            protoTy->getPointerAddressSpace())
            return true;
    }
    return false;
}

void BIImport::removeFunctionBitcasts(Module& M)
{
    std::vector<Instruction*> list_delete;
    DenseMap<Function*, std::vector<Function*>> bitcastFunctionMap;

    for (Function& func : M)
    {
        for (auto& BB : func)
        {
            for (auto I = BB.begin(), E = BB.end(); I != E; I++)
            {
                CallInst* pInstCall = dyn_cast<CallInst>(I);
                if (!pInstCall || pInstCall->getCalledFunction())
                    continue;

                Function *funcToBeChanged = nullptr;

                // The call instruction might either use bitcast const expression or the function directly.
                Value *calledVal = IGCLLVM::getCalledValue(pInstCall);
                ConstantExpr *constExpr = dyn_cast<ConstantExpr>(calledVal);
                if (constExpr)
                {
                    funcToBeChanged = dyn_cast<Function>(constExpr->stripPointerCasts());
                }
                else if (Function *directFunc = dyn_cast<Function>(calledVal))
                {
                    if (needsPointerASFix(pInstCall, directFunc))
                        funcToBeChanged = directFunc;
                }

                if (!funcToBeChanged || funcToBeChanged->isDeclaration())
                    continue;

                // Map between values (functions) in source of bitcast
                // to their counterpart values in destination
                llvm::ValueToValueMapTy  operandMap;
                Function* pDstFunc = nullptr;
                auto BCFMI = bitcastFunctionMap.find(funcToBeChanged);
                bool notExists = BCFMI == bitcastFunctionMap.end();
                if (!notExists)
                {
                    auto funcVec = bitcastFunctionMap[funcToBeChanged];
                    notExists = true;
                    for (Function* F : funcVec) {
                        if (pInstCall->getFunctionType() == F->getFunctionType())
                        {
                            notExists = false;
                            pDstFunc = F;
                            break;
                        }
                    }
                }

                if (notExists)
                {
                    pDstFunc = Function::Create(pInstCall->getFunctionType(), funcToBeChanged->getLinkage(), funcToBeChanged->getName(), &M);
                    if (pDstFunc->arg_size() != funcToBeChanged->arg_size()) continue;
                    // Go through and convert function arguments over, remembering the mapping.
                    Function::arg_iterator itSrcFunc = funcToBeChanged->arg_begin();
                    Function::arg_iterator eSrcFunc = funcToBeChanged->arg_end();
                    llvm::Function::arg_iterator itDest = pDstFunc->arg_begin();

                    // Fix incorrect address space or incorrect pointer type caused by CloneFunctionInto later
                    // 1. AddressSpaceCast example: CloneFunctionInto causes incorrect LLVM IR, like below
                    //     %arrayidx.le.i = getelementptr inbounds i8, i8 addrspace(1)* %8, i64 %conv.le.i
                    //     %9 = load i8, i8 addrspace(4)* %arrayidx.le.i, align 1, !tbaa !309
                    // Address space should match for %arrayidx.le.i, so we insert necessary
                    // address space casts, which should be eliminated later by other passes
                    // 2. incorrect type example:
                    //     %0 = load i16, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(4)* %x, align 2
                    // Load value type should match pointer type for %x, so we insert necessary bitcast:
                    //     %x.bcast = bitcast %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(4)* %x to i16 addrspace(4)*
                    //     %0 = load i16, i16 addrspace(4)* %x.bcast, align 2
                    SmallVector<Instruction *, 5> castInsts;

                    for (; itSrcFunc != eSrcFunc; ++itSrcFunc, ++itDest)
                    {
                        itDest->setName(itSrcFunc->getName());

                        Type *srcType = (*itSrcFunc).getType();
                        Value *destVal = &(*itDest);
                        Type *destType = destVal->getType();
                        if (srcType->isPointerTy() && destType->isPointerTy())
                        {
                            if (srcType->getPointerAddressSpace() != destType->getPointerAddressSpace())
                            {
                                AddrSpaceCastInst *newASC = new AddrSpaceCastInst(destVal, srcType, destVal->getName() + ".ascast");
                                castInsts.push_back(newASC);
                                destVal = newASC;
                            }
                            PointerType* pSrcType = cast<PointerType>(srcType);
                            if (!pSrcType->isOpaqueOrPointeeTypeMatches(destType))
                            {
                                BitCastInst *newBT = new BitCastInst(destVal, srcType, destVal->getName() + ".bcast");
                                castInsts.push_back(newBT);
                                destVal = newBT;
                            }
                        }

                        operandMap[&(*itSrcFunc)] = destVal;
                    }

                    // Clone the body of the function into the dest function.
                    SmallVector<ReturnInst*, 8> Returns; // Ignore returns.
                    IGCLLVM::CloneFunctionInto(
                        pDstFunc,
                        funcToBeChanged,
                        operandMap,
                        IGCLLVM::CloneFunctionChangeType::LocalChangesOnly,
                        Returns,
                        "");

                    // Need to copy the attributes over too.
                    AttributeList FuncAttrs = funcToBeChanged->getAttributes();
                    pDstFunc->setAttributes(FuncAttrs);

                    // get first instruction in function and insert addressspacecast before it
                    Instruction *firstInst = &(*pDstFunc->begin()->getFirstInsertionPt());
                    for (Instruction *valToInsert : castInsts)
                        valToInsert->insertBefore(firstInst);

                    pDstFunc->setCallingConv(funcToBeChanged->getCallingConv());
                    bitcastFunctionMap[funcToBeChanged].push_back(pDstFunc);
                }

                std::vector<Value*> Args;
                for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(pInstCall); I != E; ++I) {
                    Args.push_back(pInstCall->getArgOperand(I));
                }
                auto newCI = CallInst::Create(pDstFunc, Args, "", pInstCall);
                newCI->takeName(pInstCall);
                newCI->setCallingConv(pInstCall->getCallingConv());
                newCI->setAttributes(pInstCall->getAttributes());
                newCI->setDebugLoc(pInstCall->getDebugLoc());
                pInstCall->replaceAllUsesWith(newCI);
                pInstCall->dropAllReferences();
                if (constExpr && constExpr->use_empty())
                    constExpr->dropAllReferences();
                if (funcToBeChanged->use_empty())
                    funcToBeChanged->eraseFromParent();

                list_delete.push_back(pInstCall);
            }
        }
    }

    for (auto i : list_delete)
    {
        i->eraseFromParent();
    }
}

void BIImport::InitializeBIFlags(Module& M)
{
    /// @brief Set global variable linkage to extrnal. This is needed for
    ///        variables that will become relocations.
    auto makeVarExternal = [&M](StringRef varName)
    {
        GlobalVariable *gv = M.getGlobalVariable(varName);
        if (gv == nullptr)
            return;
        gv->setLinkage(GlobalValue::ExternalLinkage);
    };

    makeVarExternal("__SubDeviceID");
}

extern "C" llvm::ModulePass* createBuiltInImportPass()
{
    return new BIImport();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////


// Register pass to igc-opt
#define PASS_FLAG2 "igc-Pre-BIImport-Analysis"
#define PASS_DESCRIPTION2 "searching OCL GetGlobalOffset function"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(PreBIImportAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(PreBIImportAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char PreBIImportAnalysis::ID = 0;

const llvm::StringRef PreBIImportAnalysis::OCL_GET_GLOBAL_OFFSET = "_Z17get_global_offsetj";
const llvm::StringRef PreBIImportAnalysis::OCL_GET_LOCAL_ID = "_Z12get_local_idj";
const llvm::StringRef PreBIImportAnalysis::OCL_GET_GROUP_ID = "_Z12get_group_idj";
const llvm::StringRef PreBIImportAnalysis::OCL_GET_SUBGROUP_ID_IGC_SPVIR = "__builtin_spirv_BuiltInSubgroupId";
const llvm::StringRef PreBIImportAnalysis::OCL_GET_SUBGROUP_ID_KHR_SPVIR = "_Z25__spirv_BuiltInSubgroupIdv";
const llvm::StringRef PreBIImportAnalysis::OCL_GET_SUBGROUP_ID = "_Z16get_sub_group_idv";
const llvm::StringRef PreBIImportAnalysis::OCL_SUBGROUP_BLOCK_PREFIX = "__builtin_spirv_OpSubgroupBlock";
const llvm::StringRef PreBIImportAnalysis::OCL_SUBGROUP_IMAGE_BLOCK_PREFIX = "__builtin_spirv_OpSubgroupImageBlock";

PreBIImportAnalysis::PreBIImportAnalysis() : ModulePass(ID)
{
    initializeWIFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool PreBIImportAnalysis::runOnModule(Module& M)
{
    // Run on all functions defined in this module
    SmallVector<std::tuple<Instruction*, double, unsigned>, 8> InstToModify;
    SmallVector<std::pair<Instruction*, Instruction*>, 8> CallToReplace;

    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);

        // To make the behavior predictable, fix the workgroup XYZ walk order here.
        if (IGC_IS_FLAG_ENABLED(ForceXYZworkGroupWalkOrder))
        {
            MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
            if (isEntryFunc(pMdUtils, pFunc)) {
                uint32_t WGSize = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, pFunc);
                uint32_t WGSizeHint = IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, pFunc);
                if (WGSize != 1 && WGSizeHint != 1)
                {
                    modMD->FuncMD[pFunc].workGroupWalkOrder.dim0 = 0;
                    modMD->FuncMD[pFunc].workGroupWalkOrder.dim1 = 1;
                    modMD->FuncMD[pFunc].workGroupWalkOrder.dim2 = 2;
                }
            }
        }

        StringRef funcName = pFunc->getName();
        bool isFuncNameToSearch = (funcName == OCL_GET_GLOBAL_OFFSET ||
            funcName == OCL_GET_LOCAL_ID ||
            funcName == OCL_GET_GROUP_ID ||
            funcName == OCL_GET_SUBGROUP_ID_IGC_SPVIR ||
            funcName == OCL_GET_SUBGROUP_ID_KHR_SPVIR ||
            funcName == OCL_GET_SUBGROUP_ID);
        bool isSubgroupBlockFunc = false;
        const auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        if (pCtx->platform.hasHWLocalThreadID())
        {
            isSubgroupBlockFunc = funcName.contains(OCL_SUBGROUP_BLOCK_PREFIX) ||
                funcName.contains(OCL_SUBGROUP_IMAGE_BLOCK_PREFIX);
            isFuncNameToSearch |= isSubgroupBlockFunc;
        }
        if (isFuncNameToSearch)
        {
            MetaDataUtils* pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

            // Breadth-first search

            std::set<llvm::Function*>   visited;
            std::queue<llvm::Function*> functQueue;

            // push _Z... into queue first
            functQueue.push(pFunc);

            while (!functQueue.empty())
            {
                Function* pCallee = functQueue.front();
                functQueue.pop();

                if (visited.find(pCallee) != visited.end())
                {
                    // this function has been visited before
                    continue;
                }

                // add the function into visited pool
                visited.insert(pCallee);

                // search all users(callers)
                std::set<llvm::Function*> callerSet;
                for (auto U = pCallee->user_begin(), UE = pCallee->user_end(); U != UE; ++U)
                {
                    CallInst* cInst = dyn_cast<CallInst>(*U);
                    if (cInst)
                    {
                        callerSet.insert(cInst->getParent()->getParent());
                    }
                }

                // loop all callers
                for (auto f : callerSet) {
                  if (visited.find(f) == visited.end()) {
                    // this function is not visited before,
                    // insert it into queue
                    functQueue.push(f);
                  }
                }
            }

            // search all marked functions
            for (auto f : visited)
            {
              if (pMdUtil->findFunctionsInfoItem(f) != pMdUtil->end_FunctionsInfo())
              {
                // It is kernel Function, set metaData
                if (funcName == OCL_GET_GLOBAL_OFFSET)
                {
                  modMD->FuncMD[f].globalIDPresent = true;
                }
                else if (funcName == OCL_GET_LOCAL_ID)
                {
                  //localIDPresent info will be added to new framework here
                  //and extracted from new framework later
                  modMD->FuncMD[f].localIDPresent = true;
                }
                else if (funcName == OCL_GET_GROUP_ID)
                {
                  //groupIDPresent info will be added to new framework here
                  //and extracted from new framework later
                  modMD->FuncMD[f].groupIDPresent = true;
                }
                else if (funcName == OCL_GET_SUBGROUP_ID_IGC_SPVIR ||
                         funcName == OCL_GET_SUBGROUP_ID_KHR_SPVIR ||
                         funcName == OCL_GET_SUBGROUP_ID)
                {
                  if (!pCtx->platform.hasHWLocalThreadID())
                  {
                    // For pre-XeHP_SDV currently without using patch tokens to request local thread id from UMD,
                    // we are forcing walk order 0 1 2 when we have get_subgroup_id in kernel.
                    modMD->FuncMD[f].workGroupWalkOrder.dim0 = 0;
                    modMD->FuncMD[f].workGroupWalkOrder.dim1 = 1;
                    modMD->FuncMD[f].workGroupWalkOrder.dim2 = 2;
                  }
                }
                else if (isSubgroupBlockFunc)
                {
                    if (modMD->FuncMD[f].workGroupWalkOrder.dim0 == 0 &&
                        modMD->FuncMD[f].workGroupWalkOrder.dim1 == 0 &&
                        modMD->FuncMD[f].workGroupWalkOrder.dim2 == 0)
                    {
                        modMD->FuncMD[f].workGroupWalkOrder.dim0 = 0;
                        modMD->FuncMD[f].workGroupWalkOrder.dim1 = 1;
                        modMD->FuncMD[f].workGroupWalkOrder.dim2 = 2;
                    }
                }
              }
            }
        }

        std::string sinBuiltinName = "_Z15__spirv_ocl_sinf";
        std::string cosBuiltinName = "_Z15__spirv_ocl_cosf";
        std::string sinPiBuiltinName = "_Z17__spirv_ocl_sinpif";
        std::string cosPiBuiltinName = "_Z17__spirv_ocl_cospif";
        auto violatesPromotionConds = [&](Instruction* inst) {
            IGC_ASSERT(inst->getOpcode() == Instruction::FMul || inst->getOpcode() == Instruction::Load);
            if (inst && inst->hasNUsesOrMore(2)) {
                Value::use_iterator instUse = inst->use_begin();
                Value::use_iterator instEnd = inst->use_end();

                for (; instUse != instEnd; ++instUse) {
                    if (CallInst* useInst =
                        dyn_cast<CallInst>(instUse->getUser())) {
                        StringRef funcName = useInst->getCalledFunction()->getName();
                        if (!funcName.startswith(cosBuiltinName) &&
                            !funcName.startswith(sinBuiltinName) &&
                            !funcName.startswith(sinPiBuiltinName) &&
                            !funcName.startswith(cosPiBuiltinName)) {
                            return true;
                        }
                    }
                    else
                        return true;
                }
            }
            return false;
        };

        auto modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if ((modMD->compOpt.MatchSinCosPi) &&
            !(modMD->compOpt.FastRelaxedMath) &&
            (funcName.startswith(cosBuiltinName) ||
             funcName.startswith(sinBuiltinName))) {
          for (auto Users : pFunc->users()) {
            if (auto CI = dyn_cast<CallInst>(Users)) {
              IRBuilder<> builder(CI);
              Value* inputV = CI->getOperand(0);
              bool isCandidate = false;
              BinaryOperator* fmulInst = dyn_cast<BinaryOperator>(inputV);

              if (LoadInst* load = dyn_cast<LoadInst>(inputV)) {
                Value* ptrV = load->getPointerOperand();

                if (AllocaInst* ptrAlloca = dyn_cast<AllocaInst>(ptrV)) {
                  Value::use_iterator allocaUse = ptrV->use_begin();
                  Value::use_iterator allocaUseEnd = ptrV->use_end();
                  StoreInst* store = nullptr;

                  for (; allocaUse != allocaUseEnd; ++allocaUse) {
                    if (StoreInst* useInst =
                      dyn_cast<StoreInst>(allocaUse->getUser())) {
                      if (store == nullptr) {
                        store = dyn_cast<StoreInst>(allocaUse->getUser());
                      }
                      else {
                        store = nullptr;
                        break;
                      }
                    }
                  }

                  Value* storeV = nullptr;

                  if (store && (storeV = store->getValueOperand())) {
                    fmulInst = dyn_cast<BinaryOperator>(storeV);
                  }
                }

                if (fmulInst && fmulInst->getOpcode() == Instruction::FMul)
                    isCandidate = !violatesPromotionConds(load) && !violatesPromotionConds(fmulInst);
              }

              if (isCandidate) {
                unsigned srcPos = 0;
                ConstantFP *fmulConstant =
                    dyn_cast<ConstantFP>(fmulInst->getOperand(0));

                if (!fmulConstant) {
                  fmulConstant = dyn_cast<ConstantFP>(fmulInst->getOperand(1));
                  srcPos = 1;
                }

                double intValue = 0.0;
                double fractValue = 1.0;
                if (fmulConstant) {
                  auto APF = fmulConstant->getValueAPF();
                  llvm::Type *type = fmulInst->getType();
                  double fmulValue = type->isFloatTy() ? APF.convertToFloat()
                                                       : APF.convertToDouble();
                  const double PI =
                      3.1415926535897932384626433832795028841971693993751058209;
                  double coefficient = fmulValue / PI;
                  intValue = trunc(coefficient);
                  fractValue = coefficient - intValue;
                }

                if (intValue != 0.0 && fabs(fractValue) <= 0.0001) {
                  InstToModify.push_back(
                      std::make_tuple(fmulInst, intValue, srcPos));

                  std::string newName;
                  if (funcName.startswith(cosBuiltinName)) {
                    newName = cosPiBuiltinName;
                  } else if (funcName.startswith(sinBuiltinName)) {
                    newName = sinPiBuiltinName;
                  }

                  Function *newFunc = M.getFunction(newName);
                  if (newFunc == nullptr) {
                    FunctionType *FT = pFunc->getFunctionType();
                    newFunc = Function::Create(FT, pFunc->getLinkage(),
                                                  newName, M);
                  }
                  SmallVector<Value *, 8> Args;
                  for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(CI);
                       I != E; ++I) {
                    Args.push_back(CI->getArgOperand(I));
                  }
                  auto newCI = CallInst::Create(newFunc, Args);
                  newCI->setCallingConv(CI->getCallingConv());
                  CallToReplace.push_back(
                      std::pair<Instruction *, Instruction *>(CI, newCI));
                }
              }
            }
          }
        }

        // Clang and SPIRV-Translator support integer-to-integer conversions with a provided rounding mode.
        // Before LLVM 16, these cases were ignored by SPIRV-Translator, but with LLVM 16, IGC can receive
        // builtins with a rounding mode for int-to-int conversions. For these conversions, we do not need to take
        // the rounding mode into account and can handle them as default conversion builtins. To address this,
        // we need to investigate these functions and strip the rounding mode from the function name.
        // This also requires updating the length field in the builtin name. In the end, we should replace
        // the old functions with the new ones.
        // Clang: https://clang.llvm.org/doxygen/opencl-c_8h_source.html
        // SPIRV-Translator: https://github.com/KhronosGroup/SPIRV-LLVM-Translator/blob/main/lib/SPIRV/runtime/OpenCL/inc/spirv_convert.h
        //
        // Example of handling conversion by the code below:
        // Before:
        //     declare spir_func i8 @_Z30__spirv_SConvert_Rchar_sat_rtei(i32)
        // After:
        //     declare spir_func i8 @_Z26__spirv_SConvert_Rchar_sati(i32)

        std::string sConvert ="__spirv_SConvert";
        std::string uConvert ="__spirv_UConvert";
        std::string satConvertUToS ="__spirv_SatConvertUToS";
        std::string satConvertSToU ="__spirv_SatConvertSToU";

        auto stripRoundingMode = [](const std::string& mangled) -> std::string {
            const std::vector<std::string> roundingModes = {"_rtz", "_rte", "_rtp", "_rtn"};
            if (mangled[0] != '_' || mangled[1] != 'Z' || !std::isdigit(mangled[2]))
                return mangled;

            size_t lenStart = 2;
            size_t lenEnd = lenStart;
            while (lenEnd < mangled.size() && std::isdigit(mangled[lenEnd])) {
                ++lenEnd;
            }
            if (lenEnd == lenStart)
                return mangled;

            int oldLen = std::stoi(mangled.substr(lenStart, lenEnd - lenStart));
            std::string rest = mangled.substr(lenEnd);

            for (const auto& mode : roundingModes) {
                size_t pos = rest.find(mode);
                if (pos != std::string::npos) {
                    rest.erase(pos, mode.length());
                    int newLen = oldLen - mode.length();
                    return mangled.substr(0, lenStart) + std::to_string(newLen) + rest;
                }
            }
            return mangled;
        };

        if(funcName.contains(sConvert) || funcName.contains(uConvert) ||
           funcName.contains(satConvertUToS) || funcName.contains(satConvertSToU)) {

            std::string newName = stripRoundingMode(funcName.str());

            for (auto Users : pFunc->users()) {
                if (auto CI = dyn_cast<CallInst>(Users)) {
                    Function *newFunc = M.getFunction(newName);
                    if (newFunc == nullptr) {
                        FunctionType *FT = pFunc->getFunctionType();
                        newFunc = Function::Create(FT, pFunc->getLinkage(), newName, M);
                    }
                    SmallVector<Value *, 8> Args;
                    for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(CI); I != E; ++I) {
                        Args.push_back(CI->getArgOperand(I));
                    }
                    auto newCI = CallInst::Create(newFunc, Args);
                    newCI->setCallingConv(CI->getCallingConv());
                    CallToReplace.push_back(std::pair<Instruction *, Instruction *>(CI, newCI));
                }
            }
        }
    }

    for (const auto &InstTuple : InstToModify)
    {
      auto [inst, value, srcPos] = InstTuple;
      inst->setOperand(
          srcPos,
          ConstantFP::get(
          Type::getFloatTy(inst->getContext()),
          (float) value));
    }

    for (const auto& CIPair : CallToReplace)
    {
      auto oldCI = CIPair.first;
      auto newCI = CIPair.second;
      oldCI->replaceAllUsesWith(newCI);
      ReplaceInstWithInst(oldCI, newCI);
    }

    return true;
}
