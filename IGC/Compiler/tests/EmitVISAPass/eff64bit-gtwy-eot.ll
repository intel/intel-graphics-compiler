;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that EmitVISAPass emits sendg.gtwy EOT from GenISA_thread_exit intrinsic
; on CRI (xe3p+ with efficient 64b addressing) instead of legacy send.

; REQUIRES: regkeys

; Check VISAASM code: expect raw_sendg_eot with SFID=3 (gateway) and desc=0x0
; RUN: igc_opt -platformCri -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefix=VISAASM

; Check ASM code: expect sendg.gtwy with desc=0x0
; RUN: igc_opt -platformCri -igc-emit-visa %s -regkey DumpASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefix=ASM

; VISAASM-LABEL: .kernel "test_gtwy_eot"
; VISAASM:       raw_sendg_eot.0x3 (M1, 1) %null.0/0 R0_0v.0/64 %null.0/0 %null(0,0)<0;1,0> %null(0,0)<0;1,0> 0x0

; ASM-LABEL: .kernel test_gtwy_eot
; ASM:          _test_gtwy_eot_001_if_then:
; ASM-NEXT:     sendg.gtwy (1|M0)        null     r{{[0-9]+}}:1  null:0  0x0

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_gtwy_eot(i32 addrspace(1)* %buf, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %tid = zext i16 %localIdX to i32
  %cmp = icmp eq i32 %tid, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:
  ; Early thread exit for tid == 0
  call void @llvm.genx.GenISA.thread.exit()
  br label %if.end

if.end:
  %ptr = getelementptr inbounds i32, i32 addrspace(1)* %buf, i64 0
  store i32 %tid, i32 addrspace(1)* %ptr, align 4
  ret void
}

declare void @llvm.genx.GenISA.thread.exit() #1

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" "null-pointer-is-valid"="true" }
attributes #1 = { nounwind }

!IGCMetadata = !{!0}
!igc.functions = !{!20}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16)* @test_gtwy_eot}
!3 = !{!"FuncMDValue[0]", !4, !5}
!4 = !{!"functionType", !"KernelFunction"}
!5 = !{!"resAllocMD", !6, !7, !8, !9, !16}
!6 = !{!"uavsNumType", i32 0}
!7 = !{!"srvsNumType", i32 0}
!8 = !{!"samplersNumType", i32 0}
!9 = !{!"argAllocMDList", !10, !14, !15, !17, !18, !19}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !11, !12, !13}
!15 = !{!"argAllocMDListVec[2]", !11, !12, !13}
!16 = !{!"inlineSamplersMD"}
!17 = !{!"argAllocMDListVec[3]", !11, !12, !13}
!18 = !{!"argAllocMDListVec[4]", !11, !12, !13}
!19 = !{!"argAllocMDListVec[5]", !11, !12, !13}
!20 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16)* @test_gtwy_eot, !21}
!21 = !{!22, !23}
!22 = !{!"function_type", i32 0}
!23 = !{!"implicit_arg_desc", !24, !25, !26, !27, !28}
!24 = !{i32 0}
!25 = !{i32 1}
!26 = !{i32 8}
!27 = !{i32 9}
!28 = !{i32 10}
