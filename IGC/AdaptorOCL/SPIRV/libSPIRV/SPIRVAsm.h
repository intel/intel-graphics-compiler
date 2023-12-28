/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// SPIRVAsm.h - This file declares SPIR-V Inline Assembler Entries --*- C++
// This file defines the inline assembler entries defined in SPIRV spec with op
// codes.

#ifndef SPIRV_LIBSPIRV_SPIRVASM_H
#define SPIRV_LIBSPIRV_SPIRVASM_H

#include "SPIRVEntry.h"
#include "SPIRVValue.h"
#include "SPIRVInstruction.h"
#include "Probe/Assertion.h"

#include <iostream>
#include <map>
#include <tuple>
#include <vector>

namespace igc_spv {

class SPIRVAsmTargetINTEL : public SPIRVEntry {
public:
  static const SPIRVWord FixedWC = 2;
  static const Op OC = OpAsmTargetINTEL;
  // Complete constructor
  SPIRVAsmTargetINTEL(SPIRVModule *M, SPIRVId TheId,
                      const std::string &TheTarget)
      : SPIRVEntry(M, FixedWC + getSizeInWords(TheTarget), OC, TheId),
        Target(TheTarget) {
    validate();
  }
  // Incomplete constructor
  SPIRVAsmTargetINTEL() : SPIRVEntry(OC) {}
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityAsmINTEL);
  }
  const std::string &getTarget() const { return Target; }

protected:
  void validate() const override {
    SPIRVEntry::validate();
    IGC_ASSERT(WordCount > FixedWC);
    IGC_ASSERT(OpCode == OC);
  }
  _SPIRV_DEF_DEC2(Id, Target)
  std::string Target;
};

class SPIRVAsmINTEL : public SPIRVValue {
public:
  static const SPIRVWord FixedWC = 5;
  static const Op OC = OpAsmINTEL;
  // Complete constructor
  SPIRVAsmINTEL(SPIRVModule *M, SPIRVTypeFunction *TheFunctionType,
                SPIRVId TheId, SPIRVAsmTargetINTEL *TheTarget,
                const std::string &TheInstructions,
                const std::string &TheConstraints)
      : SPIRVValue(M,
                   FixedWC + getSizeInWords(TheInstructions) +
                       getSizeInWords(TheConstraints),
                   OC, TheFunctionType->getReturnType(), TheId),
        Target(TheTarget), FunctionType(TheFunctionType),
        Instructions(TheInstructions), Constraints(TheConstraints) {
    validate();
  }
  // Incomplete constructor
  SPIRVAsmINTEL() : SPIRVValue(OC) {}
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityAsmINTEL);
  }
  const std::string &getInstructions() const { return Instructions; }
  const std::string &getConstraints() const { return Constraints; }
  SPIRVTypeFunction *getFunctionType() const { return FunctionType; }

protected:
  _SPIRV_DEF_DEC6(Type, Id, FunctionType, Target, Instructions, Constraints)
  void validate() const override {
    SPIRVValue::validate();
    IGC_ASSERT(WordCount > FixedWC);
    IGC_ASSERT(OpCode == OC);
  }
  SPIRVAsmTargetINTEL *Target = nullptr;
  SPIRVTypeFunction *FunctionType = nullptr;
  std::string Instructions;
  std::string Constraints;
};

class SPIRVAsmCallINTEL : public SPIRVInstruction {
public:
  static const SPIRVWord FixedWC = 4;
  static const Op OC = OpAsmCallINTEL;
  // Complete constructor
  SPIRVAsmCallINTEL(SPIRVId TheId, SPIRVAsmINTEL *TheAsm,
                    const std::vector<SPIRVWord> &TheArgs,
                    SPIRVBasicBlock *TheBB)
      : SPIRVInstruction(FixedWC + TheArgs.size(), OC, TheAsm->getType(), TheId,
                         TheBB),
        Asm(TheAsm), Args(TheArgs) {
    validate();
  }
  // Incomplete constructor
  SPIRVAsmCallINTEL() : SPIRVInstruction(OC) {}
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityAsmINTEL);
  }
  bool isOperandLiteral(unsigned I) const override { return false; }

  void setWordCount(SPIRVWord TheWordCount) override {
    SPIRVEntry::setWordCount(TheWordCount);
    Args.resize(TheWordCount - FixedWC);
  }
  const std::vector<SPIRVWord> &getArguments() const { return Args; }

  SPIRVAsmINTEL *getAsm() const { return Asm; }

protected:
  _SPIRV_DEF_DEC4_OVERRIDE(Type, Id, Asm, Args)
  void validate() const override {
    SPIRVInstruction::validate();
    IGC_ASSERT(WordCount >= FixedWC);
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT_MESSAGE(getBasicBlock(), "Invalid BB");
    IGC_ASSERT(getBasicBlock()->getModule() == Asm->getModule());
  }
  SPIRVAsmINTEL *Asm;
  std::vector<SPIRVWord> Args;
};

} // namespace igc_spv
#endif // SPIRV_LIBSPIRV_SPIRVASM_H
