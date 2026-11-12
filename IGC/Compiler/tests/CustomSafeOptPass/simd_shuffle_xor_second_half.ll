;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -platformbmg -igc-custom-safe-opt -S %s -o - | FileCheck %s --check-prefixes=CHECK,ONE-INSTANCE
; RUN: igc_opt --opaque-pointers -platformpvc -igc-custom-safe-opt -S %s -o - | FileCheck %s --check-prefixes=CHECK,ONE-INSTANCE
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-custom-safe-opt -S %s -o - | FileCheck %s --check-prefixes=CHECK,TWO-INSTANCES
; RUN: igc_opt --opaque-pointers -platformmtl -igc-custom-safe-opt -S %s -o - | FileCheck %s --check-prefixes=CHECK,TWO-INSTANCES

; Xor values 16-31 select a lane from the other SIMD16 half of a SIMD32
; subgroup. Platforms that emit SIMD32 as two SIMD16 instances keep the
; generic shuffle for them, because simdShuffleXor is emitted per instance.

declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)

define void @test_xor_15(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 15
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_xor_15
; CHECK: call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %x, i32 15)

define void @test_xor_16(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 16
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_xor_16
; ONE-INSTANCE: call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %x, i32 16)
; TWO-INSTANCES: call i32 @llvm.genx.GenISA.WaveShuffleIndex.{{.*}}(i32 %x, i32 %xor.i, i32 0)

define void @test_xor_31(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 31
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_xor_31
; ONE-INSTANCE: call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %x, i32 31)
; TWO-INSTANCES: call i32 @llvm.genx.GenISA.WaveShuffleIndex.{{.*}}(i32 %x, i32 %xor.i, i32 0)

define void @test_xor_32(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 32
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_xor_32
; CHECK: call i32 @llvm.genx.GenISA.WaveShuffleIndex.{{.*}}(i32 %x, i32 %xor.i, i32 0)
