;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; To test setting stackcall for DP math function, such as pow, sin, etc if DP
; emulation is needed.
;   note: attribute #1 will not include "visaStackCall" when using igc_opt.
;         When igc_opt's output will include "visaStackCall", this test shall
;         test 'visaStackCall"
;
; REQUIRES: regkeys
;
; RUN: igc_opt %s -ocl -regkey EnableDPEmulation=1 \
; RUN:            --platformdg2 --igc-process-func-attributes -S \
; RUN:   | FileCheck %s

; CHECK-LABEL: define spir_kernel void @test_ddiv
; CHECK:       ret void
; CHECK:       {{; Function Attrs: }}
; CHECK-NOT:   alwaysinline
; CHECK-SAME:  nounwind
; CHECK:       define internal spir_func double @__ocl_svml_pow(double %a, double %b) local_unnamed_addr [[ATTR1:.*]] {
; CHECK:       attributes [[ATTR1]]
; CHECK-SAME:  visaStackCall

; Function Attrs: convergent nounwind
define spir_kernel void @test_ddiv(double addrspace(1)* %dst, <2 x double> addrspace(1)* %src, i32 %ix) #0 {
entry:
  %idxprom = zext i32 %ix to i64
  %arrayidx = getelementptr inbounds <2 x double>, <2 x double> addrspace(1)* %src, i64 %idxprom
  %0 = load <2 x double>, <2 x double> addrspace(1)* %arrayidx, align 16
  %.scalar = extractelement <2 x double> %0, i32 0
  %.scalar1 = extractelement <2 x double> %0, i32 1
  %call1.i = call spir_func double @__ocl_svml_pow(double %.scalar, double %.scalar1) #2
  %idxprom1 = zext i32 %ix to i64
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %dst, i64 %idxprom1
  store double %call1.i, double addrspace(1)* %arrayidx2, align 8
  ret void
}

; For testing double builtin, pow() body does not matter. Removing it to make the test smaller.
;
; Function Attrs: convergent nounwind readnone
define internal spir_func double @__ocl_svml_pow(double %a, double %b) local_unnamed_addr #1 {
entry:
  ret double %a
}

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone "OclBuiltin" }
attributes #2 = { nounwind }

!igc.functions = !{!302, !305}

!302 = !{double (double, double)* @__ocl_svml_pow, !303}
!303 = !{!304}
!304 = !{!"function_type", i32 2}
!305 = !{void (double addrspace(1)*, <2 x double> addrspace(1)*, i32)* @test_ddiv, !306}
!306 = !{!307}
!307 = !{!"function_type", i32 0}
