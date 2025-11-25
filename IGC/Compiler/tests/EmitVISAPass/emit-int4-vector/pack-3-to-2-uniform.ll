;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 -inputrt -regkey DumpVISAASMToConsole -S %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .kernel "test"
define spir_kernel void @test(i32 addrspace(1)* %src, i16 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %loaded = load i32, i32 addrspace(1)* %src, align 4

  ; Extract first 3 bytes from loaded i32
  %loadedBytes = bitcast i32 %loaded to <4 x i8>
  %l0 = extractelement <4 x i8> %loadedBytes, i32 0
  %l1 = extractelement <4 x i8> %loadedBytes, i32 1
  %l2 = extractelement <4 x i8> %loadedBytes, i32 2

  ; Insert first 3 bytes into a <3 x i8> vector
  %i0 = insertelement <3 x i8> undef, i8 %l0, i32 0
  %i1 = insertelement <3 x i8> %i0, i8 %l1, i32 1
  %i2 = insertelement <3 x i8> %i1, i8 %l2, i32 2

  ; CHECK: and (M1_NM, 1) [[PACKED:[A-z0-9]+]](0,0)<1> [[LOADED:[A-z0-9]+]](0,0)<0;1,0> 0xf:ub
  ; CHECK: shl (M1_NM, 1) [[SHIFTED:[A-z0-9]+]](0,0)<1> [[LOADED]](0,1)<0;1,0> 0x4:ub
  ; CHECK: or (M1_NM, 1) [[PACKED]](0,0)<1> [[PACKED]](0,0)<0;1,0> [[SHIFTED]](0,0)<0;1,0>
  ; CHECK: and (M1_NM, 1) [[PACKED]](0,1)<1> [[LOADED]](0,2)<0;1,0> 0xf:ub
  %packed = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %i2)

  ; Store the packed result
  %packedCast = bitcast <2 x i8> %packed to i16
  store i16 %packedCast, i16 addrspace(1)* %dst, align 2
  ret void
}

declare <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8>)

!igc.functions = !{!3}
!3 = !{void (i32 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <3 x i32>, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !13}
!7 = !{i32 0}
!8 = !{i32 2}
!9 = !{i32 62}
!10 = !{i32 63}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 0}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 1}
