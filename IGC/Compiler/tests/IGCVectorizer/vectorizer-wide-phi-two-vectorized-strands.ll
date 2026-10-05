;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

define spir_kernel void @test_phi_two_strands(ptr addrspace(1) %srcA, ptr addrspace(1) %srcB, ptr addrspace(1) %dst16, ptr addrspace(1) %dstA, ptr addrspace(1) %dstB, i1 %cond) {
; CHECK-LABEL: @test_phi_two_strands(
; CHECK:       loop:
; CHECK:         [[PHI:%.*]] = phi <16 x float> [ zeroinitializer, %entry ], [ [[GATHER:%.*]], %loop ]
; the high strand becomes an 8 wide fmul over the second load
; CHECK:         [[FMUL:%.*]] = fmul <8 x float> %loadB, {{.*}}
; CHECK:         [[HI0:%.*]] = extractelement <8 x float> [[FMUL]], i32 0
; the low strand becomes an 8 wide fadd over the first load
; CHECK:         [[FADD:%.*]] = fadd <8 x float> %loadA, {{.*}}
; CHECK:         [[LO0:%.*]] = extractelement <8 x float> [[FADD]], i32 0
; CHECK:         store <16 x float> [[PHI]], ptr addrspace(1) %dst16
; the gather stays <16 x float>, lanes 0-7 are taken from the fadd and lanes
; 8-15 from the fmul, it is not replaced by either of the narrow binaries
; CHECK:         insertelement <16 x float> undef, float [[LO0]], i32 0
; CHECK:         insertelement <16 x float> {{.*}}, float [[HI0]], i32 8
; CHECK:         [[GATHER]] = insertelement <16 x float> {{.*}}, i32 15
; CHECK:         store <8 x float> [[FADD]], ptr addrspace(1) %dstA
; CHECK:         store <8 x float> [[FMUL]], ptr addrspace(1) %dstB
entry:
  %loadA = load <8 x float>, ptr addrspace(1) %srcA, align 32
  %loadB = load <8 x float>, ptr addrspace(1) %srcB, align 32
  br label %loop

loop:                                             ; preds = %loop, %entry
  %phi0 = phi float [ 0.000000e+00, %entry ], [ %next0, %loop ]
  %phi1 = phi float [ 0.000000e+00, %entry ], [ %next1, %loop ]
  %phi2 = phi float [ 0.000000e+00, %entry ], [ %next2, %loop ]
  %phi3 = phi float [ 0.000000e+00, %entry ], [ %next3, %loop ]
  %phi4 = phi float [ 0.000000e+00, %entry ], [ %next4, %loop ]
  %phi5 = phi float [ 0.000000e+00, %entry ], [ %next5, %loop ]
  %phi6 = phi float [ 0.000000e+00, %entry ], [ %next6, %loop ]
  %phi7 = phi float [ 0.000000e+00, %entry ], [ %next7, %loop ]
  %phi8 = phi float [ 0.000000e+00, %entry ], [ %next8, %loop ]
  %phi9 = phi float [ 0.000000e+00, %entry ], [ %next9, %loop ]
  %phi10 = phi float [ 0.000000e+00, %entry ], [ %next10, %loop ]
  %phi11 = phi float [ 0.000000e+00, %entry ], [ %next11, %loop ]
  %phi12 = phi float [ 0.000000e+00, %entry ], [ %next12, %loop ]
  %phi13 = phi float [ 0.000000e+00, %entry ], [ %next13, %loop ]
  %phi14 = phi float [ 0.000000e+00, %entry ], [ %next14, %loop ]
  %phi15 = phi float [ 0.000000e+00, %entry ], [ %next15, %loop ]
  %acc0 = insertelement <16 x float> zeroinitializer, float %phi0, i64 0
  %acc1 = insertelement <16 x float> %acc0, float %phi1, i64 1
  %acc2 = insertelement <16 x float> %acc1, float %phi2, i64 2
  %acc3 = insertelement <16 x float> %acc2, float %phi3, i64 3
  %acc4 = insertelement <16 x float> %acc3, float %phi4, i64 4
  %acc5 = insertelement <16 x float> %acc4, float %phi5, i64 5
  %acc6 = insertelement <16 x float> %acc5, float %phi6, i64 6
  %acc7 = insertelement <16 x float> %acc6, float %phi7, i64 7
  %acc8 = insertelement <16 x float> %acc7, float %phi8, i64 8
  %acc9 = insertelement <16 x float> %acc8, float %phi9, i64 9
  %acc10 = insertelement <16 x float> %acc9, float %phi10, i64 10
  %acc11 = insertelement <16 x float> %acc10, float %phi11, i64 11
  %acc12 = insertelement <16 x float> %acc11, float %phi12, i64 12
  %acc13 = insertelement <16 x float> %acc12, float %phi13, i64 13
  %acc14 = insertelement <16 x float> %acc13, float %phi14, i64 14
  %acc15 = insertelement <16 x float> %acc14, float %phi15, i64 15
  store <16 x float> %acc15, ptr addrspace(1) %dst16, align 64

  %a0 = extractelement <8 x float> %loadA, i64 0
  %a1 = extractelement <8 x float> %loadA, i64 1
  %a2 = extractelement <8 x float> %loadA, i64 2
  %a3 = extractelement <8 x float> %loadA, i64 3
  %a4 = extractelement <8 x float> %loadA, i64 4
  %a5 = extractelement <8 x float> %loadA, i64 5
  %a6 = extractelement <8 x float> %loadA, i64 6
  %a7 = extractelement <8 x float> %loadA, i64 7
  %next0 = fadd float %a0, 1.000000e+00
  %next1 = fadd float %a1, 1.000000e+00
  %next2 = fadd float %a2, 1.000000e+00
  %next3 = fadd float %a3, 1.000000e+00
  %next4 = fadd float %a4, 1.000000e+00
  %next5 = fadd float %a5, 1.000000e+00
  %next6 = fadd float %a6, 1.000000e+00
  %next7 = fadd float %a7, 1.000000e+00

  %b0 = extractelement <8 x float> %loadB, i64 0
  %b1 = extractelement <8 x float> %loadB, i64 1
  %b2 = extractelement <8 x float> %loadB, i64 2
  %b3 = extractelement <8 x float> %loadB, i64 3
  %b4 = extractelement <8 x float> %loadB, i64 4
  %b5 = extractelement <8 x float> %loadB, i64 5
  %b6 = extractelement <8 x float> %loadB, i64 6
  %b7 = extractelement <8 x float> %loadB, i64 7
  %next8 = fmul float %b0, 2.000000e+00
  %next9 = fmul float %b1, 2.000000e+00
  %next10 = fmul float %b2, 2.000000e+00
  %next11 = fmul float %b3, 2.000000e+00
  %next12 = fmul float %b4, 2.000000e+00
  %next13 = fmul float %b5, 2.000000e+00
  %next14 = fmul float %b6, 2.000000e+00
  %next15 = fmul float %b7, 2.000000e+00

  %va0 = insertelement <8 x float> zeroinitializer, float %next0, i64 0
  %va1 = insertelement <8 x float> %va0, float %next1, i64 1
  %va2 = insertelement <8 x float> %va1, float %next2, i64 2
  %va3 = insertelement <8 x float> %va2, float %next3, i64 3
  %va4 = insertelement <8 x float> %va3, float %next4, i64 4
  %va5 = insertelement <8 x float> %va4, float %next5, i64 5
  %va6 = insertelement <8 x float> %va5, float %next6, i64 6
  %va7 = insertelement <8 x float> %va6, float %next7, i64 7
  store <8 x float> %va7, ptr addrspace(1) %dstA, align 32

  %vb0 = insertelement <8 x float> zeroinitializer, float %next8, i64 0
  %vb1 = insertelement <8 x float> %vb0, float %next9, i64 1
  %vb2 = insertelement <8 x float> %vb1, float %next10, i64 2
  %vb3 = insertelement <8 x float> %vb2, float %next11, i64 3
  %vb4 = insertelement <8 x float> %vb3, float %next12, i64 4
  %vb5 = insertelement <8 x float> %vb4, float %next13, i64 5
  %vb6 = insertelement <8 x float> %vb5, float %next14, i64 6
  %vb7 = insertelement <8 x float> %vb6, float %next15, i64 7
  store <8 x float> %vb7, ptr addrspace(1) %dstB, align 32
  br i1 %cond, label %loop, label %exit

exit:                                             ; preds = %loop
  ret void
}

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), i1)* @test_phi_two_strands, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), ptr addrspace(1), i1)* @test_phi_two_strands}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
