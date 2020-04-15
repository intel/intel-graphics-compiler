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

// The entry of this array must match to its corresponding Attributes::ID
Attributes::SAttrInfo Attributes::AttrsInfo[Attributes::ATTR_TOTAL_NUM] =
{
  /* Attribute Enum */             /* Attribute Name */       /* Default value */

  /////////////////////////////////////
  /////      Kernel Attributes     ////
  /////////////////////////////////////
  /* ATTR_Target */                { "Target",                VISA_CM },
  /* ATTR_SLMSize */               { "SLMSIZE",               0 },
  /* ATTR_SurfaceUsage */          { "SurfaceUsage",          0 },
  /* ATTR_SpillMemOffset */        { "SpillMemOffset",        0 },
  /* ATTR_ArgSize */               { "ArgSize",               0 },
  /* ATTR_RetValSize */            { "RetValSize",            0 },
  /* ATTR_FESPSize */              { "FESPSize",              0 },
  /* ATTR_PerThreadInputSize */    { "PerThreadInputSize",    0 },
  /* ATTR_Extern */                { "Extern",                0 },
  /* ATTR_NoBarrier */             { "NoBarrier",             0 },
  /* ATTR_AsmName */               { "AsmName",               (const char*)0 },
  /* ATTR_Entry */                 { "Entry",                 (const char*)0 },
  /* ATTR_Callable */              { "Callable",              (const char*)0 },
  /* ATTR_Caller */                { "Caller",                (const char*)0 },
  /* ATTR_Composable */            { "Composable",            (const char*)0 },

  /////////////////////////////////////////
  /////    int non-Kernel Attributes   ////
  /////////////////////////////////////////
  /* ATTR_Input */                 { "Input",                 -1 },
  /* ATTR_Output */                { "Output",                -1 },
  /* ATTR_Scope */                 { "Scope",                 -1 },
  /* ATTR_Input_Output */          { "Input_Output",          -1 },
  /* ATTR_NoWidening */            { "NoWidening",            -1 },

  ////////////////////////////////////////////
  /////    string non-Kernel Attributes   ////
  ////////////////////////////////////////////
};

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
        if (!strcmp(AttrName, AttrsInfo[i].m_attrName) &&
            AttrLen == strlen(AttrsInfo[i].m_attrName))
        {
            return (ID)i;
        }
    }

    // temporary. Once upstread change them, remove this code.
    if (AttrLen == 13 && !strcmp(AttrName, "OutputAsmPath"))
    {
        return ATTR_AsmName;
    }
    if (AttrLen == 18 && !strcmp(AttrName, "perThreadInputSize"))
    {   // start with a lower case 'p'
        return ATTR_PerThreadInputSize;
    }
    return ATTR_INVALID;
}

void Attributes::setIntKernelAttribute(Attributes::ID kID, int val)
{
    m_kernelAttrs[kID].m_val.m_intVal = val;
    m_kernelAttrs[kID].m_isSet = true;
}

void Attributes::setStringKernelAttribute(Attributes::ID kID, const char* val)
{
    m_kernelAttrs[kID].m_val.m_stringVal = val;
    m_kernelAttrs[kID].m_isSet = true;
}