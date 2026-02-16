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
define spir_kernel void @test(i16 addrspace(1)* %src, i32 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ; Compute offsetted pointers based on local ID
  %id = zext i16 %localIdX to i64
  %srcInt = ptrtoint i16 addrspace(1)* %src to i64
  %id1 = shl i64 %id, 1
  %srcIntOffsetted = add i64 %id1, %srcInt
  %dstInt = ptrtoint i32 addrspace(1)* %dst to i64
  %id2 = shl i64 %id, 2
  %dstIntOffsetted = add i64 %id2, %dstInt
  %srcOffsetted = inttoptr i64 %srcIntOffsetted to i16 addrspace(1)*
  %dstOffsetted = inttoptr i64 %dstIntOffsetted to i32 addrspace(1)*

  %loaded = load i16, i16 addrspace(1)* %srcOffsetted, align 2
  %loadedCast = bitcast i16 %loaded to <2 x i8>

  ; CHECK: and (M1, 16) [[UNPACKED:[A-z0-9]*]](0,0)<1> [[LOADED:[A-z0-9]*]](0,0)<1;1,0> 0xf:ub
  ; CHECK: shr (M1, 16) [[UNPACKED]](0,16)<1> [[LOADED]](0,0)<1;1,0> 0x4:ub
  ; CHECK: and (M1, 16) [[UNPACKED]](0,32)<1> [[LOADED]](0,16)<1;1,0> 0xf:ub
  %unpacked = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %loadedCast, i8 0)

  ; Extract the first 3 bytes from the unpacked result
  %u0 = extractelement <3 x i8> %unpacked, i32 0
  %u1 = extractelement <3 x i8> %unpacked, i32 1
  %u2 = extractelement <3 x i8> %unpacked, i32 2

  ; Insert the 3 bytes into a <4 x i8> vector, setting the 4th byte to 0
  %i0 = insertelement <4 x i8> undef, i8 %u0, i32 0
  %i1 = insertelement <4 x i8> %i0, i8 %u1, i32 1
  %i2 = insertelement <4 x i8> %i1, i8 %u2, i32 2
  %i3 = insertelement <4 x i8> %i2, i8 0, i32 3
  %i3Cast = bitcast <4 x i8> %i3 to i32

  ; Store the unpacked result
  store i32 %i3Cast, i32 addrspace(1)* %dstOffsetted, align 4
  ret void
}

declare <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8>, i8)

!igc.functions = !{!3}
!3 = !{void (i16 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
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
