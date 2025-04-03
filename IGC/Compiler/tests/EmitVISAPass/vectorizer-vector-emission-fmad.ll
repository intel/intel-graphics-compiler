; UNSUPPORTED: system-windows
; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_phi v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vector v_type=G type=f num_elts=8 align=dword

; CHECK: mad (M1, 16) vectorized_phi(0,0)<1> vectorized_phi(0,0)<1;1,0> vector(0,0)<0;1,0> vectorized_phi(0,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(1,0)<1> vectorized_phi(1,0)<1;1,0> vector(0,1)<0;1,0> vectorized_phi(1,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(2,0)<1> vectorized_phi(2,0)<1;1,0> vector(0,2)<0;1,0> vectorized_phi(2,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(3,0)<1> vectorized_phi(3,0)<1;1,0> vector(0,3)<0;1,0> vectorized_phi(3,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(4,0)<1> vectorized_phi(4,0)<1;1,0> vector(0,4)<0;1,0> vectorized_phi(4,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(5,0)<1> vectorized_phi(5,0)<1;1,0> vector(0,5)<0;1,0> vectorized_phi(5,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(6,0)<1> vectorized_phi(6,0)<1;1,0> vector(0,6)<0;1,0> vectorized_phi(6,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi(7,0)<1> vectorized_phi(7,0)<1;1,0> vector(0,7)<0;1,0> vectorized_phi(7,0)<1;1,0>


; ModuleID = 'vectorizer-vector-emission-fmad.ll'
source_filename = "vectorizer-vector-emission-fmad.ll"

define spir_kernel void @_attn_fwd(half addrspace(1)* %0, half addrspace(1)* %1, half addrspace(1)* %2, float %3, i8 addrspace(1)* %4, float addrspace(1)* %5, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2, i32 %bufferOffset3, i32 %bufferOffset4) {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %6
  %7 = phi float [ 0.000000e+00, %6 ], [ %7, %._crit_edge.._crit_edge_crit_edge ]
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %6 ], [ %8, %._crit_edge.._crit_edge_crit_edge ]
  %vector = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %vectorized_binary = fmul fast <8 x float> %vector, %vectorized_phi
  %vectorized_binary_1 = fadd fast <8 x float> %vectorized_binary, %vectorized_phi
  %8 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary_1, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge.._crit_edge_crit_edge

._crit_edge.._crit_edge_crit_edge:                ; preds = %._crit_edge
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = distinct !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32, i32)* @_attn_fwd, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
!4 = distinct !{!"ModuleMD", !5}
!5 = distinct !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32, i32)* @_attn_fwd}
!7 = distinct !{!"FuncMDValue[0]", !8}
!8 = distinct !{!"resAllocMD", !9}
!9 = distinct !{!"argAllocMDList", !10}
!10 = distinct !{!"argAllocMDListVec[0]"}
