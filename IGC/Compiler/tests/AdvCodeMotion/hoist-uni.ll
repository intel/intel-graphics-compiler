;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -adv-codemotion-cm=1 -igc-advcodemotion -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; AdvCodeMotion
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(ptr addrspace(1) %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, <3 x i32> %globalSize, <3 x i32> %enqueuedLocalSize, <3 x i32> %localSize, ptr %privateBase, i32 %bufferOffset) #0 {

; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = extractelement <8 x i32> [[R0:%[A-z0-9]*]], i32 1
; CHECK:    [[TMP1:%[A-z0-9]*]] = extractelement <3 x i32> [[GLOBALSIZE:%[A-z0-9]*]], i32 0
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <3 x i32> [[LOCALSIZE:%[A-z0-9]*]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = extractelement <3 x i32> [[ENQUEUEDLOCALSIZE:%[A-z0-9]*]], i32 0
; CHECK:    [[TMP4:%[A-z0-9]*]] = mul i32 [[TMP3]], [[TMP0]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = zext i16 [[LOCALIDX:%[A-z0-9]*]] to i32
; CHECK:    [[TMP6:%[A-z0-9]*]] = add i32 [[TMP5]], [[TMP4]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = extractelement <8 x i32> [[PAYLOADHEADER:%[A-z0-9]*]], i32 0
; CHECK:    [[TMP8:%[A-z0-9]*]] = add i32 [[TMP6]], [[TMP7]]
; CHECK:    br label [[BB3:%[A-z0-9]*]]
; CHECK:  bb1:
; CHECK:    [[A:%[A-z0-9]*]] = phi i32 [ [[B:%[A-z0-9]*]], [[BB3]] ], [ [[AI:%[A-z0-9]*]], [[BB1:%[A-z0-9]*]] ]
; CHECK:    [[LC:%[A-z0-9]*]] = phi i32 [ [[BI:%[A-z0-9]*]], [[BB3]] ], [ [[LC]], [[BB1]] ]
; CHECK:    [[AI]] = add i32 [[A]], 1
; CHECK:    [[AC:%[A-z0-9]*]] = icmp ne i32 [[TMP8]], [[AI]]
; CHECK:    [[CC:%[A-z0-9]*]] = icmp eq i32 [[AI]], [[LC]]
; CHECK:    br i1 [[CC]], label [[BB2:%[A-z0-9]*]], label [[BB1]]
; CHECK:  bb2:
; CHECK:    [[AAA:%[A-z0-9]*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    br i1 [[AC]], label [[TBB:%[A-z0-9]*]], label [[FBB:%[A-z0-9]*]]
; CHECK:  bb3:
; CHECK:    [[B]] = phi i32 [ -1, [[ENTRY:%[A-z0-9]*]] ], [ [[BI]], [[JOIN:%[A-z0-9]*]] ]
; CHECK:    [[BL:%[A-z0-9]*]] = phi i32 [ 0, [[ENTRY]] ], [ [[BLI:%[A-z0-9]*]], [[JOIN]] ]
; CHECK:    [[BI]] = add i32 [[B]], [[TMP2]]
; CHECK:    [[BLI]] = add i32 [[BL]], [[TMP2]]
; CHECK:    [[BC:%[A-z0-9]*]] = icmp ult i32 [[BLI]], [[TMP1]]
; CHECK:    br i1 [[BC]], label [[BB1]], label [[END:%[A-z0-9]*]]
; CHECK:  tbb:
; CHECK:    br label [[JOIN]]
; CHECK:  fbb:
; CHECK:    br label [[JOIN]]
; CHECK:  join:
; CHECK:    br label [[BB3]]
; CHECK:  end:
; CHECK:    store i32 [[TMP8]], ptr addrspace(1) [[DST:%[A-z0-9]*]], align 4
; CHECK:    ret void

entry:
  %0 = extractelement <8 x i32> %r0, i32 1
  %1 = extractelement <3 x i32> %globalSize, i32 0
  %2 = extractelement <3 x i32> %localSize, i32 0
  %3 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %4 = mul i32 %3, %0
  %5 = zext i16 %localIdX to i32
  %6 = add i32 %5, %4
  %7 = extractelement <8 x i32> %payloadHeader, i32 0
  %8 = add i32 %6, %7
  br label %bb3

bb1:                                              ; preds = %bb3, %bb1
  %a = phi i32 [ %b, %bb3 ], [ %ai, %bb1 ]
  %lc = phi i32 [ %bi, %bb3 ], [ %lc, %bb1 ]
  %ai = add i32 %a, 1
  %ac = icmp ne i32 %8, %ai
  %cc = icmp eq i32 %ai, %lc
  br i1 %cc, label %bb2, label %bb1

bb2:                                              ; preds = %bb1
  br i1 %ac, label %tbb, label %fbb

bb3:                                              ; preds = %join, %entry
  %b = phi i32 [ -1, %entry ], [ %bi, %join ]
  %bl = phi i32 [ 0, %entry ], [ %bli, %join ]
  %bi = add i32 %b, %2
  %bli = add i32 %bl, %2
  %bc = icmp ult i32 %bli, %1
  br i1 %bc, label %bb1, label %end

tbb:                                              ; preds = %bb2
  %aaa = add i32 %1, %2
  br label %join

fbb:                                              ; preds = %bb2
  br label %join

join:                                             ; preds = %fbb, %tbb
  br label %bb3

end:                                              ; preds = %bb3
  store i32 %8, ptr addrspace(1) %dst, align 4
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_size(i32) local_unnamed_addr #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent nounwind readnone }

!igc.functions = !{!3}

!3 = !{ptr @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 5}
!10 = !{i32 6}
!11 = !{i32 13}
!12 = !{i32 15, !13}
!13 = !{!"explicit_arg_num", i32 0}
