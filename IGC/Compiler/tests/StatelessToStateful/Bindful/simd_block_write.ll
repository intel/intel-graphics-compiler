;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -platformpvc -igc-serialize-metadata | FileCheck %s

; This test verifies whether proper BTIs are assigned to buffers when pointer is used by simdBlockWrite instruction.
; BTIs are expected to be assigned consecutively:
; %in:  BTI 0
; %out: BTI 1

; CHECK: target datalayout = {{.*}}-p131072:32:32:32-p131073:32:32:32"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test(i32 addrspace(1)* align 4 %in, i32 addrspace(1)* align 4 %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 1
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  ; CHECK: load i32, i32 addrspace(131072)* %{{.*}}, align 4
  %0 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  ; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p131073i32.i32(i32 addrspace(131073)* %{{.*}}, i32 %{{.*}})
  call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* %arrayidx, i32 %0)
  ret void
}

declare void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)*, i32)

; CHECK-DAG: ![[BTI0:[0-9]+]] = !{!"indexType", i32 0}
; CHECK-DAG: ![[BTI1:[0-9]+]] = !{!"indexType", i32 1}

; CHECK-DAG: !{!"argAllocMDListVec[0]", {{.*}}, ![[BTI0]]}
; CHECK-DAG: !{!"argAllocMDListVec[1]", {{.*}}, ![[BTI1]]}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !5}
!1 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*)* @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10, !14}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !11, !12, !13}
