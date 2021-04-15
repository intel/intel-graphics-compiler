/*========================== begin_copyright_notice ============================

Copyright (c) 2010-2021 Intel Corporation

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

#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "DebugInfo/VISAModule.hpp"
#include "llvm/IR/Function.h"

namespace IGC {

class CShader;
class CVariable;
class CodeGenContext;

void insertOCLMissingDebugConstMetadata(CodeGenContext* ctx);

// Helper functions to analyze Debug info metadata present in LLVM IR
class DebugMetadataInfo
{
public:
    static bool hasDashGOption(CodeGenContext* ctx);
    static bool hasAnyDebugInfo(CodeGenContext* ctx, bool& fullDebugInfo, bool& lineNumbersOnly);
};

class ScalarVisaModule final : public IGC::VISAModule {

public:

    static std::unique_ptr<IGC::VISAModule> BuildNew(CShader* S, llvm::Function *F);

    unsigned int getUnpaddedProgramSize() const override {
        return m_pShader->ProgramOutput()->m_unpaddedProgramSize;
    }
    bool isLineTableOnly() const override {
      return isLineTableOnly(m_pShader);
    }
    unsigned getPrivateBaseReg() const override;

    unsigned getGRFSize() const override {
      return m_pShader->getGRFSize();
    }

    unsigned getNumGRFs() const override {
        return m_pShader->ProgramOutput()->m_numGRFTotal;
    }

    unsigned getPointerSize() const override;

    llvm::ArrayRef<char> getGenDebug() const override {
      const auto& PO = *m_pShader->ProgramOutput();
      return llvm::ArrayRef<char>((const char*)PO.m_debugDataGenISA, PO.m_debugDataGenISASize);
    }
    llvm::ArrayRef<char> getGenBinary() const override {
      const auto& PO = *m_pShader->ProgramOutput();
      return llvm::ArrayRef<char>((const char*)PO.m_programBin, PO.m_programSize);
    }
    std::vector<VISAVariableLocation>
        GetVariableLocation(const llvm::Instruction* pInst) const override;

    void UpdateVisaId() override;
    void ValidateVisaId() override;
    uint16_t GetSIMDSize() const override;

    static bool isLineTableOnly(CShader* s) {
      bool lineTableOnly = false, fullDebugInfo = false;
      DebugMetadataInfo::hasAnyDebugInfo(s->GetContext(), fullDebugInfo, lineTableOnly);

      return lineTableOnly && !fullDebugInfo;
    }

    bool IsCatchAllIntrinsic(const llvm::Instruction* pInst) const override;
    bool IsIntelSymbolTableVoidProgram() const override;

    CVariable* GetGlobalCVar(llvm::Value* pValue) {
        return m_pShader->GetGlobalCVar(pValue);
    }

    CVariable* GetSymbol(const llvm::Instruction* pInst, llvm::Value* pValue) const;

    void setFramePtr(CVariable* pFP) { m_framePtr = pFP; }

    CVariable* getFramePtr() const { return m_framePtr; }

    int getDeclarationID(CVariable* pVar, bool isSecondSimd32Instruction) const;

    void setPerThreadOffset(llvm::Instruction* perThreadOffset) {
        IGC_ASSERT_MESSAGE(perThreadOffset, "Clear perThreadOffset");
        m_perThreadOffset = perThreadOffset;
    }

    llvm::Instruction* getPerThreadOffset() const {
        return m_perThreadOffset;
    }

    bool hasPTO() const  override {
        return getPerThreadOffset() != nullptr;
    }
    uint64_t getFPOffset() const override;
    int getPTOReg() const override;
    int getFPReg() const override;

    llvm::StringRef GetVISAFuncName(llvm::StringRef OldName) const override;

    void setPrivateBase(void* V) override
    {
        privateBase = (IGC::CVariable*)V;
    }

    void* getPrivateBase() const override { return privateBase; }

private:
    /// @brief Constructor.
    /// @param m_pShader holds the processed entry point function and generated VISA code.
    explicit ScalarVisaModule (CShader* TheShader, llvm::Function *TheFunction);
    /// @brief Trace given value to its origin value, searching for LLVM Argument.
    /// @param pVal value to process.
    /// @param isAddress indecates if the value represents an address.
    /// @param LLVM Argument if the origin value is an argument, nullptr otherwsie.
    const llvm::Argument* GetTracedArgument(const llvm::Value* pVal, bool isAddress) const;
    const llvm::Argument* GetTracedArgument64Ops(const llvm::Value* pVal) const;

    CShader* m_pShader;
    CVariable* m_framePtr = nullptr;
    llvm::Instruction* m_perThreadOffset = nullptr;
    CVariable* privateBase = nullptr;
};

}

