/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A memory arena class implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#ifndef _ARENA_H_
#define _ARENA_H_

#include <assert.h>
#include <cstddef>
#include <iostream>
#include <stdlib.h>

#include "Assertions.h"
#include "Option.h"

// #define COLLECT_ALLOCATION_STATS

#ifdef COLLECT_ALLOCATION_STATS
extern int numAllocations;
extern int numMallocCalls;
extern int totalAllocSize;
extern int totalMallocSize;
extern int numMemManagers;
extern int maxArenaLength;
extern int currentMallocSize;
#endif

namespace vISA {
class Mem_Manager;
class ArenaHeader {
  friend class ArenaManager;

public:
  // FIXME: Currently there should be no type that needs 16-byte alignment.
  // If such types exist in the future, either change this to
  // __STDCPP_DEFAULT_NEW_ALIGNMENT__ or use the overloaded alloc with
  // align_val_t arg We avoid using std::max_align_t here as it's 16 on some
  // implementations and thus may waste memory
  static const size_t defaultAlign = 8;

  // Functions
  static size_t DefaultAlign(size_t addr) {
    return (addr + (defaultAlign - 1)) & ~(defaultAlign - 1);
  }

  static size_t AlignAddr(size_t addr, size_t al) {
    vASSERT((al & (al - 1)) == 0);
    return (addr + (al - 1)) & ~(al - 1);
  }
  static size_t GetArenaSize(size_t dataSize) {
    return DefaultAlign(sizeof(ArenaHeader)) + dataSize;
  }

  unsigned char *GetArenaData() const {
    vASSERT(DefaultAlign(size_t(this)) == size_t(this));
    return (unsigned char *)(DefaultAlign(size_t(this) + sizeof(ArenaHeader)));
  }

  void *operator new(size_t, unsigned char *memory) { return memory; }

  void operator delete(void *, unsigned char *) {
    // Do nothing
  }

private:
  ArenaHeader(size_t dataSize, ArenaHeader *nextArena)
      : _nextArena(0), size(dataSize) {
    _nextByte = GetArenaData();
    _lastByte = _nextByte + dataSize;
    vASSERT(((unsigned char *)(this) + GetArenaSize(dataSize)) == _lastByte);
  }

  ~ArenaHeader() {
    _nextByte = _lastByte = 0;
    _nextArena = 0;
  }

  ArenaHeader(const ArenaHeader&) = delete;
  ArenaHeader& operator=(const ArenaHeader&) = delete;

  void *AllocSpace(size_t size, size_t align);

  // Data

  ArenaHeader *_nextArena;  // Word aligned
  unsigned char *_nextByte; // Char aligned
  unsigned char *_lastByte; // Char aligned
  size_t size;
};

class ArenaManager {
  friend class Mem_Manager;

private:
  // Functions

  ArenaManager(size_t defaultArenaSize)
      : _arenas(0), _defaultArenaSize(defaultArenaSize) {
    CreateArena(_defaultArenaSize);
  }

  ~ArenaManager() { FreeArenas(); }
  ArenaManager(const ArenaManager&) = delete;
  ArenaManager& operator=(const ArenaManager&) = delete;

  void *AllocDataSpace(size_t size, size_t al) {
    // Do separate memory allocations of debugMemAlloc is set, to allow
    // valgrind/drmemory to find more buffer over-reads/writes
#if !defined(NDEBUG) && defined(vISA_DEBUG_MEM_ALLOC)
    return size == 0 ? 0 : malloc(size);
#endif
    void *space = nullptr;

    if (size) {
      space = _arenas->AllocSpace(size, al);

      if (space == 0) {
        CreateArena(size);
        space = _arenas->AllocSpace(size, al);
      }

      vASSERT(space);
    }

#ifdef COLLECT_ALLOCATION_STATS
    numAllocations++;
    totalAllocSize += size;
#endif

    return space;
  }

  ArenaHeader *CreateArena(size_t size) {
    size_t arenaDataSize =
        (size > _defaultArenaSize) ? size : _defaultArenaSize;
    arenaDataSize = ArenaHeader::DefaultAlign(arenaDataSize);
    unsigned char *arena =
        new unsigned char[ArenaHeader::GetArenaSize(arenaDataSize)];

    ArenaHeader *newArena = new (arena) ArenaHeader(arenaDataSize, _arenas);
    // Add new arena to the head of queue
    if (_arenas != NULL) {
      newArena->_nextArena = _arenas;
    }

    _arenas = newArena;

#ifdef COLLECT_ALLOCATION_STATS
    numMallocCalls++;
    totalMallocSize += arenaDataSize;
    currentMallocSize += arenaDataSize;
    int numArenas = 0;
    for (ArenaHeader *tmpArena = _arenas; tmpArena != NULL;
         tmpArena = tmpArena->_nextArena) {
      numArenas++;
    }
    if (numArenas > maxArenaLength) {
      maxArenaLength = numArenas;
    }
    if (numArenas == 1) {
      numMemManagers++;
    }
#endif

    return _arenas;
  }

  void FreeArenas();

  // Data

  ArenaHeader *_arenas;
  const size_t _defaultArenaSize;
};
} // namespace vISA
#endif
