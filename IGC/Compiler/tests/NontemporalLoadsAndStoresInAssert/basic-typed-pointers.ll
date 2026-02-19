;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-nontemporal-loads-and-stores-in-assert -S < %s 2>&1 | FileCheck %s

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test() #0 {
entry:
  call spir_func void @__devicelib_assert_fail(i8 addrspace(2)* null, i8 addrspace(2)* null, i32 2, i8 addrspace(2)* null, i64 1, i64 2, i64 3, i64 1, i64 2, i64 3) #0
  ret void
}

; Function Attrs: alwaysinline builtin convergent nounwind
define internal spir_func void @__devicelib_assert_fail(i8 addrspace(2)* %expr, i8 addrspace(2)* %file, i32 %line, i8 addrspace(2)* %func, i64 %gid0, i64 %gid1, i64 %gid2, i64 %lid0, i64 %lid1, i64 %lid2) #21{
entry:
  %call1 = tail call spir_func i8 addrspace(1)* @__builtin_IB_get_assert_buffer() #2
  %flag = getelementptr inbounds i8, i8 addrspace(1)* %call1, i64 4
  %0 = bitcast i8 addrspace(1)* %flag to i32 addrspace(1)*
  %1 = addrspacecast i32 addrspace(1)* %0 to i32 addrspace(4)*
  store i32 1, i32 addrspace(4)* %1, align 4
; CHECK: store i32 1, i32 addrspace(4)* %1, {{.*}} !nontemporal ![[MDNODE:[0-9]+]]
  ret void
}

; Function Attrs: convergent
declare spir_func i8 addrspace(1)* @__builtin_IB_get_assert_buffer() local_unnamed_addr #4

attributes #0 = { nounwind }
attributes #1 = { alwaysinline builtin convergent nounwind }
attributes #2 = { convergent nounwind }

; CHECK: ![[MDNODE]] = !{i32 1}
