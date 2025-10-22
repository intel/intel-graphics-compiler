;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies PreprocessSPVIR retypes TargetExtTy inside structs used by
; GEPs.

; CHECK-NOT: target("spirv.Event")

%"class.sycl::_V1::device_event" = type { target("spirv.Event") }
; CHECK: %"class.sycl::_V1::device_event" = type { ptr }

define internal spir_func void @_ZN4sycl3_V112device_event4waitEv(ptr addrspace(4) align 8 %arg) {
  %a = alloca ptr addrspace(4), align 8
  %b = addrspacecast ptr %a to ptr addrspace(4)
  store ptr addrspace(4) %arg, ptr addrspace(4) %b, align 8
  %c = load ptr addrspace(4), ptr addrspace(4) %b, align 8
; CHECK: %d = getelementptr inbounds %"class.sycl::_V1::device_event", ptr addrspace(4) %c, i32 0, i32 0
  %d = getelementptr inbounds %"class.sycl::_V1::device_event", ptr addrspace(4) %c, i32 0, i32 0
; CHECK: %e = getelementptr inbounds %"class.sycl::_V1::device_event", ptr addrspace(4) %c, i32 0, i32 0
  %e = getelementptr inbounds %"class.sycl::_V1::device_event", ptr addrspace(4) %c, i32 0, i32 0
  ret void
}
