/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
