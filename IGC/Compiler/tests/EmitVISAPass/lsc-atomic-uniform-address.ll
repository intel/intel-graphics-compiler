;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -ocl -platformdg2 -igc-emit-visa -simd-mode 16 %s -regkey DumpVISAASMToConsole | FileCheck %s

; Verifies that LSC atomics with a uniform address and per-lane values are
; emitted for every lane, not as a single SIMD1 message.

; CHECK-LABEL: .kernel "test"
; CHECK-NOT: (M1_NM, 1) {{.*}}flat[
; CHECK:     lsc_atomic_iadd.ugm (M1, 16)
; CHECK-NOT: (M1_NM, 1) {{.*}}flat[
; CHECK:     lsc_atomic_icas.ugm (M1, 16)
; CHECK-NOT: (M1_NM, 1) {{.*}}flat[
; CHECK:     ret

define spir_kernel void @test(ptr addrspace(1) %total_value, ptr addrspace(1) %fetched, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i32
  %old = call i32 @llvm.genx.GenISA.LSCAtomicInts.i32.p1.i32.i32(ptr addrspace(1) %total_value, i32 0, i32 %lid, i32 0, i32 0, i32 0)
  %cas = call i32 @llvm.genx.GenISA.LSCAtomicInts.i32.p1.i32.i32(ptr addrspace(1) %total_value, i32 0, i32 %old, i32 %lid, i32 7, i32 0)
  %idx = zext i16 %localIdX to i64
  %ptr = getelementptr inbounds i32, ptr addrspace(1) %fetched, i64 %idx
  store i32 %cas, ptr addrspace(1) %ptr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.LSCAtomicInts.i32.p1.i32.i32(ptr addrspace(1), i32, i32, i32, i32, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7, !8, !19}
!7 = !{!"functionType", !"KernelFunction"}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10, !14, !15, !16, !17}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !11, !12, !13}
!15 = !{!"argAllocMDListVec[2]", !11, !12, !13}
!16 = !{!"argAllocMDListVec[3]", !11, !12, !13}
!17 = !{!"argAllocMDListVec[4]", !11, !12, !13}
!19 = !{!"implicitArgInfoList", !20, !22, !24}
!20 = !{!"implicitArgInfoListVec[0]", !21}
!21 = !{!"argId", i32 0}
!22 = !{!"implicitArgInfoListVec[1]", !23}
!23 = !{!"argId", i32 1}
!24 = !{!"implicitArgInfoListVec[2]", !25}
!25 = !{!"argId", i32 8}
