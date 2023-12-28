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

#ifndef SPIRVFUNCTION_HPP_
#define SPIRVFUNCTION_HPP_
#include "SPIRVValue.h"
#include "SPIRVBasicBlock.h"
#include "Probe/Assertion.h"

namespace igc_spv{


class SPIRVDecoder;

class SPIRVFunctionParameter: public SPIRVValue {
public:
  SPIRVFunctionParameter(SPIRVType *TheType, SPIRVId TheId,
      SPIRVFunction *TheParent, unsigned TheArgNo);
  SPIRVFunctionParameter():SPIRVValue(OpFunctionParameter),
      ParentFunc(nullptr), ArgNo(0){}
  unsigned getArgNo()const { return ArgNo;}
  void foreachAttr(std::function<void(SPIRVFuncParamAttrKind)>);
  void addAttr(SPIRVFuncParamAttrKind Kind) {
    addDecorate(new SPIRVDecorate(DecorationFuncParamAttr, this, Kind));
  }
  void setParent(SPIRVFunction *Parent) { ParentFunc = Parent;}
  bool hasAttr(SPIRVFuncParamAttrKind Kind) const {
    return (getDecorate(DecorationFuncParamAttr).count(Kind) > 0) ;
  }
  bool isByVal()const { return hasAttr(SPIRVFuncParamAttrKind::FunctionParameterAttributeByVal); }
  bool isZext()const { return hasAttr(SPIRVFuncParamAttrKind::FunctionParameterAttributeZext); }
  bool hasMaxByteOffset(SPIRVWord &offset) const
  {
      return hasDecorate(DecorationMaxByteOffset, 0, &offset);
  }
  CapVec getRequiredCapability() const override {
     if (hasLinkageType() && getLinkageType() == SPIRVLinkageTypeKind::LinkageTypeImport)
        return getVec(SPIRVCapabilityKind::CapabilityLinkage);
    return CapVec();
  }
protected:
  void validate()const override {
    SPIRVValue::validate();
    IGC_ASSERT_MESSAGE(ParentFunc, "Invalid parent function");
  }
  _SPIRV_DEF_DEC2(Type, Id)
private:
  SPIRVFunction *ParentFunc;
  unsigned ArgNo;
};

typedef std::map<const llvm::BasicBlock*, const igc_spv::SPIRVValue*>
SPIRVToLLVMLoopMetadataMap;

class SPIRVFunction: public SPIRVValue, public SPIRVComponentExecutionModes {
public:
  // Complete constructor. It does not construct basic blocks.
  SPIRVFunction(SPIRVModule *M, SPIRVTypeFunction *FunctionType, SPIRVId TheId)
    :SPIRVValue(M, 5, OpFunction, FunctionType->getReturnType(), TheId),
    FuncType(FunctionType), FCtrlMask(SPIRVFunctionControlMaskKind::FunctionControlMaskNone) {
    addAllArguments(TheId + 1);
    validate();
  }

  // Incomplete constructor
  SPIRVFunction():SPIRVValue(OpFunction),FuncType(NULL),
     FCtrlMask(SPIRVFunctionControlMaskKind::FunctionControlMaskNone){}

  SPIRVDecoder getDecoder(std::istream &IS) override;
  SPIRVTypeFunction *getFunctionType() const { return FuncType;}
  SPIRVWord getFuncCtlMask() const { return FCtrlMask;}
  SPIRVToLLVMLoopMetadataMap& getFuncLoopMetadataMap() { return FuncLoopMetadataMap; }
  size_t getNumBasicBlock() const { return BBVec.size();}
  SPIRVBasicBlock *getBasicBlock(size_t i) const { return BBVec[i];}
  size_t getNumArguments() const {
    return getFunctionType()->getNumParameters();
  }
  SPIRVId getArgumentId(size_t i)const { return Parameters[i]->getId();}
  SPIRVFunctionParameter *getArgument(size_t i) const {
    return Parameters[i];
  }
  void foreachArgument(std::function<void(SPIRVFunctionParameter *)>Func) {
    for (size_t I = 0, E = getNumArguments(); I != E; ++I)
      Func(getArgument(I));
  }

  void foreachReturnValueAttr(std::function<void(SPIRVFuncParamAttrKind)>);

  void setFunctionControlMask(SPIRVWord Mask) {
    FCtrlMask = Mask;
  }

  void takeExecutionModes(SPIRVForward *Forward) {
    ExecModes = std::move(Forward->ExecModes);
  }

  // Assume BB contains valid Id.
  SPIRVBasicBlock *addBasicBlock(SPIRVBasicBlock *BB) {
    Module->add(BB);
    BB->setParent(this);
    BBVec.push_back(BB);
    return BB;
  }

  _SPIRV_DCL_DEC
  void validate() const override {
    SPIRVValue::validate();
    IGC_ASSERT_MESSAGE(FuncType, "Invalid func type");
  }

private:
  SPIRVFunctionParameter *addArgument(unsigned TheArgNo, SPIRVId TheId) {
    SPIRVFunctionParameter *Arg = new SPIRVFunctionParameter(
        getFunctionType()->getParameterType(TheArgNo),
        TheId, this, TheArgNo);
    Module->add(Arg);
    Parameters.push_back(Arg);
    return Arg;
  }

  void addAllArguments(SPIRVId FirstArgId) {
    for (size_t i = 0, e = getFunctionType()->getNumParameters(); i != e; ++i)
      addArgument(i, FirstArgId + i);
  }
  void decodeBB(SPIRVDecoder &);

  SPIRVTypeFunction *FuncType;                  // Function type
  SPIRVWord FCtrlMask;                          // Function control mask

  std::vector<SPIRVFunctionParameter *> Parameters;
  typedef std::vector<SPIRVBasicBlock *> SPIRVLBasicBlockVector;
  SPIRVLBasicBlockVector BBVec;
  // Loops metadata is translated in the end of a function translation.
  // This storage contains pairs of translated loop header basic block and loop
  // metadata SPIR-V instruction in SPIR-V representation of this basic block.
  SPIRVToLLVMLoopMetadataMap FuncLoopMetadataMap;
};

typedef SPIRVEntryOpCodeOnly<OpFunctionEnd> SPIRVFunctionEnd;

class SPIRVConstFunctionPointerINTEL : public SPIRVValue {
  const static Op OC = OpConstFunctionPointerINTEL;
  const static SPIRVWord FixedWordCount = 4;

public:
  SPIRVConstFunctionPointerINTEL(SPIRVId TheId, SPIRVType* TheType,
    SPIRVFunction* TheFunction, SPIRVModule* M)
    : SPIRVValue(M, FixedWordCount, OC, TheType, TheId),
    TheFunction(TheFunction->getId()) {
    validate();
  }
  SPIRVConstFunctionPointerINTEL()
    : SPIRVValue(OC), TheFunction(SPIRVID_INVALID) {}
  SPIRVFunction* getFunction() const { return get<SPIRVFunction>(TheFunction); }
  _SPIRV_DEF_DEC3_OVERRIDE(Type, Id, TheFunction)
  void validate() const override { SPIRVValue::validate(); }
  CapVec getRequiredCapability() const override {
    return getVec(CapabilityFunctionPointersINTEL);
  }

protected:
  SPIRVId TheFunction;
};

}

#endif /* SPIRVFUNCTION_HPP_ */
