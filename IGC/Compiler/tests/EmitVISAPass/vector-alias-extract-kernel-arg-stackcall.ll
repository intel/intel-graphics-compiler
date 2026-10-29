;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -GenXCodeGenModule -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=4 | FileCheck %s
; RUN: igc_opt %s --opaque-pointers -GenXCodeGenModule -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=9 | FileCheck %s

; When an OpenCL kernel has stack calls, the uses of the kernel argument are
; moved to the copy after the VRA alias variables are created. An extractelement
; from such an argument must not be aliased to it, otherwise it reads the
; payload register, which a stack call can overwrite.

; CHECK-LABEL: .kernel "test"
; CHECK:       mov (M1_NM, 8) [[COPY:[A-Za-z0-9_]+]](0,0)<1> a29(0,0)<1;1,0>
; CHECK:       fcall
; CHECK:       [[COPY]](0,3)<0;1,0>

define spir_kernel void @test(<16 x i32> %a0, <16 x i32> %a1, <16 x i32> %a2, <16 x i32> %a3, <16 x i32> %a4, <16 x i32> %a5, <16 x i32> %a6, <16 x i32> %a7, <16 x i32> %a8, <16 x i32> %a9, <16 x i32> %a10, <16 x i32> %a11, <16 x i32> %a12, <16 x i32> %a13, <16 x i32> %a14, <16 x i32> %a15, <16 x i32> %a16, <16 x i32> %a17, <16 x i32> %a18, <16 x i32> %a19, <16 x i32> %a20, <16 x i32> %a21, <16 x i32> %a22, <16 x i32> %a23, <16 x i32> %a24, <16 x i32> %a25, <16 x i32> %a26, <16 x i32> %a27, <16 x i32> %a28, <16 x i32> %a29, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %e = extractelement <16 x i32> %a29, i32 3
  call spir_func void @stack_callee()
  store i32 %e, ptr addrspace(1) null, align 4
  ret void
}

define internal spir_func void @stack_callee() "visaStackCall" {
entry:
  store i32 0, ptr addrspace(1) null, align 4
  ret void
}

!igc.functions = !{!0, !1}
!0 = !{ptr @test, !2}
!1 = !{ptr @stack_callee, !3}
!2 = !{!{!"function_type", i32 0}}
!3 = !{!{!"function_type", i32 2}}

!IGCMetadata = !{!4}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", ptr @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"implicitArgInfoList", !9, !10, !11}
!9 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!10 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!11 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 13}}
