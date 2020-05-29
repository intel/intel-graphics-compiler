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

//===-- llvm/IntrinsicInst.h - Intrinsic Instruction Wrappers ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines classes that make it really easy to deal with intrinsic
// functions with the isa/dyncast family of functions.  In particular, this
// allows you to do things like:
//
//     if (MemCpyInst *MCI = dyn_cast<MemCpyInst>(Inst))
//        ... MCI->getDest() ... MCI->getSource() ...
//
// All intrinsic function calls are instances of the call instruction, so these
// are all subclasses of the CallInst class.  Note that none of these classes
// has state or virtual methods, which is an important part of this gross/neat
// hack working.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/Types.hpp"
#include "GenIntrinsics.h"

namespace llvm {
/// IntrinsicInst - A useful wrapper class for inspecting calls to intrinsic
/// functions.  This allows the standard isa/dyncast/cast functionality to
/// work with calls to intrinsic functions.
class GenIntrinsicInst : public CallInst {
    GenIntrinsicInst() = delete;
    GenIntrinsicInst(const GenIntrinsicInst&) = delete;
    void operator=(const GenIntrinsicInst&) = delete;

protected:
    inline uint64_t valueToImm64(Value* cv) const
    {
        return cast<ConstantInt>(cv)->getZExtValue();
    }

public:
    /// getIntrinsicID - Return the intrinsic ID of this intrinsic.
    ///
    GenISAIntrinsic::ID getIntrinsicID() const {
        return GenISAIntrinsic::getIntrinsicID(getCalledFunction());
    }

    bool isGenIntrinsic(GenISAIntrinsic::ID intrinID) const {
        return getIntrinsicID() == intrinID;
    }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const CallInst *I) {
        if (const Function *CF = I->getCalledFunction()) {
            return (CF->getName().startswith(GenISAIntrinsic::getGenIntrinsicPrefix()));
        }
        return false;
    }
    static inline bool classof(const Value *V) {
        return isa<CallInst>(V) && classof(cast<CallInst>(V));
    }

    uint64_t getImm64Operand(unsigned idx) const {
        assert(isa<ConstantInt>(getOperand(idx)));
        return valueToImm64(getOperand(idx));
    }
};

class RTWritIntrinsic : public GenIntrinsicInst {
protected:
    Value* getOperand(unsigned i) const { return GenIntrinsicInst::getOperand(i); }
    void setOperand(unsigned i, Value* v) { return GenIntrinsicInst::setOperand(i, v); }

public:
    inline Value* getSource0Alpha()     const { return getOperand(0); }
    inline Value* getOMask()            const { return getOperand(1); }
    inline Value* getPMask()            const { return getOperand(2); }
    inline void setPMask(Value* pmask)        { setOperand(2, pmask); }
    inline Value* getRed()              const { return getOperand(3); }
    inline Value* getGreen()            const { return getOperand(4); }
    inline Value* getBlue()             const { return getOperand(5); }
    inline Value* getAlpha()            const { return getOperand(6); }
    inline Value* getDepth()            const { return getOperand(7); }
    inline Value* getStencil()          const { return getOperand(8); }
    inline Value* getRTIndex()          const { return getOperand(9); }
    inline Value* getBlendStateIndex()  const { return getOperand(10); }
    inline Value* getSampleIndex()      const { return getOperand(15); }

    inline bool isImmRTIndex() const
    { return llvm::isa<llvm::ConstantInt>(getRTIndex()); }

    inline int getRTIndexImm()  const
    { return (int)llvm::cast<llvm::ConstantInt>(getRTIndex())->getZExtValue(); }

    inline bool hasMask()    const
    {
        return valueToImm64(getOperand(11)) != 0;
    }
    inline bool hasDepth()   const
    {
        return valueToImm64(getOperand(12)) != 0;
    }
    inline bool hasStencil() const
    {
        return valueToImm64(getOperand(13)) != 0;
    }
    inline bool perSample()  const
    {
        return valueToImm64(getOperand(14)) != 0;
    }

    void setPerSample()
    {
        Value* btrue = ConstantInt::get(
            Type::getInt1Ty(this->getParent()->getContext()), 1);
        setOperand(14, btrue);
    }
    void setSampleIndex(Value* v)       { setOperand(15, v); }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        switch (I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_RTWrite:
            return true;
        default: return false;
        }
    }
    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class RTDualBlendSourceIntrinsic : public GenIntrinsicInst {
protected:
    Value* getOperand(unsigned i) const { return GenIntrinsicInst::getOperand(i); }
    void setOperand(unsigned i, Value* v) { return GenIntrinsicInst::setOperand(i, v); }

public:
    inline Value* getSource0Alpha()     const { return nullptr; }
    inline Value* getOMask()            const { return getOperand(0); }
    inline Value* getPMask()            const { return getOperand(1); }
    inline void setPMask(Value* mask)         { setOperand(1, mask);  }
    inline Value* getRed0()             const { return getOperand(2); }
    inline Value* getGreen0()           const { return getOperand(3); }
    inline Value* getBlue0()            const { return getOperand(4); }
    inline Value* getAlpha0()           const { return getOperand(5); }
    inline Value* getRed1()             const { return getOperand(6); }
    inline Value* getGreen1()           const { return getOperand(7); }
    inline Value* getBlue1()            const { return getOperand(8); }
    inline Value* getAlpha1()           const { return getOperand(9); }
    inline Value* getDepth()            const { return getOperand(10); }
    inline Value* getStencil()          const { return getOperand(11); }
    inline Value* getRTIndex()          const { return getOperand(12); }
    inline Value* getSampleIndex()      const { return getOperand(17); }

    inline int getRTIndexImm()  const
    { return (int)llvm::cast<llvm::ConstantInt>(getRTIndex())->getZExtValue(); }

    inline bool hasMask()    const
    {
        return valueToImm64(getOperand(13)) != 0;
    }
    inline bool hasDepth()   const
    {
        return valueToImm64(getOperand(14)) != 0;
    }
    inline bool hasStencil() const
    {
        return valueToImm64(getOperand(15)) != 0;
    }
    inline bool perSample()  const
    {
        return valueToImm64(getOperand(16)) != 0;
    }

    void setPerSample()
    {
        Value* btrue = ConstantInt::get(
            Type::getInt1Ty(this->getParent()->getContext()), 1);
        setOperand(16, btrue);
    }
    void setSampleIndex(Value* v)       { setOperand(17, v); }

    static inline bool classof(const GenIntrinsicInst *I) {
        switch (I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_RTDualBlendSource:
            return true;
        default: return false;
        }
    }
    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};


class SamplerGatherIntrinsic : public GenIntrinsicInst {
public:
    inline Value* getTextureValue() const { return getOperand(getNumOperands() - 7); }
    inline Value* getSamplerValue() const { return getOperand(getNumOperands() - 6); }

    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_gather4ptr:
        case GenISAIntrinsic::GenISA_gather4Cptr:
        case GenISAIntrinsic::GenISA_gather4POptr:
        case GenISAIntrinsic::GenISA_gather4POCptr:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class InfoIntrinsic : public GenIntrinsicInst {
public:
    static inline bool classof(const GenIntrinsicInst *I) {
        switch (I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_sampleinfoptr:
        case GenISAIntrinsic::GenISA_resinfoptr:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class SamplerLoadIntrinsic : public GenIntrinsicInst {
public:
    inline unsigned int getTextureIndex() const { return getNumOperands() - 5; }
    inline Value* getTextureValue() const { return getOperand(getTextureIndex()); }

    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_ldptr:
        case GenISAIntrinsic::GenISA_ldmsptr:
        case GenISAIntrinsic::GenISA_ldmcsptr:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class LdMSIntrinsic : public SamplerLoadIntrinsic {
public:
    inline Value* getImmOffset(unsigned int i)
    {
        return getOperand(getNumArgOperands() - 3 + i);
    }
    inline void setImmOffset(unsigned int i, Value* val)
    {
        return setOperand(getNumArgOperands() - 3 + i, val);
    }
    inline Value* getCoordinate(unsigned int i)
    {
        return getOperand(getNumArgOperands() - 8 + i);
    }
    inline void setCoordinate(unsigned int i, Value* val)
    {
        return setOperand(getNumArgOperands() - 8 + i, val);
    }
    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_ldmsptr:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};


class LdmcsInstrinsic : public SamplerLoadIntrinsic
{
public:
    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_ldmcsptr:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class LdmsInstrinsic : public SamplerLoadIntrinsic
{
public:
    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_ldmsptr:
            return true;
        default: return false;
        }
    }
    unsigned int getNumMcsOperands()
    {
        return 2;
    }
    Value* getMcsOperand(unsigned int i) { return getOperand(i + 1); }
    Value* getSampleIndexValue() { return getOperand(0); }
    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class SampleIntrinsic : public GenIntrinsicInst {
public:
    inline Value* getSamplerValue() const
    {
        unsigned int index = getSamplerIndex();
        return getOperand(index);
    }

    inline Value* getTextureValue() const
    {
        unsigned int index = getTextureIndex();
        return getOperand(index);
    }

    inline unsigned int getTextureIndex() const
    {
        unsigned int index = getNumOperands() - 6;
        if (IsLODInst())
        {
            index = getNumOperands() - 3;
        }

        return index;
    }

    inline int getSamplerIndex() const
    {
        unsigned int index = getNumOperands() - 5;
        if(IsLODInst())
        {
            index = getNumOperands() - 2;
        }
        return index;
    }


    inline bool IsLODInst() const
    {
        return getIntrinsicID() == GenISAIntrinsic::GenISA_lodptr;
    }


    bool ZeroLOD() const
    {
        if(getIntrinsicID() == GenISAIntrinsic::GenISA_sampleLptr)
        {
            if(ConstantFP* lod = dyn_cast<ConstantFP>(getOperand(0)))
            {
                return lod->isZero();
            }
        }
        else if(getIntrinsicID() == GenISAIntrinsic::GenISA_sampleLCptr)
        {
            if(ConstantFP* lod = dyn_cast<ConstantFP>(getOperand(1)))
            {
                return lod->isZero();
            }
        }
        return false;
    }

    bool IsDerivative() const
    {
        switch(getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_sampleptr:
        case GenISAIntrinsic::GenISA_sampleKillPix:
        case GenISAIntrinsic::GenISA_sampleBptr:
        case GenISAIntrinsic::GenISA_sampleCptr:
        case GenISAIntrinsic::GenISA_sampleBCptr:
        case GenISAIntrinsic::GenISA_lodptr:
            return true;
        default:
            break;
        }
        return false;
    }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        switch(I->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_sampleptr:
        case GenISAIntrinsic::GenISA_sampleBptr:
        case GenISAIntrinsic::GenISA_sampleCptr:
        case GenISAIntrinsic::GenISA_sampleDptr:
        case GenISAIntrinsic::GenISA_sampleDCptr:
        case GenISAIntrinsic::GenISA_sampleLptr:
        case GenISAIntrinsic::GenISA_sampleLCptr:
        case GenISAIntrinsic::GenISA_sampleBCptr:
        case GenISAIntrinsic::GenISA_lodptr:
        case GenISAIntrinsic::GenISA_sampleKillPix:
            return true;
        default: return false;
        }
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class LdRawIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if(ID == GenISAIntrinsic::GenISA_ldraw_indexed ||
            ID == GenISAIntrinsic::GenISA_ldrawvector_indexed)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
    inline Value* getOffsetValue() const
    {
        return getOperand(1);
    }
    inline Value* getResourceValue() const{
        return getOperand(0);
    }
    inline Value* getAlignmentValue() const {
        return getOperand(2);
    }
    inline unsigned int getAlignment() const {
        return static_cast<unsigned int>(cast<ConstantInt>(getAlignmentValue())->getZExtValue());
    }
    inline bool isVolatile() const {
        assert(isa<ConstantInt>(getOperand(3)));
        ConstantInt* val = dyn_cast<ConstantInt>(getOperand(3));
        const bool isVolatile = val ? val->getZExtValue() : false;
        return isVolatile;
    }


    inline void setAlignment(unsigned int alignment)
    {
        Value* newAlignment = ConstantInt::get(getOperand(2)->getType(), alignment);
        setOperand(2, newAlignment);
    }

    inline void setOffsetValue(Value* V){
        setOperand(1, V);
    }
};

class StoreRawIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_storeraw_indexed ||
            ID == GenISAIntrinsic::GenISA_storerawvector_indexed)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
    inline Value* getAlignmentValue() const {
        return getOperand(3);
    }
    inline Value* getOffsetValue() const
    {
        return getOperand(1);
    }
    inline Value* getResourceValue() const {
        return getOperand(0);
    }
    inline unsigned int getAlignment() const {
        assert(isa<ConstantInt>(getAlignmentValue()));
        ConstantInt* val = dyn_cast<ConstantInt>(getAlignmentValue());
        const unsigned int alignment = val ? int_cast<unsigned int>(val->getZExtValue()) : 1;
        return alignment;
    }
    inline bool isVolatile() const {
        assert(isa<ConstantInt>(getOperand(4)));
        ConstantInt* val = dyn_cast<ConstantInt>(getOperand(4));
        const bool isVolatile = val ? val->getZExtValue() : false;
        return isVolatile;
    }

    inline void setOffsetValue(Value* V) {
        setOperand(1, V);
    }
    inline void setAlignment(unsigned int alignment)
    {
        Value* newAlignment = ConstantInt::get(getOperand(3)->getType(), alignment);
        setOperand(3, newAlignment);
    }
};

class AtomicRawIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_intatomicraw ||
            ID == GenISAIntrinsic::GenISA_intatomicrawA64 ||
            ID == GenISAIntrinsic::GenISA_floatatomicraw ||
            ID == GenISAIntrinsic::GenISA_floatatomicrawA64 ||
            ID == GenISAIntrinsic::GenISA_icmpxchgatomicraw ||
            ID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
            ID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
            ID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    inline Value* getResourceValue() const {
        return getOperand(0);
    }
};

class AtomicStructuredIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_dwordatomicstructured ||
            ID == GenISAIntrinsic::GenISA_floatatomicstructured ||
            ID == GenISAIntrinsic::GenISA_cmpxchgatomicstructured ||
            ID == GenISAIntrinsic::GenISA_fcmpxchgatomicstructured)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    inline Value* getResourceValue() const {
        return getOperand(0);
    }

    inline Value* getArrayIdx() const {
        return getOperand(1);
    }

    inline void setOffsetValue(Value* V) {
        setOperand(1, V);
    }
};

class AtomicTypedIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_intatomictyped ||
            ID == GenISAIntrinsic::GenISA_icmpxchgatomictyped )
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    inline Value* getResourceValue() const {
        return getOperand(0);
    }
};

class AtomicCounterIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_atomiccounterinc ||
            ID == GenISAIntrinsic::GenISA_atomiccounterpredec)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    inline Value* getResourceValue() const {
        return getOperand(0);
    }
};

class SGVIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        if(ID == GenISAIntrinsic::GenISA_DCL_GSsystemValue ||
            ID == GenISAIntrinsic::GenISA_DCL_SystemValue)
        {
            return true;
        }
        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
    inline IGC::SGVUsage getUsage() const
    {
        IGC::SGVUsage usage;
        if(getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_GSsystemValue)
        {
            // TODO: deprecate usage of GSSystemValue intrinsic
            usage = static_cast<IGC::SGVUsage>(
                llvm::cast<llvm::ConstantInt>(getOperand(1))->getZExtValue());
        }
        else
        {
            usage = static_cast<IGC::SGVUsage>(
                llvm::cast<llvm::ConstantInt>(getOperand(0))->getZExtValue());
        }
        return usage;
    }
};

class WavePrefixIntrinsic : public GenIntrinsicInst
{
public:
    Value  *getSrc() const { return getOperand(0); }
    IGC::WaveOps getOpKind() const
    {
        return static_cast<IGC::WaveOps>(getImm64Operand(1));
    }
    bool isInclusiveScan() const
    {
        return getImm64Operand(2) != 0;
    }
    Value *getMask() const { return getOperand(3); }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        return I->getIntrinsicID() == GenISAIntrinsic::GenISA_WavePrefix;
    }
    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class QuadPrefixIntrinsic : public GenIntrinsicInst
{
public:
    Value  *getSrc() const { return getOperand(0); }
    IGC::WaveOps getOpKind() const
    {
        return static_cast<IGC::WaveOps>(getImm64Operand(1));
    }
    bool isInclusiveScan() const
    {
        return getImm64Operand(2) != 0;
    }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        return I->getIntrinsicID() == GenISAIntrinsic::GenISA_QuadPrefix;
    }
    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};


template <class X, class Y>
inline bool isa(const Y &Val, GenISAIntrinsic::ID id)
{
    if (isa<X>(Val))
    {
        X* r = cast<X>(Val);
        if (r->getIntrinsicID() == id)
        {
            return true;
        }
    }
    return false;
}

template <class X, class Y>
inline typename cast_retty<X, Y *>::ret_type
dyn_cast(Y *Val, GenISAIntrinsic::ID id)
{
    if (isa<X>(Val))
    {
        X* r = cast<X>(Val);
        if (r->getIntrinsicID() == id)
        {
            return r;
        }
    }
    return nullptr;
}

template <class X, class Y>
inline typename cast_retty<X, Y>::ret_type
dyn_cast(Y &Val, GenISAIntrinsic::ID id)
{
    if (isa<X>(Val))
    {
        typename cast_retty<X, Y>::ret_type r = cast<X>(Val);
        if (r->getIntrinsicID() == id)
        {
            return r;
        }
    }
    return nullptr;
}

// TODO: add more classes to make our intrinsics easier to use

}
