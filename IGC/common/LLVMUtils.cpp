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
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/PassTimer.hpp"
#include "common/Stats.hpp"
#include "common/debug/Dump.hpp"
#include "common/shaderOverride.hpp"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"

using namespace IGC;
using namespace IGC::Debug;
using namespace llvm;

void IGCPassManager::add(Pass *P)
{
    PassManager::add(P);
    if(IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
    {
        std::string passName = m_name + '_' + std::string(P->getPassName());
        auto name =
            IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(m_pContext->type)
            .Hash(m_pContext->hash)
            .Pass(passName, m_pContext->m_numPasses++)
            .Extension("ll");
        // The dump object needs to be on the Heap because it owns the stream, and the stream
        // is taken by reference into the printer pass. If the Dump object had been on the
        // stack, then that reference would go bad as soon as we exit this scope, and then
        // the printer pass would access an invalid pointer later on when we call PassManager::run()
        IGC::Debug::Dump* pDump = new IGC::Debug::Dump(name, IGC::Debug::DumpType::PASS_IR_TEXT);
        PassManager::add(P->createPrinterPass(pDump->stream(), ""));
        m_irDumps.push_back(pDump);
    }
}

IGCPassManager::~IGCPassManager()
{
    if(IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
    {
        for(auto it : m_irDumps)
        {
            delete it;
        }
    }
}

void DumpLLVMIR(IGC::CodeGenContext* pContext, const char* dumpName)
{
	SetCurrentDebugHash(pContext->hash.asmHash.value);
    if (IGC_IS_FLAG_ENABLED(DumpLLVMIR))
    {
        pContext->getMetaDataUtils()->save(toLLVMContext(*pContext));
        serialize(*(pContext->getModuleMetaData()), pContext->getModule());
        using namespace IGC::Debug;
        auto name =
            DumpName(IGC::Debug::GetShaderOutputName())
            .Hash(pContext->hash)
            .Type(pContext->type)
            .Pass(dumpName)
            .Retry(pContext->m_retryManager.GetRetryId())
            .Extension("ll");
        DumpLLVMIRText(
            pContext->getModule(),
            Dump(name, DumpType::PASS_IR_TEXT),
            pContext->annotater);
    }
    if (IGC_IS_FLAG_ENABLED(ShaderOverride))
    {
        auto name =
            DumpName(IGC::Debug::GetShaderOutputName())
            .Hash(pContext->hash)
            .Type(pContext->type)
            .Pass(dumpName)
            .Extension("ll");
        SMDiagnostic Err;
        std::string fileName = name.overridePath();
        FILE* fp = fopen(fileName.c_str(), "r");
        if (fp != nullptr)
        {
            fclose(fp);
            errs() << "Override shader: " << fileName << "\n";
            Module* mod = parseIRFile(fileName, Err, toLLVMContext(*pContext)).release();
            if (mod)
            {
                pContext->deleteModule();
                pContext->setModule(mod);
                deserialize(*(pContext->getModuleMetaData()), mod);
                appendToShaderOverrideLogFile(fileName, "OVERRIDEN: ");
            }
            else
            {
                std::stringstream ss;
                ss << "Parse IR failed.\n";
                ss << Err.getLineNo() << ": "
                    << Err.getLineContents().str() << "\n"
                    << Err.getMessage().str() << "\n";

                std::string str = ss.str();
                errs() << str;
                appendToShaderOverrideLogFile(fileName, str.c_str());
            }
        }
    }
}

// This function was implemented due to disability of creating new arguments for
// already existing functions via argument constructor. LLVM 7 does not give us
// such possibility. The only way is to create totally new function with new 
// signature and then splice the basic block list. 
// BE CAREFUL: this function changes pointer to the function passed as a parameter.
// It is reassigned to the new created function (with an additional parameters). 
// The function initially passed as an argument is not needed anymore, because all
// of the basic blocks are spliced to the new function.
std::vector<llvm::Argument *> SpliceFuncWithNewArguments(
    llvm::Function* &pSourceFunc,
    const std::vector<llvm::Type*>& newArgs,
    const std::vector<std::string>& newArgsNames)
{
    assert(newArgsNames.size() <= newArgs.size() && "To many arguments names");
    std::vector<llvm::Type*> arguments;

    // gather all arguments from source function
    for (auto& argIter : range(pSourceFunc->arg_begin(), pSourceFunc->arg_end()))
    {
        arguments.push_back(argIter.getType());
    }
    // concatenate old arguments and new arguments
    arguments.insert(arguments.end(), newArgs.begin(), newArgs.end());
    
    llvm::Function* pNewFunction = llvm::Function::Create(llvm::FunctionType::get(
        pSourceFunc->getReturnType(), arguments, false),
        pSourceFunc->getLinkage(),
        pSourceFunc->getName(),
        pSourceFunc->getParent());

    pNewFunction->setAttributes(pSourceFunc->getAttributes());
    pNewFunction->getBasicBlockList().splice(pNewFunction->begin(), pSourceFunc->getBasicBlockList());

    // set names of copied arguments
    auto newArg = pNewFunction->arg_begin();
    for (auto oldArg = pSourceFunc->arg_begin(),
        oldArgEnd = pSourceFunc->arg_end();
        oldArg != oldArgEnd;
        ++oldArg, ++newArg)
    {
        oldArg->replaceAllUsesWith(&(*newArg));
        newArg->takeName(&(*oldArg));
    }
    // set names for newly added arguments and collect them to the vector
    // at this moment, pointer newArg must point to the first new argument
    std::vector<llvm::Argument *>newArguments;
    for (auto argName = newArgsNames.begin(); newArg != pNewFunction->arg_end(); newArg++)
    {
        if (argName != newArgsNames.end())
        {
            newArg->setName(*argName);
            argName++;
        }
        newArguments.push_back(&*newArg);
    }
    pNewFunction->takeName(pSourceFunc);
    //pNewFunction->setName(pSourceFunc->getName());
    pSourceFunc->removeFromParent();
    // reassign function pointer to newly created function (with new arguments)
    pSourceFunc = pNewFunction;

    assert(newArguments.size() && "This function should create one argument at least.");

    return newArguments;
}

llvm::Argument* SpliceFuncWithNewArgument(
    llvm::Function* &pSourceFunc,
    llvm::Type* newArg,
    std::string newArgName)
{
    return SpliceFuncWithNewArguments(pSourceFunc, std::vector<llvm::Type*>(1, newArg), std::vector<std::string>(1, newArgName)).at(0);
}