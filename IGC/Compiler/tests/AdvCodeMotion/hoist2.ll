;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -adv-codemotion-cm=1 -igc-advcodemotion -S < %s | FileCheck %s
; ------------------------------------------------
; AdvCodeMotion
; ------------------------------------------------

define spir_kernel void @test(ptr addrspace(1) %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, <3 x i32> %globalSize, <3 x i32> %enqueuedLocalSize, <3 x i32> %localSize, ptr %privateBase, i32 %bufferOffset) #0 {

; CHECK-LABEL: @test(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = extractelement <8 x i32> [[R0:%.*]], i32 1
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <3 x i32> [[GLOBALSIZE:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <3 x i32> [[LOCALSIZE:%.*]], i32 0
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <3 x i32> [[ENQUEUEDLOCALSIZE:%.*]], i32 0
; CHECK-NEXT:    [[TMP4:%.*]] = mul i32 [[TMP3]], [[TMP0]]
; CHECK-NEXT:    [[TMP5:%.*]] = zext i16 [[LOCALIDX:%.*]] to i32
; CHECK-NEXT:    [[TMP6:%.*]] = add i32 [[TMP5]], [[TMP4]]
; CHECK-NEXT:    [[TMP7:%.*]] = extractelement <8 x i32> [[PAYLOADHEADER:%.*]], i32 0
; CHECK-NEXT:    [[TMP8:%.*]] = add i32 [[TMP6]], [[TMP7]]
; CHECK-NEXT:    br label [[BB3:%.*]]
; CHECK:       bb1:
; CHECK-NEXT:    [[A:%.*]] = phi i32 [ [[B:%.*]], [[BB3]] ], [ [[AI:%.*]], [[BB1:%.*]] ]
; CHECK-NEXT:    [[LC:%.*]] = phi i32 [ [[BI:%.*]], [[BB3]] ], [ [[LC]], [[BB1]] ]
; CHECK-NEXT:    [[AI]] = add i32 [[A]], 1
; CHECK-NEXT:    [[AC:%.*]] = icmp ne i32 [[TMP8]], [[AI]]
; CHECK-NEXT:    [[CC:%.*]] = icmp eq i32 [[AI]], [[LC]]
; CHECK-NEXT:    br i1 [[CC]], label [[BB2:%.*]], label [[BB1]]
; CHECK:       bb2:
; CHECK-NEXT:    [[AAA:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[ACC:%.*]] = icmp eq i32 [[AAA]], 0
; CHECK-NEXT:    [[TMP9:%.*]] = and i1 [[AC]], [[ACC]]
; CHECK-NEXT:    br i1 [[TMP9]], label [[TBB2:%.*]], label [[FBB2:%.*]]
; CHECK:       bb3:
; CHECK-NEXT:    [[B]] = phi i32 [ -1, [[ENTRY:%.*]] ], [ [[BI]], [[JOIN2:%.*]] ]
; CHECK-NEXT:    [[BL:%.*]] = phi i32 [ 0, [[ENTRY]] ], [ [[BLI:%.*]], [[JOIN2]] ]
; CHECK-NEXT:    [[BI]] = add i32 [[B]], [[TMP2]]
; CHECK-NEXT:    [[BLI]] = add i32 [[BL]], [[TMP2]]
; CHECK-NEXT:    [[BC:%.*]] = icmp ult i32 [[BLI]], [[TMP1]]
; CHECK-NEXT:    br i1 [[BC]], label [[BB1]], label [[END:%.*]]
; CHECK:       fbb2:
; CHECK-NEXT:    br label [[JOIN2]]
; CHECK:       tbb2:
; CHECK-NEXT:    [[BBB:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    br label [[JOIN2]]
; CHECK:       join2:
; CHECK-NEXT:    [[J2PHI:%.*]] = phi i32 [ [[BBB]], [[TBB2]] ], [ 0, [[FBB2]] ]
; CHECK-NEXT:    [[ORPHI:%.*]] = phi i32 [ 1, [[TBB2]] ], [ [[TMP1]], [[FBB2]] ]
; CHECK-NEXT:    store i32 [[J2PHI]], ptr addrspace(1) [[DST:%.*]], align 4
; CHECK-NEXT:    store i32 [[ORPHI]], ptr addrspace(1) [[DST]], align 4
; CHECK-NEXT:    store i32 -1, ptr addrspace(1) [[DST]], align 4
; CHECK-NEXT:    br label [[BB3]]
; CHECK:       end:
; CHECK-NEXT:    store i32 [[TMP8]], ptr addrspace(1) [[DST]], align 4
; CHECK-NEXT:    ret void

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
  br i1 %ac, label %bb4, label %fbb

bb3:                                              ; preds = %join, %entry
  %b = phi i32 [ -1, %entry ], [ %bi, %join ]
  %bl = phi i32 [ 0, %entry ], [ %bli, %join ]
  %bi = add i32 %b, %2
  %bli = add i32 %bl, %2
  %bc = icmp ult i32 %bli, %1
  br i1 %bc, label %bb1, label %end

bb4:                                              ; preds = %bb2
  %aaa = add i32 %1, %2
  %acc = icmp eq i32 %aaa, 0
  br i1 %acc, label %tbb2, label %fbb2

fbb:                                              ; preds = %bb2
  br label %join

fbb2:                                             ; preds = %bb4
  br label %join2

tbb2:                                             ; preds = %bb4
  %bbb = add i32 %1, %2
  br label %join2

join2:                                            ; preds = %tbb2, %fbb2
  %j2phi = phi i32 [ %bbb, %tbb2 ], [ 0, %fbb2 ]
  %orphi = phi i32 [ 1, %tbb2 ], [ 0, %fbb2 ]
  store i32 %j2phi, ptr addrspace(1) %dst, align 4
  %oropt = or i32 %1, %orphi
  store i32 %oropt, ptr addrspace(1) %dst, align 4
  br label %join

join:                                             ; preds = %join2, %fbb
  %jphi = phi i32 [ -1, %join2 ], [ 0, %fbb ]
  store i32 %jphi, ptr addrspace(1) %dst, align 4
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

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8, !9}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 5}
!7 = !{i32 6}
!8 = !{i32 13}
!9 = !{i32 15, !10}
!10 = !{!"explicit_arg_num", i32 0}
