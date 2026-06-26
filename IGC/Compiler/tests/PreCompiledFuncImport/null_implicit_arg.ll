;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Regression test for the implicit-arg resolution in replaceFunc().
;
; When a double-emulation builtin is imported as a subroutine and it uses
; private memory, PreCompiledFuncImport appends the caller's R0 / PRIVATE_BASE
; implicit args to every call. When the caller is a non-entry function that does
; not itself carry those implicit args in its signature, getImplicitArg() returns
; null and the value must be materialized as an intrinsic (getR0 / getPrivateBase)
; instead of pushing a null operand.
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=2 \
; RUN:            --platformdg2 --igc-precompiled-import -S 2>&1 | FileCheck %s
;
; Without the fix, getImplicitArg() returns null for the non-entry caller and the
; null is pushed verbatim, producing "<null operand!>" and a broken-function abort.
; In the non-entry caller the missing implicit args are materialized as intrinsics
; and forwarded to the emulation call (never a null operand).
; CHECK-LABEL: define internal void @sqrt_helper
; CHECK-DAG: call <8 x i32> @llvm.genx.GenISA.getR0
; CHECK-DAG: call ptr @llvm.genx.GenISA.getPrivateBase
; CHECK: call double @__igcbuiltin_dp_sqrt(double {{.*}}, i32 {{.*}}, i32 {{.*}}, i32 {{.*}}, ptr {{.*}}, <8 x i32> {{.*}}, ptr {{.*}})
;
; The emulation subroutine gains trailing R0 / PRIVATE_BASE implicit params.
; CHECK: define internal double @__igcbuiltin_dp_sqrt(double {{.*}}, i32 {{.*}}, i32 {{.*}}, i32 {{.*}}, ptr {{.*}}, <8 x i32> {{.*}}, ptr {{.*}})

; Function Attrs: convergent nounwind
define spir_kernel void @test_entry(double addrspace(1)* %Dst, double addrspace(1)* %Src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) #0 {
entry:
  call void @sqrt_helper(double addrspace(1)* %Dst, double addrspace(1)* %Src)
  ret void
}

; Non-entry helper: no R0 / privateBase in its signature.
; Function Attrs: convergent nounwind
define internal void @sqrt_helper(double addrspace(1)* %Dst, double addrspace(1)* %Src) #0 {
entry:
  %0 = load double, double addrspace(1)* %Src, align 8
  %call.i.i1 = call double @llvm.sqrt.f64(double %0)
  store double %call.i.i1, double addrspace(1)* %Dst, align 8
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare double @llvm.sqrt.f64(double) #2

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #2 = { nounwind readnone speculatable }

; Only the kernel is registered (as an entry). The helper is deliberately not
; listed, so it is a non-entry function with an empty implicit-arg list -- which
; is what forces getImplicitArg() to return null for it in replaceFunc().
!igc.functions = !{!20}

!20 = !{void (double addrspace(1)*, double addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test_entry, !21}
!21 = !{!22}
!22 = !{!"function_type", i32 0}
