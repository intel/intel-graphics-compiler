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
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "SPIRV/SPIRVInternal.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Attributes.h"

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

// __builtin_spirv related OpGroup call implementations contain both
// workgroup and subgroup code in them that is switched on based on the
// 'Execution' and 'Operation' parameters and these will almost always
// be compile time literals.  Let's inline these functions so we have a chance
// at optimizing away the branches that contain workgroup code that will cause
// SLM allocations when we're really doing a subgroup calls.
static DenseSet<Function*> collectMemPoolUsage(const Module &M)
{
    const char *BUILTIN_MEMPOOL = "__builtin_IB_AllocLocalMemPool";
    auto *MemPool = M.getFunction(BUILTIN_MEMPOOL);

    DenseSet<Function*> FuncsToInline;

    if (!MemPool)
        return FuncsToInline;

    for (auto *U : MemPool->users())
    {
        if (auto *CI = dyn_cast<CallInst>(U))
        {
            FuncsToInline.insert(CI->getFunction());
        }
    }

    return FuncsToInline;
}

bool ProcessFuncAttributes::runOnModule(Module& M)
{
    MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
    MetaDataUtils *pMdUtils = mduw.getMetaDataUtils();
    ModuleMetaData *modMD = mduw.getModuleMetaData();
    auto MemPoolFuncs = collectMemPoolUsage(M);
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

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

    // lambda for setting indirect function attributes
    auto SetIndirectFuncAttributes = [this](Function* F)->void
    {
        F->addFnAttr("IndirectlyCalled");
        F->addFnAttr("visaStackCall");

        if (!F->isDeclaration())
        {
            // Require global relocation if any global values are used in indirect functions, since we cannot pass implicit args
            F->addFnAttr("EnableGlobalRelocation");

            if (F->hasFnAttribute("IFCALL_BUILTIN") ||
                IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_INDIRECTCALL)
            {
                F->removeFnAttr(llvm::Attribute::AlwaysInline);
                F->addFnAttr(llvm::Attribute::NoInline);
            }
        }
    };

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
        if (F->isDeclaration())
        {
            if (F->getName() == "__translate_sampler_initializer")
                F->addFnAttr(llvm::Attribute::ReadOnly);
            if (F->hasFnAttribute("referenced-indirectly"))
            {
                pCtx->m_enableFunctionPointer = true;
                SetIndirectFuncAttributes(F);
            }

            // It is not a defined function
            continue;
        }

        // If EnableOCLNoInlineAttr is on and F does have
        // NoInline, do not reset it.
        if (IGC_IS_FLAG_ENABLED(EnableOCLNoInlineAttr) &&
            pCtx->type == ShaderType::OPENCL_SHADER &&
            F->hasFnAttribute(llvm::Attribute::NoInline) &&
            !F->hasFnAttribute(llvm::Attribute::Builtin))
        {
            continue;
        }

        // Do not reset it for critical section builtins
        if (F->hasFnAttribute("KMPLOCK"))
        {
            continue;
        }

        // Remove noinline attr if present.
        F->removeFnAttr(llvm::Attribute::NoInline);

        if (IGC_IS_FLAG_ENABLED(DisableAddingAlwaysAttribute))
        {
            // Add always attribute if function has an argument with opaque type
            // Curently, ExtensionArgAnalysis assumes that all functions with image arguments to be inlined
            // This patch makes sure that we add always inline for such cases
            for (auto& arg : F->args())
            {
                if (containsOpaque(arg.getType()))
                {
                    F->addFnAttr(llvm::Attribute::AlwaysInline);
                    break;
                }
            }

            // Add always attribtue if function is a builtin
            if (F->hasFnAttribute(llvm::Attribute::Builtin))
            {
                F->addFnAttr(llvm::Attribute::AlwaysInline);
            }
        }
        else
        {
            // Add AlwaysInline attribute to force inlining all calls.
            F->addFnAttr(llvm::Attribute::AlwaysInline);
        }

        // Go through call sites and remove NoInline atrributes.
        for (auto I : F->users()) {
            if (CallInst* callInst = dyn_cast<CallInst>(&*I)) {
                if (callInst->hasFnAttr(llvm::Attribute::NoInline)) {
                    callInst->removeAttribute(IGCLLVM::AttributeSet::FunctionIndex, llvm::Attribute::NoInline);
                }
            }
        }

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
        bool keepAlwaysInline = (MemPoolFuncs.count(F) != 0);
        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            if (!keepAlwaysInline)
            {
                for (auto &arg : F->args())
                {
                    // If argument contains an opaque type e.g. image, then always inline it.
                    if (containsOpaque(arg.getType()))
                    {
                        keepAlwaysInline = true;
                        break;
                    }
                }

                // always inline spirv builtins
                if (F->getName().startswith(spv::kLLVMName::builtinPrefix))
                {
                    keepAlwaysInline = true;
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

        bool istrue = false;
        if (notKernel || istrue)
        {
            if (!keepAlwaysInline)
            {
                bool forceSubroutine = IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_SUBROUTINE;
                bool forceStackCall = IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_STACKCALL;

                if ((forceSubroutine || forceStackCall) && (istrue == false))
                {
                    // add the following line in order to stress-test
                    // subroutine call or stack call
                    F->removeFnAttr(llvm::Attribute::AlwaysInline);
                    F->addFnAttr(llvm::Attribute::NoInline);
                    if (forceStackCall)
                    {
                        F->addFnAttr("visaStackCall");
                    }
                }
            }

            if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer))
            {
                // Check if the function can be called either from
                // externally or as a function pointer
                bool isExtern = (F->hasFnAttribute("referenced-indirectly"));
                bool isIndirect = false;

                for (auto u = F->user_begin(), e = F->user_end(); u != e; u++)
                {
                    CallInst* call = dyn_cast<CallInst>(*u);

                    if (!call || call->getCalledValue() != F)
                    {
                        isIndirect = true;
                    }
                }

                if (isExtern || isIndirect)
                {
                    pCtx->m_enableFunctionPointer = true;
                    SetIndirectFuncAttributes(F);

                    if(istrue)
                        F->removeFnAttr("visaStackCall");

                    if (isExtern)
                    {
                        F->setLinkage(GlobalValue::ExternalLinkage);
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

        // add AlwaysInline for functions. It will be handle in optimization phase
        if (!F->hasFnAttribute(llvm::Attribute::NoInline))
            F->addFnAttr(llvm::Attribute::AlwaysInline);

        // disable JumpThread optimization on the block that contains this function
        F->setConvergent();

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
    IGC::ModuleMetaData* modMD = getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->getModuleMetaData();
    FunctionMetaData *funcMD = &modMD->FuncMD[pFunc]; //okay to insert if not present
    funcMD->functionType = IGC::FunctionTypeMD::UserFunction;
    fHandle->setType(FunctionTypeMD::UserFunction);
    for (auto arg = pFunc->arg_begin(); arg != pFunc->arg_end(); ++arg)
    {
        std::string typeStr;
        llvm::raw_string_ostream x(typeStr);
        arg->getType()->print(x);

        funcMD->m_OpenCLArgNames.push_back(arg->getName());
        funcMD->m_OpenCLArgAccessQualifiers.push_back("none");
        funcMD->m_OpenCLArgBaseTypes.push_back(x.str());
    }
    m_pMdUtil->setFunctionsInfoItem(pFunc, fHandle);
}
