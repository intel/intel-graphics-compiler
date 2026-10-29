;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -GenXCodeGenModule -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=4 | FileCheck %s

; In a leaf stack-call function, emitStackFuncEntry remaps the argument's
; symbol to an alias of ARGV after the VRA alias variables are created.
; An extractelement from the argument must not be aliased to the argument,
; otherwise it reads the original argument variable, which is never written.

; CHECK-LABEL: .global_function "stack_callee"
; CHECK:       .decl [[ARGV:ARGV_[0-9]+v]] v_type=G type=d num_elts=192 {{.*}} alias=<%arg, 0>
; CHECK:       lsc_store.ugm (M1, 16) flat[{{.*}}]:a64 [[ARGV]].128:d32

define spir_kernel void @test(<8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %in = load <8 x i32>, ptr addrspace(1) null, align 32
  call spir_func void @stack_callee(<8 x i32> %in)
  ret void
}

define internal spir_func void @stack_callee(<8 x i32> %v) "visaStackCall" {
entry:
  %e = extractelement <8 x i32> %v, i32 2
  store i32 %e, ptr addrspace(1) null, align 4
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
