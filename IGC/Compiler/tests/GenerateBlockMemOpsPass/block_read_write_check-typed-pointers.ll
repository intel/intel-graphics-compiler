;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;

; RUN: igc_opt %s -S -o - -generate-block-mem-ops -platformpvc | FileCheck %s

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

  ; CHECK: [[TMP0:%.*]] = call float @llvm.genx.GenISA.simdBlockRead.f32.p1f32(float addrspace(1)* %arrayidx) [[ATTR_NUM:#.*]]

  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4

  ; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1f32.f32(float addrspace(1)* %arrayidx1, float [[TMP0]]) [[ATTR_NUM]]

  ret void
}

; IGC cannot turn load/store statements into block statements for the testNoUnif function, since these instructions
; work with non-contiguous memory relative to the subgroup. See thread_group_size metadata below.

define spir_kernel void @testNoUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {

; CHECK-LABEL: @testNoUnif(

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

  ; CHECK-NOT: %{{.*}} = simdBlockRead

  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4

  ; CHECK-NOT: simdBlockWrite

  ret void

; CHECK: ret void

}

define spir_kernel void @testYZUnifLoop(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i64 %limit) {
; CHECK: %{{.*}} = load
; CHECK: store
; CHECK: [[TMP0:%.*]] = call float @llvm.genx.GenISA.simdBlockRead.f32.p1f32(float addrspace(1)* %{{.*}}) [[ATTR_NUM]]
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1f32.f32(float addrspace(1)* %{{.*}}, float [[TMP0]]) [[ATTR_NUM]]
entry:
  %offset = extractelement <8 x i32> %payloadHeader, i64 0
  %groupNumX = extractelement <8 x i32> %r0, i64 1
  %shl = shl i32 %groupNumX, 5
  %localIdX31 = zext i16 %localIdX to i32
  %globalIdX = add i32 %shl, %localIdX31
  %sum = add i32 %globalIdX, %offset
  %sum64 = zext i32 %sum to i64
  %precond = icmp slt i64 %sum64, %limit
  br i1 %precond, label %preheader, label %terminator
preheader:
  br label %latch
latch:
  %ind = phi i64 [ %sum64, %preheader ], [ %incr, %latch ]
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %ind
  %load = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %ind
  store float %load, float addrspace(1)* %arrayidx1, align 4
  %incr = add nsw i64 %ind, 32
  %cond = icmp slt i64 %incr, %limit
  br i1 %cond, label %latch, label %exit
exit:
  br label %terminator
terminator:
  ret void
}

; CHECK: attributes #2 = { "alignmentrequirements"="4" }

!igc.functions = !{!1, !2, !3}
!IGCMetadata = !{!19}

!1 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif, !41}
!2 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif, !42}
!3 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i64)* @testYZUnifLoop, !43}
!41 = !{!5, !6, !17}
!42 = !{!5, !6}
!43 = !{!5, !6, !17}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !15}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 5}
!10 = !{i32 7}
!11 = !{i32 8}
!12 = !{i32 9}
!13 = !{i32 14, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{i32 14, !16}
!16 = !{!"explicit_arg_num", i32 1}

; This metadata provides information about the size of the work group.
; The IGC can generate block memory instructions only if data access is contiguous across the workgroup.
; This requires that the workgroup be completely vectorized along the x-axis, in other words local_size_x % 32 == 0 (case !17).

!17 = !{!"thread_group_size", i32 32, i32 32, i32 32}

; IGC cannot apply the optimization in the !18 case because local_size_x % 32 != 0.

!18 = !{!"thread_group_size", i32 16, i32 32, i32 32}
!19 = !{!"ModuleMD", !112}
!112 = !{!"FuncMD", !113, !114, !333, !334, !335, !336}
!113 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif}
!114 = !{!"FuncMDValue[0]", !116}
!333 = !{!"FuncMDMap[1]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif}
!334 = !{!"FuncMDValue[1]", !116}
!335 = !{!"FuncMDMap[2]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i64)* @testYZUnifLoop}
!336 = !{!"FuncMDValue[2]", !116}
!116 = !{!"workGroupWalkOrder", !117, !118, !119}
!117 = !{!"dim0", i32 0}
!118 = !{!"dim1", i32 1}
!119 = !{!"dim2", i32 2}
