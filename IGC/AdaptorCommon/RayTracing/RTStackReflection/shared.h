/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

struct AddrspaceInfo {
  uint32_t AS;
  bool Constant;
  constexpr AddrspaceInfo(uint32_t AS, bool Constant) : AS(AS), Constant(Constant) {}
};

constexpr AddrspaceInfo ReservedAS[] = {
    // clang-format off
    AddrspaceInfo{ 100, false },
    AddrspaceInfo{ 101, true  },
    AddrspaceInfo{ 102, false },
    AddrspaceInfo{ 103, false },
    AddrspaceInfo{ 104, false },
    // clang-format on
};
