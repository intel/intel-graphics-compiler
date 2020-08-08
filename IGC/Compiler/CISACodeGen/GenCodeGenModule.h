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
#pragma once
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CallGraph.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC {
    class GenXFunctionGroupAnalysis;

    /// \brief A module pass to initialize GenXFunctionGroupAnalysis and sort function
    /// list in a module properly.
    ///
    /// The module pass's results have two parts:
    ///
    /// (1) GenXFunctionGroupAnalysis object, which stores information needed for vISA
    ///     emission. E.g, each non-empty function belongs to a uniquely defined
    ///     function group with a kernel function as the head.
    ///
    /// (2) The module itself. All functions reachable from different function
    ///     groups will be cloned if necessary; they will be ordered such that each
    ///     callee will be after the caller in the module function list. When clone
    ///     happens, this module pass returns true, otherwise it returns false.
    ///     Currently, we assume no kernel functions will be called. This
    ///     requirement could be enforced before this pass by inlining kernels.
    ///
    class GenXCodeGenModule : public llvm::ModulePass {
    public:
        static char ID;
        GenXCodeGenModule();
        ~GenXCodeGenModule();
        virtual llvm::StringRef getPassName() const  override { return "GenX CodeGen module"; }
        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
        bool runOnModule(llvm::Module& M) override;
    private:
        void processFunction(llvm::Function& F);
        void processSCC(std::vector<llvm::CallGraphNode*>* SCCNodes);
        llvm::Function* cloneFunc(llvm::Function* F);
        GenXFunctionGroupAnalysis* FGA;
        IGC::IGCMD::MetaDataUtils* pMdUtils;
        bool Modified;
    };

    /// \brief A collection of functions that are reachable from a kernel.
    class FunctionGroup {
    public:
        friend class GenXFunctionGroupAnalysis;
        /// \brief use 2-d vector of Functions to represent FunctionGroup.
        /// Each sub-vector is a subgroup led by a kernel or a stack-call function.
        /// Element [0][0] is the group head. Element[i][0] is the sub-group head.
        llvm::SmallVector<llvm::SmallVector<llvm::AssertingVH<llvm::Function>, 8>*, 4> Functions;

        ~FunctionGroup() {
            for (auto I = Functions.begin(), E = Functions.end(); I != E; I++) {
                delete (*I);
            }
        }
        /// \brief The entry kernel function of group.
        llvm::Function* getHead() {
            return Functions.front()->front();
        }
        /// \brief The tail function of a group.
        llvm::Function* back() {
            return Functions.back()->back();
        }
        /// \brief Only one function in this group
        bool isSingle() {
            return (Functions.size() == 1 && Functions.front()->size() == 1);
        }
        /// \brief Function group has a stack call (including indirect calls)
        bool hasStackCall() {
            return m_hasStackCall;
        }

        void replaceGroupHead(llvm::Function* OH, llvm::Function* NH) {
            auto headSG = Functions[0];
            llvm::AssertingVH<llvm::Function>& HVH = (*headSG)[0];
            IGC_ASSERT_MESSAGE(&(*HVH) == OH, "Group's head isn't set up correctly!");
            HVH = NH;
        }

    private:
        bool m_hasStackCall = false;
    };

    class GenXFunctionGroupAnalysis : public llvm::ImmutablePass {
        /// \brief The module being analyzed.
        llvm::Module* M;

        /// \brief Function groups constructed in this module.
        llvm::SmallVector<FunctionGroup*, 8> Groups;

        /// \brief Each function belongs to a uniquely defined function group.
        llvm::DenseMap<llvm::Function*, FunctionGroup*> GroupMap;

        /// \brief Each function also belongs to a uniquely defined sub-group of a stack-call entry
        llvm::DenseMap<llvm::Function*, llvm::Function*> SubGroupMap;

    public:
        static char ID;
        explicit GenXFunctionGroupAnalysis();
        ~GenXFunctionGroupAnalysis() { clear(); }

        virtual llvm::StringRef getPassName() const  override { return "FunctionGroup analysis"; }

        /// This function returns nullptr if no analysis is not available.
        llvm::Module* getModule() { return M; }
        void setModule(llvm::Module* Mod) { M = Mod; }
        void clear();

        /// \brief This function rebuilds function groups with the assumption that
        /// no function will be directly or undirectly called from more than one
        /// kernel, and functions in the module are well-ordered. It returns false if
        /// fails to rebuild and returns true otherwise.
        ///
        bool rebuild(llvm::Module* Mod);

        // Attach all indirectly called functions to a single kernel group
        void addIndirectFuncsToKernelGroup(llvm::Module* pModule);

        // Replace OF with NF in Groups and GroupMap
        void replaceEntryFunc(llvm::Function* OF, llvm::Function* NF);

        /// \brief Verify if this analysis result is valid.
        bool verify();

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        /// \brief Get the FunctionGroup containing Function F, else nullptr.
        FunctionGroup* getGroup(llvm::Function* F);

        /// \brief Get the FunctionGroup for which Function F is the head, else
        /// nullptr.
        FunctionGroup* getGroupForHead(llvm::Function* F);

        /// \brief Get the group head for the group to which F belongs.
        llvm::Function* getGroupHead(llvm::Function* F) {
            auto FG = getGroup(F);
            IGC_ASSERT(nullptr != FG);
            return FG->getHead();
        }
        /// \brief Get the subgroup head for the subgroup to which F belongs
        llvm::Function* getSubGroupMap(llvm::Function* F) {
            auto I = SubGroupMap.find(F);
            if (I == SubGroupMap.end())
                return nullptr;
            return I->second;
        }

        void setSubGroupMap(llvm::Function* F, llvm::Function* SubGroupHead) {
            SubGroupMap[F] = SubGroupHead;
        }

        void setGroupStackCall() {
            for (auto FG : Groups) {
                FG->m_hasStackCall = (FG->Functions.size() > 1);
            }
        }

        /// \brief Check whether this is a group header.
        bool isGroupHead(llvm::Function* F) {
            return getGroupForHead(F) != nullptr;
        }

        /// \brief Check whether this is the last function in a function group. This
        /// order is also reflected in the module function list.
        bool isGroupTail(llvm::Function* F) {
            FunctionGroup* FG = getGroup(F);
            IGC_ASSERT_MESSAGE(nullptr != FG, "not in function group");
            return F == FG->back();
        }

        /// this is the knob to enable vISA stack-call for OpenCL
        bool useStackCall(llvm::Function* F);

        typedef llvm::SmallVectorImpl<FunctionGroup*>::iterator iterator;
        iterator begin() { return iterator(Groups.begin()); }
        iterator end() { return iterator(Groups.end()); }

        /// \brief Returns the number of groups, aka. kernels.
        size_t size() { return Groups.size(); }

        /// \brife add Function F to FunctionGroup FG.
        void addToFunctionGroup(llvm::Function* F, FunctionGroup* FG, llvm::Function* SubGrpH);

        /// \brief Create a new FunctionGroup with head F.
        FunctionGroup* createFunctionGroup(llvm::Function* F);

        void print(llvm::raw_ostream& os);

#if defined(_DEBUG)
        void dump();
#endif
    };

    llvm::ModulePass* createGenXCodeGenModulePass();
    llvm::ImmutablePass* createGenXFunctionGroupAnalysisPass();
    llvm::Pass* createSubroutineInlinerPass();

} // namespace IGC
