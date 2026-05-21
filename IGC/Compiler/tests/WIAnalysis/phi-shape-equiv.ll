;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output --regkey=PrintToConsole=1,EnableWIPhiStructuralEquivalence=1 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,EQUIV
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output --regkey=PrintToConsole=1,EnableWIPhiStructuralEquivalence=0 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,LEGACY

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Two structurally-identical incomings at a divergent join. The branch is
; driven by LocalID_X (RANDOM), so updatePHIDepAtJoin would normally
; downgrade %phi to RANDOM. With EnableWIPhiStructuralEquivalence=1 the
; new predicate proves path-invariance from shape alone and keeps %phi
; uniform_global. With =0 we fall through to the legacy downgrade.
; ============================================================================
define spir_kernel void @test_shape_equiv_flat(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %div_cmp = icmp ult i32 %tid, 32
  br i1 %div_cmp, label %A, label %B

A:
  %a = add i32 %K1, %K2
  br label %join

B:
  %b = add i32 %K1, %K2
  br label %join

join:
  %phi = phi i32 [%a, %A], [%b, %B]
  store i32 %phi, ptr addrspace(1) %out, align 4
  ret void
}

; ============================================================================
; Recursive shape match: phi(mul(add(K1,K2), K3), mul(add(K1,K2), K3)).
; Exercises the recursive descent of areShapeEquivalentAcrossPaths.
; ============================================================================
define spir_kernel void @test_shape_equiv_recursive(i32 %K1, i32 %K2, i32 %K3, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %div_cmp = icmp ult i32 %tid, 32
  br i1 %div_cmp, label %A, label %B

A:
  %a1 = add i32 %K1, %K2
  %a = mul i32 %a1, %K3
  br label %join

B:
  %b1 = add i32 %K1, %K2
  %b = mul i32 %b1, %K3
  br label %join

join:
  %phi = phi i32 [%a, %A], [%b, %B]
  store i32 %phi, ptr addrspace(1) %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1, !2}

!0 = !{!"ModuleMD", !20}
!1 = !{ptr @test_shape_equiv_flat, !30}
!2 = !{ptr @test_shape_equiv_recursive, !30}
!20 = !{!"FuncMD", !21, !22, !23, !24}
!21 = !{!"FuncMDMap[0]", ptr @test_shape_equiv_flat}
!22 = !{!"FuncMDValue[0]", !40, !41, !42, !43}
!23 = !{!"FuncMDMap[1]", ptr @test_shape_equiv_recursive}
!24 = !{!"FuncMDValue[1]", !40, !41, !42, !43}
!30 = !{!31}
!31 = !{!"function_type", i32 0}
!40 = !{!"localOffsets"}
!41 = !{!"workGroupWalkOrder", !50, !51, !52}
!42 = !{!"funcArgs"}
!43 = !{!"functionType", !"KernelFunction"}
!50 = !{!"dim0", i32 0}
!51 = !{!"dim1", i32 1}
!52 = !{!"dim2", i32 2}

; Flat case ------------------------------------------------------------------
; CHECK-LABEL: define spir_kernel void @test_shape_equiv_flat
; CHECK: random {{.*}}%tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32
; CHECK: random {{.*}}%div_cmp = icmp ult
; CHECK: uniform_global {{.*}}%a = add i32 %K1, %K2
; CHECK: uniform_global {{.*}}%b = add i32 %K1, %K2
; EQUIV:  uniform_global {{.*}}%phi = phi i32
; LEGACY: random {{.*}}%phi = phi i32

; Recursive case -------------------------------------------------------------
; CHECK-LABEL: define spir_kernel void @test_shape_equiv_recursive
; CHECK: uniform_global {{.*}}%a = mul i32 %a1, %K3
; CHECK: uniform_global {{.*}}%b = mul i32 %b1, %K3
; EQUIV:  uniform_global {{.*}}%phi = phi i32
; LEGACY: random {{.*}}%phi = phi i32
