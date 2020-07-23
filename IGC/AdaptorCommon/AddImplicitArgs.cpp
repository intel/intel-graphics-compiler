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

#include "AddImplicitArgs.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SCCIterator.h"
#include <llvm/IR/Module.h>
#include <llvmWrapper/IR/Function.h>
#include <llvmWrapper/ADT/STLExtras.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/DebugInfo/VISADebugEmitter.hpp"
#include "common/debug/Debug.hpp"
#include <map>
#include <utility>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-add-implicit-args"
#define PASS_DESCRIPTION "Add implicit args to all functions in the module and adjusts call to these functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AddImplicitArgs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(AddImplicitArgs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char AddImplicitArgs::ID = 0;

AddImplicitArgs::AddImplicitArgs() : ModulePass(ID)
{
    initializeAddImplicitArgsPass(*PassRegistry::getPassRegistry());
}

bool AddImplicitArgs::runOnModule(Module &M)
{
    MapList<Function*, Function*> funcsMapping;
    MapList<Function*, Function*> funcsMappingForReplacement;
    m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // Update function signatures
    // Create new functions with implicit args
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        // Only handle functions defined in this module
        if (pFunc->isDeclaration()) continue;
        // skip non-entry functions
        if (m_pMdUtils->findFunctionsInfoItem(pFunc) == m_pMdUtils->end_FunctionsInfo()) continue;
        // Skip functions called from function marked with IndirectlyCalled attribute
        // TODO: This function verifies only parent caller, in the future we may need to
        //       also verify all the callers up to the tree.
        if (hasIndirectlyCalledParent(pFunc)) continue;
        // No implicit arg support for stack calls
        if (pFunc->hasFnAttribute("visaStackCall")) continue;

        // see the detail in StatelessToStatefull.cpp.
        // If SToSProducesPositivePointer is true, do not generate implicit arguments.
        if (IGC_IS_FLAG_DISABLED(SToSProducesPositivePointer) &&
            (ctx->getModuleMetaData()->compOpt.HasBufferOffsetArg ||
             IGC_IS_FLAG_ENABLED(EnableSupportBufferOffset)))
        {
            ImplicitArgs::addBufferOffsetArgs(*pFunc, m_pMdUtils, ctx->getModuleMetaData());
        }

        ImplicitArgs implicitArgs(*pFunc, m_pMdUtils);

        // Create the new function body and insert it into the module
        FunctionType *pNewFTy = getNewFuncType(pFunc, &implicitArgs);
        Function* pNewFunc = Function::Create(pNewFTy, pFunc->getLinkage());
        pNewFunc->copyAttributesFrom(pFunc);
        pNewFunc->setSubprogram(pFunc->getSubprogram());
        M.getFunctionList().insert(pFunc->getIterator(), pNewFunc);
        pNewFunc->takeName(pFunc);

        // Since we have now created the new function, splice the body of the old
        // function right into the new function, leaving the old body of the function empty.
        pNewFunc->getBasicBlockList().splice(pNewFunc->begin(), pFunc->getBasicBlockList());

        // Loop over the argument list, transferring uses of the old arguments over to
        // the new arguments
        updateNewFuncArgs(pFunc, pNewFunc, &implicitArgs);

        // Map old func to new func
        funcsMapping[pFunc] = pNewFunc;

        if (!pFunc->use_empty())
        {
            funcsMappingForReplacement[pFunc] = pNewFunc;
        }
    }

    if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
    {
        for (auto I : funcsMappingForReplacement)
        {
            replaceAllUsesWithNewOCLBuiltinFunction(ctx, I.first, I.second);
        }
    }

    // Update IGC Metadata
    // Function declarations are changing, this needs to be reflected in the metadata.
    MetadataBuilder mbuilder(&M);
    auto &FuncMD = ctx->getModuleMetaData()->FuncMD;
    for (auto i : funcsMapping)
    {
        auto oldFuncIter = m_pMdUtils->findFunctionsInfoItem(i.first);
        m_pMdUtils->setFunctionsInfoItem(i.second, oldFuncIter->second);
        m_pMdUtils->eraseFunctionsInfoItem(oldFuncIter);
        mbuilder.UpdateShadingRate(i.first, i.second);
        auto loc = FuncMD.find(i.first);
        if (loc != FuncMD.end())
        {
            auto funcInfo = loc->second;
            FuncMD.erase(i.first);
            FuncMD[i.second] = funcInfo;
        }
    }

    // Update LLVM metadata based on IGC MetadataUtils
    m_pMdUtils->save(M.getContext());

    //Return if any error
    if (!(getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->oclErrorMessage.empty()))
    {
        return false;
    }
    // Go over all changed functions
    for (MapList<Function*, Function*>::const_iterator I = funcsMapping.begin(), E = funcsMapping.end(); I != E; ++I)
    {
        Function* pFunc = I->first;

        IGC_ASSERT(nullptr != pFunc);
        IGC_ASSERT_MESSAGE(pFunc->use_empty(), "Assume all user function are inlined at this point");

        // Now, after changing funciton signature,
        // and validate there are no calls to the old function we can erase it.
        pFunc->eraseFromParent();
    }

    return true;
}

bool AddImplicitArgs::hasIndirectlyCalledParent(Function* F)
{
    for (auto u = F->user_begin(), e = F->user_end(); u != e; u++)
    {
        if (CallInst* call = dyn_cast<CallInst>(*u))
        {
            Function* parent = call->getParent()->getParent();
            if (parent->hasFnAttribute("IndirectlyCalled"))
                return true;
        }
    }
    return false;
}

FunctionType* AddImplicitArgs::getNewFuncType(Function* pFunc, const ImplicitArgs* pImplicitArgs)
{
    // Add all explicit parameters
    FunctionType* pFuncType = pFunc->getFunctionType();
    std::vector<Type *> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());

    // Add implicit arguments parameter types
    for(unsigned int i = 0; i < pImplicitArgs->size(); ++i)
    {
        newParamTypes.push_back((*pImplicitArgs)[i].getLLVMType(pFunc->getContext()));
    }

    // Create new function type with explicit and implicit parameter types
    return FunctionType::get( pFunc->getReturnType(),newParamTypes, pFunc->isVarArg());
}

void AddImplicitArgs::updateNewFuncArgs(llvm::Function* pFunc, llvm::Function* pNewFunc, const ImplicitArgs* pImplicitArgs)
{
    // Loop over the argument list, transferring uses of the old arguments over to
    // the new arguments, also transferring over the names as well.
    Function::arg_iterator currArg = pNewFunc->arg_begin();
    std::map<void*, unsigned int> argMap;
    std::vector<std::pair<llvm::Instruction*, unsigned int>> newAddr;
    bool fullDebugInfo = false;
    IF_DEBUG_INFO(bool lineNumbersOnly = false;)
    IF_DEBUG_INFO(CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();)
    IF_DEBUG_INFO(DebugMetadataInfo::hasAnyDebugInfo(ctx, fullDebugInfo, lineNumbersOnly);)

    if (fullDebugInfo)
    {
        unsigned int i = 0;

        // Create a map storing function arguments of pFunc with their position
        // of occurrence.
        for (auto arg = pFunc->arg_begin(); arg != pFunc->arg_end(); ++arg, ++i)
        {
            argMap.insert(std::make_pair(&(*arg), i));
        }

        // Iterate over each dbg.declare intrinsic call. If the address operand
        // matches with any argument from old function, pFunc, store it in a
        // data structure so we can fix it later.
        for (auto bb = pNewFunc->begin(); bb != pNewFunc->end(); ++bb)
        {
            for (auto inst = bb->begin(); inst != bb->end();)
            {
                auto DIInst = dyn_cast_or_null<DbgDeclareInst>(&(*inst));
                if (DIInst)
                {
                    {
                        auto addr = dyn_cast_or_null<Value>(DIInst->getAddress());
                        if (addr)
                        {
                            if (argMap.find(addr) != argMap.end())
                            {
                                newAddr.push_back(std::make_pair(DIInst, argMap.find(addr)->second));
                            }
                        }
                    }
                }
                ++inst;
            }
        }
    }

    for (Function::arg_iterator I = pFunc->arg_begin(), E = pFunc->arg_end(); I != E; ++I, ++currArg)
    {
        llvm::Value* newArg = &(*currArg);
        if ((*I).getType() != currArg->getType())
        {
            // fix opague type mismatch on %opencl.image...
            std::string str0;
            llvm::raw_string_ostream s(str0);
            currArg->getType()->print(s);

            BasicBlock &entry = pNewFunc->getEntryBlock();
            newArg = new llvm::BitCastInst(&(*currArg), (*I).getType(), "", &entry.front());
        }
        // Move the name and users over to the new version.
        I->replaceAllUsesWith(newArg);
        currArg->takeName(&(*I));
    }

    // In following loop, fix dbg.declare nodes that reference function arguments.
    // This occurs for example when a struct type is passed as a kernel parameter
    // byval. Bug#GD-429 had this exact issue. If we dont do this then we lose
    // mapping of argument to dbg.declare and elf file comes up with empty
    // storage location for the variable.
    for (auto toReplace : newAddr)
    {
        unsigned int i = 0;
        for (auto pNewFuncArg = pNewFunc->arg_begin(); pNewFuncArg != pNewFunc->arg_end(); ++pNewFuncArg, ++i)
        {
            if (i != toReplace.second)
                continue;

            IF_DEBUG_INFO(auto d = dyn_cast<DbgDeclareInst>(toReplace.first);)

            llvm::DIBuilder Builder(*pNewFunc->getParent());
            IF_DEBUG_INFO(auto DIVar = d->getVariable();)
            IF_DEBUG_INFO(auto DIExpr = d->getExpression();)

            Value* v = dyn_cast_or_null<Value>(&(*pNewFuncArg));
            if (v)
            {
                IF_DEBUG_INFO(Builder.insertDeclare(v, DIVar, DIExpr, d->getDebugLoc().get(), d);)
                IF_DEBUG_INFO(auto oldInst = d;)
                IF_DEBUG_INFO(oldInst->eraseFromParent();)
            }

            break;
        }
    }

    // Set implict argument names
    InfoToImpArgMap &infoToArg = m_FuncInfoToImpArgMap[pNewFunc];
    ImpArgToExpNum &argImpToExpNum = m_FuncImpToExpNumMap[pNewFunc];
    for (unsigned int i = 0; i < pImplicitArgs->size(); ++i, ++currArg)
    {
        ImplicitStructArgument info;

        currArg->setName((*pImplicitArgs)[i].getName());

        ImplicitArg::ArgType argId = (*pImplicitArgs)[i].getArgType();
        info.DW0.All.argId = argId;
        info.DW0.All.argExplictNum = 0;
        info.DW0.All.argOffset = 0;
        if (argId < ImplicitArg::ArgType::STRUCT_START)
        {
            infoToArg[info.DW0.Value] = &(*currArg);
        }
        else if (argId <= ImplicitArg::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE)
        {
            // struct, image
            FunctionInfoMetaDataHandle funcInfo = m_pMdUtils->getFunctionsInfoItem(pFunc);
            ArgInfoMetaDataHandle argInfo = funcInfo->getImplicitArgInfoListItem(i);
            IGC_ASSERT_MESSAGE(argInfo->isExplicitArgNumHasValue(), "wrong data in MetaData");

            info.DW0.All.argExplictNum = argInfo->getExplicitArgNum();
            if (argId <= ImplicitArg::ArgType::STRUCT_END)
            {
                IGC_ASSERT_MESSAGE(argInfo->isStructArgOffsetHasValue(), "wrong data in MetaData");
                info.DW0.All.argOffset = argInfo->getStructArgOffset();
            }
            infoToArg[info.DW0.Value] = &(*currArg);
            argImpToExpNum[&(*currArg)] = info.DW0.All.argExplictNum;
        }
        else
        {
            infoToArg[info.DW0.Value] = &(*currArg);
        }
    }
}

void AddImplicitArgs::replaceAllUsesWithNewOCLBuiltinFunction(CodeGenContext* ctx, llvm::Function* old_func, llvm::Function* new_func)
{
    IGC_ASSERT(!old_func->use_empty());

    FunctionInfoMetaDataHandle subFuncInfo = m_pMdUtils->getFunctionsInfoItem(old_func);

    std::vector<Instruction*> list_delete;
    old_func->removeDeadConstantUsers();
    std::vector<Value*> functionUserList(old_func->user_begin(), old_func->user_end());
    for (auto U : functionUserList)
    {
        CallInst *cInst = dyn_cast<CallInst>(U);
        auto BC = dyn_cast<BitCastInst>(U);
        if (BC && BC->hasOneUse())
            cInst = dyn_cast<CallInst>(BC->user_back());

        if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer))
        {
            if (!cInst || cInst->getCalledValue() != old_func)
            {
                // Support indirect function pointer usages
                if (Instruction* userInst = dyn_cast<Instruction>(U))
                {
                    IRBuilder<> builder(userInst);
                    Value* fncast = builder.CreateBitCast(new_func, old_func->getType());
                    userInst->replaceUsesOfWith(old_func, fncast);
                    continue;
                } else if (ConstantExpr *OldExpr = dyn_cast<ConstantExpr>(U)) {
                    Constant* fncast = ConstantExpr::getBitCast(new_func, old_func->getType());
                    SmallVector<Constant*, 8> NewOps;
                    for (auto Op : OldExpr->operand_values()) {
                        (Op == old_func) ?
                            NewOps.push_back(fncast) :
                            NewOps.push_back(cast<Constant>(Op));
                    }
                    auto NewExpr = OldExpr->getWithOperands(NewOps);
                    OldExpr->replaceAllUsesWith(NewExpr);
                    OldExpr->destroyConstant();
                    continue;
                }
            }
        }

        if (!cInst)
        {
            IGC_ASSERT_MESSAGE(0, "Unknown function usage");
            getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(" undefined reference to `jmp()' ");
            return;
        }
        //Return if any error
        if (!(getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->oclErrorMessage.empty()))
        {
            return;
        }

        std::vector<Value*> new_args;
        Function *parent_func = cInst->getParent()->getParent();
        size_t numArgOperands = cInst->getNumArgOperands();

        // let 's prepare argument list on new call function
        llvm::Function::arg_iterator new_arg_iter = new_func->arg_begin();
        llvm::Function::arg_iterator new_arg_end = new_func->arg_end();

        IGC_ASSERT(IGCLLVM::GetFuncArgSize(new_func) >= numArgOperands);

        // basic arguments
        for (unsigned int i = 0; i < numArgOperands; ++i, ++new_arg_iter)
        {
            llvm::Value* arg = cInst->getOperand(i);
            if (arg->getType() != new_arg_iter->getType())
            {
                // fix opague type mismatch on %opencl...
                std::string str0;
                llvm::raw_string_ostream s(str0);
                arg->getType()->print(s);

                arg = new llvm::BitCastInst(arg, new_arg_iter->getType(), "", cInst);
            }
            new_args.push_back(arg);
        }

        // implicit arguments
        int cImpCount = 0;
        InfoToImpArgMap &infoToArg = m_FuncInfoToImpArgMap[parent_func];
        ImpArgToExpNum &argImpToExpNum = m_FuncImpToExpNumMap[new_func];
        while (new_arg_iter != new_arg_end)
        {
            ArgInfoMetaDataHandle argInfo = subFuncInfo->getImplicitArgInfoListItem(cImpCount);
            ImplicitArg::ArgType argId = (ImplicitArg::ArgType)argInfo->getArgId();

            ImplicitStructArgument info;
            info.DW0.All.argId = argId;
            info.DW0.All.argExplictNum = 0;
            info.DW0.All.argOffset = 0;

            if (argId < ImplicitArg::ArgType::STRUCT_START)
            {
                IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(), "Can't find the implicit argument on parent function");
                new_args.push_back(infoToArg[info.DW0.Value]);
            }
            else if (argId <= ImplicitArg::ArgType::STRUCT_END)
            {
                IGC_ASSERT_MESSAGE(0, "wrong argument type in user function");
            }
            else if (argId <= ImplicitArg::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE)
            {
                // special handling for image info types, such as ImageWidth, ImageHeight, ...
                // and struct type

                IGC_ASSERT_MESSAGE(argImpToExpNum.find(&(*new_arg_iter)) != argImpToExpNum.end(), "Can't find explicit argument number");

                // tracing it on parent function argument list
                Value* callArg = CImagesBI::CImagesUtils::traceImageOrSamplerArgument(cInst, argImpToExpNum[&(*new_arg_iter)]);
                Argument* arg = dyn_cast<Argument>(callArg);

                IGC_ASSERT_MESSAGE(arg, "Not supported");

                // build info

                info.DW0.All.argExplictNum = arg->getArgNo();
                IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(), "Can't find the implicit argument on parent function");

                new_args.push_back(infoToArg[info.DW0.Value]);
            }
            else
            {
                IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(), "Can't find the implicit argument on parent function");
                new_args.push_back(infoToArg[info.DW0.Value]);
            }
            ++new_arg_iter;
            ++cImpCount;
        }

        // insert new call instruction before old one
        llvm::CallInst *inst;
        if (new_func->getReturnType()->isVoidTy())
        {
            inst = CallInst::Create(new_func, new_args, "", cInst);
        }
        else
        {
            inst = CallInst::Create(new_func, new_args, new_func->getName(), cInst);
        }
        inst->setCallingConv(new_func->getCallingConv());
        inst->setDebugLoc(cInst->getDebugLoc());
        cInst->replaceAllUsesWith(inst);
        list_delete.push_back(cInst);
    }

    for (auto i : list_delete)
    {
        i->eraseFromParent();
    }
}

// Builtin CallGraph Analysis
#define PASS_FLAG2 "igc-callgraphscc-analysis"
#define PASS_DESCRIPTION2 "Analyzes CallGraphSCC"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(BuiltinCallGraphAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BuiltinCallGraphAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char BuiltinCallGraphAnalysis::ID = 0;

BuiltinCallGraphAnalysis::BuiltinCallGraphAnalysis() : ModulePass(ID)
{
    initializeBuiltinCallGraphAnalysisPass(*PassRegistry::getPassRegistry());
}

/// Check whether there are recursions.
static bool hasRecursion(CallGraph &CG)
{
    // Use Tarjan's algorithm to detect recursions.
    for (auto I = scc_begin(&CG), E = scc_end(&CG); I != E; ++I)
    {
        const std::vector<CallGraphNode *> &SCCNodes = *I;
        if (SCCNodes.size() >= 2)
        {
            return true;
        }

        // Check self-recursion.
        auto Node = SCCNodes.back();
        for (auto Callee : *Node)
        {
            if (Callee.second == Node)
            {
                return true;
            }
        }
    }

    // No recursion.
    return false;
}

bool BuiltinCallGraphAnalysis::runOnModule(Module &M)
{
    if (IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_FORCE_INLINE)
    {
        return false;
    }

    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    if (IGC_IS_FLAG_DISABLED(EnableRecursionOpenCL) &&
        !ctx->m_DriverInfo.AllowRecursion() &&
        hasRecursion(CG))
    {
        IGC_ASSERT_MESSAGE(0, "Recursion detected!");
        ctx->EmitError(" undefined reference to `jmp()' ");
        return false;
    }

    //Return if any error
    if (!(ctx->oclErrorMessage.empty()))
    {
        return false;
    }

    for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I)
    {
        const std::vector<CallGraphNode *> &SCCNodes = *I;
        traveseCallGraphSCC(SCCNodes);
    }
    return false;
}

void BuiltinCallGraphAnalysis::traveseCallGraphSCC(const std::vector<CallGraphNode *> &SCCNodes)
{
    // all functions in one scc should end up with the same result
    ImplicitArgmentDetail *argData = nullptr;
    for (auto CGN : SCCNodes)
    {
        Function *f = CGN->getFunction();
        if (!f || f->isDeclaration())
            continue;
        // Fail on variadic functions.
        if (f->isVarArg())
        {
            std::string Msg = "Invalid user defined function being processed: ";
            Msg += f->getName();
            Msg += "()\n";
            getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(Msg.c_str());
            return;
        }
        if (argData == nullptr)
        {
            argDetails.push_back(IGCLLVM::make_unique<ImplicitArgmentDetail>());
            argData = argDetails[argDetails.size() - 1].get();
        }

        // calculate args from sub-routine.
        // This function have not beeen processed yet, therefore no map-entry for it yet
        IGC_ASSERT(argMap.count(f) == 0);
        for (auto N : (*CGN))
        {
            Function *sub = N.second->getFunction();
            // if callee has not been visited
            auto argMapIter = argMap.find(sub);
            // if we have processed the arguments for callee
            if (argMapIter != argMap.end())
            {
                IGC_ASSERT(nullptr != argMapIter->second);
                combineTwoArgDetail(*argData, *(argMapIter->second), N.first);
            }
        }
    }

    for (auto CGN : SCCNodes)
    {
        Function *f = CGN->getFunction();
        if (!f || f->isDeclaration())
            continue;
        FunctionInfoMetaDataHandle funcInfo = m_pMdUtils->getFunctionsInfoItem(f);
        // calculate implicit args from function metadata
        auto I = funcInfo->begin_ImplicitArgInfoList();
        auto E = funcInfo->end_ImplicitArgInfoList();

        // build everything
        for (; I != E; I++)
        {
            ArgInfoMetaDataHandle argInfo = *I;
            ImplicitArg::ArgType argId = (ImplicitArg::ArgType)argInfo->getArgId();

            if (argId < ImplicitArg::ArgType::STRUCT_START)
            {
                // unique implicit argument

                // if not exit, the following line will add one.
                ImplicitArg::ArgValSet * setx = &(argData->ArgsMaps[argId]);
                setx->insert(0);
            }
            else if (argId <= ImplicitArg::ArgType::STRUCT_END)
            {
                // aggregate implicity argument

                ImplicitStructArgument info;
                info.DW0.All.argExplictNum = argInfo->getExplicitArgNum();
                info.DW0.All.argOffset = argInfo->getStructArgOffset();
                info.DW0.All.argId = argId;
                (*argData).StructArgSet.insert(info.DW0.Value);
            }
            else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE)
            {
                // image index, project id

                int argNum = argInfo->getExplicitArgNum();
                ImplicitArg::ArgValSet * setx = &(argData->ArgsMaps[argId]);
                setx->insert(argNum);
            }
            else
            {
                // unique implicit argument

                // if not exit, the following line will add one.
                ImplicitArg::ArgValSet * setx = &(argData->ArgsMaps[argId]);
                setx->insert(0);
            }
        }
    }
    for (auto CGN : SCCNodes)
    {
        Function *f = CGN->getFunction();
        if (!f || f->isDeclaration())
            continue;
        // write everything back into metaData
        writeBackAllIntoMetaData(*argData, f);
        argMap.insert(std::make_pair(f, argData));
    }
}

void BuiltinCallGraphAnalysis::combineTwoArgDetail(
    ImplicitArgmentDetail &retD,
    ImplicitArgmentDetail &argD,
    llvm::Value* v)
{
    for (const auto& argPair : argD.ArgsMaps)
    {
        ImplicitArg::ArgType argId = argPair.first;
        if (argId < ImplicitArg::ArgType::STRUCT_START)
        {
            // unique implicit argument
            // if not exit, the following line will add one.
            ImplicitArg::ArgValSet * setx = &retD.ArgsMaps[argId];
            setx->insert(0);
        }
        else if (argId <= ImplicitArg::ArgType::STRUCT_END)
        {
            // aggregate implicity argument

            IGC_ASSERT_MESSAGE(0, "wrong location for this kind of argument type");

        }
        else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE)
        {
            // image index

            CallInst *cInst = dyn_cast<CallInst>(v);
            if (!cInst)
            {
                IGC_ASSERT_MESSAGE(0, " Not supported");
                getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(" undefined reference to `jmp()' ");
                return;
            }

            ImplicitArg::ArgValSet argSet = argPair.second;
            ImplicitArg::ArgValSet * setx = &retD.ArgsMaps[argId];

            // loop all image arguments on the sub-funtion.
            for (const auto& argI : argSet)
            {
                // find it from calling instruction, and trace it back to parent function argument
                Value* callArg = CImagesBI::CImagesUtils::traceImageOrSamplerArgument(cInst, argI);
                Argument* const arg = dyn_cast<Argument>(callArg);
                IGC_ASSERT_MESSAGE(nullptr != arg, "Not supported");
                setx->insert(arg->getArgNo());
            }
        }
        else
        {
            // unique implicit argument
            // if not exit, the following line will add one.
            ImplicitArg::ArgValSet * setx = &retD.ArgsMaps[argId];
            setx->insert(0);
        }
    }

    IGC_ASSERT_MESSAGE(0 == argD.StructArgSet.size(), "wrong argument type in user function");
}

void BuiltinCallGraphAnalysis::writeBackAllIntoMetaData(ImplicitArgmentDetail& data, Function * f)
{
    if (f->hasFnAttribute("IndirectlyCalled"))
        return;

    FunctionInfoMetaDataHandle funcInfo = m_pMdUtils->getFunctionsInfoItem(f);
    funcInfo->clearImplicitArgInfoList();

    for (const auto& A : data.ArgsMaps)
    {
        ImplicitArg::ArgType argId = A.first;
        if (argId < ImplicitArg::ArgType::STRUCT_START)
        {
            // unique implicit argument, add it on metadata

            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
            argMD->setArgId(argId);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
        else if (argId <= ImplicitArg::ArgType::STRUCT_END)
        {
            // aggregate implicity argument

            IGC_ASSERT_MESSAGE(0, "wrong location for this kind of argument type");

        }
        else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE)
        {
            // image index

            for (const auto& vOnSet: A.second)
            {
                ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
                argMD->setArgId(argId);
                argMD->setExplicitArgNum(vOnSet);
                funcInfo->addImplicitArgInfoListItem(argMD);
            }
        }
        else
        {
            // unique implicit argument

            ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
            argMD->setArgId(argId);
            funcInfo->addImplicitArgInfoListItem(argMD);
        }
    }

    for (const auto N : data.StructArgSet) // argument number
    {
        ArgInfoMetaDataHandle argMD = ArgInfoMetaDataHandle(ArgInfoMetaData::get());
        ImplicitStructArgument info;
        info.DW0.Value = N;

        argMD->setExplicitArgNum(info.DW0.All.argExplictNum);
        argMD->setStructArgOffset(info.DW0.All.argOffset);   // offset
        argMD->setArgId(info.DW0.All.argId);  // type
        funcInfo->addImplicitArgInfoListItem(argMD);
    }

    m_pMdUtils->save(f->getParent()->getContext());
}

