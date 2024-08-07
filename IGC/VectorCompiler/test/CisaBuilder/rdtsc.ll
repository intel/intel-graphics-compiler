;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: .decl [[TMC_1:V[^ ]+]] v_type=G type=q num_elts=1 align=qword
; CHECK: .decl [[TMC_2:V[^ ]+]] v_type=G type=q num_elts=1 align=qword
; CHECK: .decl [[RES:V[^ ]+]] v_type=G type=d num_elts=8 align=GRF
; CHECK: .decl [[ALIAS_1:V[^ ]+]] v_type=G type=d num_elts=2 alias=<[[TMC_1]], 0>
; CHECK: .decl [[ALIAS_2:V[^ ]+]] v_type=G type=d num_elts=2 alias=<[[TMC_2]], 0>
; CHECK: .decl [[ALIAS_RES:V[^ ]+]] v_type=G type=q num_elts=4 alias=<[[RES]], 0>
; CHECK: .decl [[SURFACE:T[^ ]+]] v_type=T num_elts=1

; CHECK: mov (M1, 2) [[ALIAS_1]](0,0)<1> %tsc(0,0)<1;1,0>
; CHECK: mov (M1, 2) [[ALIAS_2]](0,0)<1> %tsc(0,0)<1;1,0>
; CHECK: mov (M1, 1) [[ALIAS_RES]](0,0)<1> [[TMC_1]](0,0)<0;1,0>
; CHECK: mov (M1, 1) [[ALIAS_RES]](0,1)<1> [[TMC_2]](0,0)<0;1,0>
; CHECK: oword_st (2) [[SURFACE]] 0x0:ud [[RES]]

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;


declare i64 @llvm.readcyclecounter() #2
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)
declare <4 x i64> @llvm.genx.wrregioni.v4i64.v1i64.i16(<4 x i64>, <1 x i64>, i32, i32, i32, i16, i32, i1)

define dllexport spir_kernel void @kernel(i32 %dst) local_unnamed_addr #1 {
  %tmc.1 = call i64 @llvm.readcyclecounter()
  %tmc.2 = call i64 @llvm.readcyclecounter()
  %tmc.1.cast = bitcast i64 %tmc.1 to <1 x i64>
  %tmc.2.cast = bitcast i64 %tmc.2 to <1 x i64>
  %wrreg.1 = tail call <4 x i64> @llvm.genx.wrregioni.v4i64.v1i64.i16(<4 x i64> undef, <1 x i64> %tmc.1.cast, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %wrreg.2 = tail call <4 x i64> @llvm.genx.wrregioni.v4i64.v1i64.i16(<4 x i64> %wrreg.1, <1 x i64> %tmc.2.cast, i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true)
  %res = bitcast <4 x i64> %wrreg.2 to <8 x i32>
  tail call void @llvm.genx.oword.st.v8i32(i32 %dst, i32 0, <8 x i32> %res)
  ret void
}

attributes #1 = { noinline nounwind "CMGenxMain" }
attributes #2 = { nofree nosync nounwind willreturn }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!1 = !{i32 0}
!4 = !{void (i32)* @kernel, !"kernel", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2}
!6 = !{i32 64}
!7 = !{!"buffer_t"}
!8 = !{void (i32)* @kernel, null, null, null, null}
