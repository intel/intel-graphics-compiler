;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; SIMD32 shuffle down with a constant delta is split into two SIMD16 movs. The
; second one writes lanes 16-31 and must take their channel enables, which
; matters when lanes 0-15 and 16-31 are enabled differently.

; REQUIRES: regkeys, bmg-supported
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

; CHECK-LABEL: .kernel "shuffle_down_const_divergent"
; CHECK:       goto (M1, 32)
; CHECK-NOT:   addr_add
; CHECK:       mov (M1_NM, 32) [[TMP:(ShuffleTmp|V[0-9]+)]](0,0)<1>
; CHECK-NEXT:  mov (M1_NM, 32) [[TMP]](2,0)<1>
; CHECK-NEXT:  mov (M1, 16) {{.*}}(0,0)<1> [[TMP]](0,1)<1;1,0>
; CHECK-NEXT:  mov (M5, 16) {{.*}}(1,0)<1> [[TMP]](1,1)<1;1,0>
; CHECK-NOT:   addr_add
; CHECK:       ret
define spir_kernel void @shuffle_down_const_divergent(ptr addrspace(1) %out, i32 %cur, i32 %next) !intel_reqd_sub_group_size !1 {
entry:
  %lane = call spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()
  %cmp = icmp ugt i32 %lane, 7
  br i1 %cmp, label %then, label %exit

then:
  %shuffled = call spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32 %lane, i32 0, i32 1)
  br label %exit

exit:
  %res = phi i32 [ %shuffled, %then ], [ -1, %entry ]
  %idx = zext i32 %lane to i64
  %ptr = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
  store i32 %res, ptr addrspace(1) %ptr
  ret void
}

declare spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()
declare spir_func i32 @_Z32__spirv_SubgroupShuffleDownINTELiij(i32, i32, i32)

!opencl.ocl.version = !{!0}
!0 = !{i32 2, i32 0}
!1 = !{i32 32}
