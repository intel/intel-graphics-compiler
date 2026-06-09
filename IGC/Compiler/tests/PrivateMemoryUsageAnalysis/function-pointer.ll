;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-private-mem-usage-analysis -platformmtl -ocl -S < %s -o /dev/null
; ------------------------------------------------
; PrivateMemoryUsageAnalysis
; ------------------------------------------------

; Make sure the pass doesn't crash when analyzing a function called via a function pointer

define spir_kernel void @kernel(i64 addrspace(1)* %dst, i32 addrspace(1)* %src) {
entry:
  %val = load i32, i32 addrspace(1)* %src
  %fcast = bitcast i32 (i32, i32)* @add to i64 (i32, i32)*
  %result = call spir_func i64 %fcast(i32 %val, i32 %val)
  store i64 %result, i64 addrspace(1)* %dst, align 8
  ret void
}

define spir_func i32 @add(i32 %x, i32 %y)
{
    %sum = add i32 %x, %y
    ret i32 %sum
}

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, i32 addrspace(1)*)* @kernel}
!12 = !{!"FuncMDValue[0]", !10, !40}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i64 addrspace(1)*, i32 addrspace(1)*)* @kernel, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 0}
!28 = !{!"argId", i32 0}
!29 = !{!"implicitArgInfoListVec[0]", !28}
!30 = !{!"argId", i32 1}
!31 = !{!"implicitArgInfoListVec[1]", !30}
!32 = !{!"argId", i32 13}
!33 = !{!"implicitArgInfoListVec[2]", !32}
!34 = !{!"argId", i32 15}
!35 = !{!"explicitArgNum", i32 0}
!36 = !{!"implicitArgInfoListVec[3]", !34, !35}
!37 = !{!"argId", i32 15}
!38 = !{!"explicitArgNum", i32 1}
!39 = !{!"implicitArgInfoListVec[4]", !37, !38}
!40 = !{!"implicitArgInfoList", !29, !31, !33, !36, !39}
