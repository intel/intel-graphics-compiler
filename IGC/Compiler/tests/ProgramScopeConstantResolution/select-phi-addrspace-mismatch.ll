;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --verify --opaque-pointers --igc-programscope-constant-analysis --igc-programscope-constant-resolve -S < %s 2>&1 | FileCheck %s

; @gVar is lowered from addrspace(0) to a GEP off %constBase, in addrspace(2),
; and cast back so select and phi hands keep matching types.
;
; CHECK-LABEL: @test_kernel
; CHECK: %offgVar = getelementptr i8, ptr addrspace(2) %constBase, i64 0
; CHECK: %castgVar = addrspacecast ptr addrspace(2) %offgVar to ptr
; CHECK: %sel = select i1 %c, ptr %p, ptr %castgVar

@gVar = internal global [128 x i8] zeroinitializer, align 32

define spir_kernel void @test_kernel(ptr %p, i1 %c, ptr addrspace(2) %constBase) {
entry:
  %sel = select i1 %c, ptr %p, ptr @gVar
  %v = load <8 x float>, ptr %sel, align 32
  ret void
}

; CHECK-LABEL: @test_kernel_gep_phi
; CHECK: %offgVar = getelementptr i8, ptr addrspace(2) %constBase, i64 0
; CHECK: %castgVar = addrspacecast ptr addrspace(2) %offgVar to ptr
; CHECK: %g = getelementptr inbounds [128 x i8], ptr %castgVar, i64 0, i64 32
; CHECK: %selg = select i1 %c, ptr %p, ptr %g
; CHECK: %ph = phi ptr [ %castgVar, %entry ], [ %g, %bb ]

define spir_kernel void @test_kernel_gep_phi(ptr %p, i1 %c, ptr addrspace(2) %constBase) {
entry:
  %g = getelementptr inbounds [128 x i8], ptr @gVar, i64 0, i64 32
  %selg = select i1 %c, ptr %p, ptr %g
  br i1 %c, label %bb, label %join

bb:
  br label %join

join:
  %ph = phi ptr [ @gVar, %entry ], [ %g, %bb ]
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!10, !12}

!0 = !{!"ModuleMD", !1, !4}
!1 = !{!"inlineProgramScopeOffsets", !2, !3}
!2 = !{!"inlineProgramScopeOffsetsMap[0]", ptr @gVar}
!3 = !{!"inlineProgramScopeOffsetsValue[0]", i64 0}
!4 = !{!"FuncMD", !5, !6, !8, !9}
!5 = !{!"FuncMDMap[0]", ptr @test_kernel}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"implicitArgInfoList", !20}
!8 = !{!"FuncMDMap[1]", ptr @test_kernel_gep_phi}
!9 = !{!"FuncMDValue[1]", !7}

!10 = !{ptr @test_kernel, !11}
!11 = !{!{!"function_type", i32 0}}
!12 = !{ptr @test_kernel_gep_phi, !11}

!20 = !{!"implicitArgInfoListVec[0]", !21}
!21 = !{!"argId", i32 11}
