/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DIE.h
///////////////////////////////////////////////////////////////////////////////

#pragma once
// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/Compiler.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "Probe/Assertion.h"

#include <vector>

namespace llvm {

// namespace dwarf
// Intel extensions
#ifndef DW_AT_INTEL_simd_width
#define DW_AT_INTEL_simd_width 0x2400
#endif
#ifndef DW_OP_INTEL_regs
#define DW_OP_INTEL_regs 0xeb
#endif
#ifndef DW_OP_INTEL_push_bit_piece_stack
#define DW_OP_INTEL_push_bit_piece_stack 0xec
#endif
#ifndef DW_OP_INTEL_push_simd_lane
#define DW_OP_INTEL_push_simd_lane 0xed
#endif
#ifndef DW_OP_INTEL_piece_stack
#define DW_OP_INTEL_piece_stack 0xee
#endif
#ifndef DW_OP_INTEL_bit_piece_stack
#define DW_OP_INTEL_bit_piece_stack 0xef
#endif

class MCSymbol;
class raw_ostream;
class MCExpr;
} // namespace llvm

namespace IGC {
class RegisterNumbering {
public:
  constexpr static unsigned int IP = 0;
  constexpr static unsigned int EMask = 1;
  constexpr static unsigned int BTBase = 5;
  constexpr static unsigned int ScratchBase = 6;
  constexpr static unsigned int GenStateBase = 7;
  constexpr static unsigned int SurfStateBase = 8;
  constexpr static unsigned int BindlessSurfStateBase = 9;
  constexpr static unsigned int BindlessSamplerStateBase = 10;
  constexpr static unsigned int GRFBase = 16;
  constexpr static unsigned int A0Base = 272;
  constexpr static unsigned int F0Base = 288;
  constexpr static unsigned int Acc0Base = 304;
  constexpr static unsigned int Mme0Base = 335;
};

// Use following templated method to get encoded register number
// for regx and bregx operations. For eg, if GRF to encode in regx is
// 10 then invoke method as GetEncodedRegNum<RegisterNumbering::GRFBase>(10).
template <unsigned int EncodeBase>
unsigned int GetEncodedRegNum(unsigned int i) {
  return (EncodeBase + i);
}

class StreamEmitter;

//===--------------------------------------------------------------------===//
/// DIEAbbrevData - Dwarf abbreviation data, describes one attribute of a
/// Dwarf abbreviation.
class DIEAbbrevData {
  /// Attribute - Dwarf attribute code.
  ///
  llvm::dwarf::Attribute Attribute;

  /// Form - Dwarf form code.
  ///
  llvm::dwarf::Form Form;

public:
  DIEAbbrevData(llvm::dwarf::Attribute A, llvm::dwarf::Form F)
      : Attribute(A), Form(F) {}

  // Accessors.
  llvm::dwarf::Attribute getAttribute() const { return Attribute; }
  llvm::dwarf::Form getForm() const { return Form; }

  /// Profile - Used to gather unique data for the abbreviation folding set.
  ///
  void Profile(llvm::FoldingSetNodeID &ID) const;
};

//===--------------------------------------------------------------------===//
/// DIEAbbrev - Dwarf abbreviation, describes the organization of a debug
/// information object.
class DIEAbbrev : public llvm::FoldingSetNode {
  /// Tag - Dwarf tag code.
  ///
  llvm::dwarf::Tag Tag;

  /// ChildrenFlag - Dwarf children flag.
  ///
  uint16_t ChildrenFlag;

  /// Unique number for node.
  ///
  unsigned Number;

  /// Data - Raw data bytes for abbreviation.
  ///
  llvm::SmallVector<DIEAbbrevData, 12> Data;

public:
  DIEAbbrev(llvm::dwarf::Tag T, uint16_t C) : Tag(T), ChildrenFlag(C), Data() {}

  // Accessors.
  llvm::dwarf::Tag getTag() const { return Tag; }
  unsigned getNumber() const { return Number; }
  uint16_t getChildrenFlag() const { return ChildrenFlag; }
  const llvm::SmallVectorImpl<DIEAbbrevData> &getData() const { return Data; }
  void setChildrenFlag(uint16_t CF) { ChildrenFlag = CF; }
  void setNumber(unsigned N) { Number = N; }

  /// AddAttribute - Adds another set of attribute information to the
  /// abbreviation.
  void AddAttribute(llvm::dwarf::Attribute Attribute, llvm::dwarf::Form Form) {
    Data.push_back(DIEAbbrevData(Attribute, Form));
  }

  /// Profile - Used to gather unique data for the abbreviation folding set.
  ///
  void Profile(llvm::FoldingSetNodeID &ID) const;

  /// Emit - Print the abbreviation using the specified asm printer.
  ///
  void Emit(StreamEmitter *AP) const;

#ifndef NDEBUG
  void print(llvm::raw_ostream &O);
  void dump();
#endif
};

//===--------------------------------------------------------------------===//
/// DIE - A structured debug information entry.  Has an abbreviation which
/// describes its organization.
class DIEValue;

class DIE {
protected:
  /// Offset - Offset in debug info section.
  ///
  unsigned Offset;

  /// Size - Size of instance + children.
  ///
  unsigned Size;

  /// Abbrev - Buffer for constructing abbreviation.
  ///
  DIEAbbrev Abbrev;

  /// Children DIEs.
  ///
  std::vector<DIE *> Children;

  DIE *Parent;

  /// Attribute values.
  ///
  llvm::SmallVector<DIEValue *, 12> Values;

public:
  explicit DIE(unsigned Tag)
      : Offset(0), Size(0),
        Abbrev((llvm::dwarf::Tag)Tag, llvm::dwarf::DW_CHILDREN_no), Parent(0) {}
  virtual ~DIE();

  // Accessors.
  DIEAbbrev &getAbbrev() { return Abbrev; }
  const DIEAbbrev &getAbbrev() const { return Abbrev; }
  unsigned getAbbrevNumber() const { return Abbrev.getNumber(); }
  llvm::dwarf::Tag getTag() const { return Abbrev.getTag(); }
  unsigned getOffset() const { return Offset; }
  unsigned getSize() const { return Size; }
  const std::vector<DIE *> &getChildren() const { return Children; }
  const llvm::SmallVectorImpl<DIEValue *> &getValues() const { return Values; }
  DIE *getParent() const { return Parent; }
  /// Climb up the parent chain to get the compile unit DIE this DIE belongs
  /// to.
  const DIE *getCompileUnit() const;
  /// Similar to getCompileUnit, returns null when DIE is not added to an
  /// owner yet.
  const DIE *getCompileUnitOrNull() const;
  void setOffset(unsigned O) { Offset = O; }
  void setSize(unsigned S) { Size = S; }

  /// addValue - Add a value and attributes to a DIE.
  ///
  void addValue(llvm::dwarf::Attribute Attribute, llvm::dwarf::Form Form,
                DIEValue *Value) {
    Abbrev.AddAttribute(Attribute, Form);
    Values.push_back(Value);
  }

  /// addChild - Add a child to the DIE.
  ///
  void addChild(DIE *Child) {
    IGC_ASSERT(!Child->getParent());
    Abbrev.setChildrenFlag(llvm::dwarf::DW_CHILDREN_yes);
    Children.push_back(Child);
    Child->Parent = this;
  }

  /// findAttribute - Find a value in the DIE with the attribute given, returns
  /// NULL if no such attribute exists.
  DIEValue *findAttribute(uint16_t Attribute);

#ifndef NDEBUG
  void print(llvm::raw_ostream &O, unsigned IndentCount = 0) const;
  void dump();
#endif
};

//===--------------------------------------------------------------------===//
/// DIEValue - A debug information entry value.
///
class DIEValue {
  virtual void anchor() {}

public:
  enum {
    isInteger,
    isString,
    isExpr,
    isLabel,
    isDelta,
    isEntry,
    isBlock,
    isInlinedString
  };

protected:
  /// Type - Type of data stored in the value.
  ///
  unsigned Type;

public:
  explicit DIEValue(unsigned T) : Type(T) {}
  virtual ~DIEValue() {}

  // Accessors
  unsigned getType() const { return Type; }

  /// EmitValue - Emit value via the Dwarf writer.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const = 0;

  /// SizeOf - Return the size of a value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const = 0;

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const = 0;
  void dump() const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEInteger - An integer value DIE.
///
class DIEInteger : public DIEValue {
  uint64_t Integer;

public:
  explicit DIEInteger(uint64_t I) : DIEValue(isInteger), Integer(I) {}

  /// BestForm - Choose the best form for integer.
  ///
  static llvm::dwarf::Form BestForm(bool IsSigned, uint64_t Int) {
    if (IsSigned) {
      const int64_t SignedInt = Int;
      if ((char)Int == SignedInt)
        return llvm::dwarf::DW_FORM_data1;
      if ((short)Int == SignedInt)
        return llvm::dwarf::DW_FORM_data2;
      if ((int)Int == SignedInt)
        return llvm::dwarf::DW_FORM_data4;
    } else {
      if ((unsigned char)Int == Int)
        return llvm::dwarf::DW_FORM_data1;
      if ((unsigned short)Int == Int)
        return llvm::dwarf::DW_FORM_data2;
      if ((unsigned int)Int == Int)
        return llvm::dwarf::DW_FORM_data4;
    }
    return llvm::dwarf::DW_FORM_data8;
  }

  /// EmitValue - Emit integer of appropriate size.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  uint64_t getValue() const { return Integer; }

  void setValue(uint64_t v) { Integer = v; }

  /// SizeOf - Determine size of integer value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *I) { return I->getType() == isInteger; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEExpr - An expression DIE.
//
class DIEExpr : public DIEValue {
  const llvm::MCExpr *Expr;

public:
  explicit DIEExpr(const llvm::MCExpr *E) : DIEValue(isExpr), Expr(E) {}

  /// EmitValue - Emit expression value.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// getValue - Get llvm::MCExpr.
  ///
  const llvm::MCExpr *getValue() const { return Expr; }

  /// SizeOf - Determine size of expression value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *E) { return E->getType() == isExpr; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIELabel - A label DIE.
//
class DIELabel : public DIEValue {
  const llvm::MCSymbol *Label;

public:
  explicit DIELabel(const llvm::MCSymbol *L) : DIEValue(isLabel), Label(L) {}

  /// EmitValue - Emit label value.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// getValue - Get llvm::MCSymbol.
  ///
  const llvm::MCSymbol *getValue() const { return Label; }

  /// SizeOf - Determine size of label value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *L) { return L->getType() == isLabel; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEDelta - A simple label difference DIE.
///
class DIEDelta : public DIEValue {
  const llvm::MCSymbol *LabelHi;
  const llvm::MCSymbol *LabelLo;

public:
  DIEDelta(const llvm::MCSymbol *Hi, const llvm::MCSymbol *Lo)
      : DIEValue(isDelta), LabelHi(Hi), LabelLo(Lo) {}

  /// EmitValue - Emit delta value.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// SizeOf - Determine size of delta value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *D) { return D->getType() == isDelta; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEString - A container for string values.
///
class DIEString : public DIEValue {
  const DIEValue *Access;
  const llvm::StringRef Str;

public:
  DIEString(const DIEValue *Acc, const llvm::StringRef S)
      : DIEValue(isString), Access(Acc), Str(S) {}

  /// getString - Grab the string out of the object.
  llvm::StringRef getString() const { return Str; }

  /// EmitValue - Emit delta value.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// SizeOf - Determine size of delta value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *D) { return D->getType() == isString; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEInlinedString - A container for inlined string values.
///
class DIEInlinedString : public DIEValue {
  std::string Str;

public:
  DIEInlinedString(const llvm::StringRef S) : DIEValue(isInlinedString) {
    Str = S.str();
  }

  /// getString - Grab the string out of the object.
  llvm::StringRef getString() const { return Str; }

  /// EmitValue - Emit delta value.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// SizeOf - Determine size of delta value in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *D) {
    return D->getType() == isInlinedString;
  }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEEntry - A pointer to another debug information entry.  An instance of
/// this class can also be used as a proxy for a debug information entry not
/// yet defined (ie. types.)
class DIEEntry : public DIEValue {
  DIE *const Entry;
  unsigned DwarfVersion;

public:
  explicit DIEEntry(DIE *E, unsigned Version)
      : DIEValue(isEntry), Entry(E), DwarfVersion(Version) {
    IGC_ASSERT_MESSAGE(nullptr != E,
                       "Cannot construct a DIEEntry with a null DIE");
  }

  DIE *getEntry() const { return Entry; }

  /// EmitValue - Emit debug information entry offset.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// SizeOf - Determine size of debug information entry in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const {
    return Form == llvm::dwarf::DW_FORM_ref_addr
               ? getRefAddrSize(AP, DwarfVersion)
               : sizeof(int32_t);
  }

  /// Returns size of a ref_addr entry.
  static unsigned getRefAddrSize(StreamEmitter *AP, unsigned DwarfVersion);

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *E) { return E->getType() == isEntry; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
#endif
};

//===--------------------------------------------------------------------===//
/// DIEBlock - A block of values.  Primarily used for location expressions.
//
class DIEBlock : public DIEValue, public DIE {
  unsigned Size; // Size in bytes excluding size header.
public:
  DIEBlock() : DIEValue(isBlock), DIE(0), Size(0) {}

  /// ComputeSize - calculate the size of the block.
  ///
  unsigned ComputeSize(StreamEmitter *AP);

  /// ComputeSizeOnTheFly - calculate size of block on the fly.
  ///
  unsigned ComputeSizeOnTheFly(StreamEmitter *AP) const;

  /// EmitToRawBuffer - emit data to raw buffer for encoding in debug_loc
  ///
  void EmitToRawBuffer(std::vector<unsigned char> &buffer);

  /// BestForm - Choose the best form for data.
  ///
  llvm::dwarf::Form BestForm() const {
    if ((unsigned char)Size == Size)
      return llvm::dwarf::DW_FORM_block1;
    if ((unsigned short)Size == Size)
      return llvm::dwarf::DW_FORM_block2;
    if ((unsigned int)Size == Size)
      return llvm::dwarf::DW_FORM_block4;
    return llvm::dwarf::DW_FORM_block;
  }

  /// EmitValue - Emit block data.
  ///
  virtual void EmitValue(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  /// SizeOf - Determine size of block data in bytes.
  ///
  virtual unsigned SizeOf(StreamEmitter *AP, llvm::dwarf::Form Form) const;

  // Implement isa/cast/dyncast.
  static bool classof(const DIEValue *E) { return E->getType() == isBlock; }

#ifndef NDEBUG
  virtual void print(llvm::raw_ostream &O) const;
  virtual void dump() const;
#endif
};

} // namespace IGC
