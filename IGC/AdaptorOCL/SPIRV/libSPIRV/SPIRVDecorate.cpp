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

// This file implements SPIR-V decorations.

#include "SPIRVDecorate.h"
#include "SPIRVValue.h"
#include "Probe/Assertion.h"

namespace igc_spv{

SPIRVDecorateGeneric::SPIRVDecorateGeneric(Op OC, SPIRVWord WC,
    Decoration TheDec, SPIRVEntry *TheTarget)
  :SPIRVAnnotationGeneric(TheTarget, WC, OC), Dec(TheDec), Owner(nullptr){
  validate();
}

SPIRVDecorateGeneric::SPIRVDecorateGeneric(Op OC, SPIRVWord WC,
    Decoration TheDec, SPIRVEntry *TheTarget, SPIRVWord V)
  :SPIRVAnnotationGeneric(TheTarget, WC, OC), Dec(TheDec), Owner(nullptr){
  Literals.push_back(V);
  validate();
}

SPIRVDecorateGeneric::SPIRVDecorateGeneric(Op OC)
  :SPIRVAnnotationGeneric(OC), Dec(DecorationRelaxedPrecision), Owner(nullptr){
}

Decoration
SPIRVDecorateGeneric::getDecorateKind()const {
  return Dec;
}

SPIRVWord
SPIRVDecorateGeneric::getLiteral(size_t i) const {
  IGC_ASSERT_EXIT_MESSAGE(i <= Literals.size(), "Out of bounds");
  return Literals[i];
}

std::vector<SPIRVWord> SPIRVDecorateGeneric::getVecLiteral() const {
  return Literals;
}

size_t
SPIRVDecorateGeneric::getLiteralCount() const {
  return Literals.size();
}

void
SPIRVDecorate::setWordCount(SPIRVWord Count){
  WordCount = Count;
  Literals.resize(WordCount - FixedWC);
}

void
SPIRVDecorate::decode(std::istream &I)
{
    getDecoder(I) >> Target >> Dec;

    auto literalsStartPos = I.tellg();
    getDecoder(I) >> Literals;
    auto literalsEndPos = I.tellg();

    auto *target = getOrCreateTarget();

    if (Dec == DecorationLinkageAttributes)
    {
        I.seekg(literalsStartPos);
        std::string funcName;
        getDecoder(I) >> funcName;
        target->setName(funcName);
        I.seekg(literalsEndPos);
    }

    target->addDecorate(this);
}

void
SPIRVMemberDecorate::setWordCount(SPIRVWord Count){
  WordCount = Count;
  Literals.resize(WordCount - FixedWC);
}

void
SPIRVMemberDecorate::decode(std::istream &I){
  getDecoder(I) >> Target >> MemberNumber >> Dec >> Literals;
  getOrCreateTarget()->addMemberDecorate(this);
}

void
SPIRVDecorationGroup::decode(std::istream &I){
  getDecoder(I) >> Id;
  Module->addDecorationGroup(this);
}

void
SPIRVGroupDecorateGeneric::decode(std::istream &I){
  getDecoder(I) >> DecorationGroup >> Targets;
  Module->addGroupDecorateGeneric(this);
}

void
SPIRVGroupDecorate::decorateTargets() {
  for(auto &I:Targets) {
    auto Target = getOrCreate(I);
    for (auto &Dec:DecorationGroup->getDecorations()) {
      IGC_ASSERT(Dec->isDecorate());
      Target->addDecorate(static_cast<const SPIRVDecorate *const>(Dec));
    }
  }
}

void
SPIRVGroupMemberDecorate::decorateTargets() {
  for(auto &I:Targets) {
    auto Target = getOrCreate(I);
    for (auto &Dec:DecorationGroup->getDecorations()) {
      IGC_ASSERT(Dec->isMemberDecorate());
      Target->addMemberDecorate(static_cast<const SPIRVMemberDecorate*>(Dec));
    }
  }
}

bool operator==(const SPIRVDecorateGeneric &A, const SPIRVDecorateGeneric &B) {
  if (A.getTargetId() != B.getTargetId())
    return false;
  if (A.getOpCode() != B.getOpCode())
    return false;
  if (A.getDecorateKind() != B.getDecorateKind())
    return false;
  if (A.getLiteralCount() != B.getLiteralCount())
    return false;
  for (size_t I = 0, E = A.getLiteralCount(); I != E; ++I) {
    auto EA = A.getLiteral(I);
    auto EB = B.getLiteral(I);
    if (EA != EB)
      return false;
  }
  return true;
}

void SPIRVDecorateId::setWordCount(SPIRVWord Count) {
    WordCount = Count;
    Literals.resize(WordCount - FixedWC);
}

void SPIRVDecorateId::decode(std::istream& I) {
    SPIRVDecoder Decoder = getDecoder(I);
    Decoder >> Target >> Dec >> Literals;
    getOrCreateTarget()->addDecorate(this);
}

}

