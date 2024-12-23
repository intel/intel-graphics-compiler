;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S --platformdg2 --inputcs --igc-add-required-memory-fences | FileCheck %s


; Test that pass added an SLM fence after the store.
define void @f2(ptr addrspace(3) %address, i1 %cond) {
Label-0:
  br i1 %cond, label %Label-True, label %Label-End
Label-True:
  store i32 0, ptr addrspace(3) %address
  br label %Label-End
Label-End:
  ret void
}
; CHECK-LABEL: define void @f2
; CHECK-LABEL: Label-True:
; CHECK-NEXT: store i32 0, ptr addrspace(3) %address
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK:      ret void

; Test that pass added an SLM fence in the last basic block (common post dominator)
define void @f3(ptr addrspace(3) %address, i1 %cond) {
Label-0:
  store i32 0, ptr addrspace(3) %address
  br i1 %cond, label %Label-True, label %Label-End
Label-True:
  store i32 0, ptr addrspace(3) %address
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

; Test that pass made no changes, all stores are fenced.
define void @f4(ptr addrspace(3) %address, i1 %cond) {
Label-0:
  br i1 %cond, label %Label-True, label %Label-End
Label-True:
  store i32 0, ptr addrspace(3) %address
  br label %Label-End
Label-End:
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  ret void
}
; CHECK-LABEL: define void @f4
; CHECK-LABEL: Label-True:
; CHECK:      store i32 0, ptr addrspace(3) %address
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK-LABEL: Label-End:
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
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
