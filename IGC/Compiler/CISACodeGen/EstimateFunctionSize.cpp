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
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char EstimateFunctionSize::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)
IGC_INITIALIZE_PASS_END(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)

llvm::ModulePass* IGC::createEstimateFunctionSizePass() {
    initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
    return new EstimateFunctionSize;
}

llvm::ModulePass*
IGC::createEstimateFunctionSizePass(EstimateFunctionSize::AnalysisLevel AL) {
    initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
    return new EstimateFunctionSize(AL);
}

EstimateFunctionSize::EstimateFunctionSize(AnalysisLevel AL)
    : ModulePass(ID), M(nullptr), AL(AL), HasRecursion(false) {}

EstimateFunctionSize::~EstimateFunctionSize() { clear(); }

void EstimateFunctionSize::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

bool EstimateFunctionSize::runOnModule(Module& Mod) {
    clear();
    M = &Mod;
    analyze();
    checkSubroutine();
    return false;
}

// Given a module, estimate the maximal function size with complete inlining.
/*
   A ----> B ----> C ---> D ---> F
    \       \       \
     \       \       \---> E
      \       \
       \       \---> C ---> D --> F
        \             \
         \----> F      \---> E
*/
// ExpandedSize(A) = size(A) + size(B) + 2 * size(C) + 2 * size(D)
//                   + 2 * size(E) + 3 * size(F)
//
// We compute the size as follows:
//
// (1) Initialize the data structure
//
// A --> {size(A), [B, F], [] }
// B --> {size(B), [C, C], [A] }
// C --> {size(C), [D, E], [B] }
// D --> {size(D), [F],    [C] }
// E --> {size(E), [],     [C] }
// F --> {size(F), [],     [A, D] }
//
// where the first list consists of functions to be expanded and the second list
// consists of its caller functions.
//
// (2) Expand all leaf nodes until all become leaf nodes. We update the data
// structure to the following after expanding functions E and F:
//
// A --> {size(A) + size(F), [B],    [] }
// B --> {size(B),           [C, C], [A] }
// C --> {size(C) + size(E), [D],    [B] }
// D --> {size(D) + size(F), [],     [C] }
// E --> {size(E),           [],     [C] }
// F --> {size(F),           [],     [A, D] }
//
namespace {

    /// Associate each function with a partially expanded size and remaining
    /// unexpanded function list.
    struct FunctionNode {
        FunctionNode(Function* F, std::size_t Size)
            : F(F), Size(Size), Processed(false), CallingSubroutine(false) {}

        Function* F;

        /// \brief Partially expanded size, or completely expanded size for a
        /// leaf node.
        std::size_t Size;

        /// \brief A flag to indicate whether this node has been fully expanded.
        bool Processed;

        /// \brief A flag to indicate whether this node has a subroutine call before
        /// expanding.
        bool CallingSubroutine;

        /// \brief All functions directly called in this function.
        std::vector<Function*> CalleeList;

        /// \brief All functions that call this function F.
        std::vector<Function*> CallerList;

        /// \brief A node becomes a leaf when all called functions are expanded.
        bool isLeaf() const { return CalleeList.empty(); }

        /// \brief Add a caller or callee.
        void addCallee(Function* G) {
            assert(!G->empty());
            CalleeList.push_back(G);
            CallingSubroutine = true;
        }
        void addCaller(Function* G) {
            assert(!G->empty());
            CallerList.push_back(G);
        }

        /// \brief A single step to expand F: accumulate the size and remove it
        /// from the callee list.
        void expand(FunctionNode* Node) {
            // Multiple calls to a function is allowed as in the example above.
            for (auto I = CalleeList.begin(); I != CalleeList.end(); /* empty */) {
                if (*I == Node->F) {
                    Size += Node->Size;
                    I = CalleeList.erase(I);
                }
                else
                    ++I;
            }
        }

#if defined(_DEBUG)
        void print(raw_ostream& os);

        void dump() { print(llvm::errs()); }
#endif
    };

} // namespace
#if defined(_DEBUG)

void FunctionNode::print(raw_ostream& os) {
    os << "Function: " << F->getName() << ", " << Size << "\n";
    for (auto G : CalleeList)
        os << "--->>>" << G->getName() << "\n";
    for (auto G : CallerList)
        os << "<<<---" << G->getName() << "\n";
}
#endif

void EstimateFunctionSize::clear() {
    M = nullptr;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        auto Node = (FunctionNode*)I->second;
        delete Node;
    }
    ECG.clear();
}

void EstimateFunctionSize::analyze() {
    auto getSize = [](llvm::Function& F) -> std::size_t {
        std::size_t Size = 0;
        for (auto& BB : F.getBasicBlockList())
            Size += BB.size();
        return Size;
    };

    // Initial the data structure.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;
        ECG[&F] = new FunctionNode(&F, getSize(F));
    }

    // Visit all call instructions and populate CG.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;

        FunctionNode* Node = get<FunctionNode>(&F);
        for (auto U : F.users()) {
            // Other users (like bitcast/store) are ignored.
            if (auto * CI = dyn_cast<CallInst>(U)) {
                // G calls F, or G --> F
                Function* G = CI->getParent()->getParent();
                get<FunctionNode>(G)->addCallee(&F);
                Node->addCaller(G);
            }
        }
    }

    // Expand leaf nodes until all are expanded (the second list is empty).
    while (true) {
        // Find unexpanded leaf nodes.
        SmallVector<FunctionNode*, 8> LeafNodes;
        for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
            auto Node = (FunctionNode*)I->second;
            if (!Node->Processed && Node->isLeaf())
                LeafNodes.push_back(Node);
        }

        // Done.
        if (LeafNodes.empty())
            break;

        // Expand leaf nodes one by one.
        for (auto Node : LeafNodes) {
            assert(Node->CalleeList.empty());
            // Populate to its Callers.
            for (auto Caller : Node->CallerList)
                get<FunctionNode>(Caller)->expand(Node);
            Node->Processed = true;
        }
    }

    HasRecursion = false;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        FunctionNode* Node = (FunctionNode*)I->second;
        if (!Node->isLeaf()) {
            HasRecursion = true;
        }
    }
}

/// \brief Return the estimated maximal function size after complete inlining.
std::size_t EstimateFunctionSize::getMaxExpandedSize() const {
    std::size_t MaxSize = 0;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        FunctionNode* Node = (FunctionNode*)I->second;
        // Only functions with subroutine calls count.
        if (Node->CallingSubroutine)
            MaxSize = std::max(MaxSize, Node->Size);
    }
    return MaxSize;
}

void EstimateFunctionSize::checkSubroutine() {
    auto CGW = getAnalysisIfAvailable<CodeGenContextWrapper>();
    if (!CGW || AL != AL_Module)
        return;

    bool neverInline = false;

    CodeGenContext* pContext = CGW->getCodeGenContext();
    bool EnableSubroutine = true;
    if (pContext->type != ShaderType::OPENCL_SHADER &&
        pContext->type != ShaderType::COMPUTE_SHADER)
        EnableSubroutine = false;
    else if (pContext->m_instrTypes.hasIndirectCall)
        EnableSubroutine = false;

    // Enable subroutine if function has the "UserSubroutine" attribute
    if (!EnableSubroutine) {
        for (Function& F : *M) {
            if (F.hasFnAttribute("UserSubroutine")) {
                EnableSubroutine = true;
                if (F.hasFnAttribute(llvm::Attribute::NoInline)) {
                    neverInline = true;
                }
            }
        }
    }

    if (neverInline) {
        EnableSubroutine = true;
    }
    else if (EnableSubroutine) {
        std::size_t Threshold = IGC_GET_FLAG_VALUE(SubroutineThreshold);
        std::size_t MaxSize = getMaxExpandedSize();
        if (MaxSize <= Threshold && !HasRecursion)
            EnableSubroutine = false;
    }

    if (IGC_IS_FLAG_ENABLED(EnableOCLNoInlineAttr) &&
        pContext->type == ShaderType::OPENCL_SHADER)
    {
        for (Function& F : *M)
        {
            if (F.hasFnAttribute(llvm::Attribute::NoInline) &&
                !F.hasFnAttribute(llvm::Attribute::Builtin)) {
                EnableSubroutine = true;
                break;
            }
        }
    }

    for (Function& F : *M)
    {
        if (F.hasFnAttribute("KMPLOCK"))
        {
            EnableSubroutine = true;
            break;
        }
    }

    if (EnableSubroutine) {
        // Disable retry manager when subroutine is enabled.
        pContext->m_retryManager.Disable();
    }

    assert(!HasRecursion || EnableSubroutine);
    // Store result into the context (this decision should be immutable).
    pContext->m_enableSubroutine = EnableSubroutine;
}

std::size_t EstimateFunctionSize::getExpandedSize(const Function* F) const {
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        assert(F == Node->F);
        return Node->Size;
    }

    // Unknown.
    return std::numeric_limits<std::size_t>::max();
}

bool EstimateFunctionSize::onlyCalledOnce(const Function* F) {
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        assert(F == Node->F);
        // one call-site and not a recursion
        if (Node->CallerList.size() == 1 &&
            Node->CallerList.front() != F) {
            return true;
        }
        // OpenCL specific, called once by each kernel
        SmallPtrSet<Function*, 8> CallerSet;
        auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
        if (MdWrapper) {
            auto pMdUtils = MdWrapper->getMetaDataUtils();
            for (auto Caller : Node->CallerList) {
                if (!isEntryFunc(pMdUtils, Caller)) {
                    return false;
                }
                if (CallerSet.count(Caller)) {
                    return false;
                }
                CallerSet.insert(Caller);
            }
            return true;
        }
    }
    return false;
}
