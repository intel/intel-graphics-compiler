;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-pressure-publisher -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s

; Verifies that FunctionMetaData::maxRegPressure round-trips through the
; autogen-serialized IGCMetadata MDNode. An input value of 42 (set via
; FuncMDValue[0]) is deserialized into ModuleMetaData::FuncMD; the pressure
; publisher sees the existing value and skips republishing; the serializer
; re-emits FuncMD intact.

; CHECK: "maxRegNonUniformPressure", i32 42

define spir_kernel void @kernel(ptr addrspace(1) %out) {
entry:
  store i32 0, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!10}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @kernel}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"maxRegNonUniformPressure", i32 42}

!10 = !{ptr @kernel, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
