/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common.h"
#include "Attributes.hpp"
#include "BuildIR.h"

#include <string>
#include <utility>

using namespace vISA;

Attributes::SAttrInfo Attributes::AttrsInfo[Attributes::ATTR_TOTAL_NUM] =
{
#if 1
#define DEF_ATTR_BOOL(E, N, K, I, D)  { K, N, { AttrType::Bool,    (uint64_t) I }, D },
#define DEF_ATTR_INT32(E, N, K, I, D) { K, N, { AttrType::Int32,   (uint64_t) I }, D },
#define DEF_ATTR_INT64(E, N, K, I, D) { K, N, { AttrType::Int64,   (uint64_t) I }, D },
#define DEF_ATTR_CSTR(E, N, K, I, D)  { K, N, { AttrType::CString, (uint64_t)(size_t) I }, D },
#else
// vs2019 : designated initializer is a preview feature. Don't use it until the feature is offical.
#define DEF_ATTR_BOOL(E, N, K, I, D)  { K, N, { AttrType::Bool,    { .m_bool = I }}, D },
#define DEF_ATTR_INT32(E, N, K, I, D) { K, N, { AttrType::Int32,   { .m_i32  = I }}, D },
#define DEF_ATTR_INT64(E, N, K, I, D) { K, N, { AttrType::Int64,   { .m_i64  = I }}, D },
#define DEF_ATTR_CSTR(E, N, K, I, D)  { K, N, { AttrType::CString, { .m_cstr = I }}, D },
#endif
#include "VISAAttributes.h"
};

Attributes::Attributes()
{
    /// <summary>
    /// Initialize per-kernel attribute map m_kernelAttrs.
    /// </summary>
    for (int i = 0; i < ATTR_TOTAL_NUM; ++i)
    {
        if (AttrsInfo[i].m_attrKind == AK_KERNEL)
        {
            SAttrValue* pAV = &m_attrValueStorage[i];
            pAV->m_isSet = false;
            pAV->m_val.m_attrType = AttrsInfo[i].m_defaultVal.m_attrType;
            if (pAV->m_val.m_attrType == AttrType::Bool)
                pAV->m_val.u.m_bool = AttrsInfo[i].m_defaultVal.u.m_bool;
            else if (pAV->m_val.m_attrType == AttrType::Int32)
                pAV->m_val.u.m_i32 = AttrsInfo[i].m_defaultVal.u.m_i32;
            else if (pAV->m_val.m_attrType == AttrType::Int64)
                pAV->m_val.u.m_i64 = AttrsInfo[i].m_defaultVal.u.m_i64;
            else if (pAV->m_val.m_attrType == AttrType::CString)
                pAV->m_val.u.m_cstr = AttrsInfo[i].m_defaultVal.u.m_cstr;
            m_kernelAttrs.insert(std::make_pair(i, pAV));
        }
    }
}

Attributes::ID Attributes::getAttributeID(const char* AttrName)
{
    std::string aName(AttrName);
    for (int i=0; i < ATTR_TOTAL_NUM; ++i)
    {
        if (aName == AttrsInfo[i].m_attrName)
        {
            return (ID)i;
        }
    }

    // temporary. Once upstream components change them, remove the code.
    if (aName == "AsmName")
    {   // "AsmName" deprecated
        return ATTR_OutputAsmPath;
    }
    if (aName == "perThreadInputSize")
    {   // start with a lower case 'p'
        return ATTR_PerThreadInputSize;
    }
    if (aName == "crossThreadInputSize")
    {   // start with a lower case 'c'
        return ATTR_CrossThreadInputSize;
    }
    if (aName == "perThreadInputSize")
    {   // start with a lower case 'p'
        return ATTR_PerThreadInputSize;
    }
    return ATTR_INVALID;
}

void Attributes::setKernelAttr(ID kID, bool v)
{
    SAttrValue* pAV = getKernelAttrValue(kID);
    assert(pAV->m_val.m_attrType == AttrType::Bool);
    pAV->m_val.u.m_bool = v;
    pAV->m_isSet = true;
}
void Attributes::setKernelAttr(ID kID, int32_t v, const IR_Builder& irb)
{
    SAttrValue* pAV = getKernelAttrValue(kID);
    assert(pAV->m_val.m_attrType == AttrType::Int32);

    // Verify kernel attribute
    switch (kID) {
    case ATTR_SpillMemOffset:
    {
        assert((v & (irb.getGRFSize() - 1)) == 0 &&
            "Kernel attribute: SpillMemOffset is mis-aligned!");
        break;
    }
    case ATTR_SimdSize:
    {
        // allow 0
        assert((v == 0 || v == 8 || v == 16 || v == 32) &&
            "Kernel attribute: SimdSize must be 0|8|16|32!");
        break;
    }
    default:
        break;
    }

    pAV->m_val.u.m_i32 = v;
    pAV->m_isSet = true;
}
void Attributes::setKernelAttr(ID kID, int64_t v)
{
    SAttrValue* pAV = getKernelAttrValue(kID);
    assert(pAV->m_val.m_attrType == AttrType::Int64);
    pAV->m_val.u.m_i64 = v;
    pAV->m_isSet = true;
}
void Attributes::setKernelAttr(ID kID, const char* v)
{
    SAttrValue* pAV = getKernelAttrValue(kID);
    assert(pAV->m_val.m_attrType == AttrType::CString);
    pAV->m_val.u.m_cstr = v;
    pAV->m_isSet = true;
}
