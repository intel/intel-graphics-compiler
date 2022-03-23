/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

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

    static std::unique_ptr<IGC::VISAModule>
        BuildNew(CShader* S, llvm::Function *F, bool IsPrimary = false);

    unsigned int getUnpaddedProgramSize() const override {
        return m_pShader->ProgramOutput()->m_unpaddedProgramSize;
    }
    bool isLineTableOnly() const override {
      return isLineTableOnly(m_pShader);
    }
    unsigned getPrivateBaseReg() const override;

    unsigned getGRFSizeInBytes() const override {
      return m_pShader->getGRFSize();
    }

    unsigned getNumGRFs() const override {
        return m_pShader->ProgramOutput()->m_numGRFTotal;
    }

    unsigned getPointerSize() const override;
    uint64_t getTypeSizeInBits(Type*) const override;

    llvm::ArrayRef<char> getGenDebug() const override {
      const auto& PO = *m_pShader->ProgramOutput();
      return llvm::ArrayRef<char>((const char*)PO.m_debugDataGenISA, PO.m_debugDataGenISASize);
    }
    llvm::ArrayRef<char> getGenBinary() const override {
      const auto& PO = *m_pShader->ProgramOutput();
      return llvm::ArrayRef<char>((const char*)PO.m_programBin, PO.m_programSize);
    }
    VISAVariableLocation GetVariableLocation(const llvm::Instruction* pInst) const override;

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

    llvm::StringRef GetVISAFuncName() const override;

    void setPrivateBase(void* V) override
    {
        privateBase = (IGC::CVariable*)V;
    }

    void* getPrivateBase() const override { return privateBase; }

private:
    /// @brief Constructor.
    /// @param m_pShader holds the Shader object that provides information
    /// about the properties of the compiled program.
    /// @param TheFunction holds currently processed llvm IR function
    /// used to emit vISA code.
    /// @param IsPrimary indicates if the currently processed function
    /// is a "primary entry point" to the corresponding Shader object.
    /// See Source/IGC/DebugInfo/VISAModule.hpp for more details
    ScalarVisaModule(CShader* TheShader, llvm::Function *TheFunction, bool IsPrimary);
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

