;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check address mode of bindless inline sampler is parsed from implicit arg and
; inlineSamplersMD metadata.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution -S %s -o - | FileCheck %s

define spir_kernel void @test(i32 %inlineSampler) {
entry:
; CHECK: = icmp eq i32 2, 0

  %0 = zext i32 %inlineSampler to i64
  %1 = or i64 %0, 1
  %2 = trunc i64 %1 to i32
  %3 = call spir_func i32 @__builtin_IB_get_address_mode(i32 %2)
  %4 = icmp eq i32 %3, 0
  ret void
}

declare spir_func i32 @__builtin_IB_get_address_mode(i32)

!igc.functions = !{!0}
!IGCMetadata = !{!5}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"implicit_arg_desc", !3}
!3 = !{i32 33, !4}
!4 = !{!"explicit_arg_num", i32 19}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", ptr @test}
!8 = !{!"FuncMDValue[0]", !9}
!9 = !{!"resAllocMD", !10, !15}
!10 = !{!"argAllocMDList", !11}
!11 = !{!"argAllocMDListVec[8]", !12, !13, !14}
!12 = !{!"type", i32 0}
!13 = !{!"extensionType", i32 -1}
!14 = !{!"indexType", i32 -1}
!15 = !{!"inlineSamplersMD", !16}
!16 = !{!"inlineSamplersMDVec[0]", !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31}
!17 = !{!"m_Value", i32 19}
!18 = !{!"addressMode", i32 2}
!19 = !{!"index", i32 0}
!20 = !{!"TCXAddressMode", i32 2}
!21 = !{!"TCYAddressMode", i32 2}
!22 = !{!"TCZAddressMode", i32 2}
!23 = !{!"MagFilterType", i32 0}
!24 = !{!"MinFilterType", i32 0}
!25 = !{!"MipFilterType", i32 0}
!26 = !{!"CompareFunc", i32 0}
!27 = !{!"NormalizedCoords", i32 8}
!28 = !{!"BorderColorR", float 0.000000e+00}
!29 = !{!"BorderColorG", float 0.000000e+00}
!30 = !{!"BorderColorB", float 0.000000e+00}
!31 = !{!"BorderColorA", float 0.000000e+00}
