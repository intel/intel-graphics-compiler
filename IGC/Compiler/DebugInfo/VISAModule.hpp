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

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/DataTypes.h"
#include "common/LLVMWarningsPop.hpp"

#include <vector>
#include <map>
#include <string>
#include <unordered_set>
#include "Compiler/DebugInfo/LexicalScopes.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace llvm
{
class Module;
class Function;
class Instruction;
class LLVMContext;
class DISubprogram;

class Value;
class Argument;
class Constant;
class MDNode;
}

namespace IGC
{
class CShader;
class CVariable;

/// @brief VISAVariableLocation holds information on the source variable
///        location with respect to the VISA virtual machine.
///        Also holds attribute to state whether the variable was
///        vectorized or is uniform.
struct VISAVariableLocation
{
public:
    /// @brief Default Constructor. Creates empty location.
    VISAVariableLocation()
    {
        Reset();
    }

    /// @brief Constructor. Creates constant value location.
    /// @param pConstVal constant value.
    VISAVariableLocation(const llvm::Constant* pConstVal)
    {
        Reset();
        m_isImmediate = true;
        m_pConstVal = pConstVal;
    }

    /// @brief Constructor. Creates surface entry location.
    /// @param surfaceReg register number that indicates the surface entry.
    VISAVariableLocation(unsigned int surfaceReg)
    {
        Reset();
        m_hasSurface = true;
        m_surfaceReg = surfaceReg;
    }

    /// @brief Constructor. Creates surface entry + offset location.
    /// @param surfaceReg register number that indicates the surface entry.
    /// @param locationValue value indicates the offset of the location.
    /// @param isRegister true if offset value is a register, false if it is immediate.
    /// @param isInMemory true if location is stored in memory, false otherwise.
    /// @param isVectorized true if the underlying virtual variable has been vectorized during codegen.
    VISAVariableLocation(unsigned int surfaceReg, unsigned int locationValue, bool isRegister, bool isInMemory, bool isVectorized)
    {
        Reset();
        m_hasSurface = true;
        m_surfaceReg = surfaceReg;
        m_hasLocation = true;
        m_isInMemory = isInMemory;
        m_isRegister = isRegister;
        m_locationReg = isRegister ? locationValue : 0;
        m_locationOffset = isRegister ? 0 : locationValue;
        m_IsVectorized = isVectorized;
    }

    /// @brief Constructor. Creates address/register location.
    /// @param locationValue value indicates the address/register of the location.
    /// @param isRegister true if offset value is a register, false if it is immediate.
    /// @param isInMemory true if location is stored in memory, false otherwise.
    /// @param isVectorized true if the underlying virtual variable has been vectorized during codegen.
    /// @param isGlobalAddrSpace true if variable represents a src var belonging to global address space.
    VISAVariableLocation(unsigned int locationValue, bool isRegister, 
        bool isInMemory, bool isVectorized, bool isGlobalAddrSpace)
    {
        Reset();
        m_hasLocation = true;
        m_isInMemory = isInMemory;
        m_isRegister = isRegister;
        m_locationReg = isRegister ? locationValue : 0;
        m_locationOffset = isRegister ? 0 : locationValue;
        m_IsVectorized = isVectorized;
        m_isGlobalAddrSpace = isGlobalAddrSpace;
    }

    // Getter methods
    bool IsImmediate() { return m_isImmediate; }
    bool HasSurface() { return m_hasSurface; }
    bool HasLocation() { return m_hasLocation; }
    bool IsInMemory() { return m_isInMemory; }
    bool IsRegister() { return m_isRegister; }
    bool IsVectorized() { return m_IsVectorized; }
    bool IsInGlobalAddrSpace() { return m_isGlobalAddrSpace; }


    const llvm::Constant* GetImmediate() { return m_pConstVal; }
    unsigned int GetSurface() { return m_surfaceReg; }
    unsigned int GetRegister() { return m_locationReg; }
    unsigned int GetOffset() { return m_locationOffset; }

private:
    /// @brief Initialize all class members to default (empty location).
    void Reset()
    {
        m_isImmediate = false;
        m_hasSurface = false;
        m_hasLocation = false;
        m_isInMemory = false;
        m_isRegister = false;
        m_pConstVal = nullptr;
        m_surfaceReg = (~0);
        m_locationReg = (~0);
        m_locationOffset = (~0);
        m_IsVectorized = false;
        m_isGlobalAddrSpace = false;
    }

private:
    bool m_isImmediate;
    bool m_hasSurface;
    bool m_hasLocation;
    bool m_isInMemory;
    bool m_isRegister;
    bool m_IsVectorized;
    bool m_isGlobalAddrSpace;

    const llvm::Constant* m_pConstVal;
    unsigned int m_surfaceReg;
    unsigned int m_locationReg;
    unsigned int m_locationOffset;

};

const unsigned int INVALID_SIZE = (~0);

class InstructionInfo
{
public:
    InstructionInfo() : m_size(INVALID_SIZE), m_offset(0) {}
    InstructionInfo(unsigned int size, unsigned int offset) : m_size(size), m_offset(offset) {}
    unsigned int m_size;
    unsigned int m_offset;
};
typedef std::map<const llvm::Instruction*, InstructionInfo> InstInfoMap;

/// @brief VISAModule holds information on LLVM entry point function
///        and have access to emitted VISA code.
class VISAModule
{
public:
    typedef std::vector<const llvm::Instruction*> InstList;
    typedef InstList::iterator iterator;
    typedef InstList::const_iterator const_iterator;
    typedef std::vector<unsigned char> DataVector;
public:
	/// @brief Constructor.
    /// @param m_pShader holds the processed entry point function and generated VISA code.
    explicit VISAModule(CShader* m_pShader);

    /// @brief Destructor.
    ~VISAModule();

    /// @brief Return first instruction to process.
    /// @return iterator to first instruction in the entry point function.
    const_iterator begin() const;

    /// @brief Return after last instruction to process.
    /// @return iterator to after last instruction in the entry point function.
    const_iterator end() const;

    /// @brief Process instruction before emitting its VISA code.
    /// @param Instruction to process.
    void BeginInstruction(llvm::Instruction *);

    /// @brief Process instruction after emitting its VISA code.
    /// @param Instruction to process.
    void EndInstruction(llvm::Instruction *);

    /// @brief Mark begin of VISA code emitting section.
    void BeginEncodingMark();

    /// @brief Mark end of VISA code emitting section.
    void EndEncodingMark();

    /// @brief Return VISA offset (in instructions) mapped to given instruction.
    /// @param Instruction to query.
    /// @return VISA offset (in instructions)
    unsigned int GetVisaOffset(const llvm::Instruction *) const;

    /// @brief Return VISA code size (in instructions) generated by given instruction.
    /// @param Instruction to query.
    /// @return VISA code size (in instructions)
    unsigned int GetVisaSize(const llvm::Instruction *) const;

    /// @brief Return given function unique number.
    /// @param Function to query.
    /// @return unique number for given function
    unsigned GetFunctionNumber(const llvm::Function*);
    unsigned GetFunctionNumber(const char* name);

    /// @brief Return true if given instruction represents a debug info intrinsic.
    /// @param Instruction to query.
    /// @return true if given instruction is a debug info intrinsic, false otherwise.
    bool IsDebugValue(const llvm::Instruction*) const;

    /// @brief Return debug info variable from given debug info instruction.
    /// @param Instruction to query.
    /// @return debug info variable.
    const llvm::MDNode* GetDebugVariable(const llvm::Instruction*) const;

    /// @brief Return variable location in VISA for from given debug info instruction.
    /// @param Instruction to query.
    /// @return variable location in VISA.
    VISAVariableLocation GetVariableLocation(const llvm::Instruction*) const;

    /// @brief Return raw data of given LLVM constant value.
    /// @param pConstVal constant value to process.
    /// @param rawData output buffer to append processed raw data to.
    void GetConstantData(const llvm::Constant *pConstVal, DataVector& rawData) const;

    /// @brief Return LLVM module.
    /// @return LLVM module.
    const llvm::Module* GetModule() const;

    /// @brief Return entry point LLVM function.
    /// @return LLVM function.
    const llvm::Function* GetEntryFunction() const;

    /// @brief Return LLVM context.
    /// @return LLVM context.
    const llvm::LLVMContext& GetContext() const;

    /// @brief Return data layout string.
    /// @return data layout string.
    const std::string GetDataLayout() const;

    /// @brief Return target triple string.
    /// @return target triple string.
    const std::string& GetTargetTriple() const;

	/// @brief Return SIMD size of kernel
	uint16_t GetSIMDSize() const;

    void SetEntryFunction(llvm::Function* F, bool c)
    { 
        m_pEntryFunc = F;  
        isCloned = c;
    }
    void Reset();

    std::string m_triple = "vISA_64";

    CShader* m_pShader;

    void setDISPToFuncMap(std::map<llvm::DISubprogram*, const llvm::Function*>* d)
    {
        DISPToFunc = d;
    }

    bool isDirectElfInput = false;
    // Store first VISA index->llvm::Instruction mapping
    std::map<unsigned int, const llvm::Instruction*> VISAIndexToInst;
    // Store VISA index->[header VISA index, #VISA instructions] corresponding 
    // to same llvm::Instruction. If llvm inst A generates VISA 3,4,5 then
    // this structure will have 3 entries:
    // 3 -> [3,3]
    // 4 -> [3,3]
    // 5 -> [3,3]
    std::map<unsigned int, std::pair<unsigned int, unsigned int>> VISAIndexToSize;
    std::vector<std::pair<unsigned int, unsigned int>> GenISAToVISAIndex;
    std::map<unsigned int, std::vector<unsigned int>> VISAIndexToAllGenISAOff;
    std::map<unsigned int, unsigned int> GenISAInstSizeBytes;

    void buildDirectElfMaps();

    std::vector<std::pair<unsigned int, unsigned int>> getGenISARange(const InsnRange& Range);

    unsigned int getUnpaddedProgramSize()
    {
        return m_pShader->ProgramOutput()->m_unpaddedProgramSize;
    }

    const InstInfoMap* GetInstInfoMap() { return &m_instInfoMap; }


    VISAModule& operator=(VISAModule& other) = default;

    static VISAModule* BuildNew(CShader* s)
    {
        auto n = new VISAModule(s);

        if (n->m_pShader->GetContext()->m_DriverInfo.SupportElfFormat() ||
            IGC_GET_FLAG_VALUE(EnableOneStepElf))
        {
            n->isDirectElfInput = true;
        }

        return n;
    }

    unsigned int GetCurrentVISAId() { return m_currentVisaId; }

private:
    /// @brief Default Constructor.
    ///        Defined as private to prevent creation of default constructor.
    VISAModule();

    /// @brief Updates VISA instruction id to current instruction number.
    void UpdateVisaId();

    /// @brief Validate that VISA instruction id is updated to current instruction number.
    void ValidateVisaId();

    /// @brief Trace given value to its origin value, searching for LLVM Argument.
    /// @param pVal value to process.
    /// @param isAddress indecates if the value represents an address.
    /// @param LLVM Argument if the origin value is an argument, nullptr otherwsie.
    const llvm::Argument* GetTracedArgument(const llvm::Value *pVal, bool isAddress) const;
    const llvm::Argument* GetTracedArgument64Ops(const llvm::Value *pVal) const;

private:
    // Triple to use for debug info
    //std::string m_triple = "vISA_64";
    const llvm::Module* m_pModule;
    const llvm::Function* m_pEntryFunc;
    InstList m_instList;
    std::map<const llvm::Function*, unsigned int> FuncIDMap;
    mutable std::unordered_set<const CVariable*> m_outputVals;
    std::map<llvm::DISubprogram*, const llvm::Function*>* DISPToFunc = nullptr;

    unsigned int m_currentVisaId;

    bool isCloned;

    InstInfoMap m_instInfoMap;

    /// Constants represents VISA register encoding in DWARF
    static const unsigned int LOCAL_SURFACE_BTI = (254);
    static const unsigned int GENERAL_REGISTER_BEGIN = (0);
    static const unsigned int GENERAL_REGISTER_NUM = (65536);
    static const unsigned int SAMPLER_REGISTER_BEGIN = (73728);
    static const unsigned int SAMPLER_REGISTER_NUM = (16);
    static const unsigned int TEXTURE_REGISTER_BEGIN = (74744);
    static const unsigned int TEXTURE_REGISTER_NUM = (255);
};

} // namespace IGC