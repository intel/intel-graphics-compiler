;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt -igc-SelectCSWalkOrder --inputcs -regkey EnableSelectCSWalkOrderPass -S < %s 2>&1 | FileCheck %s
;
; RUN: igc_opt -igc-SelectCSWalkOrder --inputcs -regkey EnableSelectCSWalkOrderPass,OverrideHWGenerateLID=1 -S < %s 2>&1 | FileCheck %s
;
; RUN: igc_opt -igc-SelectCSWalkOrder --inputcs -regkey EnableSelectCSWalkOrderPass,OverrideHWGenerateLID=0 -S < %s 2>&1 | FileCheck %s

; ------------------------------------------------
; SelectCSWalkOrder
; ------------------------------------------------

%__2D_DIM_Resource = type opaque
%dx.types.Handle = type { i8* }
@ThreadGroupSize_X = constant i32 8
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define void @main(<8 x i32> %r0) {
; CHECK:    [[TMP1:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
; CHECK:    %GroupID_X = bitcast float [[TMP1]] to i32
; CHECK:    [[TMP2:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
; CHECK:    %GroupID_Y = bitcast float [[TMP2]] to i32
; CHECK:    [[TMP3:%.*]] = shl i32 %GroupID_X, 4
; CHECK:    [[TMP4:%.*]] = shl i32 %GroupID_Y, 4
; CHECK:    %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
; CHECK:    %ThreadID_Y = add i32 [[TMP4]], %LocalID_Y

  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %1 to i32
  %2 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
  %GroupID_Y = bitcast float %2 to i32
  %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %u0 = inttoptr i32 %3 to %__2D_DIM_Resource addrspace(2490368)*
  %4 = shl i32 %GroupID_X, 4
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ThreadID_X = add i32 %4, %LocalID_X
  br label %5

5:                                                ; preds = %5, %0
  %6 = phi i32 [ 0, %0 ], [ %9, %5 ]
  %7 = load i32, i32 addrspace(3)* null, align 2147483648, !tbaa !390
  %8 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* nonnull inttoptr (i32 4 to i32 addrspace(3)*), i32 addrspace(3)* nonnull inttoptr (i32 4 to i32 addrspace(3)*), i32 %7, i32 0)
  store i32 %8, i32 addrspace(3)* inttoptr (i32 8 to i32 addrspace(3)*), align 8, !tbaa !390
  %9 = add nuw nsw i32 %6, 1
  %10 = icmp eq i32 %9, 10
  br i1 %10, label %11, label %5

11:                                               ; preds = %5
  %12 = shl i32 %GroupID_Y, 4
  %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
  %ThreadID_Y = add i32 %12, %LocalID_Y
  %13 = load i32, i32 addrspace(3)* inttoptr (i32 16 to i32 addrspace(3)*), align 16, !tbaa !390
  %14 = bitcast i32 %13 to float
  %15 = bitcast i32 %13 to float
  %16 = bitcast i32 %13 to float
  %17 = bitcast i32 %13 to float
  call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %u0, i32 %ThreadID_X, i32 %ThreadID_Y, i32 0, i32 0, float %14, float %15, float %16, float %17)
  ret void
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #0
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)*, i32 addrspace(3)*, i32, i32) #3
declare void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)*, i32, i32, i32, i32, float, float, float, float) #4
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #0
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

!390 = !{!391, !391, i64 0}
!391 = !{!"int", !392, i64 0}
!392 = !{!"omnipotent char", !393, i64 0}
!393 = !{!"Simple C/C++ TBAA"}
