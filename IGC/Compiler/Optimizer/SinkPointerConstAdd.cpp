/*========================== begin_copyright_notice ============================

Copyright (C) 2025-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SinkPointerConstAdd.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/KnownBits.h"
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"

// Sinks pointer-constant adds toward inttoptr so GVN can fold redundant
// pointer arithmetic:
//
//   %p1 = <base1> + const          %p1 = <base1> + <base2>
//   %p2 = <base2> + %p1     ===>   %p2 = %p1 + const
//   %ptr = inttoptr %p2            %ptr = inttoptr %p2
//
// Crossing an i32->i64 zext is the hard case. The frontend (DXIL) emits
// `zext` for every i32->i64 in pointer arithmetic, but the i32 chain may
// have been built with either unsigned or signed semantics:
//   - Unsigned: chain is a "large unsigned" descriptor offset that
//     happens to have bit 31 = 1 at runtime; zext is correct.
//   - Signed: chain is signed (e.g., add(rv, mul x, -32)) and can
//     legitimately go negative; zext is wrong, sext is needed.
// The two cases are isomorphic at the IR level; the frontend already erased
// the distinction. This pass classifies the chain into three buckets and
// only acts when it can be confident:
//   - ProvenNonNegative      : KnownBits proves bit 31 = 0 (zext == sext).
//                              Sink freely; cast stays as-is.
//   - HasExplicitSignedArith : chain contains a `mul` by a negative
//                              ConstantInt. This is the only marker we
//                              treat as signed-intent. `Sub`, `and -N`,
//                              `add -N`, `or -N`, `xor -N` are all
//                              valid unsigned/modular idioms and are NOT
//                              taken as evidence of signed semantics.
//                              Frontend's zext is wrong for this chain;
//                              flip to sext, then sink.
//   - Unknown                : anything else (RV alone, bitcast, phi,
//                              select, intrinsics we don't recognize, etc.).
//                              We can't tell which extension is correct, so
//                              we leave the IR alone. This is intentional
//                              over-conservatism: the alternative is a guess
//                              that produces wrong code in one of the cases
//                              above.
// The pass is fundamentally a heuristic; the only formal fix is upstream in
// the frontend, where signedness intent should be preserved.

using namespace llvm;

class SinkPointerConstAddPass : public llvm::FunctionPass {
public:
  SinkPointerConstAddPass() : llvm::FunctionPass(ID) {
    initializeSinkPointerConstAddPassPass(*llvm::PassRegistry::getPassRegistry());
  }
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
  virtual llvm::StringRef getPassName() const override { return "SinkPointerConstAdd"; }
  static char ID;

private:
  enum class ChainSign {
    HasExplicitSignedArith,
    Unknown,
  };

  bool getConstantOffset(llvm::Value *value, int &offset, const llvm::DataLayout &DL);
  ChainSign classifyChain(llvm::Instruction *op, llvm::BasicBlock *parentBB);

  // Tracks IR mutations that don't surface through getConstantOffset's bool
  // return (specifically the zext->sext flip). runOnFunction OR's this into
  // its `changed` return so pass-manager invalidation is honored even when
  // we flipped a cast but found no constant to sink afterward.
  bool m_castFlipped = false;
};

#define PASS_FLAG "igc-sink-ptr-const-add"
#define PASS_DESCRIPTION "Sink pointer const add"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(SinkPointerConstAddPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// Walks the chain rooted at `op` and looks for an explicit signed-arith
// marker. The only marker we trust is `mul` by a negative ConstantInt --
// the frontend would only emit this if the source-level type was signed.
// Other "looks negative" patterns are NOT taken as signed-intent because
// they have legitimate unsigned uses:
//   - `sub` is valid in unsigned modular arithmetic (e.g. `len - 1`).
//   - `and x, -64` (i.e., bit-31-set mask) is alignment, not signed.
//   - `add x, -N` is modular subtraction, not signed.
//   - `or` / `xor` with negative ConstantInt are bitmask idioms.
// Returns Unknown for everything else (RV, Argument, LoadInst, bitcast,
// phi, select, unrecognized ops). Unknown is the conservative answer:
// the caller leaves the IR untouched rather than guess.
SinkPointerConstAddPass::ChainSign SinkPointerConstAddPass::classifyChain(llvm::Instruction *op,
                                                                          llvm::BasicBlock *parentBB) {
  if (!op || op->getParent() != parentBB) {
    return ChainSign::Unknown;
  }
  if (llvm::ExtractElementInst *eei = llvm::dyn_cast<llvm::ExtractElementInst>(op)) {
    return classifyChain(llvm::dyn_cast<llvm::Instruction>(eei->getOperand(0)), parentBB);
  }
  if (llvm::BinaryOperator *bo = llvm::dyn_cast<llvm::BinaryOperator>(op)) {
    if (bo->getOpcode() == llvm::Instruction::Mul) {
      llvm::ConstantInt *cOp0 = llvm::dyn_cast<llvm::ConstantInt>(bo->getOperand(0));
      llvm::ConstantInt *cOp1 = llvm::dyn_cast<llvm::ConstantInt>(bo->getOperand(1));
      if ((cOp0 && cOp0->isNegative()) || (cOp1 && cOp1->isNegative())) {
        return ChainSign::HasExplicitSignedArith;
      }
    }
    auto a = classifyChain(llvm::dyn_cast<llvm::Instruction>(bo->getOperand(0)), parentBB);
    if (a == ChainSign::HasExplicitSignedArith) {
      return ChainSign::HasExplicitSignedArith;
    }
    auto b = classifyChain(llvm::dyn_cast<llvm::Instruction>(bo->getOperand(1)), parentBB);
    if (b == ChainSign::HasExplicitSignedArith) {
      return ChainSign::HasExplicitSignedArith;
    }
    return ChainSign::Unknown;
  }
  // RV / Argument / LoadInst / bitcast / phi / select / unrecognized intrinsic:
  // we have no signal that the chain is signed, but no signal that it isn't.
  return ChainSign::Unknown;
}

bool SinkPointerConstAddPass::getConstantOffset(llvm::Value *value, int &offset, const llvm::DataLayout &DL) {
  // Recursively search for constant add operations - this will stop after the first const add found,
  // and should be called repeatedly until no more const adds can be sunk.

  if (!value->hasOneUse()) {
    return false; // We cannot sink constant add if the value is used more than once.
  }

  if (llvm::Instruction *instr = llvm::dyn_cast<llvm::Instruction>(value)) {
    if (instr->getOpcode() == llvm::Instruction::Trunc || instr->getOpcode() == llvm::Instruction::ZExt ||
        instr->getOpcode() == llvm::Instruction::SExt) {
      llvm::Instruction *op = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(0));
      if (!op || instr->getParent() != op->getParent()) {
        return false;
      }
      if (instr->getOpcode() == llvm::Instruction::ZExt) {
        llvm::Value *operand = instr->getOperand(0);
        unsigned bw = operand->getType()->getIntegerBitWidth();
        llvm::KnownBits known = llvm::computeKnownBits(operand, DL);
        if (!known.Zero[bw - 1]) {
          // bit 31 unproven. Look for explicit signed-arith markers in the chain.
          ChainSign sign = classifyChain(op, instr->getParent());
          if (sign == ChainSign::HasExplicitSignedArith) {
            // Frontend's zext is wrong for this chain; flip to sext.
            llvm::IRBuilder<> builder(instr);
            llvm::Value *sext = builder.CreateSExt(operand, instr->getType());
            instr->replaceAllUsesWith(sext);
            instr->eraseFromParent();
            m_castFlipped = true;
            return getConstantOffset(operand, offset, DL);
          }
          // Unknown: we can't tell if zext or sext is right. Leave the IR alone.
          return false;
        }
      }
      return getConstantOffset(instr->getOperand(0), offset, DL);
    } else if (instr->getOpcode() == llvm::Instruction::Add || instr->getOpcode() == llvm::Instruction::Sub) {
      if (llvm::ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(instr->getOperand(1))) {
        offset = instr->getOpcode() == llvm::Instruction::Add ? offset + constInt->getSExtValue()
                                                              : offset - constInt->getSExtValue();
        instr->replaceAllUsesWith(instr->getOperand(0));
        instr->eraseFromParent();
        return true;
      } else if (llvm::ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(instr->getOperand(0))) {
        if (instr->getOpcode() == llvm::Instruction::Sub) {
          // Cannot sink constant on the left side of subtraction
          return false;
        }
        offset += constInt->getSExtValue();
        instr->replaceAllUsesWith(instr->getOperand(1));
        instr->eraseFromParent();
        return true;
      } else {
        llvm::Instruction *op0 = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(0));
        llvm::Instruction *op1 = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(1));
        // This is a simple pass, only sink within the same basic block
        return (op0 && instr->getParent() == op0->getParent() && getConstantOffset(op0, offset, DL)) ? true
               : (op1 && instr->getParent() == op1->getParent()) ? getConstantOffset(op1, offset, DL)
                                                                 : false;
      }
    }
  }

  return false;
}

bool SinkPointerConstAddPass::runOnFunction(llvm::Function &F) {
  bool changed = false;
  m_castFlipped = false;
  const llvm::DataLayout &DL = F.getParent()->getDataLayout();
  std::vector<llvm::IntToPtrInst *> intToPtrInsts;
  intToPtrInsts.reserve(50);

  // Collect all inttoptr instructions first
  for (llvm::BasicBlock &BB : F) {
    for (llvm::Instruction &inst : BB) {
      if (llvm::IntToPtrInst *intrinsic = llvm::dyn_cast<llvm::IntToPtrInst>(&inst)) {
        intToPtrInsts.push_back(intrinsic);
      }
    }
  }

  for (auto &intrinsic : intToPtrInsts) {
    int offset = 0;
    // Keep sinking constant adds until no more can be sunk
    while (getConstantOffset(intrinsic->getOperand(0), offset, DL)) {
      changed = true;
    }

    // If we found any constant offset, create new pointer calculation
    if (offset != 0) {
      llvm::IRBuilder<> builder(intrinsic);
      llvm::Value *newPtr =
          builder.CreateIntToPtr(builder.CreateAdd(intrinsic->getOperand(0),
                                                   llvm::ConstantInt::get(intrinsic->getOperand(0)->getType(), offset)),
                                 intrinsic->getType());
      intrinsic->replaceAllUsesWith(newPtr);
      intrinsic->eraseFromParent();
    }
  }

  return changed || m_castFlipped;
}

char SinkPointerConstAddPass::ID = 0;

llvm::FunctionPass *createSinkPointerConstAddPass() { return new SinkPointerConstAddPass(); }
