/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "BuildIR.h" // capability check
#include "Common_ISA.h"
#include <map>
#include <string>
#include <vector>

// forward declaration
class VISAKernelImpl;
class VISAKernel_format_provider;

namespace vISA {
class IR_Builder;
}

class vISAVerifier {
  const VISAKernel_format_provider *header;
  Options *options;
  const vISA::IR_Builder *irBuilder = nullptr; // for capability check

  std::vector<std::string> kerror_list;
  std::vector<std::string> error_list;

  // true -- a label (referred to by its id) is defined in the kernel
  // false -- a label is used in the kernel but not yet defined
  std::map<int, bool> labelDefs;

public:
  vISAVerifier(const VISAKernel_format_provider *kernelHeader, Options *opt,
               const vISA::IR_Builder *ir_Builder)
      : header(kernelHeader), options(opt), irBuilder(ir_Builder) {}

  virtual ~vISAVerifier() = default;

  void run(VISAKernelImpl *kernel);

  int verifyInstruction(const CISA_INST *inst);

  bool hasErrors() const { return kerror_list.size() + error_list.size() > 0; }
  size_t getNumErrors() const { return kerror_list.size() + error_list.size(); }

  void writeReport(const char *filename);

  std::optional<std::string> getLastErrorFound() const {
    if (error_list.empty())
      return std::nullopt;
    return error_list.back();
  }

private:
  void verifyKernelHeader();

  // checks that can only be done once the whole kernel is processed.
  void finalize();

  void verifyVariableDecl(unsigned declID);
  void verifyPredicateDecl(unsigned declID);
  void verifyAddressDecl(unsigned declID);
  void verifyRegion(const CISA_INST *inst, unsigned i);
  void verifyRawOperandType(const CISA_INST *inst, const raw_opnd &opnd,
                            bool (*typeFunc)(VISA_Type));
  void verifyRawOperand(const CISA_INST *inst, unsigned i);
  void verifyVectorOperand(const CISA_INST *inst, unsigned i);
  void verifyOperand(const CISA_INST *inst, unsigned i);
  void verifyInstructionSVM(const CISA_INST *inst);
  void verifyInstructionMove(const CISA_INST *inst);
  void verifyInstructionSync(const CISA_INST *inst);
  void verifyInstructionControlFlow(const CISA_INST *inst);
  void verifyInstructionMisc(const CISA_INST *inst);
  void verifyInstructionArith(const CISA_INST *inst);
  void verifyInstructionLogic(const CISA_INST *inst);
  void verifyInstructionCompare(const CISA_INST *inst);
  void verifyInstructionAddress(const CISA_INST *inst);
  void verifyInstructionSampler(const CISA_INST *inst);
  void verifyInstructionSIMDFlow(const CISA_INST *inst);
  void verifyInstructionDataport(const CISA_INST *inst);
  void verifyKernelAttributes();

  bool checkImmediateIntegerOpnd(const vector_opnd &opnd,
                                 VISA_Type expected_type);

  // Feature-based verifier
  //     additional verification beside generic verification.
  void verifyBFMixedMode(const CISA_INST *inst);

  // Return Operand visa type. Return ISA_TYPE_NUM if unknown.
  VISA_Type getOperandVISAType(const CISA_INST *I, unsigned Ix) const;
  // Check if I's operands use the given type, if so, return true.
  bool useGivenVISAType(const CISA_INST *I, VISA_Type givenType) const;
  // If region is available, return true; otherwise, return false.
  bool getRegion(const vector_opnd &VecOpnd, uint16_t &row_offset,
                 uint16_t &col_offset, uint16_t &v_stride, uint16_t &width,
                 uint16_t &h_stride) const;

  void verifyInstructionLsc(const CISA_INST *inst);
  void verifyInstructionSrnd(const CISA_INST *inst);
};
