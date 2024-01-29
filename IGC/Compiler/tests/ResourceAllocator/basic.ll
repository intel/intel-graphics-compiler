;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify  --igc-resource-allocator -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ResourceAllocator
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Check that resource md is updated
; CHECK: !{!"uavsNumType", i32 1}
; CHECK: !{!"srvsNumType", i32 1}
; CHECK: !{!"samplersNumType", i32 2}

%opencl.image2d_t.read_only = type opaque

define void @test_ress(%opencl.image2d_t.read_only addrspace(1)* %input, i32* %output, i32 addrspace(2)* %sample, i32 addrspace(2)* %sample_b) {
  %1 = alloca %opencl.image2d_t.read_only addrspace(1)*, align 4
  store %opencl.image2d_t.read_only addrspace(1)* %input, %opencl.image2d_t.read_only addrspace(1)** %1, align 8
  %2 = load i32, i32* %output, align 4
  %3 = load i32, i32 addrspace(2)* %sample, align 4
  %4 = load i32, i32 addrspace(2)* %sample_b, align 4
  %5 = add i32 %2, %3
  %6 = add i32 %5, %4
  store i32 %6, i32* %output, align 4
  ret void
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (%opencl.image2d_t.read_only addrspace(1)*, i32*, i32 addrspace(2)*, i32 addrspace(2)*)* @test_ress, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (%opencl.image2d_t.read_only addrspace(1)*, i32*, i32 addrspace(2)*, i32 addrspace(2)*)* @test_ress}
!7 = !{!"FuncMDValue[0]", !8, !13}
!8 = !{!"resAllocMD", !9, !10, !11, !12}
!9 = !{!"uavsNumType", i32 32}
!10 = !{!"srvsNumType", i32 31}
!11 = !{!"samplersNumType", i32 10}
!12 = !{!"argAllocMDList"}
!13 = !{!"m_OpenCLArgBaseTypes", !14, !15, !16, !17}
!14 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image2d_t"}
!15 = !{!"m_OpenCLArgBaseTypesVec[1]", !""}
!16 = !{!"m_OpenCLArgBaseTypesVec[2]", !"sampler_t"}
!17 = !{!"m_OpenCLArgBaseTypesVec[3]", !"bindless_sampler_t"}
