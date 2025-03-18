;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -igc-serialize-metadata -platformpvc | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test(i8 addrspace(1)* %dst, i32 %bufferOffset) {
entry:
  ; CHECK: %[[OFFSET:[0-9]+]] = add i32 %bufferOffset, 3
  ; CHECK: %[[PTR:[0-9]+]] = inttoptr i32 %[[OFFSET]] to i8 addrspace(131072)*
  ; CHECK: store i8 0, i8 addrspace(131072)* %[[PTR]], align 1
  %ptr = getelementptr inbounds i8, i8 addrspace(1)* %dst, i64 3
  store i8 0, i8 addrspace(1)* %ptr, align 1
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !5, !16}
!1 = !{void (i8 addrspace(1)*, i32)* @test, !2}
!2 = !{!3, !14}
!3 = !{!"function_type", i32 0}
!4 = !{}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i32)* @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10}
!10 = !{!"argAllocMDListVec[0]", !11}
!11 = !{!"type", i32 1}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 0}
!14 = !{!"implicit_arg_desc", !15}
!15 = !{i32 15}
!16 = !{!"compOpt", !17}
!17 = !{!"HasBufferOffsetArg", i1 true}
