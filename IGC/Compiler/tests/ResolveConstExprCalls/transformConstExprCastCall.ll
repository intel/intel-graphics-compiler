;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-resolve-constexpr-calls --igc-process-func-attributes -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks that constexpr bitcast calls transformed to arguments bitcasts
; and not get "referenced-indirectly" attribute
;

; CHECK: define spir_kernel void @kernel_(
; CHECK-NOT: define
; CHECK: bitcast %"struct.ispc::Renderer" addrspace(4)* {{.*}} to %"struct.ispc::Renderer.542" addrspace(4)*
; CHECK: bitcast %"struct.ispc::World" addrspace(4)* {{.*}} to %"struct.ispc::World.563" addrspace(4)*
; CHECK: call spir_func void @func_Renderer(
; CHECK: define internal spir_func void @func_Renderer({{.*}}[[ATTR:#[0-9]*]]
; CHECK-NOT: attributes [[ATTR]] = { {{.*}}"referenced-indirectly"{{.*}} }


%"struct.ispc::Renderer" = type { i32, void (%"struct.ispc::Renderer" addrspace(4)*, %"struct.ispc::World" addrspace(4)*, i8 addrspace(4)*)*, i32, i32, float, i32 }
%"struct.ispc::World" = type { %"struct.ispc::Instance" addrspace(4)* addrspace(4)*, i32 }
%"struct.ispc::Instance" = type { i32, float, float, %"struct.ispc::Renderer" addrspace(4)*, i32, i32 }
%structtype = type {}
%"struct.ispc::Renderer.542" = type { i32, %structtype*, i32, i32, float, i32 }
%"struct.ispc::World.563" = type { %"struct.ispc::Instance.559" addrspace(4)* addrspace(4)*, i32 }
%"struct.ispc::Instance.559" = type { i32, float, float, %"struct.ispc::Renderer.542" addrspace(4)*, i32, i32 }

define spir_kernel void @kernel_(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0) {
entry:
  call spir_func void bitcast (void (%"struct.ispc::Renderer.542" addrspace(4)*, %"struct.ispc::World.563" addrspace(4)*, i8 addrspace(4)*)* @func_Renderer to void (%"struct.ispc::Renderer" addrspace(4)*, %"struct.ispc::World" addrspace(4)*, i8 addrspace(4)*)*)(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0, i8 addrspace(4)* null)
  ret void
}

define spir_func void @func_Renderer(%"struct.ispc::Renderer.542" addrspace(4)* %_self, %"struct.ispc::World.563" addrspace(4)* %world, i8 addrspace(4)* %perFrameData) #0 {
entry:
  ret void
}

attributes #0 = { alwaysinline nounwind }
