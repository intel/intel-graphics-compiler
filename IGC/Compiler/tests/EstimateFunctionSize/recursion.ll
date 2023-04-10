;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt -regkey PrintFunctionSizeAnalysis=1  --EstimateFunctionSize -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; EstimateFunctionSize
; ------------------------------------------------

; CHECK: Unit size (kernel entry) test_estimate: 16
; CHECK: Function count= 2
; CHECK: Kernel count= 1
; CHECK: Manual stack call count= 0
; CHECK: Address taken function call count= 0

; Fix - debug counts for head size

define spir_kernel void @test_estimate(i32* %s) {
entry:
  %0 = alloca i32, align 4
  %1 = call spir_func i32 @foo(i32* %s)
  br label %end

end:                                              ; preds = %entry
  %2 = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId()
  %3 = add i32 %2, %1
  store i32 %3, i32* %0, align 4
  ret void
}

define spir_func i32 @foo(i32* %a) {
entry:
  %0 = load i32, i32* %a, align 4
  %1 = ptrtoint i32* %a to i32
  %2 = icmp slt i32 %0, %1
  br i1 %2, label %end, label %continue

continue:                                         ; preds = %entry
  %3 = inttoptr i32 %0 to i32*
  %4 = call spir_func i32 @foo(i32* %3)
  br label %end

end:                                              ; preds = %continue, %entry
  %5 = phi i32 [ %1, %entry ], [ %4, %continue ]
  ret i32 %5
}

declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId()

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0, !3}
!0 = !{void (i32*)* @test_estimate, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i32 (i32*)* @foo, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 2}

