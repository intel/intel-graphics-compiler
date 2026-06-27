;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole --CheckInstrTypes --enable-instrtypes-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CheckInstrTypes sets SInstrTypes::hasDPAS when a GenISA DPAS intrinsic is present.
; ------------------------------------------------

; CHECK: hasDPAS: 1
; CHECK: numCall: 1

define spir_kernel void @dpas_func(i32 %acc, i16 %a, <8 x i32> %b, ptr %dst) {
  %r = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %acc, i16 %a, <8 x i32> %b, i32 9, i32 9, i32 8, i32 1, i1 false)
  store i32 %r, ptr %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32, i16, <8 x i32>, i32, i32, i32, i32, i1)

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD"}
!1 = !{ptr @dpas_func, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
