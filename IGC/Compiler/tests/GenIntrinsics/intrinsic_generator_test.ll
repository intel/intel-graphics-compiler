;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -S %s | FileCheck %s

; The purpose of this test is to check whether Gen Intrinsics module handles the new way of defining captures (in .yml files) correctly

; NEW SYNTAX:

;- !<ArgumentDefinition>
;   name: Payload
;   type_definition: *p_any_
;   comment: "Payload ptr"
;   capture: "None" (or other values)

; OLD SYNTAX:

;- !<ArgumentDefinition>
;   name: Payload
;   type_definition: *p_any_
;   comment: "Payload ptr"
;   param_attr: !ParamAttributeID "NoCapture"

; CHECK: declare void @llvm.genx.GenISA.TraceRayAsyncHL{{.*}}(i32, ptr, ptr, i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, ptr {{nocapture|captures\(none\)}}, i32, i32, i32, float)

define spir_kernel void @main(ptr %payload) {
entry:
  call void @llvm.genx.GenISA.TraceRayAsyncHL(i32 -1, ptr null, ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, float 0.0, float 0.0, float 0.0, float 0.0, float 1.0, float 1.0, float 0.0, float 100.0, ptr %payload, i32 0, i32 0, i32 0, float 0.0)
  ret void
}

declare void @llvm.genx.GenISA.TraceRayAsyncHL(i32, ptr, ptr, i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, ptr, i32, i32, i32, float)

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @main}
!3 = !{!"FuncMDValue[0]"}
!6 = !{ptr @main, !7}
!7 = !{}