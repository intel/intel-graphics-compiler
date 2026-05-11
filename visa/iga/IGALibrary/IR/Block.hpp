/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BLOCK_HPP
#define IGA_BLOCK_HPP

// WARNING: this internal IR is subject to change without any notice.
// External tools should use the official interfaces in the external
// API (IGA/api).  Those interfaces are tested between releases and maintained
// even with changes to the internal IR (within reason).
#include "../ErrorHandler.hpp"
#include "../MemManager/MemManager.hpp"
#include "../MemManager/StdArenaAllocator.hpp"
#include "Instruction.hpp"

#include <list>
#include <map>

namespace iga {
typedef std::list<iga::Instruction *,
                  std_arena_based_allocator<iga::Instruction *>>
    InstList;
typedef InstList::iterator InstListIterator;

class Block {
public:
  Block(int32_t pc = -1, const Loc &loc = Loc::INVALID)
      : m_offset(pc), m_loc(loc), m_id(pc) {}
  ~Block() {
    // Destruct instructions.  The memory allocated for them will be
    // de-allocated by the top-level MemManager allocator, but we need
    // to explicitly call ~Instruction() to cleanup Instruction members
    // which don't use MemManager-allocated memory
    for (auto inst : m_instructions) {
      inst->~Instruction();
    }
  }

  void operator delete(void *, MemManager *) {}
  void *operator new(size_t sz, MemManager *m) { return m->alloc(sz); }

  void appendInstruction(Instruction *inst) { m_instructions.push_back(inst); }

  PC getPC() const { return m_offset; }
  void setPC(PC pc) { m_offset = pc; }
  Loc getLoc() const { return m_loc; }
  void setLoc(const Loc &loc) { m_loc = loc; }
  void setID(int id) { m_id = id; }
  int getID() const { return m_id; }
  const InstList &getInstList() const { return m_instructions; }
  InstList &getInstList() { return m_instructions; }
  void insertInstBefore(InstList::iterator iter, Instruction *inst);

  // infers the control flow graph
  // sets the Block* within these instructions
  static std::map<int32_t, Block *>
  inferBlocks(ErrorHandler &errHandler, MemManager &mem, InstList &insts);

private:
  int32_t m_offset;
  Loc m_loc; // optional src location
  InstList m_instructions;
  int m_id;

  Block(const Block &) = delete;
  Block &operator=(const Block &) = delete;
}; // class Block
} // namespace iga

#endif // BLOCK_HPP
