;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-advmemopt -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; AdvMemOpt - checks early exit due to memory writes
; ------------------------------------------------

; CHECK-LABEL: @test(
; CHECK:       bb:
; CHECK:         call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B:%[A-z0-9]*]], i64 4, i1 true, i32 42){{.*}}, !uniform [[TRUE_MD:![0-9]*]]
; CHECK-NEXT:    br label %bb1
; CHECK:      bb1:
; CHECK-NEXT:   store i32 0, i32* [[B]], align 4
; CHECK-NEXT:   call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B]], i64 4, i1 true, i32 43){{.*}}, !uniform [[TRUE_MD]]
; CHECK:        ret void

define void @test(i32 %a, i32* %b) {
entry:
  %0 = icmp slt i32 %a, 13
  br i1 %0, label %bb, label %end

bb:
  %1 = phi i32 [ 0, %entry ], [ %3, %bb1 ]
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 42), !uniform !4
  br label %bb1

bb1:
  store i32 0, i32* %b, align 4
  %c = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 43), !uniform !4
  %3 = add i32 %1, %2
  %4 = add i32 %c, %3
  %5 = icmp slt i32 %3, %a
  br i1 %5, label %bb, label %end

end:
  %6 = phi i32 [ %a, %entry ], [ %4, %bb1 ]
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %b, i32 %6, i64 4, i1 true)
  ret void
}

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32*, i32, i64, i1)

attributes #0 = { nounwind readonly }

; CHECK: [[TRUE_MD]] = !{i1 true}

!igc.functions = !{!0}

!0 = !{void (i32, i32*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
