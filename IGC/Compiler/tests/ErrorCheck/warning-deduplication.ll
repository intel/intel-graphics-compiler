;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt --opaque-pointers -igc-error-check --platformdg2 -S 2>&1 < %s \
; RUN:   --regkey DisableDuplicateWarnings=0 --regkey ForceDPEmulation=1 | FileCheck %s --check-prefixes=CHECK,CHECK_DUP
; RUN: igc_opt --opaque-pointers -igc-error-check --platformdg2 -S 2>&1 < %s \
; RUN:  --regkey DisableDuplicateWarnings=1 --regkey ForceDPEmulation=1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; This LIT verifies proper deduplication with an use of DisableDuplicateWarnings flag.

define void @fp64_warning_deduplication_test(
  double addrspace(1)* align 8 %src,
  double addrspace(1)* align 8 %dst) {
; CHECK-LABEL: define void @fp64_warning_deduplication_test(
; CHECK: [[TMP1:%.*]] = load double, ptr addrspace(1) [[SRC:%.*]], align 8
; CHECK: [[TMP2:%.*]] = fadd double [[TMP1]], 1.000000e+00
; CHECK: store double [[TMP2]], ptr addrspace(1) [[DST:%.*]], align 8
; CHECK: ret void
; CHECK: Used two options for FP64 emulation: for FP64 conversion emu (-cl/-ze-fp64-gen-conv-emu) and FP64 full emu (-cl/-ze-fp64-gen-emu). The FP64 conversion emu flag will be ignored by the compiler.
; CHECK_DUP: Used two options for FP64 emulation: for FP64 conversion emu (-cl/-ze-fp64-gen-conv-emu) and FP64 full emu (-cl/-ze-fp64-gen-emu). The FP64 conversion emu flag will be ignored by the compiler.
; CHECK_DUP: Used two options for FP64 emulation: for FP64 conversion emu (-cl/-ze-fp64-gen-conv-emu) and FP64 full emu (-cl/-ze-fp64-gen-emu). The FP64 conversion emu flag will be ignored by the compiler.
; CHECK-NOT: Used two options for FP64 emulation: for FP64 conversion emu (-cl/-ze-fp64-gen-conv-emu) and FP64 full emu (-cl/-ze-fp64-gen-emu). The FP64 conversion emu flag will be ignored by the compiler.
  %1 = load double, ptr addrspace(1) %src, align 8
  %2 = fadd double %1, 1.000000e+00
  store double %2, ptr addrspace(1) %dst, align 8
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (double addrspace(1)*, double addrspace(1)*)* @fp64_warning_deduplication_test, !1}
!1 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{!"ModuleMD", !5}
!5 = !{!"compOpt", !77, !78}

!77 = !{!"FP64GenEmulationEnabled", i1 false}
!78 = !{!"FP64GenConvEmulationEnabled", i1 true}