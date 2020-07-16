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

#include "Compiler/CISACodeGen/CISABuilder.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <cstdint>
#include <string>
#include <sstream>

namespace IGC {
    // A data type to track a variable's LLVM symbol name conditionally.
    // The intent is zero overhead on release builds, but otherwise
    // enable name tracking in release-internal or debug builds.
    class CName {
#if defined(_DEBUG) || defined(_INTERNAL)
        // NOTE: if CVariable/CName's exist past the length of the underlying
        //   LLVM, then we should use a std::string or something else.
        std::string value;
#else
        // to give struct non-zero size; use a different name for this
        // than 'value' so someone doesn't accidentially reference it.
        char dummy_value;
#endif
    public:
        CName() : CName(nullptr) { }

        CName(const CName &) = default;

        CName(const std::string &arg)
#if defined(_DEBUG) || defined(_INTERNAL)
            : value(arg)
#endif
        { }

        CName(const llvm::StringRef &arg)
#if defined(_DEBUG) || defined(_INTERNAL)
            : value(arg.str())
#endif
        { }

        // explicit: some compilers are very liberal about what can be a
        // const char *; make sure the user really means it
        CName(int) = delete;
        CName(bool) = delete;

        CName(const char *arg)
#if defined(_DEBUG) || defined(_INTERNAL)
            : value(arg == nullptr ? "" : arg)
#endif
        { }

        // For split variables
        // For example, an LLVM instruction split for 64b into two halves
        // might use the names:
        //   ... CName(llvmValue->getName(), "Lo32")
        //   ... CName(llvmValue->getName(), "Hi32")
        CName(const CName &prefix, const char *suffix)
#if defined(_DEBUG) || defined(_INTERNAL)
            : CName(prefix.value + suffix)
#endif
        {
        }

        // For instance variables:
        // e.g. for (i = 0; ...) ... CName("inputVar", i)
        CName(const CName &prefix, int suffix)
#if defined(_DEBUG) || defined(_INTERNAL)
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
#if defined(_DEBUG) || defined(_INTERNAL)
            return value.empty() ? nullptr : value.c_str();
#else
            return nullptr;
#endif
        }

        // A constant used to represent a name without a special name
        // Use this value when you want vISA to allocate an automatic name.
        static const CName NONE;
    };

    ///-----------------------------------------------------------------------------
    /// CVariable
    ///-----------------------------------------------------------------------------
    class CVariable {
        // immediate or undef value
        CVariable(uint64_t immediate, VISA_Type type, uint16_t nbElem, bool undef);
    public:
        // immediate variable
        CVariable(uint64_t immediate, VISA_Type type);

        // undef variable
        // these are represented as immediate but can considered as trash data
        CVariable(VISA_Type type);

        // alias variable; if numElements is 0, this is an alias to the whole
        // variable, otherwise it's only to a part of it.
        CVariable(
            CVariable* var, VISA_Type type, uint16_t offset,
            uint16_t numElements, bool uniform);

        // general variable
        CVariable(
            uint16_t nbElement,
            bool uniform,
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
        bool IsUniform() const { return m_uniform; }

        uint GetSize() { return m_nbElement * CEncoder::GetCISADataTypeSize(m_type); }

        uint GetElemSize() { return CEncoder::GetCISADataTypeSize(m_type); }
        CVariable* GetAlias() { return m_alias; }
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
            IGC_ASSERT_MESSAGE((!m_isImmediate || (m_isImmediate && m_uniform)), "IsImmediate => IsUniform invariant broken");
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
            VISA_GenVar* visaGenVariable[2];
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
            return m_llvmName.getVisaCString();
        }

    private:
        const uint64_t      m_immediateValue;

        CVariable*          m_alias;

        uint16_t            m_nbElement;
        uint16_t            m_aliasOffset;

        const uint8_t       m_numberOfInstance;
        const VISA_Type     m_type;
        const e_varType     m_varType;
        e_alignment         m_align;

        const unsigned char m_uniform : 1;
        const unsigned char m_isImmediate : 1;
        const unsigned char m_subspanUse : 1;
        const unsigned char m_uniformVector : 1;
        const unsigned char m_undef : 1;

        // unpacked means the layout of the vector is stored as unpacked half
        unsigned char       m_isUnpacked : 1;

        CName       m_llvmName;
    };

} // namespace IGC
