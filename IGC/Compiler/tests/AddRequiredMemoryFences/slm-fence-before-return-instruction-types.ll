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
define void @f1(ptr addrspace(3) %address) {
Label-0:
  store i32 0, ptr addrspace(3) %address
  ret void
}
; CHECK-LABEL: define void @f1
; CHECK:      store i32 0, ptr addrspace(3) %address
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK:      ret void

; Test that pass made no changes, all stores are fenced.
define void @f2(ptr addrspace(3) %address) {
Label-0:
  store i32 0, ptr addrspace(3) %address
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  ret void
}
; CHECK-LABEL: define void @f2
; CHECK:      store i32 0, ptr addrspace(3) %address
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK:      ret void

; Test that pass made no changes, all stores are fenced.
define void @f3(ptr addrspace(3) %address) {
Label-0:
  store i32 0, ptr addrspace(3) %address
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
  ret void
}
; CHECK-LABEL: define void @f3
; CHECK:      store i32 0, ptr addrspace(3) %address
; CHECK-NEXT: call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK:      ret void

; Test that pass added an SLM fence after the atomic since the result
; of the atomic operation is not used
define void @f7(ptr addrspace(3) %address) {
Label-0:
  %a = call i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3) %address, i32 0, i32 0, i32 0)
  ret void
}
; CHECK-LABEL: define void @f7
; CHECK:      %a = call i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3) %address, i32 0, i32 0, i32 0)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NEXT: ret void

; Test that no fence is added since the result of the atomic operation is used.
define i32 @f8(ptr addrspace(3) %address) {
Label-0:
  %a = call i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3) %address, i32 0, i32 0, i32 0)
  ret i32 %a
}
; CHECK-LABEL: define i32 @f8
; CHECK:      %a = call i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3) %address, i32 0, i32 0, i32 0)
; CHECK-NOT:  call void @llvm.genx.GenISA.LSCFence({{.*}})
; CHECK-NEXT: ret i32 %a


; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32) #2
declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1) #2
; Function Attrs: argmemonly nounwind
declare i32 @llvm.genx.GenISA.intatomicraw.i32.p3(ptr addrspace(3), i32, i32, i32) #3

attributes #1 = { "null-pointer-is-valid"="true" }
attributes #2 = { convergent nounwind }
attributes #3 = { argmemonly nounwind }
attributes #4 = { nounwind }
