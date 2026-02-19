;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-promote-stateless-to-bindless -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteStatelessToBindless : load and store part
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_promote(i32 addrspace(1)* %src, float addrspace(2)* %dst) {
; CHECK-LABEL: @test_promote(
; CHECK:    [[TMP1:%.*]] = addrspacecast i32 addrspace(1)* [[SRC:%.*]] to i32 addrspace(2490368)*
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368i32(i32 addrspace(2490368)* [[TMP1]], i32 0, i32 4, i1 false)
; CHECK:    [[TMP3:%.*]] = addrspacecast float addrspace(2)* null to float addrspace(1)*
; CHECK:    [[TMP4:%.*]] = bitcast float addrspace(1)* [[TMP3]] to i32 addrspace(1)*
; CHECK:    [[TMP5:%.*]] = ptrtoint i32 addrspace(1)* [[TMP4]] to i32
; CHECK:    [[TMP6:%.*]] = addrspacecast float addrspace(2)* [[DST:%.*]] to i32 addrspace(2490368)*
; CHECK:    call void @llvm.genx.GenISA.storeraw.indexed.p2490368i32.i32(i32 addrspace(2490368)* [[TMP6]], i32 [[TMP5]], i32 [[TMP2]], i32 4, i1 false)
; CHECK:    ret void
;
  %1 = load i32, i32 addrspace(1)* %src
  %2 = addrspacecast float addrspace(2)* %dst to float addrspace(1)*
  %3 = bitcast float addrspace(1)* %2 to i32 addrspace(1)*
  store i32 %1, i32 addrspace(1)* %3
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i32 addrspace(1)*, float addrspace(2)*)* @test_promote, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, float addrspace(2)*)* @test_promote}
!7 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{!"resAllocMD", !11, !12, !13, !14, !22}
!11 = !{!"uavsNumType", i32 4}
!12 = !{!"srvsNumType", i32 0}
!13 = !{!"samplersNumType", i32 0}
!14 = !{!"argAllocMDList", !15, !19}
!15 = !{!"argAllocMDListVec[0]", !16, !17, !18}
!16 = !{!"type", i32 0}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 -1}
!19 = !{!"argAllocMDListVec[1]", !20, !17, !21}
!20 = !{!"type", i32 1}
!21 = !{!"indexType", i32 0}
!22 = !{!"inlineSamplersMD"}
