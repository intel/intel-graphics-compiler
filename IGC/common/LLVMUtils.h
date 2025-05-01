/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <list>
#include "Stats.hpp"
#include <string.h>

namespace IGC
{
    namespace Debug
    {
        class Dump;
    }
    class CodeGenContext;

    class IGCPassManager : public llvm::legacy::PassManager
    {
    public:
        IGCPassManager(CodeGenContext* ctx, const char* name = "") : m_pContext(ctx), m_name(name)
        {
        }
        void add(llvm::Pass *P);
    private:
        CodeGenContext* const m_pContext;
        const std::string m_name;
        std::list<Debug::Dump> m_irDumps;

        void addPrintPass(llvm::Pass* P, bool isBefore);
        bool isPrintBefore(llvm::Pass* P);
        bool isPrintAfter(llvm::Pass* P);

        // List: comma/semicolon-separate list of names.
        // Return true if N is in the list.
        bool isInList(const llvm::StringRef& N, const llvm::StringRef& List) const;
    };
}

void DumpLLVMIR(IGC::CodeGenContext* pContext, const char* dumpName);
void DumpHashToOptions(const ShaderHash&, const ShaderType);

class InlineHelper
{
private:
    // create a map of (type, array size, address space to alloca instructions)
    using AllocaMap = llvm::MapVector<
        std::tuple<llvm::Type*, uint64_t, uint32_t>,
        llvm::SmallVector<llvm::AllocaInst*>
    >;

    llvm::DenseMap<llvm::Function*, AllocaMap> m_InlinedStaticArrayAllocas;

#if defined(_RELEASE_INTERNAL) || defined(_DEBUG)
    llvm::Function* m_calledFunction = nullptr;
#endif

public:
    InlineHelper() = default;
    ~InlineHelper() = default;

    void InlineAndOptimize(llvm::CallInst* callInst);
};
