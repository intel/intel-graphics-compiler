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
define spir_kernel void @test(i32 addrspace(1)* %src, i16 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ; Compute offsetted pointers based on local ID
  %id = zext i16 %localIdX to i64
  %srcInt = ptrtoint i32 addrspace(1)* %src to i64
  %id2 = shl i64 %id, 2
  %srcIntOffsetted = add i64 %id2, %srcInt
  %dstInt = ptrtoint i16 addrspace(1)* %dst to i64
  %id1 = shl i64 %id, 1
  %dstIntOffsetted = add i64 %id1, %dstInt
  %srcOffsetted = inttoptr i64 %srcIntOffsetted to i32 addrspace(1)*
  %dstOffsetted = inttoptr i64 %dstIntOffsetted to i16 addrspace(1)*

  ; Load src and cast it to <4 x i8>
  %loaded = load i32, i32 addrspace(1)* %srcOffsetted, align 4
  %loadedCast = bitcast i32 %loaded to <4 x i8>

  ; CHECK: and (M1, 16) [[PACKED:[A-z0-9]+]](0,0)<1> [[LOADED:[A-z0-9]+]](0,0)<1;1,0> 0xf:ub
  ; CHECK: shl (M1, 16) [[SHIFTED1:[A-z0-9]+]](0,0)<1> [[LOADED]](0,16)<1;1,0> 0x4:ub
  ; CHECK: or (M1, 16) [[PACKED]](0,0)<1> [[PACKED]](0,0)<1;1,0> [[SHIFTED1]](0,0)<1;1,0>
  ; CHECK: and (M1, 16) [[PACKED]](0,16)<1> [[LOADED]](0,32)<1;1,0> 0xf:ub
  ; CHECK: shl (M1, 16) [[SHIFTED2:[A-z0-9]+]](0,0)<1> [[LOADED]](0,48)<1;1,0> 0x4:ub
  ; CHECK: or (M1, 16) [[PACKED]](0,16)<1> [[PACKED]](0,16)<1;1,0> [[SHIFTED2]](0,0)<1;1,0>
  %packed = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %loadedCast)

  ; Store the packed result
  %packedCast = bitcast <2 x i8> %packed to i16
  store i16 %packedCast, i16 addrspace(1)* %dstOffsetted, align 2
  ret void
}

declare <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8>)

!igc.functions = !{!3}
!3 = !{void (i32 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !14, !15, !17}
!7 = !{i32 0}
!8 = !{i32 2}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 56}
!14 = !{i32 57}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 0}
!17 = !{i32 15, !18}
!18 = !{!"explicit_arg_num", i32 1}
