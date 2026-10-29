;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -GenXCodeGenModule -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=4 | FileCheck %s

; In a stack-call function, emitImplicitArgIntrinsic maps getR0's symbol to
; the predefined %r0 after the VRA alias variables are created. An
; extractelement from getR0 must not be aliased to it, otherwise it reads the
; original variable, which is never written.

; CHECK-LABEL: .global_function "stack_callee"
; CHECK:       .decl [[R0A:[A-Za-z0-9_]+]] v_type=G type=d num_elts=8 {{.*}} alias=<%r0, 0>
; CHECK:       mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[R0A]](0,2)<0;1,0>

define spir_kernel void @test(<8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  call spir_func void @stack_callee()
  ret void
}

define internal spir_func void @stack_callee() "visaStackCall" {
entry:
  %r = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
  %tid = extractelement <8 x i32> %r, i32 2
  store i32 %tid, ptr addrspace(1) null, align 4
  ret void
}

declare <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

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
