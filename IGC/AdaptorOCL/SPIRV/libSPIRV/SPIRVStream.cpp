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

// This file implements SPIR-V stream class.

#include "SPIRVDebug.h"
#include "SPIRVStream.h"
#include "SPIRVFunction.h"
#include "SPIRVInstruction.h"
#include "SPIRVDebugInfoExt.h"
#include "Probe/Assertion.h"

namespace igc_spv{

SPIRVDecoder::SPIRVDecoder(std::istream &InputStream, SPIRVFunction &F)
  :IS(InputStream), M(*F.getModule()), WordCount(0), OpCode(OpNop),
   Scope(&F){}

SPIRVDecoder::SPIRVDecoder(std::istream &InputStream, SPIRVBasicBlock &BB)
  :IS(InputStream), M(*BB.getModule()), WordCount(0), OpCode(OpNop),
   Scope(&BB){}

void
SPIRVDecoder::setScope(SPIRVEntry *TheScope) {
  IGC_ASSERT(TheScope && (TheScope->getOpCode() == OpFunction ||
      TheScope->getOpCode() == OpLabel));
  Scope = TheScope;
}

template<>
const SPIRVDecoder& DecodeBinary(const SPIRVDecoder& I, bool &V) {
   SPIRVWord W;
   I.IS.read(reinterpret_cast<char*>(&W), sizeof(W));
   V = (W == 0) ? false : true;
   return I;
}

template<>
const SPIRVDecoder&
DecodeBinary(const SPIRVDecoder& I, SPIRVWord &V) {
   I.IS.read(reinterpret_cast<char*>(&V), sizeof(V));
   return I;
}

template<typename T>
const SPIRVDecoder& DecodeBinary(const SPIRVDecoder& I, T &V) {
   SPIRVWord W;
   DecodeBinary(I,W);
   V = static_cast<T>(W);
   return I;
}

//explicitly instantiate DecodeBinary for enum types
#define INSTANTIATE_DECODER_BINARY(Type) \
  template \
  const SPIRVDecoder& DecodeBinary \
  (const SPIRVDecoder& I, Type &V);

INSTANTIATE_DECODER_BINARY(enum igc_spv::StorageClass)
INSTANTIATE_DECODER_BINARY(enum igc_spv::Dim)
INSTANTIATE_DECODER_BINARY(enum igc_spv::AccessQualifier)
INSTANTIATE_DECODER_BINARY(enum igc_spv::Scope)
INSTANTIATE_DECODER_BINARY(enum igc_spv::ExecutionModel)
INSTANTIATE_DECODER_BINARY(enum igc_spv::ExecutionMode)
INSTANTIATE_DECODER_BINARY(enum igc_spv::AddressingModel)
INSTANTIATE_DECODER_BINARY(enum igc_spv::MemoryModel)
INSTANTIATE_DECODER_BINARY(enum igc_spv::SpvSourceLanguage)
INSTANTIATE_DECODER_BINARY(enum igc_spv::Capability)
INSTANTIATE_DECODER_BINARY(enum igc_spv::SPIRVVersionSupported)
INSTANTIATE_DECODER_BINARY(enum igc_spv::SPIRVGeneratorKind)
INSTANTIATE_DECODER_BINARY(enum igc_spv::SPIRVInstructionSchemaKind)
#undef SPIRV_DEF_DEC


template<class T>
const SPIRVDecoder&
decode(const SPIRVDecoder& I, T &V) {
  return DecodeBinary(I, V);
}

#define SPIRV_DEF_DEC(Type)                                      \
const SPIRVDecoder& operator>>(const SPIRVDecoder& I, Type &V) { \
  return decode(I, V);                                           \
}

SPIRV_DEF_DEC(Op)
SPIRV_DEF_DEC(Decoration)
SPIRV_DEF_DEC(OCLExtOpKind)
SPIRV_DEF_DEC(OCLExtOpDbgKind)
#undef SPIRV_DEF_DEC

// Read a string with padded 0's at the end so that they form a stream of
// words.
const SPIRVDecoder&
operator>>(const SPIRVDecoder&I, std::string& Str) {
  uint64_t Count = 0;
  char Ch = '\0';
  while ((!I.IS.eof() && I.IS.get(Ch)) && Ch != '\0') {
    Str += Ch;
    ++Count;
  }
  Count = (Count + 1) % 4;
  Count = Count ? 4 - Count : 0;
  for (;Count; --Count) {
    (!I.IS.eof() && I.IS.get(Ch));
    IGC_ASSERT(Ch == '\0' && "Invalid string in SPIRV");
  }
  return I;
}

bool
SPIRVDecoder::getWordCountAndOpCode() {
  if (IS.eof()) {
    WordCount = 0;
    OpCode = OpNop;
    return false;
  }

  SPIRVWord WordCountAndOpCode;
  *this >> WordCountAndOpCode;
  WordCount = WordCountAndOpCode >> 16;
  OpCode = static_cast<Op>(WordCountAndOpCode & 0xFFFF);

  IGC_ASSERT_MESSAGE(false == IS.bad(), "SPIRV stream is bad");
  if (IS.fail()) {
    WordCount = 0;
    OpCode = OpNop;
    return false;
  }
  return true;
}

SPIRVEntry *
SPIRVDecoder::getEntry() {
  if (WordCount == 0 || OpCode == OpNop)
    return NULL;
  SPIRVEntry *Entry = SPIRVEntry::create(OpCode);
  if (Entry) {
    Entry->setModule(&M);
    Entry->setWordCount(WordCount);
    IS >> *Entry;

    if (M.getErrorLog().getErrorCode() == SPIRVEC_UnsupportedSPIRVOpcode)
      return nullptr;

    if ((isModuleScopeAllowedOpCode(OpCode) && !Scope) ||
      // No need to attach scope to debug info extension operations
      (Entry->hasNoScope()))
    {}
    else
      Entry->setScope(Scope);

    IGC_ASSERT_MESSAGE(false == IS.bad(), "SPIRV stream fails");
    IGC_ASSERT_MESSAGE(false == IS.fail(), "SPIRV stream fails");
    M.add(Entry);
  }
  else {
    M.getErrorLog().setError(SPIRVEC_UnsupportedSPIRVOpcode,
      "IGC SPIRV Consumer does not support the following opcode: " + std::to_string(OpCode) + "\n");
  }
  return Entry;
}

void
SPIRVDecoder::validate()const {
  IGC_ASSERT_MESSAGE(OpCode != OpNop, "Invalid op code");
  IGC_ASSERT_MESSAGE(WordCount, "Invalid word count");
  IGC_ASSERT_MESSAGE(!IS.bad(), "Bad iInput stream");
}

// Read the next word from the stream and if OpCode matches the argument,
// decode the whole instruction. Multiple such instructions are possible. If
// OpCode doesn't match the argument, set position of the next character to be
// extracted from the stream to the beginning of the non-matching instruction.
// Returns vector of extracted instructions.
// Used to decode SPIRVTypeStructContinuedINTEL,
// SPIRVConstantCompositeContinuedINTEL and
// SPIRVSpecConstantCompositeContinuedINTEL.
std::vector<SPIRVEntry*>
SPIRVDecoder::getContinuedInstructions(const Op ContinuedOpCode) {
    std::vector<SPIRVEntry*> ContinuedInst;
    std::streampos Pos = IS.tellg(); // remember position
    getWordCountAndOpCode();
    while (OpCode == ContinuedOpCode) {
        SPIRVEntry* Entry = getEntry();
        assert(Entry && "Failed to decode entry! Invalid instruction!");
        M.add(Entry);
        ContinuedInst.push_back(Entry);
        Pos = IS.tellg();
        getWordCountAndOpCode();
    }
    IS.seekg(Pos); // restore position
    return ContinuedInst;
}

}
