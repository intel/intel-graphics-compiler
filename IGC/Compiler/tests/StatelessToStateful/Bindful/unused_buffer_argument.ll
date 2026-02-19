;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -platformpvc -igc-serialize-metadata | FileCheck %s

; This test verifies the scenario where there are four global buffer arguments and the 3rd one is
; not used in the kernel. All memory instructions operating on 0th, 1st and 4th arguments
; are expected to be promoted to stateful. The main goal of this test is to verify whether
; consecutive BTIs are assigned to arguments:
; %srcA: BTI 0
; %srcB: BTI 1
; %srcC: no BTI assigned
; %dst:  BTI 2

; CHECK: target datalayout = {{.*}}-p131072:32:32:32-p131073:32:32:32-p131074:32:32:32"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test(i32 addrspace(1)* align 4 %srcA, i32 addrspace(1)* align 4 %srcB, i32 addrspace(1)* align 4 %srcC, i32 addrspace(1)* align 4 %dst) {
entry:
  %ptrA = getelementptr inbounds i32, i32 addrspace(1)* %srcA, i64 1
  ; CHECK: %[[VAL_A:[0-9]+]] = load i32, i32 addrspace(131072)* %{{.*}}, align 4
  %valA = load i32, i32 addrspace(1)* %ptrA, align 4

  %ptrB = getelementptr inbounds i32, i32 addrspace(1)* %srcB, i64 2
  ; CHECK: %[[VAL_B:[0-9]+]] = load i32, i32 addrspace(131073)* %{{.*}}, align 4
  %valB = load i32, i32 addrspace(1)* %ptrB, align 4

  ; %sum = add i32 %[[VAL_A]], %[[VAL_B]]
  %sum = add i32 %valA, %valB

  %ptrDst = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 3
  ; store i32 %sum, i32 addrspace(131074)* %{{.*}}, align 4
  store i32 %sum, i32 addrspace(1)* %ptrDst, align 4
  ret void
}

; CHECK-DAG: ![[BTI0:[0-9]+]] = !{!"indexType", i32 0}
; CHECK-DAG: ![[BTI1:[0-9]+]] = !{!"indexType", i32 1}
; CHECK-DAG: ![[BTI2:[0-9]+]] = !{!"indexType", i32 2}

; CHECK-DAG: !{!"argAllocMDListVec[0]", {{.*}}, ![[BTI0]]}
; CHECK-DAG: !{!"argAllocMDListVec[1]", {{.*}}, ![[BTI1]]}
; CHECK-DAG: !{!"argAllocMDListVec[3]", {{.*}}, ![[BTI2]]}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !5}
!1 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*)* @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*)* @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10, !14, !16, !18}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !11, !12, !15}
!15 = !{!"indexType", i32 -1}
!16 = !{!"argAllocMDListVec[2]", !11, !12, !17}
!17 = !{!"indexType", i32 -1}
!18 = !{!"argAllocMDListVec[3]", !11, !12, !19}
!19 = !{!"indexType", i32 -1}
