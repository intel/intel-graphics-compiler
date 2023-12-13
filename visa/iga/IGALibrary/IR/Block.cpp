/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// #define DEBUG_BLOCKS
#define TRACE_BLOCK(...) DebugTrace(iga::format(__VA_ARGS__).c_str())

#include "Block.hpp"
#include "../ErrorHandler.hpp"
#include "../Frontend/IRToString.hpp"
#include "../asserts.hpp"
#include "Instruction.hpp"

#include <set>
#include <sstream>
#include <vector>

using namespace iga;

struct BlockInference {
  MemManager *allocator;

  std::map<int32_t, Block *> &blockStarts;
  std::set<int32_t> instStarts;

  struct ResolvedTarget {
    Loc loc;   // instruction location
    int srcIx; // source index
    int32_t targetPc;
    ResolvedTarget(Loc _loc, int _srcIx, int32_t _targetPc)
        : loc(_loc), srcIx(_srcIx), targetPc(_targetPc) {}
  };
  std::vector<ResolvedTarget> resolved;

  BlockInference(std::map<int32_t, Block *> &bs, MemManager *a)
      : allocator(a), blockStarts(bs) {}

  Block *getBlock(int32_t pc) {
    auto itr = blockStarts.find(pc);
    if (itr == blockStarts.end()) {
      Block *blk = new (allocator) Block(pc);
      blockStarts[pc] = blk;
      return blk;
    } else {
      return itr->second;
    }
  }

  void replaceNumericLabel(ErrorHandler &errHandler, size_t binaryLength,
                           int32_t pc, int32_t instLen, Instruction *inst,
                           int srcIx) {
    Operand &src = inst->getSource(srcIx);
    if (src.getKind() == Operand::Kind::LABEL) {
      int32_t targetPc = src.getImmediateValue().s32;
      if (!inst->getOpSpec().isJipAbsolute())
        targetPc += pc;
      if (targetPc < 0 || targetPc > (int32_t)binaryLength) {
        std::stringstream ss;
        ss << "src" << srcIx << " targets";
        if (targetPc < 0) {
          ss << " before kernel start";
        } else {
          ss << " after kernel end";
        }
        ss << ": PC " << targetPc;

        Loc loc(0, 0, (uint32_t)pc, (uint32_t)instLen);
        errHandler.reportError(loc, ss.str());
      } else {
        resolved.emplace_back(inst->getLoc(), srcIx, targetPc);
      }
      src.setLabelSource(getBlock(targetPc), src.getType());
    }
  }

  void run(ErrorHandler &errHandler, int32_t binaryLength, InstList &insts) {
    // define start block to ensure at least one block exists
    (void)getBlock(0);

    int32_t pc = 0;
    for (Instruction *inst : insts) {
      instStarts.insert(inst->getPC());
      int32_t instLen = inst->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
      if (inst->getOpSpec().isBranching() || inst->isMovWithLabel()) {
        // all branching instructions can redirect to next instruction
        // start a new block after this one
        (void)getBlock(pc + instLen);
        // replace src0
        replaceNumericLabel(errHandler, binaryLength, pc, instLen, inst, 0);
        // replace src1
        if (inst->getSourceCount() > 1) {
          replaceNumericLabel(errHandler, binaryLength, pc, instLen, inst, 1);
        }
      } else if (inst->hasInstOpt(InstOpt::EOT)) {
        // also treat EOT as the end of a BB
        (void)getBlock(pc + instLen);
      }
      pc += instLen;
    }

    // for each block, we need to append the following instructions
    pc = 0;
    auto bitr = blockStarts.begin();
    Block *currBlock = bitr->second;
    bitr++;

    for (Instruction *inst : insts) {
      int32_t instLen = inst->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
      if (bitr != blockStarts.end() && pc >= bitr->first) {
        currBlock = bitr->second;
        bitr++;
      }

      currBlock->appendInstruction(inst);

      pc += instLen;
    }

    for (const ResolvedTarget &rt : resolved) {
      if (rt.targetPc != binaryLength && // EOF is also a valid target
          instStarts.find(rt.targetPc) == instStarts.end()) {
        std::stringstream ss;
        ss << "src" << rt.srcIx <<
            // it must've been a numeric label because symbolic labels
            // couldn't cause this
            ": numeric label targets the middle of an instruction";
        errHandler.reportError(rt.loc, ss.str());
      }
    }
  }
}; // class BlockInference

#ifdef DEBUG_BLOCKS
static void traceOperand(const Operand &op) {
  TRACE_BLOCK("  ");
  switch (op.getKind()) {
  case Operand::Kind::DIRECT:
    TRACE_BLOCK(ToSyntax(op.getDirRegName()), (int)op.getDirRegRef().regNum,
                ".", (int)op.getDirRegRef().subRegNum);
    break;
  case Operand::Kind::INDIRECT:
    TRACE_BLOCK("IND");
    break;
  case Operand::Kind::IMMEDIATE:
    TRACE_BLOCK("0x", hex(op.getImmediateValue().u64));
    if (op.getType() == Type::F) {
      TRACE_BLOCK("(=", op.getImmediateValue().f32, ")");
    } else if (op.getType() == Type::DF) {
      TRACE_BLOCK("(=", op.getImmediateValue().f64, ")");
    }
    break;
  case Operand::Kind::LABEL:
    TRACE_BLOCK("@", op.getTargetBlock(), ":", op.getImmediateValue().s32);
    break;
  case Operand::Kind::INVALID:
    TRACE_BLOCK("INV");
    break;
  default:
    TRACE_BLOCK("??");
    break;
  }
  if (op.getType() != Type::INVALID) {
    TRACE_BLOCK(ToSyntax(op.getType()).c_str());
  }
}
#endif // DEBUG_BLOCKS

std::map<int32_t, Block *>
Block::inferBlocks(ErrorHandler &errHandler, MemManager &mem, InstList &insts) {
  std::map<int32_t, Block *> blockStarts;
  BlockInference bi(blockStarts, &mem);
  int32_t binaryLength = 0;
  if (!insts.empty()) {
    Instruction *i = insts.back();
    binaryLength = i->getPC();
    binaryLength += i->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
  }
  bi.run(errHandler, binaryLength, insts);
#ifdef DEBUG_BLOCKS
  TRACE_BLOCK("**********************************************\n");
  for (const auto& bitr : blockStarts) {
    auto instList = bitr.second->getInstList();
    TRACE_BLOCK("BLOCK ", (int)bitr.first, " (", bitr.second, ") => ",
                (int)instList.size(), " instrs\n");
    for (auto inst : instList) {
      auto mne = inst->getOpSpec().mnemonic.str();
      TRACE_BLOCK("  ID", inst->getID(), " => PC", inst->getPC(), "  ", mne);
      traceOperand(inst->getDestination());

      for (int srcIx = 0; srcIx < (int)inst->getSourceCount(); srcIx++) {
        TRACE_BLOCK(",  ");
        traceOperand(inst->getSource(srcIx));
      }
      if (inst->hasInstOpt(InstOpt::EOT)) {
        TRACE_BLOCK(" {EOT}");
      }
      TRACE_BLOCK("\n");
    }
  }
#endif // DEBUG_BLOCKS

  return blockStarts;
}

void Block::insertInstBefore(InstList::iterator itr, Instruction *i) {
  m_instructions.insert(itr, i);
}
