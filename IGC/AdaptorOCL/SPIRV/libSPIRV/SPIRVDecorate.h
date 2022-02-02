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

// This file defines SPIR-V decorations.

#ifndef SPIRVDECORATE_HPP_
#define SPIRVDECORATE_HPP_

#include "SPIRVEntry.h"
#include "Probe/Assertion.h"

namespace igc_spv{
class SPIRVDecorationGroup;
class SPIRVDecorateGeneric:public SPIRVAnnotationGeneric{
public:
  // Complete constructor for decorations without literals
  SPIRVDecorateGeneric(Op OC, SPIRVWord WC, Decoration TheDec,
      SPIRVEntry *TheTarget);
  // Complete constructor for decorations with one word literal
  SPIRVDecorateGeneric(Op OC, SPIRVWord WC, Decoration TheDec,
      SPIRVEntry *TheTarget, SPIRVWord V);
  // Incomplete constructor
  SPIRVDecorateGeneric(Op OC);

  SPIRVWord getLiteral(size_t) const;
  std::vector<SPIRVWord> getVecLiteral() const;
  Decoration getDecorateKind() const;
  size_t getLiteralCount() const;
  /// Compare kind, literals and target.
  friend bool operator==(const SPIRVDecorateGeneric &A,
      const SPIRVDecorateGeneric &B);

  SPIRVDecorationGroup* getOwner() const {
    return Owner;
  }

  void setOwner(SPIRVDecorationGroup* owner) {
    Owner = owner;
  }

protected:
  Decoration Dec;
  std::vector<SPIRVWord> Literals;
  SPIRVDecorationGroup *Owner; // Owning decorate group
};

typedef std::vector<const SPIRVDecorateGeneric *> SPIRVDecorateSet;

class SPIRVDecorate:public SPIRVDecorateGeneric{
public:
  static const Op OC = OpDecorate;
  static const SPIRVWord FixedWC = 3;
  // Complete constructor for decorations without literals
  SPIRVDecorate(Decoration TheDec, SPIRVEntry *TheTarget)
    :SPIRVDecorateGeneric(OC, 3, TheDec, TheTarget){}
  // Complete constructor for decorations with one word literal
  SPIRVDecorate(Decoration TheDec, SPIRVEntry *TheTarget, SPIRVWord V)
    :SPIRVDecorateGeneric(OC, 4, TheDec, TheTarget, V){}
  // Incomplete constructor
  SPIRVDecorate():SPIRVDecorateGeneric(OC){}

  _SPIRV_DCL_DEC
  void setWordCount(SPIRVWord);
};

class SPIRVMemberDecorate:public SPIRVDecorateGeneric{
public:
  static const Op OC = OpMemberDecorate;
  static const SPIRVWord FixedWC = 4;
  // Complete constructor for decorations without literals
  SPIRVMemberDecorate(Decoration TheDec, SPIRVWord Member, SPIRVEntry *TheTarget)
    :SPIRVDecorateGeneric(OC, 4, TheDec, TheTarget), MemberNumber(Member){}
  // Complete constructor for decorations with one word literal
  SPIRVMemberDecorate(Decoration TheDec, SPIRVWord Member, SPIRVEntry *TheTarget,
      SPIRVWord V)
    :SPIRVDecorateGeneric(OC, 5, TheDec, TheTarget, V), MemberNumber(Member){}
  // Incomplete constructor
  SPIRVMemberDecorate():SPIRVDecorateGeneric(OC), MemberNumber(SPIRVWORD_MAX){}

  SPIRVWord getMemberNumber() const { return MemberNumber;}
  std::pair<SPIRVWord, Decoration> getPair() const {
    return std::make_pair(MemberNumber, Dec);
  }

  _SPIRV_DCL_DEC
  void setWordCount(SPIRVWord);
protected:
  SPIRVWord MemberNumber;
};

class SPIRVDecorationGroup:public SPIRVEntry{
public:
  static const Op OC = OpDecorationGroup;
  static const SPIRVWord WC = 2;
  // Complete constructor. Does not populate Decorations.
  SPIRVDecorationGroup(SPIRVModule *TheModule, SPIRVId TheId)
    :SPIRVEntry(TheModule, WC, OC, TheId){
    validate();
  };
  // Incomplete constructor
  SPIRVDecorationGroup():SPIRVEntry(OC){}
  _SPIRV_DCL_DEC
  // Move the given decorates to the decoration group
  void takeDecorates(SPIRVDecorateSet &Decs) {
    Decorations = std::move(Decs);
    for (auto &I:Decorations)
      const_cast<SPIRVDecorateGeneric *>(I)->setOwner(this);
    Decs.clear();
  }

  SPIRVDecorateSet& getDecorations() {
    return Decorations;
  }

protected:
  SPIRVDecorateSet Decorations;
  void validate()const {
    IGC_ASSERT(OpCode == OC);
    IGC_ASSERT(WordCount == WC);
  }
};

class SPIRVGroupDecorateGeneric:public SPIRVEntryNoIdGeneric{
public:
  static const SPIRVWord FixedWC = 2;
  // Complete constructor
  SPIRVGroupDecorateGeneric(Op OC, SPIRVDecorationGroup *TheGroup,
      const std::vector<SPIRVId> &TheTargets)
    :SPIRVEntryNoIdGeneric(TheGroup->getModule(), FixedWC + TheTargets.size(),
        OC),
     DecorationGroup(TheGroup), Targets(TheTargets){
  }
  // Incomplete constructor
  SPIRVGroupDecorateGeneric(Op OC)
    :SPIRVEntryNoIdGeneric(OC), DecorationGroup(nullptr){}

  void setWordCount(SPIRVWord WC) {
    SPIRVEntryNoIdGeneric::setWordCount(WC);
    Targets.resize(WC - FixedWC);
  }
  virtual void decorateTargets() = 0;
  _SPIRV_DCL_DEC
protected:
  SPIRVDecorationGroup *DecorationGroup;
  std::vector<SPIRVId> Targets;
};

class SPIRVGroupDecorate:public SPIRVGroupDecorateGeneric{
public:
  static const Op OC = OpGroupDecorate;
  // Complete constructor
  SPIRVGroupDecorate(SPIRVDecorationGroup *TheGroup,
      const std::vector<SPIRVId> &TheTargets)
    :SPIRVGroupDecorateGeneric(OC, TheGroup, TheTargets){}
  // Incomplete constructor
  SPIRVGroupDecorate()
    :SPIRVGroupDecorateGeneric(OC){}

  virtual void decorateTargets();
};

class SPIRVGroupMemberDecorate:public SPIRVGroupDecorateGeneric{
public:
  static const Op OC = OpGroupMemberDecorate;
  // Complete constructor
  SPIRVGroupMemberDecorate(SPIRVDecorationGroup *TheGroup,
      const std::vector<SPIRVId> &TheTargets)
    :SPIRVGroupDecorateGeneric(OC, TheGroup, TheTargets){}
  // Incomplete constructor
  SPIRVGroupMemberDecorate()
    :SPIRVGroupDecorateGeneric(OC){}

  virtual void decorateTargets();
};

class SPIRVDecorateId : public SPIRVDecorateGeneric {
public:
    static const Op OC = OpDecorateId;
    static const SPIRVWord FixedWC = 3;
    // Complete constructor for decorations with one id operand
    SPIRVDecorateId(Decoration TheDec, SPIRVEntry* TheTarget, SPIRVId V)
        : SPIRVDecorateGeneric(OC, 4, TheDec, TheTarget, V) {}
    // Incomplete constructor
    SPIRVDecorateId() : SPIRVDecorateGeneric(OC) {}

    _SPIRV_DCL_DEC_OVERRIDE
    void setWordCount(SPIRVWord) override;
    void validate() const override {
        SPIRVDecorateGeneric::validate();
        IGC_ASSERT(WordCount == Literals.size() + FixedWC);
    }
};

class SPIRVDecorateAliasScopeINTEL : public SPIRVDecorateId {
public:
    // Complete constructor for SPIRVDecorateAliasScopeINTEL
    SPIRVDecorateAliasScopeINTEL(SPIRVEntry* TheTarget, SPIRVId AliasList)
        : SPIRVDecorateId(DecorationAliasScopeINTEL, TheTarget,
            AliasList) {};
};

class SPIRVDecorateNoAliasINTEL : public SPIRVDecorateId {
public:
    // Complete constructor for SPIRVDecorateNoAliasINTEL
    SPIRVDecorateNoAliasINTEL(SPIRVEntry* TheTarget, SPIRVId AliasList)
        : SPIRVDecorateId(DecorationNoAliasINTEL, TheTarget,
            AliasList) {};
};

class SPIRVDecorateHostAccessINTEL : public SPIRVDecorate {
public:
  // Complete constructor for SPIRVHostAccessINTEL
  SPIRVDecorateHostAccessINTEL(SPIRVEntry* TheTarget, SPIRVWord AccessMode,
    const std::string& VarName)
    : SPIRVDecorate(DecorationHostAccessINTEL, TheTarget) {
    Literals.push_back(AccessMode);
    for (auto& I : getVec(VarName))
        Literals.push_back(I);
    WordCount += Literals.size();
  };

  SPIRVWord getAccessMode() const { return Literals.front(); }
  std::string getVarName() const {
    return getString(Literals.cbegin() + 1, Literals.cend());
  }
};

}


#endif /* SPIRVDECORATE_HPP_ */
