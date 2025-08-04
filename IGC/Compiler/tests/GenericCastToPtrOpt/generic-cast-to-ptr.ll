;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --igc-generic-cast-to-ptr-opt --regkey EnableGenericCastToPtrOpt=1 -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; GenericCastToPtrOpt
; ------------------------------------------------

; Check that calls to GenericCastToPtrExplicit_ToGlobal are correctly transformed to addrsspacecasts

declare spir_func void @testExternFn(i8 addrspace(1)* noundef)

declare spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef)

declare spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef)

define internal spir_func i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci(i8 addrspace(4)* noundef %0, i32 noundef %1) {
  %3 = tail call spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef %0) #18
  %4 = icmp eq i8 addrspace(3)* %3, null

  %5 = tail call spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef %0) #18
  %6 = icmp eq i8* %5, null

  %7 = and i1 %4, %6
  %8 = addrspacecast i8 addrspace(4)* %0 to i8 addrspace(1)*
  %9 = select i1 %7, i8 addrspace(1)* %8, i8 addrspace(1)* null
  ret i8 addrspace(1)* %9
}

; CHECK-LABEL: testFn
; CHECK-NOT: %result = call spir_func i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci(i8 addrspace(4)* %ptr, i32 0)
; CHECK: %generic_cast_to_ptr = addrspacecast [[PTR:.*]] addrspace(4){{.*}} %ptr to [[PTR]] addrspace(1){{.*}}
; CHECK: call spir_func void @testExternFn([[PTR]] addrspace(1){{.*}} %generic_cast_to_ptr)
define spir_kernel void @testFn() {
  %ptr = inttoptr i64 0 to i8 addrspace(4)*
  %result = call spir_func i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci(i8 addrspace(4)* %ptr, i32 0)
  call spir_func void @testExternFn(i8 addrspace(1)* %result)
  ret void
}
