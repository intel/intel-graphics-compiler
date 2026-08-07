/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

namespace BdpasPackedScaleLayout {

// A SIMD16 packed region occupies one 64-byte GRF on Xe3P. Each A component
// consumes 8 bytes, so one region can hold four logical A scales. Each B
// component consumes 16 bytes, so one region can hold two logical B scales:
//
//   byte       0                16               32               48       64
//   A          | scales 0/1 low | scales 2/3 low | scales 0/1 high| scales 2/3 high
//   B          | scale 0 low    | scale 1 low    | scale 0 high   | scale 1 high
//              +----------------+----------------+----------------+----------------+
//
// BDPAS reads each scale's high component HighComponentStride bytes after its
// low component. TileScaleBytes is the byte distance between corresponding
// components of the two scales combined by automatic 2x2 reconstruction.
//
// IGC represents a varying SIMD16 <2 x i8> value passed to
// GenISA_sub_group_bdpas in its normal component-major layout: 16
// low-component bytes followed immediately by 16 high-component bytes.
// NormalComponentStride describes this source layout. The ordinary lowering
// copies these blocks into the HighComponentStride layout required by BDPAS;
// the packed region already has the required spacing.
//
// BDPAS reads 8 bytes from each A component and 16 bytes from each B component:
//
//   scale 0 A: [0,8)   and [32,40)   scale 0 B: [0,16)  and [32,48)
//   scale 1 A: [8,16)  and [40,48)   scale 1 B: [16,32) and [48,64)
//   scale 2 A: [16,24) and [48,56)
//   scale 3 A: [24,32) and [56,64)
//
constexpr unsigned AComponentBytes = 8;
constexpr unsigned BComponentBytes = 16;
constexpr unsigned TileScaleBytes = 16;
constexpr unsigned NormalComponentStride = TileScaleBytes;
constexpr unsigned HighComponentStride = 32;
constexpr unsigned PackedRegionBytes = 4 * TileScaleBytes;
constexpr unsigned AReadExtent = HighComponentStride + AComponentBytes;
constexpr unsigned BReadExtent = HighComponentStride + BComponentBytes;
constexpr unsigned MaxAOffset = PackedRegionBytes - AReadExtent;
constexpr unsigned MaxBOffset = PackedRegionBytes - BReadExtent;

} // namespace BdpasPackedScaleLayout

llvm::FunctionPass *createBdpasScaleCoalescingPass();

} // namespace IGC
