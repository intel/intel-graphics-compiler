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

#include "common.h"
#include "Attributes.hpp"

#include <string.h>

using namespace vISA;

#define  ATTR_ENTRY(S, default)  { S, sizeof(S) - 1, default }

// The entry of this array must match to its corresponding attribute enum !
Attributes::SAttrInfo Attributes::AttrsInfo[Attributes::ATTR_TOTAL_NUM] =
{
  /* ATTR_ENTRY(AttrName, DefaultValue) */       /* Attribute Enum */

  /////////////////////////////////////
  /////      Kernel Attributes     ////
  /////////////////////////////////////
  ATTR_ENTRY("Target",  VISA_CM) ,               /* ATTR_Target */
  ATTR_ENTRY("SLMSIZE", 0 ) ,                    /* ATTR_SLMSize */
  ATTR_ENTRY("SpillMemOffset",  0) ,             /* ATTR_SpillMemOffset */
  ATTR_ENTRY("ArgSize", 0) ,                     /* ATTR_ArgSize */
  ATTR_ENTRY("RetValSize", 0) ,                  /* ATTR_RetValSize */
  ATTR_ENTRY("PerThreadInputSize", 0) ,          /* ATTR_PerThreadInputSize */
  ATTR_ENTRY("Extern", 0) ,                      /* ATTR_Extern */
  ATTR_ENTRY("NoBarrier", 0) ,                   /* ATTR_NoBarrier */
  ATTR_ENTRY("SimdSize", 0) ,                    /* ATTR_SimdSize */
  ATTR_ENTRY("OutputAsmPath", nullptr) ,         /* ATTR_OuputAsmPath */
  ATTR_ENTRY("Entry", nullptr) ,                 /* ATTR_Entry */
  ATTR_ENTRY("Callable", nullptr) ,              /* ATTR_Callable */
  ATTR_ENTRY("Caller", nullptr) ,                /* ATTR_Caller */
  ATTR_ENTRY("Composable", nullptr) ,            /* ATTR_Composable */

  /////////////////////////////////////////
  /////    int non-Kernel Attributes   ////
  /////////////////////////////////////////
  ATTR_ENTRY("Input", -1) ,                      /* ATTR_Input */
  ATTR_ENTRY("Output", -1) ,                     /* ATTR_Output */
  ATTR_ENTRY("Scope", -1) ,                      /* ATTR_Scope */
  ATTR_ENTRY("Input_Output", -1) ,               /* ATTR_Input_Output */
  ATTR_ENTRY("NoWidening", -1) ,                 /* ATTR_NoWidening */
  ATTR_ENTRY("SurfaceUsage", 0) ,                /* ATTR_SurfaceUsage */

  ////////////////////////////////////////////
  /////    string non-Kernel Attributes   ////
  ////////////////////////////////////////////
};

#undef ATTR_ENTRY

Attributes::Attributes()
{
    for (int i = 0; i < ATTR_NUM_KERNEL_ATTRS; ++i)
    {
        m_kernelAttrs[i].m_isSet = false;
        m_kernelAttrs[i].m_val = AttrsInfo[i].m_defaultVal;
    }
}

Attributes::ID Attributes::getAttributeID(const char* AttrName)
{
    uint32_t AttrLen = strlen(AttrName);
    for (int i = 0; i < ATTR_TOTAL_NUM; ++i)
    {
        if (AttrLen == AttrsInfo[i].m_attrNameBytes &&
            strcmp(AttrName, AttrsInfo[i].m_attrName) == 0)
        {
            return (ID)i;
        }
    }

    // temporary. Once upstream components change them, remove the code.
    if (AttrLen == 7 && !strcmp(AttrName, "AsmName"))
    {   // "AsmName" deprecated
        return ATTR_OutputAsmPath;
    }
    if (AttrLen == 18 && !strcmp(AttrName, "perThreadInputSize"))
    {   // start with a lower case 'p'
        return ATTR_PerThreadInputSize;
    }
    return ATTR_INVALID;
}

const char* Attributes::getAttributeName(Attributes::ID aID)
{
    assert(aID >= ATTR_START_INT_KERNEL_ATTR && aID < ATTR_TOTAL_NUM &&
        "vISA: Invalid attribute ID!");
    return AttrsInfo[(int)aID].m_attrName;
}

bool Attributes::isAttribute(ID aID, const char* AttrName)
{
    assert(aID >= ATTR_START_INT_KERNEL_ATTR && aID < ATTR_TOTAL_NUM &&
        "vISA: Invalid attribute ID!");
    const char* aIDName = getAttributeName(aID);
    uint32_t bytes = AttrsInfo[(int)aID].m_attrNameBytes;
    return strcmp(AttrName, aIDName) == 0 && strlen(AttrName) == bytes;
}

void Attributes::setIntKernelAttribute(Attributes::ID kID, int val)
{
    // Verify kernel attribute
    switch (kID) {
    case ATTR_SpillMemOffset :
    {
        assert((val & (GENX_GRF_REG_SIZ - 1)) == 0  &&
            "Kernel attribute: SpillMemOffset is mis-aligned!");
        break;
    }
    case ATTR_SimdSize :
    {
        // allow 0
        assert((val == 0 || val == 8 || val == 16 || val == 32) &&
            "Kernel attribute: SimdSize must be 0|8|16|32!");
        break;
    }
    default:
        break;
    }

    m_kernelAttrs[kID].m_val.m_intVal = val;
    m_kernelAttrs[kID].m_isSet = true;
}

void Attributes::setStringKernelAttribute(Attributes::ID kID, const char* val)
{
    m_kernelAttrs[kID].m_val.m_stringVal = val;
    m_kernelAttrs[kID].m_isSet = true;
}
