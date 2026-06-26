;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -igc-wi-func-analysis -regkey ShortImplicitPayloadHeader=0 -igc-serialize-metadata -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-LONG-PAYLOAD
; RUN: igc_opt --opaque-pointers -igc-wi-func-analysis -regkey ShortImplicitPayloadHeader=1 -igc-serialize-metadata -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-SHORT-PAYLOAD

; Test switching between long (original) and short implicit payload header.

declare i32 @__builtin_IB_get_local_id_x()

define i32 @foo(i32 %dim) nounwind {
  %id = call i32 @__builtin_IB_get_local_id_x()
  ret i32 %id
}

!IGCMetadata = !{!7}
!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"functionType", !"KernelFunction"}
!4 = !{!"FuncMDMap[0]", i32 (i32)* @foo}
!5 = !{!"FuncMDValue[0]", !3}
!6 = !{!"FuncMD", !4, !5}
!7 = !{!"ModuleMD", !6}

; The second implicit arg differs by payload config: PAYLOAD_HEADER (argId 1) for
; the long header, GLOBAL_OFFSET (argId 2) for the short header.
;CHECK:               !{!"implicitArgInfoList"
;CHECK:               !{!"argId", i32 0}
;CHECK-LONG-PAYLOAD:  !{!"argId", i32 1}
;CHECK-SHORT-PAYLOAD: !{!"argId", i32 2}
;CHECK:               !{!"argId", i32 8}
;CHECK:               !{!"argId", i32 9}
;CHECK:               !{!"argId", i32 10}
