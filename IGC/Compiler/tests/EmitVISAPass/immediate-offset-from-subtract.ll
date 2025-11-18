;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s --check-prefixes=CHECK,R-CHECK
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey LscImmOffsMatch=3 | FileCheck %s --check-prefixes=CHECK,D-CHECK
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Starting with Xe2, hardware is able to handle the following load:
;
; load [VAR+IMM]
;
; Purpose:
;   Verify immediate offset folding for subtraction-derived address patterns.
;   Each COM section below defines a distinct canonical pattern:
;     1) IMM - VAR (VAR known negative)
;     2) IMM - VAR (VAR known non-negative)
;     3) VAR - (-IMM) (VAR known non-negative)
;     4) VAR - IMM
;     5) VAR - (-IMM) (VAR known negative)
;   The chains are intentionally minimal. llvm.assume is used to fix sign info
;   so the matcher can choose positive / negative variants with no extra ops.

declare void @llvm.assume(i1 %cond)

define spir_kernel void @test(ptr addrspace(1) %out, i32 %positiveOffset, i32 %negativeOffset) {
entry:
  %pos_cond = icmp sgt i32 %positiveOffset, 0
  call void @llvm.assume(i1 %pos_cond)
  %neg_cond = icmp slt i32 %negativeOffset, 0
  call void @llvm.assume(i1 %neg_cond)

  %offset_0 = sub nsw i32 1020, %negativeOffset
  %ptr_0 = inttoptr i32 %offset_0 to ptr addrspace(3)
; COM: IMM - VAR pattern where VAR is negative
; CHECK:        mov (M1_NM, 1) [[NEG_VAR:.*]](0,0)<1> (-){{.*}}(0,0)<0;1,0>
; CHECK-NEXT:   mov (M1_NM, 1) [[NEG_VAR_COPY:.*]](0,0)<1> [[NEG_VAR]](0,0)<0;1,0>
; CHECK-NEXT:   lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[[[NEG_VAR_COPY]]+0x3fc]:a32
  %out_0 = load i32, ptr addrspace(3) %ptr_0, align 4
  store i32 %out_0, ptr addrspace(1) %out

  %offset_1 = sub nsw i32 512, %positiveOffset
  %ptr_1 = inttoptr i32 %offset_1 to ptr addrspace(3)
; COM: IMM - VAR pattern where VAR is positive
; D-CHECK:      mov (M1_NM, 1) [[NEG_VAR:.*]](0,0)<1> (-){{.*}}(0,0)<0;1,0>
; D-CHECK-NEXT: mov (M1_NM, 1) [[NEG_VAR_COPY:.*]](0,0)<1> [[NEG_VAR]](0,0)<0;1,0>
; D-CHECK-NEXT: lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[[[NEG_VAR_COPY]]+0x200]:a32
; R-CHECK-NOT:  lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{.*}}+0x200]:a32
; R-CHECK:      lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{[^+-]*}}]:a32
  %out_1 = load i32, ptr addrspace(3) %ptr_1, align 4
  store i32 %out_1, ptr addrspace(1) %out

  %offset_2 = sub nsw i32 %positiveOffset, -1020
  %ptr_2 = inttoptr i32 %offset_2 to ptr addrspace(3)
; COM: VAR - (-IMM) pattern where VAR is positive
; CHECK:       lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{.*}}+0x3fc]:a32
  %out_2 = load i32, ptr addrspace(3) %ptr_2, align 4
  store i32 %out_2, ptr addrspace(1) %out

  %offset_3 = sub nsw i32 %positiveOffset, 256
  %ptr_3 = inttoptr i32 %offset_3 to ptr addrspace(3)
; COM: VAR - IMM pattern where VAR is positive
; CHECK:       lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{.*}}-0x100]:a32
  %out_3 = load i32, ptr addrspace(3) %ptr_3, align 4
  store i32 %out_3, ptr addrspace(1) %out

  %offset_4 = sub nsw i32 %negativeOffset, -1020
  %ptr_4 = inttoptr i32 %offset_4 to ptr addrspace(3)
; COM: VAR - (-IMM) pattern where VAR is negative
; D-CHECK:      lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{.*}}+0x3fc]:a32
; R-CHECK-NOT:  lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{.*}}+0x3fc]:a32
; R-CHECK:      lsc_load.slm (M1_NM, 1)  {{.*}}:d32t  flat[{{[^+-]*}}]:a32
  %out_4 = load i32, ptr addrspace(3) %ptr_4, align 4
  store i32 %out_4, ptr addrspace(1) %out

  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!12}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !7, !8, !9}
!12 = !{ptr @test, !13}
!13 = !{!14, !15}
!14 = !{!"function_type", i32 0}
!15 = !{!"implicit_arg_desc", !18}
!18 = !{i32 15, !19}
!19 = !{!"explicit_arg_num", i32 3}
