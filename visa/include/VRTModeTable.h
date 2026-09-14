/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <array>
#include <utility>

namespace IGC {
namespace VRT {

// Variable Register Targeting (VRT) mode tables. Each entry is
// {GRF budget, number of HW threads per EU available with that budget},
// ordered by increasing budget.
using ModeEntry = std::pair<int, int>;

// Xe3 family (IGFX_XE3_CORE render core, IGFX_PTL).
inline constexpr std::array<ModeEntry, 7> ModeTableXe3 = {{
    {32, 10}, {64, 10}, {96, 10}, {128, 8}, {160, 6}, {192, 5}, {256, 4}}};

// NVL (IGFX_NVL), which extends the Xe3 table with higher budgets.
inline constexpr std::array<ModeEntry, 10> ModeTableNVL = {{
    {32, 10}, {64, 10}, {96, 10}, {128, 8}, {160, 6}, {192, 5}, {256, 4}, {320, 3}, {448, 2}, {512, 2}}};

// Xe3P family (IGFX_CRI).
inline constexpr std::array<ModeEntry, 8> ModeTableXe3P = {{
    {32, 8}, {64, 8}, {96, 8}, {128, 8}, {160, 8}, {192, 8}, {256, 4}, {512, 4}}};

} // namespace VRT
} // namespace IGC
