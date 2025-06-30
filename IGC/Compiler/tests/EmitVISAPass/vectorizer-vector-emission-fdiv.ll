; UNSUPPORTED: system-windows
; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 --regkey=VectorizerUniformValueVectorizationEnabled=0 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_phi1095 v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vectorized_phi1116 v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vector1029 v_type=G type=f num_elts=8 align=dword
; CHECK: .decl vector1052 v_type=G type=f num_elts=8 align=dword


; CHECK: div (M1_NM, 1) vectorized_binary1096(0,0)<1> vectorized_phi1095(0,0)<0;1,0> vector1029(0,0)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,1)<1> vectorized_phi1095(0,1)<0;1,0> vector1029(0,1)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,2)<1> vectorized_phi1095(0,2)<0;1,0> vector1029(0,2)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,3)<1> vectorized_phi1095(0,3)<0;1,0> vector1029(0,3)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,4)<1> vectorized_phi1095(0,4)<0;1,0> vector1029(0,4)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,5)<1> vectorized_phi1095(0,5)<0;1,0> vector1029(0,5)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,6)<1> vectorized_phi1095(0,6)<0;1,0> vector1029(0,6)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1096(0,7)<1> vectorized_phi1095(0,7)<0;1,0> vector1029(0,7)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,0)<1> vectorized_phi1116(0,0)<0;1,0> vector1052(0,0)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,1)<1> vectorized_phi1116(0,1)<0;1,0> vector1052(0,1)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,2)<1> vectorized_phi1116(0,2)<0;1,0> vector1052(0,2)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,3)<1> vectorized_phi1116(0,3)<0;1,0> vector1052(0,3)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,4)<1> vectorized_phi1116(0,4)<0;1,0> vector1052(0,4)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,5)<1> vectorized_phi1116(0,5)<0;1,0> vector1052(0,5)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,6)<1> vectorized_phi1116(0,6)<0;1,0> vector1052(0,6)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_binary1117(0,7)<1> vectorized_phi1116(0,7)<0;1,0> vector1052(0,7)<0;1,0>

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @_attn_fwd(half addrspace(1)* %0, half addrspace(1)* %1, half addrspace(1)* %2, float %3, i8 addrspace(1)* %4, float addrspace(1)* %5, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2, i32 %bufferOffset3, i32 %bufferOffset4) {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %6
  %vectorized_phi1095 = phi <8 x float> [ zeroinitializer, %6 ], [ %vectorized_binary1105, %._crit_edge.._crit_edge_crit_edge ]
  %vectorized_phi1116 = phi <8 x float> [ zeroinitializer, %6 ], [ %vectorized_binary1126, %._crit_edge.._crit_edge_crit_edge ]
  %vector1029 = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %vector1052 = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %vectorized_binary1096 = fdiv <8 x float> %vectorized_phi1095, %vector1029
  %vectorized_binary1117 = fdiv <8 x float> %vectorized_phi1116, %vector1052
  %vectorized_binary1105 = fadd <8 x float> %vectorized_binary1096, zeroinitializer
  %vectorized_binary1126 = fadd <8 x float> %vectorized_binary1117, zeroinitializer
  br i1 false, label %._crit_edge.._crit_edge_crit_edge, label %7

._crit_edge.._crit_edge_crit_edge:                ; preds = %._crit_edge
  br label %._crit_edge

7:                                                ; preds = %._crit_edge
  %.assembled.vect934 = bitcast <8 x float> %vectorized_binary1126 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %.assembled.vect934)
  %.assembled.vect950 = bitcast <8 x float> %vectorized_binary1105 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %.assembled.vect950)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; uselistorder directives
uselistorder void (i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)* @llvm.genx.GenISA.LSC2DBlockWrite.v8i32, { 1, 0 }

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = distinct !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32, i32)* @_attn_fwd, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
!4 = distinct !{!"ModuleMD", !5}
!5 = distinct !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32, i32)* @_attn_fwd}
!7 = distinct !{!"FuncMDValue[0]", !8}
!8 = distinct !{!"resAllocMD", !9}
!9 = distinct !{!"argAllocMDList", !10}
!10 = distinct !{!"argAllocMDListVec[0]"}
