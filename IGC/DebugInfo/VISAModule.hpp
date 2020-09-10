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
#include "llvm/IR/Function.h"
#include "llvm/ADT/APInt.h"
#include "common/LLVMWarningsPop.hpp"

#include "VISAIDebugEmitter.hpp"
#include "LexicalScopes.hpp"

#include <vector>
#include <map>
#include <string>

#include "Probe/Assertion.h"

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
    class VISAModule;
    class DwarfDebug;

    /// @brief VISAVariableLocation holds information on the source variable
    ///        location with respect to the VISA virtual machine.
    ///        Also holds attribute to state whether the variable was
    ///        vectorized or is uniform.
    struct VISAVariableLocation
    {
    public:
        /// @brief Default Constructor. Creates empty location.
        VISAVariableLocation(const VISAModule* m)
        {
            Reset();
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates constant value location.
        /// @param pConstVal constant value.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(const llvm::Constant* pConstVal, const VISAModule* m)
        {
            Reset();
            m_isImmediate = true;
            m_pConstVal = pConstVal;
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates surface entry location.
        /// @param surfaceReg register number that indicates the surface entry.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(unsigned int surfaceReg, const VISAModule* m)
        {
            Reset();
            m_hasSurface = true;
            m_surfaceReg = surfaceReg;
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates surface entry + offset location.
        /// @param surfaceReg register number that indicates the surface entry.
        /// @param locationValue value indicates the offset of the location.
        /// @param isRegister true if offset value is a register, false if it is immediate.
        /// @param isInMemory true if location is stored in memory, false otherwise.
        /// @param isVectorized true if the underlying virtual variable has been vectorized during codegen.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(unsigned int surfaceReg, unsigned int locationValue, bool isRegister,
            bool isInMemory, unsigned int vectorNumElements, bool isVectorized, const VISAModule* m)
        {
            Reset();
            m_hasSurface = true;
            m_surfaceReg = surfaceReg;
            m_hasLocation = true;
            m_isInMemory = isInMemory;
            m_isRegister = isRegister;
            m_locationReg = isRegister ? locationValue : 0;
            m_locationOffset = isRegister ? 0 : locationValue;
            m_isVectorized = isVectorized;
            m_vectorNumElements = isVectorized ? vectorNumElements : 0;
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates address/register location.
        /// @param locationValue value indicates the address/register of the location.
        /// @param isRegister true if offset value is a register, false if it is immediate.
        /// @param isInMemory true if location is stored in memory, false otherwise.
        /// @param isVectorized true if the underlying virtual variable has been vectorized during codegen.
        /// @param isGlobalAddrSpace true if variable represents a src var belonging to global address space.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(unsigned int locationValue, bool isRegister, bool isInMemory,
             unsigned int vectorNumElements, bool isVectorized, bool isGlobalAddrSpace, const VISAModule* m)
        {
            Reset();
            m_hasLocation = true;
            m_isInMemory = isInMemory;
            m_isRegister = isRegister;
            m_locationReg = isRegister ? locationValue : 0;
            m_locationOffset = isRegister ? 0 : locationValue;
            m_isVectorized = isVectorized;
            m_vectorNumElements = isVectorized ? vectorNumElements : 0;
            m_isGlobalAddrSpace = isGlobalAddrSpace;
            m_pVISAModule = m;
        }

        // Getter methods
        bool IsImmediate() const { return m_isImmediate; }
        bool HasSurface() const { return m_hasSurface; }
        bool HasLocation() const { return m_hasLocation; }
        bool IsInMemory() const { return m_isInMemory; }
        bool IsRegister() const { return m_isRegister; }
        bool IsVectorized() const { return m_isVectorized; }
        bool IsInGlobalAddrSpace() const { return m_isGlobalAddrSpace; }

        const llvm::Constant* GetImmediate() { return m_pConstVal; }
        unsigned int GetSurface() const { return m_surfaceReg; }
        unsigned int GetRegister() const { return m_locationReg; }
        unsigned int GetOffset() const { return m_locationOffset; }
        unsigned int GetVectorNumElements() const { return m_vectorNumElements; }
        const VISAModule* GetVISAModule() { return m_pVISAModule; }

        bool IsSampler() const;
        bool IsTexture() const;
        bool IsSLM() const;

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
            m_isVectorized = false;
            m_isGlobalAddrSpace = false;
            m_pVISAModule = nullptr;
        }

    private:
        bool m_isImmediate;
        bool m_hasSurface;
        bool m_hasLocation;
        bool m_isInMemory;
        bool m_isRegister;
        bool m_isVectorized;
        bool m_isGlobalAddrSpace;

        const llvm::Constant* m_pConstVal;
        unsigned int m_surfaceReg;
        unsigned int m_locationReg;
        unsigned int m_locationOffset;
        unsigned int m_vectorNumElements;
        const VISAModule* m_pVISAModule = nullptr;
    };

    typedef uint64_t GfxAddress;

    /*
    * Encoder/decoder for manipulating 64-bit GFX addresses.
    *
    *  63            56 55                   48  47            0
    *  ---------------------------------------------------------
    * | Address Space  |   Index (BTI/Sampler)  |    Offset     |
    *  ---------------------------------------------------------
    *
    *  Upper      8 bits represent the address space (flat, sampler, surface, etc).
    *  Following  8 bits represent the index (valid only for surfaces/sampler), and
    *  Following 48 bits respesent the offset within a memory region.
    *
    */
    class Address
    {
    public:
        enum class Space : GfxAddress
        {
            eContextFlat = 0,
            eContextFlat2 = 255,
            eSampler = 1,
            eSurface = 2,
            eGRF = 3,
            eSurfaceRelocated = 4,
            eMMIO = 5,
            ePhysical = 6,
            eVirtual = 7,
            ePCI = 8,
            eScratch = 9,
            eLocal = 10
        };

    private:
        static const uint32_t c_space_shift = 56;

        static const uint32_t c_index_shift = 48;
        static const uint32_t c_offset_shift = 0;

        static const uint32_t c_space_size = 8;
        static const uint32_t c_index_size = 8;
        static const uint32_t c_offset_size = 64 - c_index_size - c_space_size;

        static const GfxAddress c_offset_mask = ((1ull << c_offset_size) - 1);
        static const GfxAddress c_space_mask = ((1ull << c_space_size) - 1);
        static const GfxAddress c_index_mask = ((1ull << c_index_size) - 1);

    public:
        Address(GfxAddress addr = 0)
            : m_addr(addr)
        {
        }

        Space
            GetSpace() const
        {
            return (Space)((m_addr >> c_space_shift) & c_space_mask);
        }

        uint32_t
            GetIndex() const
        {
            return (uint32_t)((m_addr >> c_index_shift) & c_index_mask);
        }

        uint64_t
            GetOffset() const
        {
            return ((m_addr >> c_offset_shift) & c_offset_mask);
        }

        uint64_t
            GetAddress() const
        {
            return m_addr;
        }

        uint64_t
            GetCanonicalizedAddress() const
        {
            bool isSignBitSet = ((1ull << 47) & m_addr) != 0;

            return isSignBitSet ? (~c_offset_mask) | m_addr : c_offset_mask & m_addr;
        }

        void
            SetSpace(Space space)
        {
            GfxAddress s = static_cast<GfxAddress>(space);
            IGC_ASSERT(s == (s & c_space_mask));
            m_addr = (m_addr & ~(c_space_mask << c_space_shift)) | ((s & c_space_mask) << c_space_shift);
        }

        void
            SetIndex(uint32_t index)
        {
            IGC_ASSERT(index == (index & c_index_mask));
            m_addr = (m_addr & ~(c_index_mask << c_index_shift)) | ((index & c_index_mask) << c_index_shift);
        }

        void
            SetOffset(uint64_t offset)
        {
            IGC_ASSERT(offset == (offset & c_offset_mask));
            m_addr = (m_addr & ~(c_offset_mask << c_offset_shift)) | ((offset & c_offset_mask) << c_offset_shift);
        }

        void
            Set(Space space, uint32_t index, uint64_t offset)
        {
            SetSpace(space);
            SetIndex(index);
            SetOffset(offset);
        }

        void
            Set(GfxAddress address)
        {
            m_addr = address;
        }

    private:
        uint64_t m_addr;
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
        VISAModule(llvm::Function * Entry);
        /// @brief Destructor.
        virtual ~VISAModule();

        /// @brief Return first instruction to process.
        /// @return iterator to first instruction in the entry point function.
        const_iterator begin() const;

        /// @brief Return after last instruction to process.
        /// @return iterator to after last instruction in the entry point function.
        const_iterator end() const;

        /// @brief Process instruction before emitting its VISA code.
        /// @param Instruction to process.
        void BeginInstruction(llvm::Instruction*);

        /// @brief Process instruction after emitting its VISA code.
        /// @param Instruction to process.
        void EndInstruction(llvm::Instruction*);

        /// @brief Mark begin of VISA code emitting section.
        void BeginEncodingMark();

        /// @brief Mark end of VISA code emitting section.
        void EndEncodingMark();

        /// @brief Return VISA offset (in instructions) mapped to given instruction.
        /// @param Instruction to query.
        /// @return VISA offset (in instructions)
        unsigned int GetVisaOffset(const llvm::Instruction*) const;

        /// @brief Return VISA code size (in instructions) generated by given instruction.
        /// @param Instruction to query.
        /// @return VISA code size (in instructions)
        unsigned int GetVisaSize(const llvm::Instruction*) const;

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

        /// @brief Return raw data of given LLVM constant value.
        /// @param pConstVal constant value to process.
        /// @param rawData output buffer to append processed raw data to.
        void GetConstantData(const llvm::Constant* pConstVal, DataVector& rawData) const;

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

        /// @brief Return variable location in VISA for from given debug info instruction.
        /// Return type is a vector since for SIMD32 a single src variable may map to 2
        /// VISA variables. In case of SIMD32, each entry is treated to be one half of SIMD16.
        /// @param Instruction to query.
        /// @return variable location in VISA.
        virtual std::vector<VISAVariableLocation>
            GetVariableLocation(const llvm::Instruction* pInst) const = 0;

        /// @brief Updates VISA instruction id to current instruction number.
        virtual void UpdateVisaId() = 0;
        /// @brief Validate that VISA instruction id is updated to current instruction number.
        virtual void ValidateVisaId() = 0;
        /// @brief Return SIMD size of kernel
        virtual uint16_t GetSIMDSize() const = 0;


        void SetEntryFunction(llvm::Function* F, bool c)
        {
            m_pEntryFunc = F;
            isCloned = c;
        }
        void Reset();

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
        class comparer
        {
        public:
            bool operator()(const std::string& v1, const std::string& v2) const
            {
                return v1.compare(v2) < 0;
            }
        };
        std::map<std::string, DbgDecoder::VarInfo, comparer> VirToPhyMap;

        bool getVarInfo(std::string prefix, unsigned int vreg, DbgDecoder::VarInfo& var);
        bool hasOrIsStackCall() const;
        std::vector<DbgDecoder::SubroutineInfo>* getSubroutines() const;
        DbgDecoder::DbgInfoFormat* getCompileUnit() const;

        void buildDirectElfMaps();

        std::vector<std::pair<unsigned int, unsigned int>> getGenISARange(const InsnRange& Range);

        virtual unsigned getUnpaddedProgramSize() const = 0;
        virtual bool isLineTableOnly() const = 0;
        virtual unsigned getPrivateBaseReg() const = 0;
        virtual unsigned getGRFSize() const = 0;
        virtual unsigned getPointerSize() const = 0;
        virtual VISAModule* makeNew() const = 0;

        virtual llvm::ArrayRef<char> getGenDebug() const = 0;
        virtual llvm::ArrayRef<char> getGenBinary() const = 0;

        const InstInfoMap* GetInstInfoMap() { return &m_instInfoMap; }


        VISAModule& operator=(VISAModule& other) = default;


        unsigned int GetCurrentVISAId() { return m_currentVisaId; }
        void SetVISAId(unsigned ID) { m_currentVisaId = ID; }

        void SetDwarfDebug(DwarfDebug* d) { dd = d; }

        DwarfDebug* GetDwarfDebug() { return dd; }

        enum class ObjectType
        {
            UNKNOWN = 0,
            KERNEL = 1,
            STACKCALL_FUNC = 2,
            SUBROUTINE = 3
        };

        ObjectType GetType() const { return m_objectType; }
        void SetType(ObjectType t) { m_objectType = t; }

    private:
        std::string m_triple = "vISA_64";
        const llvm::Module* m_pModule = nullptr;
        // m_pEntryFunction points to llvm::Function that resulted in this VISAModule instance.
        // There is a 1:1 mapping between the two.
        // Its value is setup in DebugInfo pass, prior to it this is undefined.
        const llvm::Function* m_pEntryFunc;
        InstList m_instList;
        std::map<const llvm::Function*, unsigned int> FuncIDMap;
        std::map<llvm::DISubprogram*, const llvm::Function*>* DISPToFunc = nullptr;
        DwarfDebug* dd = nullptr;

        unsigned int m_currentVisaId = 0;

        bool isCloned = false;

        InstInfoMap m_instInfoMap;

        ObjectType m_objectType = ObjectType::UNKNOWN;

    public:
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
