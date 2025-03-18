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
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i64 addrspace(1)*, i32 addrspace(1)*)* @kernel, !18}
!18 = !{!19, !20}
!19 = !{!"function_type", i32 0}
!20 = !{!"implicit_arg_desc", !21, !22, !23, !24, !26}
!21 = !{i32 0}
!22 = !{i32 1}
!23 = !{i32 13}
!24 = !{i32 15, !25}
!25 = !{!"explicit_arg_num", i32 0}
!26 = !{i32 15, !27}
!27 = !{!"explicit_arg_num", i32 1}
