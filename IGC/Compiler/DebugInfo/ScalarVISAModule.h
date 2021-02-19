#ifndef SCALAR_VISAMODULE_HPP_1YOTGOUE
#define SCALAR_VISAMODULE_HPP_1YOTGOUE

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "common/debug/Debug.hpp"
#include "DebugInfo/VISAModule.hpp"

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
    unsigned getPrivateBaseReg() const override {
      auto pVar = m_pShader->GetPrivateBase();
      unsigned privateBaseRegNum = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(pVar->visaGenVariable[0]);
      return privateBaseRegNum;
    }
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

    CVariable* GetSymbol(const llvm::Instruction* pInst, llvm::Value* pValue) const {
        // CShader's symbols are emptied before compiling a new function.
        // Whereas debug info emission starts after compilation of all functions.
        return m_pShader->GetDebugInfoData()->getMapping(*pInst->getFunction(), pValue);
    }

    void setFramePtr(CVariable* pFP) { m_framePtr = pFP; }

    CVariable* getFramePtr() const { return m_framePtr; }

    int getDeclarationID(CVariable* pVar, bool isSecondSimd32Instruction) const {
        int varId = isSecondSimd32Instruction ? 1 : 0;
        if (isSecondSimd32Instruction) {
            if (!((GetSIMDSize() == 32 && pVar->visaGenVariable[1] && !pVar->IsUniform()))) {
                return -1; // Cannot get 2nd variable in SIMD32 (?) mode
            }
        }
        return m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(pVar->visaGenVariable[varId]);
    }

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

private:
    /// @brief Constructor.
    /// @param m_pShader holds the processed entry point function and generated VISA code.
    explicit ScalarVisaModule (CShader* TheShader, Function *TheFunction);
    /// @brief Trace given value to its origin value, searching for LLVM Argument.
    /// @param pVal value to process.
    /// @param isAddress indecates if the value represents an address.
    /// @param LLVM Argument if the origin value is an argument, nullptr otherwsie.
    const llvm::Argument* GetTracedArgument(const llvm::Value* pVal, bool isAddress) const;
    const llvm::Argument* GetTracedArgument64Ops(const llvm::Value* pVal) const;

    CShader* m_pShader;
    CVariable* m_framePtr = nullptr;
    llvm::Instruction* m_perThreadOffset = nullptr;
};

}

#endif /* end of include guard: SCALAR_VISAMODULE_HPP_1YOTGOUE */
