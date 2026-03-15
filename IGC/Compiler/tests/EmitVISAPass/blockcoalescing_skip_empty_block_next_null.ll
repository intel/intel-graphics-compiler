;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify BlockCoalescing skipping empty basic block does not cause SEGFAULT in
; case where last BB is empty.

; REQUIRES: regkeys, llvm-16-plus

; RUN: igc_opt -S --opaque-pointers -platformCri -igc-emit-visa \
; RUN: --regkey EnableEfficient64b=1,DumpVISAASMToConsole=1 < %s 2>&1 \
; RUN: | FileCheck %s

; CHECK-LABEL: .function

define spir_kernel void @_attn_fwd(i8 %0) #0 {
  %b2s = sext i8 %0 to i16
  %b2s1 = and i16 %b2s, 1
  %2 = trunc i16 %b2s1 to i8
  %b2s2 = zext i8 %2 to i16
  %b2s3 = icmp eq i16 %b2s2, 0
  br label %._crit_edge, !stats.blockFrequency.digits !475, !stats.blockFrequency.scale !476

._crit_edge:                                      ; preds = %._crit_edge.loopexit, %1
  br label %T, !stats.blockFrequency.digits !477, !stats.blockFrequency.scale !476

T:                                                ; preds = %T.T_crit_edge, %._crit_edge
  br i1 %b2s3, label %T.T_crit_edge, label %._crit_edge.loopexit, !stats.blockFrequency.digits !478, !stats.blockFrequency.scale !476

T.T_crit_edge:                                    ; preds = %T
  br label %T, !stats.blockFrequency.digits !479, !stats.blockFrequency.scale !476

._crit_edge.loopexit:                             ; preds = %T
  br label %._crit_edge, !stats.blockFrequency.digits !477, !stats.blockFrequency.scale !476
}

attributes #0 = { convergent null_pointer_is_valid }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{ptr @_attn_fwd, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 0}
!9 = !{!"ModuleMD", !149}
!149 = !{!"FuncMD", !150, !151}
!150 = !{!"FuncMDMap[0]", ptr @_attn_fwd}
!151 = !{!"FuncMDValue[0]", !188}
!188 = !{!"resAllocMD", !189, !190, !191, !192, !201}
!189 = !{!"uavsNumType", i32 0}
!190 = !{!"srvsNumType", i32 0}
!191 = !{!"samplersNumType", i32 0}
!192 = !{!"argAllocMDList", !193}
!193 = !{!"argAllocMDListVec[0]", !194, !195, !196}
!194 = !{!"type", i32 0}
!195 = !{!"extensionType", i32 -1}
!196 = !{!"indexType", i32 -1}
!201 = !{!"inlineSamplersMD"}
!221 = !{!"UserAnnotations"}
!475 = !{!"80"}
!476 = !{!"-3"}
!477 = !{!"327680"}
!478 = !{!"10485760"}
!479 = !{!"10158080"}
