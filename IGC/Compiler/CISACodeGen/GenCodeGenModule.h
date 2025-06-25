/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CallGraph.h"
#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"
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
        void setFuncProperties(llvm::CallGraph& CG);
        void copyFuncProperties(llvm::Function* To, llvm::Function* From);

        GenXFunctionGroupAnalysis* FGA;
        IGC::IGCMD::MetaDataUtils* pMdUtils;
        bool Modified;
        unsigned m_FunctionCloningThreshold = 0;

        // Find the set of functions that cannot be promoted to indirect, must be cloned
        void detectUnpromotableFunctions(llvm::Module* pM);
        std::set<llvm::Function*> m_UnpromotableFuncs;
    };

    /// \brief A collection of functions that are reachable from a kernel.
    class FunctionGroup {
    public:
        friend class GenXFunctionGroupAnalysis;
        /// \brief use 2-d vector of Functions to represent FunctionGroup.
        /// Each sub-vector is a subgroup led by a kernel or a stack-call function.
        /// Element [0][0] is the group head. Element[i][0] is the sub-group head.
        typedef llvm::SmallVector<llvm::AssertingVH<llvm::Function>, 8> SubGroupContainer;
        typedef llvm::SmallVector<SubGroupContainer*, 4> FunctionGroupContainer;
        FunctionGroupContainer Functions;

        class iterator
        {
        public:
            enum POS { BEGIN, END };
            iterator(FunctionGroupContainer &FC, POS pos)
            {
                if (pos == BEGIN)
                {
                    major = FC.begin();
                    majorEnd = FC.end();
                    minor = (*major)->begin();
                }
                else if (pos == END)
                {
                    major = FC.end();
                    majorEnd = FC.end();
                    minor = FC.back()->end();
                }
            }
            iterator& operator++()
            {
                minor++;
                if (minor == (*major)->end())
                {
                    major++;
                    if (major != majorEnd)
                        minor = (*major)->begin();
                }
                return *this;
            }
            llvm::Function* operator*()
            {
                return *minor;
            }
            bool operator==(const iterator& rhs) const
            {
                return (major == rhs.major && minor == rhs.minor);
            }
            bool operator!=(const iterator& rhs) const
            {
                return !(*this == rhs);
            }
        private:
            FunctionGroupContainer::iterator major;
            FunctionGroupContainer::iterator majorEnd;
            SubGroupContainer::iterator minor;
        };

        iterator begin() { return iterator(Functions, iterator::BEGIN); }
        iterator end() { return iterator(Functions, iterator::END); }

        ~FunctionGroup() {
            for (auto F : Functions) {
                delete F;
            }
        }
        FunctionGroup() = default;
        FunctionGroup(const FunctionGroup&) = delete;
        FunctionGroup& operator=(const FunctionGroup&) = delete;

        /// \brief The entry kernel function of group.
        llvm::Function* getHead() const {
            return Functions.front()->front();
        }
        /// \brief The tail function of a group.
        llvm::Function* back() const {
            return Functions.back()->back();
        }
        /// \brief Only one function in this group
        bool isSingle() const {
            return (Functions.size() == 1 && Functions.front()->size() == 1);
        }
        /// \brief Only one function in this group ignoring stack overflow detection methods
        bool isSingleIgnoringStackOverflowDetection() const {
            auto isNotStackOverflowDetection = [](const llvm::Function* F) {
                return !F->getName().startswith("__stackoverflow_detection") && !F->getName().startswith("__stackoverflow_init");
            };
            return (Functions.size() == 1 &&
                    std::count_if(Functions.front()->begin(), Functions.front()->end(), isNotStackOverflowDetection) == 1);
        }
        /// \brief Function group has a subroutine
        bool hasSubroutine() const {
            return m_hasSubroutine;
        }
        /// \brief Function group has a stack call (including indirect calls)
        bool hasStackCall() const {
            return m_hasStackCall;
        }
        bool hasInlineAsm() const {
            return m_hasInlineAsm;
        }
        /// \brief Function group has a variable length alloca
        bool hasVariableLengthAlloca() const {
            return m_hasVariableLengthAlloca;
        }
        /// \brief Function group has indirect calls (i.e. calls functions in the Indirect Function Group)
        bool hasIndirectCall() const {
            return m_hasIndirectCall;
        }
        /// \brief Function group has nested calls
        bool hasNestedCall() const {
            return m_hasNestedCall;
        }
        /// \brief Function group has indirect calls where the full CG is not available (e.g. calls function pointer, or callees in external module)
        bool hasPartialCallGraph() const {
            return m_hasPartialCallGraph;
        }
        /// \brief Function group has recursion
        bool hasRecursion() const {
            return m_hasRecursion;
        }

        void replaceGroupHead(llvm::Function* OH, llvm::Function* NH) {
            IGC_UNUSED(OH);
            auto headSG = Functions[0];
            llvm::AssertingVH<llvm::Function>& HVH = (*headSG)[0];
            IGC_ASSERT_MESSAGE(&(*HVH) == OH, "Group's head isn't set up correctly!");
            HVH = NH;
        }

        // For a single FG, an SIMD mode is valid only if SIMD modes of all
        // functions in that group are valid.
        bool checkSimdModeValid(SIMDMode Mode) const;
        void setSimdModeInvalid(SIMDMode Mode);

    private:
        bool m_hasSubroutine = false;
        bool m_hasStackCall = false;
        bool m_hasInlineAsm = false;
        bool m_hasVariableLengthAlloca = false;
        bool m_hasIndirectCall = false;
        bool m_hasRecursion = false;
        bool m_hasNestedCall = false;
        bool m_hasPartialCallGraph = false;
        bool SIMDModeValid[3] = {true, true, true};
    };

    class GenXFunctionGroupAnalysis : public llvm::ImmutablePass {
        enum class FuncPropertyInfo : uint32_t {
            FPI_LEAF = 0x1        // bit 0:  1 (leaf function)
        };

        /// \brief The module being analyzed.
        llvm::Module* M;

        /// \brief Function groups constructed in this module.
        llvm::SmallVector<FunctionGroup*, 8> Groups;

        /// \brief Each function belongs to a uniquely defined function group.
        llvm::DenseMap<const llvm::Function*, FunctionGroup*> GroupMap;

        /// \brief Each function also belongs to a uniquely defined sub-group of a stack-call entry
        llvm::DenseMap<const llvm::Function*, llvm::Function*> SubGroupMap;

        /// \brief Properties for each function
        llvm::DenseMap<const llvm::Function*, uint32_t> FuncProperties;

        /// \brief Special group that contains all indirect call functions
        /// SIMD variants stored with SIMD8 in idx 0, SIMD16 in idx 1, SIMD32 in idx 2
        llvm::SmallVector<FunctionGroup*, 3> IndirectCallGroup{ nullptr, nullptr, nullptr };

    public:
        static char ID;
        explicit GenXFunctionGroupAnalysis();
        ~GenXFunctionGroupAnalysis() { clear(); }
        GenXFunctionGroupAnalysis(const GenXFunctionGroupAnalysis&) = delete;
        GenXFunctionGroupAnalysis& operator=(const GenXFunctionGroupAnalysis&) = delete;

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
        FunctionGroup* getGroup(const llvm::Function* F);

        /// \brief Get the FunctionGroup for which Function F is the head, else
        /// nullptr.
        FunctionGroup* getGroupForHead(const llvm::Function* F);

        /// \brief Get the group head for the group to which F belongs.
        llvm::Function* getGroupHead(const llvm::Function* F) {
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

        bool isIndirectCallGroup(const FunctionGroup* FG) {
            for (auto ICG : IndirectCallGroup) {
                if (FG && FG == ICG) return true;
            }
            return false;
        }

        bool isIndirectCallGroup(const llvm::Function* F) {
            IGC_ASSERT(F);
            FunctionGroup* FG = getGroup(F);
            return isIndirectCallGroup(FG);
        }

        /// \brief Check whether this is a group header.
        bool isGroupHead(const llvm::Function* F) {
            return getGroupForHead(F) != nullptr;
        }

        /// \brief Check whether this is the last function in a function group. This
        /// order is also reflected in the module function list.
        bool isGroupTail(const llvm::Function* F) {
            FunctionGroup* FG = getGroup(F);
            IGC_ASSERT_MESSAGE(nullptr != FG, "not in function group");
            return F == FG->back();
        }

        bool isLeafFunc(const llvm::Function* F) {
            auto FI = FuncProperties.find(F);
            if (FI != FuncProperties.end()) {
                return (FI->second & (uint32_t)FuncPropertyInfo::FPI_LEAF);
            }
            return false;
        }
        void setLeafFunc(const llvm::Function* F) {
            auto II = FuncProperties.find(F);
            uint32_t P = (II != FuncProperties.end()) ? II->second : 0;
            FuncProperties[F] = (P | (uint32_t)FuncPropertyInfo::FPI_LEAF);
        }
        void copyFuncProperties(const llvm::Function* To, const llvm::Function* From) {
            auto II = FuncProperties.find(From);
            if (II != FuncProperties.end()) {
                FuncProperties[To] = II->second;
            }
        }

        /// get or create the function group that holds all indirectly-called functions
        FunctionGroup* getOrCreateIndirectCallGroup(llvm::Module* pModule, int SimdSize = 0);

        /// for multi SIMD variant compilation, clone functions from the default dummy kernel group
        /// into the SIMD variant version of the dummy kernel.
        void CloneFunctionGroupForMultiSIMDCompile(llvm::Module* pModule);

        /// check if function is stack-called
        bool useStackCall(llvm::Function* F);

        /// sets function group attribute flags
        void setGroupAttributes();

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
