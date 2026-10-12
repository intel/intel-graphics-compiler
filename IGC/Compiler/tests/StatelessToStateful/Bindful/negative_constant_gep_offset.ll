;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-stateless-to-stateful-resolution -platformdg2 | FileCheck %s
; ------------------------------------------------
; StatelessToStateful : negative constant GEP index
; ------------------------------------------------
;
; CHECK-LABEL: @test
; CHECK: [[OFFSET:%.*]] = add i32 %bufferOffset, -4
; CHECK: [[PTR:%.*]] = inttoptr i32 [[OFFSET]] to ptr addrspace(131072)
; CHECK: store float 1.000000e+00, ptr addrspace(131072) [[PTR]], align 4

define spir_kernel void @test(ptr addrspace(1) %buf, <8 x i32> %r0, i32 %bufferOffset) #0 {
entry:
  %arrayidx = getelementptr inbounds i8, ptr addrspace(1) %buf, i64 -4
  store float 1.000000e+00, ptr addrspace(1) %arrayidx, align 4
  ret void
}

attributes #0 = { convergent nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4, !8}
!4 = !{!"compOpt", !5, !6, !7}
!5 = !{!"HasBufferOffsetArg", i1 true}
!6 = !{!"BufferOffsetArgOptional", i1 true}
!7 = !{!"UseBindlessMode", i1 false}
!8 = !{!"FuncMD", !9, !10}
!9 = !{!"FuncMDMap[0]", ptr @test}
!10 = !{!"FuncMDValue[0]", !11, !18, !19}
!11 = !{!"implicitArgInfoList", !12, !15}
!12 = !{!"implicitArgInfoListVec[0]", !13, !14}
!13 = !{!"argId", i32 0}
!14 = !{!"explicitArgNum", i32 -1}
!15 = !{!"implicitArgInfoListVec[1]", !16, !17}
!16 = !{!"argId", i32 15}
!17 = !{!"explicitArgNum", i32 0}
!18 = !{!"functionType", !"KernelFunction"}
!19 = !{!"resAllocMD", !20}
!20 = !{!"argAllocMDList", !21, !22, !23}
!21 = !{!"argAllocMDListVec[0]", !24, !25}
!22 = !{!"argAllocMDListVec[1]", !24, !25}
!23 = !{!"argAllocMDListVec[2]", !24, !25}
!24 = !{!"type", i32 1}
!25 = !{!"indexType", i32 0}
