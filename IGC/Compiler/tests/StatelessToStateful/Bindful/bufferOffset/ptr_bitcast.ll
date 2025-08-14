;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-stateless-to-stateful-resolution -igc-serialize-metadata -platformpvc | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test(ptr addrspace(1) %dst, i32 %bufferOffset) {
entry:
; CHECK: %[[OFFSET:[0-9]+]] = add i32 %bufferOffset, 1
; CHECK: %[[PTR:[0-9]+]] = inttoptr i32 %[[OFFSET]] to ptr addrspace(131072)
; CHECK: store i8 0, ptr addrspace(131072) %[[PTR]], align 1
  %bc = bitcast ptr addrspace(1) %dst to ptr addrspace(1)
  %ptr = getelementptr inbounds i8, ptr addrspace(1) %bc, i64 1
  store i8 0, ptr addrspace(1) %ptr, align 1
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!10}

!0 = !{!"ModuleMD", !1, !8}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 1}
!8 = !{!"compOpt", !9}
!9 = !{!"HasBufferOffsetArg", i1 true}
!10 = !{ptr @test, !11}
!11 = !{!12, !13}
!12 = !{!"function_type", i32 0}
!13 = !{!"implicit_arg_desc", !14}
!14 = !{i32 15}
