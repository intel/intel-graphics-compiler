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

#include "SPIRVFunction.h"
#include "SPIRVInstruction.h"
#include "SPIRVStream.h"
#include "Probe/Assertion.h"

using namespace igc_spv;

SPIRVFunctionParameter::SPIRVFunctionParameter(SPIRVType *TheType, SPIRVId TheId,
    SPIRVFunction *TheParent, unsigned TheArgNo):
        SPIRVValue(TheParent->getModule(), 3, OpFunctionParameter,
        TheType, TheId),
    ParentFunc(TheParent),
    ArgNo(TheArgNo){
  validate();
}

void
SPIRVFunctionParameter::foreachAttr(
    std::function<void(SPIRVFuncParamAttrKind)>Func){
  auto Locs = Decorates.equal_range(DecorationFuncParamAttr);
  for (auto I = Locs.first, E = Locs.second; I != E; ++I){
    auto Attr = static_cast<SPIRVFuncParamAttrKind>(
        I->second->getLiteral(0));
    IGC_ASSERT(isValid(Attr));
    Func(Attr);
  }
}

SPIRVDecoder
SPIRVFunction::getDecoder(std::istream &IS) {
  return SPIRVDecoder(IS, *this);
}

void
SPIRVFunction::decode(std::istream &I) {
  SPIRVDecoder Decoder = getDecoder(I);
  Decoder >> Type >> Id >> FCtrlMask >> FuncType;
  Module->addFunction(this);

  Decoder.getWordCountAndOpCode();
  while (!I.eof()) {
    if (Decoder.OpCode == OpFunctionEnd)
      break;

    switch(Decoder.OpCode) {
    case OpFunctionParameter: {
      auto Param = static_cast<SPIRVFunctionParameter *>(Decoder.getEntry());
      if (Param != NULL){
          Param->setParent(this);
          Parameters.push_back(Param);
          Decoder.getWordCountAndOpCode();
          continue;
      }
      else{
          IGC_ASSERT_EXIT(0);
      }
      break;
    }
    case OpLabel: {
      decodeBB(Decoder);
      if(Decoder.M.getErrorLog().getErrorCode() == SPIRVEC_UnsupportedSPIRVOpcode)
        return;
      break;
    }
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "Invalid SPIRV format");
      break;
    }
  }
}

/// Decode basic block and contained instructions.
/// Do it here instead of in BB:decode to avoid back track in input stream.
void
SPIRVFunction::decodeBB(SPIRVDecoder &Decoder) {
  SPIRVBasicBlock* const BB = static_cast<SPIRVBasicBlock*>(Decoder.getEntry());
  if (nullptr != BB){
      SPIRVLine* line = nullptr;
      SPIRVExtInst* diScope = nullptr;
      addBasicBlock(BB);

      Decoder.setScope(BB);
      while (Decoder.getWordCountAndOpCode()) {
          if (Decoder.OpCode == OpFunctionEnd ||
              Decoder.OpCode == OpLabel) {
              break;
          }

          switch (Decoder.OpCode)
          {
          case OpNop:
          case OpName:
          case OpDecorate:
          // We don't currently have a use for these structured control flow
          // opcodes, just ignore them for now.
          case OpSelectionMerge:
          // OpUndef is in the strange "Miscellaneous Instructions" category which means
          // that it can be used as a constant or as an instruction.  LLVM represents it
          // as an constant so if we encounter it as an instruction, don't add it to the
          // basic block but store it so it can be referenced later and emitted in LLVM
          // as a constant.
          case OpUndef:
              Decoder.getEntry();
              continue;
          default: break;
          }

          auto newEntry = Decoder.getEntry();
          if (!newEntry)
            return;

          if (newEntry->isInst() && !newEntry->isScope())
          {
              SPIRVInstruction *Inst = static_cast<SPIRVInstruction *>(newEntry);
              BB->addInstruction(Inst);
              Inst->setLine(line);
              Inst->setDIScope(diScope);
          }
          else if (newEntry->isOpLine())
              line = static_cast<SPIRVLine*>(newEntry);
          else if (newEntry->isOpNoLine())
              line = nullptr;
          else if (newEntry->startsScope())
              diScope = static_cast<SPIRVExtInst*>(newEntry);
          else if (newEntry->endsScope())
              diScope = nullptr;
          else
              IGC_ASSERT_MESSAGE(0, "Non-instruction entry found");
      }
      Decoder.setScope(this);
  }
  else{
      IGC_ASSERT_EXIT(0);
  }
}

void
SPIRVFunction::foreachReturnValueAttr(
    std::function<void(SPIRVFuncParamAttrKind)>Func){
  auto Locs = Decorates.equal_range(DecorationFuncParamAttr);
  for (auto I = Locs.first, E = Locs.second; I != E; ++I){
    auto Attr = static_cast<SPIRVFuncParamAttrKind>(
        I->second->getLiteral(0));
    IGC_ASSERT(isValid(Attr));
    Func(Attr);
  }
}



