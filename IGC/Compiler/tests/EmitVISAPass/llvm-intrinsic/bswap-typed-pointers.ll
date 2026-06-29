;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt -platformdg2 -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that llvm.bswap call is emited as sequence of
; moves that do the byte swap

; CHECK-DAG: mov (M1, 16) [[RES32:[A-z0-9]*]]_{{.*}}(0,3)<4> [[A:[A-z0-9_]*]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 16) [[RES32]]{{.*}}(0,0)<4> [[A]](0,3)<0;1,0>
; CHECK-DAG: mov (M1, 16) [[RES32]]{{.*}}(0,2)<4> [[A]](0,1)<0;1,0>
; CHECK-DAG: mov (M1, 16) [[RES32]]{{.*}}(0,1)<4> [[A]](0,2)<0;1,0>
; CHECK-DAG: lsc_store.ugm (M1, 16)  {{.*}} [[RES32]]:d32

; CHECK: bb1:
; CHECK-DAG: mov (M1_NM, 1) [[RES32_0:[A-z0-9]*]](0,3)<1> [[A64:[A-z0-9_]*]](0,0)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_0]](0,2)<1> [[A64]](0,1)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_0]](0,1)<1> [[A64]](0,2)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_0]](0,0)<1> [[A64]](0,3)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_1:[A-z0-9]*]](0,3)<1> [[A64]](0,4)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_1]](0,2)<1> [[A64]](0,5)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_1]](0,1)<1> [[A64]](0,6)<0;1,0>
; CHECK-DAG: mov (M1_NM, 1) [[RES32_1]](0,0)<1> [[A64]](0,7)<0;1,0>

define spir_kernel void @intrinsic_bswap(i32 %a, i32 addrspace(1)* %res32, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %bswap32 = call i32 @llvm.bswap.i32(i32 %a)
  store i32 %bswap32, i32 addrspace(1)* %res32
  br label %bb1
bb1:
  %a64 = sext i32 %a to i64
  %bswap64 = call i64 @llvm.bswap.i64(i64 %a64)
  %res64 = bitcast i32 addrspace(1)* %res32 to i64 addrspace(1)*
  store i64 %bswap64, i64 addrspace(1)* %res64
  ret void
}

declare i32 @llvm.bswap.i32(i32) #5
declare i64 @llvm.bswap.i64(i64) #5

attributes #5 = { readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_bswap}
!3 = !{!"FuncMDValue[0]", !4, !17, !36}
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
!21 = !{void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_bswap, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
!29 = !{!"argId", i32 0}
!30 = !{!"implicitArgInfoListVec[0]", !29}
!31 = !{!"argId", i32 1}
!32 = !{!"implicitArgInfoListVec[1]", !31}
!33 = !{!"argId", i32 15}
!34 = !{!"explicitArgNum", i32 2}
!35 = !{!"implicitArgInfoListVec[2]", !33, !34}
!36 = !{!"implicitArgInfoList", !30, !32, !35}
