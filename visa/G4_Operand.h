/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_OPERAND_H
#define G4_OPERAND_H

#include "Assertions.h"
#include "G4_Declare.h"
#include "G4_Opcode.h"
#include "G4_Register.h"
#include "include/RelocationInfo.h"

#include <iostream>
#include <vector>

// RegionWH and RegionV are special for the different modes of source register
// indirect addressing RegionWH = <width, horzStride>, we set vertStride to
// UNDEFINED_SHORT RegionV = <horzStride>, we set both vertStride and width to
// UNDEFINED_SHORT
//
// FIXME: Move it into vISA namespace and fix all unscoped references.
struct RegionDesc {
  const uint16_t vertStride;
  const uint16_t width;
  const uint16_t horzStride;

  RegionDesc(uint16_t vs, uint16_t w, uint16_t hs)
      : vertStride(vs), width(w), horzStride(hs) {
    vISA_ASSERT(isLegal(), "illegal region desc");
  }
  void *operator new(size_t sz, vISA::Mem_Manager &m) { return m.alloc(sz); }

  // The legal values for Width are {1, 2, 4, 8, 16}.
  // The legal values for VertStride are {0, 1, 2, 4, 8, 16, 32}.
  // The legal values for HorzStride are {0, 1, 2, 4}.
  bool isLegal() const { return isLegal(vertStride, width, horzStride); }

  static bool isLegal(unsigned vs, unsigned w, unsigned hs);

  enum RegionDescKind {
    RK_Other,   // all others like <4; 2, 1> etc.
    RK_Stride0, // <0;1,0> aka scalar
    RK_Stride1, // <1;1,0> aka contiguous
    RK_Stride2, // <2;1,0>
    RK_Stride4  // <4;1,0>
  };

  // Determine the region description kind. Strided case only.
  static RegionDescKind getRegionDescKind(uint16_t size, uint16_t vstride,
                                          uint16_t width, uint16_t hstride);

  bool isRegionWH() const {
    return vertStride == UNDEFINED_SHORT && width != UNDEFINED_SHORT;
  }
  bool isRegionV() const {
    return vertStride == UNDEFINED_SHORT && width == UNDEFINED_SHORT;
  }
  bool isRegion110() const {
    return vertStride == 1 && width == 1 && horzStride == 0;
  }
  bool isScalar() const {
    return (vertStride == 0 && horzStride == 0) ||
           (width == 1 && vertStride == 0);
  } // to support decompression
  bool isRegionSW() const {
    return vertStride != UNDEFINED_SHORT && width == UNDEFINED_SHORT &&
           horzStride == UNDEFINED_SHORT;
  }
  bool isEqual(const RegionDesc *r) const {
    return vertStride == r->vertStride && width == r->width &&
           horzStride == r->horzStride;
  } // to support re-compression
  void emit(std::ostream &output) const;
  bool isPackedRegion() const {
    return ((horzStride == 0 && vertStride <= 1) ||
            (horzStride == 1 && vertStride <= width));
  }
  bool isFlatRegion() const {
    return (isScalar() || vertStride == horzStride * width);
  }
  bool isRepeatRegion(unsigned short execSize) const {
    return (!isScalar() &&
            (execSize > width && vertStride < horzStride * width));
  }

  // Contiguous regions are:
  // (1) ExSize is 1, or
  // (2) <1; 1, *> with arbitrary ExSize, or
  // (3) <N; N, 1> with arbitrary ExSize, or
  // (4) <*; N, 1> with ExSize == N.
  //
  // A region is contiguous iff sequence
  // { f(0, 0), f(0, 1), ..., f(1, 0), ..., f(ExSize / width - 1, width - 1) }
  // has a common difference 1, where
  //
  // f(i, j) = i x vstride + j x hstride
  //
  // for 0 <= i < ExSize / width and 0 <= j < width
  bool isContiguous(unsigned ExSize) const;
  bool isSingleNonUnitStride(uint32_t execSize, uint16_t &stride) const;
  bool isSingleStride(uint32_t execSize, uint16_t &stride) const;
  bool isSingleStride(uint32_t execSize) const {
    uint16_t stride = 0;
    return isSingleStride(execSize, stride);
  }
};

namespace vISA {

// Forward declarations.
class IR_Builder;
// Forward declarations for the instruction classes.
class G4_INST;
class G4_InstSend;
class G4_FillIntrinsic;
class G4_SpillIntrinsic;
class G4_PseudoMovInstrinsic;
class G4_InstDpas;
// Forward declarations for the concrete operand classes. We need them here
// because the base class has APIs that perform downcasts to the concrete type.
class G4_Imm;
class G4_Reloc_Imm;
class G4_Label;
class G4_AddrExp;
class G4_DstRegRegion;
class G4_SrcRegRegion;
class G4_CondMod;
class G4_Predicate;

class G4_Operand {
  // TODO: revisit the decision to allow G4_INST to access internal G4_Operand
  // field (mainly for bound?)
  friend class G4_INST;
  friend class G4_InstSend;
  friend class G4_InstIntrinsic;
  friend class G4_FillIntrinsic;
  friend class G4_SpillIntrinsic;
  friend class G4_PseudoMovInstrinsic;
  friend class G4_InstDpas;

public:
  enum Kind : unsigned char {
    immediate,
    srcRegRegion,
    dstRegRegion,
    predicate, // instruction predicate
    condMod,   // condition modifier
    addrExp,
    label
  };
  virtual ~G4_Operand() {}

protected:
  G4_INST *inst = nullptr;

  // FIXME: It's redundant to keep both top_dcl and base. We should have a union
  // based on operand kind.
  G4_Declare *top_dcl = nullptr;
  G4_VarBase *base = nullptr;

  // TODO: Should we track footprint at word granularity instead?
  uint64_t bitVec[2]; // bit masks at byte granularity (for flags, at bit
                      // granularity)

  // Group byte-sized fields together.
  Kind kind;
  G4_Type type = Type_UNDEF;
  bool rightBoundSet = false;
  G4_AccRegSel accRegSel = ACC_UNDEFINED;

  // [left_bound, right_bound] describes the region in the root variable that
  // this operand touches. for variables and addresses:
  //  lb = offset of the first byte of the first element
  //  rb = offset of the last byte of the last element
  //  for non-send instructions, (rb - lb) < 64 always holds since operand can't
  //  cross 2GRF boundary for send instructions, rb is determined by the
  //  message/response length
  // for flags:
  //  lb = bit offset of the first flag bit
  //  rb = bit offset of the last flag bit
  //  (rb - lb) < 32 always holds for flags
  //  for predicate and conditonal modifiers, the bounds are also effected by the
  //  quarter control
  uint16_t left_bound = 0;
  uint16_t right_bound = 0;
  uint16_t byteOffset = 0;

  explicit G4_Operand(Kind k, G4_Type ty = Type_UNDEF,
                      G4_VarBase *base = nullptr)
      : base(base), kind(k), type(ty){
    bitVec[0] = bitVec[1] = 0;
  }

  G4_Operand(Kind k, G4_VarBase *base)
      : base(base), kind(k) {
    bitVec[0] = bitVec[1] = 0;
  }

public:
  Kind getKind() const { return kind; }
  G4_Type getType() const { return type; }
  unsigned short getTypeSize() const { return TypeSize(getType()); }

  bool isImm() const { return kind == Kind::immediate; }
  bool isVectImm() const {
    return (isImm() && (type == Type_UV || type == Type_V || type == Type_VF));
  }
  bool isSrcRegRegion() const { return kind == Kind::srcRegRegion; }
  bool isDstRegRegion() const { return kind == Kind::dstRegRegion; }
  bool isRegRegion() const {
    return kind == srcRegRegion || kind == dstRegRegion;
  }
  bool isPredicate() const { return kind == predicate; }
  bool isCondMod() const { return kind == condMod; }
  bool isLabel() const { return kind == label; }
  bool isAddrExp() const { return kind == addrExp; }

  const G4_Declare *getTopDcl() const { return top_dcl; }
  G4_Declare *getTopDcl() { return top_dcl; }
  void setTopDcl(G4_Declare *dcl) { top_dcl = dcl; }

  const G4_VarBase *getBase() const { return base; }
  G4_VarBase *getBase() { return base; }
  void setBase(G4_VarBase *b) { base = b; }
  bool isIndirect() const;
  bool isVxHIndirect() const;

  const G4_Declare *getBaseRegVarRootDeclare() const;
  G4_Declare *getBaseRegVarRootDeclare();

  virtual bool isRelocImm() const { return false; }
  virtual void emit(std::ostream &output) = 0;
  void dump() const;
  std::string print() const;

  bool isGreg() const;
  bool isAreg() const;
  bool isNullReg() const;
  bool isIpReg() const;
  bool isNReg() const;
  bool isAccReg() const;
  bool isFlag() const;
  bool isMaskReg() const;
  bool isMsReg() const;
  bool isSrReg() const;
  bool isCrReg() const;
  bool isDbgReg() const;
  bool isTmReg() const;
  bool isTDRReg() const;
  bool isS0() const;

  const G4_AddrExp *asAddrExp() const {
#ifdef _DEBUG
    if (!isAddrExp()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_AddrExp *>(this);
  }
  G4_AddrExp *asAddrExp() {
    return const_cast<G4_AddrExp *>(((const G4_Operand *)this)->asAddrExp());
  }

  const G4_DstRegRegion *asDstRegRegion() const {
#ifdef _DEBUG
    if (!isDstRegRegion()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_DstRegRegion *>(this);
  }
  G4_DstRegRegion *asDstRegRegion() {
    return const_cast<G4_DstRegRegion *>(
        ((const G4_Operand *)this)->asDstRegRegion());
  }

  const G4_SrcRegRegion *asSrcRegRegion() const {
#ifdef _DEBUG
    if (!isSrcRegRegion()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_SrcRegRegion *>(this);
  }
  G4_SrcRegRegion *asSrcRegRegion() {
    return const_cast<G4_SrcRegRegion *>(
        ((const G4_Operand *)this)->asSrcRegRegion());
  }

  const G4_Imm *asImm() const {
#ifdef _DEBUG
    if (!isImm()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_Imm *>(this);
  }
  G4_Imm *asImm() {
    return const_cast<G4_Imm *>(((const G4_Operand *)this)->asImm());
  }

  const G4_Reloc_Imm *asRelocImm() const {
#ifdef _DEBUG
    if (!isRelocImm()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_Reloc_Imm *>(this);
  }
  G4_Reloc_Imm *asRelocImm() {
    return const_cast<G4_Reloc_Imm *>(((const G4_Operand *)this)->asRelocImm());
  }

  const G4_Predicate *asPredicate() const {
#ifdef _DEBUG
    if (!isPredicate()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_Predicate *>(this);
  }
  G4_Predicate *asPredicate() {
    return const_cast<G4_Predicate *>(
        ((const G4_Operand *)this)->asPredicate());
  }

  const G4_CondMod *asCondMod() const {
#ifdef _DEBUG
    if (!isCondMod()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_CondMod *>(this);
  }

  G4_CondMod *asCondMod() {
    return const_cast<G4_CondMod *>(((const G4_Operand *)this)->asCondMod());
  }

  const G4_Label *asLabel() const {
#ifdef _DEBUG
    if (!isLabel()) {
      return nullptr;
    }
#endif
    return reinterpret_cast<const G4_Label *>(this);
  }
  G4_Label *asLabel() {
    return const_cast<G4_Label *>(((const G4_Operand *)this)->asLabel());
  }

  bool isSrc() const { return isImm() || isAddrExp() || isSrcRegRegion(); }

  bool isScalarSrc() const;

  bool crossGRF(const IR_Builder &builder);

  unsigned getLeftBound();
  unsigned getRightBound();
  bool isRightBoundSet() const { return rightBoundSet; }
  uint64_t getBitVecL();
  uint64_t getBitVecH(const IR_Builder &builder);
  // For operands that do use it, it is computed during left bound compuation.
  unsigned getByteOffset() const { return byteOffset; }

  // ToDo: get rid of this setter
  void setBitVecL(uint64_t bvl) { bitVec[0] = bvl; }

  void setBitVecFromSize(uint32_t NBytes, const IR_Builder &builder);

  void updateFootPrint(BitSet &footprint, bool isSet,
                       const IR_Builder &builder);

  virtual unsigned computeRightBound(uint8_t exec_size) { return left_bound; }
  void setRightBound(unsigned val) {
    rightBoundSet = true;
    right_bound = val;
  }
  void unsetRightBound() { rightBoundSet = false; }
  void setLeftBound(unsigned val) { left_bound = val; }
  const G4_INST *getInst() const { return inst; }
  G4_INST *getInst() { return inst; }
  void setInst(G4_INST *op) { inst = op; }
  void setAccRegSel(G4_AccRegSel value) { accRegSel = value; }
  G4_AccRegSel getAccRegSel() const { return accRegSel; }
  bool isAccRegValid() const { return accRegSel != ACC_UNDEFINED; }
  bool isPhysicallyAllocatedRegVar(bool includeAccRegSel = true) const;

  unsigned getLinearizedStart();
  unsigned getLinearizedEnd();

  // compare if this operand is the same as the input w.r.t physical register in
  // the end
  virtual G4_CmpRelation compareOperand(G4_Operand *opnd,
                                        const IR_Builder &builder) {
    return Rel_disjoint;
  }

  static G4_Type GetNonVectorImmType(G4_Type type) {
    switch (type) {
    case Type_V:
      return Type_W;
    case Type_UV:
      return Type_UW;
    case Type_VF:
      return Type_F;
    default:
      return type;
    }
  }
};

class G4_Imm : public G4_Operand {
  // Requirement for the immediate value 'imm'
  //   Given a value V of type T, and let <V-as-uint> be its bit pattern as
  //   unsigned integer type whose size == sizeof(T). Let 'imm' be the
  //   immediate for V, the following must hold:
  //     (uint64_t)(<V-as-uint>) == (uint64_t)imm.num
  //     i.e.  int16_t v ---> (uint64_t)(*(uint16_t*)&v) == (uint64_t)imm.num
  //           float f   ---> (uint64_t)(*(uint32_t*)&f) == (uint64_t)imm.num
  union {
    int64_t num;
    uint32_t num32;
    double fp;
    float fp32;
  } imm;

public:
  G4_Imm(int64_t i, G4_Type ty) : G4_Operand(G4_Operand::immediate, ty) {
    imm.num = i;
  }

  G4_Imm(double fp, G4_Type ty) : G4_Operand(G4_Operand::immediate, ty) {
    imm.fp = fp;
  }

  G4_Imm(float fp) : G4_Operand(G4_Operand::immediate, Type_F) {
    imm.num = 0; // make sure to clear all the bits
    imm.fp32 = fp;
  }

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  int64_t getImm() const { return imm.num; } // Get bits of imm AS integer.
  int64_t getInt() const {
    vISA_ASSERT(!IS_TYPE_F32_F64(type), ERROR_UNKNOWN);
    return imm.num;
  }
  float getFloat() const {
    // if fp32 is sNAN, it will return qNAN. Be careful!
    vISA_ASSERT(IS_FTYPE(type), ERROR_UNKNOWN);
    return imm.fp32;
  }
  double getDouble() const {
    vISA_ASSERT(IS_DFTYPE(type), ERROR_UNKNOWN);
    return imm.fp;
  }
  bool isZero() const;
  // True if this is a signed integer and its sign bit(s) are 0.
  bool isSignBitZero() const;
  void emit(std::ostream &output) override;
  void emitAutoFmt(std::ostream &output);

  bool isEqualTo(G4_Imm &imm1) const;
  bool isEqualTo(G4_Imm *imm1) const { return isEqualTo(*imm1); }

  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;
  G4_RegFileKind getRegFile() const { return G4_UndefinedRF; }

  static bool isInTypeRange(int64_t imm, G4_Type ty);

  static int64_t typecastVals(int64_t value, G4_Type type);
};

class G4_Reloc_Imm : public G4_Imm {
  friend class IR_Builder;

  GenRelocType relocKind;
  const char  *symbol;

  // G4_Reloc_Imm is a relocation target field.
  // Use Build_IR::createRelocImm to construct one of these operands.
  G4_Reloc_Imm(GenRelocType rt, const char *sym, int64_t val, G4_Type ty)
    : G4_Imm(val, ty), relocKind(rt), symbol(sym) {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
public:

  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;

  bool isRelocImm() const override { return true; }

  void emit(std::ostream &output) override;

  // magic value used when no default relocation value is given
  static const uint32_t DEFAULT_MAGIC = 0x6e10ca2e;
};

class G4_Label : public G4_Operand {
  friend class IR_Builder;

  const char *label;
  VISA_Label_Kind kind;

  G4_Label(const char *l, VISA_Label_Kind k)
      : G4_Operand(G4_Operand::label), label(l), kind(k) {}

public:
  const char *getLabelName() const { return label; }
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  void emit(std::ostream &output) override;
  bool isStackFunction() const { return kind == LABEL_FUNCTION; }
  bool isSubroutine() const { return kind == LABEL_SUBROUTINE; }
  bool isFCLabel() const { return kind == LABEL_FC; }
  bool isBlock() const {
    return kind == LABEL_BLOCK || kind == LABEL_DIVERGENT_RESOURCE_LOOP;
  }
  bool isDivergentResourceLoop() const {
    return kind == LABEL_DIVERGENT_RESOURCE_LOOP;
  }
  VISA_Label_Kind getLabelKind() const { return kind; }
};

class G4_SrcRegRegion final : public G4_Operand {
  friend class IR_Builder;

  const RegionDesc *desc;
  const short regOff;    // base+regOff is the starting register of the region
  const short subRegOff; // sub reg offset related to the regVar in "base"
  // FIXME: regOff (direct) and immAddrOff (indriect) should be mutually
  // exclusive. We should place them in a union.
  short immAddrOff; // imm addr offset
  G4_SrcModifier mod;
  const G4_RegAccess acc;

  G4_SrcRegRegion(const IR_Builder &builder, G4_SrcModifier m, G4_RegAccess a,
                  G4_VarBase *b, short roff, short sroff, const RegionDesc *rd,
                  G4_Type ty, G4_AccRegSel regSel = ACC_UNDEFINED)
      : G4_Operand(G4_Operand::srcRegRegion, ty, b), desc(rd), regOff(roff),
        subRegOff(sroff), mod(m), acc(a) {
    immAddrOff = 0;
    accRegSel = regSel;

    computeLeftBound(builder);
    right_bound = 0;
  }

  void setSrcBitVec(uint8_t exec_size, const IR_Builder &irb);

public:
  G4_SrcRegRegion(G4_SrcRegRegion &rgn);
  G4_SrcRegRegion& operator=(const G4_SrcRegRegion&) = delete;
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  bool operator==(const G4_SrcRegRegion &other) const {
    if (base != other.base || regOff != other.regOff ||
        subRegOff != other.subRegOff ||
        desc->vertStride != other.desc->vertStride ||
        desc->horzStride != other.desc->horzStride ||
        desc->width != other.desc->width || mod != other.mod ||
        acc != other.acc || type != other.type) {
      return false;
    }

    if (acc == IndirGRF && immAddrOff != other.immAddrOff) {
      return false;
    }

    return true;
  }

  void computeLeftBound(const IR_Builder &builder);
  short getRegOff() const { return regOff; }
  short getSubRegOff() const { return subRegOff; }

  G4_SrcModifier getModifier() const { return mod; }
  bool hasModifier() const { return mod != Mod_src_undef; }
  const RegionDesc *getRegion() const { return desc; }
  G4_RegAccess getRegAccess() const { return acc; }
  short getAddrImm() const { return immAddrOff; }
  unsigned short getElemSize() const { return TypeSize(type); }

  void setImmAddrOff(short off) { immAddrOff = off; }
  void setModifier(G4_SrcModifier m) { mod = m; }

  bool sameSrcRegRegion(G4_SrcRegRegion &rgn);

  void emit(std::ostream &output) override;
  void emitRegVarOff(std::ostream &output);
  void emitRegVarOffNoRegion(std::ostream &output);

  bool isAreg() const { return base->isAreg(); }
  bool isNullReg() const { return base->isNullReg(); }
  bool isIpReg() const { return base->isIpReg(); }
  bool isFlag() const { return base->isFlag(); }
  bool isNReg() const { return base->isNReg(); }
  bool isAccReg() const { return base->isAccReg(); }
  bool isMaskReg() const { return base->isMaskReg(); }
  bool isMsReg() const { return base->isMsReg(); }
  bool isSrReg() const { return base->isSrReg(); }
  bool isCrReg() const { return base->isCrReg(); }
  bool isDbgReg() const { return base->isDbgReg(); }
  bool isTmReg() const { return base->isTmReg(); }
  bool isTDRReg() const { return base->isTDRReg(); }
  bool isGreg() const { return base->isGreg(); }
  // Returns true if this operand is a direct address operand (e.g.,
  // a0.n<0;1,0>:uw)
  // The difference between the two versions happens when base is an address
  // variable; isDirectA0() returns false before RA when the declare is not
  // assigned a physical AddrReg, while isDirectAddress() always returns true.
  // Having two versions is unfortunate but we keep the behavior to avoid
  // potential regressions in legacy code.
  bool isDirectA0() const { return acc == Direct && base->isA0(); }
  bool isDirectAddress() const { return acc == Direct && base->isAddress(); }
  bool isScalar() const;
  bool isS0() const { return base->isS0(); }
  bool isFlatRegRegion(
      uint8_t exChannelWidth,
      std::function<bool(uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
                         uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
                         uint8_t exChannelWidth)> checkFlatRegRegionFunc);

  unsigned short ExRegNum(bool &) const;
  unsigned short ExSubRegNum(bool &);
  unsigned short ExIndSubRegNum(bool &);
  short ExIndImmVal(void);

  bool isIndirect() const { return acc != Direct; }

  bool isVxHIndirect() const {
    return isIndirect() && getRegion()->isRegionWH() && getRegion()->width == 1;
  }

  unsigned computeRightBound(uint8_t exec_size) override;
  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;

  void setType(const IR_Builder &builder, G4_Type ty) {
    // FIXME: we should forbid setType() where ty has a different size than old
    // type
    bool recomputeLeftBound = false;

    if (TypeSize(type) != TypeSize(ty)) {
      unsetRightBound();
      recomputeLeftBound = true;
    }

    type = ty;

    if (recomputeLeftBound) {
      computeLeftBound(builder);
    }
  }

  void setRegion(const IR_Builder &builder, const RegionDesc *rd,
                 bool isInvariant = false) {
    if (!isInvariant && !desc->isEqual(rd)) {
      unsetRightBound();
      desc = rd;
      computeLeftBound(builder);
    } else {
      desc = rd;
    }
  }

  bool isNativeType() const;
  bool isNativePackedRowRegion() const;
  bool isNativePackedRegion() const;
  bool evenlySplitCrossGRF(const IR_Builder &builder, uint8_t execSize,
                           bool &sameSubRegOff, bool &vertCrossGRF,
                           bool &contRegion, uint8_t &eleInFirstGRF);
  bool evenlySplitCrossGRF(const IR_Builder &builder, uint8_t execSize);
  bool coverTwoGRF(const IR_Builder &builder);
  bool checkGRFAlign(const IR_Builder &builder);
  bool hasFixedSubregOffset(const IR_Builder &builder, uint32_t &offset);
  bool isNativePackedSrcRegion();
  uint8_t getMaxExecSize(const IR_Builder &builder, int pos, uint8_t maxExSize,
                         bool allowCrossGRF, uint16_t &vs, uint16_t &wd,
                         bool &twoGRFsrc);

  bool isSpilled() const {
    if (getBase() && getBase()->isRegVar()) {
      return getBase()->asRegVar()->isSpilled();
    }

    return false;
  }

  // return the byte offset from the region start for the element at "pos"
  int getByteOffset(int pos) const {
    int rowIdx = pos / desc->width;
    int colIdx = pos % desc->width;
    return rowIdx * desc->vertStride * getElemSize() +
           colIdx * desc->horzStride * getElemSize();
  }

  void rewriteContiguousRegion(IR_Builder &builder, uint16_t opNum);
};

class G4_DstRegRegion final : public G4_Operand {
  friend class IR_Builder;

  G4_RegAccess acc; // direct, indirect GenReg or indirect MsgReg
  short regOff;     // base+regOff is the starting register of the region
  short subRegOff;  // sub reg offset related to the regVar in "base"
  // FIXME: regOff (direct) and immAddrOff (indriect) should be mutually
  // exclusive. We should place them in a union.
  short immAddrOff;          // imm addr offset for indirect dst
  unsigned short horzStride; // <DstRegion> has only horzStride

  G4_DstRegRegion(const IR_Builder &builder, G4_RegAccess a, G4_VarBase *b,
                  short roff, short sroff, unsigned short hstride, G4_Type ty,
                  G4_AccRegSel regSel = ACC_UNDEFINED)
      : G4_Operand(G4_Operand::dstRegRegion, ty, b), acc(a),
        horzStride(hstride) {
    immAddrOff = 0;
    accRegSel = regSel;

    regOff = (roff == ((short)UNDEFINED_SHORT)) ? 0 : roff;
    subRegOff = sroff;

    computeLeftBound(builder);
    right_bound = 0;
  }

  // DstRegRegion should only be constructed through IR_Builder
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

public:
  G4_DstRegRegion(G4_DstRegRegion &rgn);
  G4_DstRegRegion(const IR_Builder &builder, G4_DstRegRegion &rgn,
                  G4_VarBase *new_base);
  G4_DstRegRegion& operator=(const G4_DstRegRegion&) = delete;

  void computeLeftBound(const IR_Builder &builder);

  G4_RegAccess getRegAccess() const { return acc; }
  short getRegOff() const { return regOff; }
  short getSubRegOff() const { return subRegOff; }

  bool isCrossGRFDst(const IR_Builder &builder);
  unsigned short getHorzStride() const { return horzStride; }
  short getAddrImm() const { return immAddrOff; }
  unsigned short getElemSize() const { return getTypeSize(); }
  unsigned short getExecTypeSize() const { return horzStride * getElemSize(); }

  void setImmAddrOff(short off) { immAddrOff = off; }
  void emit(std::ostream &output) override;
  void emitRegVarOff(std::ostream &output);

  bool isAreg() const { return base->isAreg(); }
  bool isNullReg() const { return base->isNullReg(); }
  bool isIpReg() const { return base->isIpReg(); }
  bool isFlag() const { return base->isFlag(); }
  bool isNReg() const { return base->isNReg(); }
  bool isAccReg() const { return base->isAccReg(); }
  bool isMaskReg() const { return base->isMaskReg(); }
  bool isMsReg() const { return base->isMsReg(); }
  bool isSrReg() const { return base->isSrReg(); }
  bool isCrReg() const { return base->isCrReg(); }
  bool isDbgReg() const { return base->isDbgReg(); }
  bool isTmReg() const { return base->isTmReg(); }
  bool isTDRReg() const { return base->isTDRReg(); }
  bool isGreg() const { return base->isGreg(); }
  // Returns true if this operand is a direct address operand (e.g.,
  // a0.n<1>:uw)
  // The difference between the two versions happens when base is an address
  // variable; isDirectA0() returns false before RA when the declare is not
  // assigned a physical AddrReg, while isDirectAddress() always returns true.
  // Having two versions is unfortunate but we keep the behavior to avoid
  // potential regressions in legacy code.
  bool isDirectA0() const { return acc == Direct && base->isA0(); }
  bool isDirectAddress() const { return acc == Direct && base->isAddress(); }
  bool isS0() const { return base->isS0(); }

  unsigned short ExRegNum(bool &);
  unsigned short ExSubRegNum(bool &);
  unsigned short ExIndSubRegNum(bool &);
  short ExIndImmVal(void);

  bool isIndirect() const { return acc != Direct; }

  void setType(const IR_Builder &builder, G4_Type ty);

  void setHorzStride(unsigned short hs) {
    if (horzStride != hs) {
      unsetRightBound();
    }

    horzStride = hs;
  }
  void setDstBitVec(uint8_t exec_size);
  unsigned computeRightBound(uint8_t exec_size) override;
  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;
  bool isNativeType() const;
  bool isNativePackedRowRegion() const;
  bool isNativePackedRegion() const;
  bool coverGRF(const IR_Builder &builder, uint16_t numGRF, uint8_t execSize);
  bool goodOneGRFDst(const IR_Builder &builder, uint8_t execSize);
  bool goodtwoGRFDst(const IR_Builder &builder, uint8_t execSize);
  bool evenlySplitCrossGRF(const IR_Builder &builder, uint8_t execSize);
  bool checkGRFAlign(const IR_Builder &builder) const;
  bool hasFixedSubregOffset(const IR_Builder &builder, uint32_t &offset);
  uint8_t getMaxExecSize(const IR_Builder &builder, int pos, uint8_t maxExSize,
                         bool twoGRFsrc);
  bool isSpilled() const {
    if (getBase() && getBase()->isRegVar()) {
      return getBase()->asRegVar()->isSpilled();
    }

    return false;
  }
};

typedef enum : unsigned char {
  PRED_DEFAULT,
  PRED_ANY2H,
  PRED_ANY4H,
  PRED_ANY8H,
  PRED_ANY16H,
  PRED_ANY32H,
  PRED_ALL2H,
  PRED_ALL4H,
  PRED_ALL8H,
  PRED_ALL16H,
  PRED_ALL32H,
  PRED_ANYV,
  PRED_ALLV,
  PRED_ANY_WHOLE, // any of the flag-bits
  PRED_ALL_WHOLE  // all of the flag-bits
} G4_Predicate_Control;

//
// predicate control for inst
//
class G4_Predicate final : public G4_Operand {
  friend class IR_Builder;

  G4_PredState state; // + or -
  G4_Predicate_Control control;
  unsigned short subRegOff;

  G4_Predicate(G4_PredState s, G4_VarBase *flag, unsigned short srOff,
               G4_Predicate_Control ctrl)
      : G4_Operand(G4_Operand::predicate, flag), state(s), control(ctrl),
        subRegOff(srOff) {
    top_dcl = getBase()->asRegVar()->getDeclare();
    vISA_ASSERT(flag->isFlag(), ERROR_INTERNAL_ARGUMENT);
    if (getBase()->asRegVar()->getPhyReg()) {
      left_bound = srOff * 16;

      byteOffset = srOff * 2;

      auto flagNum = getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
      left_bound += flagNum * 32;
      byteOffset += flagNum * 4;
    } else {
      left_bound = 0;
      byteOffset = 0;
    }
  }

public:
  G4_Predicate(G4_Predicate &prd);
  G4_Predicate& operator=(const G4_Predicate&) = delete;

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  unsigned short getSubRegOff() const { return subRegOff; }
  unsigned short getRegOff() const {
    vISA_ASSERT(getBase()->isAreg(), ERROR_INTERNAL_ARGUMENT);
    return getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
  }

  G4_PredState getState() const { return state; }
  void setState(G4_PredState s) { state = s; }
  G4_Predicate_Control getControl() const { return control; }
  void setControl(G4_Predicate_Control PredCtrl) { control = PredCtrl; }
  bool samePredicate(const G4_Predicate &prd) const;
  void emit(std::ostream &output) override;
  void emit_body(std::ostream &output);

  unsigned computeRightBound(uint8_t exec_size) override;
  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;
  void splitPred();
  unsigned getPredCtrlGroupSize() const {
    switch (control) {
    case PRED_ANY2H:
    case PRED_ALL2H:
      return 2;
    case PRED_ANY4H:
    case PRED_ALL4H:
      return 4;
    case PRED_ANY8H:
    case PRED_ALL8H:
      return 8;
    case PRED_ANY16H:
    case PRED_ALL16H:
      return 16;
    case PRED_ANY32H:
    case PRED_ALL32H:
      return 32;
    default:
      return 1;
    }
  }
  static bool isAnyH(G4_Predicate_Control Ctrl) {
    switch (Ctrl) {
    default:
      break;
    case PRED_ANY2H:
    case PRED_ANY4H:
    case PRED_ANY8H:
    case PRED_ANY16H:
    case PRED_ANY32H:
      return true;
    }
    return false;
  }
  static bool isAllH(G4_Predicate_Control Ctrl) {
    switch (Ctrl) {
    default:
      break;
    case PRED_ALL2H:
    case PRED_ALL4H:
    case PRED_ALL8H:
    case PRED_ALL16H:
    case PRED_ALL32H:
      return true;
    }
    return false;
  }
};

//
// condition modifier for inst
//
class G4_CondMod final : public G4_Operand {
  friend class IR_Builder;
  G4_CondModifier mod;
  unsigned short subRegOff;

  G4_CondMod(G4_CondModifier m, G4_VarBase *flag, unsigned short off)
      : G4_Operand(G4_Operand::condMod, flag), mod(m), subRegOff(off) {
    if (flag != nullptr) {
      top_dcl = getBase()->asRegVar()->getDeclare();
      vISA_ASSERT(flag->isFlag(), ERROR_INTERNAL_ARGUMENT);
      if (getBase()->asRegVar()->getPhyReg()) {
        left_bound = off * 16;
        byteOffset = off * 2;

        auto flagNum =
            getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
        left_bound += flagNum * 32;
        byteOffset += flagNum * 4;
      } else {
        left_bound = 0;
        byteOffset = 0;
      }
    }
  }

public:
  G4_CondMod(G4_CondMod &cMod);
  G4_CondMod(const G4_CondMod&) = delete;
  G4_CondMod& operator=(G4_CondMod cMod) = delete;
  ~G4_CondMod() = default;
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  G4_CondModifier getMod() const { return mod; }
  unsigned short getRegOff() const {
    vISA_ASSERT(getBase()->isAreg(), ERROR_INTERNAL_ARGUMENT);
    vISA_ASSERT(getBase()->asRegVar()->getPhyReg(),
                 "getRegOff is called for non-PhyReg");
    return getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
  }
  unsigned short getSubRegOff() const { return subRegOff; }
  bool sameCondMod(const G4_CondMod &prd) const;
  void emit(std::ostream &output) override;

  // Get condition modifier when operands are reversed.
  static G4_CondModifier getReverseCondMod(G4_CondModifier mod) {
    switch (mod) {
    default:
      break;
    case Mod_g:
      return Mod_le;
    case Mod_ge:
      return Mod_l;
    case Mod_l:
      return Mod_ge;
    case Mod_le:
      return Mod_g;
    }

    return mod;
  }

  unsigned computeRightBound(uint8_t exec_size) override;
  G4_CmpRelation compareOperand(G4_Operand *opnd,
                                const IR_Builder &builder) override;
  void splitCondMod();
};

class G4_AddrExp final : public G4_Operand {
  G4_RegVar *m_addressedReg;
  int m_offset; // current implementation: byte offset

public:
  G4_AddrExp(G4_RegVar *reg, int offset, G4_Type ty)
      : G4_Operand(G4_Operand::addrExp, ty), m_addressedReg(reg),
        m_offset(offset) {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  const G4_RegVar *getRegVar() const { return m_addressedReg; }
  G4_RegVar *getRegVar() { return m_addressedReg; }
  void setRegVar(G4_RegVar *var) { m_addressedReg = var; }
  int getOffset() const { return m_offset; }
  void setOffset(int tOffset) { m_offset = tOffset; }

  int eval(const IR_Builder &builder);
  bool isRegAllocPartaker() const {
    return m_addressedReg->isRegAllocPartaker();
  }

  void emit(std::ostream &output);
};

// Inlined functions for G4_Operand class.
// TODO: Move them inside the class.
inline bool G4_Operand::isGreg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isGreg();
}
inline bool G4_Operand::isAreg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isAreg();
}
inline bool G4_Operand::isNullReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isNullReg();
}
inline bool G4_Operand::isIpReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isIpReg();
}
inline bool G4_Operand::isNReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isNReg();
}
inline bool G4_Operand::isAccReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isAccReg();
}
inline bool G4_Operand::isFlag() const {
  if (isRegRegion() && const_cast<G4_VarBase *>(getBase())->isFlag())
    return true;
  return isPredicate() || isCondMod();
}
inline bool G4_Operand::isMaskReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isMaskReg();
}
inline bool G4_Operand::isMsReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isMsReg();
}
inline bool G4_Operand::isSrReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isSrReg();
}
inline bool G4_Operand::isCrReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isCrReg();
}
inline bool G4_Operand::isDbgReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isDbgReg();
}
inline bool G4_Operand::isTmReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isTmReg();
}
inline bool G4_Operand::isTDRReg() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isTDRReg();
}
inline bool G4_Operand::isS0() const {
  return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isS0();
}

inline bool G4_Operand::isScalarSrc() const {
  return isImm() || isAddrExp() ||
         (isSrcRegRegion() && asSrcRegRegion()->isScalar());
}

inline const G4_Declare *G4_Operand::getBaseRegVarRootDeclare() const {
  return getBase()->asRegVar()->getDeclare()->getRootDeclare();
}
inline G4_Declare *G4_Operand::getBaseRegVarRootDeclare() {
  return getBase()->asRegVar()->getDeclare()->getRootDeclare();
}

} // namespace vISA

#endif // G4_OPERAND_H
