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
; RUN:   | FileCheck %s --check-prefixes=CHECK,ENABLED

; RUN: igc_opt %s \
; RUN:     --opaque-pointers \
; RUN:     -GenXCodeGenModule \
; RUN:     -igc-emit-visa \
; RUN:     -platformbmg \
; RUN:     -simd-mode 16 \
; RUN:     -regkey DumpVISAASMToConsole,DisableUniformAnalysis=1 \
; RUN:   | FileCheck %s --check-prefixes=CHECK,DISABLED

; In a stack-called function r0 comes from the intrinsic rather than from an
; implicit argument, and emitImplicitArgIntrinsic aliases the intrinsic's symbol
; onto the predefined %r0 register. The intrinsic's symbol must remain a shared
; eight-dword value whether or not WIAnalysis ran.

; CHECK-LABEL: .global_function "stack_callee"
; CHECK:       .decl [[R0A:[A-Za-z0-9_]+]] v_type=G type=d num_elts=8 {{.*}} alias=<%r0, 0>
; DISABLED:    .decl [[TID:[A-Za-z0-9_]+]] v_type=G type=d num_elts=16
; ENABLED:     [[R0A]](0,2)<0;1,0>
; DISABLED:    mov (M1, 16) [[TID]](0,0)<1> [[R0A]](0,2)<0;1,0>
; DISABLED:    lsc_store.ugm (M1, 16) {{.*}} [[TID]]:d32

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
