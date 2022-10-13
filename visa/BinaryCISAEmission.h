/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BINARYCISAEMISSION_H_
#define _BINARYCISAEMISSION_H_

#include "Common_ISA.h"
#include "Common_ISA_framework.h"
#include "Common_ISA_util.h"
#include "visa_igc_common_header.h"
#include <map>

namespace vISA {
class CISALabelInfo {
public:
  CISALabelInfo(int name_index, int label_table_index, bool kind)
      : m_name_index(name_index), m_label_table_index(label_table_index),
        m_kind(kind) {}

  int m_name_index;
  int m_label_table_index;
  bool m_kind;
};

// Define a map of labels
class CISALabelMap : public std::map<std::string, CISALabelInfo> {};

class CBinaryCISAEmitter {
public:
  int Emit(VISAKernelImpl *cisa_kernel, unsigned int &);
  CBinaryCISAEmitter() {}
  ~CBinaryCISAEmitter() {}

private:
  void emitVarInfo(VISAKernelImpl *cisa_kernel, var_info_t *var);
  void emitStateInfo(VISAKernelImpl *cisa_kernel, state_info_t *var);
  void emitAddressInfo(VISAKernelImpl *cisa_kernel, addr_info_t *addr);
  void emitPredicateInfo(VISAKernelImpl *cisa_kernel, pred_info_t *pred);
  void emitLabelInfo(VISAKernelImpl *cisa_kernel, label_info_t *lbl);
  void emitInputInfo(VISAKernelImpl *cisa_kernel, input_info_t *in);
  void emitAttributeInfo(VISAKernelImpl *cisa_kernel, attribute_info_t *attr);
  int emitCisaInst(VISAKernelImpl *cisa_kernel, const CISA_INST *inst,
                   const VISA_INST_Desc *desc);
  void emitVectorOpnd(VISAKernelImpl *cisa_kernel, vector_opnd *v_opnd);
  void emitRawOpnd(VISAKernelImpl *cisa_kernel, raw_opnd *v_opnd);
};
} // namespace vISA
#endif // _BINARYCISAEMISSION_H_
