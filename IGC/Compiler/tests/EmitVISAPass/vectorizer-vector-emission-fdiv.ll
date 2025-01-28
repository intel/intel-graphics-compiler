; REQUIRES: pvc-supported, regkeys

; RUN: igc_opt -S -dce -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_binary378 v_type=G type=f num_elts=8 align=dword
; CHECK: .decl V0035 v_type=G type=f num_elts=8 align=wordx32
; CHECK: .decl vectorized_binary402 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl V0036 v_type=G type=f num_elts=8 align=wordx32

; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,0)<1> V0035(0,0)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,1)<1> V0035(0,1)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,2)<1> V0035(0,2)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,3)<1> V0035(0,3)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,4)<1> V0035(0,4)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,5)<1> V0035(0,5)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,6)<1> V0035(0,6)<0;1,0>
; CHECK:     inv (M1_NM, 1) vectorized_binary378(0,7)<1> V0035(0,7)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(0,0)<1> V0032(0,0)<1;1,0> V0036(0,0)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(1,0)<1> V0032(1,0)<1;1,0> V0036(0,1)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(2,0)<1> V0032(2,0)<1;1,0> V0036(0,2)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(3,0)<1> V0032(3,0)<1;1,0> V0036(0,3)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(4,0)<1> V0032(4,0)<1;1,0> V0036(0,4)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(5,0)<1> V0032(5,0)<1;1,0> V0036(0,5)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(6,0)<1> V0032(6,0)<1;1,0> V0036(0,6)<0;1,0>
; CHECK:     div (M1, 16) vectorized_binary402(7,0)<1> V0032(7,0)<1;1,0> V0036(0,7)<0;1,0>

define spir_kernel void @_attn_fwd(half addrspace(1)* %0, half addrspace(1)* %1, half addrspace(1)* %2, float %3, i8 addrspace(1)* %4, float addrspace(1)* %5, <8 x i32> %r0) {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %6
  %7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br i1 false, label %._crit_edge, label %8

8:                                                ; preds = %._crit_edge
  %vectorized_binary378 = fdiv <8 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, zeroinitializer
  %vectorized_binary402 = fdiv <8 x float> %7, zeroinitializer
  %9 = bitcast <8 x float> %vectorized_binary378 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %9)
  %10 = bitcast <8 x float> %vectorized_binary402 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %10)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; uselistorder directives
uselistorder void (i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)* @llvm.genx.GenISA.LSC2DBlockWrite.v8i32, { 1, 0 }

!igc.functions = !{!0}

!0 = !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32, i32)* bitcast (void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>)* @_attn_fwd to void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32, i32)*), !1}
!1 = !{!2, !3, !16}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !8, !10, !12, !14}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 14, !7}
!7 = !{!"explicit_arg_num", i32 0}
!8 = !{i32 14, !9}
!9 = !{!"explicit_arg_num", i32 1}
!10 = !{i32 14, !11}
!11 = !{!"explicit_arg_num", i32 2}
!12 = !{i32 14, !13}
!13 = !{!"explicit_arg_num", i32 4}
!14 = !{i32 14, !15}
!15 = !{!"explicit_arg_num", i32 5}
!16 = !{!"sub_group_size", i32 16}
