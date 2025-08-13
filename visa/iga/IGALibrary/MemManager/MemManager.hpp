/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// An arena based memory manager implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#ifndef _MemManager_HPP_
#define _MemManager_HPP_

#include "Arena.hpp"
#include <cstring>

namespace iga {
class MemManager {
public:
  MemManager(size_t defaultArenaSize);
  ~MemManager();

  MemManager(const MemManager &) = delete;
  MemManager &operator=(const MemManager &) = delete;

  void *alloc(size_t size) { return _arenaManager.AllocDataSpace(size); }

private:
  ArenaManager _arenaManager;
};
} // namespace iga
#endif
