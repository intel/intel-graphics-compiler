;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S --platformdg2 --inputcs --igc-add-required-memory-fences | FileCheck %s


; Test that pass added an SLM fence in the last basic block
define void @f3(ptr addrspace(3) %address, i1 %cond, i1 %cond1) {
Label-0:
  store i32 0, ptr addrspace(3) %address
  br i1 %cond, label %Label-True, label %Label-Merge
Label-True:
  store i32 0, ptr addrspace(3) %address
  br i1 %cond1, label %Label-Unreachable, label %Label-Merge
Label-Unreachable:
  unreachable
Label-Merge:
  br label %Label-End
Label-End:
  ret void
}
; CHECK-LABEL: define void @f3
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK-LABEL: Label-End:
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK:      ret void



; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32) #2
; Function Attrs: argmemonly nounwind
declare i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3), i32, i32, i32) #3

attributes #1 = { "null-pointer-is-valid"="true" }
attributes #2 = { convergent nounwind }
attributes #3 = { argmemonly nounwind }
attributes #4 = { nounwind }
