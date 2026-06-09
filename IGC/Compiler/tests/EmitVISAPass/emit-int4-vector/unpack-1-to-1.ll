;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
;
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 -inputrt -regkey DumpVISAASMToConsole -S %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .kernel "test"
define spir_kernel void @test(i8 addrspace(1)* %src, i8 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ; Compute offsetted pointers based on local ID
  %id = zext i16 %localIdX to i64
  %srcInt = ptrtoint i8 addrspace(1)* %src to i64
  %srcIntOffsetted = add i64 %id, %srcInt
  %dstInt = ptrtoint i8 addrspace(1)* %dst to i64
  %dstIntOffsetted = add i64 %id, %dstInt
  %srcOffsetted = inttoptr i64 %srcIntOffsetted to i8 addrspace(1)*
  %dstOffsetted = inttoptr i64 %dstIntOffsetted to i8 addrspace(1)*

  %loaded = load i8, i8 addrspace(1)* %srcOffsetted, align 1

  ; CHECK: and (M1, 16) [[UNPACKED:[A-z0-9]*]](0,0)<1> [[LOADED:[A-z0-9]*]](0,0)<1;1,0> 0xf:ub
  %unpacked = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %loaded, i8 0)

  ; Store the unpacked result
  store i8 %unpacked, i8 addrspace(1)* %dstOffsetted, align 1
  ret void
}

declare i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8, i8)

!igc.functions = !{!3}
!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!19 = !{!"argId", i32 0}
!20 = !{!"implicitArgInfoListVec[0]", !19}
!21 = !{!"argId", i32 2}
!22 = !{!"implicitArgInfoListVec[1]", !21}
!23 = !{!"argId", i32 7}
!24 = !{!"implicitArgInfoListVec[2]", !23}
!25 = !{!"argId", i32 8}
!26 = !{!"implicitArgInfoListVec[3]", !25}
!27 = !{!"argId", i32 9}
!28 = !{!"implicitArgInfoListVec[4]", !27}
!29 = !{!"argId", i32 10}
!30 = !{!"implicitArgInfoListVec[5]", !29}
!31 = !{!"argId", i32 57}
!32 = !{!"implicitArgInfoListVec[6]", !31}
!33 = !{!"argId", i32 58}
!34 = !{!"implicitArgInfoListVec[7]", !33}
!35 = !{!"argId", i32 15}
!36 = !{!"explicitArgNum", i32 0}
!37 = !{!"implicitArgInfoListVec[8]", !35, !36}
!38 = !{!"argId", i32 15}
!39 = !{!"explicitArgNum", i32 1}
!40 = !{!"implicitArgInfoListVec[9]", !38, !39}
!41 = !{!"implicitArgInfoList", !20, !22, !24, !26, !28, !30, !32, !34, !37, !40}
!42 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test}
!43 = !{!"FuncMDValue[0]", !41}
!44 = !{!"FuncMD", !42, !43}
!45 = !{!"ModuleMD", !44}
!IGCMetadata = !{!45}
