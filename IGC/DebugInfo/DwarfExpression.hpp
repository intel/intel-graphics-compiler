/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "DwarfDebug.hpp"
#include "StreamEmitter.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <cassert>
#include <cstdint>
#include <iterator>

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

  DIExpressionCursor(ArrayRef<uint64_t> Expr)
      : Start(Expr.begin()), End(Expr.end()) {}

  DIExpressionCursor(const DIExpressionCursor &) = default;

  /// Consume one operation.
  Optional<DIExpression::ExprOperand> take() {
    if (Start == End)
      return None;
    return *(Start++);
  }

  /// Consume N operations.
  void consume(unsigned N) { std::advance(Start, N); }

  /// Return the current operation.
  Optional<DIExpression::ExprOperand> peek() const {
    if (Start == End)
      return None;
    return *(Start);
  }

  /// Return the next operation.
  Optional<DIExpression::ExprOperand> peekNext() const {
    if (Start == End)
      return None;

    auto Next = Start.getNext();
    if (Next == End)
      return None;

    return *Next;
  }

  /// Determine whether there are any operations left in this expression.
  operator bool() const { return Start != End; }

  DIExpression::expr_op_iterator begin() const { return Start; }
  DIExpression::expr_op_iterator end() const { return End; }

  /// Retrieve the fragment information, if any.
  Optional<DIExpression::FragmentInfo> getFragmentInfo() const {
    return DIExpression::getFragmentInfo(Start, End);
  }
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
  DwarfExpression(CompileUnit &CU) : CU(CU){};

  /// This needs to be called last to commit any pending changes.
  void finalize();

  /// Emit all remaining operations in the DIExpressionCursor. The
  /// cursor must not contain any DW_OP_LLVM_arg operations.
  void addExpression(DIExpressionCursor &&Expr);

  /// Emit all remaining operations in the DIExpressionCursor.
  /// DW_OP_LLVM_arg operations are resolved by calling (\p InsertArg).
  //
  /// \return false if any call to (\p InsertArg) returns false.
  bool addExpression(
      DIExpressionCursor &&Expr,
      llvm::function_ref<bool(unsigned, DIExpressionCursor &)> InsertArg);

  ~DwarfExpression() = default;
};

class DIEDwarfExpression final : public DwarfExpression {
  const StreamEmitter &AP;
  DIEBlock &OutDIE;
  DIEBlock TmpDIE;
  bool IsBuffering = false;

  /// Return the DIE that currently is being emitted to.
  DIEBlock &getActiveDIE() { return IsBuffering ? TmpDIE : OutDIE; }

  void emitOp(uint8_t Op, const char *Comment = nullptr) override;
  void emitSigned(int64_t Value) override;
  void emitUnsigned(uint64_t Value) override;
  void emitData1(uint8_t Value) override;

public:
  DIEDwarfExpression(const StreamEmitter &AP, CompileUnit &CU, DIEBlock &DIE);

  DIEBlock *finalize() {
    DwarfExpression::finalize();
    return &OutDIE;
  }
};

} // namespace IGC
