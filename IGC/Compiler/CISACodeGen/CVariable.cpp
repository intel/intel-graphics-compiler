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

#include "Compiler/CISACodeGen/CVariable.hpp"
#include "Probe/Assertion.h"

#include <sstream>

using namespace IGC;

const CName CName::NONE;


void CVariable::ResolveAlias()
{
    // If a variable alias another alias, make it point to the main variable
    CVariable* aliasVar = m_alias;
    IGC_ASSERT(nullptr != aliasVar);
    uint offset = m_aliasOffset;
    while (aliasVar->m_alias != nullptr)
    {
        offset += aliasVar->m_aliasOffset;
        aliasVar = aliasVar->m_alias;
    }
    m_alias = aliasVar;
    IGC_ASSERT_MESSAGE((offset < (UINT16_MAX)), "offset > higher than 64k");

    m_aliasOffset = (uint16_t)offset;
}

/// CVariable constructor, for most generic cases
///
CVariable::CVariable(
    uint16_t nbElement,
    bool uniform,
    VISA_Type type,
    e_varType varType,
    e_alignment align,
    bool vectorUniform,
    uint16_t numberOfInstance,
    const CName &name) :
    m_immediateValue(0),
    m_alias(nullptr),
    m_nbElement(nbElement),
    m_aliasOffset(0),
    m_numberOfInstance(int_cast<uint8_t>(numberOfInstance)),
    m_type(type),
    m_varType(varType),
    m_align(align),
    m_uniform(uniform),
    m_isImmediate(false),
    m_subspanUse(false),
    m_uniformVector(vectorUniform),
    m_undef(false),
    m_isUnpacked(false),
    m_llvmName(name)
{
}

static unsigned
getAlignment(e_alignment align)
{
    switch (align)
    {
    case EALIGN_BYTE:   return 1;
    case EALIGN_WORD:   return 2;
    case EALIGN_DWORD:  return 4;
    case EALIGN_QWORD:  return 8;
    case EALIGN_OWORD:  return 16;
    case EALIGN_HWORD:    return 32;
    case EALIGN_32WORD:   return 64;
    case EALIGN_64WORD:   return 128;

    default:
        break;
    }
    return 1;
}

static e_alignment
updateAlign(e_alignment align, unsigned offset)
{
    IGC_ASSERT(align != EALIGN_AUTO);
    return CVariable::getAlignment(int_cast<unsigned int>(llvm::MinAlign(getAlignment(align), offset)));
}

/// CVariable constructor, for alias
///
CVariable::CVariable(
    CVariable* var,
    VISA_Type type,
    uint16_t offset,
    uint16_t numElements,
    bool uniform) :
    m_immediateValue(0),
    m_alias(var),
    m_aliasOffset(offset),
    m_numberOfInstance(var->m_numberOfInstance),
    m_type(type),
    m_varType(EVARTYPE_GENERAL),
    m_align(updateAlign(var->m_align, offset)),
    m_uniform(uniform),
    m_isImmediate(false),
    m_subspanUse(var->m_subspanUse),
    m_uniformVector(false),
    m_undef(false),
    m_isUnpacked(false),
    m_llvmName(var->m_llvmName)
{
    if (numElements)
    {
        m_nbElement = numElements;
    }
    else
    {
        const unsigned int denominator = CEncoder::GetCISADataTypeSize(m_type);
        IGC_ASSERT(denominator);
        m_nbElement = var->m_nbElement * CEncoder::GetCISADataTypeSize(var->m_type) / denominator;
    }
    IGC_ASSERT_MESSAGE(var->m_varType == EVARTYPE_GENERAL, "only general variable can have alias");
}

/// CVariable constructor, for immediate
///
CVariable::CVariable(
    uint64_t immediate, VISA_Type type, uint16_t nbElem, bool undef) :
    m_immediateValue(immediate),
    m_alias(nullptr),
    m_nbElement(nbElem),
    m_numberOfInstance(1),
    m_type(type),
    m_varType(EVARTYPE_GENERAL),
    m_uniform(true),
    m_isImmediate(true),
    m_subspanUse(false),
    m_uniformVector(false),
    m_undef(undef),
    m_isUnpacked(false)
{
    visaGenVariable[0] = visaGenVariable[1] = nullptr;
}

CVariable::CVariable(uint64_t immediate, VISA_Type type)
    : CVariable(immediate, type, 1, false)
{
}

/// CVariable constructor, for undef
///
CVariable::CVariable(VISA_Type type)
    : CVariable(0, type, 0, true)
{
}
