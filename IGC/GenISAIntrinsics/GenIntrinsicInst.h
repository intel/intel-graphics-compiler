/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

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
#include "common/EmUtils.h"
#include "common/Types.hpp"
#include "GenIntrinsics.h"
#include "Probe/Assertion.h"

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
    inline uint32_t valueToImm32(Value* cv) const
    {
        uint64_t v = valueToImm64(cv);
        return int_cast<uint32_t>(v);
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
        IGC_ASSERT(isa<ConstantInt>(getOperand(idx)));
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
    bool hasRef() const
    {
        switch (getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_gather4Cptr:
        case GenISAIntrinsic::GenISA_gather4POCptr:
            return true;
        default:
            break;
        }
        return false;
    }


    inline unsigned int getTextureIndex() const { return getNumOperands() - 7; }
    inline unsigned int getSamplerIndex() const { return getNumOperands() - 6; }
    inline Value* getTextureValue() const { return getOperand(getTextureIndex()); }
    inline Value* getSamplerValue() const { return getOperand(getSamplerIndex()); }

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
    inline unsigned int getCoordinateIndex(unsigned int i) const
    {
        if (i == 2)
        {
            return 3;
        }
        else if (i < 2)
        {
            return i;
        }
        else
        {
            IGC_ASSERT(0);
            return UINT_MAX;
        }
    }
    inline unsigned int getTextureIndex() const { return getNumOperands() - 5; }
    inline unsigned int getOffsetIndex(unsigned int i) const
    {
        return 5 + i;
    }

    inline Value* getTextureValue() const { return getOperand(getTextureIndex()); }
    inline Value* getCoordinateValue(unsigned int i) const { return getOperand(getCoordinateIndex(i)); }
    inline void   setCoordinateValue(unsigned int i, Value* val) { setOperand(getCoordinateIndex(i), val); }
    inline Value* getOffsetValue(unsigned int i) const { return getOperand(getOffsetIndex(i)); }
    inline void   setOffsetValue(unsigned int i, Value* val) { setOperand(getOffsetIndex(i), val); }

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
    bool hasRef() const
    {
        switch (getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_sampleCptr:
        case GenISAIntrinsic::GenISA_sampleDCptr:
        case GenISAIntrinsic::GenISA_sampleLCptr:
        case GenISAIntrinsic::GenISA_sampleBCptr:
            return true;
        default:
            break;
        }
        return false;
    }

    bool hasBias() const
    {
        switch (getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_sampleBptr:
        case GenISAIntrinsic::GenISA_sampleBCptr:
            return true;
        default:
            break;
        }
        return false;
    }

    unsigned int getBiasIndex() const
    {
        unsigned int shift = hasRef() ? 1 : 0;
        return hasBias() ? shift : std::numeric_limits<unsigned int>::max();
    }

    Value* getBiasValue() const
    {
        unsigned int index = getBiasIndex();
        return hasBias() ? getOperand(index) : nullptr;
    }

    bool hasLod() const
    {
        switch (getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_sampleLptr:
        case GenISAIntrinsic::GenISA_sampleLCptr:
            return true;
        default:
            break;
        }
        return false;
    }

    unsigned int getLodIndex() const
    {
        unsigned int shift = hasRef() ? 1 : 0;
        return hasLod() ? shift : std::numeric_limits<unsigned int>::max();
    }

    Value* getLodValue() const
    {
        unsigned int index = getLodIndex();
        return hasLod() ? getOperand(index) : nullptr;
    }

    bool hasImmediateOffsets() const
    {
        return true;
    }

    unsigned int getImmediateOffsetsIndex() const
    {
        return hasImmediateOffsets() ?
            getNumOperands() - 4 : std::numeric_limits<unsigned int>::max();
    }

    Value* getImmediateOffsetsValue(unsigned int coordIndex) const
    {
        unsigned int operandCoordIndex = getImmediateOffsetsIndex() + coordIndex;
        return hasImmediateOffsets() ? getOperand(operandCoordIndex) : nullptr;
    }

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
        IGC_ASSERT(isa<ConstantInt>(getOperand(3)));
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
    inline Value* getStoreValue() const {
        return getOperand(2);
    }
    inline unsigned int getAlignment() const {
        IGC_ASSERT(isa<ConstantInt>(getAlignmentValue()));
        ConstantInt* val = dyn_cast<ConstantInt>(getAlignmentValue());
        const unsigned int alignment = val ? int_cast<unsigned int>(val->getZExtValue()) : 1;
        return alignment;
    }
    inline bool isVolatile() const {
        IGC_ASSERT(isa<ConstantInt>(getOperand(4)));
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

    inline IGC::AtomicOp getAtomicOp() const {
        GenISAIntrinsic::ID ID = getIntrinsicID();
        switch (ID)
        {
        case GenISAIntrinsic::GenISA_intatomicraw:
        case GenISAIntrinsic::GenISA_intatomicrawA64:
        case GenISAIntrinsic::GenISA_floatatomicraw:
        case GenISAIntrinsic::GenISA_floatatomicrawA64:
            return static_cast<IGC::AtomicOp>(getImm64Operand(3));
        case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
            return IGC::EATOMIC_CMPXCHG;
        case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
            return IGC::EATOMIC_CMPXCHG64;
        case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
        case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
            return IGC::EATOMIC_FCMPWR;
        default:
            IGC_ASSERT_MESSAGE(false, "Unexpected atomic raw intrinisc!");
        }
        return IGC::EATOMIC_UNDEF;
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

// This is just a meta intrinsic that encapsulates the idea of intrinsics
// that contain continuation IDs.
class ContinuationHLIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        switch (ID)
        {
        case GenISAIntrinsic::GenISA_TraceRayAsyncHL:
        case GenISAIntrinsic::GenISA_CallShaderHL:
            return true;
        default:
            break;
        }

        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    uint32_t getContinuationID() const {
        return (uint32_t)cast<ConstantInt>(getOperand(0))->getZExtValue();
    }
    void setContinuationID(uint32_t ID) {
        IRBuilder<> IRB(this->getContext());
        setOperand(0, IRB.getInt32(ID));
    }
    bool isValidContinuationID() const {
        return getContinuationID() != (uint32_t)-1;
    }

    Function* getContinuationFn() const {
        return cast<Function>(getOperand(1)->stripPointerCasts());
    }
    void setContinuationFn(Function *F) {
        IRBuilder<> IRB(this->getContext());
        auto* Ty = getOperand(1)->getType();
        auto* Cast = IRB.CreatePointerBitCastOrAddrSpaceCast(F, Ty);
        setOperand(1, Cast);
    }
    bool isValidContinuationFn() const {
        return isa<Function>(getContinuationFn());
    }
};

class TraceRayAsyncHLIntrinsic : public ContinuationHLIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const ContinuationHLIntrinsic *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayAsyncHL;
    }

    static inline bool classof(const Value *V) {
        return isa<ContinuationHLIntrinsic>(V) &&
               classof(cast<ContinuationHLIntrinsic>(V));
    }

    Value* getBVH()  const { return getOperand(2); }
    Value* getFlag() const { return getOperand(3); }
    Value* getMask() const { return getOperand(4); }
    Value* getRayContributionToHitGroupIndex() const {
        return getOperand(5);
    }
    Value* getMultiplierForGeometryContributionToHitGroupIndex() const {
        return getOperand(6);
    }
    Value* getMissShaderIndex() const { return getOperand(7); }
    Value* getRayInfo(uint32_t Idx) const {
        IGC_ASSERT_MESSAGE(Idx < 8, "Index out of range!");
        return getOperand(8 + Idx);
    }
    // Get the 0 - X, 1 - Y, or 2 - Z component of the ray origin
    Value* getRayOrig(uint32_t Dim) const {
        IGC_ASSERT_MESSAGE(Dim < 3, "Dim out of range!");
        return getOperand(8 + Dim);
    }
    // Get the 0 - X, 1 - Y, or 2 - Z component of the ray direction
    Value* getRayDir(uint32_t Dim) const {
        IGC_ASSERT_MESSAGE(Dim < 3, "Dim out of range!");
        return getOperand(11 + Dim);
    }
    Value* getTMin() const { return getOperand(14); }
    Value* getTMax() const { return getOperand(15); }
    Value* getPayload() const { return getOperand(16); }
};

class CallShaderHLIntrinsic : public ContinuationHLIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const ContinuationHLIntrinsic* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_CallShaderHL;
    }

    static inline bool classof(const Value* V) {
        return isa<ContinuationHLIntrinsic>(V) &&
            classof(cast<ContinuationHLIntrinsic>(V));
    }

    Value* getShaderIndex()         const { return getOperand(2); }
    Value* getParameter()           const { return getOperand(3); }
};


class TraceRayIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        switch (ID)
        {
        case GenISAIntrinsic::GenISA_TraceRaySync:
        case GenISAIntrinsic::GenISA_TraceRayAsync:
            return true;
        default:
            break;
        }

        return false;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getGlobalBufferPointer() const { return getOperand(0); }
    Value* getPayload()             const { return getOperand(1); }
};

class TraceRayAsyncIntrinsic : public TraceRayIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayAsync;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class TraceRaySyncIntrinsic : public TraceRayIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRaySync;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class BTDIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_BindlessThreadDispatch;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getGlobalBufferPointer() const { return getOperand(0); }
    Value* getStackID()             const { return getOperand(1); }
    Value* getShaderRecordAddress() const { return getOperand(2); }
};

class StackIDReleaseIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_StackIDRelease;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getStackID() const { return getOperand(0); }
};

class ReportHitHLIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_ReportHitHL;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getTHit()  const { return getOperand(0); }
    Value* getHitKind() const { return getOperand(1); }
    Value* getAttributes() const { return getOperand(2); }
};

class SpillValueIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_SpillValue;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    unsigned getDataIdx() const { return 0; }
    unsigned getOffsetIdx() const { return 1; }

    Value* getData()     const { return getOperand(getDataIdx()); }
    void   setData(Value *V)   { setOperand(getDataIdx(), V); }
    uint64_t getOffset() const {
        return cast<ConstantInt>(getOperand(getOffsetIdx()))->getZExtValue();
    }
    void setOffset(uint64_t Offset) {
        Type* Ty = getOperand(getOffsetIdx())->getType();
        setOperand(1, ConstantInt::get(Ty, Offset));
    }
};

class RayInfoIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_RayInfo ||
               ID == GenISAIntrinsic::GenISA_RayTCurrent;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    IGC::DISPATCH_SHADER_RAY_INFO_TYPE getInfoKind() const {
        uint64_t Val = cast<ConstantInt>(getOperand(0))->getZExtValue();
        return (IGC::DISPATCH_SHADER_RAY_INFO_TYPE)Val;
    }

    uint32_t getDim() const {
        return (uint32_t)cast<ConstantInt>(getOperand(1))->getZExtValue();
    }
};

class FillValueIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_FillValue;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    uint64_t getOffset() const {
        return cast<ConstantInt>(getOperand(0))->getZExtValue();
    }
    void setOffset(uint64_t Offset) {
        Type* Ty = getOperand(0)->getType();
        setOperand(0, ConstantInt::get(Ty, Offset));
    }
};

class AllocaNumberIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_AllocaNumber;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    uint64_t getNumber() const {
        return cast<ConstantInt>(getOperand(0))->getZExtValue();
    }
};

class LocalBufferPointerIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_LocalBufferPointer;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class GlobalBufferPointerIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_GlobalBufferPointer;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class InlineDataIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_InlinedData;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    uint64_t getArg() const {
        return cast<ConstantInt>(getOperand(0))->getZExtValue();
    }
};

// Common methods for X and Y components
class TileIntrinsic : public GenIntrinsicInst {
public:
    Value* getTID() const { return getOperand(0); }
    uint32_t getTileXDim() const {
        return (uint32_t)cast<ConstantInt>(getOperand(1))->getZExtValue();
    }
    uint32_t getSubtileXDim() const {
        return (uint32_t)cast<ConstantInt>(getOperand(2))->getZExtValue();
    }
    uint32_t getSubtileYDim() const {
        return (uint32_t)cast<ConstantInt>(getOperand(3))->getZExtValue();
    }
};

class TileXIntrinsic : public TileIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TileXOffset;
    }

    static inline bool classof(const Value* V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class TileYIntrinsic : public TileIntrinsic {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TileYOffset;
    }

    static inline bool classof(const Value* V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class SWStackPtrIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_SWStackPtr;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getAddr() const { return getOperand(0); }
};

class HitKindIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_HitKind;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }
};

class AllocateRayQueryIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_AllocateRayQuery;
    }

    static inline bool classof(const Value* V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getFlags()  const { return getOperand(0); }
};

class RayQueryInstrisicBase : public GenIntrinsicInst
{
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        switch (ID)
        {
        case GenISAIntrinsic::GenISA_TraceRayInlineHL:
        case GenISAIntrinsic::GenISA_TraceRaySyncProceedHL:
        case GenISAIntrinsic::GenISA_TraceRaySyncProceed:
        case GenISAIntrinsic::GenISA_ShadowMemoryToSyncStack:
        case GenISAIntrinsic::GenISA_SyncStackToShadowMemory:
        case GenISAIntrinsic::GenISA_TraceRayInlineAbort:
        case GenISAIntrinsic::GenISA_TraceRayInlineCommittedStatus:
        case GenISAIntrinsic::GenISA_TraceRayInlineCandidateType:
        case GenISAIntrinsic::GenISA_TraceRayInlineRayInfo:
        case GenISAIntrinsic::GenISA_TraceRayInlineCommitNonOpaqueTriangleHit:
        case GenISAIntrinsic::GenISA_TraceRayInlineCommitProceduralPrimitiveHit:
            return true;
        default:
            break;
        }

        return false;
    }

    static inline bool classof(const Value* V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getQueryObjIndex() const { return getOperand(0); }
};

class TraceRayInlineHLIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineHL;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }

    Value* getBVH()  const { return getOperand(1); }
    Value* getFlag() const { return getOperand(2); }
    Value* getMask() const { return getOperand(3); }

    Value* getRayInfo(uint32_t Idx) const {
        IGC_ASSERT_MESSAGE(Idx < 8, "Index out of range!");
        return getOperand(4 + Idx);
    }
    // Get the 0 - X, 1 - Y, or 2 - Z component of the ray origin
    Value* getRayOrig(uint32_t Dim) const {
        IGC_ASSERT_MESSAGE(Dim < 3, "Dim out of range!");
        return getOperand(4 + Dim);
    }
    // Get the 0 - X, 1 - Y, or 2 - Z component of the ray direction
    Value* getRayDir(uint32_t Dim) const {
        IGC_ASSERT_MESSAGE(Dim < 3, "Dim out of range!");
        return getOperand(7 + Dim);
    }
    Value* getTMin() const { return getOperand(10); }
    Value* getTMax() const { return getOperand(11); }
};

class TraceRaySyncProceedHLIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRaySyncProceedHL;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class TraceRaySyncProceedIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRaySyncProceed;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQueryAbortIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineAbort;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQueryCommittedStatusIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineCommittedStatus;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQueryCandidateTypeIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineCandidateType;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQueryCommitNonOpaqueTriangleHit : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineCommitNonOpaqueTriangleHit;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQueryCommitProceduralPrimitiveHit : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineCommitProceduralPrimitiveHit;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }

    Value* getTHit() { return getOperand(1); }
};

class RayQueryInfoIntrinsic : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_TraceRayInlineRayInfo;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }

    uint32_t getInfoKind() const {
        return (uint32_t)cast<ConstantInt>(getOperand(1))->getZExtValue();
    }

    Value* getDim() const {return getOperand(2);}
};

class RayQueryShadowMemoryToSyncStack : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_ShadowMemoryToSyncStack;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class RayQuerySyncStackToShadowMemory : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_SyncStackToShadowMemory;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }

    Value* getProceedReturnVal() const { return getOperand(1); }
};

class RayQueryReadTraceRaySync : public RayQueryInstrisicBase {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const RayQueryInstrisicBase* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_ReadTraceRaySync;
    }

    static inline bool classof(const Value* V) {
        return isa<RayQueryInstrisicBase>(V) && classof(cast<RayQueryInstrisicBase>(V));
    }
};

class GetShaderRecordPtrIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_GetShaderRecordPtr;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Function* getContinuationFn() const {
        return cast<Function>(getOperand(0)->stripPointerCasts());
    }
};

class PayloadPtrIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_PayloadPtr;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getPayloadPtr() const   { return getOperand(0);    }
    void   setPayloadPtr(Value *V) { return setOperand(0, V); }
};

class ContinuationSignpostIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst *I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_ContinuationSignpost;
    }

    static inline bool classof(const Value *V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    Value* getFrameAddr() const { return getOperand(0); }
    Value* getOffset() const    { return getOperand(1); }

    void setOffset(uint32_t Offset) {
        auto& C = this->getContext();
        setOperand(1, ConstantInt::get(Type::getInt32Ty(C), Offset));
    }
};

class StaticConstantPatchIntrinsic : public GenIntrinsicInst {
public:
    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const GenIntrinsicInst* I) {
        GenISAIntrinsic::ID ID = I->getIntrinsicID();
        return ID == GenISAIntrinsic::GenISA_staticConstantPatchValue;
    }

    static inline bool classof(const Value* V) {
        return isa<GenIntrinsicInst>(V) && classof(cast<GenIntrinsicInst>(V));
    }

    llvm::StringRef getPatchName() const
    {
        llvm::ConstantDataArray* constantVal = llvm::dyn_cast<llvm::ConstantDataArray>(getOperand(0));
        return constantVal->getAsString();
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
