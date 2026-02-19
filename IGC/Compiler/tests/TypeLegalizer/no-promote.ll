;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --enable-debugify -igc-type-legalizer -S < %s 2>&1 | FileCheck %s

; Test checks legal cases for several unsupported instructions and cases that ignore
; illegal operands/return value

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; Note: Illegal imm will result in assert as not supported, should follow Elementize routine
define void @test_aggregate(i8 %src) {
; CHECK-LABEL: @test_aggregate(
; CHECK:    [[TMP1:%.*]] = insertvalue [2 x i8] c"\0D\12", i8 [[SRC:%.*]], 0
; CHECK:    [[TMP2:%.*]] = extractvalue [2 x i8] [[TMP1]], 0
; CHECK:    call void @use.i8(i8 [[TMP2]])
; CHECK:    ret void
;
  %1 = insertvalue [2 x i8] [i8 13, i8 18], i8 %src, 0
  %2 = extractvalue [2 x i8] %1, 0
  call void @use.i8(i8 %2)
  ret void
}

; Note: llvm does required checkers for pow2 and larger then byte access for atomics, but large ints are currently not supported
define void @test_atomic_cmp(i128* %src) {
; CHECK-LABEL: @test_atomic_cmp(
; CHECK:    [[TMP1:%.*]] = cmpxchg i128* [[SRC:%.*]], i128 13, i128 14 acq_rel monotonic
; CHECK:    [[TMP2:%.*]] = extractvalue { i128, i1 } [[TMP1]], 0
; CHECK:    call void @use.i128(i128 [[TMP2]])
; CHECK:    ret void
;
  %1 = cmpxchg i128* %src, i128 13, i128 14 acq_rel monotonic
  %2 = extractvalue {i128, i1} %1, 0
  call void @use.i128(i128 %2)
  ret void
}

; Note: llvm does required checkers for pow2 and larger then byte access for atomics, but large ints are currently not supported
define void @test_atomic_rmw(i128* %src) {
; CHECK-LABEL: @test_atomic_rmw(
; CHECK:    [[TMP1:%.*]] = atomicrmw add i128* [[SRC:%.*]], i128 1 acquire
; CHECK:    call void @use.i128(i128 [[TMP1]])
; CHECK:    ret void
;
  %1 = atomicrmw add i128* %src, i128 1 acquire
  call void @use.i128(i128 %1)
  ret void
}

; Note: no type, always legal
define void @test_fence() {
; CHECK-LABEL: @test_fence(
; CHECK:    fence acquire
; CHECK:    ret void
;
  fence acquire
  ret void
}

; Note: GenX intrinsics are expected to be legal, no check is present
define void @test_gen_intinsic() {
; CHECK-LABEL: @test_gen_intinsic(
; CHECK:    [[TMP1:%.*]] = call i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4 3, i32 11, i32 13)
; CHECK:    [[TMP2:%.*]] = sext i4 [[TMP1]] to i8
; CHECK:    call void @use.i8(i8 [[TMP2]])
; CHECK:    ret void
;
  %1 = call i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4 3, i32 11, i32 13)
  %2 = sext i4 %1 to i8
  call void @use.i8(i8 %2)
  ret void
}
declare i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4, i32, i32)

; Note: Landing pad is not supported and treated as legal
define void @test_landingpad(i8* %src) personality i8* null {
; CHECK-LABEL: @test_landingpad(
; CHECK:    invoke void @foo()
; CHECK:    to label [[EXIT:%.*]] unwind label [[LPAD:%.*]]
; CHECK:       lpad:
; CHECK:    [[TMP1:%.*]] = landingpad { i8*, i4 }
; CHECK:    cleanup
; CHECK:    ret void
; CHECK:       exit:
; CHECK:    ret void
;
  invoke void @foo() to label %exit unwind label %lpad
lpad:
  %1 = landingpad { i8*, i4 } cleanup
  ret void
exit:
  ret void
}
declare void @foo()

; Note: Shuffle vector is not supported, illegal cases result in assert
define void @test_shuffle() {
; CHECK-LABEL: @test_shuffle(
; CHECK:    [[TMP1:%.*]] = shufflevector <3 x i8> <i8 1, i8 2, i8 3>, <3 x i8> <i8 4, i8 5, i8 6>, <2 x i32> <i32 0, i32 1>
; CHECK:    ret void
;
  %1 = shufflevector <3 x i8> <i8 1, i8 2, i8 3>, <3 x i8> <i8 4, i8 5, i8 6>, <2 x i32> <i32 0, i32 1>
  ret void
}

; Note: memtransfer intrinsic - always legal
define void @test_memcpy(i4* %src, i4* %dst) {
; CHECK-LABEL: @test_memcpy(
; CHECK:    call void @llvm.memcpy.p0i4.p0i4.i32(i4* [[SRC:%.*]], i4* [[DST:%.*]], i32 10, i1 false)
; CHECK:    ret void
;
  call void @llvm.memcpy.p0i4.p0i4.i32(i4* %src, i4* %dst, i32 10, i1 0)
  ret void
}
declare void @llvm.memcpy.p0i4.p0i4.i32(i4*, i4*, i32, i1)

; Note: mem intrinsic - always legal
define void @test_memset(i4* %dst) {
; CHECK-LABEL: @test_memset(
; CHECK:    call void @llvm.memset.p0i4.i32(i4* [[DST:%.*]], i8 3, i32 10, i1 false)
; CHECK:    ret void
;
  call void @llvm.memset.p0i4.i32(i4* %dst, i8 3, i32 10, i1 0)
  ret void
}
declare void @llvm.memset.p0i4.i32(i4*, i8, i32, i1)

; Note: VA intrinsics always legal, and have a checker in llvm
define void @test_va(i8* %src, i8* %dst) {
; CHECK-LABEL: @test_va(
; CHECK:    call void @llvm.va_start(i8* [[SRC:%.*]])
; CHECK:    call void @llvm.va_copy(i8* [[DST:%.*]], i8* [[SRC]])
; CHECK:    call void @llvm.va_end(i8* [[DST]])
; CHECK:    ret void
;
  call void @llvm.va_start(i8* %src)
  call void @llvm.va_copy(i8* %dst, i8* %src)
  call void @llvm.va_end(i8* %dst)
  ret void
}
declare void @llvm.va_start(i8*)
declare void @llvm.va_copy(i8*, i8*)
declare void @llvm.va_end(i8*)

declare void @use.i8(i8)
declare void @use.i128(i128)
