; UNSUPPORTED: system-windows
; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vector331 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vectorized_cast v_type=G type=hf num_elts=128 align=wordx32

; CHECK: exp (M1, 16) vectorized_intrinsic(0,0)<1> vector331(0,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(1,0)<1> vector331(1,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(2,0)<1> vector331(2,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(3,0)<1> vector331(3,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(4,0)<1> vector331(4,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(5,0)<1> vector331(5,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(6,0)<1> vector331(6,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(7,0)<1> vector331(7,0)<1;1,0>

; CHECK: mov (M1, 16) vectorized_cast(0,0)<1> vectorized_intrinsic(0,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(0,16)<1> vectorized_intrinsic(1,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(1,0)<1> vectorized_intrinsic(2,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(1,16)<1> vectorized_intrinsic(3,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(2,0)<1> vectorized_intrinsic(4,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(2,16)<1> vectorized_intrinsic(5,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(3,0)<1> vectorized_intrinsic(6,0)<1;1,0>
; CHECK: mov (M1, 16) vectorized_cast(3,16)<1> vectorized_intrinsic(7,0)<1;1,0>


define spir_kernel void @_attn_fwd(half addrspace(1)* %0, half addrspace(1)* %1, half addrspace(1)* %2, float %3, i8 addrspace(1)* %4, float addrspace(1)* %5, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2, i32 %bufferOffset3, i32 %bufferOffset4) {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %6
  %7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %8 = extractelement <8 x float> %7, i64 0
  %vector331 = insertelement <8 x float> zeroinitializer, float %8, i32 0
  %vectorized_intrinsic = call <8 x float> @llvm.exp2.v8f32(<8 x float> %vector331)
  %vectorized_cast = fptrunc <8 x float> %vectorized_intrinsic to <8 x half>
  %9 = bitcast <8 x half> %vectorized_cast to <8 x i16>
  %10 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %9, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare <8 x float> @llvm.exp2.v8f32(<8 x float>) #1

; uselistorder directives
uselistorder <8 x float> (<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32, { 1, 0 }


attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
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


