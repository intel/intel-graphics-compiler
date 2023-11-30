/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include <common/Types.hpp>
#include <common/allocator.h>
#include <common/Stats.hpp>

namespace IGC
{
    class CShader;
    class CodeGenContext;

    /// This class contains the information for the different SIMD version
    /// of a kernel. Each kernel in the module is associated to one CShaderProgram
    class CShaderProgram
    {
    public:
        typedef llvm::MapVector<llvm::Function*, CShaderProgram*> KernelShaderMap;
        CShaderProgram(CodeGenContext* ctx, llvm::Function* kernel);
        ~CShaderProgram();
        CShaderProgram(const CShaderProgram&) = delete;
        CShaderProgram& operator=(const CShaderProgram&) = delete;
        CShader* GetOrCreateShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
        CShader* GetShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
        void DeleteShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
        CodeGenContext* GetContext() { return m_context; }

        llvm::Function* getLLVMFunction() const { return m_kernel; }
        ShaderStats* m_shaderStats;

        // invoked to clear Func ptr when the current module is deleted (so is func within it).
        void clearBeforeRetry();

        bool hasShaderOutput(CShader* shader);

        void freeShaderOutput(CShader* shader);

        void ClearShaderPtr(SIMDMode simd);

    protected:
        CShader*& GetShaderPtr(SIMDMode simd, ShaderDispatchMode mode);
        CShader* CreateNewShader(SIMDMode simd);

        CodeGenContext* m_context;
        llvm::Function* m_kernel;
        std::array<CShader*, 8> m_SIMDshaders;

    public:
        typedef std::unique_ptr<CShaderProgram> UPtr;
    };
}