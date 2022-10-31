/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// An arena based memory manager implementation.

#include "MemManager.hpp"
using namespace iga;
MemManager::MemManager(size_t defaultArenaSize)
    : _arenaManager(defaultArenaSize) {}

MemManager::~MemManager() {}
