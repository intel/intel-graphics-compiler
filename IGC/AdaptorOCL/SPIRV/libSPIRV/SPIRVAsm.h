/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
  SPIRVAsmTargetINTEL *Target;
  SPIRVTypeFunction *FunctionType;
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
