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

// A memory arena class implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#ifndef _ARENA_HPP_
#define _ARENA_HPP_

#include <assert.h>
#include <stdlib.h>
#include <iostream>

//#include "Option.h"

//#define COLLECT_ALLOCATION_STATS

#ifdef COLLECT_ALLOCATION_STATS
extern int numAllocations;
extern int numMallocCalls;
extern int totalAllocSize;
extern int totalMallocSize;
extern int numMemManagers;
extern int maxArenaLength;
#endif

namespace iga
{
class ArenaHeader
{
    friend class ArenaManager;

public:
    static size_t WordAlign(size_t addr)
    {
        return (addr + 0x3) & ~0x3;
    }

    static size_t GetArenaSize(size_t dataSize)
    {
        return WordAlign(sizeof (ArenaHeader)) + dataSize;
    }

    unsigned char* GetArenaData() const
    {
        assert(WordAlign (size_t (this)) == size_t (this));
        return (unsigned char*)(WordAlign (size_t (this) + sizeof(ArenaHeader)));
    }

    void* operator new(size_t, unsigned char* memory)
    {
        return memory;
    }

    void operator delete(void*, unsigned char*)
    {
        // Do nothing
    }

private:

    ArenaHeader(size_t dataSize, ArenaHeader* /* nextArena */)
        : _nextArena (nullptr)
    {
        _nextByte = GetArenaData ();
        _lastByte = _nextByte + dataSize;
        assert (((unsigned char*)(this) + GetArenaSize (dataSize)) == _lastByte);
    }

    ~ArenaHeader()
    {
        _nextByte = _lastByte = 0;
        _nextArena = 0;
    }

    void* AllocSpace(size_t size);


    ArenaHeader*   _nextArena;  // Word aligned
    unsigned char* _nextByte;   // Char aligned
    unsigned char* _lastByte;   // Char aligned
};

class ArenaManager
{
    friend class MemManager;

private:

    ArenaManager(size_t defaultArenaSize) :
       _arenas (0),
       _defaultArenaSize(defaultArenaSize)
    {
        CreateArena(_defaultArenaSize);
    }

    ~ArenaManager()
    {
        FreeArenas();
    }

    void* AllocDataSpace(size_t size)
    {
        // Do separate memory allocations of debugMemAlloc is set, to allow
        // valgrind/drmemory to find more buffer over-reads/writes
#if !defined(NDEBUG) && defined(IGA_DEBUG_MEM_ALLOC)
        return size == 0 ? 0 : malloc(size);
#endif
        void *space = nullptr;

        if (size)
        {
            space =_arenas->AllocSpace(size);

            if (space == 0)
            {
                CreateArena (size);
                space =_arenas->AllocSpace(size);
            }

            assert (space);
        }

#ifdef COLLECT_ALLOCATION_STATS
        numAllocations++;
        totalAllocSize += size;
#endif

        return space;
    }

    ArenaHeader *CreateArena(size_t size)
    {
        size_t arenaDataSize = (size > _defaultArenaSize)? size : _defaultArenaSize;
        arenaDataSize = ArenaHeader::WordAlign(arenaDataSize);
        unsigned char *arena =
            new unsigned char [ArenaHeader::GetArenaSize(arenaDataSize)];
        ArenaHeader *newArena = new (arena)ArenaHeader(arenaDataSize, _arenas);
        // Add new arena to the head of queue
        if (_arenas != NULL)
        {
            newArena->_nextArena = _arenas;
        }

        //std::cout << "Create new Buffer: " << (arenaDataSize / 1024) << " KB" << std::endl;
        _arenas = newArena;

#ifdef COLLECT_ALLOCATION_STATS
        numMallocCalls++;
        totalMallocSize += arenaDataSize;
        int numArenas = 0;
        for (ArenaHeader *tmpArena = _arenas; tmpArena != NULL; tmpArena = tmpArena->_nextArena)
        {
            numArenas++;
        }
        if (numArenas > maxArenaLength)
        {
            maxArenaLength = numArenas;
        }
        if (numArenas == 1)
        {
            numMemManagers++;
        }
#endif

        return _arenas;
    }

    void FreeArenas();

    // Data

    ArenaHeader  *_arenas;
    const size_t  _defaultArenaSize;
};
}
#endif
