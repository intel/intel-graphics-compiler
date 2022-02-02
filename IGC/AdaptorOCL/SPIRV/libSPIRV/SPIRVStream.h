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

// This file defines Word class for SPIR-V.

#ifndef SPIRVSTREAM_H
#define SPIRVSTREAM_H

#include "SPIRVDebug.h"
#include "SPIRVModule.h"
#include "SPIRVExtInst.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>

namespace igc_spv{

class SPIRVFunction;
class SPIRVBasicBlock;

class SPIRVDecoder {
public:
  SPIRVDecoder(std::istream& InputStream, SPIRVModule& Module)
    :IS(InputStream), M(Module), WordCount(0), OpCode(OpNop),
     Scope(NULL){}
  SPIRVDecoder(std::istream& InputStream, SPIRVFunction& F);
  SPIRVDecoder(std::istream& InputStream, SPIRVBasicBlock &BB);

  void setScope(SPIRVEntry *);
  bool getWordCountAndOpCode();
  SPIRVEntry *getEntry();
  void validate()const;

  std::istream &IS;
  SPIRVModule &M;
  SPIRVWord WordCount;
  Op OpCode;
  SPIRVEntry *Scope; // A function or basic block

  std::vector<SPIRVEntry*>
      getContinuedInstructions(const Op ContinuedOpCode);
};

template<typename T>
const SPIRVDecoder&
DecodeBinary(const SPIRVDecoder& I, T &V);

template<typename T>
const SPIRVDecoder&
operator>>(const SPIRVDecoder& I, T &V) {
  return DecodeBinary(I, V);
}

template<typename T>
const SPIRVDecoder&
operator>>(const SPIRVDecoder& I, T *&P) {
  SPIRVId Id;
  I >> Id;
  P = static_cast<T*>(I.M.getEntry(Id));
  return I;
}

template<typename IterTy>
const SPIRVDecoder&
operator>>(const SPIRVDecoder& Decoder, const std::pair<IterTy,IterTy> &Range) {
  for (IterTy I = Range.first, E = Range.second; I != E; ++I)
    Decoder >> *I;
  return Decoder;
}

template<typename T>
const SPIRVDecoder&
operator>>(const SPIRVDecoder& I, std::vector<T> &V) {
  for (size_t i = 0, e = V.size(); i != e; ++i)
    I >> V[i];
  return I;
}

template <typename T>
const SPIRVDecoder&
operator>>(const SPIRVDecoder& I, llvm::Optional<T>& V) {
  if (V)
    I >> V.getValue();
  return I;
}

#define SPIRV_DEC_DEC(Type) \
    const SPIRVDecoder& operator>>(const SPIRVDecoder& I, Type &V);

SPIRV_DEC_DEC(Op)
SPIRV_DEC_DEC(Decoration)
SPIRV_DEC_DEC(OCLExtOpKind)
SPIRV_DEC_DEC(OCLExtOpDbgKind)

const SPIRVDecoder& operator>>(const SPIRVDecoder&I, std::string& Str);

} // namespace igc_spv
#endif
