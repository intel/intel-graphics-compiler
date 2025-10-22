;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies PreprocessSPVIR retypes TargetExtTy inside structs used in
; function argument sret attributes.

%union.anon = type { ptr addrspace(1) }
%"class.sycl::_V1::multi_ptr" = type { ptr addrspace(3) }
%"class.sycl::_V1::device_event" = type { target("spirv.Event") }

; CHECK-NOT: target("spirv.Event")
; CHECK: %"class.sycl::_V1::device_event" = type { ptr }

define void @f(ptr addrspace(4) noalias sret(%"class.sycl::_V1::device_event") align 8 %arg1, ptr addrspace(4) align 1 %arg2, ptr byval(%"class.sycl::_V1::multi_ptr") align 8 %arg3, ptr byval(%union.anon) align 8 %arg4, i64 %arg5) {
  call spir_func void @g(ptr addrspace(4) noalias sret(%"class.sycl::_V1::device_event") align 8 %arg1, ptr addrspace(4) align 1 %arg2, ptr byval(%"class.sycl::_V1::multi_ptr") align 8 %arg3, ptr byval(%union.anon) align 8 %arg4, i64 1, i64 1)
; CHECK: call spir_func void @g(ptr addrspace(4) noalias sret(%"class.sycl::_V1::device_event") align 8 %arg1, ptr addrspace(4) align 1 %arg2, ptr byval(%"class.sycl::_V1::multi_ptr") align 8 %arg3, ptr byval(%union.anon) align 8 %arg4, i64 1, i64 1)
  ret void
}

define void @g(ptr addrspace(4) noalias sret(%"class.sycl::_V1::device_event") align 8 %arg1, ptr addrspace(4) align 1 %arg2, ptr byval(%"class.sycl::_V1::multi_ptr") align 8 %arg3, ptr byval(%union.anon) align 8 %arg4, i64 %arg5, i64 %arg6) {
  ret void
}
