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

//===- SPIRVBasicBlock.h - SPIR-V Basic Block --------------------*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines Basic Block class for SPIR-V.
///
//===----------------------------------------------------------------------===//

#ifndef SPIRVBASICBLOCK_HPP_
#define SPIRVBASICBLOCK_HPP_

#include "SPIRVValue.h"
#include <algorithm>

namespace spv{
class SPIRVFunction;
class SPIRVInstruction;
class SPIRVDecoder;
class SPIRVBasicBlock: public SPIRVValue {

public:
  SPIRVBasicBlock(SPIRVId TheId, SPIRVFunction *Func);

  SPIRVBasicBlock():SPIRVValue(OpLabel), ParentF(NULL){
    setAttr();
  }

  SPIRVDecoder getDecoder(std::istream &IS);
  SPIRVFunction *getParent() const { return ParentF;}
  size_t getNumInst() const { return InstVec.size();}
  SPIRVInstruction *getInst(size_t I) const { return InstVec[I];}
  SPIRVInstruction *getPrevious(const SPIRVInstruction *I) const {
    auto Loc = find(I);
    if (Loc == InstVec.end() || Loc == InstVec.begin())
      return nullptr;
    return *(--Loc);
  }
  SPIRVInstruction *getNext(const SPIRVInstruction *I) const {
    auto Loc = find(I);
    if (Loc == InstVec.end())
      return nullptr;
    ++Loc;
    if (Loc == InstVec.end())
      return nullptr;
    return *Loc;
  }
  // Return the last instruction in the BB or nullptr if the BB is empty.
  const SPIRVInstruction *getTerminateInstr() const {
    return InstVec.empty() ? nullptr : InstVec.back();
  }

  void setScope(SPIRVEntry *Scope);
  void setParent(SPIRVFunction *F) { ParentF = F;}
  SPIRVInstruction *
  addInstruction(SPIRVInstruction *I,
                 const SPIRVInstruction *InsertBefore = nullptr);

  void setAttr() { setHasNoType();}
  _SPIRV_DCL_DEC
  void validate()const {
    SPIRVValue::validate();
    assert(ParentF && "Invalid parent function");
  }

private:
  SPIRVFunction *ParentF;
  typedef std::vector<SPIRVInstruction *> SPIRVInstructionVector;
  SPIRVInstructionVector InstVec;

  SPIRVInstructionVector::const_iterator find(const SPIRVInstruction *Inst)
    const {
    return std::find(InstVec.begin(), InstVec.end(), Inst);
  }
};

typedef SPIRVBasicBlock SPIRVLabel;
}

#endif
