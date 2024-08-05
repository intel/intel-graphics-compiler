;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 -inputrt -regkey DumpVISAASMToConsole -S %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

define spir_kernel void @test(<64 x i32> addrspace(1)* %src, <32 x i64> addrspace(2)* %dst) {
entry:
  %a = load <64 x i32>, <64 x i32> addrspace(1)* %src
  %b = bitcast <64 x i32> %a to <32 x i64>
; CHECK: mov (M1_NM, 16) b_0(0,0)<1> a(0,0)<1;1,0>
; CHECK: mov (M1_NM, 16) b_0(1,0)<1> a(1,0)<1;1,0>
; CHECK: mov (M1_NM, 16) b_0(2,0)<1> a(2,0)<1;1,0>
; CHECK: mov (M1_NM, 16) b_0(3,0)<1> a(3,0)<1;1,0>
  store <32 x i64> %b, <32 x i64> addrspace(2)* %dst
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (<64 x i32> addrspace(1)*, <32 x i64> addrspace(2)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (<64 x i32> addrspace(1)*, <32 x i64> addrspace(2)*)* @test}
!6 = !{!"FuncMDValue[0]"}
