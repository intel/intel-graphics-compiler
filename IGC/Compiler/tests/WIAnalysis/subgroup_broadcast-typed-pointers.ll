;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: debug

; RUN: igc_opt --igc-wi-analysis -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; WIAnalysis
; ------------------------------------------------

; This test checks optional warning message for non-uniform OpGroupBroadcast.

; LLVM IR reduced from OpenCL test kernels:

; __kernel void test_uniform(global int *dst, int src)
; {
;     dst[0] = sub_group_broadcast(src, 0);
; }

; __kernel void test_nonuniform(global int *dst, int src)
; {
;     dst[0] = sub_group_broadcast(src, get_global_id(0));
; }


; CHECK-NOT: Detected llvm.genx.GenISA.WaveBroadcast{{.*}}kernel test_uniform
; CHECK:     Detected llvm.genx.GenISA.WaveBroadcast{{.*}}kernel test_nonuniform

; Function Attrs: convergent nounwind
define spir_kernel void @test_uniform(i32 addrspace(1)* %dst, i32 %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %simdBroadcast = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %src, i32 0, i32 0)
  store i32 %simdBroadcast, i32 addrspace(1)* %dst, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_nonuniform(i32 addrspace(1)* %dst, i32 %src, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %0 = extractelement <8 x i32> %payloadHeader, i32 0
  %1 = extractelement <8 x i32> %payloadHeader, i32 1
  %2 = extractelement <8 x i32> %payloadHeader, i32 2
  %3 = extractelement <8 x i32> %payloadHeader, i32 3
  %4 = extractelement <8 x i32> %payloadHeader, i32 4
  %5 = extractelement <8 x i32> %payloadHeader, i32 5
  %6 = extractelement <8 x i32> %payloadHeader, i32 6
  %7 = extractelement <8 x i32> %payloadHeader, i32 7
  %8 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %9 = extractelement <3 x i32> %enqueuedLocalSize, i32 1
  %10 = extractelement <3 x i32> %enqueuedLocalSize, i32 2
  %11 = extractelement <8 x i32> %r0, i32 0
  %12 = extractelement <8 x i32> %r0, i32 1
  %13 = extractelement <8 x i32> %r0, i32 2
  %14 = extractelement <8 x i32> %r0, i32 3
  %15 = extractelement <8 x i32> %r0, i32 4
  %16 = extractelement <8 x i32> %r0, i32 5
  %17 = extractelement <8 x i32> %r0, i32 6
  %18 = extractelement <8 x i32> %r0, i32 7
  %mul.i.i.i = mul i32 %8, %12
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %0
  %simdBroadcast = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %src, i32 %add4.i.i.i, i32 0)
  store i32 %simdBroadcast, i32 addrspace(1)* %dst, align 4
  ret void
}

; Function Desc: Broadcasts from specific lane to all lanes. Overlaps with WaveShuffleIndex, but guarantees that lane is thread-uniform.
; Output: result
; Arg 0: value
; Arg 1: lane
; Arg 2: helperLaneMode - to match WaveShuffleIndex
; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32)

!igc.functions = !{!358, !366}

!358 = !{void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i32)* @test_uniform, !359}
!359 = !{!360, !361}
!360 = !{!"function_type", i32 0}
!361 = !{!"implicit_arg_desc", !362, !363, !364}
!362 = !{i32 0}
!363 = !{i32 1}
!364 = !{i32 15, !365}
!365 = !{!"explicit_arg_num", i32 0}
!366 = !{void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_nonuniform, !367}
!367 = !{!360, !368}
!368 = !{!"implicit_arg_desc", !362, !363, !369, !370, !371, !372, !364}
!369 = !{i32 7}
!370 = !{i32 8}
!371 = !{i32 9}
!372 = !{i32 10}
