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
; Checks that call graph updated correctly and hasRecursion attribute set
;

; CHECK: define internal spir_func void @func4({{.*}}[[ATTR1:#[0-9]*]]
; CHECK-NOT: define
; CHECK: bitcast %"struct.ispc::Renderer" addrspace(4)* {{.*}} to %"struct.ispc::Renderer.542" addrspace(4)*
; CHECK: bitcast %"struct.ispc::World" addrspace(4)* {{.*}} to %"struct.ispc::World.563" addrspace(4)*
; CHECK: call spir_func void @func1(
; CHECK: attributes [[ATTR1]] = { {{.*}}"hasRecursion" "visaStackCall"{{.*}} }
; CHECK-NOT: attributes [[ATTR1]] = { {{.*}}"referenced-indirectly"{{.*}} }

%"struct.ispc::Renderer" = type { i32, void (%"struct.ispc::Renderer" addrspace(4)*, %"struct.ispc::World" addrspace(4)*, i8 addrspace(4)*)*, i32, i32, float, i32 }
%"struct.ispc::World" = type { %"struct.ispc::Instance" addrspace(4)* addrspace(4)*, i32 }
%"struct.ispc::Instance" = type { i32, float, float, %"struct.ispc::Renderer" addrspace(4)*, i32, i32 }
%structtype = type {}
%"struct.ispc::Renderer.542" = type { i32, %structtype*, i32, i32, float, i32 }
%"struct.ispc::World.563" = type { %"struct.ispc::Instance.559" addrspace(4)* addrspace(4)*, i32 }
%"struct.ispc::Instance.559" = type { i32, float, float, %"struct.ispc::Renderer.542" addrspace(4)*, i32, i32 }

define spir_func void @func1(%"struct.ispc::Renderer.542" addrspace(4)* %_self, %"struct.ispc::World.563" addrspace(4)* %world, i8 addrspace(4)* %perFrameData) {
entry:
  call spir_func void bitcast (void (%"struct.ispc::Renderer" addrspace(4)*, %"struct.ispc::World" addrspace(4)*)* @func3 to void (%"struct.ispc::Renderer.542" addrspace(4)*, %"struct.ispc::World.563" addrspace(4)*)*)(%"struct.ispc::Renderer.542" addrspace(4)* %_self, %"struct.ispc::World.563" addrspace(4)* %world)
  ret void
}

define spir_func void @func3(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0) {
entry:
  call spir_func void @func4(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0)
  ret void
}

define spir_func void @func4(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0) {
entry:
  call spir_func void bitcast (void (%"struct.ispc::Renderer.542" addrspace(4)*, %"struct.ispc::World.563" addrspace(4)*, i8 addrspace(4)*)* @func1 to void (%"struct.ispc::Renderer" addrspace(4)*, %"struct.ispc::World" addrspace(4)*, i8 addrspace(4)*)*)(%"struct.ispc::Renderer" addrspace(4)* %super.i, %"struct.ispc::World" addrspace(4)* %a.0, i8 addrspace(4)* null)
  ret void
}
