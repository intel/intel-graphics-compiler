/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "GenXRegionUtils.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include "gtest/gtest.h"

using namespace llvm;

namespace {
TEST(GenXCodeGen, RegionOverlapping) {
  LLVMContext Context;

  genx::Region R1(
      IGCLLVM::FixedVectorType::get(Type::getDoubleTy(Context), 16));
  R1.VStride = 0;
  R1.NumElements = R1.Width = 16;
  R1.Stride = 1;
  R1.Offset = 128;
  genx::Region R2(IGCLLVM::FixedVectorType::get(Type::getDoubleTy(Context), 8));
  R2.VStride = 0;
  R2.NumElements = R2.Width = 8;
  R2.Stride = 1;
  R2.Offset = 192;
  EXPECT_EQ(R1.overlap(R2), true);
  R2.Offset = 256;
  EXPECT_EQ(R2.overlap(R1), false);

  genx::Region R3(IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Context), 4));
  R3.VStride = 2;
  R3.NumElements = 8;
  R3.Width = 1;
  R3.Stride = 0;
  R3.Offset = 0;
  genx::Region R4(R3);
  EXPECT_EQ(R3.overlap(R4), true);
  R4.Offset = R4.ElementBytes;
  EXPECT_EQ(R3.overlap(R4), false);
  R4.Offset = R4.ElementBytes * 2;
  EXPECT_EQ(R3.overlap(R4), true);
  R4.Offset = 6;
  EXPECT_EQ(R3.overlap(R4), true);

  genx::Region R5(IGCLLVM::FixedVectorType::get(Type::getInt16Ty(Context), 4));
  R5.VStride = 8;
  R5.NumElements = 4;
  R5.Width = 2;
  R5.Stride = 1;
  R5.Offset = 0;
  genx::Region R6(R5);
  R6.Offset = R6.ElementBytes;
  EXPECT_EQ(R5.overlap(R6), true);
  R6.Offset = R6.ElementBytes * 2;
  EXPECT_EQ(R5.overlap(R6), false);

  genx::Region R7(
      IGCLLVM::FixedVectorType::get(Type::getDoubleTy(Context), 128));
  R7.VStride = 32;
  R7.NumElements = 128;
  R7.Width = 8;
  R7.Stride = 2;
  R7.Offset = 0;
  genx::Region R8(
      IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Context), 256));
  R8.VStride = 1;
  R8.Width = R8.NumElements = 128;
  R8.Stride = 4;
  R8.Offset = R7.ElementBytes;
  EXPECT_EQ(R7.overlap(R8), false);
  R8.Offset--;
  EXPECT_EQ(R7.overlap(R8), true);
}

} // namespace
