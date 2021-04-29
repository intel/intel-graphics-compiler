/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the documentation
and/or other materials provided with the distribution.
Neither the names of Advanced Micro Devices, Inc., nor the names of its
contributors may be used to endorse or promote products derived from this
Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file implements SPIRV basic block.

#include "SPIRVBasicBlock.h"
#include "SPIRVInstruction.h"
#include "SPIRVFunction.h"
#include "Probe/Assertion.h"

using namespace igc_spv;

SPIRVBasicBlock::SPIRVBasicBlock(SPIRVId TheId, SPIRVFunction *Func)
  :SPIRVValue(Func->getModule(), 2, OpLabel, TheId), ParentF(Func) {
  setAttr();
  validate();
}

SPIRVDecoder
SPIRVBasicBlock::getDecoder(std::istream &IS){
  return SPIRVDecoder(IS, *this);
}

/// Assume I contains valid Id.
SPIRVInstruction *
SPIRVBasicBlock::addInstruction(SPIRVInstruction *I,
                                const SPIRVInstruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(I, "Invalid instruction");
  Module->add(I);
  I->setParent(this);
  if (InsertBefore) {
    auto Pos = find(InsertBefore);
    // If insertion of a new instruction before the one passed to the function
    // is illegal, insertion before the returned instruction is guaranteed
    // to retain correct instruction order in a block
    if (Pos != InstVec.begin() && isa<OpLoopMerge>(*std::prev(Pos)))
      --Pos;
    InstVec.insert(Pos, I);
  } else
    InstVec.push_back(I);
  return I;
}

_SPIRV_IMP_DEC1(SPIRVBasicBlock, Id)

void
SPIRVBasicBlock::setScope(SPIRVEntry *Scope) {
  IGC_ASSERT_MESSAGE(Scope && Scope->getOpCode() == OpFunction, "Invalid scope");
  setParent(static_cast<SPIRVFunction*>(Scope));
}
