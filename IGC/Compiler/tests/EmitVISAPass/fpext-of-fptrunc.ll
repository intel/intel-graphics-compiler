;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1 %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; CHECK-LABEL: .function "_main_0"
; CHECK:         lsc_load.ugm (M1, 32)  [[SRC:[A-Za-z0-9_]+]]:d32
; CHECK-NEXT:    mov (M1, 32) [[TMP:[A-Za-z0-9_]*_fptrunc]](0,0)<2> [[SRC]](0,0)<1;1,0>
; CHECK-NEXT:    mov (M1, 32) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[TMP]](0,0)<2;1,0>
; CHECK-NEXT:    mul (M1, 32) [[PRODUCT:[A-Za-z0-9_]+]](0,0)<1> [[DST]](0,0)<1;1,0> {{[A-Za-z0-9_]+}}(0,0)<0;1,0>
; CHECK:         lsc_store.ugm (M1, 32) {{.*}} [[PRODUCT]]:d32
;
; CHECK:       // .decl [[SRC]] v_type=G type=f num_elts=32 align=wordx32
; CHECK-NEXT:  // .decl [[DST]] v_type=G type=f num_elts=32 align=wordx32
; CHECK-NEXT:  // .decl [[TMP]] v_type=G type=hf num_elts=64 align=wordx32

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %dst, float %factor) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %offset = zext i16 %lane to i64
  %address = getelementptr float, ptr addrspace(1) %src, i64 %offset
  %value = load float, ptr addrspace(1) %address, align 4
  %truncated = fptrunc float %value to half
  %extended = fpext half %truncated to float
  %product = fmul float %extended, %factor
  %out = getelementptr float, ptr addrspace(1) %dst, i64 %offset
  store float %product, ptr addrspace(1) %out, align 4
  ret void
}

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
!8 = !{!"argAllocMDList", !9, !13, !14}
!9 = !{!"argAllocMDListVec[0]", !10, !11, !12}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[1]", !10, !11, !12}
!14 = !{!"argAllocMDListVec[2]", !10, !11, !12}
