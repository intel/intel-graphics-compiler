/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef COMMON_ISA_FRAMEWORK
#define COMMON_ISA_FRAMEWORK

#include <cstdio>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Common_ISA.h"
#include "IsaDescription.h"
#include "IsaVerification.h"
#include "Mem_Manager.h"
#include "visa_igc_common_header.h"

#define CISA_INVALID_ADDR_ID -1
#define CISA_INVALID_PRED_ID -1
#define CISA_INVALID_VAR_ID ((unsigned)-1)
#define CISA_INVALID_SURFACE_ID -1
#define CISA_INVALID_SAMPLER_ID -1
#define INVALID_LABEL_ID -1

// reserve p0 for the case of no predication
#define COMMON_ISA_NUM_PREDEFINED_PRED 1

#if 0
#define DEBUG_PRINT_SIZE(msg, value)                                           \
  { std::cout << msg << value << "\n"; }
#define DEBUG_PRINT_SIZE_INSTRUCTION(msg, inst, value)                         \
  { std::cerr << msg << ISA_Inst_Table[inst].str << " : " << value << "\n"; }
#else
#define DEBUG_PRINT_SIZE(msg, value)
#define DEBUG_PRINT_SIZE_INSTRUCTION(msg, inst, value)
#endif

struct attr_gen_struct {
  const char *name;
  bool isInt;
  int value;
  const char *string_val;
  bool attr_set;
};

class VISAKernel;
class VISAKernelImpl;
class CISA_IR_Builder;

namespace CisaFramework {

// Wrapper for CISA_INST that also keeps track of its size in vISA binary.
class CisaInst {
public:
  CisaInst(vISA::Mem_Manager &mem) : m_mem(mem), m_size(0) {
    m_size = 1; // opcode size
  }

  virtual ~CisaInst() {}

  CISA_INST m_cisa_instruction;
  const VISA_INST_Desc* m_inst_desc = nullptr;

  int createCisaInstruction(ISA_Opcode opcode, unsigned char exec_size,
                            unsigned char modifier, PredicateOpnd pred,
                            VISA_opnd **opnd, int numOpnds,
                            const VISA_INST_Desc *inst_desc,
                            vISAVerifier *verifier = nullptr);

  int getSize() const { return m_size; }
  CISA_INST *getCISAInst() { return &m_cisa_instruction; }
  const VISA_INST_Desc *getCISAInstDesc() const { return m_inst_desc; }
  void *operator new(size_t sz, vISA::Mem_Manager &m) { return m.alloc(sz); }

private:
  vISA::Mem_Manager &m_mem;
  short m_size;
};

bool allowDump(const Options &options, const std::string &fileName);

} // namespace CisaFramework
#endif
