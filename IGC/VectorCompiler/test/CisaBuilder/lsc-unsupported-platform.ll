;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; COM: Verify that using an LSC intrinsic on a platform without LSC support
; COM: produces a recoverable compiler error instead of crashing the process.
; COM: This tests the fix introduced in GSD-7796, where VC erroneously called
; COM: exit(1) on compilation errors. The fix made GenXDiagnostic emit a proper
; COM: error message and exit with a non-zero code, allowing plain 'not' instead
; COM: of 'not --crash' to detect the failure.

; RUN: not llc %s -march=genx64 -mcpu=XeLP -vc-skip-ocl-runtime-info -o /dev/null 2>&1 \
; RUN: | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: error: LLVM ERROR: GenXCisaBuilder failed for:
; CHECK-SAME: @llvm.vc.internal.lsc.load.ugm
; CHECK-SAME: Intrinsic is not supported by the <XeLP> platform

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <1 x i32>)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i64 %base) local_unnamed_addr {
  %res = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(
      <1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer,
      i64 0, i64 %base, i16 1, i32 0, <1 x i32> undef)
  ret void
}

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i64)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i64)* @the_test, null, null, null, null}
