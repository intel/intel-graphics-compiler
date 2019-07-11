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
#ifndef IGA_BLOCK_HPP
#define IGA_BLOCK_HPP

// WARNING: this internal IR is subject to change without any notice.
// External tools should use the official interfaces in the external
// API (IGA/api).  Those interfaces are tested between releases and maintained
// even with changes to the internal IR (within reason).
#include "../MemManager/MemManager.hpp"
#include "../MemManager/StdArenaAllocator.hpp"
#include "../ErrorHandler.hpp"
#include "Instruction.hpp"

#include <map>
#include <list>

namespace iga
{
    typedef std::list<
       iga::Instruction*, std_arena_based_allocator<iga::Instruction*> > InstList;
    typedef InstList::iterator InstListIterator;

    class Block
    {
    public:
        Block(int32_t pc = -1, const Loc &loc = Loc::INVALID)
            : m_offset(pc)
            , m_loc(loc)
            , m_id(pc)
        {
        }
        ~Block() {
            // Destruct instructions.  The memory allocated for them will be
            // de-allocated by the top-level MemManager allocator, but we need
            // to explicitly call ~Instruction() to cleanup Instruction members
            // which don't use MemManager-allocated memory
            for (auto inst : m_instructions) {
                inst->~Instruction();
            }
        }

        void operator delete(void *, MemManager* m) { }
        void *operator new(size_t sz, MemManager* m) { return m->alloc(sz); }

        void appendInstruction(Instruction *inst) {
            m_instructions.push_back(inst);
        }

        PC                 getPC() const { return m_offset; }
        void               setPC(PC pc) { m_offset = pc; }
        Loc                getLoc() const { return m_loc; }
        void               setLoc(const Loc &loc) { m_loc = loc; }
        void               setID(int id) { m_id = id; }
        int                getID() const { return m_id; }
        const InstList&    getInstList() const { return m_instructions; }
              InstList&    getInstList()       { return m_instructions; }
        void               insertInstBefore(InstList::iterator iter,
                                            Instruction *inst);

        // infers the control flow graph
        // sets the Block* within these instructions
        static std::map<int32_t,Block*> inferBlocks(
            ErrorHandler &errHandler,
            MemManager& mem,
            InstList &insts);

    private:
        int32_t             m_offset;
        Loc                 m_loc; // optional src location
        InstList            m_instructions;
        int                 m_id;

        Block(const Block &) = delete;
        Block& operator=(const Block&) = delete;
    }; // class Block
} // namespace iga

#endif // BLOCK_HPP
