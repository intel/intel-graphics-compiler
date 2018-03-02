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

#include "ProcessFuncAttributes.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataApi/IGCMetaDataDefs.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"
#include <string>
#include <set>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

class ProcessFuncAttributes : public ModulePass
{
public:
    static char ID;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    ProcessFuncAttributes();

    ~ProcessFuncAttributes() {}

    virtual bool runOnModule(Module &M);

    virtual llvm::StringRef getPassName() const
    {
        return "ProcessFuncAttributes";
    }

private:
    bool isGASPointer(Value* arg);

};

} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-process-func-attributes"
#define PASS_DESCRIPTION "Set Functions' linkage and attributes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ProcessFuncAttributes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
//IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ProcessFuncAttributes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ProcessFuncAttributes::ID = 0;

ProcessFuncAttributes::ProcessFuncAttributes() : ModulePass(ID)
{
    initializeProcessFuncAttributesPass(*PassRegistry::getPassRegistry());
}

inline bool ProcessFuncAttributes::isGASPointer(Value* V)
{
    if (PointerType *PTy = dyn_cast<PointerType>(V->getType()))
    {
        return PTy->getAddressSpace() == ADDRESS_SPACE_GENERIC;
    }
    return false;
}

ModulePass *createProcessFuncAttributesPass()
{
    return new ProcessFuncAttributes();
}

extern bool isSupportedAggregateArgument(Argument* arg);

// Only pointer, struct and array types are considered. E.g. vector type
// cannot contain opaque subtypes, function type may contain but ignored.
static void getContainedStructType(Type *T, SmallPtrSetImpl<StructType *> &Tys)
{
    if (StructType *ST = dyn_cast<llvm::StructType>(T))
    {
        // Check if this has been checked, to avoid spinning on %T = { %T *}.
        if (!Tys.count(ST))
        {
            Tys.insert(ST);
            for (auto I = ST->element_begin(), E = ST->element_end(); I != E; ++I)
            {
                getContainedStructType(*I, Tys);
            }
        }
    }
    else if (auto PT = dyn_cast<PointerType>(T))
    {
        return getContainedStructType(PT->getElementType(), Tys);
    }
    else if (auto AT = dyn_cast<ArrayType>(T))
    {
        return getContainedStructType(AT->getElementType(), Tys);
    }
}

// Check the existence of an opaque type.
static bool containsOpaque(llvm::Type *T)
{
    // All (nested) struct types in T.
    SmallPtrSet<StructType *, 8> StructTys;
    getContainedStructType(T, StructTys);

    for (auto I = StructTys.begin(), E = StructTys.end(); I != E; ++I)
    {
        StructType *ST = *I;
        if (ST->isOpaque())
        {
            return true;
        }
    }

    return false;
}

static bool hasSLMUsage(llvm::Module &M)
{
    for (auto &G : M.getGlobalList())
    {
        if (!G.use_empty() &&
            G.getType()->getAddressSpace() == ADDRESS_SPACE_LOCAL)
        {
            return true;
        }
    }

    const char *BUILTIN_MEMPOOL = "__builtin_IB_AllocLocalMemPool";
    if (auto F = M.getFunction(BUILTIN_MEMPOOL))
    {
        return !F->use_empty();
    }

    return false;
}

bool ProcessFuncAttributes::runOnModule(Module& M)
{
    MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
    MetaDataUtils *pMdUtils = mduw.getMetaDataUtils();
    ModuleMetaData *modMD = mduw.getModuleMetaData();
    bool containsSLM = false; // hasSLMUsage(M);

    std::set<llvm::Function *> fastMathFunct;
    GlobalVariable *gv_fastMath = M.getGlobalVariable("__FastRelaxedMath", true);
    if (gv_fastMath)
    {
        if (gv_fastMath->getInitializer()->isOneValue())
        {
            // Find the functions which __FastRelaxedMath belongs to.... 
            for (Value::user_iterator U = gv_fastMath->user_begin(), UE = gv_fastMath->user_end(); U != UE; ++U)
            {
                Instruction* user = dyn_cast<Instruction>(*U);
                if (!user)
                {
                    continue;
                }

                fastMathFunct.insert(user->getParent()->getParent());
            }
        }
    }

    // 1. Set function's linkage type to InternalLinkage (C's static) so that
    //    LLVM can remove the dead functions asap, which saves compiling time.
    //    Only non-kernel function with function bodies are set.
    //
    // 2. For correctness, add AlwaysInline to all functions' attributes so
    //    that AlwaysInliner will inline all of them.
    bool Changed = false;
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function *F = &(*I);
        if (!F || F->isDeclaration())
        {
            if (F->getName() == "__translate_sampler_initializer")
                F->addFnAttr(llvm::Attribute::ReadOnly);
            // It is not a defined function
            continue;
        }
        // Remove noinline attr if present.
        F->removeFnAttr(llvm::Attribute::NoInline);

        // Add AlwaysInline attribute to force inlining all calls.
        F->addFnAttr(llvm::Attribute::AlwaysInline);

        // set function attributes according to build options so
        // inliner doesn't conservatively turn off unsafe optimizations
        // when inlining BIFs (see mergeAttributesForInlining() in inliner).

        const auto &opts = modMD->compOpt;

        if (opts.MadEnable)
            F->addFnAttr("less-precise-fpmad", "true");

        if (opts.UnsafeMathOptimizations || opts.FastRelaxedMath)
            F->addFnAttr("unsafe-fp-math", "true");

        if (opts.FiniteMathOnly || opts.FastRelaxedMath)
        {
            F->addFnAttr("no-infs-fp-math", "true");
            F->addFnAttr("no-nans-fp-math", "true");
        }

        // F is not a kernel
        // it is builtin, or user function
        const bool notKernel =  pMdUtils->findFunctionsInfoItem(F) == pMdUtils->end_FunctionsInfo();

        if (notKernel)
        {
            F->setLinkage(GlobalValue::InternalLinkage);
            Changed = true;
        }

        // inline all OCL math functions if __FastRelaxedMath is set
        if (fastMathFunct.find(F) != fastMathFunct.end()) continue;

        // The following subroutine check is added to disable two-phase-inlining
        // when we do not enable subroutines.
        bool keepAlwaysInline = containsSLM;
        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            // OCL2.0 allows Objective C's blocks which introduce users other
            // than calls. Keep AlwaysInline attribute if there is any use other
            // than call. This is missing feature that igc cannot handle
            // indirect calls yet. Also AddImplicitArgs cannot handle load/store
            // with function pointers.
            if (!keepAlwaysInline)
            {
                for (auto U : F->users())
                {
                    if (!isa<CallInst>(U))
                    {
                        keepAlwaysInline = true;
                        break;
                    }
                }
            }

            if (!keepAlwaysInline)
            {
                for (auto &arg : F->getArgumentList())
                {
                    // If argument contains an opaque type e.g. image, then always inline it.
                    // If argument is a pointer to GAS, always inline it for perf reason.
                    //
                    // Note that this workaround should be removed.
                    if (containsOpaque(arg.getType()) || isSupportedAggregateArgument(&arg) ||
                        isGASPointer(&arg))
                    {
                        keepAlwaysInline = true;
                        break;
                    }
                }
            }

            if (!keepAlwaysInline)
            {
                F->removeFnAttr(llvm::Attribute::AlwaysInline);
            }
        }

        // Add Optnone to user functions but not on builtins. This allows to run
        // optimizations on builtins.
        if (getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.OptDisable)
        {
            if (!F->hasFnAttribute(llvm::Attribute::Builtin))
            {
                F->addFnAttr(llvm::Attribute::OptimizeNone);
            }
        }

        if (notKernel)
        {
            if (!keepAlwaysInline)
            {
                if (IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_SUBROUTINE ||
                    IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_STACKCALL)
                {
                    // add the following line in order to stress-test 
                    // subroutine call or stack call
                    F->removeFnAttr(llvm::Attribute::AlwaysInline);
                    F->addFnAttr(llvm::Attribute::NoInline);
                    if (IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_STACKCALL)
                    {
                        F->addFnAttr("visaStackCall");
                    }
                }
            }
        }
        Changed = true;
    }
    return Changed;
}

//
// ProcessBuiltinMetaData
//
namespace {

    class ProcessBuiltinMetaData : public ModulePass
    {
    public:
        static char ID;
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        ProcessBuiltinMetaData();

        ~ProcessBuiltinMetaData() {}

        virtual bool runOnModule(Module &M);

        virtual llvm::StringRef getPassName() const
        {
            return "ProcessBuiltinMetaData";
        }
    private:

        void updateBuiltinFunctionMetaData(llvm::Function*);

        MetaDataUtils *m_pMdUtil;
    };

} // namespace

// Register pass to igc-opt
#define PASS_FLAG2 "igc-process-builtin-metaData"
#define PASS_DESCRIPTION2 "Set builtin MetaData"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(ProcessBuiltinMetaData, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ProcessBuiltinMetaData, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char ProcessBuiltinMetaData::ID = 0;

ProcessBuiltinMetaData::ProcessBuiltinMetaData() : ModulePass(ID)
{
    initializeProcessBuiltinMetaDataPass(*PassRegistry::getPassRegistry());
}

ModulePass *createProcessBuiltinMetaDataPass()
{
    return new ProcessBuiltinMetaData();
}

bool ProcessBuiltinMetaData::runOnModule(Module& M)
{
    if (IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_INLINE)
    {
        return false;
    }

    m_pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool Changed = false;
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function *F = &(*I);
        if (!F || F->isDeclaration()) continue;

        // add AlwaysInline for all functions. It will be handle in optimization phase
        F->addFnAttr(llvm::Attribute::AlwaysInline);

        // disable JumpThread optimization on the block that contains this function
        F->setCannotDuplicate();

        if (m_pMdUtil->findFunctionsInfoItem(F) == m_pMdUtil->end_FunctionsInfo())
        {
            // It is user Function
            updateBuiltinFunctionMetaData(F);
        }
        Changed = true;
    }
    return Changed;
}

void ProcessBuiltinMetaData::updateBuiltinFunctionMetaData(llvm::Function* pFunc)
{
    IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(IGCMD::FunctionInfoMetaData::get());
    fHandle->setType(IGC::IGCMD::FunctionTypeEnum::OtherFunctionType);
    for (auto arg = pFunc->arg_begin(); arg != pFunc->arg_end(); ++arg)
    {
        std::string typeStr;
        llvm::raw_string_ostream x(typeStr);
        arg->getType()->print(x);

        fHandle->addOpenCLArgNamesItem(arg->getName());
        fHandle->addOpenCLArgAccessQualifiersItem("none");
        fHandle->addOpenCLArgBaseTypesItem(x.str());
    }
    m_pMdUtil->setFunctionsInfoItem(pFunc, fHandle);
}
