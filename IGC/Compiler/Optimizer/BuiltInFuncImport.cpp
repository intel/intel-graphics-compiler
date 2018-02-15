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
#include "Compiler/Optimizer/BuiltInFuncImport.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataApi/IGCMetaDataDefs.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/Error.h>
#include "common/LLVMWarningsPop.hpp"

#include <unordered_map>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

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

Function *BIImport::GetBuiltinFunction(llvm::StringRef funcName) const
{
    Function *pFunc = nullptr;
    if ((pFunc = m_GenericModule->getFunction(funcName)) && !pFunc->isDeclaration())
        return pFunc;
    // If the generic and size modules are linked before hand, don't
    // look in the size module because it doesn't exist.
    else if (m_SizeModule && (pFunc = m_SizeModule->getFunction(funcName)) && !pFunc->isDeclaration())
        return pFunc;

    return nullptr;
}

static bool materialized_use_empty(const Value *v)
{
    return v->materialized_use_begin() == v->use_end();
}

static bool isMangledImageFn(StringRef FName, const MangleSubstTy &MangleSubst)
{
    bool UpdateMangle = std::any_of(MangleSubst.begin(), MangleSubst.end(),
        [=](PairTy Pair) { return FName.find(Pair.first) != StringRef::npos; });

    return (FName.startswith("_Z") && UpdateMangle);
}

static std::string updatedMangleName(const std::string &FuncName, const std::string &Mangle)
{
    std::string Qual = (FuncName.find("write") != std::string::npos) ? "_wo" : "_ro";
    return Mangle + Qual;
}

static std::string updateSPIRmangleName(StringRef FuncName, const MangleSubstTy &MangleSubst)
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

bool BIImport::runOnModule(Module &M)
{
    if (m_GenericModule == nullptr)
    {
        return false;
    }

    
    for (auto &F : M)
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
                NewFuncName = FuncName;
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

    std::function<void(Function*)> Explore = [&](Function *pRoot) -> void
    {
        TFunctionsVec calledFuncs;
        GetCalledFunctions(pRoot, calledFuncs);

        for (auto *pCallee : calledFuncs)
        {
            Function *pFunc = nullptr;
            if (pCallee->isDeclaration())
            {
                auto funcName = pCallee->getName();
                Function* pSrcFunc = GetBuiltinFunction(funcName);
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
                    handleAllErrors(std::move(Err), [&](ErrorInfoBase &EIB) {
                        errs() << "===> Materialize Failure: " << EIB.message().c_str() << '\n';
                    });
                    assert(0 && "Failed to materialize Global Variables");
                }
                else {
                    pFunc->addAttribute(AttributeSet::FunctionIndex, llvm::Attribute::Builtin);
                    Explore(pFunc);
                }
            }
        }
    };

    for (auto &func : M)
    {
        Explore(&func);
    }

    // nuke the unused functions so we can materializeAll() quickly
    auto CleanUnused = [](Module *Module)
    {
        for (auto I = Module->begin(), E = Module->end(); I != E; )
        {
            auto *F = &(*I++);
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
        assert(0 && "materializeAll failed for generic builtin module");
    }

    if (ld.linkInModule(std::move(m_GenericModule)))
    {
        assert(0 && "Error linking generic builtin module");
    }

    if (m_SizeModule)
    {
        CleanUnused(m_SizeModule.get());
        if (Error err = m_SizeModule->materializeAll())
        {
            assert(0 && "materializeAll failed for size_t builtin module");
        }

        if (ld.linkInModule(std::move(m_SizeModule)))
        {
            assert(0 && "Error linking size_t builtin module");
        }
    }

    InitializeBIFlags(M);
    removeFunctionBitcasts(M);

    //temporary work around for sampler types and pipes
    for (auto &func : M)
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
                    CI->eraseFromParent();
                }
            }
        }
    }

    return true;
}

void BIImport::GetCalledFunctions(const Function* pFunc, TFunctionsVec& calledFuncs)
{
    SmallPtrSet<Function*, 8> visitedSet;
    // Iterate over function instructions and look for call instructions
    for (const_inst_iterator it = inst_begin(pFunc), e = inst_end(pFunc); it != e; ++it)
    {
        const CallInst *pInstCall = dyn_cast<CallInst>(&*it);
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

void BIImport::removeFunctionBitcasts(Module &M)
{
    std::vector<Instruction*> list_delete;
    DenseMap<Function *, std::vector<Function *>> bitcastFunctionMap;

    for (Function &func : M)
    {
        for (auto &BB : func)
        {
            for (auto I = BB.begin(), E = BB.end(); I != E; I++)
            {
                CallInst *pInstCall = dyn_cast<CallInst>(I);
                if (!pInstCall || pInstCall->getCalledFunction()) continue;
                if (auto constExpr = dyn_cast<llvm::ConstantExpr>(pInstCall->getCalledValue()))
                {
                    if (auto funcTobeChanged = dyn_cast<llvm::Function>(constExpr->stripPointerCasts()))
                    {
                        if (funcTobeChanged->isDeclaration()) continue;
                        // Map between values (functions) in source of bitcast
                        // to their counterpart values in destination
                        llvm::ValueToValueMapTy  operandMap;
                        Function *pDstFunc = nullptr;
                        auto BCFMI = bitcastFunctionMap.find(funcTobeChanged);
                        bool notExists = BCFMI == bitcastFunctionMap.end();
                        if (!notExists)
                        {
                            auto funcVec = bitcastFunctionMap[funcTobeChanged];
                            notExists = true;
                            for (Function *F : funcVec) {
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
                            AttributeSet FuncAttrs = funcTobeChanged->getAttributes();
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
                            CloneFunctionInto(
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
                        for (unsigned I = 0, E = pInstCall->getNumArgOperands(); I != E; ++I) {
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

void BIImport::InitializeBIFlags(Module &M)
{
    auto mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto MD = *(getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    /// @brief  Adds initialization to a global variable according to given value.
    ///         If the given global variable does not exist, does nothing.
    auto initializeVarWithValue = [&M](StringRef varName, uint32_t value)
    {
        GlobalVariable *gv = M.getGlobalVariable(varName);
        if (gv == nullptr)
            return;
        gv->setInitializer(ConstantInt::get(Type::getInt32Ty(M.getContext()), value));
    };

    bool isFlushDenormToZero =
        ((pCtx->m_floatDenormMode32 == FLOAT_DENORM_FLUSH_TO_ZERO) ||
            MD.compOpt.DenormsAreZero);
    initializeVarWithValue("__FlushDenormals", isFlushDenormToZero ? 1 : 0);

    initializeVarWithValue("__DashGSpecified", MD.compOpt.DashGSpecified ? 1 : 0);
    initializeVarWithValue("__FastRelaxedMath", MD.compOpt.RelaxedBuiltins ? 1 : 0);
    initializeVarWithValue("__UseNative64BitSubgroupBuiltin",
        pCtx->platform.hasNo64BitInst() ? 0 : 1);
    initializeVarWithValue("__CRMacros",
        pCtx->platform.hasCorrectlyRoundedMacros() ? 1 : 0);

    if (mdUtils->isInputIRVersionsHasValue() && mdUtils->size_InputIRVersions() > 0)
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
}

extern "C" llvm::ModulePass *createBuiltInImportPass(
    std::unique_ptr<Module> pGenericModule, std::unique_ptr<Module> pSizeModule)
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

PreBIImportAnalysis::PreBIImportAnalysis() : ModulePass(ID)
{
    initializeWIFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool PreBIImportAnalysis::runOnModule(Module &M)
{
    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);

        StringRef funcName = pFunc->getName();
        if (funcName == OCL_GET_GLOBAL_OFFSET ||
            funcName == OCL_GET_LOCAL_ID ||
            funcName == OCL_GET_GROUP_ID)
        {
            MetaDataUtils *pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();



            // Breadth-first search

            std::set<llvm::Function*>   visited;
            std::queue<llvm::Function*> functQueue;

            // push _Z... into queue first
            functQueue.push(pFunc);

            while (!functQueue.empty())
            {
                Function *pCallee = functQueue.front();
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
                    CallInst *cInst = dyn_cast<CallInst>(*U);
                    if (cInst)
                    {
                        callerSet.insert(cInst->getParent()->getParent());
                    }
                }

                // loop all callers
                for (auto f : callerSet)
                {
                    if (visited.find(f) == visited.end())
                    {
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
                        pMdUtil->getFunctionsInfoItem(f)->setGlobalOffsetPresent(1);
                    }
                    else if (funcName == OCL_GET_LOCAL_ID)
                    {
                        pMdUtil->getFunctionsInfoItem(f)->setLocalIDPresent(1);
                    }
                    else if (funcName == OCL_GET_GROUP_ID)
                    {
                        pMdUtil->getFunctionsInfoItem(f)->setGroupIDPresent(1);
                    }
                }
            }
        }
    }
    return true;
}
