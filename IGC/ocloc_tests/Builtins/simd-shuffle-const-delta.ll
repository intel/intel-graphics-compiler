;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Lane id + delta below twice the SIMD size selects from current or next, so a
; constant delta past the SIMD size still has in-range lanes, read with direct
; regions inside the combined data.

; REQUIRES: regkeys, dg2-supported
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

; A delta of the SIMD size reads all of next.
; CHECK-LABEL: .kernel "shuffle_down_const_simd_size"
; CHECK:       .input [[CUR:(cur|V[0-9]+)]] offset=40
; CHECK:       .input [[NEXT:(next|V[0-9]+)]] offset=44
; CHECK-NOT:   addr_add
; CHECK:       mov (M1_NM, 16) [[TMP:(ShuffleTmp|V[0-9]+)]](0,0)<1> [[CUR]](0,0)
; CHECK-NEXT:  mov (M1_NM, 16) [[TMP]](2,0)<1> [[NEXT]](0,0)
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](2,0)<1;1,0>
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](3,0)<1;1,0>
; CHECK-NOT:   addr_add
; CHECK:       ret
define spir_kernel void @shuffle_down_const_simd_size(ptr addrspace(1) %out, i32 %cur, i32 %next) !intel_reqd_sub_group_size !1 {
  %shuffled = call spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32 %cur, i32 %next, i32 16)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

; A larger delta reads next from element 4 for its in-range lanes 0-11.
; CHECK-LABEL: .kernel "shuffle_down_const_past_simd_size"
; CHECK:       .input [[CUR:(cur|V[0-9]+)]] offset=40
; CHECK:       .input [[NEXT:(next|V[0-9]+)]] offset=44
; CHECK-NOT:   addr_add
; CHECK:       mov (M1_NM, 16) [[TMP:(ShuffleTmp|V[0-9]+)]](0,0)<1> [[NEXT]](0,0)
; CHECK-NEXT:  mov (M1_NM, 16) [[TMP]](2,0)<1> [[CUR]](0,0)
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](0,4)<1;1,0>
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](1,4)<1;1,0>
; CHECK-NOT:   addr_add
; CHECK:       ret
define spir_kernel void @shuffle_down_const_past_simd_size(ptr addrspace(1) %out, i32 %cur, i32 %next) !intel_reqd_sub_group_size !1 {
  %shuffled = call spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32 %cur, i32 %next, i32 20)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

; Shuffle up by 20 is shuffle down by 16 - 20; in-range lanes 4-15 read previous.
; CHECK-LABEL: .kernel "shuffle_up_const_past_simd_size"
; CHECK:       .input [[PREV:(prev|V[0-9]+)]] offset=40
; CHECK:       .input [[CUR:(cur|V[0-9]+)]] offset=44
; CHECK-NOT:   addr_add
; CHECK:       mov (M1_NM, 16) [[TMP:(ShuffleTmp|V[0-9]+)]](0,0)<1> [[CUR]](0,0)
; CHECK-NEXT:  mov (M1_NM, 16) [[TMP]](2,0)<1> [[PREV]](0,0)
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](1,4)<1;1,0>
; CHECK-NEXT:  mov (M1, 8) {{.*}} [[TMP]](2,4)<1;1,0>
; CHECK-NOT:   addr_add
; CHECK:       ret
define spir_kernel void @shuffle_up_const_past_simd_size(ptr addrspace(1) %out, i32 %prev, i32 %cur) !intel_reqd_sub_group_size !1 {
  %shuffled = call spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32 %prev, i32 %cur, i32 20)
  store i32 %shuffled, ptr addrspace(1) %out
  ret void
}

declare spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32, i32, i32)
declare spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32, i32, i32)

!opencl.ocl.version = !{!0}
!0 = !{i32 2, i32 0}
!1 = !{i32 16}
