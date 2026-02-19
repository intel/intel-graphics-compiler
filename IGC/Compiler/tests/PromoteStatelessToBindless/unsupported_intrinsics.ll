;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-promote-stateless-to-bindless -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteStatelessToBindless supports promotion of only two intrinsics: GenISA_simdBlockRead and
; GenISA_simdBlockWrite. This test verifies that the pass can correctly bail out and leave
; unsupported intrinsics untouched without any crash.
; ------------------------------------------------

define spir_func float @test_unsupported(i32 addrspace(1)* %src_i, float addrspace(1)* %src_f) {
; CHECK-LABEL: @test_unsupported(
; CHECK: call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* {{%.*}}, i32 addrspace(1)* {{%.*}}, i32 1, i32 1)
; CHECK: call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* {{%.*}}, i32 addrspace(1)* {{%.*}}, i32 0, i32 19)
; CHECK: call float @llvm.genx.GenISA.floatatomicrawA64.f32.p1f32.p1f32(float addrspace(1)* {{%.*}}, float addrspace(1)* {{%.*}}, float {{.*}}, i32 19)

  %1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src_i, i32 addrspace(1)* %src_i, i32 1, i32 1)
  %2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %src_i, i32 addrspace(1)* %src_i, i32 0, i32 19)
  %3 = call float @llvm.genx.GenISA.floatatomicrawA64.f32.p1f32.p1f32(float addrspace(1)* %src_f, float addrspace(1)* %src_f, float 0.0, i32 19)
  %4 = icmp eq i32 %1, %2
  %5  = select i1 %4, float %3, float 0.0
  ret float %5
}

declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)*, i32 addrspace(1)*, i32, i32)
declare i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)*, i32 addrspace(1)*, i32, i32)
declare float @llvm.genx.GenISA.floatatomicrawA64.f32.p1f32.p1f32(float addrspace(1)*, float addrspace(1)*, float, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{float (i32 addrspace(1)*, float addrspace(1)*)* @test_unsupported, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", float (i32 addrspace(1)*, float addrspace(1)*)* @test_unsupported}
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
