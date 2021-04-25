/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

#include <set>
#include <map>

namespace IGC
{
    class CodeGenContext;
}

namespace IGC
{
    // struct
    typedef std::map<unsigned int, llvm::Argument*> InfoToImpArgMap;
    typedef std::map<llvm::Function*, InfoToImpArgMap> FuncInfoToImpArgMap;


    typedef std::map<llvm::Argument*, unsigned int> ImpArgToExpNum;
    typedef std::map<llvm::Function*, ImpArgToExpNum> FuncImpToExpNumMap;

    /// @brief  AddImplicitArgs pass used for changing the function signatures and calls to represent
    ///         the implicit arguments needed by each function and passed from the OpenCL runtime
    ///         This pass depdends on the WIFuncAnalysis pass runing before it
    /// @Author Marina Yatsina
    class AddImplicitArgs : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        AddImplicitArgs();

        /// @brief  Destructor
        ~AddImplicitArgs() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "AddImplictArgs";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief  Main entry point.
        ///         Goes over all functions and changes their signature to contain the implicit arguments
        ///         needed by each function, goes over all function calls and adds the implicit arguments
        ///         to the function calls
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module &M) override;

    private:
        /// @brief  Check if a function has indirectly called parent to
        ///         decide whether implicit args should be added
        /// @param  pFunc           Source function
        static bool hasIndirectlyCalledParent(const llvm::Function* pFunc);

        /// @brief  Create the type of the new function,
        ///         including all explicit and needed impliict parameters
        /// @param  pFunc           The old function
        /// @param  pImplicitArgs   The implicit arguments needed by this function
        /// @returns    The new function type
        static llvm::FunctionType* getNewFuncType(const llvm::Function* pFunc, const ImplicitArgs& implicitArgs);

        /// @brief  Transfers uses of old arguments to new arguments, sets names of all arguments
        /// @param  pFunc           The old function
        /// @param  pNewFunc        The new function
        /// @param  pImplicitArgs   The implicit arguments needed by this function
        void updateNewFuncArgs(llvm::Function* pFunc, llvm::Function* pNewFunc, const ImplicitArgs& implicitArgs);

        /// @brief  Replace old CallInst with new CallInst
        void replaceAllUsesWithNewOCLBuiltinFunction(llvm::Function* old_func, llvm::Function* new_func);

        static llvm::Value* coerce(llvm::Value* arg, llvm::Type* type, llvm::Instruction* insertBefore);

        /// @brief  Metadata API object.
        IGC::IGCMD::MetaDataUtils *m_pMdUtils;

        FuncInfoToImpArgMap     m_FuncInfoToImpArgMap;
        FuncImpToExpNumMap      m_FuncImpToExpNumMap;
    };

} // namespace IGC

// Builtin CallGraph Analysis Class
namespace IGC
{

    struct ImplicitArgumentDetail
    {
        ImplicitArg::ArgMap     ArgsMaps;
        std::set<unsigned int>  StructArgSet;
    };

    class BuiltinCallGraphAnalysis : public llvm::ModulePass
    {
    public:
        static char ID;

        BuiltinCallGraphAnalysis();
        ~BuiltinCallGraphAnalysis() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "BuiltinCallGraphAnalysis";
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::CallGraphWrapperPass>();
        }

        virtual bool runOnModule(llvm::Module &M) override;

        void traverseCallGraphSCC(const std::vector<llvm::CallGraphNode *> &SCCNodes);
        void combineTwoArgDetail(ImplicitArgumentDetail&, const ImplicitArgumentDetail&, llvm::Value*) const;
        void writeBackAllIntoMetaData(const ImplicitArgumentDetail&, llvm::Function*);

        bool pruneCallGraphForStackCalls(llvm::CallGraph& CG);

    private:
        IGC::IGCMD::MetaDataUtils *m_pMdUtils;
        llvm::SmallDenseMap<llvm::Function*, ImplicitArgumentDetail *> argMap;
        llvm::SmallVector<std::unique_ptr<ImplicitArgumentDetail>, 4> argDetails;
    };

} // namespace IGC
