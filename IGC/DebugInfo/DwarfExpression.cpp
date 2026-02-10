/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "DwarfExpression.hpp"
#include <optional>

using namespace IGC;

void DwarfExpression::emitConstu(uint64_t Value) {
  if (Value < 32)
    emitOp(static_cast<uint8_t>(dwarf::DW_OP_lit0 + Value));
  else if (Value == std::numeric_limits<uint64_t>::max()) {
    // Only do this for 64-bit values as the DWARF expression stack uses
    // target-address-size values.
    emitOp(dwarf::DW_OP_lit0);
    emitOp(dwarf::DW_OP_not);
  } else {
    emitOp(dwarf::DW_OP_constu);
    emitUnsigned(Value);
  }
}

bool DwarfExpression::addExpression(DIExpressionCursor &&ExprCursor,
                                    llvm::function_ref<bool(unsigned, DIExpressionCursor &)> InsertArg) {
  // Entry values can currently only cover the initial register location,
  // and not any other parts of the following DWARF expression.
  // assert(!IsEmittingEntryValue && "Can't emit entry value around
  // expression");

  while (ExprCursor) {
    auto Op = ExprCursor.take();
    uint64_t OpNum = Op->getOp();

    // if (OpNum >= dwarf::DW_OP_reg0 && OpNum <= dwarf::DW_OP_reg31) {
    //   emitOp(OpNum);
    //   continue;
    // } else if (OpNum >= dwarf::DW_OP_breg0 && OpNum <= dwarf::DW_OP_breg31) {
    //   addBReg(OpNum - dwarf::DW_OP_breg0, Op->getArg(0));
    //   continue;
    // }

    switch (OpNum) {
    // case dwarf::DW_OP_LLVM_arg:
    //   if (!InsertArg(Op->getArg(0), ExprCursor)) {
    //     LocationKind = Unknown;
    //     return false;
    //   }
    //   break;
    // case dwarf::DW_OP_LLVM_fragment: {
    //   unsigned SizeInBits = Op->getArg(1);
    //   unsigned FragmentOffset = Op->getArg(0);
    //   // The fragment offset must have already been adjusted by emitting an
    //   // empty DW_OP_piece / DW_OP_bit_piece before we emitted the base
    //   // location.
    //   assert(OffsetInBits >= FragmentOffset && "fragment offset not added?");
    //   assert(SizeInBits >= OffsetInBits - FragmentOffset && "size
    //   underflow");

    //  // If addMachineReg already emitted DW_OP_piece operations to represent
    //  // a super-register by splicing together sub-registers, subtract the
    //  size
    //  // of the pieces that was already emitted.
    //  SizeInBits -= OffsetInBits - FragmentOffset;

    //  // If addMachineReg requested a DW_OP_bit_piece to stencil out a
    //  // sub-register that is smaller than the current fragment's size, use
    //  it. if (SubRegisterSizeInBits)
    //    SizeInBits = std::min<unsigned>(SizeInBits, SubRegisterSizeInBits);

    //  // Emit a DW_OP_stack_value for implicit location descriptions.
    //  if (isImplicitLocation())
    //    addStackValue();

    //  // Emit the DW_OP_piece.
    //  addOpPiece(SizeInBits, SubRegisterOffsetInBits);
    //  setSubRegisterPiece(0, 0);
    //  // Reset the location description kind.
    //  LocationKind = Unknown;
    //  return true;
    //}
    case dwarf::DW_OP_plus_uconst:
      // assert(!isRegisterLocation());
      emitOp(dwarf::DW_OP_plus_uconst);
      emitUnsigned(Op->getArg(0));
      break;
    case dwarf::DW_OP_plus:
    case dwarf::DW_OP_minus:
    case dwarf::DW_OP_mul:
    case dwarf::DW_OP_div:
    case dwarf::DW_OP_mod:
    case dwarf::DW_OP_or:
    case dwarf::DW_OP_and:
    case dwarf::DW_OP_xor:
    case dwarf::DW_OP_shl:
    case dwarf::DW_OP_shr:
    case dwarf::DW_OP_shra:
    case dwarf::DW_OP_lit0:
    case dwarf::DW_OP_not:
    case dwarf::DW_OP_dup:
    case dwarf::DW_OP_push_object_address:
    case dwarf::DW_OP_over:
      emitOp(static_cast<uint8_t>(OpNum));
      break;
    case dwarf::DW_OP_deref:
      // assert(!isRegisterLocation());
      // if (!isMemoryLocation() && ::isMemoryLocation(ExprCursor))
      //   // Turning this into a memory location description makes the deref
      //   // implicit.
      //   LocationKind = Memory;
      // else
      emitOp(dwarf::DW_OP_deref);
      break;
    case dwarf::DW_OP_constu:
      // assert(!isRegisterLocation());
      emitConstu(Op->getArg(0));
      break;
    case dwarf::DW_OP_consts:
      // assert(!isRegisterLocation());
      emitOp(dwarf::DW_OP_consts);
      emitSigned(Op->getArg(0));
      break;
    // case dwarf::DW_OP_LLVM_convert: {
    //   unsigned BitSize = Op->getArg(0);
    //   dwarf::TypeKind Encoding = static_cast<dwarf::TypeKind>(Op->getArg(1));
    //   if (DwarfVersion >= 5 && CU.getDwarfDebug().useOpConvert()) {
    //     emitOp(dwarf::DW_OP_convert);
    //     // If targeting a location-list; simply emit the index into the raw
    //     // byte stream as ULEB128, DwarfDebug::emitDebugLocEntry has been
    //     // fitted with means to extract it later.
    //     // If targeting a inlined DW_AT_location; insert a DIEBaseTypeRef
    //     // (containing the index and a resolve mechanism during emit) into
    //     the
    //     // DIE value list.
    //     emitBaseTypeRef(getOrCreateBaseType(BitSize, Encoding));
    //   } else {
    //     if (PrevConvertOp && PrevConvertOp->getArg(0) < BitSize) {
    //       if (Encoding == dwarf::DW_ATE_signed)
    //         emitLegacySExt(PrevConvertOp->getArg(0));
    //       else if (Encoding == dwarf::DW_ATE_unsigned)
    //         emitLegacyZExt(PrevConvertOp->getArg(0));
    //       PrevConvertOp = None;
    //     } else {
    //       PrevConvertOp = Op;
    //     }
    //   }
    //   break;
    // }
    // case dwarf::DW_OP_stack_value:
    //   LocationKind = Implicit;
    //   break;
    case dwarf::DW_OP_swap:
      // assert(!isRegisterLocation());
      emitOp(dwarf::DW_OP_swap);
      break;
    case dwarf::DW_OP_xderef:
      // assert(!isRegisterLocation());
      emitOp(dwarf::DW_OP_xderef);
      break;
    case dwarf::DW_OP_deref_size:
      emitOp(dwarf::DW_OP_deref_size);
      emitData1(static_cast<uint8_t>(Op->getArg(0)));
      break;
    // case dwarf::DW_OP_LLVM_tag_offset:
    //   TagOffset = Op->getArg(0);
    //   break;
    case dwarf::DW_OP_regx:
      emitOp(dwarf::DW_OP_regx);
      emitUnsigned(Op->getArg(0));
      break;
    case dwarf::DW_OP_bregx:
      emitOp(dwarf::DW_OP_bregx);
      emitUnsigned(Op->getArg(0));
      emitSigned(Op->getArg(1));
      break;
    default:
      llvm_unreachable("unhandled opcode found in expression");
    }
  }

  // if (isImplicitLocation() && !isParameterValue())
  //   // Turn this into an implicit location description.
  //   addStackValue();

  return true;
}

void DwarfExpression::finalize() {
  // assert(DwarfRegs.size() == 0 && "dwarf registers not emitted");
  //// Emit any outstanding DW_OP_piece operations to mask out subregisters.
  // if (SubRegisterSizeInBits == 0)
  //  return;
  //// Don't emit a DW_OP_piece for a subregister at offset 0.
  // if (SubRegisterOffsetInBits == 0)
  //  return;
  // addOpPiece(SubRegisterSizeInBits, SubRegisterOffsetInBits);
}