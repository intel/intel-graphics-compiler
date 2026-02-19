;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: %"struct::joint_matrix::C.resolved" = type { <8 x float> }
%"struct::joint_matrix::C" = type { target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) }

; CHECK: %"struct::joint_matrix::A.resolved" = type { <8 x i16> }
%"struct::joint_matrix::A" = type { target("spirv.JointMatrixINTEL", i16, 8, 16, 0, 3, 0) }

; CHECK: %"struct::joint_matrix::B.resolved" = type { <8 x i32> }
%"struct::joint_matrix::B" = type { target("spirv.JointMatrixINTEL", i16, 16, 16, 2, 3, 1) }

; CHECK: %"struct::almost_joint_matrix" = type { i64 addrspace(1)* }
%"struct::almost_joint_matrix" = type { i64 addrspace(1)* }

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: i64 [[OFFSET:%.*]], i8 addrspace(1)* [[PTR:%.*]], i8* [[PTR1:%.*]]) {
define spir_kernel void @test(i64 %offset, i8 addrspace(1)* %ptr, i8* %ptr1) {
entry:

; CHECK:         [[TC_I1:%.*]] = alloca [4 x [4 x %"struct::joint_matrix::C.resolved"]], align 8
; CHECK-NEXT:    [[TA_I3:%.*]] = alloca [4 x [2 x %"struct::joint_matrix::A.resolved"]], align 8
; CHECK-NEXT:    [[TB_I5:%.*]] = alloca [4 x [2 x %"struct::joint_matrix::B.resolved"]], align 8
; CHECK-NEXT:    [[TI_I:%.*]] = alloca [4 x [4 x %"struct::almost_joint_matrix"]], align 8
; CHECK-NEXT:    [[GROUPID:%.*]] = alloca [3 x i64], align 8
  %tC.i = alloca [4 x [4 x %"struct::joint_matrix::C"]], align 8
  %tA.i = alloca [4 x [2 x %"struct::joint_matrix::A"]], align 8
  %tB.i = alloca [4 x [2 x %"struct::joint_matrix::B"]], align 8
  %tI.i = alloca [4 x [4 x %"struct::almost_joint_matrix"]], align 8
  %GroupID = alloca [3 x i64], align 8

; Update lifetime.start size
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* [[TC_I1]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.start.p0i8(i64 512, i8* [[TMP1]])
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::A.resolved"]]* [[TA_I3]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.start.p0i8(i64 128, i8* [[TMP2]])
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::B.resolved"]]* [[TB_I5]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.start.p0i8(i64 256, i8* [[TMP3]])
  %b1 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i8*
  call void @llvm.lifetime.start.p0i8(i64 128, i8* %b1)
  %b2 = bitcast [4 x [2 x %"struct::joint_matrix::A"]]* %tA.i to i8*
  call void @llvm.lifetime.start.p0i8(i64 64, i8* %b2)
  %b3 = bitcast [4 x [2 x %"struct::joint_matrix::B"]]* %tB.i to i8*
  call void @llvm.lifetime.start.p0i8(i64 64, i8* %b3)

; Update GEP offsets coming from alloca directly
; CHECK-NEXT:    [[TMP4:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* [[TC_I1]] to i8*
; CHECK-NEXT:    [[I1:%.*]] = getelementptr inbounds i8, i8* [[TMP4]], i64 512
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::A.resolved"]]* [[TA_I3]] to i8*
; CHECK-NEXT:    [[I2:%.*]] = getelementptr inbounds i8, i8* [[TMP5]], i64 128
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::B.resolved"]]* [[TB_I5]] to i8*
; CHECK-NEXT:    [[I3:%.*]] = getelementptr inbounds i8, i8* [[TMP6]], i64 256
  %b4 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i8*
  %i1 = getelementptr inbounds i8, i8* %b4, i64 128
  %b5 = bitcast [4 x [2 x %"struct::joint_matrix::A"]]* %tA.i to i8*
  %i2 = getelementptr inbounds i8, i8* %b5, i64 64
  %b6 = bitcast [4 x [2 x %"struct::joint_matrix::B"]]* %tB.i to i8*
  %i3 = getelementptr inbounds i8, i8* %b6, i64 64

; Do not touch if offest is not a constant
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* [[TC_I1]] to i8*
; CHECK-NEXT:    [[I4:%.*]] = getelementptr inbounds i8, i8* [[TMP7]], i64 [[OFFSET]]
  %b7 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i8*
  %i4 = getelementptr inbounds i8, i8* %b7, i64 %offset

; no change: GEP operand is comming from kernel argument - not from matrix type
; CHECK-NEXT:    [[I5:%.*]] = getelementptr inbounds i8, i8* [[PTR1]], i64 128
  %i5 = getelementptr inbounds i8, i8* %ptr1, i64 128

; no change: GEP comes from bitcast that doesn't come from matrix type
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast [4 x [4 x %"struct::almost_joint_matrix"]]* [[TI_I]] to i8*
; CHECK-NEXT:    [[I6:%.*]] = getelementptr inbounds i8, i8* [[TMP8]], i64 128
  %b8 = bitcast [4 x [4 x %"struct::almost_joint_matrix"]]* %tI.i to i8*
  %i6 = getelementptr inbounds i8, i8* %b8, i64 128

; no change - GEP is not based on i8
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* [[TC_I1]] to i16*
; CHECK-NEXT:    [[ARRAYCTOR_END_I:%.*]] = getelementptr inbounds i16, i16* [[TMP9]], i64 128
  %b9 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i16*
  %arrayctor.end.i = getelementptr inbounds i16, i16* %b9, i64 128
  br label %loop_header

; Test going through phi value + check that we aren't doing ininite recursion
loop_header:
  %loop_cond = phi i1 [1, %entry], [0, %loop_header]
  %loop_matrix = phi [4 x [4 x %"struct::joint_matrix::C"]]* [%tC.i, %entry], [%loop_matrix, %loop_header]

; Update GEP offsets coming from phi value
; CHECK:         [[TMP10:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* {{.*}} to i8*
; CHECK-NEXT:    {{.*}} = getelementptr inbounds i8, i8* [[TMP10]], i64 128
  %b10 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i8*
  %i7 = getelementptr inbounds i8, i8* %b10, i64 32

  br i1 %loop_cond, label %loop_header, label %after_loop

after_loop:
; Life time end size update
; CHECK:         [[TMP11:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::B.resolved"]]* [[TB_I5]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.end.p0i8(i64 256, i8* [[TMP11]])
; CHECK-NEXT:    [[TMP12:%.*]] = bitcast [4 x [2 x %"struct::joint_matrix::A.resolved"]]* [[TA_I3]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.end.p0i8(i64 128, i8* [[TMP12]])
; CHECK-NEXT:    [[TMP13:%.*]] = bitcast [4 x [4 x %"struct::joint_matrix::C.resolved"]]* [[TC_I1]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.end.p0i8(i64 512, i8* [[TMP13]])
  %b11 = bitcast [4 x [2 x %"struct::joint_matrix::B"]]* %tB.i to i8*
  call void @llvm.lifetime.end.p0i8(i64 64, i8* %b11)
  %b12 = bitcast [4 x [2 x %"struct::joint_matrix::A"]]* %tA.i to i8*
  call void @llvm.lifetime.end.p0i8(i64 64, i8* %b12)
  %b13 = bitcast [4 x [4 x %"struct::joint_matrix::C"]]* %tC.i to i8*
  call void @llvm.lifetime.end.p0i8(i64 128, i8* %b13)

; do not touch life time intrinsics if not for Joint Matrix types
; CHECK-NEXT:    [[GROUPID_ASCAST:%.*]] = addrspacecast [3 x i64]* [[GROUPID]] to [3 x i64] addrspace(4)*
; CHECK-NEXT:    [[TMP14:%.*]] = bitcast [3 x i64]* [[GROUPID]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.start.p0i8(i64 24, i8* [[TMP14]])
; CHECK-NEXT:    [[TMP15:%.*]] = bitcast [3 x i64]* [[GROUPID]] to i8*
; CHECK-NEXT:    call void @llvm.lifetime.end.p0i8(i64 24, i8* [[TMP15]])
  %GroupID.ascast = addrspacecast [3 x i64]* %GroupID to [3 x i64] addrspace(4)*
  %b14 = bitcast [3 x i64]* %GroupID to i8*
  call void @llvm.lifetime.start.p0i8(i64 24, i8* %b14)
  %b15 = bitcast [3 x i64]* %GroupID to i8*
  call void @llvm.lifetime.end.p0i8(i64 24, i8* %b15)

; CHECK-NEXT:    ret void
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

!igc.functions = !{!0}
!0 = !{void (i64, i8 addrspace(1)*, i8*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
