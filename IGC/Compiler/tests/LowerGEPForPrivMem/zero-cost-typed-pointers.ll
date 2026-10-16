;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; Zero-cost promotion: a small array whose every load is preceded, in the same
; block, by constant-index stores to all elements. Every function is marked as
; having the pass disabled (m_OptsToDisablePerFunc), as the retry manager does,
; so the pass runs in restricted mode and promotes only zero-cost arrays.
; Zero-cost promotion is on by default (DisablePrivMemZeroCostPromotion=0).
;
; RUN: igc_opt --typed-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s --check-prefix=RESTRICTED
; RUN: igc_opt --typed-pointers -igc-priv-mem-to-reg --regkey DisablePrivMemZeroCostPromotion=1 -S < %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OFF

; Fully initialized in the block of the dynamic-index load: promoted by default,
; not promoted when zero-cost promotion is disabled.
define float @zero_cost(float %a, float %b, float %c, float %d, i32 %i) {
; RESTRICTED-LABEL: @zero_cost(
; RESTRICTED-NOT:     alloca [4 x float]
; RESTRICTED:         alloca <4 x float>
; RESTRICTED:         extractelement <4 x float> {{%.*}}, i32 {{%.*}}
; OFF-LABEL:        @zero_cost(
; OFF-NOT:            alloca <4 x float>
; OFF:                alloca [4 x float]
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %p3 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 3
  store float %d, float* %p3, align 4
  %idx = zext i32 %i to i64
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  ret float %v
}

; i32 array in a non-entry block; the latest store to an element covers it.
define i32 @zero_cost_i32(i1 %cond, i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i64 %idx) {
; RESTRICTED-LABEL: @zero_cost_i32(
; RESTRICTED-NOT:     alloca [4 x i32]
; RESTRICTED:         alloca <4 x i32>
entry:
  %arr = alloca [4 x i32], align 4
  br i1 %cond, label %then, label %exit
then:
  %p0 = getelementptr inbounds [4 x i32], [4 x i32]* %arr, i64 0, i64 0
  store i32 %a, i32* %p0, align 4
  %p1 = getelementptr inbounds [4 x i32], [4 x i32]* %arr, i64 0, i64 1
  store i32 %b, i32* %p1, align 4
  %p2 = getelementptr inbounds [4 x i32], [4 x i32]* %arr, i64 0, i64 2
  store i32 %c, i32* %p2, align 4
  %p3 = getelementptr inbounds [4 x i32], [4 x i32]* %arr, i64 0, i64 3
  store i32 %d, i32* %p3, align 4
  store i32 %e, i32* %p0, align 4
  %pi = getelementptr inbounds [4 x i32], [4 x i32]* %arr, i64 0, i64 %idx
  %v = load i32, i32* %pi, align 4
  br label %exit
exit:
  %r = phi i32 [ %v, %then ], [ 0, %entry ]
  ret i32 %r
}

; Not every element is stored before the load: not zero cost. Normal mode still
; promotes it through the pressure heuristic.
define float @partial_init(float %a, float %b, float %c, i64 %idx) {
; RESTRICTED-LABEL: @partial_init(
; RESTRICTED:         alloca [4 x float]
; RESTRICTED-NOT:     alloca <4 x float>
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  ret float %v
}

; The stores are in a different block than the load: the promoted vector would
; be live across blocks, so it is not zero cost.
define float @init_in_other_block(float %a, float %b, float %c, float %d, i64 %idx) {
; RESTRICTED-LABEL: @init_in_other_block(
; RESTRICTED:         alloca [4 x float]
; RESTRICTED-NOT:     alloca <4 x float>
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %p3 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 3
  store float %d, float* %p3, align 4
  br label %next
next:
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  ret float %v
}

; A dynamic-index store between the initialization and the load may overwrite
; any element: not zero cost.
define float @dynamic_store_between(float %a, float %b, float %c, float %d, float %e, i64 %j, i64 %idx) {
; RESTRICTED-LABEL: @dynamic_store_between(
; RESTRICTED:         alloca [4 x float]
; RESTRICTED-NOT:     alloca <4 x float>
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %pj = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %j
  store float %e, float* %pj, align 4
  %p3 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 3
  store float %d, float* %p3, align 4
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  ret float %v
}

; A dynamic-index store anywhere (here after the load) would become an indirect
; write after promotion: not zero cost.
define float @dynamic_store_after_load(float %a, float %b, float %c, float %d, float %e, i64 %j, i64 %idx) {
; RESTRICTED-LABEL: @dynamic_store_after_load(
; RESTRICTED:         alloca [4 x float]
; RESTRICTED-NOT:     alloca <4 x float>
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %p3 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 3
  store float %d, float* %p3, align 4
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  %pj = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %j
  store float %e, float* %pj, align 4
  ret float %v
}

declare void @use(float*)

; The array escapes to a call: not zero cost.
define float @escaping(float %a, float %b, float %c, float %d, i64 %idx) {
; RESTRICTED-LABEL: @escaping(
; RESTRICTED:         alloca [4 x float]
; RESTRICTED-NOT:     alloca <4 x float>
entry:
  %arr = alloca [4 x float], align 4
  %p0 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 0
  store float %a, float* %p0, align 4
  %p1 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 1
  store float %b, float* %p1, align 4
  %p2 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 2
  store float %c, float* %p2, align 4
  %p3 = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 3
  store float %d, float* %p3, align 4
  call void @use(float* %p0)
  %pi = getelementptr inbounds [4 x float], [4 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %pi, align 4
  ret float %v
}

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8}

!0 = !{float (float, float, float, float, i32)* @zero_cost, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i32 (i1, i32, i32, i32, i32, i32, i64)* @zero_cost_i32, !1}
!4 = !{float (float, float, float, i64)* @partial_init, !1}
!5 = !{float (float, float, float, float, i64)* @init_in_other_block, !1}
!6 = !{float (float, float, float, float, float, i64, i64)* @dynamic_store_between, !1}
!7 = !{float (float, float, float, float, i64)* @escaping, !1}
!8 = !{float (float, float, float, float, float, i64, i64)* @dynamic_store_after_load, !1}

!IGCMetadata = !{!9}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24}
!11 = !{!"FuncMDMap[0]", float (float, float, float, float, i32)* @zero_cost}
!12 = !{!"FuncMDValue[0]", !30}
!13 = !{!"FuncMDMap[1]", i32 (i1, i32, i32, i32, i32, i32, i64)* @zero_cost_i32}
!14 = !{!"FuncMDValue[1]", !30}
!15 = !{!"FuncMDMap[2]", float (float, float, float, i64)* @partial_init}
!16 = !{!"FuncMDValue[2]", !30}
!17 = !{!"FuncMDMap[3]", float (float, float, float, float, i64)* @init_in_other_block}
!18 = !{!"FuncMDValue[3]", !30}
!19 = !{!"FuncMDMap[4]", float (float, float, float, float, float, i64, i64)* @dynamic_store_between}
!20 = !{!"FuncMDValue[4]", !30}
!21 = !{!"FuncMDMap[5]", float (float, float, float, float, i64)* @escaping}
!22 = !{!"FuncMDValue[5]", !30}
!23 = !{!"FuncMDMap[6]", float (float, float, float, float, float, i64, i64)* @dynamic_store_after_load}
!24 = !{!"FuncMDValue[6]", !30}
!30 = !{!"m_OptsToDisablePerFunc", !31}
!31 = !{!"m_OptsToDisablePerFuncSet[0]", !"IGC-LowerGEPForPrivMem"}
