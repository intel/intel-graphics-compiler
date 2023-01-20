/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

void *ArenaHeader::AllocSpace(size_t size, size_t al) {
  vASSERT(DefaultAlign(size_t(_nextByte)) == size_t(_nextByte));

  void *allocSpace = (void *)AlignAddr((size_t)_nextByte, al);

  if (size) {
    size = DefaultAlign(
        size); // round up size so that next address is at least max aligned

    if (_nextByte + size <= _lastByte) {
      _nextByte += size;
    } else {
      allocSpace = 0;
    }
  } else {
    allocSpace = 0;
  }

  return allocSpace;
}

void ArenaManager::FreeArenas() {
  while (_arenas) {
#ifdef COLLECT_ALLOCATION_STATS
    currentMallocSize -= _arenas->size;
#endif
    unsigned char *killed = (unsigned char *)_arenas;
    _arenas = _arenas->_nextArena;
    delete[] killed;
  }

  _arenas = 0;
}
