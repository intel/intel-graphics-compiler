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
define spir_kernel void @test(i8 addrspace(1)* %src, i8 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %loaded = load i8, i8 addrspace(1)* %src, align 1

  ; CHECK: and (M1_NM, 1) [[UNPACKED:[A-z0-9]*]](0,0)<1> [[LOADED:[A-z0-9]*]](0,0)<0;1,0> 0xf:ub
  %unpacked = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %loaded, i8 0)

  ; Store the unpacked result
  store i8 %unpacked, i8 addrspace(1)* %dst, align 1
  ret void
}

declare i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8, i8)

!igc.functions = !{!3}
!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !13}
!7 = !{i32 0}
!8 = !{i32 2}
!9 = !{i32 57}
!10 = !{i32 58}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 0}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 1}
