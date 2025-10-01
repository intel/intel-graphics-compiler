; REQUIRES: regkeys, llvm-16-plus, debug

; RUN: igc_opt -S %s --opaque-pointers -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s
; RUN: igc_opt -S %s --opaque-pointers -platformpvc -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: .decl vectorized_phi v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vectorized_intrinsic v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vectorized_select v_type=G type=f num_elts=8 align=dword
; CHECK: .decl nonuniformness v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vectorized_select_non v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vectorized_select917 v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vectorized_binary1068 v_type=G type=f num_elts=128 align=wordx32

; CHECK: mov (M1_NM, 8) [[ZERO_0:V.*]](0,0)<1> 0x0:f
; CHECK: cmp.eq (M1_NM, 8) [[PRED_0:P.*]] vectorized_intrinsic(0,0)<1;1,0> [[ZERO_0]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[ZERO:V.*]](0,0)<1> 0x0:f
; CHECK: ([[PRED_0]]) sel (M1_NM, 8) vectorized_select(0,0)<1> [[ZERO]](0,0)<1;1,0> vectorized_intrinsic(0,0)<1;1,0>
; CHECK: add (M1_NM, 8) vectorized_binary712(0,0)<1> {{r0_0.*}}(0,0)<1;1,0> (-)vectorized_select(0,0)<1;1,0>

; CHECK: cmp.eq (M1_NM, 16) [[PRED_1:P.*]] scalar_argument(0,0)<0;1,0> 0x3f800000:f
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(0,0)<1> nonuniformness(0,0)<1;1,0> vectorized_intrinsic(0,0)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(1,0)<1> nonuniformness(1,0)<1;1,0> vectorized_intrinsic(0,1)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(2,0)<1> nonuniformness(2,0)<1;1,0> vectorized_intrinsic(0,2)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(3,0)<1> nonuniformness(3,0)<1;1,0> vectorized_intrinsic(0,3)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(4,0)<1> nonuniformness(4,0)<1;1,0> vectorized_intrinsic(0,4)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(5,0)<1> nonuniformness(5,0)<1;1,0> vectorized_intrinsic(0,5)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(6,0)<1> nonuniformness(6,0)<1;1,0> vectorized_intrinsic(0,6)<0;1,0>
; CHECK: ([[PRED_1]]) sel (M1, 16) vectorized_select_non(7,0)<1> nonuniformness(7,0)<1;1,0> vectorized_intrinsic(0,7)<0;1,0>

; CHECK: cmp.eq (M1_NM, 8) [[PRED:P.*]] [[ARG:.*]](0,0)<1;1,0> {{V.*}}(0,0)
; CHECK: mov (M1_NM, 8) [[ZERO_1:V.*]](0,0)<1> 0x0:f
; CHECK: mov (M1_NM, 8) [[ZERO_2:V.*]](0,0)<1> 0x0:f
; CHECK: ([[PRED]]) sel (M1_NM, 8) vectorized_select917(0,0)<1> [[ZERO_1]](0,0)<1;1,0> [[ZERO_2]](0,0)<1;1,0>

; CHECK: div (M1, 16) vectorized_binary1068(0,0)<1> vectorized_select_non(0,0)<1;1,0> vectorized_select917(0,0)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(1,0)<1> vectorized_select_non(1,0)<1;1,0> vectorized_select917(0,1)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(2,0)<1> vectorized_select_non(2,0)<1;1,0> vectorized_select917(0,2)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(3,0)<1> vectorized_select_non(3,0)<1;1,0> vectorized_select917(0,3)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(4,0)<1> vectorized_select_non(4,0)<1;1,0> vectorized_select917(0,4)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(5,0)<1> vectorized_select_non(5,0)<1;1,0> vectorized_select917(0,5)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(6,0)<1> vectorized_select_non(6,0)<1;1,0> vectorized_select917(0,6)<0;1,0>
; CHECK: div (M1, 16) vectorized_binary1068(7,0)<1> vectorized_select_non(7,0)<1;1,0> vectorized_select917(0,7)<0;1,0>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(float %scalar_argument, <8 x float> %vector651, <8 x float> %vectorized_binary612, <8 x float> %vectorized_phi1059) {
entry:

  br label %._crit_edge575

._crit_edge575:                                   ; preds = %._crit_edge575, %.lr.ph89
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %entry ], [ %vectorized_intrinsic721, %._crit_edge575 ]
  %vectorized_intrinsic = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_phi, <8 x float> %vector651)
  %vectorized_cmp = fcmp oeq <8 x float> %vectorized_intrinsic, zeroinitializer
  %vectorized_select = select <8 x i1> %vectorized_cmp, <8 x float> zeroinitializer, <8 x float> %vectorized_intrinsic
  %vectorized_binary712 = fsub <8 x float> %vectorized_binary612, %vectorized_select
  %vectorized_intrinsic721 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %vectorized_binary712)
  %nonuniformness = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_intrinsic721, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %flag = fcmp oeq float %scalar_argument, 1.0
  %vectorized_select_non = select i1 %flag, <8 x float> %nonuniformness, <8 x float> %vectorized_intrinsic
  %br = icmp eq i32 0, 0
  br i1 %br, label %._crit_edge90, label %._crit_edge575

._crit_edge90:                                    ; preds = %._crit_edge575, %._crit_edge
  %vectorized_cmp908 = fcmp oeq <8 x float> %vectorized_phi1059, zeroinitializer
  %vectorized_select917 = select <8 x i1> %vectorized_cmp908, <8 x float> zeroinitializer, <8 x float> zeroinitializer
  %vectorized_binary1068 = fdiv <8 x float> %vectorized_select_non, %vectorized_select917
  %vectorized_cast1077 = fptrunc <8 x float> %vectorized_binary1068 to <8 x half>
  %.assembled.vect567 = bitcast <8 x half> %vectorized_cast1077 to <8 x i16>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 16, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i16> %.assembled.vect567)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>)

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2, !3, !6, !7}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5}
!4 = !{i32 0}
!5 = !{i32 2}
!6 = !{!"sub_group_size", i32 16}
!7 = !{!"max_reg_pressure", i32 151}
