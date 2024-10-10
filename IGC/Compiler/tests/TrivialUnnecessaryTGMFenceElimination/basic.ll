;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -TrivialUnnecessaryTGMFenceElimination -S < %s | FileCheck %s
; ------------------------------------------------
; TrivialUnnecessaryTGMFenceElimination
; ------------------------------------------------
; This test checks that TGM Fences with a typed write before and no typed read
; after are removed by the pass
; ------------------------------------------------

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"

%__2D_DIM_Resource = type opaque
%dx.types.Handle = type { i8* }
%dx.types.twoi32 = type { i32, i32 }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%dx.types.u32 = type { i32 }

@TGSM0 = internal addrspace(3) global [924 x i8] undef, align 4
@llvm.used = appending global [1 x i8*] [i8* addrspacecast (i8 addrspace(3)* getelementptr inbounds ([924 x i8], [924 x i8] addrspace(3)* @TGSM0, i32 0, i32 0) to i8*)], section "llvm.metadata"
@ThreadGroupSize_X = constant i32 3
@ThreadGroupSize_Y = constant i32 7
@ThreadGroupSize_Z = constant i32 11

define void @remove_tgm_fence_without_typedread(<8 x i32> %r0, i8* %privateBase) {
entry:
  %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
  %u0 = inttoptr i32 %0 to <4 x float> addrspace(2490368)*
  %bb = add i32 %0, 64
  %u1 = inttoptr i32 %bb to %__2D_DIM_Resource addrspace(2490369)*
  %1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %2 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 2)
  %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 3)
  %4 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
  %5 = bitcast i32 %4 to float
  call void @llvm.genx.GenISA.typedwrite.p2490369__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490369)* %u1, i32 %1, i32 %2, i32 %3, i32 0, float %5, float %5, float %5, float %5)
; CHECK: call void @llvm.genx.GenISA.typedwrite{{.*}}
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-NOT: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
  ret void
; CHECK-NEXT: ret void
}

define void @donot_remove_tgm_fence_with_typedread(<8 x i32> %r0, i8* %privateBase) {
entry:
  %a = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 3)
  %u00 = inttoptr i32 %a to <4 x float> addrspace(2490368)*
  %b = add i32 %a, 64
  %u01 = inttoptr i32 %b to %__2D_DIM_Resource addrspace(2490369)*
  %a8 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
  %a9 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %a10 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 2)
  %a12 = bitcast i32 %a10 to float
  %a13 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 3)
  call void @llvm.genx.GenISA.typedwrite.p2490369__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490369)* %u01, i32 %a8, i32 %a9, i32 0, i32 0, float %a12, float %a12, float %a12, float %a12)
; CHECK: call void @llvm.genx.GenISA.typedwrite{{.*}}
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
  %a17 = call fast <4 x float> @llvm.genx.GenISA.typedread.p2490369__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490369)* %u01, i32 %a13, i32 %a13, i32 0, i32 0)
; CHECK-NEXT: %a17 = call fast <4 x float> @llvm.genx.GenISA.typedread{{.*}}
  ret void
; CHECK-NEXT: ret void
}

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32) #1

; Function Attrs: argmemonly nounwind readonly
declare <4 x float> @llvm.genx.GenISA.typedread.p2490369__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490369)*, i32, i32, i32, i32) #2

; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.typedwrite.p2490369__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490369)*, i32, i32, i32, i32, float, float, float, float) #0

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #3

; Function Attrs: nounwind readnone
declare float @dx.op.bitcastI32toF32(i32, i32) #3

attributes #0 = { argmemonly nounwind writeonly }
attributes #1 = { convergent nounwind }
attributes #2 = { argmemonly nounwind readonly }
attributes #3 = { nounwind readnone }

!igc.functions = !{!0, !3}

!0 = !{void (<8 x i32>, i8*)* @donot_remove_tgm_fence_with_typedread, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (<8 x i32>, i8*)* @donot_remove_tgm_fence_with_typedread, !1}