;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -annotate_uniform_allocas -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; AnnotateUniformAllocas
; ------------------------------------------------
;
; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 3
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(i32 %a) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = alloca i32, align 4, {{.*}} !uniform [[TRUE_MD:![0-9]*]], !UseAssumeUniform [[TRUE_MD]]
; CHECK:    [[TMP2:%.*]] = alloca i32, align 4, {{.*}} !uniform [[TRUE_MD]]
; CHECK:    store i32 [[A:%.*]], i32* [[TMP1]]
; CHECK:    store i32 13, i32* [[TMP2]]
; CHECK:    ret void
;
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  call void @llvm.genx.GenISA.assume.uniform.p0i32(i32* %1)
  store i32 %a, i32* %1
  store i32 13, i32* %2
  ret void
}

; CHECK: [[TRUE_MD]] = !{i1 true}

declare void @llvm.genx.GenISA.assume.uniform.p0i32(i32*)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!1}
!1 = !{void (i32)* @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
