;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare !internal_intrinsic_id !20 <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <1 x float>) #1
declare !internal_intrinsic_id !21 void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v1f32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <1 x float>) #2

define dllexport spir_kernel void @test(float addrspace(1)* nocapture align 4 %ptr) local_unnamed_addr #0 {
  %p2i.0 = ptrtoint float addrspace(1)* %ptr to i64
  %src0 = call <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %p2i.0, i16 1, i32 0, <1 x float> undef)
  %A = bitcast <1 x float> %src0 to float
  %p2i.1 = add i64 %p2i.0, 4
  %src1 = call <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %p2i.1, i16 1, i32 0, <1 x float> undef)
  %B = bitcast <1 x float> %src1 to float
  %cmp = fcmp ole float %A, %B
  br i1 %cmp, label %if, label %else

; CHECK-LABEL: if:
; CHECK: %A.inv = call float @llvm.genx.inv.f32(float %A)
; CHECK: [[DIV_T:%[^ ]+]] = fmul reassoc nsz arcp contract float %B, %A.inv
if:
  %div.t = fdiv reassoc nsz arcp contract float %B, %A
  br label %end

; CHECK-LABEL: else:
; CHECK: %B.inv = call float @llvm.genx.inv.f32(float %B)
; CHECK: [[DIV_F:%[^ ]+]] = fmul reassoc nsz arcp contract float %A, %B.inv
; CHECK-NOT: fmul reassoc nsz arcp contract float %A, %A.inv
else:
  %div.f = fdiv reassoc nsz arcp contract float %A, %B
  br label %end

; CHECK-LABEL: end:
; %res = phi float [ [[DIV_T]], %if ], [ [[DIV_F]], %else ]
end:
  %res = phi float [ %div.t, %if ], [ %div.f, %else ]
  %res.v = bitcast float %res to <1 x float>
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v1f32(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %p2i.0, i16 1, i32 0, <1 x float> %res.v)
  ret void
}

attributes #0 = { mustprogress nofree nosync nounwind willreturn "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind writeonly }

!genx.kernels = !{!7}
!genx.kernel.internal = !{!14}

!7 = !{void (float addrspace(1)*)* @test, !"test", !8, i32 0, !9, !10, !11, i32 0}
!8 = !{i32 0}
!9 = !{i32 64}
!10 = !{i32 0}
!11 = !{!"svmptr_t"}
!14 = !{void (float addrspace(1)*)* @test, !15, !16, !18, !17}
!15 = !{i32 0}
!16 = !{i32 0}
!17 = !{i32 255}
!18 = !{}
!20 = !{i32 11206}
!21 = !{i32 11224}
