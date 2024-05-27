;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check __devicelib_exit is resolved.

; RUN: igc_opt -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(<8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
; CHECK: call void @llvm.genx.GenISA.thread.exit()

  call spir_func void @__devicelib_exit()
  ret void
}

declare spir_func void @__devicelib_exit()

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (<8 x i32>, <8 x i32>)* @test, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (<8 x i32>, <8 x i32>)* @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !12}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"indexType", i32 -1}
!12 = !{!"argAllocMDListVec[1]", !9, !10, !11}
