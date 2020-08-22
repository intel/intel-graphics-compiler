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

#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

namespace vISA
{
    class Attributes
    {
    public:
        // Attribute's value
        union SAttrVal {
            int          m_intVal;
            const char* m_stringVal;

            SAttrVal() : m_intVal(0) {}
            SAttrVal(int v) : m_intVal(v) {}
            SAttrVal(const char* p) : m_stringVal(p) {}
        };

        struct SAttrInfo
        {
            const char* m_attrName;
            const uint32_t m_attrNameBytes;  // strlen(m_attrName)
            SAttrVal   m_defaultVal;
        };

        enum ID
        {
            // Keep the following order for all attributes:
            //   1. kernel (and function) attributes of int value
            //   2. kernel (and function) attributes of string value
            //   3. other non-kernel (non-function) attributes of int value (or no value)
            //   4. other non-kernel attributes of string value.

            /****************************************/
            /*     int-typed kernel attributes      */
            /****************************************/
            ATTR_START_INT_KERNEL_ATTR,
            ATTR_Target = ATTR_START_INT_KERNEL_ATTR,
            ATTR_SLMSize,
            ATTR_SpillMemOffset,       // Offset at which spill/fill starts
            ATTR_ArgSize,
            ATTR_RetValSize,
            ATTR_PerThreadInputSize,
            ATTR_Extern,
            ATTR_NoBarrier,
            ATTR_SimdSize,


            /********************************************/
            /*      string-typed kernel attributes      */
            /********************************************/
            ATTR_START_STRING_KERNEL_ATTR,
            ATTR_OutputAsmPath = ATTR_START_STRING_KERNEL_ATTR,
            ATTR_Entry,
            ATTR_Callable,
            ATTR_Caller,
            ATTR_Composable,

            /******************************************************/
            /*    int non-kernel attributes, such as variables'   */
            /******************************************************/
            ATTR_START_INT_NON_KERNEL_ATTR,
            ATTR_Input = ATTR_START_INT_NON_KERNEL_ATTR,
            ATTR_Output,
            ATTR_Scope,
            ATTR_Input_Output,
            ATTR_NoWidening,
            ATTR_SurfaceUsage,

            /********************************************************/
            /*    string non-kernel attributes, such as variables'  */
            /********************************************************/
            ATTR_START_STRING_NON_KERNEL_ATTR,

            // key enum values
            ATTR_TOTAL_NUM = ATTR_START_STRING_NON_KERNEL_ATTR,
            ATTR_NUM_KERNEL_ATTRS = ATTR_START_INT_NON_KERNEL_ATTR,

            ATTR_INVALID
        };

        struct SKernelAttrVal {
            SAttrVal m_val;
            bool     m_isSet;
        };

        /// Given an attribute name, return its ID
        static ID getAttributeID(const char* AttrName);

        /// Given an attribute ID, return its name
        static const char* getAttributeName(ID aID);

        /// Return true if the given AttrName's ID == aID
        static bool isAttribute(ID aID, const char* AttrName);

        static bool isIntKernelAttribute(ID aID)
        {
            return aID >= ATTR_START_INT_KERNEL_ATTR &&
                aID < ATTR_START_STRING_KERNEL_ATTR;
        }
        static bool isStringKernelAttribute(ID aID)
        {
            return aID >= ATTR_START_STRING_KERNEL_ATTR &&
                aID < ATTR_NUM_KERNEL_ATTRS;
        }

        static bool isIntNonKernelAttribute(ID aID)
        {
            return aID >= ATTR_START_INT_NON_KERNEL_ATTR &&
                aID < ATTR_START_STRING_NON_KERNEL_ATTR;
        }

        static bool isStringNonKernelAttribute(ID aID)
        {
            return aID >= ATTR_START_STRING_NON_KERNEL_ATTR &&
                aID < ATTR_TOTAL_NUM;
        }

        static bool isIntAttribute(ID aID)
        {
            return isIntKernelAttribute(aID) || isIntNonKernelAttribute(aID);
        }
        static bool isStringAttribute(ID aID)
        {
            return aID != ATTR_INVALID && !isIntAttribute(aID);
        }

        Attributes();

        void setIntKernelAttribute(ID kID, int val);
        int getIntKernelAttribute(ID kID) const
        {
            return m_kernelAttrs[kID].m_val.m_intVal;
        }
        void setStringKernelAttribute(ID kID, const char* val);
        const char* getStringKernelAttribute(ID kID) const
        {
            return m_kernelAttrs[kID].m_val.m_stringVal;
        }
        bool isSet(ID aID) const { return m_kernelAttrs[aID].m_isSet; }

    private:
        SKernelAttrVal m_kernelAttrs[ATTR_NUM_KERNEL_ATTRS];

        static SAttrInfo AttrsInfo[ATTR_TOTAL_NUM];
    };
};
#endif // _ATTRIBUTES_H_
