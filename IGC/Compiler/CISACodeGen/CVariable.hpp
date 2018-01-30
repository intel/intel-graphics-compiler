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
#include <cstdint>

namespace IGC {

///-----------------------------------------------------------------------------
/// CVariable
///-----------------------------------------------------------------------------
class CVariable {
public:
    // immediate variable
    CVariable(uint64_t immediate, VISA_Type type);

    // alias variable. if numElements is 0, alias to the whole variable, otherwise
    // only to a part of it.
    CVariable(CVariable* var, VISA_Type type, uint16_t offset,
        uint16_t numElements, bool uniform);

    // general variable
    CVariable(uint16_t nbElement, bool uniform, VISA_Type type, e_varType varType,
        e_alignment align, bool vectorUniform, uint16_t numberInstance);

    // undef variable
    CVariable(VISA_Type type);

    e_alignment GetAlign() const
    {
        assert(!m_isImmediate && "Calling GetAlign() on an immediate returns undefined result");
        return m_align;
    }

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
        assert(IsImmediate());
        return m_immediateValue;
    }
    bool IsImmediate() const
    {
        assert((!m_isImmediate || (m_isImmediate && m_uniform)) && "IsImmediate => IsUniform invariant broken");
        return m_isImmediate;
    }
    bool IsVectorUniform() const { return m_uniformVector; }
    uint8_t GetNumberInstance() const { return m_numberOfInstance; }
    bool IsUndef() const { return m_undef; }
    bool IsGRFAligned(e_alignment requiredAlign = EALIGN_GRF) const
    {
        e_alignment align = GetAlign();
        if (requiredAlign == EALIGN_GRF)
            return align == EALIGN_GRF || align == EALIGN_2GRF;
        return align == requiredAlign;
    }

    void setisUnpacked() { m_isUnpacked = true; }
    bool isUnpacked() { return m_isUnpacked; }
    uint8_t getOffsetMultiplier() { return (m_isUnpacked) ? 2 : 1; }
    void ResolveAlias();

    // 4 bytes
    union {
        VISA_GenVar* visaGenVariable[2];
        VISA_SurfaceVar* visaSurfVariable;
        VISA_PredVar* visaPredVariable;
        VISA_AddrVar* visaAddrVariable;
        VISA_SamplerVar* visaSamplerVariable;
    };

private:
    // packing of structure fields so they better fit the alignment

    // 8 bytes
    uint64_t m_immediateValue;

    // 4 bytes - pointer
    CVariable* m_alias;

    // 2 bytes types
    uint16_t m_nbElement;
    uint16_t m_aliasOffset;

    // 1 byte types
    uint8_t m_numberOfInstance;
    VISA_Type m_type;
    e_varType m_varType;
    e_alignment m_align;

    unsigned char m_uniform : 1;
    unsigned char m_isImmediate : 1;
    unsigned char m_subspanUse : 1;
    unsigned char m_uniformVector : 1;
    unsigned char m_undef : 1;

    // unpacked means the layout of the vector is stored as unpacked half
    unsigned char m_isUnpacked : 1;
};

} // namespace IGC
