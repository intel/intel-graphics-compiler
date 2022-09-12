;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-fold-workgroup-sizes -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FoldKnownWorkGroupSizes
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_foldws_global() {
; CHECK-LABEL: @test_foldws_global(
; CHECK:    [[TMP1:%.*]] = call spir_func i32 @__builtin_IB_get_global_offset(i32 12)
; CHECK:    call void @use.i32(i32 0)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_get_global_offset(i32 12)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_foldws_local() {
; CHECK-LABEL: @test_foldws_local(
; CHECK:    [[TMP1:%.*]] = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0)
; CHECK:    call void @use.i32(i32 13)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0)
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i32(i32)

;  %2 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0)
;  store i32 %1, i32* %1, align 4

declare spir_func i32 @__builtin_IB_get_global_offset(i32)

declare spir_func i32 @__builtin_IB_get_enqueued_local_size(i32)

!IGCMetadata = !{!0}
!igc.functions = !{!3, !6}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"replaceGlobalOffsetsByZero", i1 true}
!3 = !{void ()* @test_foldws_global, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{void ()* @test_foldws_local, !7}
!7 = !{!5, !8}
!8 = !{!"thread_group_size", i32 13, i32 2, i32 3}
