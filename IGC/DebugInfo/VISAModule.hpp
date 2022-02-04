/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/DataTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/Debug.h"
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
    class VISAVariableLocation
    {
    public:
        // @brief Default Constructor. Creates empty location.
        VISAVariableLocation() = default;


        /// @brief Default Constructor. Creates empty location.
        VISAVariableLocation(const VISAModule* m)
        {
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates constant value location.
        /// @param pConstVal constant value.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(const llvm::Constant* pConstVal, const VISAModule* m)
        {
            m_isImmediate = true;
            m_pConstVal = pConstVal;
            m_pVISAModule = m;
        }

        /// @brief Constructor. Creates surface entry location.
        /// @param surfaceReg register number that indicates the surface entry.
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(unsigned int surfaceReg, const VISAModule* m)
        {
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

        /// @brief Constructor. Creates register location with possible region-based addressing.
        ///        VC-backend specific constructor.
        /// @param locationValue value indicates the address/register of the location.
        /// @param offsets list of offsets for each piece of location
        /// @param m points to VISAModule corresponding to this location
        VISAVariableLocation(unsigned int locationValue, llvm::SmallVector<unsigned, 0> &&offsets,
             const VISAModule* m) : m_offsets(std::move(offsets))
        {
            m_hasLocation = true;
            m_isRegister = true;
            m_locationReg = locationValue;
            m_vectorNumElements = offsets.size();
            m_pVISAModule = m;
        }

        /// @brief Copy Constructor.
        /// @param copied value.
        VISAVariableLocation(const VISAVariableLocation&) = default;

        /// @brief Move Constructor. Creates constant value location.
        /// @param copied value.
        VISAVariableLocation& operator=(const VISAVariableLocation&) = default;

        /// @brief Destructor
        ~VISAVariableLocation() {};

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
        void SetRegister(unsigned int locationReg) { m_locationReg = locationReg; }
        unsigned int GetRegister() const { return m_locationReg; }
        unsigned int GetOffset() const { return m_locationOffset; }
        unsigned int GetVectorNumElements() const { return m_vectorNumElements; }
        const VISAModule* GetVISAModule() const { IGC_ASSERT(m_pVISAModule); return m_pVISAModule; }

        bool IsSampler() const;
        bool IsTexture() const;
        bool IsSLM() const;

        void AddSecondReg(unsigned int locationValue) {
            IGC_ASSERT_MESSAGE(m_isRegister, "Second location must be filled only for regs");
            m_locationSecondReg = locationValue;
        }
        bool HasLocationSecondReg() const { return m_locationSecondReg != ~0; }
        unsigned int GetSecondReg() const { IGC_ASSERT(HasLocationSecondReg()); return m_locationSecondReg; }

        // Regon-base addressing data (vc-backend specific)
        bool isRegionBasedAddress() const { return m_offsets.size() > 0; }
        unsigned GetRegionOffset(size_t i) const { IGC_ASSERT(m_offsets.size() > i); return m_offsets[i];}
        size_t GetRegionOffsetsCount() const { return m_offsets.size(); }

        void dump() const;
        void print (llvm::raw_ostream &OS) const;

    private:

        bool m_isImmediate = false;
        bool m_hasSurface = false;
        bool m_hasLocation = false;
        bool m_isInMemory = false;
        bool m_isRegister = false;
        bool m_isVectorized = false;
        bool m_isGlobalAddrSpace = false;

        const llvm::Constant* m_pConstVal = nullptr;
        unsigned int m_surfaceReg = ~0;
        unsigned int m_locationReg = ~0;
        // In case of SIMD 32, each register is treated to be one half of SIMD16.
        // Next variable is used to save second half in SIMD32-mode(first in m_locationReg):
        unsigned int m_locationSecondReg = ~0;
        unsigned int m_locationOffset = ~0;
        unsigned int m_vectorNumElements = ~0;
        const VISAModule* m_pVISAModule = nullptr;
        // Regon-base addressing info (vc-backend specific)
        llvm::SmallVector<unsigned, 0> m_offsets;
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

    /// @brief VISAModule holds information on LLVM function from which
    ///     visa function was constructed.
    /// TODO: rename this class to something like VISAFunction/VISAObject
    class VISAModule
    {
    public:
        enum class ObjectType
        {
            UNKNOWN = 0,
            KERNEL = 1,
            STACKCALL_FUNC = 2,
            SUBROUTINE = 3
        };

        using InstList = std::vector<const llvm::Instruction*>;
        using iterator = InstList::iterator;
        using const_iterator = InstList::const_iterator;

        /// Constants represents VISA register encoding in DWARF
        static constexpr unsigned int LOCAL_SURFACE_BTI = (254);
        static constexpr unsigned int GENERAL_REGISTER_NUM = (65536);
        static constexpr unsigned int SAMPLER_REGISTER_BEGIN = (73728);
        static constexpr unsigned int SAMPLER_REGISTER_NUM = (16);
        static constexpr unsigned int TEXTURE_REGISTER_BEGIN = (74744);
        static constexpr unsigned int TEXTURE_REGISTER_NUM = (255);

        // Store VISA index->[header VISA index, #VISA instructions] corresponding
        // to same llvm::Instruction. If llvm inst A generates VISA 3,4,5 then
        // this structure will have 3 entries:
        // 3 -> [3,3]
        // 4 -> [3,3]
        // 5 -> [3,3]
        struct VisaInterval {
            unsigned VisaOffset;
            unsigned VisaInstrNum;
        };
        struct IDX_Gen2Visa {
            unsigned GenOffset;
            unsigned VisaOffset;
        };
        // Store first VISA index->llvm::Instruction mapping
        llvm::DenseMap<unsigned, const llvm::Instruction*> VISAIndexToInst;
        llvm::DenseMap<unsigned, VisaInterval> VISAIndexToSize;
        llvm::DenseMap<unsigned, unsigned> GenISAInstSizeBytes;
        std::map<unsigned, std::vector<unsigned>> VISAIndexToAllGenISAOff;
        std::vector<IDX_Gen2Visa> GenISAToVISAIndex;

    private:
        using VarInfoCache =
            std::unordered_map<unsigned, const DbgDecoder::VarInfo*>;

        std::string m_triple = "vISA_64";
        bool IsPrimaryFunc = false;
        // m_Func points to llvm::Function that resulted in this VISAModule instance.
        // There is a 1:1 mapping between the two.
        // Its value is setup in DebugInfo pass, prior to it this is undefined.
        llvm::Function* m_Func = nullptr;
        InstList m_instList;

        unsigned int m_currentVisaId = 0;
        unsigned int m_catchAllVisaId = 0;

        InstInfoMap m_instInfoMap;

        ObjectType m_objectType = ObjectType::UNKNOWN;

        std::unique_ptr<VarInfoCache> VICache = std::make_unique<VarInfoCache>();

    public:
        /// @brief Constructor.
        /// @param AssociatedFunc holds llvm IR function associated with
        /// this vISA object
        /// @param IsPrimary indicates if the associated IR function can be
        /// classified as a "primary entry point"
        /// "Primary entry point" is a function that spawns a separate
        /// gen object (compiled gen isa code). Currently, such functions
        /// correspond to kernel functions or indirectly called functions.
        VISAModule(llvm::Function* AssociatedFunc, bool IsPrimary)
            : m_Func(AssociatedFunc), IsPrimaryFunc(IsPrimary) {}

        virtual ~VISAModule() {}

        /// @brief true if the underlying function correspond to the
        /// "primary entry point".
        bool isPrimaryFunc() const { return IsPrimaryFunc; }

        /// @brief Return first instruction to process.
        /// @return iterator to first instruction in the entry point function.
        const_iterator begin() const { return m_instList.begin(); }

        /// @brief Return after last instruction to process.
        /// @return iterator to after last instruction in the entry point function.
        const_iterator end() const { return m_instList.end(); }

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
        /// @param Instruction to query.
        /// @return variable location in VISA.
        virtual VISAVariableLocation GetVariableLocation(const llvm::Instruction* pInst) const = 0;

        /// @brief Updates VISA instruction id to current instruction number.
        virtual void UpdateVisaId() = 0;

        /// @brief Validate that VISA instruction id is updated to current instruction number.
        virtual void ValidateVisaId() = 0;

        /// @brief Return SIMD size of kernel
        virtual uint16_t GetSIMDSize() const = 0;

        /// @brief Return whether llvm instruction is the catch all intrinsic
        virtual bool IsCatchAllIntrinsic(const llvm::Instruction* pInst) const
        {
            return false;
        }

        ///  @brief return false if inst is a placeholder instruction
        bool IsExecutableInst(const llvm::Instruction& inst);
        const DbgDecoder::VarInfo* getVarInfo(const IGC::DbgDecoder& VD, unsigned int vreg) const;

        bool hasOrIsStackCall(const IGC::DbgDecoder& VD) const;
        const std::vector<DbgDecoder::SubroutineInfo>* getSubroutines(const IGC::DbgDecoder& VD) const;

        void buildDirectElfMaps(const IGC::DbgDecoder& VD);

        std::vector<std::pair<unsigned int, unsigned int>> getGenISARange(const InsnRange& Range);

        // Given %ip range and variable location, returns vector of locations where variable is
        // available in memory due to caller save sequence.
        // Return format is:
        // <start %ip of caller save, end %ip of caller save, stack slot offset for caller save>
        std::vector<std::tuple<uint64_t, uint64_t, unsigned int>> getAllCallerSave(
            const IGC::DbgDecoder& VD, uint64_t startRange, uint64_t endRange,
            DbgDecoder::LiveIntervalsVISA& Locs);

        virtual const DbgDecoder::DbgInfoFormat*
            getCompileUnit(const IGC::DbgDecoder& VD) const;

        virtual unsigned getUnpaddedProgramSize() const = 0;
        virtual bool isLineTableOnly() const = 0;
        virtual unsigned getPrivateBaseReg() const = 0;
        unsigned getGRFSizeInBits() const { return getGRFSizeInBytes() * 8; }
        virtual unsigned getGRFSizeInBytes() const = 0;
        virtual unsigned getNumGRFs() const = 0;
        // TODO: deprecate usage of this method, since it does not respect ASI
        virtual unsigned getPointerSize() const = 0;
        virtual uint64_t getTypeSizeInBits(llvm::Type*) const = 0;

        virtual void* getPrivateBase() const = 0;
        virtual void setPrivateBase(void*) = 0;
        virtual bool hasPTO() const = 0;
        virtual int getPTOReg() const = 0;
        virtual int getFPReg() const = 0;
        virtual uint64_t getFPOffset() const = 0;

        virtual llvm::ArrayRef<char> getGenDebug() const = 0;
        virtual llvm::ArrayRef<char> getGenBinary() const = 0;

        virtual bool IsIntelSymbolTableVoidProgram() const { return false; }

        virtual llvm::StringRef GetVISAFuncName(llvm::StringRef OldName) const { return OldName; }

        const InstInfoMap* GetInstInfoMap() { return &m_instInfoMap; }

        VISAModule& operator=(VISAModule& other) = delete;

        unsigned int GetCurrentVISAId() { return m_currentVisaId; }
        void SetVISAId(unsigned ID) { m_currentVisaId = ID; }

        ObjectType GetType() const { return m_objectType; }
        void SetType(ObjectType t) { m_objectType = t; }

        // This function coalesces GenISARange which is a vector of <start ip, end ip>
        static void coalesceRanges(std::vector<std::pair<unsigned int, unsigned int>>& GenISARange);

        llvm::Function* getFunction() const  { return m_Func; }
        uint64_t GetFuncId() const { return (uint64_t)m_Func; }

        void dump() const { print(llvm::dbgs()); }
        void print (llvm::raw_ostream &OS) const;
    };
} // namespace IGC
