;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

define spir_kernel void @testYZUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = extractelement <3 x i32> %localSize, i64 0
  %1 = extractelement <3 x i32> %localSize, i64 1
  %localIdZ2 = zext i16 %localIdZ to i32
  %mul.i.i = mul i32 %1, %localIdZ2
  %localIdY4 = zext i16 %localIdY to i32
  %add.i.i = add i32 %mul.i.i, %localIdY4
  %mul4.i.i = mul i32 %0, %add.i.i
  %localIdX6 = zext i16 %localIdX to i32
  %add6.i.i = add i32 %mul4.i.i, %localIdX6
  %conv.i = zext i32 %add6.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %conv.i
  %2 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4
  ret void
}

define spir_kernel void @testNoUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = extractelement <3 x i32> %localSize, i64 0
  %1 = extractelement <3 x i32> %localSize, i64 1
  %localIdZ2 = zext i16 %localIdZ to i32
  %mul.i.i = mul i32 %1, %localIdZ2
  %localIdY4 = zext i16 %localIdY to i32
  %add.i.i = add i32 %mul.i.i, %localIdY4
  %mul4.i.i = mul i32 %0, %add.i.i
  %localIdX6 = zext i16 %localIdX to i32
  %add6.i.i = add i32 %mul4.i.i, %localIdX6
  %conv.i = zext i32 %add6.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %conv.i
  %2 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4
  ret void
}

define spir_kernel void @testZUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = extractelement <3 x i32> %localSize, i64 0
  %1 = extractelement <3 x i32> %localSize, i64 1
  %localIdZ2 = zext i16 %localIdZ to i32
  %mul.i.i = mul i32 %1, %localIdZ2
  %localIdY4 = zext i16 %localIdY to i32
  %add.i.i = add i32 %mul.i.i, %localIdY4
  %mul4.i.i = mul i32 %0, %add.i.i
  %localIdX6 = zext i16 %localIdX to i32
  %add6.i.i = add i32 %mul4.i.i, %localIdX6
  %conv.i = zext i32 %add6.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %conv.i
  %2 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4
  ret void
}

;CHECK: block: entry function: testYZUnif
;CHECK: U: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: U: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

;CHECK: block: entry function: testNoUnif
;CHECK: N: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: N: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

;CHECK: block: entry function: testZUnif
;CHECK: U: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: N: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

!igc.functions = !{!1, !2, !3}
!IGCMetadata = !{!19}

!1 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif, !41}
!2 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif, !42}
!3 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testZUnif, !43}
!41 = !{!5, !6, !17}
!42 = !{!5, !6}
!43 = !{!5, !6, !18}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !15}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 6}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 1}
!17 = !{!"thread_group_size", i32 32, i32 32, i32 32}
!18 = !{!"thread_group_size", i32 16, i32 32, i32 32}
!19 = !{!"ModuleMD", !112}
!112 = !{!"FuncMD", !113, !114, !333, !334, !444, !445}
!113 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif}
!114 = !{!"FuncMDValue[0]", !116}
!333 = !{!"FuncMDMap[1]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif}
!334 = !{!"FuncMDValue[1]", !116}
!444 = !{!"FuncMDMap[2]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testZUnif}
!445 = !{!"FuncMDValue[2]", !116}
!116 = !{!"workGroupWalkOrder", !117, !118, !119}
!117 = !{!"dim0", i32 0}
!118 = !{!"dim1", i32 1}
!119 = !{!"dim2", i32 2}
