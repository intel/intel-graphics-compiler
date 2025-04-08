;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus

; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa -regkey EnableDebugging -simd-mode 16 %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

;; Test read payload size for LSC2DBlockReadAddrPayload intrinsics
;;

define spir_kernel void @test(i64 %b, i32 %x, i32 %y, i32 %k) {
entry:
;
; case 0:   u32_m7k1 transpose
;
; CHECK: Block2D: transpose block height not power of 2, zero padded.
;
  %ap = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %b, i32 0, i32 1023, i32 1023, i32 %x, i32 %y, i32 1, i32 7, i32 1)
  %val = call i32 @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.p0i32(i32* %ap, i32 0, i32 0, i32 32, i32 1, i32 7, i32 1, i1 true, i1 false, i32 0)
  %tmp0 = add i64 %b, 1024
  %addr0 = inttoptr i64 %tmp0 to i32 addrspace(1)*
  store i32 %val, i32 addrspace(1)* %addr0, align 4


;
; case 1: u16_m15k2 vnni transform
;
; CHECK: Block2D: block size not multiple of GRF size, zero padded
; CHECK: Block2D: transform block height not multiple of N (32/eltBits), zero padded
;
  %ap1 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %b, i32 0, i32 1023, i32 1023, i32 %x, i32 %y, i32 2, i32 15, i32 1)
  %val1 = call <2 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.p0i32.v2i16(i32* %ap, i32 0, i32 0, i32 16, i32 2, i32 15, i32 1, i1 false, i1 true, i32 0)
  %tmp1 = add i64 %b, 2048
  %val10 = bitcast <2 x i16> %val1 to i32
  %addr1 = inttoptr i64 %tmp1 to i32 addrspace(1)*
  store i32 %val10, i32 addrspace(1)* %addr1, align 4

  ret void
}

declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare <2 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.p0i32.v2i16(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare i32 @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)


!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i64, i32, i32, i32)* @test}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!15 = !{!"argAllocMDListVec[4]", !7, !8, !9}
!16 = !{!"argAllocMDListVec[5]", !7, !8, !9}
!17 = !{!"m_OpenCLArgTypeQualifiers", !18, !19, !20}
!18 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!19 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!21 = !{void (i64, i32, i32, i32)* @test, !22}
!22 = !{!23, !24}
!23 = !{!"function_type", i32 0}
!24 = !{!"sub_group_size", i32 16}
