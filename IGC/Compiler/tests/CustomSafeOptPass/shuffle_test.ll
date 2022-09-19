;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s

define spir_func void @test(i32 addrspace(1)* %buffer, i32 %value) {
entry:
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  ; CHECK-NOT:     xor
  %0 = xor i16 %simdLaneId16, 1
  ; CHECK:         [[ZEXT:%[a-zA-Z0-9]+]] = zext i16 %simdLaneId16 to i32
  ; CHECK:         [[SIMDSIZE:%[a-zA-Z0-9]+]] = call i32 @llvm.genx.GenISA.simdSize()
  ; CHECK:         [[SIMDLANEADD:%[a-zA-Z0-9]+]] = add i32 [[ZEXT]], 1
  ; CHECK:         [[ISODD:%[a-zA-Z0-9]+]] = trunc i32 [[SIMDLANEADD]] to i1
  ; CHECK:         [[SUB:%[a-zA-Z0-9]+]] = sub i32 [[SIMDSIZE]], 1
  %xor.i = zext i16 %0 to i32
  ; CHECK:         [[SHUFFLEDOWN:%[a-zA-Z0-9]+]] = call i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32 %value, i32 %value, i32 1)
  ; CHECK:         [[SHUFFLEUP:%[a-zA-Z0-9]+]] = call i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32 %value, i32 %value, i32 [[SUB]])
  ; CHECK:         [[SEL:%[a-zA-Z0-9]+]] = select i1 [[ISODD]], i32 [[SHUFFLEDOWN]], i32 [[SHUFFLEUP]]
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %value, i32 %xor.i, i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1
  ; CHECK:         store i32 [[SEL]], i32 addrspace(1)* %arrayidx, align 4
  store i32 %simdShuffle, i32 addrspace(1)* %arrayidx, align 4
  ret void
}


; CHECK:         declare i32 @llvm.genx.GenISA.simdSize()
; CHECK:         declare i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32, i32, i32)

declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
