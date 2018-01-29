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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"

#include <list>
#include <map>

namespace IGC
{

// This code is shared between the IGIL and IGC compilers.  Do not modify one without
// propagating the change to the other compiler's code base.

class IGILGenericAddressStaticResolution : public llvm::FunctionPass
{
public:
    /// Pass identification, replacement for typeid
    static char ID;

    /// @brief  Constructor
    IGILGenericAddressStaticResolution();
    
    /// @brief  Constructor
    /// @param  HandlePrivateSpace - determines if this pass should try to staticly resolve generic-to-private and 
    ///                              and private-to-generic address space casts.
    ///                              Currently, this is enabled for RenderScript and disabled for OCL
    IGILGenericAddressStaticResolution(bool HandlePrivateSpace);

    /// @brief  Provides name of pass
    virtual llvm::StringRef getPassName() const override {
        return "GenericAddressStaticResolution";
    }

    /// @brief  LLVM Module pass entry
    /// @param  M Module to transform
    /// @returns  true if changed
    bool runOnFunction(llvm::Function &F) override;

    /// @brief  LLVM Interface
    /// @param  AU Analysis
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        // We could declare 'PromotePass' here (as mem2reg pass is a
        // mandatory pre-requisite).
    }

private:
    /// @brief  GAS Pointer Collection Data Model for the following accesses:
    /// @brief    1. Iteration through GAS pointers' instructions in a function
    typedef std::list<llvm::Instruction*> TPointerList;
    /// @brief    2. Quick access to a GAS pointer instruction info
    /// @brief       (mapping from instruction to its estimated named space pointer result type)
    typedef std::pair<llvm::Instruction*, unsigned int> TPointerInfo;
    typedef std::map<llvm::Instruction*, unsigned int>  TPointerMap;

    /// @brief  2. GAS pointers' instructions in data flow order (for one function only!)
    TPointerList  m_GASPointers;
    /// @brief  3. GAS pointers' instructions' info for quick access (for one function only!)
    TPointerMap  m_GASEstimate;

    /// @brief  Instruction Replacement Info Data Model for the following accesses:
    /// @brief  (mapping from old instruction /first element/ to new value /second element/)
    typedef std::pair<llvm::Instruction*, llvm::Value*> TMapPair;
    /// @brief  1. Fast access to replacement info (old-to-new)
    typedef std::map<llvm::Instruction*, llvm::Value*>  TReplaceMap;
    /// @brief  2. Deterministic iteration through replacement info
    typedef std::vector<TMapPair> TReplaceVector;

    /// @brief  <old-instruction>-to-<new-value> info
    /// @brief  1. Search for mapping 
    TReplaceMap m_replaceMap;
    /// @brief  2. Iterate through mappings
    TReplaceVector  m_replaceVector;

    /// @brief  counter of pointers which the pass was unable to resolve
    unsigned  m_failCount;
    
    /// @brief  determines if this pass should try to staticly resolve generic-to-private and 
    ///         and private-to-generic address space casts.
    ///         Currently, this is enabled for RenderScript and disabled for OCL
    bool m_handlePrivateSpace;

private:
    /// @brief  Prepares collection of GAS pointers' usages from a function and
    /// @brief  estimates their named address space
    /// @param  curFuncIt - iterator to function to traverse
    void analyzeGASPointers(llvm::Function &F);

    /// @brief  Resolves GAS pointers to named address space
    /// @param  curFuncIt - iterator to function to resolve
    /// @returns  true if any pointer was statically resolved, or false otherwise
    bool resolveGASPointers(llvm::Function &F);

    //  Helpers for GAS pointers' collection processing
    // -------------------------------------------------

    /// @brief  Helper for adding of generic pointer instruction
    /// @brief  to the collection (together with its use tree)
    /// @param  pInstr - instruction to add
    /// @param  space  - target named space
    /// @param  pParentInst - instruction from which we want to propagate the address space to pInstr
    void addGASInstr(
        llvm::Instruction*  pInstr,
        unsigned int space,
        llvm::Instruction*  pParentInst);

    /// @brief  Helper for propagation of address space type to instruction's uses
    /// @param  pInstr - instruction of interest
    /// @param  space  - space type to propagate
    /// @param  pParentInst - instruction from which we want to propagate the address space to pInstr
    void propagateSpace(
        llvm::Instruction*  pInstr,
        unsigned int space,
        llvm::Instruction*  pParentInst);

    /// @brief  Checks whether an operand is a constant expression which produces
    /// @brief  GAS pointer out of named address space pointer, and adds instruction
    /// @brief  to the collection if this is the case
    /// @param  pOperand - operand to check
    /// @param  pInstr   - instruction to which this operand belongs (directly, or via
    ///                    encompassing constant expression)
    /// @returns  true if the check is successful (and the instruction is added to 
    ///           the collection), or false otherwise 
    bool handleGASConstantExprIfNeeded(
        llvm::Value*  pOperand,
        llvm::Instruction*  pInstr);

    // For two pointer instructions like cmp and select, we need to check for race on the
    // address space
    unsigned int evalGASConstantExprIfNeeded(
        llvm::Value*  pOperand,
        llvm::Instruction*  pInstr);

    // Generic-to-named address space resolvers for different instructions
    // --------------------------------------------------------------------

    /// @brief  GAS resolvers for different instructions
    /// @param  pInstr - instruction to resolve
    /// @param  space  - target named space
    /// @param  funcIt - iterator to function which is under resolution
    /// @returns  true if the IR was modified, or false otherwise
    bool resolveInstructionConvert(
        llvm::Instruction*  pInstr,
        unsigned int space);

    bool resolveInstructionOnePointer(
        llvm::Instruction*  pInstr,
        unsigned int space);

    bool resolveInstructionTwoPointers(
        llvm::Instruction*  pInstr,
        unsigned int space);

    bool resolveInstructionPhiNode(
        llvm::PHINode*  pPhiInstr,
        unsigned int space);

    // Helpers for special cases
    // --------------------------

    /// @brief  Helper for resolution of GAS pointer constant expression
    /// @brief  to the constant's named address space
    /// @param  pVal  - value to resolve
    /// @param  space - target named space
    /// @returns  resolved value if it was possible, or NULL otherwise
    llvm::Value*  resolveConstantExpression(
        llvm::Value*  pVal,
        unsigned int space);

    // All operands of some instructions must conform to a given input address space
    bool conformOperand(
        llvm::Value*  pVal,
        unsigned int space);

    /// @brief  Helper for retrieval of replacement value for original instruction (from the map)
    /// @param  pInstr - original instruction
    /// @returns  pointer to replacement value or NULL if no replacement is identified
    llvm::Value*  getReplacementForInstr(llvm::Instruction*  pInstr);

    /// @brief  Helper for production of resolved value for instruction operand
    /// @param  pVal  - instruction operand
    /// @param  space - target named space
    /// @returns  pointer to resolved value or NULL if there is no resolved value available
    llvm::Value*  getResolvedOperand(
        llvm::Value*  pOperand,
        unsigned int space);

    /// @brief  Helper for fix-up of usages of new instruction pointer result:
    /// @brief    a) For incompatible named-vs-GAS pointers - induce bitcast
    /// @brief    b) If use is already-created PHI/Select/Icmp/Call - update corresponding input
    /// @param  pNewInstr - new instruction, whose usages should be fixed
    /// @param  pOldInstr - old instruction which will be replaced by new one
    void fixUpPointerUsages(
        llvm::Instruction*  pNewInstr,
        llvm::Instruction*  pOldInstr);

    // Checkers for special cases
    // ---------------------------

    /// @brief  Checks whether given type is an alloca of struct with GAS pointer
    /// @brief  pType            - type to check
    /// @brief  isStructDetected - true if we already identified 'struct' in
    ///         the course of recursion, or false otherwise
    /// @returns  true/false according to result of the check
    bool isAllocaStructGASPointer(
        const llvm::Type* pType,
        bool isStructDetected);
};


}