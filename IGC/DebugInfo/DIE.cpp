/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DIE.cpp
///////////////////////////////////////////////////////////////////////////////

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/LEB128.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "DIE.hpp"
#include "DwarfDebug.hpp"
#include "StreamEmitter.hpp"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace ::IGC;

//===----------------------------------------------------------------------===//
// DIEAbbrevData Implementation
//===----------------------------------------------------------------------===//

/// Profile - Used to gather unique data for the abbreviation folding set.
///
void DIEAbbrevData::Profile(FoldingSetNodeID &ID) const {
  // Explicitly cast to an integer type for which FoldingSetNodeID has
  // overloads.  Otherwise MSVC 2010 thinks this call is ambiguous.
  ID.AddInteger(unsigned(Attribute));
  ID.AddInteger(unsigned(Form));
}

//===----------------------------------------------------------------------===//
// DIEAbbrev Implementation
//===----------------------------------------------------------------------===//

/// Profile - Used to gather unique data for the abbreviation folding set.
///
void DIEAbbrev::Profile(FoldingSetNodeID &ID) const {
  ID.AddInteger(unsigned(Tag));
  ID.AddInteger(ChildrenFlag);

  // For each attribute description.
  for (unsigned i = 0, N = Data.size(); i < N; ++i) {
    Data[i].Profile(ID);
  }
}

/// Emit - Print the abbreviation using the specified asm printer.
///
void DIEAbbrev::Emit(StreamEmitter *AP) const {
  // Emit its Dwarf tag type.
  AP->EmitULEB128(Tag, dwarf::TagString(Tag));

  // Emit whether it has children DIEs.
  AP->EmitULEB128(ChildrenFlag, dwarf::ChildrenString(ChildrenFlag));

  // For each attribute description.
  for (unsigned i = 0, N = Data.size(); i < N; ++i) {
    const DIEAbbrevData &AttrData = Data[i];

    // Emit attribute type.
    AP->EmitULEB128(AttrData.getAttribute(),
                    dwarf::AttributeString(AttrData.getAttribute()));

    // Emit form type.
    AP->EmitULEB128(AttrData.getForm(),
                    dwarf::FormEncodingString(AttrData.getForm()));
  }

  // Mark end of abbreviation.
  AP->EmitULEB128(0, "EOM(1)");
  AP->EmitULEB128(0, "EOM(2)");
}

#ifndef NDEBUG
void DIEAbbrev::print(raw_ostream &O) {
  O << "Abbreviation @" << format("0x%lx", (long)(intptr_t)this) << "  "
    << dwarf::TagString(Tag) << " " << dwarf::ChildrenString(ChildrenFlag)
    << '\n';

  for (unsigned i = 0, N = Data.size(); i < N; ++i) {
    O << "  " << dwarf::AttributeString(Data[i].getAttribute()) << "  "
      << dwarf::FormEncodingString(Data[i].getForm()) << '\n';
  }
}

void DIEAbbrev::dump() { print(dbgs()); }
#endif

//===----------------------------------------------------------------------===//
// DIE Implementation
//===----------------------------------------------------------------------===//

DIE::~DIE() {
  for (unsigned i = 0, N = Children.size(); i < N; ++i)
    delete Children[i];
}

/// Climb up the parent chain to get the compile unit DIE to which this DIE
/// belongs.
const DIE *DIE::getCompileUnit() const {
  const DIE *const Cu = getCompileUnitOrNull();
  IGC_ASSERT_MESSAGE(nullptr != Cu, "We should not have orphaned DIEs.");
  return Cu;
}

/// Climb up the parent chain to get the compile unit DIE this DIE belongs
/// to. Return NULL if DIE is not added to an owner yet.
const DIE *DIE::getCompileUnitOrNull() const {
  const DIE *p = this;
  while (p) {
    if (p->getTag() == dwarf::DW_TAG_compile_unit)
      return p;
    p = p->getParent();
  }
  return NULL;
}

DIEValue *DIE::findAttribute(uint16_t Attribute) {
  const SmallVectorImpl<DIEValue *> &Values = getValues();
  const DIEAbbrev &Abbrevs = getAbbrev();

  // Iterate through all the attributes until we find the one we're
  // looking for, if we can't find it return NULL.
  for (size_t i = 0; i < Values.size(); ++i)
    if (Abbrevs.getData()[i].getAttribute() == Attribute)
      return Values[i];
  return NULL;
}

#ifndef NDEBUG
void DIE::print(raw_ostream &O, unsigned IndentCount) const {
  const std::string Indent(IndentCount, ' ');
  bool isBlock = Abbrev.getTag() == 0;

  if (!isBlock) {
    O << Indent << "Die: " << format("0x%lx", (long)(intptr_t)this)
      << ", Offset: " << Offset << ", Size: " << Size << "\n";

    O << Indent << dwarf::TagString(Abbrev.getTag()) << " "
      << dwarf::ChildrenString(Abbrev.getChildrenFlag()) << "\n";
  } else {
    O << "Size: " << Size << "\n";
  }

  const SmallVectorImpl<DIEAbbrevData> &Data = Abbrev.getData();

  IndentCount += 2;
  for (unsigned i = 0, N = Data.size(); i < N; ++i) {
    O << Indent;

    if (!isBlock)
      O << dwarf::AttributeString(Data[i].getAttribute());
    else
      O << "Blk[" << i << "]";

    O << "  " << dwarf::FormEncodingString(Data[i].getForm()) << " ";
    Values[i]->print(O);
    O << "\n";
  }
  IndentCount -= 2;

  for (unsigned j = 0, M = Children.size(); j < M; ++j) {
    Children[j]->print(O, IndentCount + 4);
  }

  if (!isBlock)
    O << "\n";
}

void DIE::dump() { print(dbgs()); }
#endif

#ifndef NDEBUG
void DIEValue::dump() const { print(dbgs()); }
#endif

//===----------------------------------------------------------------------===//
// DIEInteger Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit integer of appropriate size.
///
void DIEInteger::EmitValue(StreamEmitter *Asm, dwarf::Form Form) const {
  unsigned Size = ~0U;
  switch (Form) {
  case dwarf::DW_FORM_flag_present:
    return;
  case dwarf::DW_FORM_flag: // Fall thru
  case dwarf::DW_FORM_ref1: // Fall thru
  case dwarf::DW_FORM_data1:
    Size = 1;
    break;
  case dwarf::DW_FORM_ref2: // Fall thru
  case dwarf::DW_FORM_data2:
    Size = 2;
    break;
  case dwarf::DW_FORM_sec_offset: // Fall thru
  case dwarf::DW_FORM_ref4:       // Fall thru
  case dwarf::DW_FORM_data4:
    Size = 4;
    break;
  case dwarf::DW_FORM_ref8: // Fall thru
  case dwarf::DW_FORM_data8:
    Size = 8;
    break;
  case dwarf::DW_FORM_GNU_str_index:
    Asm->EmitULEB128(Integer);
    return;
  case dwarf::DW_FORM_GNU_addr_index:
    Asm->EmitULEB128(Integer);
    return;
  case dwarf::DW_FORM_udata:
    Asm->EmitULEB128(Integer);
    return;
  case dwarf::DW_FORM_sdata:
    Asm->EmitSLEB128(Integer);
    return;
  case dwarf::DW_FORM_addr:
    Size = Asm->GetPointerSize();
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "DIE Value form not supported yet");
  }
  Asm->EmitIntValue(Integer, Size);
}

/// SizeOf - Determine size of integer value in bytes.
///
unsigned DIEInteger::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  switch (Form) {
  case dwarf::DW_FORM_flag_present:
    return 0;
  case dwarf::DW_FORM_flag: // Fall thru
  case dwarf::DW_FORM_ref1: // Fall thru
  case dwarf::DW_FORM_data1:
    return sizeof(int8_t);
  case dwarf::DW_FORM_ref2: // Fall thru
  case dwarf::DW_FORM_data2:
    return sizeof(int16_t);
  case dwarf::DW_FORM_sec_offset: // Fall thru
  case dwarf::DW_FORM_ref4:       // Fall thru
  case dwarf::DW_FORM_data4:
    return sizeof(int32_t);
  case dwarf::DW_FORM_ref8: // Fall thru
  case dwarf::DW_FORM_data8:
    return sizeof(int64_t);
  case dwarf::DW_FORM_GNU_str_index:
    return getULEB128Size(Integer);
  case dwarf::DW_FORM_GNU_addr_index:
    return getULEB128Size(Integer);
  case dwarf::DW_FORM_udata:
    return getULEB128Size(Integer);
  case dwarf::DW_FORM_sdata:
    return getSLEB128Size(Integer);
  case dwarf::DW_FORM_addr:
    return AP->GetPointerSize();
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "DIE Value form not supported yet");
  }
}

#ifndef NDEBUG
void DIEInteger::print(raw_ostream &O) const {
  O << "Int: " << (int64_t)Integer << "  0x";
  O.write_hex(Integer);
}
#endif

//===----------------------------------------------------------------------===//
// DIEExpr Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit expression value.
///
void DIEExpr::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  AP->EmitValue(Expr, SizeOf(AP, Form));
}

/// SizeOf - Determine size of expression value in bytes.
///
unsigned DIEExpr::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  if (Form == dwarf::DW_FORM_data4)
    return 4;
  if (Form == dwarf::DW_FORM_sec_offset)
    return 4;
  if (Form == dwarf::DW_FORM_strp)
    return 4;
  return AP->GetPointerSize();
}

#ifndef NDEBUG
void DIEExpr::print(raw_ostream &O) const {
  O << "Expr: ";
  // Expr->print(O);
}
#endif

//===----------------------------------------------------------------------===//
// DIELabel Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit label value.
///
void DIELabel::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  AP->EmitLabelReference(Label, SizeOf(AP, Form),
                         Form == dwarf::DW_FORM_strp ||
                             Form == dwarf::DW_FORM_sec_offset ||
                             Form == dwarf::DW_FORM_ref_addr);
}

/// SizeOf - Determine size of label value in bytes.
///
unsigned DIELabel::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  if (Form == dwarf::DW_FORM_data4)
    return 4;
  if (Form == dwarf::DW_FORM_sec_offset)
    return 4;
  if (Form == dwarf::DW_FORM_strp)
    return 4;
  return AP->GetPointerSize();
}

#ifndef NDEBUG
void DIELabel::print(raw_ostream &O) const { O << "Lbl: " << Label->getName(); }
#endif

//===----------------------------------------------------------------------===//
// DIEDelta Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit delta value.
///
void DIEDelta::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  AP->EmitLabelDifference(LabelHi, LabelLo, SizeOf(AP, Form));
}

/// SizeOf - Determine size of delta value in bytes.
///
unsigned DIEDelta::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  if (Form == dwarf::DW_FORM_data4)
    return 4;
  if (Form == dwarf::DW_FORM_strp)
    return 4;
  return AP->GetPointerSize();
}

#ifndef NDEBUG
void DIEDelta::print(raw_ostream &O) const {
  O << "Del: " << LabelHi->getName() << "-" << LabelLo->getName();
}
#endif

//===----------------------------------------------------------------------===//
// DIEString Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit string value.
///
void DIEString::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  Access->EmitValue(AP, Form);
}

/// SizeOf - Determine size of delta value in bytes.
///
unsigned DIEString::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  return Access->SizeOf(AP, Form);
}

#ifndef NDEBUG
void DIEString::print(raw_ostream &O) const {
  O << "String: " << Str << "\tSymbol: ";
  Access->print(O);
}
#endif

//===----------------------------------------------------------------------===//
// DIEInlinedString Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit string value.
///
void DIEInlinedString::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  AP->EmitBytes(Str);
  AP->EmitInt8(0);
}

/// SizeOf - Determine size of delta value in bytes.
///
unsigned DIEInlinedString::SizeOf(StreamEmitter *AP, dwarf::Form Form) const {
  return Str.size() + 1;
}

#ifndef NDEBUG
void DIEInlinedString::print(raw_ostream &O) const {
  O << "Inlined string: " << Str << "\tSymbol: ";
}
#endif

//===----------------------------------------------------------------------===//
// DIEEntry Implementation
//===----------------------------------------------------------------------===//

/// EmitValue - Emit debug information entry offset.
///
void DIEEntry::EmitValue(StreamEmitter *AP, dwarf::Form Form) const {
  AP->EmitInt32(Entry->getOffset());
}

unsigned DIEEntry::getRefAddrSize(StreamEmitter *AP, unsigned DwarfVersion) {
  // DWARF4: References that use the attribute form DW_FORM_ref_addr are
  // specified to be four bytes in the DWARF 32-bit format and eight bytes
  // in the DWARF 64-bit format, while DWARF Version 2 specifies that such
  // references have the same size as an address on the target system.
  if (DwarfVersion == 2)
    return AP->GetPointerSize();
  return sizeof(int32_t);
}

#ifndef NDEBUG
void DIEEntry::print(raw_ostream &O) const {
  O << format("Die: 0x%lx", (long)(intptr_t)Entry);
}
#endif

//===----------------------------------------------------------------------===//
// DIEBlock Implementation
//===----------------------------------------------------------------------===//

/// ComputeSize - calculate the size of the block.
///
unsigned DIEBlock::ComputeSize(StreamEmitter *AP) {
  if (!Size) {
    const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev.getData();
    for (unsigned i = 0, N = Values.size(); i < N; ++i)
      Size += Values[i]->SizeOf(AP, AbbrevData[i].getForm());
  }

  return Size;
}

/// ComputeSizeOnTheFly - calculate size of block on the fly.
/// This function is used to compute size of abbrevs already
/// emitted this far. It doesnt update Size method.
unsigned DIEBlock::ComputeSizeOnTheFly(StreamEmitter *AP) const {
  unsigned S = 0;
  const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev.getData();
  for (unsigned i = 0, N = Values.size(); i < N; ++i)
    S += Values[i]->SizeOf(AP, AbbrevData[i].getForm());

  return S;
}

/// EmitToRawBuffer - emit data to raw buffer for encoding in debug_loc.
void DIEBlock::EmitToRawBuffer(std::vector<unsigned char> &buffer) {
  auto insertData = [](const void *ptr, unsigned size,
                       std::vector<unsigned char> &vec) {
    for (unsigned i = 0; i < size; ++i) {
      vec.push_back(*(((const unsigned char *)ptr) + i));
    }
  };

  const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev.getData();
  for (unsigned i = 0, N = Values.size(); i < N; ++i) {
    uint64_t intVal = 0;
    switch (Values[i]->getType()) {
    case DIEValue::isInteger:
      intVal = cast<DIEInteger>(Values[i])->getValue();
      if (AbbrevData[i].getForm() == dwarf::DW_FORM_data1)
        insertData(&intVal, sizeof(uint8_t), buffer);
      else if (AbbrevData[i].getForm() == dwarf::DW_FORM_data2)
        insertData(&intVal, sizeof(uint16_t), buffer);
      else if (AbbrevData[i].getForm() == dwarf::DW_FORM_data4)
        insertData(&intVal, sizeof(uint32_t), buffer);
      else if (AbbrevData[i].getForm() == dwarf::DW_FORM_data8)
        insertData(&intVal, sizeof(uint64_t), buffer);
      else if (AbbrevData[i].getForm() == dwarf::DW_FORM_sdata) {
        auto slebsize = getSLEB128Size(intVal);
        auto sleb = malloc(slebsize);
        encodeSLEB128((int64_t)intVal, (uint8_t *)sleb);
        insertData(sleb, slebsize, buffer);
        free(sleb);
      } else if (AbbrevData[i].getForm() == dwarf::DW_FORM_udata) {
        auto ulebsize = getULEB128Size(intVal);
        auto uleb = malloc(ulebsize);
        encodeULEB128(intVal, (uint8_t *)uleb);
        insertData(uleb, ulebsize, buffer);
        free(uleb);
      } else {
        IGC_ASSERT_MESSAGE(false, "Unexpected dwarf encoding form");
      }
      break;
    default:
      IGC_ASSERT_MESSAGE(false, "Unexpected condition");
      break;
    }
  }
}

/// EmitValue - Emit block data.
///
void DIEBlock::EmitValue(StreamEmitter *Asm, dwarf::Form Form) const {
  switch (Form) {
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Improper form for block");
  case dwarf::DW_FORM_block1:
    Asm->EmitInt8(Size);
    break;
  case dwarf::DW_FORM_block2:
    Asm->EmitInt16(Size);
    break;
  case dwarf::DW_FORM_block4:
    Asm->EmitInt32(Size);
    break;
  case dwarf::DW_FORM_block:
    Asm->EmitULEB128(Size);
    break;
  }

  const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev.getData();
  for (unsigned i = 0, N = Values.size(); i < N; ++i)
    Values[i]->EmitValue(Asm, AbbrevData[i].getForm());
}

/// SizeOf - Determine size of block data in bytes.
///
unsigned DIEBlock::SizeOf(StreamEmitter * /*AP*/, dwarf::Form Form) const {
  switch (Form) {
  case dwarf::DW_FORM_block1:
    return Size + sizeof(int8_t);
  case dwarf::DW_FORM_block2:
    return Size + sizeof(int16_t);
  case dwarf::DW_FORM_block4:
    return Size + sizeof(int32_t);
  case dwarf::DW_FORM_block:
    return Size + getULEB128Size(Size);
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Improper form for block");
  }
}

#ifndef NDEBUG
void DIEBlock::print(raw_ostream &O) const {
  O << "    Blk: ";
  DIE::print(O, 6);
}

void DIEBlock::dump() const { print(dbgs()); }
#endif
