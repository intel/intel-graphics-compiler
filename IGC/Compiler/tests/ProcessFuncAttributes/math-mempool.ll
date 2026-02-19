;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-process-func-attributes -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks that alwaysinline attibute is set for FastRelaxMath and MemPool

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; CHECK: ; Function Attrs: alwaysinline
; CHECK: define internal void @test_prfuncattr(i32 %src, i32* %dst){{.*}} [[ATTR:#[0-9]*]]

@__bif_flag_FastRelaxedMath = constant i1 true

define void @test_prfuncattr(i32 %src, i32* %dst) {
entry:
  %0 = load i1, i1* @__bif_flag_FastRelaxedMath
  br i1 %0, label %bb1, label %end

bb1:
  store i32 %src, i32* %dst
  br label %end

end:
  ret void
}

; CHECK: ; Function Attrs: alwaysinline
; CHECK: define internal void @foo(i8 %src){{.*}} [[ATTR]]

; CHECK: attributes [[ATTR]] = { alwaysinline }

define void @foo(i8 %src) {
  %1 = call i8* @__builtin_IB_AllocLocalMemPool(i1 true, i32 16, i32 20)
  store i8 %src, i8* %1
  ret void
}

declare i8* @__builtin_IB_AllocLocalMemPool(i1, i32, i32)

