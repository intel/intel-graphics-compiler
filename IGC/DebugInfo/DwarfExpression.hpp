/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "DwarfDebug.hpp"
#include "StreamEmitter.hpp"
#include "llvm/IR/DebugInfoMetadata.h"
#include <cassert>
#include <cstdint>
#include <optional>

using namespace llvm;

namespace IGC {

/// Holds a DIExpression and keeps track of how many operands have been consumed
/// so far.
class DIExpressionCursor {
  DIExpression::expr_op_iterator Start, End;

public:
  DIExpressionCursor(const DIExpression *Expr) {
    if (!Expr) {
      assert(Start == End);
      return;
    }
    Start = Expr->expr_op_begin();
    End = Expr->expr_op_end();
  }

  /// Consume one operation.
  std::optional<DIExpression::ExprOperand> take() {
    if (Start == End)
      return std::nullopt;
    return *(Start++);
  }

  /// Determine whether there are any operations left in this expression.
  operator bool() const { return Start != End; }
};

class DwarfExpression {

protected:
  CompileUnit &CU;

  /// Output a dwarf operand and an optional assembler comment.
  virtual void emitOp(uint8_t Op, const char *Comment = nullptr) = 0;

  /// Emit a raw signed value.
  virtual void emitSigned(int64_t Value) = 0;

  /// Emit a raw unsigned value.
  virtual void emitUnsigned(uint64_t Value) = 0;

  virtual void emitData1(uint8_t Value) = 0;

  /// Emit a normalized unsigned constant.
  void emitConstu(uint64_t Value);

public:
  DwarfExpression(CompileUnit &CU) : CU(CU) {};

  /// Emit all remaining operations in the DIExpressionCursor.
  void addExpression(DIExpressionCursor &&Expr);

  ~DwarfExpression() = default;
};

class DIEDwarfExpression final : public DwarfExpression {
  const StreamEmitter &AP;
  DIEBlock &OutDIE;

  /// Return the DIE that currently is being emitted to.
  DIEBlock &getActiveDIE() { return OutDIE; }

  void emitOp(uint8_t Op, const char *Comment = nullptr) override;
  void emitSigned(int64_t Value) override;
  void emitUnsigned(uint64_t Value) override;
  void emitData1(uint8_t Value) override;

public:
  DIEDwarfExpression(const StreamEmitter &AP, CompileUnit &CU, DIEBlock &DIE);

  DIEBlock *finalize() { return &OutDIE; }
};

} // namespace IGC
