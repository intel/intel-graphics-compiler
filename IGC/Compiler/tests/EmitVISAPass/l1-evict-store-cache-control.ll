;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test that emit fence keeps L1 evict when the default L1 store policy is not write-back, but a
; store in the kernel has its own write-back or write-streaming L1 cache control, and the fence
; is wider than the work-group.

; RUN: igc_opt --opaque-pointers %s -platformbmg -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s

; CHECK-LABEL: .kernel "store_wb"
; CHECK:       lsc_fence.ugm.evict.gpu
define spir_kernel void @store_wb() {
  store i32 1, ptr addrspace(1) null, align 4, !lsc.cache.ctrl !100
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

; CHECK-LABEL: .kernel "fence_before_lsc_store_wb"
; CHECK:       lsc_fence.ugm.evict.gpu
define spir_kernel void @fence_before_lsc_store_wb() {
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  call void @llvm.genx.GenISA.LSCStore.p1.i32(ptr addrspace(1) null, i32 0, i32 1, i32 3, i32 1, i32 7)
  ret void
}

; CHECK-LABEL: .kernel "block_write_ws"
; CHECK:       lsc_fence.ugm.evict.gpu
define spir_kernel void @block_write_ws() {
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 8191, i32 7, i32 8191, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 6, <8 x i32> zeroinitializer)
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

; CHECK-LABEL: .kernel "store_wb_group_scope"
; CHECK:       lsc_fence.ugm.none.group
define spir_kernel void @store_wb_group_scope() {
  store i32 1, ptr addrspace(1) null, align 4, !lsc.cache.ctrl !100
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "store_wbp"
; CHECK:       lsc_fence.ugm.none.gpu
define spir_kernel void @store_wbp() {
  store i32 1, ptr addrspace(1) null, align 4, !lsc.cache.ctrl !101
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

; CHECK-LABEL: .kernel "private_store_wb"
; CHECK:       lsc_fence.ugm.none.gpu
define spir_kernel void @private_store_wb() {
  store i32 1, ptr null, align 4, !lsc.cache.ctrl !100
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1, i32)
declare void @llvm.genx.GenISA.LSCStore.p1.i32(ptr addrspace(1), i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!IGCMetadata = !{!7}

!0 = !{ptr @store_wb, !6}
!1 = !{ptr @fence_before_lsc_store_wb, !6}
!2 = !{ptr @block_write_ws, !6}
!3 = !{ptr @store_wb_group_scope, !6}
!4 = !{ptr @store_wbp, !6}
!5 = !{ptr @private_store_wb, !6}
!6 = !{!{!"function_type", i32 0}}

!7 = !{!"ModuleMD", !8}
!8 = !{!"compOpt", !9}
!9 = !{!"StoreCacheDefault", i32 2}  ; store - LSC_L1UC_L3C_WB (2) - L1 uncached, L3 write-back

!100 = !{i32 7} ; LSC_L1IAR_WB_L3C_WB (7) - L1 write-back, L3 write-back
!101 = !{i32 2} ; LSC_L1UC_L3C_WB     (2) - L1 uncached,   L3 write-back
