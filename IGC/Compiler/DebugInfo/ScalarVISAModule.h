#ifndef SCALAR_VISAMODULE_HPP_1YOTGOUE
#define SCALAR_VISAMODULE_HPP_1YOTGOUE

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
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
    static std::string getUniqueFuncName(llvm::Function& F);
    static std::string getOrigFuncName(std::string cloneName);
};

class ScalarVisaModule final : public IGC::VISAModule {

public:
    static VISAModule* BuildNew(CShader* s);

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

    VISAModule* makeNew() const override {
      return BuildNew(m_pShader);
    }
    void UpdateVisaId() override;
    void ValidateVisaId() override;
    uint16_t GetSIMDSize() const override;

    static bool isLineTableOnly(CShader* s) {
      bool lineTableOnly = false, fullDebugInfo = false;
      DebugMetadataInfo::hasAnyDebugInfo(s->GetContext(), fullDebugInfo, lineTableOnly);

      return lineTableOnly && !fullDebugInfo;
    }

private:
    /// @brief Constructor.
    /// @param m_pShader holds the processed entry point function and generated VISA code.
    explicit ScalarVisaModule (CShader* m_pShader);
    /// @brief Trace given value to its origin value, searching for LLVM Argument.
    /// @param pVal value to process.
    /// @param isAddress indecates if the value represents an address.
    /// @param LLVM Argument if the origin value is an argument, nullptr otherwsie.
    const llvm::Argument* GetTracedArgument(const llvm::Value* pVal, bool isAddress) const;
    const llvm::Argument* GetTracedArgument64Ops(const llvm::Value* pVal) const;

    mutable std::unordered_set<const CVariable*> m_outputVals;
    CShader* m_pShader;

};

}

#endif /* end of include guard: SCALAR_VISAMODULE_HPP_1YOTGOUE */
