;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Lane id + delta below twice the SIMD size selects from current or next and
; must be kept; only larger indices may wrap into the combined data. The pass
; side is in Compiler/tests/RegisterAccessBoundsChecks/simd-shuffle-down.ll.

; REQUIRES: regkeys, cri-supported
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1,ForceRegisterAccessBoundsChecks=1'" 2>&1 | FileCheck %s

; A variable delta wraps lane id + delta, not the delta.
; CHECK-LABEL: .kernel "shuffle_down"
; CHECK-NOT:   min (
; CHECK:       add (M1_NM, 16) [[IDX:[A-z0-9]+]](0,0)<1> {{(LaneId|V[0-9]+)}}(0,0)<1;1,0> {{(delta|V[0-9]+)}}(0,0)<0;1,0>
; CHECK-NEXT:  and (M1_NM, 16) [[IDX]](0,0)<1> [[IDX]](0,0)<1;1,0> 0x1f:ud
; CHECK-NEXT:  shl (M1_NM, 16) {{[A-z0-9]+}}(0,0)<1> [[IDX]](0,0)<1;1,0> 0x2:ud
; CHECK-NEXT:  addr_add (M1_NM, 16) A0(0)<1> &{{[A-z0-9]+}} {{[A-z0-9_]+}}(0,0)
; CHECK-NEXT:  mov {{.*}} r[A0(0),0]
define spir_kernel void @shuffle_down(ptr addrspace(1) %out, i32 %cur, i32 %next, i32 %delta) !intel_reqd_sub_group_size !1 {
  %shuffled = call spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32 %cur, i32 %next, i32 %delta)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

; Shuffle up passes SIMD size - delta, wrapped when delta exceeds the SIMD size,
; so its in-range lanes are kept by the same wrap.
; CHECK-LABEL: .kernel "shuffle_up"
; CHECK:       add (M1_NM, 1) [[DOWN:[A-z0-9]+]](0,0)<1> 0x10:w (-){{(delta|V[0-9]+)}}(0,0)<0;1,0>
; CHECK:       add (M1_NM, 16) [[IDX:[A-z0-9]+]](0,0)<1> {{(LaneId|V[0-9]+)}}(0,0)<1;1,0> [[DOWN]](0,0)<0;1,0>
; CHECK-NEXT:  and (M1_NM, 16) [[IDX]](0,0)<1> [[IDX]](0,0)<1;1,0> 0x1f:ud
define spir_kernel void @shuffle_up(ptr addrspace(1) %out, i32 %prev, i32 %cur, i32 %delta) !intel_reqd_sub_group_size !1 {
  %shuffled = call spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32 %prev, i32 %cur, i32 %delta)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: .kernel "shuffle_down_simd32"
; CHECK-NOT:   min (
; CHECK:       add (M1_NM, 32) [[IDX:[A-z0-9]+]](0,0)<1> {{(LaneId|V[0-9]+)}}(0,0)<1;1,0> {{(delta|V[0-9]+)}}(0,0)<0;1,0>
; CHECK-NEXT:  and (M1_NM, 32) [[IDX]](0,0)<1> [[IDX]](0,0)<1;1,0> 0x3f:ud
; CHECK-NEXT:  shl (M1_NM, 32) {{[A-z0-9]+}}(0,0)<1> [[IDX]](0,0)<1;1,0> 0x2:ud
; CHECK:       addr_add (M1, 32) A0(0)<1> &{{[A-z0-9]+}} {{[A-z0-9_]+}}(0,0)
; CHECK-NEXT:  mov {{.*}} r[A0(0),0]
define spir_kernel void @shuffle_down_simd32(ptr addrspace(1) %out, i32 %cur, i32 %next, i32 %delta) !intel_reqd_sub_group_size !2 {
  %shuffled = call spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32 %cur, i32 %next, i32 %delta)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: .kernel "shuffle_up_simd32"
; CHECK:       add (M1_NM, 1) [[DOWN:[A-z0-9]+]](0,0)<1> 0x20:w (-){{(delta|V[0-9]+)}}(0,0)<0;1,0>
; CHECK:       add (M1_NM, 32) [[IDX:[A-z0-9]+]](0,0)<1> {{(LaneId|V[0-9]+)}}(0,0)<1;1,0> [[DOWN]](0,0)<0;1,0>
; CHECK-NEXT:  and (M1_NM, 32) [[IDX]](0,0)<1> [[IDX]](0,0)<1;1,0> 0x3f:ud
define spir_kernel void @shuffle_up_simd32(ptr addrspace(1) %out, i32 %prev, i32 %cur, i32 %delta) !intel_reqd_sub_group_size !2 {
  %shuffled = call spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32 %prev, i32 %cur, i32 %delta)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

declare spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32, i32, i32)
declare spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32, i32, i32)

!opencl.ocl.version = !{!0}
!0 = !{i32 2, i32 0}
!1 = !{i32 16}
!2 = !{i32 32}
