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
define spir_kernel void @test(i16 addrspace(1)* %src, i32 addrspace(1)* %dst, <8 x i32> %r0, <3 x i32> %globalOffset, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ; Load src and cast it to <2 x i8>
  %loaded = load i16, i16 addrspace(1)* %src, align 2
  %loadedCast = bitcast i16 %loaded to <2 x i8>

  ; CHECK: and (M1_NM, 1) [[UNPACKED:[A-z0-9]*]](0,0)<1> [[LOADED:[A-z0-9]*]](0,0)<0;1,0> 0xf:ub
  ; CHECK: shr (M1_NM, 1) [[UNPACKED]](0,1)<1> [[LOADED]](0,0)<0;1,0> 0x4:ub
  ; CHECK: and (M1_NM, 1) [[UNPACKED]](0,2)<1> [[LOADED]](0,1)<0;1,0> 0xf:ub
  ; CHECK: shr (M1_NM, 1) [[UNPACKED]](0,3)<1> [[LOADED]](0,1)<0;1,0> 0x4:ub
  %unpacked = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %loadedCast, i8 0)

  ; Store the unpacked result
  %unpackedCast = bitcast <4 x i8> %unpacked to i32
  store i32 %unpackedCast, i32 addrspace(1)* %dst, align 4
  ret void
}

declare <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8>, i8)

!igc.functions = !{!3}
!3 = !{void (i16 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <3 x i32>, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!15 = !{!"argId", i32 0}
!16 = !{!"implicitArgInfoListVec[0]", !15}
!17 = !{!"argId", i32 2}
!18 = !{!"implicitArgInfoListVec[1]", !17}
!19 = !{!"argId", i32 57}
!20 = !{!"implicitArgInfoListVec[2]", !19}
!21 = !{!"argId", i32 58}
!22 = !{!"implicitArgInfoListVec[3]", !21}
!23 = !{!"argId", i32 15}
!24 = !{!"explicitArgNum", i32 0}
!25 = !{!"implicitArgInfoListVec[4]", !23, !24}
!26 = !{!"argId", i32 15}
!27 = !{!"explicitArgNum", i32 1}
!28 = !{!"implicitArgInfoListVec[5]", !26, !27}
!29 = !{!"implicitArgInfoList", !16, !18, !20, !22, !25, !28}
!30 = !{!"FuncMDMap[0]", void (i16 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <3 x i32>, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32)* @test}
!31 = !{!"FuncMDValue[0]", !29}
!32 = !{!"FuncMD", !30, !31}
!33 = !{!"ModuleMD", !32}
!IGCMetadata = !{!33}
