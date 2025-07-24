/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfCompilerUnit.cpp
///////////////////////////////////////////////////////////////////////////////

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/IntrinsicInst.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Debug.h"
#include "llvm/Demangle/Demangle.h"
#if LLVM_VERSION_MAJOR >= 11
#include "llvm/CodeGen/DIE.h"
#endif
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MachineLocation.h"
#include <cmath>
#include <optional>
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "DwarfCompileUnit.hpp"
#include "DIE.hpp"
#include "DwarfDebug.hpp"
#include "DwarfExpression.hpp"
#include "StreamEmitter.hpp"
#include "VISADebugInfo.hpp"
#include "VISAModule.hpp"

#include "Compiler/CISACodeGen/messageEncoding.hpp"

#include "Probe/Assertion.h"
#include <cmath>
#define DEBUG_TYPE "dwarfdebug"

using namespace llvm;
using namespace ::IGC;

static ConstantAsMetadata *getConstMdOperand(const MDNode *MD, unsigned OpIdx) {
  return cast<ConstantAsMetadata>(MD->getOperand(OpIdx));
}
static const APInt &getIntConstFromMdOperand(const MDNode *MD, unsigned OpIdx) {
  return getConstMdOperand(MD, OpIdx)->getValue()->getUniqueInteger();
}

PieceBuilder::PieceBuilder(uint16_t RegNum, size_t NumGRFs, uint64_t RegSizeBits, uint64_t VarSizeBits,
                           uint64_t SubRegOffsetBits)
    : NumGRFs(NumGRFs), RegNum(RegNum), RegSizeBits(RegSizeBits), VariableSizeInBits(VarSizeBits),
      SubRegOffsetInBits(SubRegOffsetBits) {
  IGC_ASSERT(SubRegOffsetInBits < RegSizeBits);
  IGC_ASSERT(RegSizeBits > 0);
  IGC_ASSERT(NumGRFs > 0);
}

unsigned PieceBuilder::pieceCount() const {
  auto AlignedSize = SubRegOffsetInBits + VariableSizeInBits;
  auto Count = AlignedSize / RegSizeBits;
  if (AlignedSize % RegSizeBits)
    ++Count;
  constexpr unsigned MaxUint = std::numeric_limits<unsigned>::max();
  bool NoOverflow = Count <= MaxUint;
  IGC_ASSERT_MESSAGE(NoOverflow, "number of required pieces does not fit unsigned int");
  if (!NoOverflow)
    return 0;
  return static_cast<unsigned>(Count);
}

IGC::PieceBuilder::PieceInfo PieceBuilder::get(unsigned index) const {
  assert(index < pieceCount());
  auto AlignedSize = SubRegOffsetInBits + VariableSizeInBits;
  if (index == 0) {
    auto Offset = SubRegOffsetInBits;
    auto Size = (AlignedSize > RegSizeBits) ? RegSizeBits - Offset : VariableSizeInBits;
    return PieceInfo{index + RegNum, Size, Offset};
  }
  if (RegSizeBits * (index + 1) <= AlignedSize) {
    return PieceInfo{index + RegNum, RegSizeBits, 0};
  }
  auto LastChunk = RegSizeBits * index;
  if (LastChunk > AlignedSize)
    return PieceInfo{0, 0, 0};
  return PieceInfo{index + RegNum, AlignedSize - LastChunk, 0};
}

/// CompileUnit - Compile unit constructor.
CompileUnit::CompileUnit(unsigned UID, DIE *D, DICompileUnit *Node, StreamEmitter *A, IGC::DwarfDebug *DW)
    : UniqueID(UID), Node(Node), CUDie(D), Asm(A), EmitSettings(A->GetEmitterSettings()), DD(DW), IndexTyDie(0),
      DebugInfoOffset(0) {
  DIEIntegerOne = new (DIEValueAllocator) DIEInteger(1);
  insertDIE(Node, D);

  // Collect metadata of externally visible functions
  if (DW && DW->GetVISAModule()) {
    auto *M = DW->GetVISAModule()->GetModule();
    for (auto &F : *M) {
      if (F.getSubprogram() &&
          (F.hasFnAttribute("visaStackCall") || F.getCallingConv() == llvm::CallingConv::SPIR_KERNEL)) {
        ExtFunc.insert(F.getSubprogram());
        if (F.getSubprogram()->getDeclaration())
          ExtFunc.insert(F.getSubprogram()->getDeclaration());
      }
    }
  }
}

/// ~CompileUnit - Destructor for compile unit.
CompileUnit::~CompileUnit() {
  for (unsigned j = 0, M = DIEBlocks.size(); j < M; ++j)
    DIEBlocks[j]->~DIEBlock();

  for (unsigned j = 0, M = DIEInlinedStrings.size(); j < M; ++j)
    DIEInlinedStrings[j]->~DIEInlinedString();
}

/// createDIEEntry - Creates a new DIEEntry to be a proxy for a debug
/// information entry.
IGC::DIEEntry *CompileUnit::createDIEEntry(DIE *Entry) {
  DIEEntry *Value = new (DIEValueAllocator) DIEEntry(Entry, DD->getDwarfVersion());
  return Value;
}

/// getDefaultLowerBound - Return the default lower bound for an array. If the
/// DWARF version doesn't handle the language, return -1.
int64_t CompileUnit::getDefaultLowerBound() const {
  switch (getLanguage()) {
  default:
    break;

  case dwarf::DW_LANG_C89:
  case dwarf::DW_LANG_C99:
  case dwarf::DW_LANG_C:
  case dwarf::DW_LANG_C_plus_plus:
  case dwarf::DW_LANG_ObjC:
  case dwarf::DW_LANG_ObjC_plus_plus:
    return 0;

  case dwarf::DW_LANG_Fortran77:
  case dwarf::DW_LANG_Fortran90:
  case dwarf::DW_LANG_Fortran95:
    return 1;

    // The languages below have valid values only if the DWARF version >= 4.
  case dwarf::DW_LANG_Java:
  case dwarf::DW_LANG_Python:
  case dwarf::DW_LANG_UPC:
  case dwarf::DW_LANG_D:
    if (dwarf::DWARF_VERSION >= 4)
      return 0;
    break;

  case dwarf::DW_LANG_Ada83:
  case dwarf::DW_LANG_Ada95:
  case dwarf::DW_LANG_Cobol74:
  case dwarf::DW_LANG_Cobol85:
  case dwarf::DW_LANG_Modula2:
  case dwarf::DW_LANG_Pascal83:
  case dwarf::DW_LANG_PLI:
    if (dwarf::DWARF_VERSION >= 4)
      return 1;
    break;
  }

  return -1;
}

/// Check whether the DIE for this MDNode can be shared across CUs.
static bool isShareableAcrossCUs(const llvm::MDNode *D) {
  // When the MDNode can be part of the type system, the DIE can be
  // shared across CUs.
  return (isa<DIType>(D) || (isa<DISubprogram>(D) && !(cast<DISubprogram>(D)->isDefinition())));
}

/// getDIE - Returns the debug information entry map slot for the
/// specified debug variable. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
::IGC::DIE *CompileUnit::getDIE(llvm::DINode *D) const {
  if (isShareableAcrossCUs(D))
    return DD->getDIE(D);
  return MDNodeToDieMap.lookup(D);
}

/// insertDIE - Insert DIE into the map. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
void CompileUnit::insertDIE(llvm::MDNode *Desc, DIE *D) {
  if (isShareableAcrossCUs(Desc)) {
    DD->insertDIE(Desc, D);
    return;
  }
  MDNodeToDieMap.insert(std::make_pair(Desc, D));
}

/// addFlag - Add a flag that is true.
void CompileUnit::addFlag(DIE *Die, dwarf::Attribute Attribute) {
  if (DD->getDwarfVersion() >= 4)
    Die->addValue(Attribute, dwarf::DW_FORM_flag_present, DIEIntegerOne);
  else
    Die->addValue(Attribute, dwarf::DW_FORM_flag, DIEIntegerOne);
}

/// addUInt - Add an unsigned integer attribute data and value.
///
void CompileUnit::addUInt(DIE *Die, dwarf::Attribute Attribute, std::optional<dwarf::Form> Form, uint64_t Integer) {
  if (!Form) {
    Form = DIEInteger::BestForm(false, Integer);
  }
  DIEValue *Value = Integer == 1 ? DIEIntegerOne : new (DIEValueAllocator) DIEInteger(Integer);
  Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addUInt(IGC::DIEBlock *Block, dwarf::Form Form, uint64_t Integer) {
  IGC_ASSERT_MESSAGE(
      (Form == dwarf::Form::DW_FORM_data1 && Integer <= std::numeric_limits<unsigned char>::max()) ||
          (Form == dwarf::Form::DW_FORM_data2 && Integer <= std::numeric_limits<unsigned short>::max()) ||
          (Form == dwarf::Form::DW_FORM_data4 && Integer <= std::numeric_limits<unsigned int>::max() ||
           (Form != dwarf::Form::DW_FORM_data1 && Form != dwarf::Form::DW_FORM_data2 &&
            Form != dwarf::Form::DW_FORM_data4)),
      "Insufficient bits in form for encoding");
  addUInt(Block, (dwarf::Attribute)0, Form, Integer);
}

void CompileUnit::addBitPiece(IGC::DIEBlock *Block, uint64_t SizeBits, uint64_t OffsetBits) {
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
  addUInt(Block, dwarf::DW_FORM_udata, SizeBits);
  addUInt(Block, dwarf::DW_FORM_udata, OffsetBits);
}

/// addSInt - Add an signed integer attribute data and value.
///
void CompileUnit::addSInt(DIE *Die, dwarf::Attribute Attribute, std::optional<dwarf::Form> Form, int64_t Integer) {
  if (!Form) {
    Form = DIEInteger::BestForm(true, Integer);
  }
  DIEValue *Value = new (DIEValueAllocator) DIEInteger(Integer);
  Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addSInt(IGC::DIEBlock *Die, std::optional<dwarf::Form> Form, int64_t Integer) {
  IGC_ASSERT_MESSAGE((Form == dwarf::Form::DW_FORM_data1 && Integer >= std::numeric_limits<signed char>::min() &&
                      Integer <= std::numeric_limits<signed char>::max()) ||
                         (Form == dwarf::Form::DW_FORM_data2 && Integer >= std::numeric_limits<short>::min() &&
                          Integer <= std::numeric_limits<short>::max()) ||
                         (Form == dwarf::Form::DW_FORM_data4 && Integer >= std::numeric_limits<int>::min() &&
                          Integer <= std::numeric_limits<int>::max()) ||
                         (Form != dwarf::Form::DW_FORM_data1 && Form != dwarf::Form::DW_FORM_data2 &&
                          Form != dwarf::Form::DW_FORM_data4),
                     "Insufficient bits in form for encoding");
  addSInt(Die, (dwarf::Attribute)0, Form, Integer);
}

/// addString - Add a string attribute data and value. We always emit a
/// reference to the string pool instead of immediate strings so that DIEs have
/// more predictable sizes. In the case of split dwarf we emit an index
/// into another table which gets us the static offset into the string
/// table.
void CompileUnit::addString(DIE *Die, dwarf::Attribute Attribute, StringRef String) {
  // Emit string inlined
  auto Str = new (DIEValueAllocator) DIEInlinedString(String);
  // Collect all inlined string DIEs to later call dtor
  DIEInlinedStrings.push_back(Str);
  Die->addValue(Attribute, dwarf::DW_FORM_string, Str);
}

/// addExpr - Add a Dwarf expression attribute data and value.
///
void CompileUnit::addExpr(IGC::DIEBlock *Die, dwarf::Form Form, const MCExpr *Expr) {
  DIEValue *Value = new (DIEValueAllocator) DIEExpr(Expr);
  Die->addValue((dwarf::Attribute)0, Form, Value);
}

/// addLabel - Add a Dwarf label attribute data and value.
///
void CompileUnit::addLabel(DIE *Die, dwarf::Attribute Attribute, dwarf::Form Form, const MCSymbol *Label) {
  DIEValue *Value = new (DIEValueAllocator) DIELabel(Label);
  Die->addValue(Attribute, Form, Value);
}

void CompileUnit::addLabel(IGC::DIEBlock *Die, dwarf::Form Form, const MCSymbol *Label) {
  addLabel(Die, (dwarf::Attribute)0, Form, Label);
}

/// addLabelAddress - Add a dwarf label attribute data and value using
/// DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addLabelAddress(DIE *Die, dwarf::Attribute Attribute, MCSymbol *Label) {
  if (Label) {
    DD->addArangeLabel(SymbolCU(this, Label));
  }

  if (Label != NULL) {
    DIEValue *Value = new (DIEValueAllocator) DIELabel(Label);
    Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
  } else {
    DIEValue *Value = new (DIEValueAllocator) DIEInteger(0);
    Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
  }
}

void CompileUnit::addLabelLoc(DIE *Die, dwarf::Attribute Attribute, MCSymbol *Label) {
  if (Label != NULL) {
    DD->addArangeLabel(SymbolCU(this, Label));
    DIEValue *Value = new (DIEValueAllocator) DIELabel(Label);
    Die->addValue(Attribute, dwarf::DW_FORM_sec_offset, Value);
  }
}

/// addOpAddress - Add a dwarf op address data and value using the
/// form given and an op of either DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addOpAddress(IGC::DIEBlock *Die, const MCSymbol *Sym) {
  DD->addArangeLabel(SymbolCU(this, Sym));
  addUInt(Die, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
  addLabel(Die, dwarf::DW_FORM_udata, Sym);
}

/// addDelta - Add a label delta attribute data and value.
///
void CompileUnit::addDelta(DIE *Die, dwarf::Attribute Attribute, dwarf::Form Form, const MCSymbol *Hi,
                           const MCSymbol *Lo) {
  DIEValue *Value = new (DIEValueAllocator) DIEDelta(Hi, Lo);
  Die->addValue(Attribute, Form, Value);
}

/// addDIEEntry - Add a DIE attribute data and value.
///
void CompileUnit::addDIEEntry(DIE *Die, dwarf::Attribute Attribute, DIE *Entry) {
  addDIEEntry(Die, Attribute, createDIEEntry(Entry));
}

void CompileUnit::addDIEEntry(DIE *Die, dwarf::Attribute Attribute, DIEEntry *Entry) {
  const DIE *DieCU = Die->getCompileUnitOrNull();
  const DIE *EntryCU = Entry->getEntry()->getCompileUnitOrNull();
  if (!DieCU) {
    // We assume that Die belongs to this CU, if it is not linked to any CU yet.
    DieCU = getCUDie();
  }
  if (!EntryCU) {
    EntryCU = getCUDie();
  }
  dwarf::Form Form = EntryCU == DieCU ? dwarf::DW_FORM_ref4 : dwarf::DW_FORM_ref_addr;
  Die->addValue(Attribute, Form, Entry);
}

/// Create a DIE with the given Tag, add the DIE to its parent, and
/// call insertDIE if MD is not null.
IGC::DIE *CompileUnit::createAndAddDIE(unsigned Tag, DIE &Parent, llvm::DINode *N) {
  DIE *Die = new DIE(Tag);
  Parent.addChild(Die);
  if (N) {
    insertDIE(N, Die);
  }

  return Die;
}

/// addBlock - Add block data.
///
void CompileUnit::addBlock(DIE *Die, dwarf::Attribute Attribute, IGC::DIEBlock *Block) {
  Block->ComputeSize(Asm);
  DIEBlocks.push_back(Block); // Memoize so we can call the destructor later on.
  Die->addValue(Attribute, Block->BestForm(), Block);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIScope *S, unsigned Line) {
  // If the line number is 0, don't add it.
  if (Line == 0)
    return;

  unsigned FileID = DD->getOrCreateSourceID(S->getFilename(), S->getDirectory(), getUniqueID());
  IGC_ASSERT_MESSAGE(FileID, "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, std::nullopt, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, std::nullopt, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIImportedEntity *IE, unsigned Line) {
  // If the line number is 0, don't add it.
  if (Line == 0)
    return;

  unsigned FileID = DD->getOrCreateSourceID(IE->getFile()->getFilename(), IE->getFile()->getDirectory(), getUniqueID());
  IGC_ASSERT_MESSAGE(FileID, "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, std::nullopt, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, std::nullopt, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIVariable *V) {
  // Verify variable.
  if (!isa<DIVariable>(V))
    return;

  addSourceLine(Die, V->getScope(), V->getLine());
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DISubprogram *SP) {
  // Verify subprogram.
  if (!isa<DISubprogram>(SP))
    return;

  addSourceLine(Die, SP, SP->getLine());
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIType *Ty) {
  // Verify type.
  if (!isa<DIType>(Ty))
    return;

  addSourceLine(Die, Ty, Ty->getLine());
}

void CompileUnit::addRegOrConst(IGC::DIEBlock *TheDie, unsigned DWReg) {
  // TODO: Confirm if this function is correctly used.
  auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(DWReg);
  addConstantUValue(TheDie, DWRegEncoded);
}

/// addRegisterOp - Add register operand.
void CompileUnit::addRegisterOp(IGC::DIEBlock *TheDie, unsigned DWReg) {
  auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(DWReg);
  if (DWRegEncoded < 32) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_reg0 + DWRegEncoded);
  } else {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_regx);
    addUInt(TheDie, dwarf::DW_FORM_udata, DWRegEncoded);
  }
}

/// addRegisterOffset - Add register offset.
void CompileUnit::addRegisterOffset(IGC::DIEBlock *TheDie, unsigned DWReg, int64_t Offset) {
  auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(DWReg);
  if (DWRegEncoded < 32) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_breg0 + DWRegEncoded);
  } else {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_bregx);
    addUInt(TheDie, dwarf::DW_FORM_udata, DWRegEncoded);
  }
  addSInt(TheDie, dwarf::DW_FORM_sdata, Offset);
}

/// isTypeSigned - Return true if the type is signed.
static bool isTypeSigned(DwarfDebug *DD, DIType *Ty, int *SizeInBits) {
  if (isa<DIDerivedType>(Ty)) {
    return isTypeSigned(DD, DD->resolve(cast<DIDerivedType>(Ty)->getBaseType()), SizeInBits);
  }
  if (isa<DIBasicType>(Ty)) {
    if (cast<DIBasicType>(Ty)->getEncoding() == dwarf::DW_ATE_signed ||
        cast<DIBasicType>(Ty)->getEncoding() == dwarf::DW_ATE_signed_char) {
      *SizeInBits = (int)cast<DIBasicType>(Ty)->getSizeInBits();
      return true;
    }
  }
  return false;
}

/// Return true if type encoding is unsigned.
bool isUnsignedDIType(DwarfDebug *DD, DIType *Ty) {
  DIDerivedType *DTy = dyn_cast_or_null<DIDerivedType>(Ty);
  if (DTy) {
    return isUnsignedDIType(DD, DD->resolve(DTy->getBaseType()));
  }

  DIBasicType *BTy = dyn_cast_or_null<DIBasicType>(Ty);
  if (BTy) {
    unsigned Encoding = BTy->getEncoding();
    if (Encoding == dwarf::DW_ATE_unsigned || Encoding == dwarf::DW_ATE_unsigned_char ||
        Encoding == dwarf::DW_ATE_boolean) {
      return true;
    }
  }
  return false;
}

/// addConstantFPValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantFPValue(DIE *Die, const ConstantFP *CFP) {
  // Pass this down to addConstantValue as an unsigned bag of bits.
  addConstantValue(Die, CFP->getValueAPF().bitcastToAPInt(), true);
}

/// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE *Die, const ConstantInt *CI, bool Unsigned) {
  addConstantValue(Die, CI->getValue(), Unsigned);
}

// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE *Die, const APInt &Val, bool Unsigned) {
  unsigned CIBitWidth = Val.getBitWidth();
  if (CIBitWidth <= 64) {
    // If we're a signed constant definitely use sdata.
    if (!Unsigned) {
      addSInt(Die, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, Val.getSExtValue());
      return;
    }

    // Else use data for now unless it's larger than we can deal with.
    dwarf::Form Form;
    switch (CIBitWidth) {
    case 8:
      Form = dwarf::DW_FORM_data1;
      break;
    case 16:
      Form = dwarf::DW_FORM_data2;
      break;
    case 32:
      Form = dwarf::DW_FORM_data4;
      break;
    case 64:
      Form = dwarf::DW_FORM_data8;
      break;
    default:
      Form = dwarf::DW_FORM_udata;
      break;
    }
    addUInt(Die, dwarf::DW_AT_const_value, Form, Val.getZExtValue());
    return;
  }

  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();

  // Get the raw data form of the large APInt.
  const uint64_t *Ptr64 = Val.getRawData();

  int NumBytes = Val.getBitWidth() / 8; // 8 bits per byte.
  bool LittleEndian = Asm->IsLittleEndian();

  // Output the constant to DWARF one byte at a time.
  for (int i = 0; i < NumBytes; i++) {
    uint8_t c;
    if (LittleEndian) {
      c = (uint8_t)(Ptr64[i / 8] >> (8 * (i & 7)));
    } else {
      c = (uint8_t)(Ptr64[(NumBytes - 1 - i) / 8] >> (8 * ((NumBytes - 1 - i) & 7)));
    }
    addUInt(Block, dwarf::DW_FORM_data1, c);
  }

  addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addConstantUValue - Add constant unsigned value entry in variable DIEBlock.
void CompileUnit::addConstantUValue(DIEBlock *TheDie, uint64_t Val) {
  if (Val <= 31) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_lit0 + Val);
  } else if (Val <= 0xff) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
    addUInt(TheDie, dwarf::DW_FORM_data1, Val);
  } else if (Val <= 0xffff) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
    addUInt(TheDie, dwarf::DW_FORM_data2, Val);
  } else if (Val <= 0xffffffff) {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_const4u);
    addUInt(TheDie, dwarf::DW_FORM_data4, Val);
  } else {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
    addUInt(TheDie, dwarf::DW_FORM_data8, Val);
  }
}

/// addConstantData - Add constant data entry in variable DIE.
void CompileUnit::addConstantData(DIE *Die, const unsigned char *Ptr8, int NumBytes) {
  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();

  bool LittleEndian = Asm->IsLittleEndian();

  // Output the constant to DWARF one byte at a time.
  for (int i = 0; i < NumBytes; i++) {
    uint8_t c = (LittleEndian) ? Ptr8[i] : Ptr8[(NumBytes - 1 - i)];

    addUInt(Block, dwarf::DW_FORM_data1, c);
  }

  addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addTemplateParams - Add template parameters into buffer.
void CompileUnit::addTemplateParams(DIE &Buffer, llvm::DINodeArray TParams) {
  // Add template parameters.
  for (const auto *Element : TParams) {
    if (auto *TTP = dyn_cast<DITemplateTypeParameter>(Element))
      constructTemplateTypeParameterDIE(Buffer, const_cast<DITemplateTypeParameter *>(TTP));
    else if (auto *TVP = dyn_cast<DITemplateValueParameter>(Element))
      constructTemplateValueParameterDIE(Buffer, const_cast<DITemplateValueParameter *>(TVP));
  }
}

/// getOrCreateContextDIE - Get context owner's DIE.
IGC::DIE *CompileUnit::getOrCreateContextDIE(DIScope *Context) {
  if (!Context || isa<DIFile>(Context))
    return getCUDie();
  if (auto *T = dyn_cast<DIType>(Context))
    return getOrCreateTypeDIE(T);
  if (auto *NS = dyn_cast<DINamespace>(Context))
    return getOrCreateNameSpace(NS);
  if (auto *SP = dyn_cast<DISubprogram>(Context))
    return getOrCreateSubprogramDIE(SP);
  if (auto *MD = dyn_cast<DIModule>(Context))
    return getOrCreateModuleDIE(MD);

  return getDIE(Context);
}

/// getOrCreateTypeDIE - Find existing DIE or create new DIE for the
/// given DIType.
IGC::DIE *CompileUnit::getOrCreateTypeDIE(const MDNode *TyNode) {
  if (!TyNode)
    return NULL;

  DIType *const Ty = cast_or_null<DIType>(const_cast<MDNode *>(TyNode));
  IGC_ASSERT(nullptr != Ty);

  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *const ContextDIE = getOrCreateContextDIE(resolve(Ty->getScope()));
  IGC_ASSERT(nullptr != ContextDIE);

  DIE *TyDIE = getDIE(Ty);
  if (nullptr != TyDIE)
    return TyDIE;

  // Create new type.
  TyDIE = createAndAddDIE(Ty->getTag(), *ContextDIE, Ty);

  if (isa<DIBasicType>(Ty))
    constructTypeDIE(*TyDIE, cast<DIBasicType>(Ty));
#if LLVM_VERSION_MAJOR >= 12
  else if (isa<DIStringType>(Ty))
    constructTypeDIE(*TyDIE, cast<DIStringType>(Ty));
#endif
  else if (isa<DICompositeType>(Ty))
    constructTypeDIE(*TyDIE, cast<DICompositeType>(Ty));
  else if (isa<DISubroutineType>(Ty))
    constructTypeDIE(*TyDIE, cast<DISubroutineType>(Ty));
  else {
    IGC_ASSERT_MESSAGE(isa<DIDerivedType>(Ty), "Unknown kind of DIType");
    constructTypeDIE(*TyDIE, cast<DIDerivedType>(Ty));
  }

  return TyDIE;
}

/// addType - Add a new type attribute to the specified entity.
void CompileUnit::addType(DIE *Entity, DIType *Ty, dwarf::Attribute Attribute) {
  IGC_ASSERT_MESSAGE(nullptr != Ty, "Trying to add a type that doesn't exist?");

  // Check for pre-existence.
  DIEEntry *Entry = getDIEEntry(Ty);
  // If it exists then use the existing value.
  if (Entry) {
    addDIEEntry(Entity, Attribute, Entry);
    return;
  }

  // Construct type.
  DIE *Buffer = getOrCreateTypeDIE(Ty);

  // Set up proxy.
  Entry = createDIEEntry(Buffer);
  insertDIEEntry(Ty, Entry);
  addDIEEntry(Entity, Attribute, Entry);
}

// addSimdWidth - add SIMD width
void CompileUnit::addSimdWidth(DIE *Die, uint16_t SimdWidth) {
  // Emit SIMD width
  addUInt(Die, (dwarf::Attribute)DW_AT_INTEL_simd_width, dwarf::DW_FORM_data2, SimdWidth);
}

void CompileUnit::extractSubRegValue(IGC::DIEBlock *Block, unsigned char Sz) {
  // This function expects that reg # and sub-reg in bits is
  // already pushed to DWARF stack.
  addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_regval_bits);
  addUInt(Block, dwarf::DW_FORM_data1, Sz);
}

// addBindlessOrStatelessLocation - add a sequence of attributes to calculate
// stateless or bindless location of variable. baseAddr is one of the following
// base addreses:
// - General State Base Address when variable located in stateless surface
// - Bindless Surface State Base Address when variable located in bindless
// surface
// - Bindless Sampler State Base Addres when variable located in bindless
// sampler Note: Scratch space location is not handled here.
void CompileUnit::addBindlessOrStatelessLocation(IGC::DIEBlock *Block, const VISAVariableLocation &Loc,
                                                 uint32_t baseAddrEncoded) {
  IGC_ASSERT_MESSAGE(Loc.IsInGlobalAddrSpace(), "Neither bindless nor stateless");
  if (Loc.IsRegister()) {
    // Stateless surface or bindless surface or bindless sampler with offset not
    // available as literal. For example, if offset were available in register
    // r0 and we assume the DWARF number of r0 to be 16, the following
    // expression can be generated for stateless surface: 1 DW_OP_breg7 0 ,
    // breg7 stands for General State Base Address 2 DW_OP_breg16 0 3 DW_OP_plus
    // or for bindless surface:
    // 1 DW_OP_breg9 0            , breg9 stands for Bindless Surface State Base
    // Address 2 DW_OP_breg16 0 3 DW_OP_plus or for bindless sampler: 1
    // DW_OP_breg10 0           , breg10 stands for Bindless Sampler State Base
    // Address 2 DW_OP_breg16 0 3 DW_OP_plus
    uint32_t regNum = Loc.GetRegister();
    const auto *VISAMod = Loc.GetVISAModule();

    const auto *VarInfo = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNum);
    if (!VarInfo) {
      LLVM_DEBUG(dbgs() << "warning: register is not available "
                        << "for bindless surface offset of a V" << regNum);
      return;
    }

    uint16_t grfRegNum = VarInfo->lrs.front().getGRF().regNum;
    unsigned int grfSubReg = VarInfo->lrs.front().getGRF().subRegNum;
    auto bitOffsetToSurfReg = grfSubReg * 8; // Bit-offset to GRF with surface offset
    auto varSizeInBits = 32;                 // To be verified with DV.getRegisterValueSizeInBits(DD);

    addUInt(Block, dwarf::DW_FORM_data1,
            baseAddrEncoded); // Base address of surface or sampler
    addSInt(Block, dwarf::DW_FORM_sdata,
            0); // No literal offset to base address

    addRegOrConst(Block, grfRegNum); // Surface offset (in GRF) to base address

    addConstantUValue(Block, bitOffsetToSurfReg);
    extractSubRegValue(Block, varSizeInBits);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
    return;
  }
  if (Loc.HasLocation()) // Is offset available as literal?
  {
    // Stateless (BTI 255 or 253) addressing using surface offset available as
    // literal 1 DW_OP_breg7 <offset>    , breg7 stands for General State Base
    // Address or Bindless Surface addressing using surface offset available as
    // literal 1 DW_OP_breg9 <offset>   , breg9 stands for Bindless Surface
    // State Base Address or Bindless Sampler addressing using surface offset
    // available as literal 1 DW_OP_breg10 <offset>   , breg10 stands for
    // Bindless Sampler State Base Address
    uint32_t offset = Loc.GetOffset();

    addUInt(Block, dwarf::DW_FORM_data1,
            baseAddrEncoded);                     // Base address of surface or sampler
    addSInt(Block, dwarf::DW_FORM_sdata, offset); // Offset to base address
    return;
  }
  IGC_ASSERT_MESSAGE(0, "Unexpected bindless or stateless variable - offset "
                        "neither literal nor in register");
}

// addStatelessLocation - add a sequence of attributes to calculate stateless
// surface location of variable
void CompileUnit::addStatelessLocation(IGC::DIEBlock *Block, const VISAVariableLocation &Loc) {
  // Use virtual debug register with Stateless Surface State Base Address
  uint32_t statelessBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::GenStateBase>(dwarf::DW_OP_breg0);
  IGC_ASSERT_MESSAGE(Loc.HasSurface(), "Missing surface for variable location");

  addBindlessOrStatelessLocation(Block, Loc, statelessBaseAddrEncoded);
}

// addBindlessSurfaceLocation - add a sequence of attributes to calculate
// bindless surface location of variable
void CompileUnit::addBindlessSurfaceLocation(IGC::DIEBlock *Block, const VISAVariableLocation &Loc) {
  // Use virtual debug register with Bindless Surface State Base Address
  uint32_t bindlessSurfBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::BindlessSurfStateBase>(dwarf::DW_OP_breg0);

  IGC_ASSERT_MESSAGE(Loc.HasSurface(), "Missing surface for variable location");

  // Bindless Surface addressing using bindless offset stored in a register (for
  // example) r0, while offset is literal: 1 DW_OP_reg16 2 DW_OP_const1u
  // <bit-offset to reg0> 3 DW_OP_const1u 32 4 DW_OP_push_bit_piece_stack 5
  // DW_OP_breg9 32 6 DW_OP_plus 7 DW_OP_deref 8 DW_OP_plus_uconst <offset> or
  // Bindless Surface addressing using bindless offset and surface offset both
  // stored in a register (for example) r0 contains bindless offset while r1
  // contains surface offset. 1 DW_OP_reg16 2 DW_OP_const1u <bit-offset to reg0>
  // 3 DW_OP_const1u 32
  // 4 DW_OP_push_bit_piece_stack
  // 5 DW_OP_breg9 32
  // 6 DW_OP_plus
  // 7 DW_OP_deref
  // 8 DW_OP_reg17
  // 9 DW_OP_const1u <bit-offset to reg1>
  // 10 DW_OP_const1u 32
  // 11 DW_OP_push_bit_piece_stack

  uint16_t regNumWithBindlessOffset = 0; // TBD Bindless offset in GRF
  uint16_t bitOffsetToBindlessReg = 0;   // TBD bit-offset to GRF with bindless offset

  addRegOrConst(Block, // Bindless offset to base address
                regNumWithBindlessOffset);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
  addUInt(Block, dwarf::DW_FORM_data1, bitOffsetToBindlessReg);

  extractSubRegValue(Block, 32);

  addUInt(Block, dwarf::DW_FORM_data1,
          bindlessSurfBaseAddrEncoded); // Bindless Surface Base Address
  addUInt(Block, dwarf::DW_FORM_udata, 32);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);

  if (Loc.IsRegister()) // Is surface offset available as register?
  {
    auto regNum = Loc.GetRegister();
    const auto *VISAMod = Loc.GetVISAModule();

    const auto *VarInfo = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNum);
    if (!VarInfo) {
      LLVM_DEBUG(dbgs() << "warning: register is not available "
                        << "for bindless surface offset of a V" << regNum);
      return;
    }

    uint16_t regNumWithSurfOffset = VarInfo->lrs.front().getGRF().regNum;
    unsigned int subReg = VarInfo->lrs.front().getGRF().subRegNum;
    auto bitOffsetToSurfReg = subReg * 8; // Bit-offset to GRF with surface offset
    // auto sizeInBits = (VISAMod->m_pShader->getGRFSize() * 8) - offsetInBits;

    addRegOrConst(Block, // Surface offset (in GRF) to base address
                  regNumWithSurfOffset);
    addConstantUValue(Block, bitOffsetToSurfReg);
    extractSubRegValue(Block, 32);
  }
  if (Loc.HasLocation()) // Is surface offset available as literal?
  {
    uint32_t offset = Loc.GetOffset(); // Surface offset
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
    addUInt(Block, dwarf::DW_FORM_udata, offset);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unexpected bindless variable - offset neither "
                          "literal nor in register");
  }
}

// addBindlessScratchSpaceLocation - add a sequence of attributes to calculate
// bindless scratch space location of variable
void CompileUnit::addBindlessScratchSpaceLocation(IGC::DIEBlock *Block, const VISAVariableLocation &Loc) {
  // Use virtual debug register with Surface State Base Address
  uint32_t surfStateBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::SurfStateBase>(dwarf::DW_OP_breg0);

  IGC_ASSERT_MESSAGE(Loc.HasSurface(), "Missing surface for variable location");

  // Note: Bindless scratch space offset aka Scratch Space Pointer is located in
  // preserved r0 GRF register, on bits 31:10 of r0.5 subregister.
  //
  // Bindless Surface addressing using bindless offset stored in a register r0,
  // while surface offset is literal:
  // 1 DW_OP_reg16
  // 2 DW_OP_const1u 5*32          , Offset in bits to r0.5
  // 3 DW_OP_const1u 32            , 32-bit long bindless offset
  // 4 DW_OP_const4u 0xffffffc0    , which is 1K-byte aligned
  // 5 DW_OP_and
  // 6 DW_OP_push_bit_piece_stack
  // 7 DW_OP_breg8 32              , we add the surface state base address plus
  // the field offset 8 DW_OP_plus                  , to fetch the surface base
  // address inside the RENDER_SURFACE_STATE object 9 DW_OP_deref 10
  // DW_OP_plus_uconst <offset> or Bindless Surface addressing using bindless
  // offset (r0.5 [31:10]) and surface offset both stored in a register while r1
  // (for example) contains surface offset. 1 DW_OP_reg16 2 DW_OP_const1u 5*32
  // , Offset in bits to r0.5 3 DW_OP_const1u 32            , 32-bit long
  // bindless offset 4 DW_OP_const4u 0xffffffc0    , which is 1K-byte aligned 5
  // DW_OP_and 6 DW_OP_push_bit_piece_stack 7 DW_OP_breg8 32 8 DW_OP_plus 9
  // DW_OP_deref 10 DW_OP_reg17 11 DW_OP_const1u <bit-offset to reg1> 12
  // DW_OP_const1u 32 13 DW_OP_push_bit_piece_stack

  uint16_t regNumWithBindlessOffset = 0; // TBD Bindless offset in GRF

  addRegOrConst(Block, // Bindless offset to base address
                regNumWithBindlessOffset);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
  addUInt(Block, dwarf::DW_FORM_data1,
          5 * 32); // bit offset to Scratch Space Pointer in r0.5
  extractSubRegValue(Block, 32);
  addUInt(Block, dwarf::DW_FORM_data1,
          surfStateBaseAddrEncoded); // Bindless Surface Base Address
  addUInt(Block, dwarf::DW_FORM_udata, 32);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);

  if (Loc.IsRegister()) // Is surface offset available as register?
  {
    auto regNum = Loc.GetRegister();
    const auto *VISAMod = Loc.GetVISAModule();

    const auto *VarInfo = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNum);
    if (!VarInfo) {
      LLVM_DEBUG(dbgs() << "warning: could not build bindless scratch offset (V" << regNum << ")");
      return;
    }

    uint16_t regNumWithSurfOffset = VarInfo->lrs.front().getGRF().regNum;
    unsigned int subReg = VarInfo->lrs.front().getGRF().subRegNum;
    auto bitOffsetToSurfReg = subReg * 8; // Bit-offset to GRF with surface offset
    // auto sizeInBits = (VISAMod->m_pShader->getGRFSize() * 8) - offsetInBits;

    addRegOrConst(Block, // Surface offset (in GRF) to base address
                  regNumWithSurfOffset);
    addConstantUValue(Block, bitOffsetToSurfReg);
    extractSubRegValue(Block, 32);
  } else if (Loc.HasLocation()) // Is surface offset available as literal?
  {
    uint32_t offset = Loc.GetOffset(); // Surface offset

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
    addUInt(Block, dwarf::DW_FORM_udata, offset);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unexpected bindless variable - offset neither "
                          "literal nor in register");
  }
}

// addBindlessSamplerLocation - add a sequence of attributes to calculate
// bindless sampler location of variable
void CompileUnit::addBindlessSamplerLocation(IGC::DIEBlock *Block, const VISAVariableLocation &Loc) {
  // Use virtual debug register with Bindless Sampler State Base Address
  uint32_t bindlessSamplerBaseAddrEncoded =
      GetEncodedRegNum<RegisterNumbering::BindlessSamplerStateBase>(dwarf::DW_OP_breg0);

  IGC_ASSERT_MESSAGE(Loc.IsSampler(), "Missing sampler for variable location");

  addBindlessOrStatelessLocation(Block, Loc, bindlessSamplerBaseAddrEncoded);
}

void CompileUnit::addBE_FP(IGC::DIEBlock *Block) {
  // Add BE_FP value to spill offset if BE_FP is non-null. This is required when
  // generating debug info for stack call functions. DW_OP_constu <regNum>
  // DW_OP_INTEL_regs
  // DW_OP_const2u <subRegNum * 8>
  // DW_OP_const2u <32>
  // DW_OP_plus
  uint32_t BE_FP_RegNum = 0, BE_FP_SubRegNum = 0;
  const auto &VisaDbgInfo = DD->getVisaDebugInfo();

  bool hasValidBEFP = VisaDbgInfo.getCFI().getBEFPRegNum(BE_FP_RegNum, BE_FP_SubRegNum);
  if (!hasValidBEFP)
    return;

  addRegOrConst(Block, BE_FP_RegNum); // Register ID will be shifted by offset

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
  addUInt(Block, dwarf::DW_FORM_data2,
          BE_FP_SubRegNum * 8u); // sub-reg offset in bits
  extractSubRegValue(Block, 32);
  if (EmitSettings.ScratchOffsetInOW) {
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
    addUInt(Block, dwarf::DW_FORM_data1, 16);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_mul);
  }

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
}

// addScratchLocation - add a sequence of attributes to emit scratch space
// location of variable
void CompileUnit::addScratchLocation(IGC::DIEBlock *Block, uint32_t memoryOffset, int32_t vectorOffset) {
  uint32_t offset = memoryOffset + vectorOffset;
  // For spills to the scratch area at offset available as literal
  uint32_t scratchBaseAddrEncoded = 0;
  if (DD->GetVISAModule()->usesSlot1ScratchSpill()) {
    // 1 DW_OP_breg11 <offset>    , breg11 stands for Scratch Space Base
    //                              Address + 1 (Slot#1)
    scratchBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::ScratchBaseSlot1>(dwarf::DW_OP_breg0);
  } else {
    // 1 DW_OP_breg6 <offset>    , breg6 stands for Scratch Space Base Address
    scratchBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::ScratchBase>(dwarf::DW_OP_breg0);
  }

  addUInt(Block, dwarf::DW_FORM_data1,
          scratchBaseAddrEncoded);              // Scratch Base Address
  addSInt(Block, dwarf::DW_FORM_sdata, offset); // Offset to base address
}

// addSimdLane - add a sequence of attributes to calculate location of
// vectorized variable among SIMD lanes, e.g. a GRF subregister.
//
// CASE 1: Example of expression generated for 64-bit (or 32-bit) pointer to a
// variable, which is located in scratch: (note: DW_OP_const8u address is
// generated earlier) DW_OP_INTEL_push_simd_lane DW_OP_lit3 (or lit2 for 32-bit
// ptr) DW_OP_shl DW_OP_plus DW_OP_deref
//
// CASE 2: Example of expressions generated for 64-bit ptr addresses in SIMD8 or
// SIMD16: 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit2
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit3
// 8 DW_OP_and
// 9 DW_OP_const1u 64
// 10 DW_OP_mul
// 11 DW_OP_const1u 64
// 12 DW_OP_INTEL_push_bit_piece_stack
//
// CASE 3: Example of expressions generated for 32-bit ptr in SIMD8 or SIMD16
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit3
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit7
// 8 DW_OP_and
// 9 DW_OP_const1u 32
// 10 DW_OP_mul
// 11 DW_OP_const1u 32  (or 16 or 8)
// 12 DW_OP_INTEL_bit_piece_stack
//
// CASE 4: Example of expression generated for 64-bit or 32-bit or
// 16-bit packed or 8-bit packed variable in SIMD8 or SIMD16:
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit2 or lit3 or lit4 or lit5 respectively for 64/32/16/8 bit variable
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit3 or lit7 or lit15 or lit31 respectively for 64/32/16/8 bit
// variable 8 DW_OP_and 9 DW_OP_const1u 64 or 32 or 16 or 8 10 DW_OP_mul 11
// DW_OP_const1u 64 or 32 or 16 or 8 12 DW_OP_INTEL_bit_piece_stack
//
// CASE 5: Example of expression generated for 16-bit or 8-bit variable unpacked
// in SIMD8 or SIMD16:
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit3
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit7
// 8 DW_OP_and
// 9 DW_OP_const1u 32
// 10 DW_OP_mul
// 11 DW_OP_const1u 16 or 8
// 12 DW_OP_INTEL_bit_piece_stack
//
void CompileUnit::addSimdLane(IGC::DIEBlock *Block, const DbgVariable &DV, const VISAVariableLocation &Loc,
                              const DbgDecoder::LiveIntervalsVISA *lr, uint16_t simdWidthOffset, bool isPacked,
                              bool isSecondHalf) {
  auto EmitPushSimdLane = [this](IGC::DIEBlock *Block, bool isSecondHalf) {
    addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_simd_lane);
    if (isSecondHalf) {
      // Fix offset to use for second half of SIMD32
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit16);
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_minus);
    }
  };

  if (!Loc.IsVectorized())
    return;

  // SIMD lane
  const auto *VISAMod = Loc.GetVISAModule();
  auto varSizeInBits = DV.getRegisterValueSizeInBits(DD);

  LLVM_DEBUG(dbgs() << "  addSimdLane(varSizeInBits: " << varSizeInBits << ", simdWidthOffset: " << simdWidthOffset
                    << ", isPacked: " << isPacked << ", isSecondHalf: " << isSecondHalf << ")\n");
  IGC_ASSERT_MESSAGE(varSizeInBits % 8 == 0, "Variable's size not aligned to byte");

  if (lr->isSpill()) {
    // CASE 1: Example of expression generated for 64-bit or 32-bit ptr to a
    // variable, which is located in scratch: (note: DW_OP_const8u address is
    // generated earlier) DW_OP_INTEL_push_simd_lane DW_OP_lit3 (or DW_OP_lit2
    // for 32-bit ptr) DW_OP_shl DW_OP_plus DW_OP_deref

    EmitPushSimdLane(Block, isSecondHalf);
    // *8 if 64-bit ptr or *4 if 32-bit ptr.
    dwarf::LocationAtom litOP = dwarf::DW_OP_lit3; // Assumed for varSizeInBits == 64

    if (varSizeInBits == 32) {
      litOP = dwarf::DW_OP_lit2;
    } else if (varSizeInBits == 16) {
      litOP = dwarf::DW_OP_lit1;
    } else if (varSizeInBits == 8) {
      litOP = dwarf::DW_OP_lit0;
    } else {
      IGC_ASSERT_MESSAGE((varSizeInBits == 64), "Unexpected spilled ptr or variable size");
    }

    addUInt(Block, dwarf::DW_FORM_data1, litOP);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_shl);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
  } else {
    // This case handles the case where a source variable is held in
    // GRF or a ptr to it is held in GRF.

    // CASE 2 and CASE 3: Expressions generated for 64-bit (or 32-bit) bit ptr
    // addresses in SIMD8 or SIMD16: 1 DW_OP_INTEL_push_simd_lane
    //   DW_OP_lit16 <--
    //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
    // 2 DW_OP_lit2 (CASE 3: lit3)
    // 3 DW_OP_shr
    // 4 DW_OP_plus_uconst(<n> +16)
    // 5 DW_OP_INTEL_regs
    // 6 DW_OP_INTEL_push_simd_lane
    // 7 DW_OP_lit3 (CASE 3: lit7)
    // 8 DW_OP_and
    // 9 DW_OP_const1u 64 (CASE 3: 32)
    // 10 DW_OP_mul
    // 11 DW_OP_const1u 64 (CASE 3: 32)
    // 12 DW_OP_INTEL_push_bit_piece_stack
    //
    // CASE 4: Example of expression generated for 64-bit or 32-bit or
    // 16-bit packed or 8-bit packed variable in SIMD8 or SIMD16:
    // 1 DW_OP_INTEL_push_simd_lane
    //   DW_OP_lit16 <--
    //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
    // 2 DW_OP_lit2 or lit3 or lit4 or lit5 respectively for 64/32/16/8 bit
    // variable 3 DW_OP_shr 4 DW_OP_plus_uconst(<n> +16) 5 DW_OP_INTEL_regs 6
    // DW_OP_INTEL_push_simd_lane 7 DW_OP_lit3 or lit7 or lit15 or lit31
    // respectively for 64/32/16/8 bit variable 8 DW_OP_and 9 DW_OP_const1u 64
    // or 32 or 16 or 8 10 DW_OP_mul 11 DW_OP_const1u 64 or 32 or 16 or 8 12
    // DW_OP_INTEL_bit_piece_stack
    //
    // CASE 5: Example of expression generated for 16-bit or 8-bit variable
    // unpacked in SIMD8 or SIMD16: 1 DW_OP_INTEL_push_simd_lane
    //   DW_OP_lit16 <--
    //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
    // 2 DW_OP_lit3
    // 3 DW_OP_shr
    // 4 DW_OP_plus_uconst(<n> +16)
    // 5 DW_OP_INTEL_regs
    // 6 DW_OP_INTEL_push_simd_lane
    // 7 DW_OP_lit7
    // 8 DW_OP_and
    // 9 DW_OP_const1u 32
    // 10 DW_OP_mul
    // 11 DW_OP_const1u 16 or 8
    // 12 DW_OP_INTEL_piece_stack

    // If unpacked then small variable takes up 32 bits else when packed fits
    // its exact size
    uint32_t bitsUsedByVar = (isPacked || varSizeInBits > 32) ? (uint32_t)varSizeInBits : 32;
    uint32_t variablesInSingleGRF = (VISAMod->getGRFSizeInBits()) / bitsUsedByVar;
    uint32_t valForSubRegLit = variablesInSingleGRF > 0 ? (uint32_t)std::log2(variablesInSingleGRF) : 0;

    // TODO: missing case lr->getGRF().subRegNum > 0
    // unsigned int subReg = lr->getGRF().subRegNum;
    // auto offsetInBits = subReg * 8;
    uint32_t regNumOffset = (variablesInSingleGRF == 0 || simdWidthOffset < variablesInSingleGRF)
                                ? 0
                                : (simdWidthOffset / variablesInSingleGRF);
    uint32_t regNum = lr->getGRF().regNum + regNumOffset;
    auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(regNum);

    EmitPushSimdLane(Block, isSecondHalf);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit0 + valForSubRegLit);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_shr);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
    addUInt(Block, dwarf::DW_FORM_udata,
            DWRegEncoded); // Register ID is shifted by offset

    EmitPushSimdLane(Block, false);

    addConstantUValue(Block, variablesInSingleGRF - 1);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_and);
    addConstantUValue(Block, bitsUsedByVar);
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_mul);
    extractSubRegValue(Block, varSizeInBits);
  }
}

// emitBitPiecesForRegVal - emit bitPieces DW_OP_bit_piece sequence for
// register value. It is used to describe vector variables in registers.
void CompileUnit::emitBitPiecesForRegVal(IGC::DIEBlock *Block, const PieceBuilder &pieceBuilder) {
  for (unsigned i = 0, e = pieceBuilder.pieceCount(); i < e; ++i) {
    auto Piece = pieceBuilder.get(i);
    addRegisterOp(Block, Piece.regNum);
    addBitPiece(Block, Piece.sizeBits, Piece.offsetBits);
  }
}

// addSimdLaneScalar - add a sequence of attributes to calculate location of
// scalar variable e.g. a GRF subregister.
void CompileUnit::addSimdLaneScalar(IGC::DIEBlock *Block, const DbgVariable &DV, const VISAVariableLocation &Loc,
                                    const DbgDecoder::LiveIntervalsVISA &lr) {
  IGC_ASSERT_MESSAGE(!lr.isSpill(), "Scalar spilled in scratch space");
  auto varSizeInBits = DV.getRegisterValueSizeInBits(DD);
  unsigned regNum = lr.getGRF().regNum;
  unsigned int subReg = lr.getGRF().subRegNum;
  auto offsetInBits = subReg * 8;
  IGC_ASSERT(offsetInBits / 8 == subReg);

  if (DD->getEmitterSettings().EnableDebugInfoValidation)
    DD->getStreamEmitter().verifyRegisterLocationExpr(DV, *DD);

  auto registerSizeInBits = Loc.GetVISAModule()->getGRFSizeInBits();
  const auto numGRFs = Loc.GetVISAModule()->getNumGRFs();

  // Direct vector value in registers. We want to emit pieces.
  if (DV.currentLocationIsVector()) {
    PieceBuilder pieceBuilder(regNum, numGRFs, registerSizeInBits, varSizeInBits, offsetInBits);
    LLVM_DEBUG(dbgs() << "  emitBitPiecesForRegVal("
                      << "varSizeInBits: " << varSizeInBits << ", offsetInBits: " << offsetInBits << ")\n");
    emitBitPiecesForRegVal(Block, pieceBuilder);
    return;
  }

  // Subregister based location. We want to extract value from register and push
  // on stack.
  addRegOrConst(Block, regNum);
  addConstantUValue(Block, offsetInBits);
  IGC_ASSERT_MESSAGE(varSizeInBits <= 64, "Entries pushed onto DWARF stack are limited to 8 bytes");
  extractSubRegValue(Block, varSizeInBits);
}

// addSimdLaneRegionBase - add a sequence of attributes to calculate location of
// region base address variable for vc-backend
void CompileUnit::addSimdLaneRegionBase(IGC::DIEBlock *Block, const DbgVariable &DV, const VISAVariableLocation &Loc,
                                        const DbgDecoder::LiveIntervalsVISA *lr) {
  auto OffsetsCount = Loc.GetRegionOffsetsCount();
  IGC_ASSERT(OffsetsCount);
  // Calculate size of register piece
  auto PieceSizeInBits = DV.getRegisterValueSizeInBits(DD) / OffsetsCount;
  if (DD->getEmitterSettings().EnableDebugInfoValidation)
    DD->getStreamEmitter().verifyRegisterLocationExpr(DV, *DD);
  LLVM_DEBUG(dbgs() << "  addSimdLaneRegionBase(PieceSizeInBits: " << PieceSizeInBits << ")\n");
  // TODO Support special logic for "sequential" regions
  for (size_t i = 0; i < OffsetsCount; ++i) {
    auto Offset = Loc.GetRegionOffset(i);
    auto Reg = lr->getGRF().regNum;
    // If offset overlapping register - increase register index
    Reg += Offset / Loc.GetVISAModule()->getGRFSizeInBits();
    if (Offset >= Loc.GetVISAModule()->getGRFSizeInBits()) {
      Offset = Offset % Loc.GetVISAModule()->getGRFSizeInBits();
    }
    IGC_ASSERT(Offset < Loc.GetVISAModule()->getGRFSizeInBits());
    // Generate piece for each element of address:
    // DW_OP_reg N; DW_OP_bit_piece: size: Size offset: Offset ;
    addRegisterOp(Block, Reg);
    addBitPiece(Block, PieceSizeInBits, Offset);
  }
}

/// getParentContextString - Walks the metadata parent chain in a language
/// specific manner (using the compile unit language) and returns
/// it as a string. This is done at the metadata level because DIEs may
/// not currently have been added to the parent context and walking the
/// DIEs looking for names is more expensive than walking the metadata.
std::string CompileUnit::getParentContextString(DIScope *Context) const {
  if (!Context)
    return "";

  // FIXME: Decide whether to implement this for non-C++ languages.
  if (getLanguage() != dwarf::DW_LANG_C_plus_plus)
    return "";

  std::string CS;

  SmallVector<DIScope *, 1> Parents;
  while (!isa<DICompileUnit>(Context)) {
    Parents.push_back(Context);
    if (Context->getScope())
      Context = resolve(Context->getScope());
    else
      // Structure, etc types will have a NULL context if they're at the top
      // level.
      break;
  }

  // Reverse iterate over our list to go from the outermost construct to the
  // innermost.
  for (SmallVectorImpl<DIScope *>::reverse_iterator I = Parents.rbegin(), E = Parents.rend(); I != E; ++I) {
    DIScope *Ctx = *I;
    StringRef Name = Ctx->getName();
    if (!Name.empty()) {
      CS += Name;
      CS += "::";
    }
  }

  return CS;
}

// Decode line number, file name and location from a string, where a line no.,
// file name and directory are separated by '?' character:
// lineNumber?fileName?directory There is a workaround for DIModule creation in
// earlier LLVM versions, where a line and a file parameters are not supported
// in DIBuilder.
void CompileUnit::decodeLineAndFileForISysRoot(StringRef &lineAndFile, unsigned int *line, std::string *file,
                                               std::string *directory) {
#if LLVM_VERSION_MAJOR < 11
  SmallVector<StringRef, 8> splitStr;
  lineAndFile.split(splitStr,
                    "?"); //   substr(0, posOfLineAndDirSeparator).str().copy()
  unsigned int posOfFirstQ = lineAndFile.find_first_of('?', 0);
  std::string lineStr = "";
  lineStr.append(splitStr[0].str().c_str(), posOfFirstQ);
  unsigned int posOfSecondQ = lineAndFile.find_first_of('?', posOfFirstQ + 1);

  directory->append(splitStr[1].str().c_str(), posOfSecondQ - posOfFirstQ);
  file->append(splitStr[2].str().c_str(), splitStr[2].size());
  *line = (unsigned int)std::atoi(splitStr[0].data());
#endif // LLVM_VERSION_MAJOR < 11
}

/// constructTypeDIE - Construct basic type die from DIBasicType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DIBasicType *BTy) {
  // Get core information.
  StringRef Name = BTy->getName();
  // Add name if not anonymous or intermediate type.
  if (!Name.empty()) {
    addString(&Buffer, dwarf::DW_AT_name, Name);
  }

  // An unspecified type only has a name attribute.
  if (BTy->getTag() == dwarf::DW_TAG_unspecified_type)
    return;

  addUInt(&Buffer, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, BTy->getEncoding());

  uint64_t Size = BTy->getSizeInBits() >> 3;
  addUInt(&Buffer, dwarf::DW_AT_byte_size, std::nullopt, Size);
}

#if LLVM_VERSION_MAJOR >= 12
/// constructTypeDIE - Construct basic type die from DIStringType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DIStringType *STy) {
  // Get core information.
  StringRef Name = STy->getName();
  // Add name if not anonymous or intermediate type.
  if (!Name.empty())
    addString(&Buffer, dwarf::DW_AT_name, Name);

  if (DIVariable *Var = STy->getStringLength()) {
    if (auto *VarDIE = getDIE(Var))
      addDIEEntry(&Buffer, dwarf::DW_AT_string_length, VarDIE);
  } else if (DIExpression *Expr = STy->getStringLengthExp()) {
    DIEBlock *Loc = new (DIEValueAllocator) DIEBlock;
    DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
    DwarfExpr.addExpression(Expr);
    addBlock(&Buffer, dwarf::DW_AT_string_length, DwarfExpr.finalize());
  } else {
    uint64_t Size = STy->getSizeInBits() >> 3;
    addUInt(&Buffer, dwarf::DW_AT_byte_size, std::nullopt, Size);
  }

  if (DIExpression *Expr = STy->getStringLocationExp()) {
    DIEBlock *Loc = new (DIEValueAllocator) DIEBlock;
    DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
    DwarfExpr.addExpression(Expr);
    addBlock(&Buffer, dwarf::DW_AT_data_location, DwarfExpr.finalize());
  }

  if (STy->getEncoding()) {
    // For eventual Unicode support.
    addUInt(&Buffer, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, STy->getEncoding());
  }
}
#endif

/// constructTypeDIE - Construct derived type die from DIDerivedType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DIDerivedType *DTy) {
  // Get core information.
  StringRef Name = DTy->getName();
  uint64_t Size = DTy->getSizeInBits() >> 3;
  uint16_t Tag = Buffer.getTag();

  // Map to main type, void will not have a type.
  DIType *FromTy = resolve(DTy->getBaseType());
  if (FromTy)
    addType(&Buffer, FromTy);

  // Add name if not anonymous or intermediate type.
  if (!Name.empty())
    addString(&Buffer, dwarf::DW_AT_name, Name);

  // Add size if non-zero (derived types might be zero-sized.)
  if (Size && Tag != dwarf::DW_TAG_pointer_type)
    addUInt(&Buffer, dwarf::DW_AT_byte_size, std::nullopt, Size);

  if (Tag == dwarf::DW_TAG_ptr_to_member_type)
    addDIEEntry(&Buffer, dwarf::DW_AT_containing_type, getOrCreateTypeDIE(resolve(DTy->getClassType())));
  // Add source line info if available and TyDesc is not a forward declaration.
  if (!DTy->isForwardDecl())
    addSourceLine(&Buffer, DTy);

  auto dwarfAddressSpaceOptional = IGCLLVM::makeOptional(DTy->getDWARFAddressSpace());

  if (dwarfAddressSpaceOptional && isSLMAddressSpaceTag(*dwarfAddressSpaceOptional)) {
    addUInt(&Buffer, dwarf::DW_AT_address_class, std::nullopt, 1);
  }
}

/// Return true if the type is appropriately scoped to be contained inside
/// its own type unit.
static bool isTypeUnitScoped(DIType *Ty, const DwarfDebug *DD) {
  DIScope *Parent = DD->resolve(Ty->getScope());
  while (Parent) {
    // Don't generate a hash for anything scoped inside a function.
    if (isa<DISubprogram>(Parent))
      return false;

    Parent = DD->resolve(Parent->getScope());
  }
  return true;
}

/// Return true if the type should be split out into a type unit.
static bool shouldCreateTypeUnit(DICompositeType *CTy, const DwarfDebug *DD) {
  uint16_t Tag = (uint16_t)CTy->getTag();

  switch (Tag) {
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_union_type:
  case dwarf::DW_TAG_enumeration_type:
  case dwarf::DW_TAG_class_type:
    // If this is a class, structure, union, or enumeration type
    // that is a definition (not a declaration), and not scoped
    // inside a function then separate this out as a type unit.
    return !CTy->isForwardDecl() && isTypeUnitScoped(CTy, DD);
  default:
    return false;
  }
}

void CompileUnit::constructTypeDIE(DIE &Buffer, DISubroutineType *STy) {
  DITypeRefArray Elements = cast<DISubroutineType>(STy)->getTypeArray();
  DIType *RTy = resolve(Elements[0]);
  if (RTy)
    addType(&Buffer, RTy);

  bool isPrototyped = true;

  if (Elements.size() == 2 && !Elements[1])
    isPrototyped = false;

  // Add arguments.
  for (unsigned i = 1, N = Elements.size(); i < N; ++i) {
    DIType *Ty = resolve(Elements[i]);
    if (!Ty) {
      createAndAddDIE(dwarf::DW_TAG_unspecified_parameters, Buffer);
      isPrototyped = false;
    } else {
      DIE *Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, Buffer);
      addType(Arg, Ty);
      if (Ty->isArtificial())
        addFlag(Arg, dwarf::DW_AT_artificial);
    }
  }
  // Add prototype flag if we're dealing with a C language and the
  // function has been prototyped.
  uint16_t Language = getLanguage();
  if (isPrototyped &&
      (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 || Language == dwarf::DW_LANG_ObjC))
    addFlag(&Buffer, dwarf::DW_AT_prototyped);
}

/// constructTypeDIE - Construct type DIE from DICompositeType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DICompositeType *CTy) {
  // Get core information.
  StringRef Name = CTy->getName();

  uint64_t Size = CTy->getSizeInBits() >> 3;
  uint16_t Tag = Buffer.getTag();

  switch (Tag) {
  case dwarf::DW_TAG_array_type:
    constructArrayTypeDIE(Buffer, CTy);
    break;
  case dwarf::DW_TAG_enumeration_type:
    constructEnumTypeDIE(Buffer, CTy);
    break;
  case dwarf::DW_TAG_subroutine_type: {
    // Add return type. A void return won't have a type.
    DITypeRefArray Elements = cast<DISubroutineType>(CTy)->getTypeArray();
    DIType *RTy = resolve(Elements[0]);
    if (RTy)
      addType(&Buffer, RTy);

    bool isPrototyped = true;

    if (Elements.size() == 2 && !Elements[1])
      isPrototyped = false;

    // Add arguments.
    for (unsigned i = 1, N = Elements.size(); i < N; ++i) {
      DIType *Ty = resolve(Elements[i]);
      if (!Ty) {
        createAndAddDIE(dwarf::DW_TAG_unspecified_parameters, Buffer);
        isPrototyped = false;
      } else {
        DIE *Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, Buffer);
        addType(Arg, Ty);
        if (Ty->isArtificial())
          addFlag(Arg, dwarf::DW_AT_artificial);
      }
    }
    // Add prototype flag if we're dealing with a C language and the
    // function has been prototyped.
    uint16_t Language = getLanguage();
    if (isPrototyped &&
        (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 || Language == dwarf::DW_LANG_ObjC))
      addFlag(&Buffer, dwarf::DW_AT_prototyped);
  } break;
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_union_type:
  case dwarf::DW_TAG_class_type: {
    // Add elements to structure type.
    DINodeArray Elements = CTy->getElements();
    for (unsigned i = 0, N = Elements.size(); i < N; ++i) {
      DINode *Element = Elements[i];
      DIE *ElemDie = NULL;
      if (isa<DISubprogram>(Element)) {
        DISubprogram *SP = cast<DISubprogram>(Element);
        ElemDie = getOrCreateSubprogramDIE(SP);

        dwarf::AccessAttribute dw_access = (SP->isProtected()) ? dwarf::DW_ACCESS_protected
                                           : (SP->isPrivate()) ? dwarf::DW_ACCESS_private
                                                               : dwarf::DW_ACCESS_public;
        addUInt(ElemDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

        if (SP->isExplicit()) {
          addFlag(ElemDie, dwarf::DW_AT_explicit);
        }
      } else if (isa<DIDerivedType>(Element)) {
        DIDerivedType *DDTy = cast<DIDerivedType>(Element);
        if (DDTy->getTag() == dwarf::DW_TAG_friend) {
          ElemDie = createAndAddDIE(dwarf::DW_TAG_friend, Buffer);
          addType(ElemDie, resolve(DDTy->getBaseType()), dwarf::DW_AT_friend);
        } else if (DDTy->isStaticMember()) {
          getOrCreateStaticMemberDIE(DDTy);
        } else {
          constructMemberDIE(Buffer, DDTy);
        }
      } else
        continue;
    }

    DIType *ContainingType = resolve(CTy->getBaseType());
    if (ContainingType && isa<DICompositeType>(ContainingType))
      addDIEEntry(&Buffer, dwarf::DW_AT_containing_type, getOrCreateTypeDIE(ContainingType));

    // Add template parameters to a class, structure or union types.
    // FIXME: The support isn't in the metadata for this yet.
    if (Tag == dwarf::DW_TAG_class_type || Tag == dwarf::DW_TAG_structure_type || Tag == dwarf::DW_TAG_union_type) {
      addTemplateParams(Buffer, CTy->getTemplateParams());
    }

    break;
  }
  default:
    break;
  }

  // Add name if not anonymous or intermediate type.
  if (!Name.empty()) {
    // llvm::demangle returns passed string if name is not mangled.
    std::string DemangledName = llvm::demangle(Name.str());
    addString(&Buffer, dwarf::DW_AT_name, DemangledName);
  }

  if (Tag == dwarf::DW_TAG_enumeration_type || Tag == dwarf::DW_TAG_class_type || Tag == dwarf::DW_TAG_structure_type ||
      Tag == dwarf::DW_TAG_union_type) {
    // Add size if non-zero (derived types might be zero-sized.)
    // TODO: Do we care about size for enum forward declarations?
    if (Size) {
      addUInt(&Buffer, dwarf::DW_AT_byte_size, std::nullopt, Size);
    } else if (!CTy->isForwardDecl()) {
      // Add zero size if it is not a forward declaration.
      addUInt(&Buffer, dwarf::DW_AT_byte_size, std::nullopt, 0);
    }

    // If we're a forward decl, say so.
    if (CTy->isForwardDecl()) {
      addFlag(&Buffer, dwarf::DW_AT_declaration);
    }

    // Add source line info if available.
    if (!CTy->isForwardDecl()) {
      addSourceLine(&Buffer, CTy);
    }
  }
  // If this is a type applicable to a type unit it then add it to the
  // list of types we'll compute a hash for later.
  if (shouldCreateTypeUnit(CTy, DD)) {
    DD->addTypeUnitType(&Buffer);
  }

  // Add flags if available
  if (CTy->isTypePassByValue())
    addUInt(&Buffer, dwarf::DW_AT_calling_convention, dwarf::DW_FORM_data1, dwarf::DW_CC_pass_by_value);
  if (CTy->isTypePassByReference())
    addUInt(&Buffer, dwarf::DW_AT_calling_convention, dwarf::DW_FORM_data1, dwarf::DW_CC_pass_by_reference);
}

/// constructTemplateTypeParameterDIE - Construct new DIE for the given
/// DITemplateTypeParameter.
void CompileUnit::constructTemplateTypeParameterDIE(DIE &Buffer, DITemplateTypeParameter *TP) {
  DIE *ParamDIE = createAndAddDIE(dwarf::DW_TAG_template_type_parameter, Buffer);
  // Add the type if it exists, it could be void and therefore no type.
  if (TP->getType()) {
    addType(ParamDIE, resolve(TP->getType()));
  }
  if (!TP->getName().empty()) {
    addString(ParamDIE, dwarf::DW_AT_name, TP->getName());
  }
}

/// constructTemplateValueParameterDIE - Construct new DIE for the given
/// DITemplateValueParameter.
void CompileUnit::constructTemplateValueParameterDIE(DIE &Buffer, DITemplateValueParameter *VP) {
  DIE *ParamDIE = createAndAddDIE(VP->getTag(), Buffer);

  // Add the type if there is one, template template and template parameter
  // packs will not have a type.
  if (VP->getTag() == dwarf::DW_TAG_template_value_parameter) {
    addType(ParamDIE, resolve(VP->getType()));
  }
  if (!VP->getName().empty()) {
    addString(ParamDIE, dwarf::DW_AT_name, VP->getName());
  }

  if (Metadata *Val = VP->getValue()) {
    if (ConstantInt *CI = mdconst::dyn_extract<ConstantInt>(Val)) {
      addConstantValue(ParamDIE, CI, isUnsignedDIType(DD, resolve(VP->getType())));
    } else if (GlobalValue *GV = mdconst::dyn_extract<GlobalValue>(Val)) {
      // For declaration non-type template parameters (such as global values and
      // functions)
      IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();
      addOpAddress(Block, Asm->GetSymbol(GV));
      // Emit DW_OP_stack_value to use the address as the immediate value of the
      // parameter, rather than a pointer to it.
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);
      addBlock(ParamDIE, dwarf::DW_AT_location, Block);
    } else if (VP->getTag() == dwarf::DW_TAG_GNU_template_template_param) {
      IGC_ASSERT(isa<MDString>(Val));
      addString(ParamDIE, dwarf::DW_AT_GNU_template_name, cast<MDString>(Val)->getString());
    } else if (VP->getTag() == dwarf::DW_TAG_GNU_template_parameter_pack) {
      IGC_ASSERT(isa<MDNode>(Val));
      // DIArray A(cast<MDNode>(Val));
      // addTemplateParams(*ParamDIE, A);
    }
  }
}

/// constructImportedEntityDIE - Create a DIE for DIImportedEntity.
IGC::DIE *CompileUnit::constructImportedEntityDIE(DIImportedEntity *Module) {
  DINode *Entity = Module->getEntity();
  if (auto *GV = dyn_cast<DIGlobalVariable>(Entity))
    return nullptr; // Missing support for imported entity linked to a global
                    // variable

  DIE *IMDie = new DIE(Module->getTag());
  insertDIE(Module, IMDie);

  DIE *EntityDie;
  if (auto *NS = dyn_cast<DINamespace>(Entity))
    EntityDie = getOrCreateNameSpace(NS);
  else if (auto *M = dyn_cast<DIModule>(Entity))
    EntityDie = getOrCreateModuleDIE(M);
  else if (auto *SP = dyn_cast<DISubprogram>(Entity))
    EntityDie = getOrCreateSubprogramDIE(SP);
  else if (auto *T = dyn_cast<DIType>(Entity))
    EntityDie = getOrCreateTypeDIE(T);
  // else if (auto* GV = dyn_cast<DIGlobalVariable>(Entity))  // TODO missing
  // support
  //    EntityDie = getOrCreateGlobalVariableDIE(GV, {});
  else
    EntityDie = getDIE(Entity);

  assert(EntityDie);

  addSourceLine(IMDie, Module, Module->getLine());
  addDIEEntry(IMDie, dwarf::DW_AT_import, EntityDie);
  StringRef Name = Module->getName();
  if (!Name.empty())
    addString(IMDie, dwarf::DW_AT_name, Name);

  return IMDie;
}

/// getOrCreateNameSpace - Create a DIE for DINameSpace.
IGC::DIE *CompileUnit::getOrCreateNameSpace(DINamespace *NS) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *ContextDIE = getOrCreateContextDIE(NS->getScope());

  DIE *NDie = getDIE(NS);
  if (NDie)
    return NDie;

  NDie = createAndAddDIE(dwarf::DW_TAG_namespace, *ContextDIE, NS);

  if (!NS->getName().empty()) {
    addString(NDie, dwarf::DW_AT_name, NS->getName());
  }
  return NDie;
}

/// getOrCreateSubprogramDIE - Create new DIE using SP.
IGC::DIE *CompileUnit::getOrCreateSubprogramDIE(DISubprogram *SP) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE (as is the case for member function
  // declarations).
  DIE *ContextDIE = getOrCreateContextDIE(resolve(SP->getScope()));

  DIE *SPDie = getDIE(SP);
  if (SPDie)
    return SPDie;

  DISubprogram *SPDecl = SP->getDeclaration();
  if (SPDecl && isa<DISubprogram>(SPDecl)) {
    // Add subprogram definitions to the CU die directly.
    ContextDIE = CUDie;
  }

  // DW_TAG_inlined_subroutine may refer to this DIE.
  SPDie = createAndAddDIE(dwarf::DW_TAG_subprogram, *ContextDIE, SP);

  DIE *DeclDie = NULL;
  if (SPDecl && isa<DISubprogram>(SPDecl)) {
    DeclDie = getOrCreateSubprogramDIE(SPDecl);
  }

  // Add function template parameters.
  addTemplateParams(*SPDie, SP->getTemplateParams());

  // Add the linkage name if we have one.
  StringRef LinkageName = SP->getLinkageName();
  if (!LinkageName.empty()) {
    if (EmitSettings.EmitATLinkageName) {
      addString(SPDie, dwarf::DW_AT_linkage_name, llvm::GlobalValue::dropLLVMManglingEscape(LinkageName));
    }
  }

  // If this DIE is going to refer declaration info using AT_specification
  // then there is no need to add other attributes.
  if (DeclDie) {
    // Refer function declaration directly.
    addDIEEntry(SPDie, dwarf::DW_AT_specification, DeclDie);

    return SPDie;
  }

  // Constructors and operators for anonymous aggregates do not have names.
  if (!SP->getName().empty()) {
    // llvm::demangle returns passed string if name is not mangled.
    std::string DemangledName = llvm::demangle(SP->getName().str());
    addString(SPDie, dwarf::DW_AT_name, DemangledName);
  }

  addSourceLine(SPDie, SP);

  addSimdWidth(SPDie, DD->simdWidth);

  // Add the prototype if we have a prototype and we have a C like
  // language.
  uint16_t Language = getLanguage();
  if (SP->isPrototyped() &&
      (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 || Language == dwarf::DW_LANG_ObjC)) {
    addFlag(SPDie, dwarf::DW_AT_prototyped);
  }

  // Following sourced from DwarfUnit.cpp from llvm3.6.2 src
  DISubroutineType *SPTy = SP->getType();

  if (!SPTy) {
    // KW fix, this branch should never be taken
    return SPDie;
  }

  IGC_ASSERT_MESSAGE(SPTy->getTag() == dwarf::DW_TAG_subroutine_type,
                     "the type of a subprogram should be a subroutine");

  DITypeRefArray Args = SPTy->getTypeArray();
  // Add a return type. If this is a type like a C/C++ void type we don't add a
  // return type.
  if (Args.size() > 0 && resolve(Args[0]))
    addType(SPDie, resolve(Args[0]));

  unsigned VK = SP->getVirtuality();
  if (VK) {
    addUInt(SPDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1, VK);
    IGC::DIEBlock *Block = getDIEBlock();
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
    addUInt(Block, dwarf::DW_FORM_udata, SP->getVirtualIndex());
    addBlock(SPDie, dwarf::DW_AT_vtable_elem_location, Block);
    ContainingTypeMap.insert(std::make_pair(SPDie, resolve(SP->getContainingType())));
  }

  if (!SP->isDefinition()) {
    addFlag(SPDie, dwarf::DW_AT_declaration);

    // Add arguments. Do not add arguments for subprogram definition. They will
    // be handled while processing variables.
    for (unsigned i = 1, N = Args.size(); i < N; ++i) {
      DIE *Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, *SPDie);
      DIType *ATy = resolve(Args[i]);
      if (!ATy)
        continue;
      addType(Arg, ATy);
      if (ATy->isArtificial()) {
        addFlag(Arg, dwarf::DW_AT_artificial);
      }
      if (ATy->isObjectPointer()) {
        addDIEEntry(SPDie, dwarf::DW_AT_object_pointer, Arg);
      }
    }
  }

  if (SP->isArtificial()) {
    addFlag(SPDie, dwarf::DW_AT_artificial);
  }

  if (!SP->isLocalToUnit() && ExtFunc.count(SP) > 0) {
    addFlag(SPDie, dwarf::DW_AT_external);
  }

  return SPDie;
}

/// getOrCreateModuleDIE - Create new DIE for DIModule.
IGC::DIE *CompileUnit::getOrCreateModuleDIE(DIModule *MD) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE (as is the case for member function
  // declarations).
  DIE *ContextDIE = getOrCreateContextDIE(MD->getScope());
  DIE *MDDie = getDIE(MD);
  if (MDDie)
    return MDDie;

  MDDie = createAndAddDIE(dwarf::DW_TAG_module, *ContextDIE, MD);
  IGC_ASSERT(MDDie);

  if (!MD->getName().empty()) {
    addString(MDDie, dwarf::DW_AT_name, MD->getName());
  }

#if LLVM_VERSION_MAJOR < 11
#if LLVM_VERSION_MAJOR < 10
  StringRef iSysRoot = MD->getISysRoot();
#else  // LLVM_VERSION_MAJOR == 10
  StringRef iSysRoot = MD->getSysRoot();
#endif // LLVM_VERSION_MAJOR == 10
  unsigned int line;
  std::string file = "";
  std::string directory = "";

  decodeLineAndFileForISysRoot(iSysRoot, &line, &file, &directory);

  // Emit a line number and a file only if line number is significant
  if (line > 0)
    addUInt(MDDie, dwarf::DW_AT_decl_line, std::nullopt, line);
  // Emit a file if not empty name
  if (!file.empty() || !directory.empty()) {
    StringRef fileRef(file);
    StringRef dirRef(directory);
    unsigned FileID = DD->getOrCreateSourceID(fileRef, dirRef, getUniqueID());
    addUInt(MDDie, dwarf::DW_AT_decl_file, std::nullopt, FileID);
  }
#else // LLVM_VERSION_MAJOR >= 11
#if LLVM_VERSION_MAJOR == 11
  addSourceLine(MDDie, MD, MD->getLineNo());
#elif LLVM_VERSION_MAJOR >= 12
  if (!MD->getIsDecl()) {
    addSourceLine(MDDie, MD, MD->getLineNo());
  } else {
    addFlag(MDDie, dwarf::DW_AT_declaration);
  }
#endif // LLVM_VERSION_MAJOR >= 12
#endif // LLVM_VERSION_MAJOR >= 11.

  return MDDie;
}

DIEDwarfExpression::DIEDwarfExpression(const StreamEmitter &AP, CompileUnit &CU, DIEBlock &DIE)
    : DwarfExpression(CU), AP(AP), OutDIE(DIE) {}

void DwarfExpression::addExpression(DIExpressionCursor &&ExprCursor) {
  addExpression(std::move(ExprCursor), [](unsigned Idx, DIExpressionCursor &Cursor) -> bool {
    llvm_unreachable("unhandled opcode found in expression");
  });
}

void DIEDwarfExpression::emitOp(uint8_t Op, const char *Comment) {
  CU.addUInt(&getActiveDIE(), dwarf::DW_FORM_data1, Op);
}

void DIEDwarfExpression::emitSigned(int64_t Value) { CU.addSInt(&getActiveDIE(), dwarf::DW_FORM_sdata, Value); }

void DIEDwarfExpression::emitUnsigned(uint64_t Value) { CU.addUInt(&getActiveDIE(), dwarf::DW_FORM_udata, Value); }

void DIEDwarfExpression::emitData1(uint8_t Value) { CU.addUInt(&getActiveDIE(), dwarf::DW_FORM_data1, Value); }

/// constructSubrangeDIE - Construct subrange DIE from DISubrange.
void CompileUnit::constructSubrangeDIE(DIE &Buffer, DISubrange *SR, DIE *IndexTy) {
  DIE *DW_Subrange = createAndAddDIE(dwarf::DW_TAG_subrange_type, Buffer);
  addDIEEntry(DW_Subrange, dwarf::DW_AT_type, IndexTy);

  // The LowerBound value defines the lower bounds which is typically zero for
  // C/C++. The Count value is the number of elements.  Values are 64 bit. If
  // Count == -1 then the array is unbounded and we do not emit
  // DW_AT_lower_bound and DW_AT_count attributes.
  int64_t DefaultLowerBound = getDefaultLowerBound();

#if LLVM_VERSION_MAJOR >= 13
  auto AddBoundTypeEntry = [&](dwarf::Attribute Attr, DISubrange::BoundType Bound) {
    if (auto *BV = Bound.dyn_cast<DIVariable *>()) {
      if (auto *VarDIE = getDIE(BV))
        addDIEEntry(DW_Subrange, Attr, VarDIE);
    } else if (auto *BE = Bound.dyn_cast<DIExpression *>()) {
      DIEBlock *Loc = new (DIEValueAllocator) DIEBlock;
      IGC::DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
      // DwarfExpr.setMemoryLocationKind();
      DwarfExpr.addExpression(BE);
      addBlock(DW_Subrange, Attr, DwarfExpr.finalize());
    } else if (auto *BI = Bound.dyn_cast<ConstantInt *>()) {
      if (Attr == dwarf::DW_AT_count) {
        if (BI->getSExtValue() != -1)
          addUInt(DW_Subrange, Attr, std::nullopt, BI->getSExtValue());
      } else if (Attr != dwarf::DW_AT_lower_bound || DefaultLowerBound == -1 || BI->getSExtValue() != DefaultLowerBound)
        addSInt(DW_Subrange, Attr, dwarf::DW_FORM_sdata, BI->getSExtValue());
    }
  };

  AddBoundTypeEntry(dwarf::DW_AT_lower_bound, SR->getLowerBound());

  AddBoundTypeEntry(dwarf::DW_AT_count, SR->getCount());

  AddBoundTypeEntry(dwarf::DW_AT_upper_bound, SR->getUpperBound());

  AddBoundTypeEntry(dwarf::DW_AT_byte_stride, SR->getStride());

#elif LLVM_VERSION_MAJOR <= 10
  int64_t LowerBound = SR->getLowerBound();
  auto *CI = SR->getCount().dyn_cast<ConstantInt *>();

  if (DefaultLowerBound == -1 || LowerBound != DefaultLowerBound) {
    addUInt(DW_Subrange, dwarf::DW_AT_lower_bound, std::nullopt, LowerBound);
  }

  if (CI) {
    int64_t Count = CI->getSExtValue();
    if (Count != -1 && Count != 0) {
      // FIXME: An unbounded array should reference the expression that defines
      // the array.
      addUInt(DW_Subrange, dwarf::DW_AT_upper_bound, std::nullopt, LowerBound + Count - 1);
    }
  }
#endif
}

/// constructArrayTypeDIE - Construct array type DIE from DICompositeType.
void CompileUnit::constructArrayTypeDIE(DIE &Buffer, DICompositeType *CTy) {
  if (CTy->isVector()) {
    addFlag(&Buffer, dwarf::DW_AT_GNU_vector);
  }
#if LLVM_VERSION_MAJOR >= 12
  // Add DW_AT_data_location attr to DWARF. Dynamic arrays are represented by
  // descriptor and allocated space. DW_AT_data_location is used to denote
  // allocated space.
  if (DIVariable *Var = CTy->getDataLocation()) {
    if (auto *VarDIE = getDIE(Var))
      addDIEEntry(&Buffer, dwarf::DW_AT_data_location, VarDIE);
  } else if (DIExpression *Expr = CTy->getDataLocationExp()) {
    DIEBlock *Loc = new (DIEValueAllocator) DIEBlock;
    IGC::DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
    // DwarfExpr.setMemoryLocationKind();
    DwarfExpr.addExpression(Expr);
    addBlock(&Buffer, dwarf::DW_AT_data_location, DwarfExpr.finalize());
  }

  // Add DW_AT_associated attr to DWARF. This is needed for the array variables
  // with pointer attr. It helps to identify the status of variable whether it
  // is currently associated
  if (DIVariable *Var = CTy->getAssociated()) {
    if (auto *VarDIE = getDIE(Var))
      addDIEEntry(&Buffer, dwarf::DW_AT_associated, VarDIE);
  } else if (DIExpression *Expr = CTy->getAssociatedExp()) {
    DIEBlock *Loc = new (DIEValueAllocator) DIEBlock;
    DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
    // DwarfExpr.setMemoryLocationKind();
    DwarfExpr.addExpression(Expr);
    addBlock(&Buffer, dwarf::DW_AT_associated, DwarfExpr.finalize());
  }
#endif
  // Emit the element type.
  addType(&Buffer, resolve(CTy->getBaseType()));

  // Get an anonymous type for index type.
  // FIXME: This type should be passed down from the front end
  // as different languages may have different sizes for indexes.
  DIE *IdxTy = getIndexTyDie();
  if (!IdxTy) {
    // Construct an anonymous type for index type.
    IdxTy = createAndAddDIE(dwarf::DW_TAG_base_type, *CUDie);
    addString(IdxTy, dwarf::DW_AT_name, "int");
    addUInt(IdxTy, dwarf::DW_AT_byte_size, std::nullopt, sizeof(int32_t));
    addUInt(IdxTy, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed);
    setIndexTyDie(IdxTy);
  }

  // Add subranges to array type.
  DINodeArray Elements = CTy->getElements();
  for (unsigned i = 0, N = Elements.size(); i < N; ++i) {
    auto Element = Elements[i];
    if (Element->getTag() == dwarf::DW_TAG_subrange_type) {
      constructSubrangeDIE(Buffer, cast<DISubrange>(Element), IdxTy);
    }
  }
}

/// constructEnumTypeDIE - Construct an enum type DIE from DICompositeType.
void CompileUnit::constructEnumTypeDIE(DIE &Buffer, DICompositeType *CTy) {
  DINodeArray Elements = CTy->getElements();

  // Add enumerators to enumeration type.
  for (unsigned i = 0, N = Elements.size(); i < N; ++i) {
    DIEnumerator *Enum = cast_or_null<DIEnumerator>(Elements[i]);
    if (Enum) {
      DIE *Enumerator = createAndAddDIE(dwarf::DW_TAG_enumerator, Buffer);
      StringRef Name = Enum->getName();
      addString(Enumerator, dwarf::DW_AT_name, Name);
      int64_t Value = Enum->getValue()
#if LLVM_VERSION_MAJOR >= 11
                          .getZExtValue()
#endif
          ;
      addSInt(Enumerator, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, Value);
    }
  }
  DIType *DTy = resolve(CTy->getBaseType());
  if (DTy) {
    addType(&Buffer, DTy);
    // Add DW_AT_enum_class when FlagEnumClass exists
    if (CTy->getFlags() & DINode::FlagEnumClass) {
      addFlag(&Buffer, dwarf::DW_AT_enum_class);
    }
  }
}

/// constructContainingTypeDIEs - Construct DIEs for types that contain
/// vtables.
void CompileUnit::constructContainingTypeDIEs() {
  for (DenseMap<DIE *, const MDNode *>::iterator CI = ContainingTypeMap.begin(), CE = ContainingTypeMap.end(); CI != CE;
       ++CI) {
    DIE *SPDie = CI->first;
    DINode *D = cast<DINode>(const_cast<MDNode *>(CI->second));
    if (!D)
      continue;
    DIE *NDie = getDIE(D);
    if (!NDie)
      continue;
    addDIEEntry(SPDie, dwarf::DW_AT_containing_type, NDie);
  }
}

IGC::DIE *CompileUnit::constructVariableDIE(DbgVariable &DV, bool isScopeAbstract) {
  StringRef Name = DV.getName();
  LLVM_DEBUG(dbgs() << "[DwarfDebug] constructing DIE for variable <" << Name << ">\n");
  // Define variable debug information entry.
  DIE *VariableDie = new DIE(DV.getTag());
  DbgVariable *AbsVar = DV.getAbstractVariable();
  DIE *AbsDIE = AbsVar ? AbsVar->getDIE() : NULL;
  if (AbsDIE) {
    addDIEEntry(VariableDie, dwarf::DW_AT_abstract_origin, AbsDIE);
  } else {
    if (!Name.empty()) {
      addString(VariableDie, dwarf::DW_AT_name, Name);
    }
    addSourceLine(VariableDie, const_cast<DILocalVariable *>(DV.getVariable()));
    addType(VariableDie, DV.getType());
  }

  if (DV.isArtificial()) {
    addFlag(VariableDie, dwarf::DW_AT_artificial);
  }

  if (isScopeAbstract) {
    DV.setDIE(VariableDie);
    LLVM_DEBUG(dbgs() << "  done. Variable is scope-abstract\n");
    return VariableDie;
  }

  // Add variable address.

  auto Offset = DV.getDotDebugLocOffset();
  if (Offset != DbgVariable::InvalidDotDebugLocOffset) {
    // Copy over references ranges to DotLocDebugEntries
    if (EmitSettings.EnableRelocation) {
      // Retrieve correct location value based on Offset.
      // Then attach label corresponding to this offset
      // to DW_AT_location attribute.
      auto LocLabel = DD->CopyDebugLoc(Offset);
      addLabelLoc(VariableDie, dwarf::DW_AT_location, LocLabel);
    } else {
      Offset = DD->CopyDebugLocNoReloc(Offset);
      addUInt(VariableDie, dwarf::DW_AT_location, dwarf::DW_FORM_sec_offset, Offset);
    }

    DV.setDIE(VariableDie);
    LLVM_DEBUG(dbgs() << "  done. Location is taken from .debug_loc\n");
    return VariableDie;
  }

  // Check if variable is described by a DBG_VALUE instruction.
  const Instruction *pDbgInst = DV.getDbgInst();
  if (!pDbgInst || !DV.currentLocationIsInlined()) {
    DV.setDIE(VariableDie);
    LLVM_DEBUG(dbgs() << " done. No dbg.inst assotiated\n");
    return VariableDie;
  }

  buildLocation(pDbgInst, DV, VariableDie);

  LLVM_DEBUG(dbgs() << " done. Location is emitted directly in DIE\n");
  return VariableDie;
}

void CompileUnit::buildLocation(const llvm::Instruction *pDbgInst, DbgVariable &DV, IGC::DIE *VariableDie) {
  auto *F = pDbgInst->getParent()->getParent();
  const auto *VISAModule = DD->GetVISAModule(F);
  auto Loc = VISAModule->GetVariableLocation(pDbgInst);
  LLVM_DEBUG(dbgs() << "  buildLocation at vISA location:\n"; Loc.print(dbgs()));

  // Variable can be immdeiate or in a location (but not both)
  if (Loc.IsImmediate()) {
    const Constant *pConstVal = Loc.GetImmediate();
    if (const ConstantInt *pConstInt = dyn_cast<ConstantInt>(pConstVal)) {
      addConstantValue(VariableDie, pConstInt, isUnsignedDIType(DD, DV.getType()));
    } else if (const ConstantFP *pConstFP = dyn_cast<ConstantFP>(pConstVal)) {
      addConstantFPValue(VariableDie, pConstFP);
    } else {
      DwarfDebug::DataVector rawData;
      DD->ExtractConstantData(pConstVal, rawData);
      addConstantData(VariableDie, rawData.data(), rawData.size());
    }
    LLVM_DEBUG(dbgs() << "  location is built as an imm\n");
    DV.setDIE(VariableDie);
    return;
  }

  IGC::DIEBlock *locationAT = nullptr;
  if (Loc.IsSLM())
    locationAT = buildSLM(DV, Loc, VariableDie);
  else if (Loc.IsSampler())
    locationAT = buildSampler(DV, Loc);
  else if (Loc.HasSurface() && (DV.getType() && DV.getType()->getTag() == dwarf::DW_TAG_pointer_type))
    locationAT = buildPointer(DV, Loc);
  else
    locationAT = buildGeneral(DV, Loc, nullptr, VariableDie);

  if (locationAT)
    addBlock(VariableDie, dwarf::DW_AT_location, locationAT);
}

IGC::DIEBlock *CompileUnit::buildPointer(const DbgVariable &var, const VISAVariableLocation &loc) {
  auto bti = loc.GetSurface() - VISAModule::TEXTURE_REGISTER_BEGIN;

  LLVM_DEBUG(dbgs() << "  buildingPointer, bti_idx = " << bti << "\n");

  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const4u);
  addUInt(Block, dwarf::DW_FORM_data4, 0);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
  addUInt(Block, dwarf::DW_FORM_data1, 32);
  addUInt(Block, dwarf::DW_FORM_data1, 0);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit0);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
  addUInt(Block, dwarf::DW_FORM_data1, 16);
  addUInt(Block, dwarf::DW_FORM_data1, 0);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
  addUInt(Block, dwarf::DW_FORM_data2, bti & 0xffff);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
  addUInt(Block, dwarf::DW_FORM_data1, 8);
  addUInt(Block, dwarf::DW_FORM_data1, 0);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit2);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
  addUInt(Block, dwarf::DW_FORM_data1, 8);
  addUInt(Block, dwarf::DW_FORM_data1, 0);

  return Block;
}

IGC::DIEBlock *CompileUnit::buildSampler(const DbgVariable &var, const VISAVariableLocation &loc) {
  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();

  if (loc.IsInGlobalAddrSpace()) {
    addBindlessSamplerLocation(Block, loc); // Emit SLM location expression
  } else {
    Address addr;
    addr.Set(Address::Space::eSampler, loc.GetSurface() - VISAModule::SAMPLER_REGISTER_BEGIN, loc.GetOffset());

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
    addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());
  }

  return Block;
}

IGC::DIEBlock *CompileUnit::buildSLM(const DbgVariable &var, const VISAVariableLocation &loc, IGC::DIE *VariableDie) {
  // Add SLM offset based location. Add DW_AT_address_class mark because for
  // some reason not adding dwarfAddressSpace on type.
  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();
  uint32_t offset = loc.GetOffset();
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
  addUInt(Block, dwarf::DW_FORM_addr, (uint64_t)offset);
  IGC_ASSERT_MESSAGE(VariableDie, "Expected variable DIE to emit address_class for SLM");
  addUInt(VariableDie, dwarf::DW_AT_address_class, std::nullopt, 1);
  return Block;
}

bool CompileUnit::buildPrivateBaseRegBased(const DbgVariable &var, IGC::DIEBlock *Block,
                                           const VISAVariableLocation &loc) {
  LLVM_DEBUG(dbgs() << " PrivateBase(%X) + (%PerThreadOffset + "
                       " (simdSize*<var_offset>) + (simdLaneId * <var_size>))");

  const auto *DbgInst = var.getDbgInst();
  const auto *storageMD = DbgInst->getMetadata("StorageOffset");
  IGC_ASSERT(storageMD != nullptr);
  const auto *VISAMod = loc.GetVISAModule();
  auto privateBaseRegNum = VISAMod->getPrivateBaseReg();
  int64_t offset = 0;

  // %Y = privateBase (%X)
  //                + (%perThreadOffset + (simdSize * <variable offset>)
  //                + (simdLaneId * <variable size>))
  // CASE with Private Base and Per Thread Offset in GRF registers (both not
  // spilled) 1 DW_OP_regx <Private Base reg encoded> 2 DW_OP_const1u/2u
  // <bit-offset to Private Base reg> 3 DW_OP_const1u 64  , i.e. size on bits 4
  // DW_OP_INTEL_push_bit_piece_stack 5 DW_OP_constu <Per Thread reg encoded> 6
  // DW_OP_INTEL_regs     , i.e. Per Thread Offset 7 DW_OP_const1u <bit-offset
  // to Per Thread Offset> 8 DW_OP_const1u 32  , i.e. size in bits 9
  // DW_OP_INTEL_push_bit_piece_stack 10 DW_OP_plus , i.e. add Private Base to
  // Per Thread Offset 11 DW_OP_plus_uconst offset , i.e. simdSize * <variable
  // offset> 12 DW_OP_INTEL_push_simd_lane 13 DW_OP_const1u/2u/4u/8u
  // <variableSize>  , i.e. size in bytes 14 DW_OP_mul 15 DW_OP_plus
  // 16 remaining opcodes from DIExpression
  auto simdSize = VISAMod->GetSIMDSize();

  // Rely on getVarInfo result here.
  const auto *VarInfoPrivBase = VISAMod->getVarInfo(DD->getVisaDebugInfo(), privateBaseRegNum);
  if (!VarInfoPrivBase) {
    LLVM_DEBUG(dbgs() << "warning: could not get PrivateBase LR (V" << privateBaseRegNum << ")");
    return false;
  }

  LLVM_DEBUG(dbgs() << "  PrivateBase: "; VarInfoPrivBase->print(dbgs()); dbgs() << "\n");

  IGC_ASSERT_MESSAGE(VarInfoPrivBase->lrs.empty() || VarInfoPrivBase->lrs.front().isGRF() ||
                         VarInfoPrivBase->lrs.front().isSpill(),
                     "Unexpected location of variable");

  if (VarInfoPrivBase->lrs.front().isGRF()) {
    uint16_t grfRegNumPrivBase = VarInfoPrivBase->lrs.front().getGRF().regNum;
    unsigned int grfSubRegNumPrivBase = VarInfoPrivBase->lrs.front().getGRF().subRegNum;
    auto bitOffsetToPrivBaseReg = grfSubRegNumPrivBase * 8; // Bit-offset to GRF with Private Base

    addRegOrConst(Block, // 1 DW_OP_regx <Private Base reg encoded>
                  grfRegNumPrivBase);
    //   Register ID is shifted by offset
    addConstantUValue(Block,
                      bitOffsetToPrivBaseReg); // 2 DW_OP_const1u/2u <bit-offset
                                               // to Private Base reg>
    extractSubRegValue(Block, 64);
  } else if (VarInfoPrivBase->lrs.front().isSpill()) {
    unsigned int memOffsetPrivBase = 0;
    memOffsetPrivBase = VarInfoPrivBase->lrs.front().getSpillOffset().memoryOffset;
    addScratchLocation(Block, memOffsetPrivBase, 0);
    addBE_FP(Block);
  }

  auto regNumPerThOff = VISAMod->getPTOReg();

  // Rely on getVarInfo result here.
  const auto *VarInfoPerThOff = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNumPerThOff);
  if (!VarInfoPerThOff) {
    LLVM_DEBUG(dbgs() << "warning: could not get PTO LR (V" << regNumPerThOff << ")");
    return false;
  }

  LLVM_DEBUG(dbgs() << "  PerThOffset: "; VarInfoPerThOff->print(dbgs()); dbgs() << "\n");
  IGC_ASSERT_MESSAGE(VarInfoPerThOff->lrs.empty() || VarInfoPerThOff->lrs.front().isGRF() ||
                         VarInfoPerThOff->lrs.front().isSpill(),
                     "Unexpected location of variable");

  if (VarInfoPerThOff->lrs.front().isGRF()) {
    uint16_t grfRegNumPTO = VarInfoPerThOff->lrs.front().getGRF().regNum;
    unsigned int grfSubRegPTO = VarInfoPerThOff->lrs.front().getGRF().subRegNum;
    auto bitOffsetToPTOReg = grfSubRegPTO * 8; // Bit-offset to GRF with Per Thread Offset

    auto DWRegPTOEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(grfRegNumPTO);
    addUInt(Block, dwarf::DW_FORM_data1,
            dwarf::DW_OP_constu); // 5 DW_OP_constu <Per Thread reg encoded>
    addUInt(Block, dwarf::DW_FORM_udata,
            DWRegPTOEncoded); // Register ID is shifted by offset

    addConstantUValue(Block,
                      bitOffsetToPTOReg); // 7 DW_OP_const1u/2u <bit-offset to
                                          // Per Thread Offset>
    extractSubRegValue(Block, 32);
  } else if (VarInfoPerThOff->lrs.front().isSpill()) {
    unsigned int memOffsetPTO = 0;
    memOffsetPTO = VarInfoPerThOff->lrs.front().getSpillOffset().memoryOffset;
    addScratchLocation(Block, memOffsetPTO, 0);
    addBE_FP(Block);
  }

  addUInt(Block, dwarf::DW_FORM_data1,
          dwarf::DW_OP_plus); // 10 DW_OP_plus   , i.e. add Private Base to Per
                              // Thread Offset

  // Variable's bit offset can be found in the first operand of StorageOffset
  // metadata node.
  offset = simdSize * getIntConstFromMdOperand(storageMD, 0).getSExtValue();

  addUInt(Block, dwarf::DW_FORM_data1,
          dwarf::DW_OP_plus_uconst);            // 11 DW_OP_plus_uconst offset , i.e.
                                                // simdSize * <variable offset>
  addUInt(Block, dwarf::DW_FORM_udata, offset); // Offset

  addUInt(Block, dwarf::DW_FORM_data1,
          DW_OP_INTEL_push_simd_lane); // 12 DW_OP_INTEL_push_simd_lane

  auto varSizeInBytes = var.getRegisterValueSizeInBits(DD) / 8;

  LLVM_DEBUG(dbgs() << "  var Offset: " << offset << ", var Size: " << varSizeInBytes << "\n");
  IGC_ASSERT_MESSAGE((var.getRegisterValueSizeInBits(DD) & 0x7) == 0, "Unexpected variable size");

  addConstantUValue(Block,
                    varSizeInBytes);                       // 13 DW_OP_const1u/2u/4u/8u <variableSize>
                                                           // , i.e. size in bytes
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_mul);  // 14 DW_OP_mul
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus); // 15 DW_OP_plus

  // 16 remaining opcodes from DIExpression
  const DIExpression *DIExpr = DbgInst->getExpression();
  for (auto I = DIExpr->expr_op_begin(), E = DIExpr->expr_op_end(); I != E; ++I) {
    auto op = I->getOp();
    auto BF = DIEInteger::BestForm(false, op);
    addUInt(Block, BF, op);
  }

  return true;
}

bool CompileUnit::buildFpBasedLoc(const DbgVariable &var, IGC::DIEBlock *Block, const VISAVariableLocation &loc) {
  const auto *storageMD = var.getDbgInst()->getMetadata("StorageOffset");
  IGC_ASSERT(nullptr != storageMD);
  const auto *VISAMod = loc.GetVISAModule();
  const auto *sizeMD = var.getDbgInst()->getMetadata("StorageSize");
  IGC_ASSERT(nullptr != sizeMD);

  LLVM_DEBUG(dbgs() << "  generating FP-based location\n");
  auto simdSize = VISAMod->GetSIMDSize();
  uint64_t storageOffset = simdSize * getIntConstFromMdOperand(storageMD, 0).getZExtValue();
  uint64_t storageSize = getIntConstFromMdOperand(sizeMD, 0).getZExtValue();
  LLVM_DEBUG(dbgs() << "  StorageOffset: " << storageOffset << ", StorageSize: " << storageSize << "\n");

  // There is a private value in the current stack frame
  // 1 DW_OP_regx <Frame Pointer reg encoded>
  // 2 DW_OP_const1u <bit-offset to Frame Pointer reg>
  // 3 DW_OP_const1u 64  , i.e. size in bits
  // 4 DW_OP_INTEL_push_bit_piece_stack
  // 5 DW_OP_plus_uconst  SIZE_OWORD         // i.e. 0x10 taken from
  // getFPOffset(); same as emitted in EmitPass::emitStackAlloca() 6
  // DW_OP_push_simd_lane 7 DW_OP_const1u/2u/4u/8u  storageSize   // MD:
  // StorageSize; the size of the variable 8 DW_OP_mul 9 DW_OP_plus 10
  // DW_OP_plus_uconst storageOffset      // MD: StorageOffset; the offset where
  // each variable is stored in the current stack frame

  auto regNumFP = VISAMod->getFPReg();

  // Rely on getVarInfo result here.
  const auto *VarInfoFP = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNumFP);
  if (!VarInfoFP) {
    LLVM_DEBUG(dbgs() << "warning: no gen loc info for FP (V" << regNumFP << ")");
    return false;
  }
  LLVM_DEBUG(dbgs() << "  FramePointer: "; VarInfoFP->print(dbgs()); dbgs() << "\n");

  uint16_t grfRegNumFP = VarInfoFP->lrs.front().getGRF().regNum;
  uint16_t grfSubRegNumFP = VarInfoFP->lrs.front().getGRF().subRegNum;
  auto bitOffsetToFPReg = grfSubRegNumFP * 8; // Bit-offset to GRF with Frame Pointer

  addRegOrConst(Block, grfRegNumFP); // 1 DW_OP_regx <Frame Pointer reg encoded>

  //   Register ID is shifted by offset
  addConstantUValue(Block,
                    bitOffsetToFPReg); // 2 DW_OP_const1u/2u <bit-offset to Frame Pointer reg>
  extractSubRegValue(Block, 64);

  addUInt(Block, dwarf::DW_FORM_data1,
          dwarf::DW_OP_plus_uconst); // 5 DW_OP_plus_uconst  SIZE_OWORD (taken
                                     // from getFPOffset())
  addUInt(Block, dwarf::DW_FORM_udata, VISAMod->getFPOffset());

  addUInt(Block, dwarf::DW_FORM_data1,
          DW_OP_INTEL_push_simd_lane);                     // 6 DW_OP_INTEL_push_simd_lane
  addConstantUValue(Block, storageSize);                   // 7 DW_OP_const1u/2u/4u/8u storageSize
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_mul);  // 8 DW_OP_mul
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus); // 9 DW_OP_plus

  addUInt(Block, dwarf::DW_FORM_data1,
          dwarf::DW_OP_plus_uconst);                   // 10 DW_OP_plus_uconst storageOffset
  addUInt(Block, dwarf::DW_FORM_udata, storageOffset); // storageOffset

  var.emitExpression(this, Block);

  return true;
}

bool CompileUnit::buildSlicedLoc(DbgVariable &var, IGC::DIEBlock *Block, const VISAVariableLocation &loc,
                                 const std::vector<DbgDecoder::LiveIntervalsVISA> *vars) {
  LLVM_DEBUG(dbgs() << "  sliced variable, pushing lane \n");
  // DW_OP_push_simd_lane
  // DW_OP_lit16
  // DW_OP_ge
  // DW_OP_bra secondHalf
  // -- emit first half
  // DW_OP_skip end
  // secondHalf:
  // -- emit second half
  // end:
  // Emit first half register
  addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_simd_lane);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit16);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_ge);
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bra);
  // branch-target - will be patched below
  addUInt(Block, dwarf::DW_FORM_data2, 0xf00d);
  DIEValue *secondHalfOff = Block->getValues().back();
  IGC_ASSERT_MESSAGE(isa<DIEInteger>(secondHalfOff), "Expecting DIEInteger");
  unsigned int offsetNotTaken = Block->ComputeSizeOnTheFly(Asm);

  // Emit first register
  if (!buildValidVar(var, Block, loc, vars, DbgRegisterType::FirstHalf))
    return false;

  // Emit second half register
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_skip);
  // else-target - will be patched by skipOff in buildGeneral
  addUInt(Block, dwarf::DW_FORM_data2, 0xbeef);
  skipOff = Block->getValues().back();
  IGC_ASSERT_MESSAGE(isa<DIEInteger>(skipOff), "Expecting DIEInteger");
  offsetTaken = Block->ComputeSizeOnTheFly(Asm);
  cast<DIEInteger>(secondHalfOff)->setValue(offsetTaken - offsetNotTaken);

  // The second register is taken from the original location, then a new
  // location is created with this 2nd register to play a role of the 1st
  // register in buildValidVar(), which always processes the 1st register only.
  VISAVariableLocation second_loc(loc);
  second_loc.SetRegister(loc.GetSecondReg());
  if (!buildValidVar(var, Block, second_loc, vars, DbgRegisterType::SecondHalf))
    return false;

  return true;
}

bool CompileUnit::buildValidVar(DbgVariable &var, IGC::DIEBlock *Block, const VISAVariableLocation &loc,
                                const std::vector<DbgDecoder::LiveIntervalsVISA> *vars, DbgRegisterType regType) {
  const DbgDecoder::VarInfo *VarInfo = nullptr;
  const auto *VISAMod = loc.GetVISAModule();

  // When vars is valid, use it to encode location directly, otherwise
  // rely on getVarInfo result here.
  if (!vars) {
    auto regNum = loc.GetRegister();
    VarInfo = VISAMod->getVarInfo(DD->getVisaDebugInfo(), regNum);

    if (VarInfo)
      LLVM_DEBUG(dbgs() << "  general vISA Variable info: "; VarInfo->print(dbgs()); dbgs() << "\n");
    else
      LLVM_DEBUG(dbgs() << "  warning: could not get vISA Variable info\n");
  }

  const bool isSecondHalf = regType == DbgRegisterType::SecondHalf;
  const unsigned NumVarsExpected = isSecondHalf ? 2 : 1;
  const unsigned LRIndex = isSecondHalf ? 1 : 0;
  const DbgDecoder::LiveIntervalsVISA *lrToUse;
  if (vars && vars->size() >= NumVarsExpected)
    lrToUse = &vars->at(LRIndex);
  else if (VarInfo)
    lrToUse = &VarInfo->lrs.front();
  else
    return false;

  LLVM_DEBUG(dbgs() << "  emitting variable location at LR: <"; lrToUse->print(dbgs()); dbgs() << ">\n");
  var.setLocationRegisterType(regType);
  emitLocation = true;
  if (lrToUse->isGRF()) {
    if (!loc.IsVectorized()) {
      if (loc.isRegionBasedAddress()) {
        // VC-backend specific addressing model
        addSimdLaneRegionBase(Block, var, loc, lrToUse);
      } else {
        addSimdLaneScalar(Block, var, loc, *lrToUse);
      }
      var.emitExpression(this, Block);
      return false;
    } else {
      for (unsigned int vectorElem = 0; vectorElem < loc.GetVectorNumElements(); ++vectorElem) {
        // Emit SIMD lane for GRF (unpacked)
        constexpr auto MaxUI16 = std::numeric_limits<uint16_t>::max();
        const auto registerSizeInBits = DD->GetVISAModule()->getGRFSizeInBits();
        const auto instrSimdWidth = (DD->simdWidth > 16 && registerSizeInBits == 256) ? 16 : DD->simdWidth;
        auto SimdOffset = instrSimdWidth * vectorElem;
        IGC_ASSERT(DD->simdWidth <= 32 && vectorElem < MaxUI16 && SimdOffset < MaxUI16);
        if (loc.IsRegister())
          addSimdLane(Block, var, loc, lrToUse, (uint16_t)(SimdOffset), false, isSecondHalf);
      }
    }
  } else if (lrToUse->isSpill()) {
    if (!loc.IsVectorized()) {
      addScratchLocation(Block, lrToUse->getSpillOffset().memoryOffset, 0);
      addBE_FP(Block);
    } else {
      // TODO: revise these calculations
      unsigned GrfSizeBytes = VISAMod->getGRFSizeInBytes();
      IGC_ASSERT(GrfSizeBytes <= std::numeric_limits<uint16_t>::max());

      unsigned GrfSizeInBits = GrfSizeBytes * 8;
      IGC_ASSERT(GrfSizeInBits <= std::numeric_limits<uint16_t>::max());

      unsigned varSizeInBits = var.getRegisterValueSizeInBits(DD);

      unsigned varSizeInReg = (loc.IsInMemory() && varSizeInBits < 32) ? 32 : varSizeInBits;

      IGC_ASSERT(DD->simdWidth != 0);
      unsigned FullSizeInBits = varSizeInReg * DD->simdWidth;
      unsigned numOfRegs = (FullSizeInBits > GrfSizeInBits) ? (FullSizeInBits / GrfSizeInBits) : 1;
      IGC_ASSERT(numOfRegs <= std::numeric_limits<uint16_t>::max());

      for (unsigned int vectorElem = 0; vectorElem < loc.GetVectorNumElements(); ++vectorElem) {
        unsigned VectorOffset = vectorElem * numOfRegs * GrfSizeBytes;
        IGC_ASSERT(VectorOffset <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
        addScratchLocation(Block, lrToUse->getSpillOffset().memoryOffset, static_cast<int32_t>(VectorOffset));
        addBE_FP(Block);
        // Emit SIMD lane for spill (unpacked)
        addSimdLane(Block, var, loc, lrToUse, 0, false, isSecondHalf);
      }
    }
  } else {
    LLVM_DEBUG(dbgs() << "  <warning> variable is neither in GRF nor spilled\n");
  }
  var.emitExpression(this, Block);
  return true;
}

IGC::DIEBlock *CompileUnit::buildGeneral(DbgVariable &var, const VISAVariableLocation &loc,
                                         const std::vector<DbgDecoder::LiveIntervalsVISA> *vars,
                                         IGC::DIE *VariableDie) {
  IGC::DIEBlock *Block = new (DIEValueAllocator) IGC::DIEBlock();
  offsetTaken = 0;
  skipOff = nullptr;
  emitLocation = false;
  stackValueOffset = 0;

  LLVM_DEBUG(dbgs() << "  building DWARF info for the variable [" << var.getName() << "]\n");
  const auto *storageMD = var.getDbgInst()->getMetadata("StorageOffset");
  const auto *VISAMod = loc.GetVISAModule();
  IGC_ASSERT_MESSAGE(VISAMod, "VISA Module is expected for LOC");

  if (VISAMod->getPrivateBase() && VISAMod->hasPTO() && storageMD) {
    // This is executed only when llvm.dbg.declare still exists and no stack
    // call is supported. With mem2reg run, data is stored in GRFs and this wont
    // be executed.

    auto privateBaseRegNum = VISAMod->getPrivateBaseReg();
    if (privateBaseRegNum) // FIX ME if 0 is allowed
    {
      emitLocation = true;
      if (buildPrivateBaseRegBased(var, Block, loc))
        return Block;
    }
  }

  const auto *sizeMD = var.getDbgInst()->getMetadata("StorageSize");
  if (storageMD && sizeMD) {
    emitLocation = true;
    if (!buildFpBasedLoc(var, Block, loc))
      return Block;
  } else {
    if (loc.HasLocationSecondReg()) {
      buildSlicedLoc(var, Block, loc, vars);
    } else {
      buildValidVar(var, Block, loc, vars, DbgRegisterType::Regular);
    }
  }

  if (skipOff) {
    // In split SIMD case, we want to skip to DW_OP_stack_value at the end,
    // not past it.
    unsigned int offsetEnd = Block->ComputeSizeOnTheFly(Asm) - stackValueOffset;
    cast<DIEInteger>(skipOff)->setValue(offsetEnd - offsetTaken);
  }

  if (!emitLocation) {
    Block->~DIEBlock();
    return nullptr;
  }
  LLVM_DEBUG(Block->dump());

  return Block;
}

/// constructMemberDIE - Construct member DIE from DIDerivedType.
void CompileUnit::constructMemberDIE(DIE &Buffer, DIDerivedType *DT) {
  DIE *MemberDie = createAndAddDIE(DT->getTag(), Buffer);
  StringRef Name = DT->getName();
  if (!Name.empty()) {
    addString(MemberDie, dwarf::DW_AT_name, Name);
  }

  DIType *BaseTy = resolve(DT->getBaseType());
  // Void type is represented as null
  if (BaseTy)
    addType(MemberDie, BaseTy);

  addSourceLine(MemberDie, DT);

  if (DT->getTag() == dwarf::DW_TAG_inheritance && DT->isVirtual()) {
    // For C++, virtual base classes are not at fixed offset. Use following
    // expression to extract appropriate offset from vtable.
    // BaseAddr = ObAddr + *((*ObAddr) - Offset)

    IGC::DIEBlock *VBaseLocationDie = new (DIEValueAllocator) IGC::DIEBlock();
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_dup);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_udata, DT->getOffsetInBits());
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_minus);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);

    addBlock(MemberDie, dwarf::DW_AT_data_member_location, VBaseLocationDie);
  } else {
    uint64_t Size = DT->getSizeInBits();
    uint64_t FieldSize = DwarfDebug::getBaseTypeSize(DT);
    uint64_t OffsetInBytes;

    bool IsBitfield = Size < FieldSize;
    if (IsBitfield) {
      // Handle bitfield.
      addUInt(MemberDie, dwarf::DW_AT_byte_size, std::nullopt, FieldSize >> 3);
      addUInt(MemberDie, dwarf::DW_AT_bit_size, std::nullopt, Size);

      uint64_t Offset = DT->getOffsetInBits();
      uint64_t AlignMask = ~(DT->getAlignInBits() - 1);
      uint64_t HiMark = (Offset + FieldSize) & AlignMask;
      uint64_t FieldOffset = (HiMark - FieldSize);
      Offset -= FieldOffset;

      // Maybe we need to work from the other end.
      if (Asm->IsLittleEndian())
        Offset = FieldSize - (Offset + Size);
      addUInt(MemberDie, dwarf::DW_AT_bit_offset, std::nullopt, Offset);

      // Here DW_AT_data_member_location points to the anonymous
      // field that includes this bit field.
      OffsetInBytes = FieldOffset >> 3;
    } else {
      // This is not a bitfield.
      OffsetInBytes = DT->getOffsetInBits() >> 3;
    }
    addUInt(MemberDie, dwarf::DW_AT_data_member_location, std::nullopt, OffsetInBytes);
  }
  dwarf::AccessAttribute dw_access = (DT->isProtected()) ? dwarf::DW_ACCESS_protected
                                     : (DT->isPrivate()) ? dwarf::DW_ACCESS_private
                                                         : dwarf::DW_ACCESS_public;
  addUInt(MemberDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

  if (DT->isVirtual()) {
    addUInt(MemberDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1, dwarf::DW_VIRTUALITY_virtual);
  }

  if (DT->isArtificial()) {
    addFlag(MemberDie, dwarf::DW_AT_artificial);
  }
}

/// getOrCreateStaticMemberDIE - Create new DIE for C++ static member.
IGC::DIE *CompileUnit::getOrCreateStaticMemberDIE(DIDerivedType *DT) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *ContextDIE = getOrCreateContextDIE(resolve(DT->getScope()));
  IGC_ASSERT_MESSAGE(dwarf::isType(ContextDIE->getTag()), "Static member should belong to a type.");

  DIE *StaticMemberDIE = getDIE(DT);
  if (StaticMemberDIE)
    return StaticMemberDIE;

  StaticMemberDIE = createAndAddDIE(DT->getTag(), *ContextDIE, DT);

  DIType *Ty = resolve(DT->getBaseType());

  addString(StaticMemberDIE, dwarf::DW_AT_name, DT->getName());
  addType(StaticMemberDIE, Ty);
  addSourceLine(StaticMemberDIE, DT);
  addFlag(StaticMemberDIE, dwarf::DW_AT_external);
  addFlag(StaticMemberDIE, dwarf::DW_AT_declaration);

  // FIXME: We could omit private if the parent is a class_type, and
  // public if the parent is something else.
  dwarf::AccessAttribute dw_access = (DT->isProtected()) ? dwarf::DW_ACCESS_protected
                                     : (DT->isPrivate()) ? dwarf::DW_ACCESS_private
                                                         : dwarf::DW_ACCESS_public;
  addUInt(StaticMemberDIE, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

  if (const ConstantInt *CI = dyn_cast_or_null<ConstantInt>(DT->getConstant())) {
    addConstantValue(StaticMemberDIE, CI, isUnsignedDIType(DD, Ty));
  }

  if (const ConstantFP *CFP = dyn_cast_or_null<ConstantFP>(DT->getConstant())) {
    addConstantFPValue(StaticMemberDIE, CFP);
  }

  return StaticMemberDIE;
}

void CompileUnit::emitHeader(const MCSection *ASection, const MCSymbol *ASectionSym) {
  // Emit ("DWARF version number");
  Asm->EmitInt16(DD->getDwarfVersion());
  // DWARF5
  if (DD->getDwarfVersion() > 4) {
    Asm->EmitInt8(dwarf::DW_UT_compile);
    Asm->EmitInt8(Asm->GetPointerSize());
  }
  // Emit ("Offset Into Abbrev. Section");
  if (EmitSettings.EnableRelocation)
    // Emit 4-byte offset since we're using DWARF4 32-bit format
    Asm->EmitLabelReference(Asm->GetTempSymbol(
                                /*ASection->getLabelBeginName()*/ ".debug_abbrev_begin"),
                            4);
  else
    Asm->EmitSectionOffset(Asm->GetTempSymbol(
                               /*ASection->getLabelBeginName()*/ ".debug_abbrev_begin"),
                           ASectionSym);
  // DWARF4
  if (DD->getDwarfVersion() <= 4) {
    Asm->EmitInt8(Asm->GetPointerSize());
  }
}
