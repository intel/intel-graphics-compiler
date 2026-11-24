;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole,EnableWideMulMad < %s | FileCheck %s

; mul i64 paired with GenISA.WideUMulHi (CodeGenPatternMatch::matchWideMul64Pair)
; must multiply the high dwords as unsigned.

; CHECK-DAG: .decl [[SRC_UD:[A-Za-z0-9_]+]] v_type=G type=ud num_elts=2 {{.*}}alias=<src, 0>
; CHECK-DAG: .decl [[TMP0_UD:[A-Za-z0-9_]+]] v_type=G type=ud num_elts=2 {{.*}}alias=<mul_hiwideTmp0, 0>
; CHECK-DAG: .decl [[TMP1_UD:[A-Za-z0-9_]+]] v_type=G type=ud num_elts=2 {{.*}}alias=<mul_hiwideTmp1, 0>
; CHECK-DAG: .decl [[HI:mul_hiwideHi]] v_type=G type=uq
; CHECK: mad (M1_NM, 1) [[HI]](0,0)<1> [[SRC_UD]](0,1)<0;1,0> [[SRC_UD]](0,1)<0;1,0> [[TMP0_UD]](0,1)<2;1,0>
; CHECK: add (M1_NM, 1) [[HI]](0,0)<1> [[HI]](0,0)<0;1,0> [[TMP1_UD]](0,1)<2;1,0>

define spir_kernel void @test(i64 %src, ptr addrspace(1) %lo, ptr addrspace(1) %hi) {
entry:
  %mul.lo = mul i64 %src, %src
  %mul.hi = call i64 @llvm.genx.GenISA.WideUMulHi(i64 %src, i64 %src)
  store i64 %mul.lo, ptr addrspace(1) %lo, align 8
  store i64 %mul.hi, ptr addrspace(1) %hi, align 8
  ret void
}

declare i64 @llvm.genx.GenISA.WideUMulHi(i64, i64)

!IGCMetadata = !{!0}
!igc.functions = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !6, !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 0}
!8 = !{ptr @test, !9}
!9 = !{!10}
!10 = !{!"function_type", i32 0}
