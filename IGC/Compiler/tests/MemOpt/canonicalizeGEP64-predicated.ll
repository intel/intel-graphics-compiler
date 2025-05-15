;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-memopt -instcombine | FileCheck %s

define void @f0(i32 %arg1, i32 %arg2, ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %or1 = or i32 %arg1, 1
  %add1 = add nsw i32 %or1, %arg2
  %sext1 = sext i32 %add1 to i64
  %add2 = add nsw i64 %sext1, -1
  %gep1 = getelementptr inbounds double, ptr addrspace(1) %src, i64 %add2
  %load1 = call double @llvm.genx.GenISA.PredicatedLoad.f64.p1.f64(ptr addrspace(1) %gep1, i64 8, i1 true, double 4.2)
  %gep2 = getelementptr inbounds double, ptr addrspace(1) %src, i64 %sext1
  %load2 = call double @llvm.genx.GenISA.PredicatedLoad.f64.p1.f64(ptr addrspace(1) %gep2, i64 8, i1 true, double 0.5)

; Check that load instructions were merged
; CHECK:    {{.*}} = call <2 x double> @llvm.genx.GenISA.PredicatedLoad.v2f64.p1.v2f64(ptr addrspace(1) {{.*}}, i64 8, i1 true, <2 x double> <double 4.200000e+00, double 5.000000e-01>)

  %add3 = fadd fast double %load1, %load2
  store double %add3, ptr addrspace(1) %dst, align 8
  ret void
}

; Function Attrs: nounwind readonly
declare double @llvm.genx.GenISA.PredicatedLoad.f64.p1.f64(ptr addrspace(1), i64, i1, double) #0

attributes #0 = { nounwind readonly }

!igc.functions = !{!0}

!0 = !{ptr @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
