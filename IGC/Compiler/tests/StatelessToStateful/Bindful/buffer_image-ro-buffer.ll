;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -igc-serialize-metadata -platformpvc | FileCheck %s

; This test verifies the scenario where there are three kernel arguments. The 0th and 2nd are global
; buffers that are expected to be promoted to stateful. The 1st argument is a read_only image which is already promoted
; to stateful by OCLBIConverter pass. It is important to notice that read_only images are qualified as SRV resources,
; not UAV resources (like buffers), hence the below LLVM module contains the following metadata:
; !19 = !{!"uavsNumType", i32 0}      <--- to be allocated by StatelessToStateful
; !20 = !{!"srvsNumType", i32 1}      <--- allocated by OCLBIConverter

; The main goal of this test is to verify whether proper BTIs are assigned to buffers:
; %srcA: UAV_BTI 0
; %img:  SRV_BTI 0 (pre-assigned in OCLBIConverter)
; %dst:  UAV_BTI 1

; CHECK: target datalayout = {{.*}}-p131072:32:32:32-p131073:32:32:32"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @test(i32 addrspace(1)* align 4 %srcA, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img, i32 addrspace(1)* align 4 %dst) {
entry:
  ; BTI for image has been assigned by OCLBIConverter
  %color = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32.p196608f32(i32 0, i32 0, i32 0, i32 0, float addrspace(196608)* undef, float addrspace(196608)* null, i32 0, i32 0, i32 0)
  %colorX = extractelement <4 x i32> %color, i32 0

  %ptrA = getelementptr inbounds i32, i32 addrspace(1)* %srcA, i64 1
  ; CHECK: %[[VAL_A:[0-9]+]] = load i32, i32 addrspace(131072)* %{{.*}}, align 4
  %valA = load i32, i32 addrspace(1)* %ptrA, align 4

  %sum = add nsw i32 %valA, %colorX

  %ptrDst = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 1
  ; CHECK: store i32 %sum, i32 addrspace(131073)* %{{.*}}, align 4
  store i32 %sum, i32 addrspace(1)* %ptrDst, align 4
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32.p196608f32(i32, i32, i32, i32, float addrspace(196608)*, float addrspace(196608)*, i32, i32, i32)

; CHECK-DAG: ![[BTI0:[0-9]+]] = !{!"indexType", i32 0}
; CHECK-DAG: ![[BTI1:[0-9]+]] = !{!"indexType", i32 1}

; CHECK-DAG: !{!"argAllocMDListVec[0]", {{.*}}, ![[BTI0]]}
; CHECK-DAG: !{!"argAllocMDListVec[2]", {{.*}}, ![[BTI1]]}

; CHECK-DAG: !{!"uavsNumType", i32 2}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !5}
!1 = !{void (i32 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, i32 addrspace(1)*)* @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, i32 addrspace(1)*)* @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9, !19, !20}
!9 = !{!"argAllocMDList", !10, !14, !18}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !15, !16, !17}
!15 = !{!"type", i32 2} ; SRVResourceType
!16 = !{!"extensionType", i32 -1}
!17 = !{!"indexType", i32 0}
!18 = !{!"argAllocMDListVec[2]", !11, !12, !13}
!19 = !{!"uavsNumType", i32 0}
!20 = !{!"srvsNumType", i32 1}
