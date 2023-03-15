/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

// === --------------------------------------------------------------------===
// === Bitfield accessors
// === --------------------------------------------------------------------===
ushort __getBits16(ushort value, uint startBit, uint width);
uint __getBits32(uint value, uint startBit, uint width);
ulong __getBits64(ulong value, uint startBit, uint width);

// === --------------------------------------------------------------------===
// === Bitfield setters
// === --------------------------------------------------------------------===
ushort __setBits16(ushort value, ushort slot, uint startBit, uint width);
uint __setBits32(uint value, uint slot, uint startBit, uint width);
ulong __setBits64(ulong value, ulong slot, uint startBit, uint width);

// === --------------------------------------------------------------------===
// === Helper functions
// === --------------------------------------------------------------------===
global void* __getImplicitDispatchGlobals();
