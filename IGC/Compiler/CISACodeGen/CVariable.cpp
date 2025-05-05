/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CVariable.hpp"
#include "Probe/Assertion.h"
#include "llvm/Support/Format.h"


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
    UniformArgWrap uniform,
    VISA_Type type,
    e_varType varType,
    e_alignment align,
    bool vectorUniform,
    uint16_t numberOfInstance,
    const CName &name) :
    m_immediateValue(0),
    m_alias(nullptr),
    m_singleInstanceAlias(nullptr),
    m_nbElement(nbElement),
    m_aliasOffset(0),
    m_numberOfInstance(int_cast<uint8_t>(numberOfInstance)),
    m_type(type),
    m_varType(varType),
    m_align(align),
    m_uniform(uniform.m_dep),
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
    UniformArgWrap uniform) :
    m_immediateValue(0),
    m_alias(var),
    m_singleInstanceAlias(var->m_singleInstanceAlias),
    m_aliasOffset(offset),
    m_numberOfInstance(var->m_numberOfInstance),
    m_type(type),
    m_varType(EVARTYPE_GENERAL),
    m_align(updateAlign(var->m_align, offset)),
    m_uniform(uniform.m_dep),
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
        const unsigned int denominator = GetCISADataTypeSize(m_type);
        IGC_ASSERT(denominator);

        if (0 == denominator)
        {
            m_nbElement = 0;
        }
        else
        {
            m_nbElement = var->m_nbElement * GetCISADataTypeSize(var->m_type) / denominator;
        }
    }
    IGC_ASSERT_MESSAGE(var->m_varType == EVARTYPE_GENERAL, "only general variable can have alias");
}

/// CVariable constructor for multi-instance alias of a single-instance variable
///
CVariable::CVariable(
    CVariable* var,
    uint16_t numInstances) :
    m_immediateValue(0),
    m_alias(nullptr),
    m_singleInstanceAlias(var),
    m_aliasOffset(0),
    m_nbElement(var->m_nbElement / numInstances),
    m_numberOfInstance(numInstances),
    m_type(var->m_type),
    m_varType(EVARTYPE_GENERAL),
    m_align(var->m_align),
    m_uniform(var->m_uniform),
    m_isImmediate(false),
    m_subspanUse(var->m_subspanUse),
    m_uniformVector(false),
    m_undef(false),
    m_isUnpacked(false),
    m_llvmName(var->m_llvmName)
{
    IGC_ASSERT(var->m_varType == EVARTYPE_GENERAL);
    IGC_ASSERT(var->GetNumberInstance() == 1);
    IGC_ASSERT(var->GetSingleInstanceAlias() == nullptr);
    IGC_ASSERT(var->GetAlias() == nullptr);
    IGC_ASSERT(numInstances > 1);
    IGC_ASSERT((var->m_nbElement % numInstances) == 0);
}

/// CVariable constructor, for immediate
///
CVariable::CVariable(
    uint64_t immediate, VISA_Type type, uint16_t nbElem, bool undef) :
    m_immediateValue(immediate),
    m_alias(nullptr),
    m_singleInstanceAlias(nullptr),
    m_aliasOffset(0),
    m_nbElement(nbElem),
    m_numberOfInstance(1),
    m_type(type),
    m_varType(EVARTYPE_GENERAL),
    m_align(),
    m_uniform(WIBaseClass::UNIFORM_GLOBAL),
    m_isImmediate(true),
    m_subspanUse(false),
    m_uniformVector(false),
    m_undef(undef),
    m_isUnpacked(false),
    m_llvmName()
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

void CVariable::dump() const
{
    print(llvm::errs());
}

void CVariable::print(llvm::raw_ostream& OS) const
{
    OS << "CVariable " << getVisaCString() << " {\n";
    if (m_isImmediate)
    {
        OS << "\tKind: immediate(" << llvm::format_hex(m_immediateValue, 6) << "\n";
    }
    else
    {
        OS << "\tKind: " << CVariable::getVarTypeStr(m_varType) << "\n";
    }
    OS << "\tNumElt: " << m_nbElement << "\n";
    OS << "\tType: " << CVariable::getVISATypeStr(m_type) << "\n";
    if (m_numberOfInstance != 1)
    {
        OS << "\tNumInstance: " << m_numberOfInstance << "\n";
    }
    if (m_alias)
    {
        OS << "\tAlias: " << m_alias->getVisaCString();
        if (m_aliasOffset)
        {
            OS << "+" << m_aliasOffset;
        }
        OS << "\n";
    }
    OS << "}\n";
}

uint CVariable::GetCISADataTypeSize(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_UD:    return 4;
    case ISA_TYPE_D:     return 4;
    case ISA_TYPE_UW:    return 2;
    case ISA_TYPE_W:     return 2;
    case ISA_TYPE_UB:    return 1;
    case ISA_TYPE_B:     return 1;
    case ISA_TYPE_DF:    return 8;
    case ISA_TYPE_F:     return 4;
    case ISA_TYPE_V:     return 4;
    case ISA_TYPE_VF:    return 4;
    case ISA_TYPE_BOOL:  return 1;
    case ISA_TYPE_UV:    return 4;
    case ISA_TYPE_Q:     return 8;
    case ISA_TYPE_UQ:    return 8;
    case ISA_TYPE_HF:    return 2;
    case ISA_TYPE_BF:    return 2;
    default:
        IGC_ASSERT_MESSAGE(0, "Unimplemented CISA Data Type");
        break;
    }

    return 0;
}

e_alignment CVariable::GetCISADataTypeAlignment(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_UD:
        return EALIGN_DWORD;
    case ISA_TYPE_D:
        return EALIGN_DWORD;
    case ISA_TYPE_UW:
        return EALIGN_WORD;
    case ISA_TYPE_W:
        return EALIGN_WORD;
    case ISA_TYPE_UB:
        return EALIGN_BYTE;
    case ISA_TYPE_B:
        return EALIGN_BYTE;
    case ISA_TYPE_DF:
        return EALIGN_QWORD;
    case ISA_TYPE_F:
        return EALIGN_DWORD;
    case ISA_TYPE_V:
        return EALIGN_DWORD;
    case ISA_TYPE_VF:
        return EALIGN_DWORD;
    case ISA_TYPE_BOOL:
        return EALIGN_BYTE;
    case ISA_TYPE_UV:
        return EALIGN_BYTE;
    case ISA_TYPE_Q:
        return EALIGN_QWORD;
    case ISA_TYPE_UQ:
        return EALIGN_QWORD;
    case ISA_TYPE_HF:
        return EALIGN_WORD;
    case ISA_TYPE_BF:
        return EALIGN_WORD;
    default:
        IGC_ASSERT_MESSAGE(0, "Unimplemented CISA Data Type");
        break;
    }

    return EALIGN_BYTE;
}
