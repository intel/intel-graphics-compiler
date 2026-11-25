;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole,EnableWideMulMad < %s | FileCheck %s

; CodeGenPatternMatch::matchWideMul64Pair emits a mul/WideSMulHi pair at the
; later of the two; it must not pair when that would define the mul after a use.

; Fusing at the hi call would define the mul after lo_user reads it.
; CHECK-LABEL: .kernel "lo_used_before_hi"
; CHECK: mov (M1_NM, 1) lo_0v(0,0)<1>
; CHECK: add (M1_NM, 1) lo_user(0,0)<1> lo(0,0)<0;1,0>
define spir_kernel void @lo_used_before_hi(i64 %x, ptr addrspace(1) %lo_out, ptr addrspace(1) %hi_out) {
entry:
  %lo = mul i64 %x, %x
  %lo_user = add i64 %lo, 7
  store i64 %lo_user, ptr addrspace(1) %lo_out, align 4
  %hi = call i64 @llvm.genx.GenISA.WideSMulHi(i64 %x, i64 %x)
  store i64 %hi, ptr addrspace(1) %hi_out, align 4
  ret void
}

; Fusing in hi_block would leave the mul undefined on the path that skips it.
; CHECK-LABEL: .kernel "hi_in_conditional_block"
; CHECK: mov (M1_NM, 1) lo_0v(0,0)<1>
; CHECK: _hi_in_conditional_block_001_hi_block:
define spir_kernel void @hi_in_conditional_block(i64 %x, ptr addrspace(1) %lo_out, ptr addrspace(1) %hi_out) {
entry:
  %lo = mul i64 %x, %x
  %cond = icmp ult i64 %x, 7
  br i1 %cond, label %hi_block, label %exit

hi_block:
  %hi = call i64 @llvm.genx.GenISA.WideSMulHi(i64 %x, i64 %x)
  store i64 %hi, ptr addrspace(1) %hi_out, align 4
  br label %exit

exit:
  store i64 %lo, ptr addrspace(1) %lo_out, align 4
  ret void
}

declare i64 @llvm.genx.GenISA.WideSMulHi(i64, i64)

!IGCMetadata = !{!0}
!igc.functions = !{!7, !9}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !6, !3}
!2 = !{!"FuncMDMap[0]", ptr @lo_used_before_hi}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !10, !10, !10}
!6 = !{!"FuncMDMap[1]", ptr @hi_in_conditional_block}
!7 = !{ptr @lo_used_before_hi, !8}
!8 = !{!11}
!9 = !{ptr @hi_in_conditional_block, !8}
!10 = !{!"argAllocMDListVec[0]"}
!11 = !{!"function_type", i32 0}
