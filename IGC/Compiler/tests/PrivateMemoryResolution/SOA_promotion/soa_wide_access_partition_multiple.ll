;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1,EnableSOAFallbackToOldAlgorithm=0 \
; RUN:   -S %s | FileCheck %s --check-prefixes=CHECK,AGG
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=0,EnableSOAFallbackToOldAlgorithm=0 \
; RUN:   -S %s | FileCheck %s --check-prefixes=CHECK,NOAGG
;
; An access larger than SOAPartitionBytes can only be lowered by the new SoA transpose when it is split
; into partition-sized chunks and EnableAggressiveSOAPromotion is enabled.
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%S8 = type { i64, [2 x i32] }
%F3 = type { float, float, float }

; 8-byte partition, 12-byte <3 x i32> access: not a multiple of the partition, never split -> AoS.
;
; CHECK-LABEL: @not_partition_multiple(
; CHECK:       [[LANE16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[LANE:%.*]] = zext i16 [[LANE16]] to i32
; CHECK:       mul i32 [[LANE]], 32
; CHECK-NOT:   ptrtoint
; CHECK:       [[P:%.*]] = getelementptr [2 x %S8], ptr %{{.*}}, i32 0, i32 %i, i32 1, i32 0
; CHECK-NEXT:  store <3 x i32> %v, ptr [[P]], align 4
; CHECK-NEXT:  ret void
define spir_kernel void @not_partition_multiple(<3 x i32> %v, i32 %i, <8 x i32> %r0, <8 x i32> %payloadHeader,
                                                ptr %privateBase) {
entry:
  %a = alloca [2 x %S8], align 8
  %p = getelementptr [2 x %S8], ptr %a, i32 0, i32 %i, i32 1, i32 0
  store <3 x i32> %v, ptr %p, align 4
  ret void
}

; 4-byte partition, 8-byte <2 x i32> access: a multiple of the partition, split into two i32 chunks
; with EnableAggressiveSOAPromotion, kept AoS without it.
;
; CHECK-LABEL: @partition_multiple(
; CHECK:       [[LANE16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[LANE:%.*]] = zext i16 [[LANE16]] to i32
; AGG:         mul i32 [[LANE]], 4
; AGG:         ptrtoint ptr %privateBase to i64
; AGG-NOT:     getelementptr [4 x %F3]
; AGG:         [[E0:%.*]] = extractelement <2 x i32> %v, i32 0
; AGG-NEXT:    store i32 [[E0]], ptr %{{.*}}, align 4
; AGG:         [[E1:%.*]] = extractelement <2 x i32> %v, i32 1
; AGG-NEXT:    store i32 [[E1]], ptr %{{.*}}, align 4
; AGG-NOT:     store <2 x i32>
; NOAGG:       mul i32 [[LANE]], 48
; NOAGG-NOT:   ptrtoint
; NOAGG:       [[P:%.*]] = getelementptr [4 x %F3], ptr %{{.*}}, i32 0, i32 %i, i32 0
; NOAGG-NEXT:  store <2 x i32> %v, ptr [[P]], align 4
; CHECK:       ret void
define spir_kernel void @partition_multiple(<2 x i32> %v, i32 %i, <8 x i32> %r0, <8 x i32> %payloadHeader,
                                            ptr %privateBase) {
entry:
  %a = alloca [4 x %F3], align 4
  %p = getelementptr [4 x %F3], ptr %a, i32 0, i32 %i, i32 0
  store <2 x i32> %v, ptr %p, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{!"FuncMD", !4, !5, !6, !7}
!4 = !{!"FuncMDMap[0]", ptr @not_partition_multiple}
!5 = !{!"FuncMDValue[0]", !2, !10}
!6 = !{!"FuncMDMap[1]", ptr @partition_multiple}
!7 = !{!"FuncMDValue[1]", !2, !10}
!10 = !{!"implicitArgInfoList", !11, !12, !13}
!11 = !{!"implicitArgInfoListVec[0]", !14}
!12 = !{!"implicitArgInfoListVec[1]", !15}
!13 = !{!"implicitArgInfoListVec[2]", !16}
!14 = !{!"argId", i32 0}
!15 = !{!"argId", i32 1}
!16 = !{!"argId", i32 13}
!20 = !{ptr @not_partition_multiple, !408}
!21 = !{ptr @partition_multiple, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
