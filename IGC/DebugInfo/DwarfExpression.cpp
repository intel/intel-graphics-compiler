/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "DwarfExpression.hpp"

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

void DwarfExpression::addExpression(DIExpressionCursor &&ExprCursor) {
  while (ExprCursor) {
    auto Op = ExprCursor.take();
    uint64_t OpNum = Op->getOp();

    switch (OpNum) {
    case dwarf::DW_OP_plus_uconst:
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
      emitOp(dwarf::DW_OP_deref);
      break;
    case dwarf::DW_OP_constu:
      emitConstu(Op->getArg(0));
      break;
    case dwarf::DW_OP_consts:
      emitOp(dwarf::DW_OP_consts);
      emitSigned(Op->getArg(0));
      break;
    case dwarf::DW_OP_swap:
      emitOp(dwarf::DW_OP_swap);
      break;
    case dwarf::DW_OP_xderef:
      emitOp(dwarf::DW_OP_xderef);
      break;
    case dwarf::DW_OP_deref_size:
      emitOp(dwarf::DW_OP_deref_size);
      emitData1(static_cast<uint8_t>(Op->getArg(0)));
      break;
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
}
