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

#include "Arena.h"

#ifdef COLLECT_ALLOCATION_STATS
int numAllocations = 0;
int numMallocCalls = 0;
int totalAllocSize = 0;
int totalMallocSize = 0;
int numMemManagers = 0;
int maxArenaLength = 0;
int currentMallocSize = 0;
#endif
using namespace vISA;
void*
ArenaHeader::AllocSpace (size_t size)
{
    assert( WordAlign (size_t (_nextByte)) == size_t (_nextByte) );
    void * allocSpace = _nextByte;

    if (size)
    {
        size = WordAlign (size);

        if (_nextByte + size <= _lastByte) {
            _nextByte += size;
        }

        else {
            allocSpace = 0;
        }
    }
    else
    {
        allocSpace = 0;
    }

    return allocSpace;
}

void
ArenaManager::FreeArenas()
{
    while (_arenas)
    {
#ifdef COLLECT_ALLOCATION_STATS
        currentMallocSize -= _arenas->size;
#endif
        unsigned char* killed = (unsigned char*) _arenas;
        _arenas = _arenas->_nextArena;
        delete [] killed;
    }

    _arenas = 0;
}
