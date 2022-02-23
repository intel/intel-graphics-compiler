/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ProcessFuncAttributes.h"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "SPIRV/SPIRVInternal.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvm/IR/Attributes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Regex.h>
#include <llvm/Analysis/CallGraph.h>
#include "llvm/ADT/SCCIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"
#include <fstream>
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
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<EstimateFunctionSize>();
        AU.addRequired<llvm::CallGraphWrapperPass>();
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
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(EstimateFunctionSize)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
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

// Check the existence of an image type.
static bool containsImageType(llvm::Type *T)
{
    // All (nested) struct types in T.
    SmallPtrSet<StructType *, 8> StructTys;
    getContainedStructType(T, StructTys);

    for (auto I = StructTys.begin(), E = StructTys.end(); I != E; ++I)
    {
        StructType *ST = *I;
        if (ST->isOpaque())
        {
            auto typeName = ST->getName();
            llvm::SmallVector<llvm::StringRef, 3> buf;
            typeName.split(buf, ".");
            if (buf.size() < 2) return false;
            bool isOpenCLImage = buf[0].equals("opencl") && buf[1].startswith("image") && buf[1].endswith("_t");
            bool isSPIRVImage = buf[0].equals("spirv") && buf[1].startswith("Image");

            if (isOpenCLImage || isSPIRVImage)
                return true;
        }
    }

    return false;
}

static bool isOptNoneBuiltin(StringRef name)
{
    return name == "__intel_typedmemfence_optnone" ||
           name == "__intel_memfence_optnone";
}

// Convert functions with recursion to stackcall, since subroutines do not support recursion
static bool convertRecursionToStackCall(CallGraph& CG)
{
    bool hasRecursion = false;
    // Use Tarjan's algorithm to detect recursions.
    for (auto I = scc_begin(&CG), E = scc_end(&CG); I != E; ++I)
    {
        const std::vector<CallGraphNode*>& SCCNodes = *I;
        if (SCCNodes.size() >= 2)
        {
            hasRecursion = true;
            // Convert all functions in the SCC to stackcall
            for (auto Node : SCCNodes)
            {
                Node->getFunction()->addFnAttr("visaStackCall");
                Node->getFunction()->addFnAttr("hasRecursion");
            }
        }
        else
        {
            // Check self-recursion.
            auto Node = SCCNodes.back();
            for (auto Callee : *Node)
            {
                if (Callee.second == Node)
                {
                    hasRecursion = true;
                    Node->getFunction()->addFnAttr("visaStackCall");
                    Node->getFunction()->addFnAttr("hasRecursion");
                    break;
                }
            }
        }
    }
    return hasRecursion;
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

void addFnAttrRecursive(Function* F, StringRef Attr, StringRef Val)
{
    F->addFnAttr(Attr, Val);
    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        if (CallInst* CI = dyn_cast<CallInst>(&*i)) {
            Function* Callee = CI->getCalledFunction();
            if (Callee != nullptr) {
                addFnAttrRecursive(Callee, Attr, Val);
            }
        }
    }
}

bool ProcessFuncAttributes::runOnModule(Module& M)
{
    MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
    MetaDataUtils *pMdUtils = mduw.getMetaDataUtils();
    ModuleMetaData *modMD = mduw.getModuleMetaData();
    auto MemPoolFuncs = collectMemPoolUsage(M);
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    EstimateFunctionSize& efs = getAnalysis<EstimateFunctionSize>();
    bool isOptDisable = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.OptDisable;
    auto FCtrl = IGC_GET_FLAG_VALUE(FunctionControl);

    std::set<llvm::Function *> fastMathFunct;
    GlobalVariable *gv_fastMath = M.getGlobalVariable("__FastRelaxedMath", true);
    if (gv_fastMath)
    {
        if (gv_fastMath->getInitializer()->isOneValue())
        {
            // Find the functions which __FastRelaxedMath belongs to....
            for (Value::user_iterator U = gv_fastMath->user_begin(), UE = gv_fastMath->user_end(); U != UE; ++U)
            {
                if (Instruction* user = dyn_cast<Instruction>(*U))
                    fastMathFunct.insert(user->getParent()->getParent());
            }
        }
    }

    auto SetNoInline = [](Function* F)->void
    {
        F->addFnAttr(llvm::Attribute::NoInline);
        F->removeFnAttr(llvm::Attribute::AlwaysInline);
    };
    auto SetAlwaysInline = [](Function* F)->void
    {
        F->addFnAttr(llvm::Attribute::AlwaysInline);
        F->removeFnAttr(llvm::Attribute::NoInline);
    };

    // Returns true if a function is either import or export and requires external linking
    auto NeedsLinking = [](Function* F)->bool
    {
        // SPIRV FE translate import/export linkage to "ExternalLinkage" in LLVMIR
        // Check all "ExternalLinkage" functions. Func declarations = Import, Func definition = Export
        if (F->hasExternalLinkage() && F->getCallingConv() == CallingConv::SPIR_FUNC)
        {
            // builtins should not be externally linked, they will always be resolved by IGC
            return !(F->hasFnAttribute(llvm::Attribute::Builtin)
                || F->getName().startswith("__builtin_")
                || F->getName().startswith("__igcbuiltin_")
                || F->getName().startswith("llvm.")
                || F->getName().equals("printf")
                || Regex("^_Z[0-9]+__spirv_").match(F->getName()));
        }
        return false;
    };

    // Returns true if a function is built-in double math function
    // Our implementations of double math built-in functions are precise only
    // if we don't make any fast relaxed math optimizations.
    auto IsBuiltinFP64 = [](Function* F)->bool
    {
        StringRef buildinPrefixOpenCL = igc_spv::kLLVMName::builtinExtInstPrefixOpenCL;
        if (F->getName().startswith(buildinPrefixOpenCL))
        {
            if (F->getReturnType()->isDoubleTy() ||
                (F->getReturnType()->isVectorTy() && F->getReturnType()->getContainedType(0)->isDoubleTy()))
            {
                auto functionName = F->getName();
                functionName = functionName.drop_front(buildinPrefixOpenCL.size());
                functionName = functionName.take_front(functionName.find('_'));

                static std::set<StringRef> mathFunctionNames = {
#define _OCL_EXT_OP(name, num) #name,
#include "SPIRV/libSPIRV/OpenCL.stdfuncs.h"
#undef _OCL_EXT_OP
                };

                if (mathFunctionNames.find(functionName) != mathFunctionNames.end()) {
                    return true;
                }
            }
        }
        return false;
    };

    // Process through all functions and add the appropriate function attributes
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* F = &(*I);
        if (F->isDeclaration())
        {
            if (F->getName() == "__translate_sampler_initializer")
                F->addFnAttr(llvm::Attribute::ReadOnly);

            // Functions requiring import from external module
            if (F->hasFnAttribute("referenced-indirectly") || NeedsLinking(F))
            {
                pCtx->m_enableFunctionPointer = true;
                F->addFnAttr("referenced-indirectly");
                F->addFnAttr("visaStackCall");
            }

            // It is not a defined function
            continue;
        }

        // Do not reset it for critical section builtins
        if (F->hasFnAttribute("KMPLOCK"))
        {
            continue;
        }
        // Do not reset attributes for SYCL unmasked functions.
        if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && F->hasFnAttribute("sycl-unmasked"))
        {
            continue;
        }
        // Always inline for non-compute
        if (pCtx->type != ShaderType::OPENCL_SHADER &&
            pCtx->type != ShaderType::RAYTRACING_SHADER &&
            pCtx->type != ShaderType::COMPUTE_SHADER)
        {
            SetAlwaysInline(F);
            continue;
        }
        // Set noinline on optnone user functions.
        if (!F->hasFnAttribute(llvm::Attribute::Builtin) &&
            F->hasFnAttribute(llvm::Attribute::OptimizeNone))
        {
            SetNoInline(F);
        }

        for (auto I : F->users()) {
            if (CallInst* callInst = dyn_cast<CallInst>(&*I)) {
                // Go through call sites and remove NoInline atrributes.
                // Verifier fails if a call has optnone but not noinline, so if we remove noinline, we must also remove optnone
                if (callInst->hasFnAttr(llvm::Attribute::NoInline)) {
                    callInst->removeAttribute(AttributeList::FunctionIndex, llvm::Attribute::NoInline);
                    callInst->removeAttribute(AttributeList::FunctionIndex, llvm::Attribute::OptimizeNone);
                }
                // Remove AlwaysInline at callsites
                if (isOptDisable && callInst->hasFnAttr(llvm::Attribute::AlwaysInline)) {
                    callInst->removeAttribute(AttributeList::FunctionIndex, llvm::Attribute::AlwaysInline);
                }
            }
        }

        // set function attributes according to build options so
        // inliner doesn't conservatively turn off unsafe optimizations
        // when inlining BIFs (see mergeAttributesForInlining() in inliner).
        const auto& opts = modMD->compOpt;
        if (opts.MadEnable)
            F->addFnAttr("less-precise-fpmad", "true");

        // Fast relaxed math implies all other flags.
        if (opts.UnsafeMathOptimizations || opts.FastRelaxedMath)
            F->addFnAttr("unsafe-fp-math", "true");

        // Finite math implies no infs and nans.
        if (opts.FiniteMathOnly || opts.FastRelaxedMath) {
            F->addFnAttr("no-infs-fp-math", "true");
            F->addFnAttr("no-nans-fp-math", "true");
        }

        // Unsafe math implies no signed zeros.
        if (opts.NoSignedZeros || opts.UnsafeMathOptimizations || opts.FastRelaxedMath) {
            F->addFnAttr("no-signed-zeros-fp-math", "true");
        }

        // Add Optnone to user functions but not on builtins. This allows to run
        // optimizations on builtins.
        if (isOptDisable)
        {
            if (!F->hasFnAttribute(llvm::Attribute::Builtin))
            {
                F->addFnAttr(llvm::Attribute::OptimizeNone);
            }
        }

        bool istrue = false;

        auto funcIt = modMD->FuncMD.find(F);
        if (funcIt != modMD->FuncMD.end())
        {
            istrue = IGC::isContinuation(funcIt->second);
        }

        // set hasVLA function attribute
        {
            bool isSet = false;
            for (auto& BB : F->getBasicBlockList()) {
                for (auto& Inst : BB.getInstList()) {
                    if (AllocaInst* AI = dyn_cast<AllocaInst>(&Inst)) {
                        if (!isa<ConstantInt>(AI->getArraySize())) {
                            F->addFnAttr("hasVLA");
                            isSet = true;
                            break;
                        }
                    }
                }
                if (isSet)
                    break;
            }
        }

        // Set for kernel functions
        const bool isKernel = isEntryFunc(pMdUtils, F);

        // Functions that have the spir_kernel calling convention
        // This may be true even if isEntryFunc returns false, for invoke kernels and cloned callable kernels
        if (!isKernel && !istrue && (F->getCallingConv() == CallingConv::SPIR_KERNEL))
        {
            // WA for callable kernels, always inline these.
            SetAlwaysInline(F);
            continue;
        }

        // Check for functions that can be indirectly called
        bool isIndirect = false;
        if (!isKernel || istrue)
        {
            isIndirect = F->hasFnAttribute("referenced-indirectly") ||
                (getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.IsLibraryCompilation && NeedsLinking(F));

            if (!isIndirect)
            {
                // Functions without Export Linkage and does not have the indirect attribute set can still be called indirectly.
                // Set the indirect flag if the function's address is taken by a non-call instruction.
                for (auto u = F->user_begin(), e = F->user_end(); u != e; u++)
                {
                    CallInst* call = dyn_cast<CallInst>(*u);
                    if (!call || IGCLLVM::getCalledValue(call) != F)
                    {
                        isIndirect = true;
                        break;
                    }
                }
            }
        }

        if (isIndirect)
        {
            // Add indirect call function attributes
            pCtx->m_enableFunctionPointer = true;
            F->addFnAttr("referenced-indirectly");
            if (!istrue)
            {
                F->addFnAttr("visaStackCall");
            }
            F->setLinkage(GlobalValue::ExternalLinkage);
        }

        if (isKernel)
        {
            // No need to process further for kernels
            continue;
        }
        else if (!isIndirect)
        {
            // Set internal linkage for remaining non-kernel functions
            F->setLinkage(GlobalValue::InternalLinkage);
        }

        // Flag for function calls where alwaysinline must be true
        bool mustAlwaysInline = false;

        // Add always attribute if function is a builtin
        if (F->hasFnAttribute(llvm::Attribute::Builtin) ||
            F->getName().startswith(igc_spv::kLLVMName::builtinPrefix))
        {
            // OptNone builtins are special versions of builtins assuring that all
            // theirs parameters are constant values.
            if (isOptNoneBuiltin(F->getName()))
            {
                // OptimizeNone attribute was only required to prevent clang optimizations.
                // We can remove it now to unblock IGC optimizations.
                F->removeFnAttr(llvm::Attribute::OptimizeNone);
                // Remove the noinline attribute to allow IGC inlining heuristic to determine inlining
                F->removeFnAttr(llvm::Attribute::NoInline);
            }
            else
            {
                mustAlwaysInline = true;
            }
        }
        // inline all OCL math functions if __FastRelaxedMath is set
        else if (fastMathFunct.find(F) != fastMathFunct.end())
        {
            mustAlwaysInline = true;
        }
        else
        {
            // Curently, ExtensionArgAnalysis assumes that all functions with image arguments
            // to be inlined. We add always inline for such cases.
            for (auto& arg : F->args())
            {
                if (containsImageType(arg.getType()))
                {
                    mustAlwaysInline = true;
                    break;
                }
            }
        }
        // WA for scheduler kernel, must inline all calls otherwise we cannot prevent spilling
        if (pCtx->type == ShaderType::OPENCL_SHADER)
        {
            auto ClContext = static_cast<OpenCLProgramContext*>(pCtx);
            if (ClContext->m_InternalOptions.NoSpill)
            {
                mustAlwaysInline = true;
            }
        }
        if (mustAlwaysInline)
        {
            SetAlwaysInline(F);
            continue;
        }

        // Fixme: Can we support user noinline attrib from non-OCL shader?
        if (pCtx->type != ShaderType::OPENCL_SHADER)
        {
            F->removeFnAttr(llvm::Attribute::NoInline);
        }

        // Set default inline mode
        if (FCtrl == FLAG_FCALL_DEFAULT)
        {
            // Set default function call mode to stack call
            if (IGC_IS_FLAG_ENABLED(EnableStackCallFuncCall))
            {
                F->addFnAttr("visaStackCall");
            }
            // default stackcall for -O0, or when FE force stackcall using attribute
            if (isOptDisable || F->hasFnAttribute("igc-force-stackcall"))
            {
                F->addFnAttr("visaStackCall");
                SetNoInline(F);
            }

            if (!F->hasFnAttribute(llvm::Attribute::NoInline) &&
                IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute))
            {
                bool shouldAlwaysInline = (MemPoolFuncs.count(F) != 0);

                if (!shouldAlwaysInline)
                {
                    for (auto& arg : F->args())
                    {
                        // If argument is a pointer to GAS or aggregate type, always inline it for perf reasons
                        if (isSupportedAggregateArgument(&arg) || isGASPointer(&arg))
                        {
                            shouldAlwaysInline = true;
                            break;
                        }
                    }
                }
                if (shouldAlwaysInline)
                {
                    if (IGC_IS_FLAG_ENABLED(ControlKernelTotalSize) &&
                        efs.shouldEnableSubroutine() &&
                        efs.isTrimmedFunction(F))
                    {
                        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 )
                        {
                            std::cout << "Trimmed function " << F->getName().str() << std::endl;
                        }

                        if (IGC_IS_FLAG_ENABLED(AddNoInlineToTrimmedFunctions))
                        {
                            SetNoInline(F);
                        }
                    }
                    else
                    {
                        SetAlwaysInline(F);
                    }
                }
            }
        }
        else if (FCtrl == FLAG_FCALL_FORCE_INLINE)
        {
            // Forced inlining all functions
            SetAlwaysInline(F);
        }
        else
        {
            // Forcing subroutines/stack-call/indirect-call
            bool forceSubroutine = FCtrl == FLAG_FCALL_FORCE_SUBROUTINE;
            bool forceStackCall = FCtrl == FLAG_FCALL_FORCE_STACKCALL;
            bool forceIndirectCall = (FCtrl == FLAG_FCALL_FORCE_INDIRECTCALL || F->hasFnAttribute("IFCALL_BUILTIN"));

            if (forceSubroutine || forceStackCall || forceIndirectCall)
            {
                SetNoInline(F);
                if (forceStackCall)
                {
                    F->addFnAttr("visaStackCall");
                }
                else if (forceIndirectCall)
                {
                    pCtx->m_enableFunctionPointer = true;
                    F->addFnAttr("referenced-indirectly");
                    F->addFnAttr("visaStackCall");
                    F->setLinkage(GlobalValue::ExternalLinkage);
                }
            }
        }
    }

    // This selectively sets the FunctionControl mode for the list of line-separated
    // functions existing in 'FunctionDebug.txt' in the default IGC output folder.
    // This flag will override the default FunctionControl setting for these functions.
    //
    // For example:
    // We can set FunctionControl=1 and SelectiveFuncionControl=3, such that all functions
    // in the module are inlined, except those found in the 'FunctionDebug.txt' file, which
    // are stack-called instead.
    //
    auto SelectFCtrl = IGC_GET_FLAG_VALUE(SelectiveFunctionControl);
    if (SelectFCtrl != FLAG_FCALL_DEFAULT)
    {
        std::ifstream inputFile(IGC::Debug::GetFunctionDebugFile());
        if (inputFile.is_open())
        {
            std::string line;
            while (std::getline(inputFile, line))
            {
                if (Function* F = M.getFunction(line))
                {
                    if (SelectFCtrl == FLAG_FCALL_FORCE_INLINE)
                    {
                        F->removeFnAttr("referenced-indirectly");
                        F->removeFnAttr("visaStackCall");
                        SetAlwaysInline(F);
                    }
                    else if (SelectFCtrl == FLAG_FCALL_FORCE_SUBROUTINE)
                    {
                        F->removeFnAttr("referenced-indirectly");
                        F->removeFnAttr("visaStackCall");
                        SetNoInline(F);
                    }
                    else if (SelectFCtrl == FLAG_FCALL_FORCE_STACKCALL)
                    {
                        F->removeFnAttr("referenced-indirectly");
                        F->addFnAttr("visaStackCall");
                        SetNoInline(F);
                    }
                    else if (SelectFCtrl == FLAG_FCALL_FORCE_INDIRECTCALL)
                    {
                        pCtx->m_enableFunctionPointer = true;
                        F->addFnAttr("referenced-indirectly");
                        F->addFnAttr("visaStackCall");
                        F->setLinkage(GlobalValue::ExternalLinkage);
                        SetNoInline(F);
                    }
                }
            }
            inputFile.close();
        }
    }

    // Process through all functions and reset the *-fp-math attributes
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
        Function* F = &(*I);
        if (!F->isDeclaration()) {
            if (IsBuiltinFP64(F)) {
                addFnAttrRecursive(F, "unsafe-fp-math", "false");
                addFnAttrRecursive(F, "no-infs-fp-math", "false");
                addFnAttrRecursive(F, "no-nans-fp-math", "false");
                addFnAttrRecursive(F, "no-signed-zeros-fp-math", "false");
            }
        }
    }

    // Detect recursive calls, and convert them to stack calls, since subroutines does not support recursion
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    if (convertRecursionToStackCall(CG))
    {
        if (IGC_IS_FLAG_DISABLED(EnableRecursionOpenCL) &&
            !pCtx->m_DriverInfo.AllowRecursion())
        {
            IGC_ASSERT_MESSAGE(0, "Recursion detected but not enabled!");
        }
        if (IGC::ForceAlwaysInline())
        {
            IGC_ASSERT_MESSAGE(0, "Cannot have recursion when forcing inline!");
        }
    }

    // Enable subroutines flag based on function attributes
    pCtx->CheckEnableSubroutine(M);

    return true;
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
    if (IGC::ForceAlwaysInline())
    {
        return false;
    }

    m_pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool Changed = false;
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* F = &(*I);
        if (!F || F->isDeclaration()) continue;

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

        funcMD->m_OpenCLArgNames.push_back(arg->getName().str());
        funcMD->m_OpenCLArgAccessQualifiers.push_back("none");
        funcMD->m_OpenCLArgBaseTypes.push_back(x.str());
    }
    m_pMdUtil->setFunctionsInfoItem(pFunc, fHandle);
}


//
// InsertDummyKernelForSymbolTable
//
namespace {

    class InsertDummyKernelForSymbolTable : public ModulePass
    {
    public:
        static char ID;
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        InsertDummyKernelForSymbolTable();

        ~InsertDummyKernelForSymbolTable() {}

        virtual bool runOnModule(Module& M);

        virtual llvm::StringRef getPassName() const
        {
            return "InsertDummyKernelForSymbolTable";
        }
    private:
    };

} // namespace

// Register pass to igc-opt
#define PASS_FLAG3 "igc-insert-dummy-kernel-for-symbol-table"
#define PASS_DESCRIPTION3 "If symbol table is needed, insert a dummy kernel to attach it to"
#define PASS_CFG_ONLY3 false
#define PASS_ANALYSIS3 false
IGC_INITIALIZE_PASS_BEGIN(InsertDummyKernelForSymbolTable, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(InsertDummyKernelForSymbolTable, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)

char InsertDummyKernelForSymbolTable::ID = 0;

InsertDummyKernelForSymbolTable::InsertDummyKernelForSymbolTable() : ModulePass(ID)
{
    initializeInsertDummyKernelForSymbolTablePass(*PassRegistry::getPassRegistry());
}

ModulePass* createInsertDummyKernelForSymbolTablePass()
{
    return new InsertDummyKernelForSymbolTable();
}

// The code is based on the assumption that a kernel that takes an address
// of an indirectly called function has to match in SIMD size with the kernel
// invoking the function and the indirectly called function itself. Thus try
// to propagate simd size from the caller. As of now assert if the function's
// users have different simd sizes.
inline void checkKernelSimdSize(Function* F, FunctionInfoMetaDataHandle funcInfoMD, MetaDataUtils* pMdUtils)
{
    Function* caller = nullptr;
    int simd_size = 0;
    for (auto U : F->users())
    {
        if (Instruction* I = dyn_cast<Instruction>(U))
        {
            // Get a kernel and its simd size
            caller = I->getParent()->getParent();
            FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(caller);
            int sz = funcInfoMD->getSubGroupSize()->getSIMD_size();
            if (sz != 0 && simd_size == 0)
                simd_size = sz;
            // Assert now if sizes don't match.
            // TODO. Extend this code to support indirect function call with
            // different simd sizes
            IGC_ASSERT_MESSAGE(simd_size == sz, "Function is called with different sub group size");
        }
    }
    if (simd_size != 0)
    {
        IGC_ASSERT_MESSAGE((simd_size == 8) || (simd_size == 16) || (simd_size == 32),
            "Kernel has incorrect SIMD size");
        // Now propagate the computed simd size to the default kernel, which was
        // created by the compiler, and to which all indirectly called functions
        // are attached.
        IGCMD::SubGroupSizeMetaDataHandle sgHandle = funcInfoMD->getSubGroupSize();
        if (sgHandle->getSIMD_size() == 0)
        {
            // The kernel still has no info set about simd size, thus set it.
            sgHandle->setSIMD_size(simd_size);
        }
        else if (sgHandle->getSIMD_size() != simd_size)
        {
            // TODO. A placeholder for a code to mark the kernel with info
            // that there should be code generation with multiple simd sizes.
            // Until there is a mechanism to support this, issue an assert here.
            IGC_ASSERT_MESSAGE(false, "INTEL_SYMBOL_TABLE_VOID_PROGRAM requires variant SIMD sizes");
        }
    }
}

bool InsertDummyKernelForSymbolTable::runOnModule(Module& M)
{
    MetaDataUtilsWrapper& mduw = getAnalysis<MetaDataUtilsWrapper>();
    MetaDataUtils* pMdUtils = mduw.getMetaDataUtils();
    ModuleMetaData* modMD = mduw.getModuleMetaData();
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool needDummyKernel = false;

    // Check when we need to generate a dummy kernel. This is only useful for attaching
    // the symbol table to its program output for indirect calls and global variable relocation.
    if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer) && pCtx->type == ShaderType::OPENCL_SHADER)
    {
        if (pCtx->m_enableFunctionPointer) {
            // Symbols are needed for external functions and function pointers
            needDummyKernel = true;
        }
        else if (!modMD->inlineProgramScopeOffsets.empty()) {
            // Create one also if global variables are present and require symbols
            needDummyKernel = true;
        }
        else if (IGC_GET_FLAG_VALUE(FunctionCloningThreshold) > 0 && pCtx->enableFunctionCall()) {
            // If this flag is enabled and there are any function calls, conservatively create a dummy kernel
            // in case we need to transform normal calls into indirect calls to avoid cloning in GenCodeGenModule.cpp
            needDummyKernel = true;
        }
    }

    if (needDummyKernel)
    {
        // Create empty kernel function
        IGC_ASSERT(IGC::getIntelSymbolTableVoidProgram(&M) == nullptr);
        Type* voidTy = Type::getVoidTy(M.getContext());
        FunctionType* fTy = FunctionType::get(voidTy, false);
        Function* pNewFunc = Function::Create(fTy, GlobalValue::ExternalLinkage, IGC::INTEL_SYMBOL_TABLE_VOID_PROGRAM, &M);
        BasicBlock* entry = BasicBlock::Create(M.getContext(), "entry", pNewFunc);
        IRBuilder<> builder(entry);
        builder.CreateRetVoid();

        // Set spirv calling convention and kernel metadata
        pNewFunc->setCallingConv(llvm::CallingConv::SPIR_KERNEL);
        IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(IGCMD::FunctionInfoMetaData::get());
        FunctionMetaData* funcMD = &modMD->FuncMD[pNewFunc];
        funcMD->functionType = IGC::FunctionTypeMD::KernelFunction;
        fHandle->setType(FunctionTypeMD::KernelFunction);

        // Promote SIMD size information from kernels, which has indirectly called
        // functions. All such functions will be connected to the default kernel in
        // GenCodeGenModule.cpp
        for (auto I = M.begin(), E = M.end(); I != E; ++I)
        {
            Function* F = &(*I);
            if (F->isDeclaration() || isEntryFunc(pMdUtils, F)) continue;
            if (F->hasFnAttribute("referenced-indirectly"))
            {
                checkKernelSimdSize(F, fHandle, pMdUtils);
            }
        }
        pMdUtils->setFunctionsInfoItem(pNewFunc, fHandle);
        pMdUtils->save(M.getContext());

        return true;
    }
    return false;
}
