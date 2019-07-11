/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef IR_TRAVERSALS_HPP
#define IR_TRAVERSALS_HPP

#include "Instruction.hpp"
#include "Kernel.hpp" // BlockList TODO: move to Blocks.hpp (elide Encoder def)
#include "../asserts.hpp"

namespace iga
{

class BranchIterator
{
    Instruction *m_inst;
    size_t       m_currSrcIx, m_srcCount;
    Instruction *m_nextTarget;

    void findNext();
public:
    BranchIterator(Instruction *_inst, size_t _nextIx = 0);

    // TODO: should this return a kernel iterator?
    Instruction& operator *() const {
        IGA_ASSERT(m_currSrcIx < m_srcCount, "BranchIterator overflow");
        return *m_nextTarget;
    }
    bool operator==(const BranchIterator &rhs) const {
        IGA_ASSERT(m_inst == rhs.m_inst,
            "BranchIterator's point to different instructions");
        return m_currSrcIx == rhs.m_currSrcIx;
    }
    bool operator!=(const BranchIterator &rhs) const {
        return !(*this == rhs);
    }

    // post-increment
    BranchIterator operator++(int) {
        auto v = *this;
        IGA_ASSERT(m_currSrcIx < m_srcCount, "BranchIterator at end");
        findNext();
        return v;
    }

    // pre increment
    BranchIterator&  operator++() {
        IGA_ASSERT(m_currSrcIx < m_srcCount, "BranchIterator overflow");
        findNext();
        return *this;
    }
};

// TODO: we need a
//  KernelIterator
//  KernelRevIterator (builds map for PRED)

// walks the branches of an instruction
class BranchWalker
{
    Instruction *inst;
    BranchIterator end_itr;

    BranchWalker(Instruction *_inst)
        : inst(_inst)
        , end_itr(_inst,_inst->getSourceCount())
    {
    }
    BranchIterator begin() { return BranchIterator(inst, 0); }
    BranchIterator &end() { return end_itr; }
};

class KernelIterator
{
    // INVARIANTS:
    //  1. bi and be both valid always
    //  2. (bi == be) ==>  (m_currBlock == mCurrInst == nullptr)
    //  3. ii and ie always valid for current block OR (bi != be)
    //     if be == be then ii and ie are the end position of the last
    //     block's iterator or default values if the empty kernel is given
    BlockList::iterator bi, be;
    InstList::iterator ii, ie;
    // non-null after construction until we reach end()
    Block *m_currBlock = nullptr;
    Instruction *m_currInst = nullptr;

    void nextBlock();
    void nextInst();
public:
    KernelIterator(const BlockList::iterator &_bi, const BlockList::iterator &_be);

    Instruction* currInst();
    Block& currBlock();

    // returns an iterator of KernelIterators for branches
    // TODO: or successors()
    BranchIterator branches() const { return BranchIterator(m_currInst); }

    // I.e. we can write
    //   for (Instruction &i : KernelIterator(...)) {
    //     ..
    //   }
    Instruction & operator *() const {
        IGA_ASSERT(bi != be, "KernelIterator at end");
        return *m_currInst;
    }
    bool operator==(const KernelIterator &rhs) const {
        return bi == rhs.bi &&
            (bi == be || ii == rhs.ii);
    }
    bool operator!=(const KernelIterator &rhs) const {
        return !(*this == rhs);
    }
    KernelIterator  operator++(int); // post-increment
    KernelIterator&  operator++() { nextInst(); return *this; } // pre increment
};

struct KernelWalker {
    BlockList &blocks;
    KernelIterator end_itr;

    KernelWalker(BlockList &_blocks);
    KernelWalker(Kernel &k) : KernelWalker(k.getBlockList()) { }
    KernelIterator begin() const {return KernelIterator(blocks.begin(), blocks.end()); }
    const KernelIterator &end() const { return end_itr; }
};

} // namespace iga

#endif // IR_TRAVERSALS_HPP
