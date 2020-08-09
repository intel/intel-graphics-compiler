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

#ifndef _BINARYCISAEMISSION_H_
#define _BINARYCISAEMISSION_H_

#include <map>
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"

namespace vISA
{
    class CISALabelInfo
    {
    public:
        CISALabelInfo(int name_index, int label_table_index, bool kind) :m_name_index(name_index),
            m_label_table_index(label_table_index), m_kind(kind) {}

        int m_name_index;
        int m_label_table_index;
        bool m_kind;
    };

    // Define a map of labels
    class CISALabelMap : public std::map<std::string, CISALabelInfo> {};

    class CBinaryCISAEmitter{
    public:
        int Emit(VISAKernelImpl * cisa_kernel, unsigned int&);
        CBinaryCISAEmitter() {}
        ~CBinaryCISAEmitter() {
        }
    private:
        void emitVarInfo(VISAKernelImpl * cisa_kernel, var_info_t * var);
        void emitStateInfo(VISAKernelImpl * cisa_kernel, state_info_t * var);
        void emitAddressInfo(VISAKernelImpl * cisa_kernel, addr_info_t * addr);
        void emitPredicateInfo(VISAKernelImpl * cisa_kernel, pred_info_t * pred);
        void emitLabelInfo(VISAKernelImpl * cisa_kernel, label_info_t * lbl);
        void emitInputInfo(VISAKernelImpl * cisa_kernel, input_info_t * in);
        void emitAttributeInfo(VISAKernelImpl * cisa_kernel, attribute_info_t * attr);
        int emitCisaInst(VISAKernelImpl * cisa_kernel, CISA_INST * inst, const VISA_INST_Desc * desc);
        void emitVectorOpnd(VISAKernelImpl * cisa_kernel, vector_opnd * v_opnd);
        void emitRawOpnd(VISAKernelImpl * cisa_kernel, raw_opnd * v_opnd);
    };
}
#endif // _BINARYCISAEMISSION_H_
