/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// An arena based memory manager implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#include "Mem_Manager.h"
using namespace vISA;
Mem_Manager::Mem_Manager(size_t defaultArenaSize)
    : _arenaManager(defaultArenaSize) {}

Mem_Manager::~Mem_Manager() {}
