/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/BuiltInFuncImport.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "llvmWrapper/IR/Attributes.h"
#include <llvm/IR/Function.h>
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/CallSite.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvmWrapper/Transforms/Utils/Cloning.h>
#include <llvm/Support/Error.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include "common/LLVMWarningsPop.hpp"
#include <unordered_set>
#include <unordered_map>
#include "Probe/Assertion.h"

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

BIImport::BIImport(std::unique_ptr<Module> pGenericModule, std::unique_ptr<Module> pSizeModule) :
    ModulePass(ID),
    m_GenericModule(std::move(pGenericModule)),
    m_SizeModule(std::move(pSizeModule))
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
    for (auto Key : MangleSubst)
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

Function* BIImport::GetBuiltinFunction2(llvm::StringRef funcName) const
{
    Function* pFunc = nullptr;
    if ((pFunc = m_GenericModule->getFunction(funcName)) && !pFunc->isDeclaration())
        return pFunc;
    // If the generic and size modules are linked before hand, don't
    // look in the size module because it doesn't exist.
    else if (m_SizeModule && (pFunc = m_SizeModule->getFunction(funcName)) && !pFunc->isDeclaration())
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

    std::function<void(Function*)> Explore = [&](Function* pRoot) -> void
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
                    pFunc->addFnAttr(llvm::Attribute::Builtin);
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

bool BIImport::runOnModule(Module& M)
{
    if (m_GenericModule == nullptr)
    {
        return false;
    }


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

    std::function<void(Function*)> Explore = [&](Function* pRoot) -> void
    {
        TFunctionsVec calledFuncs;
        GetCalledFunctions(pRoot, calledFuncs);

        for (auto* pCallee : calledFuncs)
        {
            Function* pFunc = nullptr;
            if (pCallee->isDeclaration())
            {
                auto funcName = pCallee->getName();
                Function* pSrcFunc = GetBuiltinFunction2(funcName);
                if (!pSrcFunc) continue;
                pFunc = pSrcFunc;
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
                    pFunc->addFnAttr(llvm::Attribute::Builtin);
                    Explore(pFunc);
                }
            }

            if (pFunc->getName().startswith("__builtin_IB_kmp_"))
            {
                pFunc->addFnAttr(llvm::Attribute::NoInline);
                pFunc->addFnAttr("KMPLOCK");
            }
        }
    };

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

    CleanUnused(m_GenericModule.get());
    Linker ld(M);

    if (Error err = m_GenericModule->materializeAll()) {
        IGC_ASSERT_MESSAGE(0, "materializeAll failed for generic builtin module");
    }

    if (ld.linkInModule(std::move(m_GenericModule)))
    {
        IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
    }

    if (m_SizeModule)
    {
        CleanUnused(m_SizeModule.get());
        if (Error err = m_SizeModule->materializeAll())
        {
            IGC_ASSERT_MESSAGE(0, "materializeAll failed for size_t builtin module");
        }

        if (ld.linkInModule(std::move(m_SizeModule)))
        {
            IGC_ASSERT_MESSAGE(0, "Error linking size_t builtin module");
        }
    }

    InitializeBIFlags(M);
    removeFunctionBitcasts(M);

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
                        newV = builder.CreatePtrToInt(CI->getOperand(0), CI->getType());
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
                    // Get the function by name
                    GetElementPtrInst* const funcStrV = cast<GetElementPtrInst>(CI->getArgOperand(0));
                    IGC_ASSERT(nullptr != funcStrV);
                    GlobalVariable* const global = cast<GlobalVariable>(funcStrV->getOperand(0));
                    IGC_ASSERT(nullptr != global);
                    ConstantDataArray* const gArray = cast<ConstantDataArray>(global->getInitializer());
                    IGC_ASSERT(nullptr != gArray);
                    Function* const pFunc = GetBuiltinFunction(gArray->getAsCString(), &M);
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
                    CallInst* callFunc = builder.CreateCall(funcPtr, argBuffer);
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
                    StringRef VariantName = IGCLLVM::getAttribute(CI->getAttributes(), AttributeList::FunctionIndex, "vector-variant")
                        .getValueAsString();

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
                        StringRef VariantsStr = IGCLLVM::getAttribute(CI->getAttributes(), AttributeList::FunctionIndex, "vector-variants")
                            .getValueAsString();
                        SmallVector<StringRef, 8> VariantsTable;
                        VariantsStr.split(VariantsTable, ',');

                        // Get the caller function
                        Function* callerF = CI->getParent()->getParent();
                        // Assume the caller has explicit subgroup size set, otherwise we cannot determine which variant to use
                        FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(callerF);
                        unsigned subgroup_size = (unsigned)funcInfoMD->getSubGroupSize()->getSIMD_size();
                        IGC_ASSERT(subgroup_size != 0);

                        // Parse each variant string in the table, stop at the first one that matches subgroup_size
                        for (auto var : VariantsTable)
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
                    // Load function address from the table index
                    Value* FP = builder.CreateGEP(FPTablePtr, builder.getInt32(tableIndex));
                    FP = builder.CreateLoad(FP);
                    IGC_ASSERT(FP->getType()->isPointerTy() && cast<PointerType>(FP->getType())->getPointerElementType()->isFunctionTy());
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
                if (!pInstCall || pInstCall->getCalledFunction()) continue;
                if (auto constExpr = dyn_cast<llvm::ConstantExpr>(IGCLLVM::getCalledValue(pInstCall)))
                {
                    if (auto funcTobeChanged = dyn_cast<llvm::Function>(constExpr->stripPointerCasts()))
                    {
                        if (funcTobeChanged->isDeclaration()) continue;
                        // Map between values (functions) in source of bitcast
                        // to their counterpart values in destination
                        llvm::ValueToValueMapTy  operandMap;
                        Function* pDstFunc = nullptr;
                        auto BCFMI = bitcastFunctionMap.find(funcTobeChanged);
                        bool notExists = BCFMI == bitcastFunctionMap.end();
                        if (!notExists)
                        {
                            auto funcVec = bitcastFunctionMap[funcTobeChanged];
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
                            pDstFunc = Function::Create(pInstCall->getFunctionType(), funcTobeChanged->getLinkage(), funcTobeChanged->getName(), &M);
                            if (pDstFunc->arg_size() != funcTobeChanged->arg_size()) continue;
                            // Need to copy the attributes over too.
                            AttributeList FuncAttrs = funcTobeChanged->getAttributes();
                            pDstFunc->setAttributes(FuncAttrs);

                            // Go through and convert function arguments over, remembering the mapping.
                            Function::arg_iterator itSrcFunc = funcTobeChanged->arg_begin();
                            Function::arg_iterator eSrcFunc = funcTobeChanged->arg_end();
                            llvm::Function::arg_iterator itDest = pDstFunc->arg_begin();

                            for (; itSrcFunc != eSrcFunc; ++itSrcFunc, ++itDest)
                            {
                                itDest->setName(itSrcFunc->getName());
                                operandMap[&(*itSrcFunc)] = &(*itDest);
                            }

                            // Clone the body of the function into the dest function.
                            SmallVector<ReturnInst*, 8> Returns; // Ignore returns.
                            IGCLLVM::CloneFunctionInto(
                                pDstFunc,
                                funcTobeChanged,
                                operandMap,
                                false,
                                Returns,
                                "");

                            pDstFunc->setCallingConv(funcTobeChanged->getCallingConv());
                            bitcastFunctionMap[funcTobeChanged].push_back(pDstFunc);
                        }

                        std::vector<Value*> Args;
                        for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(pInstCall); I != E; ++I) {
                            Args.push_back(pInstCall->getArgOperand(I));
                        }
                        auto newCI = CallInst::Create(pDstFunc, Args, "", pInstCall);
                        newCI->takeName(pInstCall);
                        newCI->setCallingConv(pInstCall->getCallingConv());
                        pInstCall->replaceAllUsesWith(newCI);
                        pInstCall->dropAllReferences();
                        if (constExpr->use_empty())
                            constExpr->dropAllReferences();
                        if (funcTobeChanged->use_empty())
                            funcTobeChanged->eraseFromParent();

                        list_delete.push_back(pInstCall);
                    }
                }
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
    auto MD = *(getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    /// @brief  Adds initialization to a global variable according to given value.
    ///         If the given global variable does not exist, does nothing.
    auto initializeVarWithValue = [&M](StringRef varName, uint32_t value)
    {
        GlobalVariable* gv = M.getGlobalVariable(varName);
        if (gv == nullptr)
            return;
        gv->setInitializer(ConstantInt::get(Type::getInt32Ty(M.getContext()), value));
    };

    /// @brief Set global variable linkage to extrnal. This is needed for
    ///        variables that will become relocations.
    auto makeVarExternal = [&M](StringRef varName)
    {
        GlobalVariable *gv = M.getGlobalVariable(varName);
        if (gv == nullptr)
            return;
        gv->setLinkage(GlobalValue::ExternalLinkage);
    };

    bool isFlushDenormToZero =
        ((pCtx->m_floatDenormMode32 == FLOAT_DENORM_FLUSH_TO_ZERO) ||
            MD.compOpt.DenormsAreZero);
    initializeVarWithValue("__FlushDenormals", isFlushDenormToZero ? 1 : 0);
    initializeVarWithValue("__DashGSpecified", MD.compOpt.DashGSpecified ? 1 : 0);
    initializeVarWithValue("__FastRelaxedMath", MD.compOpt.RelaxedBuiltins ? 1 : 0);
    initializeVarWithValue("__MadEnable", MD.compOpt.MadEnable ? 1 : 0);
    initializeVarWithValue("__OptDisable", MD.compOpt.OptDisable ? 1 : 0);
    bool isUseMathWithLUTEnabled = false;
    if (IGC_IS_FLAG_ENABLED(UseMathWithLUT))
    {
        isUseMathWithLUTEnabled = true;
    }
    initializeVarWithValue("__UseMathWithLUT", isUseMathWithLUTEnabled ? 1 : 0);
    initializeVarWithValue("__UseNativeFP32GlobalAtomicAdd", pCtx->platform.hasFP32GlobalAtomicAdd() ? 1 : 0);
    initializeVarWithValue("__UseNativeFP16AtomicMinMax", pCtx->platform.hasFP16AtomicMinMax() ? 1 : 0);
    initializeVarWithValue("__HasInt64SLMAtomicCAS", pCtx->platform.hasInt64SLMAtomicCAS() ? 1 : 0);
    initializeVarWithValue("__UseNativeFP64GlobalAtomicAdd", pCtx->platform.hasFP64GlobalAtomicAdd() ? 1 : 0);
    initializeVarWithValue("__UseNative64BitIntBuiltin", pCtx->platform.hasNoFullI64Support() ? 0 : 1);
    initializeVarWithValue("__UseNative64BitFloatBuiltin", pCtx->platform.hasNoFP64Inst() ? 0 : 1);
    initializeVarWithValue("__CRMacros",
        pCtx->platform.hasCorrectlyRoundedMacros() ? 1 : 0);

    if (StringRef(pCtx->getModule()->getTargetTriple()).size() > 0)
    {
        initializeVarWithValue("__APIRS", false);
    }

    if (pCtx->type == ShaderType::OPENCL_SHADER)
    {
        bool isSPIRV = static_cast<OpenCLProgramContext*>(pCtx)->isSPIRV();
        initializeVarWithValue("__IsSPIRV", isSPIRV);
    }

    initializeVarWithValue("__EnableSWSrgbWrites", IGC_GET_FLAG_VALUE(cl_khr_srgb_image_writes));

    if (pCtx->type == ShaderType::OPENCL_SHADER)
    {
        float profilingTimerResolution = static_cast<OpenCLProgramContext*>(pCtx)->getProfilingTimerResolution();
        initializeVarWithValue("__ProfilingTimerResolution", *reinterpret_cast<int*>(&profilingTimerResolution));
    }

    makeVarExternal("__SubDeviceID");
    initializeVarWithValue("__MaxHWThreadIDPerSubDevice", pCtx->platform.GetGTSystemInfo().ThreadCount);
}

extern "C" llvm::ModulePass* createBuiltInImportPass(
    std::unique_ptr<Module> pGenericModule,
    std::unique_ptr<Module> pSizeModule)
{
    return new BIImport(std::move(pGenericModule), std::move(pSizeModule));
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

PreBIImportAnalysis::PreBIImportAnalysis() : ModulePass(ID)
{
    initializeWIFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool PreBIImportAnalysis::runOnModule(Module& M)
{
    // Run on all functions defined in this module
    SmallVector<std::pair<Instruction*, double>, 8> InstToModify;
    SmallVector<std::pair<Function*, std::string>, 8> FuncToRename;
    SmallVector<std::pair<Instruction*, Instruction*>, 8> CallToReplace;

    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);

        StringRef funcName = pFunc->getName();
        bool isFuncNameToSearch = (funcName == OCL_GET_GLOBAL_OFFSET ||
            funcName == OCL_GET_LOCAL_ID ||
            funcName == OCL_GET_GROUP_ID ||
            funcName == OCL_GET_SUBGROUP_ID_IGC_SPVIR ||
            funcName == OCL_GET_SUBGROUP_ID_KHR_SPVIR ||
            funcName == OCL_GET_SUBGROUP_ID);
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
                    modMD->FuncMD[f].workGroupWalkOrder.dim0 = 0;
                    modMD->FuncMD[f].workGroupWalkOrder.dim1 = 1;
                    modMD->FuncMD[f].workGroupWalkOrder.dim2 = 2;
                }
              }
            }
        }

#if defined(IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR)
        std::string sinBuiltinName = "_Z15__spirv_ocl_sinf";
        std::string cosBuiltinName = "_Z15__spirv_ocl_cosf";
        std::string sinPiBuiltinName = "_Z17__spirv_ocl_sinpif";
        std::string cosPiBuiltinName = "_Z17__spirv_ocl_cospif";
#else // IGC Legacy SPIRV Translator
        std::string sinBuiltinName = "__builtin_spirv_OpenCL_sin_f32";
        std::string cosBuiltinName = "__builtin_spirv_OpenCL_cos_f32";
        std::string sinPiBuiltinName = "__builtin_spirv_OpenCL_sinpi_f32";
        std::string cosPiBuiltinName = "__builtin_spirv_OpenCL_cospi_f32";
#endif
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
              }

              if (fmulInst && fmulInst->getOpcode() == Instruction::FMul) {
                isCandidate = true;
              }

              // used to be: fmulInst->getNumUses() > 1
              if (isCandidate && fmulInst->hasNUsesOrMore(2)) {
                Value::use_iterator fmulUse = fmulInst->use_begin();
                Value::use_iterator fmulUseEnd = fmulInst->use_end();

                for (; fmulUse != fmulUseEnd; ++fmulUse) {
                  if (CallInst* useInst =
                    dyn_cast<CallInst>(fmulUse->getUser())) {
                    StringRef funcName = useInst->getCalledFunction()->getName();
                    if (!funcName.startswith(cosBuiltinName) &&
                      !funcName.startswith(sinBuiltinName)) {
                      isCandidate = false;
                      break;
                    }
                  }
                  else
                  {
                    isCandidate = false;
                    break;
                  }
                }
              }

              if (isCandidate) {
                ConstantFP *fmulConstant =
                    dyn_cast<ConstantFP>(fmulInst->getOperand(0));

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

                if (fabs(fractValue) <= 0.0001) {
                  InstToModify.push_back(
                      std::pair<Instruction *, double>(fmulInst, intValue));

                  std::string newName;
                  if (funcName.startswith(cosBuiltinName)) {
                    newName = cosPiBuiltinName;
                  } else if (funcName.startswith(sinBuiltinName)) {
                    newName = sinPiBuiltinName;
                  }

                  if (Function *newFunc = M.getFunction(newName)) {
                    SmallVector<Value *, 8> Args;
                    for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(CI); I != E;
                         ++I) {
                      Args.push_back(CI->getArgOperand(I));
                    }
                    auto newCI = CallInst::Create(newFunc, Args);
                    newCI->setCallingConv(CI->getCallingConv());
                    CallToReplace.push_back(
                        std::pair<Instruction *, Instruction *>(CI, newCI));
                  } else {
                    FuncToRename.push_back(
                        std::pair<Function*, std::string>(pFunc, newName));
                  }
                }
              }
            }
          }
        }
    }

    for (auto InstPair : InstToModify)
    {
      auto inst = InstPair.first;
      auto value = InstPair.second;
      inst->setOperand(
        0, ConstantFP::get(
          Type::getFloatTy(inst->getContext()),
          (float) value));
    }

    for (auto FuncPair : FuncToRename)
    {
      auto func = FuncPair.first;
      auto name = FuncPair.second;
      func->setName(name);
    }

    for (auto CIPair : CallToReplace)
    {
      auto oldCI = CIPair.first;
      auto newCI = CIPair.second;
      oldCI->replaceAllUsesWith(newCI);
      ReplaceInstWithInst(oldCI, newCI);
    }

    return true;
}
