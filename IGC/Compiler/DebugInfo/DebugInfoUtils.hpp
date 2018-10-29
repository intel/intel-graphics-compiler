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

#include "llvm/Config/llvm-config.h"

#include "Compiler/DebugInfo/Version.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{

/// @brief DebugInfoUtils holds static helper functions for debug info support.
class DebugInfoUtils
{
public:
    /// @brief return true if given module contain debug info
    /// @param M The LLVM module.
    /// @return true if given module contain debug info
    static bool HasDebugInfo(llvm::Module &M)
    {
        llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu");
        return (CU_Nodes != nullptr);
    }

    /// @brief creates a new call instruction to llvm.dbg.value intrinsic with
    ///        same information as in debug info of given global variable and
    ///        with value set to new given value.
    /// @param pGlobalVar global variable to handle its debug info
    /// @param pNewVal new value to map to the source variable (in the debug info)
    /// @param pEntryPoint entry point instruction to add new instructions before.
    /// @isIndirect true iff pNewValue type is a pointer to source variable type.
    /// @return new call instruction to llvm.dbg.value intrinsic
    static llvm::Instruction* UpdateGlobalVarDebugInfo(
        llvm::GlobalVariable* pGlobalVar, llvm::Value* pNewVal,
        llvm::Instruction *pEntryPoint, bool isIndirect)
    {
        llvm::Function* userFunc = pEntryPoint->getParent()->getParent();
        llvm::Module &M = *userFunc->getParent();
        llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu");
        if (!CU_Nodes)
        {
            return nullptr;
        }

        llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;
        // Fix debug info
        llvm::MDNode* descriptor = userFunc->getSubprogram();
        if (!descriptor)
            return nullptr;

#if 0
        llvm::DICompileUnit* CUNode = userFunc->getSubprogram()->getUnit();
        for (unsigned int i = 0; i < CU_Nodes->getNumOperands(); i++)
        {
            llvm::DICompileUnit* CUNode = llvm::cast<llvm::DICompileUnit>(CU_Nodes->getOperand(i));
            llvm::DISubprogramArray SPs = CUNode->getSubprograms();
            llvm::MDNode* descriptor = nullptr;
            for (auto &F : )
            {
                llvm::DISubprogram* SP = llvm::cast<llvm::DISubprogram>(SPs[j]);
                if (SP->getName() == userFunc->getName())
                {
                    descriptor = SP;
                    break;
                }
            }
            if (descriptor == nullptr)
            {
                continue;
            }
#endif

        bool done = false;
        llvm::DILocation* locToUse = nullptr;
        for (auto bbIt = userFunc->begin();
            bbIt != userFunc->end() && !done;
            bbIt++)
        {
            for (auto instIt = bbIt->begin();
                instIt != bbIt->end();
                instIt++)
            {
                // Discover first valid Loc in function
                // and use it in dbg.declare nodes inserted
                // later. Make sure the location belongs to
                // the function and not to an inlined
                // callee.
                if (instIt->getDebugLoc() &&
                    !instIt->getDebugLoc().getInlinedAt())
                {
                    locToUse = instIt->getDebugLoc().get();
                    done = true;
                    break;
                }
            }
        }

        //auto GVs = CUNode->getGlobalVariables();
        llvm::SmallVector<llvm::DIGlobalVariableExpression *, 1> GVs;
        pGlobalVar->getDebugInfo(GVs);
        for (unsigned int j = 0; j < GVs.size(); j++)
        {
			llvm::DIGlobalVariable* GV = GVs[j]->getVariable();
            IGCLLVM::DIBuilder Builder(M);
            llvm::DIVariable *Var = Builder.createAutoVariable(llvm::cast<llvm::DIScope>(descriptor),
                GV->getDisplayName(),
                Builder.createFile(GV->getFilename(), GV->getDirectory()),
                GV->getLine(),
                GV->getType().resolve(),
                false,
                flags
                );

                if (isIndirect)
                    return Builder.insertDeclare(pNewVal, llvm::cast<llvm::DILocalVariable>(Var), nullptr, locToUse, pEntryPoint);

                return Builder.insertDbgValueIntrinsic(pNewVal, 0, llvm::cast<llvm::DILocalVariable>(Var), nullptr, locToUse, pEntryPoint);
        }
        return nullptr;
   }

    static bool IsSpecialDebugVariable(const std::string &name)
    {
        // Check for OCL special debug variable such as:
        //   "__ocl_dbg_gid0"
        //   "__ocl_dbg_gid1"
        //   "__ocl_dbg_gid2"
        // Assuming all OCL debug special variables starts with "__ocl_dbg" prefix.
        return (name.find("__ocl_dbg", 0) == 0);
    }

private:
    /// @brief Private deafault constructor to prevent creating instance of DebugInfoUtils
    DebugInfoUtils();
};

} // namespace IGC
