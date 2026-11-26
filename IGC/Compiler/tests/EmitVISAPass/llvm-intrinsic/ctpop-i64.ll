;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformdg2 -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that llvm.ctpop with i64 source is compiled properly
; as a sequence of cbit instructions on the lower and upper 32-bit parts

; Uniform source
; CHECK: cbit (M1_NM, 1) [[ULO:[A-z0-9_]+]](0,0)<1> [[U:[A-z0-9_]+]](0,0)<0;1,0>
; CHECK: cbit (M1_NM, 1) [[UHI:[A-z0-9_]+]](0,0)<1> [[U]](0,1)<0;1,0>
; CHECK: add (M1_NM, 1) [[ULO]](0,0)<1> [[ULO]](0,0)<0;1,0> [[UHI]](0,0)<0;1,0>

; Non-uniform source
; CHECK: cbit (M1, 16) [[LO:[A-z0-9_]+]](0,0)<1> [[V:[A-z0-9_]+]](0,0)<2;1,0>
; CHECK: cbit (M1, 16) [[HI:[A-z0-9_]+]](0,0)<1> [[V]](0,1)<2;1,0>
; CHECK: add (M1, 16) [[LO]](0,0)<1> [[LO]](0,0)<1;1,0> [[HI]](0,0)<1;1,0>

define spir_kernel void @test(i64 %a, ptr addrspace(1) %dst) {
  %u = call i64 @llvm.ctpop.i64(i64 %a)
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %b = zext i16 %lane to i64
  %v = call i64 @llvm.ctpop.i64(i64 %b)
  %sum = add i64 %u, %v
  store i64 %sum, ptr addrspace(1) %dst
  ret void
}

declare i64 @llvm.ctpop.i64(i64)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0}
!IGCMetadata = !{!3}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !13}
!9 = !{!"argAllocMDListVec[0]", !10, !11, !12}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[1]", !10, !11, !12}
