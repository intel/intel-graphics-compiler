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

#ifndef BLOCK_HPP
#define BLOCK_HPP
// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include "Instruction.hpp"
#include "../MemManager/MemManager.hpp"
#include "../ErrorHandler.hpp"

#include <map>
#include <list>

namespace iga
{
    typedef std::list<
       iga::Instruction*, std_arena_based_allocator<iga::Instruction*> > InstList;

    class Block
    {
    public:
        Block(int32_t pc = -1, const Loc &loc = Loc::INVALID)
            : m_offset(pc)
            , m_loc(loc)
            , m_id(pc)
        {
        }
        ~Block() {}

        void operator delete(void *, MemManager* m) { }
        void *operator new(size_t sz, MemManager* m) { return m->alloc(sz); }

        void appendInstruction(Instruction *inst) {
            m_instructions.push_back(inst);
        }

        // TODO: deprecate
        int32_t            getOffset() const { return getPC(); }
        void               setOffset(int32_t pc) { setPC(pc); }
        int32_t            getPC() const { return m_offset; }
        void               setPC(int32_t pc) { m_offset = pc; }
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
            size_t binaryLength,
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
