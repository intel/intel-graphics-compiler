/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <cstdint>
#include <string>
#include <sstream>

namespace IGC {
#if defined(_DEBUG) || defined(_INTERNAL)
#define IGC_MAP_LLVM_NAMES_TO_VISA
#endif

    // A data type to track a variable's LLVM symbol name conditionally.
    // The intent is zero overhead on release builds, but otherwise
    // enable name tracking in release-internal or debug builds.
    class CName {
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
        // NOTE: if CVariable/CName's exist past the length of the underlying
        //   LLVM, then we should use a std::string or something else.
        std::string value;
#else
        // to give struct non-zero size; use a different name for this
        // than 'value' so someone doesn't accidentially reference it.
        char dummy_value;
#endif
    public:
        // Constructor to tell vISA to create the name
        // Prefer the value CName::NONE.
        CName() : CName(nullptr) { }

        CName(const CName &) = default;

        CName(const std::string &arg)
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            : value(arg)
#endif
        { }

        CName(const llvm::StringRef &arg)
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            : value(arg.str())
#endif
        { }

        // explicit: some compilers are very liberal about what can be a
        // const char *; make sure the user really means it
        CName(int) = delete;
        CName(bool) = delete;

        CName(const char *arg)
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            : value(arg == nullptr ? "" : arg)
#endif
        { }

        // For split variables
        // For example, an LLVM instruction split for 64b into two halves
        // might use the names:
        //   ... CName(llvmValue->getName(), "Lo32")
        //   ... CName(llvmValue->getName(), "Hi32")
        CName(const CName &prefix, const char *suffix)
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            : CName(prefix.value + suffix)
#endif
        {
        }

        // For instance variables:
        // e.g. for (i = 0; ...) ... CName("inputVar", i)
        CName(const CName &prefix, int suffix)
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
        {
            std::stringstream ss;
            ss << prefix.value << "_" << suffix;
            value = ss.str();
        }
#else
        {
        }
#endif

        const char *getVisaCString() const {
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            return value.empty() ? nullptr : value.c_str();
#else
            return nullptr; // tells vISA to allocate the name automatically
#endif
        }

        // like above but returns empty string if LLVM names are disabled
        const char *getCString() const {
            return getVisaCString() != nullptr ? getVisaCString() : "";
        }

        bool empty() const {return size() == 0;}

        size_t size() const {
#ifdef IGC_MAP_LLVM_NAMES_TO_VISA
            return value.size();
#else
            return 0;
#endif
        }

        // A constant used to represent a name without a special name
        // Use this value when you want vISA to allocate an automatic name.
        static const CName NONE;
    };

    // As uniform is splitted into multiple ones, CVariable ctor's boolean uniform
    // arg need changing. To avoid large changes, UniformArgWrap is used to allow
    // both bool (existing one) or the newer WIBaseClass::WIDependancy to pass
    // into CVariable's ctor.  When boolean is no longer passed in, this wrap class
    // should be removed.
    class UniformArgWrap {
    public:
        WIBaseClass::WIDependancy m_dep;

        UniformArgWrap() : m_dep(WIBaseClass::RANDOM) {}
        UniformArgWrap(WIBaseClass::WIDependancy d) : m_dep(d) {}
        UniformArgWrap(bool b) : m_dep(b ? WIBaseClass::UNIFORM_THREAD : WIBaseClass::RANDOM) {}
    };

    ///-----------------------------------------------------------------------------
    /// CVariable
    ///-----------------------------------------------------------------------------
    class CVariable {
        // immediate or undef value
        CVariable(uint64_t immediate, VISA_Type type, uint16_t nbElem, bool undef);
    public:
#if defined(_DEBUG) || defined(_INTERNAL)
        void* operator new(size_t size, llvm::SpecificBumpPtrAllocator<CVariable> &Allocator)
        {
            return Allocator.Allocate(size / sizeof(CVariable));
        }

        void operator delete(void*, llvm::SpecificBumpPtrAllocator<CVariable>&) {}
#endif
        // immediate variable
        CVariable(uint64_t immediate, VISA_Type type);

        // undef variable
        // these are represented as immediate but can considered as trash data
        CVariable(VISA_Type type);

        // alias variable; if numElements is 0, this is an alias to the whole
        // variable, otherwise it's only to a part of it.
        CVariable(
            CVariable* var, VISA_Type type, uint16_t offset,
            uint16_t numElements, UniformArgWrap uniform);

        // general variable
        CVariable(
            uint16_t nbElement,
            UniformArgWrap uniform,
            VISA_Type type,
            e_varType varType,
            e_alignment align,
            bool vectorUniform,
            uint16_t numberInstance,
            const CName &name);

        // Copy Ctor
        CVariable(const CVariable& V) :
            m_immediateValue(V.m_immediateValue),
            m_alias(nullptr),
            m_nbElement(V.m_nbElement),
            m_aliasOffset(0),
            m_numberOfInstance(V.m_numberOfInstance),
            m_type(V.m_type),
            m_varType(V.m_varType),
            m_align(V.m_align),
            m_uniform(V.m_uniform),
            m_isImmediate(V.m_isImmediate),
            m_subspanUse(V.m_subspanUse),
            m_uniformVector(V.m_uniformVector),
            m_undef(V.m_undef),
            m_isUnpacked(V.m_isUnpacked),
            m_llvmName(V.m_llvmName)
        {
        }

        e_alignment GetAlign() const
        {
            IGC_ASSERT_MESSAGE(!m_isImmediate, "Calling GetAlign() on an immediate returns undefined result");
            return m_align;
        }
        void SetAlign(e_alignment thisAlign) { m_align = thisAlign; }

        uint16_t GetNumberElement() const { return m_nbElement; }
        bool IsUniform() const { return WIAnalysis::isDepUniform(m_uniform); }  // uniform_thread or above
        bool IsWorkGroupOrGlobalUniform() const { return IsWorkGroupUniform() || IsGlobalUniform(); }
        bool IsWorkGroupUniform() const { return m_uniform == WIBaseClass::UNIFORM_WORKGROUP; }
        bool IsGlobalUniform() const { return m_uniform == WIBaseClass::UNIFORM_GLOBAL; }

        uint GetSize() const { return m_nbElement * GetCISADataTypeSize(m_type); }
        uint GetElemSize() const { return GetCISADataTypeSize(m_type); }

        CVariable* GetAlias() const { return m_alias; }
        uint16_t GetAliasOffset() const { return m_aliasOffset; }
        VISA_Type GetType() const { return m_type; }
        e_varType GetVarType() const { return m_varType; }
        uint64_t GetImmediateValue() const
        {
            IGC_ASSERT(IsImmediate());
            return m_immediateValue;
        }
        bool IsImmediate() const
        {
            IGC_ASSERT_MESSAGE((!m_isImmediate || (m_isImmediate && IsGlobalUniform())),
                "IsImmediate => IsUniform invariant broken");
            return m_isImmediate;
        }
        bool IsVectorUniform() const { return m_uniformVector; }
        uint8_t GetNumberInstance() const { return m_numberOfInstance; }
        bool IsUndef() const { return m_undef; }
        void setisUnpacked() { m_isUnpacked = true; }
        bool isUnpacked() { return m_isUnpacked; }
        uint8_t getOffsetMultiplier() const { return m_isUnpacked ? 2 : 1; }
        void ResolveAlias();

        union {
            VISA_GenVar* visaGenVariable[2]{ nullptr, nullptr };
            VISA_SurfaceVar* visaSurfVariable;
            VISA_PredVar* visaPredVariable;
            VISA_AddrVar* visaAddrVariable;
            VISA_SamplerVar* visaSamplerVariable;
        };

        static inline e_alignment getAlignment(unsigned off)
        {
            switch (off)
            {
            case 1: return EALIGN_BYTE;
            case 2: return EALIGN_WORD;
            case 4: return EALIGN_DWORD;
            case 8: return EALIGN_QWORD;
            case 16: return EALIGN_OWORD;
            case 32: return EALIGN_HWORD;
            case 64: return EALIGN_32WORD;
            case 128: return EALIGN_64WORD;
            default:
                break;
            }

            return EALIGN_BYTE;
        }

        // Returns a best-effort LLVM name that this variable corresponds to
        // It is not necessarily unique since LLVM variables may be split
        // into multiple CVariable (as well as other ambiguities).
        // Names are only non-empty in non-release builds.
        //
        // The vISA backend will suffix variable names to keep them
        // unique.
        const CName &getName() const {
            return m_llvmName;
        }
        // This accesses the name via a const char *.  This will return
        // nullptr if names are not tracked (e.g. release build).
        const char *getVisaCString() const {
            if (IGC_IS_FLAG_ENABLED(UseVISAVarNames))
                return nullptr;
            return m_llvmName.getVisaCString();
        }

        void print(llvm::raw_ostream& OS) const;

        void dump() const;

        // ToDo: move them elsewhere?
        static constexpr const char* getVarTypeStr(e_varType varTy)
        {
            switch (varTy)
            {
                case EVARTYPE_GENERAL: return "general";
                case EVARTYPE_ADDRESS: return "address";
                case EVARTYPE_PREDICATE: return "predicate";
                case EVARTYPE_SURFACE: return "surface";
                case EVARTYPE_SAMPLER: return "sampler";
                default: return "illegal";
            }
        }
        static constexpr const char* getVISATypeStr(VISA_Type ty)
        {
            switch (ty)
            {
            case ISA_TYPE_UD: return "ud";
            case ISA_TYPE_D: return "d";
            case ISA_TYPE_UW: return "uw";
            case ISA_TYPE_W: return "w";
            case ISA_TYPE_UB: return "ub";
            case ISA_TYPE_B: return "b";
            case ISA_TYPE_DF: return "df";
            case ISA_TYPE_F: return "f";
            case ISA_TYPE_V: return "v";
            case ISA_TYPE_VF: return "vf";
            case ISA_TYPE_BOOL: return "bool";
            case ISA_TYPE_UQ: return "uq";
            case ISA_TYPE_Q: return "q";
            case ISA_TYPE_UV: return "uv";
            case ISA_TYPE_HF: return "hf";
            case ISA_TYPE_BF: return "bf";
            default: return "illegal";
            }
        }
        static uint GetCISADataTypeSize(VISA_Type type);
        static e_alignment GetCISADataTypeAlignment(VISA_Type type);

        bool isQType() const { return (m_type == ISA_TYPE_UQ || m_type == ISA_TYPE_Q); }
        VISA_Type GetDTypeFromQType() const { return (m_type == ISA_TYPE_UQ ? ISA_TYPE_UD : ISA_TYPE_D); }

    private:
        const uint64_t      m_immediateValue;

        CVariable*          m_alias;

        uint16_t            m_nbElement;
        uint16_t            m_aliasOffset;

        const uint8_t       m_numberOfInstance;
        const VISA_Type     m_type;
        const e_varType     m_varType;
        e_alignment         m_align;
        const WIBaseClass::WIDependancy  m_uniform;

        const unsigned char m_isImmediate : 1;
        const unsigned char m_subspanUse : 1;
        const unsigned char m_uniformVector : 1;
        const unsigned char m_undef : 1;

        // unpacked means the layout of the vector is stored as unpacked half
        unsigned char       m_isUnpacked : 1;

        CName       m_llvmName;
    };

} // namespace IGC
