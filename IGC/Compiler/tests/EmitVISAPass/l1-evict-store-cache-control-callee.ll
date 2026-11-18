;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test that emit fence keeps L1 evict when the default L1 store policy is not write-back, but a
; store in the function group of the fence has its own write-back L1 cache control (in a callee,
; or in the kernel when the fence is in a callee), or a callee is in another module.

; RUN: igc_opt --opaque-pointers %s -GenXCodeGenModule -platformbmg -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s
; RUN: igc_opt --opaque-pointers %s -GenXCodeGenModule -platformbmg -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EXTERNAL
; RUN: igc_opt --opaque-pointers %s -GenXCodeGenModule -platformbmg -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=FENCE-IN-CALLEE

; CHECK-LABEL: .kernel "call_store_wb"
; CHECK:       call
; CHECK-NEXT:  lsc_fence.ugm.evict.gpu
define spir_kernel void @call_store_wb() {
  call spir_func void @store_wb()
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

define internal spir_func void @store_wb() {
  store i32 1, ptr addrspace(1) null, align 4, !lsc.cache.ctrl !7
  ret void
}

; FENCE-IN-CALLEE-LABEL: .kernel "store_wb_call_fence"
; FENCE-IN-CALLEE:       call (M1, 16) [[FENCE:fence_[0-9]+]]
; FENCE-IN-CALLEE:       [[FENCE]]:
; FENCE-IN-CALLEE-NEXT:  lsc_fence.ugm.evict.gpu
define spir_kernel void @store_wb_call_fence() {
  store i32 1, ptr addrspace(1) null, align 4, !lsc.cache.ctrl !7
  call spir_func void @fence()
  ret void
}

define internal spir_func void @fence() {
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

; EXTERNAL-LABEL: .kernel "call_external"
; EXTERNAL:       ifcall
; EXTERNAL-NEXT:  lsc_fence.ugm.evict.gpu
define spir_kernel void @call_external(ptr %privateBase) {
  call spir_func void @external()
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

declare spir_func void @external() "referenced-indirectly" "visaStackCall"

declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1, i32)

!igc.functions = !{!0, !1, !14, !15, !8}
!IGCMetadata = !{!4}

!0 = !{ptr @call_store_wb, !2}
!1 = !{ptr @store_wb, !3}
!14 = !{ptr @store_wb_call_fence, !2}
!15 = !{ptr @fence, !3}
!8 = !{ptr @call_external, !2}
!2 = !{!{!"function_type", i32 0}}
!3 = !{!{!"function_type", i32 2}}

!4 = !{!"ModuleMD", !5, !9}
!5 = !{!"compOpt", !6}
!6 = !{!"StoreCacheDefault", i32 2}  ; store - LSC_L1UC_L3C_WB (2) - L1 uncached, L3 write-back
!9 = !{!"FuncMD", !10, !11}
!10 = !{!"FuncMDMap[0]", ptr @call_external}
!11 = !{!"FuncMDValue[0]", !12}
!12 = !{!"implicitArgInfoList", !13}
!13 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 13}}  ; PRIVATE_BASE, needed by stack calls

!7 = !{i32 7} ; LSC_L1IAR_WB_L3C_WB (7) - L1 write-back, L3 write-back
