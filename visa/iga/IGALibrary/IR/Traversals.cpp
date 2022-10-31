/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Traversals.hpp"
#include "../asserts.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace iga;

void BranchIterator::findNext() {
  while (m_currSrcIx < m_srcCount) {
    const auto &op = m_inst->getSource(m_currSrcIx);
    if (op.getKind() == Operand::Kind::LABEL) {
      auto b = m_inst->getSource(m_currSrcIx).getTargetBlock();
      // FIXME!! What if it's an empty empty??
      //    jmpi FOO
      //  FOO: // oops, foo is empty!
      //  BAR:
      //    op
      //
      // we must either:
      //  A. define an IR normal form with coalesced blocks
      //  B. keep underlying block iterator copies around
      //     so we can walk forward searching for a non-empty block
      //  C. add a pointer links to Block (seems easiest)
      m_nextTarget = b->getInstList().front();
      break;
    }
    m_currSrcIx++;
  }
  if (m_currSrcIx == m_srcCount) {
    m_nextTarget = nullptr;
  }
}

BranchIterator::BranchIterator(Instruction *_inst, size_t _nextIx)
    : m_inst(_inst), m_currSrcIx(_nextIx), m_srcCount(_inst->getSourceCount()),
      m_nextTarget(nullptr) {
  if (!m_inst->isBranching()) { // early out
    m_currSrcIx = m_srcCount;
  } else {
    findNext();
  }
}

KernelIterator::KernelIterator(const BlockList::iterator &_bi,
                               const BlockList::iterator &_be)
    : bi(_bi), be(_be) {
  nextBlock();
  // invariants established
  IGA_ASSERT(bi == be || (ii != ie && m_currInst && m_currBlock),
             "KernelIterator construction invariants failed");
}

Instruction *KernelIterator::currInst() {
  IGA_ASSERT(m_currInst, "KernelIterator at end");
  return m_currInst;
}
Block &KernelIterator::currBlock() {
  IGA_ASSERT(m_currBlock, "KernelIterator at end");
  return *m_currBlock;
}

// ii and ie are equal or invalid
void KernelIterator::nextBlock() {
  while (bi != be) { // while -> skip empty blocks
    m_currBlock = *bi;
    ii = m_currBlock->getInstList().begin();
    ie = m_currBlock->getInstList().end();
    if (ii != ie) {
      m_currInst = *ii;
      return;
    }
    // next block
    bi++;
  }
  m_currInst = nullptr;
  m_currBlock = nullptr;
  return;
}

// walks to the next logical instruction
// ii and ie must be valid
void KernelIterator::nextInst() {
  IGA_ASSERT(bi != be, "KernelIterator at end");
  if (ii != ie) {
    ii++; // step
    if (ii != ie) {
      m_currInst = *ii;
      return; // same block
    }         // else { fallthrough and find next block }
  }
  nextBlock();
  bi++;
}

// post-increment
KernelIterator KernelIterator::operator++(int) {
  auto a = *this;
  IGA_ASSERT(bi != be, "KernelIterator at end");
  nextInst();
  return a;
}

KernelWalker::KernelWalker(BlockList &_blocks)
    : blocks(_blocks), end_itr(_blocks.end(), _blocks.end()) {}
