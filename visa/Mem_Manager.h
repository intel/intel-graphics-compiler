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

// An arena based memory manager implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_

#include "Arena.h"
namespace vISA
{
    class Mem_Manager {
    public:

        Mem_Manager(size_t defaultArenaSize);
        ~Mem_Manager();

        void* alloc(size_t size)
        {
            return _arenaManager.AllocDataSpace(size, ArenaHeader::defaultAlign);
        }

        void* alloc(size_t size, std::align_val_t al)
        {
            return _arenaManager.AllocDataSpace(size, static_cast<size_t>(al));
        }

    private:

        vISA::ArenaManager _arenaManager;
    };
}
#endif
