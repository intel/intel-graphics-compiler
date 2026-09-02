;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt %s \
; RUN:     --opaque-pointers \
; RUN:     -GenXCodeGenModule \
; RUN:     -igc-emit-visa \
; RUN:     -platformbmg \
; RUN:     -simd-mode 16 \
; RUN:     -regkey DumpVISAASMToConsole \
; RUN:   | FileCheck %s --check-prefixes=CHECK,ENABLED --implicit-check-not='%%sr0(0,2)'

; RUN: igc_opt %s \
; RUN:     --opaque-pointers \
; RUN:     -GenXCodeGenModule \
; RUN:     -igc-emit-visa \
; RUN:     -platformbmg \
; RUN:     -simd-mode 16 \
; RUN:     -regkey DumpVISAASMToConsole,DisableUniformAnalysis=1 \
; RUN:   | FileCheck %s --check-prefixes=CHECK,DISABLED --implicit-check-not='%%sr0(0,2)'

; A ballot inside divergent control flow must use the current execution mask,
; because only some lanes reached it. DisableUniformAnalysis leaves
; WIAnalysis::m_ctrlBranches empty, and that must not be read as convergence -
; doing so picks the dispatch mask (sr0.2) and reports lanes that never ran.
; Both runs must store the ballot derived from the execution mask. With analysis
; disabled, the shared result is copied to per-lane storage before the store.

; CHECK-LABEL: .kernel "test"
; CHECK:       setp (M1_NM, 16) [[FLAG:P[0-9]+]] 0x0:ud
; CHECK:       cmp.eq (M1, 16) [[FLAG]] [[TMP:[A-Za-z0-9_]+]](0,0)<0;1,0> [[TMP]](0,0)<0;1,0>
; CHECK:       mov (M1_NM, 1) [[MASK:[A-Za-z0-9_]+]](0,0)<1> [[FLAG]]
; CHECK:       mov (M1_NM, 1) [[BALLOT:[A-Za-z0-9_]+]](0,0)<1> [[MASK]](0,0)<0;1,0>
; DISABLED:    mov (M1, 16) [[RESULT:[A-Za-z0-9_]+]](0,0)<1> [[BALLOT]](0,0)<0;1,0>
; ENABLED:     lsc_store.ugm (M1_NM, 1) {{.*}} [[BALLOT]]:d32t
; DISABLED:    lsc_store.ugm (M1, 16) {{.*}} [[RESULT]]:d32

define spir_kernel void @test(<8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %half = icmp ult i16 %lane, 8
  br i1 %half, label %divergent, label %exit

divergent:
  %b = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  store i32 %b, ptr addrspace(1) null, align 4
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.WaveBallot(i1, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"implicitArgInfoList", !7, !8, !9}
!7 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!8 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!9 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 13}}
