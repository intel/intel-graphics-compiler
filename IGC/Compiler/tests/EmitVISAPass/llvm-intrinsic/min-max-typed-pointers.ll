;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus

; RUN: igc_opt -platformdg2 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that
; llvm.umin call is emited as min
; llvm.umax call is emited as max
; and signed ones use signed data types.


; CHECK: min (M1_NM, 1) [[UMIN:[A-z0-9]*]]_{{.*}}(0,0)<1> [[A_U:[A-z0-9]*]](0,0)<0;1,0> 0x0:ud
; CHECK: lsc_store.ugm (M1_NM, 1)  {{.*}} [[UMIN]]:d32

; CHECK: 1_bb1:
; CHECK: max (M1_NM, 1) [[UMAX:[A-z0-9]*]]_{{.*}}(0,0)<1> [[A_U]](0,0)<0;1,0> 0x0:ud
; CHECK: lsc_store.ugm (M1_NM, 1)  {{.*}} [[UMAX]]:d32

; CHECK: 2_bb2:
; CHECK: min (M1_NM, 1) [[SMIN:[A-z0-9]*]](0,0)<1> [[A:[A-z0-9]*]](0,0)<0;1,0> 0x0:d
; CHECK: lsc_store.ugm (M1_NM, 1)  {{.*}} [[SMIN]]:d32

; CHECK: 3_bb3:
; CHECK: max (M1_NM, 1) [[SMAX:[A-z0-9]*]](0,0)<1> [[A]](0,0)<0;1,0> 0x0:d
; CHECK: lsc_store.ugm (M1_NM, 1)  {{.*}} [[SMAX]]:d32

define spir_kernel void @intrinsic_max_min(i32 %a, i32 addrspace(1)* %res, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %umin = call i32 @llvm.umin.i32(i32 %a, i32 zeroinitializer)
  store i32 %umin, i32 addrspace(1)* %res
  br label %bb1
bb1:
  %umax = call i32 @llvm.umax.i32(i32 %a, i32 zeroinitializer)
  store i32 %umax, i32 addrspace(1)* %res
  br label %bb2
bb2:
  %smin = call i32 @llvm.smin.i32(i32 %a, i32 zeroinitializer)
  store i32 %smin, i32 addrspace(1)* %res
  br label %bb3
bb3:
  %smax = call i32 @llvm.smax.i32(i32 %a, i32 zeroinitializer)
  store i32 %smax, i32 addrspace(1)* %res
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.umin.i32(i32, i32) #5
declare i32 @llvm.umax.i32(i32, i32) #5
declare i32 @llvm.smin.i32(i32, i32) #5
declare i32 @llvm.smax.i32(i32, i32) #5
attributes #5 = { readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_max_min}
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
!21 = !{void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_max_min, !22}
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
