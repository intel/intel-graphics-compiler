/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

using namespace llvm;
using namespace std;

namespace IGC
{
    class DbgDecoder;
    class CVariable;
    class VISAModule;
    class CShader;
    class CVariable;
    class IDebugEmitter;

    class DebugInfoData
    {
    public:
        llvm::DenseMap<llvm::Function*, VISAModule*> m_VISAModules;
        // Store mapping of llvm::Value->CVariable per llvm::Function.
        // The mapping is obtained from CShader at end of EmitVISAPass for F.
        llvm::DenseMap<const llvm::Function*, llvm::DenseMap<llvm::Value*, CVariable*>> m_FunctionSymbols;
        CShader* m_pShader = nullptr;
        IDebugEmitter* m_pDebugEmitter = nullptr;

        // Detect instructions with an address class pattern. Then remove all opcodes of this pattern from
        // this instruction's last operand (metadata of DIExpression).
        static void extractAddressClass(llvm::Function& F, CShader* pShader, IDebugEmitter* pDebugEmitter);

        static void markOutputPrivateBase(CShader* pShader, IDebugEmitter* pDebugEmitter);
        static void markOutputVar(CShader* pShader, IDebugEmitter* pDebugEmitter, llvm::Instruction* pInst, const char* pMetaDataName);
        static void markOutput(llvm::Function& F, CShader* pShader, IDebugEmitter* pDebugEmitter);

        void markOutputVars(const llvm::Instruction* pInst);
        void markOutput(llvm::Function& F, CShader* m_currShader);

        void addVISAModule(llvm::Function* F, VISAModule* m)
        {
            IGC_ASSERT_MESSAGE(m_VISAModules.find(F) == m_VISAModules.end(), "Reinserting VISA module for function");

            m_VISAModules.insert(std::make_pair(F, m));
        }

        static bool hasDebugInfo(CShader* pShader);

        void transferMappings(const llvm::Function& F);
        CVariable* getMapping(const llvm::Function& F, const llvm::Value* V);

        unsigned int getVISADclId(const CVariable* CVar, unsigned index)
        {
            auto it = CVarToVISADclId.find(CVar);
            if (it == CVarToVISADclId.end())
            {
                IGC_ASSERT_MESSAGE(false, "Didnt find VISA dcl id");
                return 0;
            }
            if (index == 0)
                return (*it).second.first;
            else if (index == 1)
                return (*it).second.second;
            return 0;
        }

    private:
        std::unordered_set<const CVariable*> m_outputVals;
        llvm::DenseMap<const CVariable*, std::pair<unsigned int, unsigned int>> CVarToVISADclId;
    };
};
