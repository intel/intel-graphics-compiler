;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -platformdg1 -igc-priv-mem-to-reg 2>&1 | FileCheck -check-prefix=CHECK-DG1 %s
; RUN: igc_opt --typed-pointers %s -S -o - -platformdg2 -igc-priv-mem-to-reg 2>&1 | FileCheck -check-prefix=CHECK-DG2 %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------

@WaveletDenoise.buffer = external addrspace(3) global [4096 x float]

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

define spir_kernel void @WaveletDenoise(<4 x i8> addrspace(1)* %srcImage, <4 x i8> addrspace(1)* %dstImage, float %threshold, i32 %passes, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8 addrspace(2)* %constBase, i8* %privateBase) {
; CHECK-DG1-LABEL: @WaveletDenoise(
; CHECK-DG1-NEXT:  entry:
; CHECK-DG1-NEXT:    [[I:%.*]] = extractelement <3 x i32> [[LOCALSIZE:%.*]], i64 0
; CHECK-DG1-NEXT:    [[TMP0:%.*]] = alloca <64 x i8>
;
; CHECK-DG2-LABEL: @WaveletDenoise(
; CHECK-DG2-NEXT:  entry:
; CHECK-DG2-NEXT:    [[I:%.*]] = extractelement <3 x i32> [[LOCALSIZE:%.*]], i64 0
; CHECK-DG2-NEXT:    [[STAGE:%.*]] = alloca [16 x <4 x i8>]
; CHECK-DG2-NEXT:    [[TMP0:%.*]] = alloca <16 x float>
; CHECK-DG2-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-DG2:         [[I5:%.*]] = getelementptr inbounds [16 x <4 x i8>], [16 x <4 x i8>]* [[STAGE]]

entry:
  %i = extractelement <3 x i32> %localSize, i64 0
  %stage = alloca [16 x <4 x i8>], align 4
  %tmp = alloca [16 x float], align 4
  %accum = alloca [16 x float], align 4
  %sub = add i32 %passes, 0
  %i5 = getelementptr inbounds [16 x <4 x i8>], [16 x <4 x i8>]* %stage, i64 0, i64 0, i64 0
  %conv.i25 = zext i16 %localIdX to i32
  %add7 = add i32 0, %passes
  %i8 = icmp ugt i16 0, 0
  br label %entry.for.cond37.preheader_crit_edge

entry.for.cond37.preheader_crit_edge:             ; preds = %entry
  br label %for.cond37.preheader

for.cond37.preheader:                             ; preds = %entry.for.cond37.preheader_crit_edge
  %conv.i96 = zext i16 %localIdX to i64
  %i12 = bitcast [16 x float]* %accum to i8*
  %arrayidx127 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 15
  %arrayidx128 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 14
  %arrayidx129 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 13
  %arrayidx130 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 12
  %arrayidx131 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 11
  %arrayidx132 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 10
  %arrayidx133 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 9
  %arrayidx134 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 8
  %arrayidx135 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 7
  %arrayidx137 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 6
  %arrayidx138 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 5
  %arrayidx139 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 4
  %arrayidx140 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 3
  %arrayidx141 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 2
  %arrayidx142 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 1
  %arrayidx143 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 0
  %sub244 = add nsw i32 %passes, 0
  br label %for.cond37.preheader.sw.epilog_crit_edge

for.cond37.preheader.sw.epilog_crit_edge:         ; preds = %for.cond37.preheader
  br label %sw.epilog

cond-add-join186:                                 ; No predecessors!
  %arrayidx52 = getelementptr inbounds [16 x <4 x i8>], [16 x <4 x i8>]* %stage, i64 0, i64 undef
  %i16 = bitcast <4 x i8>* %arrayidx52 to i8*
  %i17 = load i8, i8* %i16, align 4
  ret void

sw.epilog:                                        ; preds = %for.cond37.preheader.sw.epilog_crit_edge
  br label %sw.epilog.sw.bb294_crit_edge

sw.epilog.sw.bb294_crit_edge:                     ; preds = %sw.epilog
  br label %sw.bb294

sw.bb294:                                         ; preds = %sw.epilog.sw.bb294_crit_edge
  br label %sw.bb294.sw.epilog360_crit_edge

sw.bb294.sw.epilog360_crit_edge:                  ; preds = %sw.bb294
  br label %sw.epilog360

cond-add-join222:                                 ; No predecessors!
  %arrayidx305 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 undef
  %i33 = load float, float* %arrayidx305, align 4
  %conv.i277 = fptoui float %i33 to i8
  %arrayidx309 = getelementptr inbounds [16 x <4 x i8>], [16 x <4 x i8>]* %stage, i64 0, i64 undef
  %i35 = insertelement <4 x i8> zeroinitializer, i8 %conv.i277, i64 0
  store <4 x i8> %i35, <4 x i8>* %arrayidx309, align 4
  ret void

sw.epilog360:                                     ; preds = %sw.bb294.sw.epilog360_crit_edge
  br label %sw.epilog360.sw.epilog.1_crit_edge

sw.epilog360.sw.epilog.1_crit_edge:               ; preds = %sw.epilog360
  br label %sw.epilog.1

sw.epilog.1:                                      ; preds = %sw.epilog360.sw.epilog.1_crit_edge
  br label %sw.epilog.1.for.body117.1_crit_edge

sw.epilog.1.for.body117.1_crit_edge:              ; preds = %sw.epilog.1
  br label %for.body117.1

for.body117.1:                                    ; preds = %if.end291.1.for.body117.1_crit_edge, %sw.epilog.1.for.body117.1_crit_edge
  br label %if.then.1

if.then.1:                                        ; preds = %for.body117.1
  store float 0.000000e+00, float* %arrayidx127, align 4
  store float 0.000000e+00, float* %arrayidx128, align 4
  store float 0.000000e+00, float* %arrayidx129, align 4
  store float 0.000000e+00, float* %arrayidx130, align 4
  store float 0.000000e+00, float* %arrayidx131, align 4
  store float 0.000000e+00, float* %arrayidx132, align 4
  store float 0.000000e+00, float* %arrayidx133, align 4
  store float 0.000000e+00, float* %arrayidx134, align 4
  store float 0.000000e+00, float* %arrayidx135, align 4
  store float 0.000000e+00, float* %arrayidx137, align 4
  store float 0.000000e+00, float* %arrayidx138, align 4
  store float 0.000000e+00, float* %arrayidx139, align 4
  store float 0.000000e+00, float* %arrayidx140, align 4
  store float 0.000000e+00, float* %arrayidx141, align 4
  store float 0.000000e+00, float* %arrayidx142, align 4
  store float 0.000000e+00, float* %arrayidx143, align 4
  br label %if.end.1

if.end.1:                                         ; preds = %if.then.1
  br i1 %i8, label %if.end.1.for.end187.1_crit_edge, label %for.body151.lr.ph.1

if.end.1.for.end187.1_crit_edge:                  ; preds = %if.end.1
  br label %for.end187.1

for.body151.lr.ph.1:                              ; preds = %if.end.1
  %add172.1 = add nuw nsw i32 0, %conv.i25
  br label %for.body151.1

for.body151.1:                                    ; preds = %if.end162.1.for.body151.1_crit_edge, %for.body151.lr.ph.1
  %i144.0152.1 = phi i32 [ 0, %for.body151.lr.ph.1 ], [ 0, %if.end162.1.for.body151.1_crit_edge ]
  %mul152.1 = shl nsw i32 %passes, 0
  ret void

cond-add-join234:                                 ; No predecessors!
  %.pre177 = sext i32 %passes to i64
  br label %if.end162.1

if.end162.1:                                      ; preds = %cond-add-join234
  %arrayidx165.1 = getelementptr inbounds [16 x float], [16 x float]* %tmp, i64 0, i64 undef
  %i42 = load float, float* %arrayidx165.1, align 4
  %idxprom170.1 = sext i32 %passes to i64
  %arrayidx171.1 = getelementptr inbounds [4096 x float], [4096 x float] addrspace(3)* @WaveletDenoise.buffer, i64 0, i64 %idxprom170.1
  %add174.1 = add nsw i32 %add172.1, %mul152.1
  %mul.i194.1 = fmul float %i42, 0.000000e+00
  %arrayidx181.1 = getelementptr inbounds [4096 x float], [4096 x float] addrspace(3)* @WaveletDenoise.buffer, i64 0, i64 %.pre177
  %add185.1 = add i32 %i144.0152.1, %i
  br label %if.end162.1.for.body151.1_crit_edge

if.end162.1.for.body151.1_crit_edge:              ; preds = %if.end162.1
  br label %for.body151.1

for.end187.1:                                     ; preds = %if.end.1.for.end187.1_crit_edge
  br i1 false, label %for.end187.1.for.end243.1_crit_edge, label %for.body195.lr.ph.1

for.end187.1.for.end243.1_crit_edge:              ; preds = %for.end187.1
  br label %for.end243.1

for.body195.lr.ph.1:                              ; preds = %for.end187.1
  br label %for.body195.1

for.body195.1:                                    ; preds = %for.body195.lr.ph.1
  br label %for.body195.1.cond-add-join246_crit_edge

for.body195.1.cond-add-join246_crit_edge:         ; preds = %for.body195.1
  br label %cond-add-join246

cond-add-join246:                                 ; preds = %for.body195.1.cond-add-join246_crit_edge
  %arrayidx217.1 = getelementptr inbounds [16 x float], [16 x float]* %tmp, i64 0, i64 undef
  store float 0.000000e+00, float* %arrayidx217.1, align 4
  %arrayidx236.1 = getelementptr inbounds [16 x float], [16 x float]* %accum, i64 0, i64 undef
  ret void

for.end243.1:                                     ; preds = %for.end187.1.for.end243.1_crit_edge
  %cmp245.1 = icmp slt i32 0, %sub244
  br label %for.cond273.preheader.1

for.cond273.preheader.1:                          ; preds = %for.end243.1
  br label %for.cond273.preheader.1.if.end291.1_crit_edge

for.cond273.preheader.1.if.end291.1_crit_edge:    ; preds = %for.cond273.preheader.1
  br label %if.end291.1

if.end291.1:                                      ; preds = %for.cond273.preheader.1.if.end291.1_crit_edge
  %cmp114.1 = icmp slt i32 0, %passes
  br i1 %cmp114.1, label %if.end291.1.for.body117.1_crit_edge, label %if.end291.1.for.cond320.preheader.1_crit_edge

if.end291.1.for.cond320.preheader.1_crit_edge:    ; preds = %if.end291.1
  br label %for.cond320.preheader.1

if.end291.1.for.body117.1_crit_edge:              ; preds = %if.end291.1
  br label %for.body117.1

for.cond320.preheader.1:                          ; preds = %if.end291.1.for.cond320.preheader.1_crit_edge
  br label %for.cond320.preheader.1.sw.epilog360.1_crit_edge

for.cond320.preheader.1.sw.epilog360.1_crit_edge: ; preds = %for.cond320.preheader.1
  br label %sw.epilog360.1

sw.epilog360.1:                                   ; preds = %for.cond320.preheader.1.sw.epilog360.1_crit_edge
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %i12)
  br label %sw.epilog360.1.sw.epilog.2_crit_edge

sw.epilog360.1.sw.epilog.2_crit_edge:             ; preds = %sw.epilog360.1
  br label %sw.epilog.2

sw.epilog.2:                                      ; preds = %sw.epilog360.1.sw.epilog.2_crit_edge
  br label %sw.epilog.2.for.cond342.preheader.2_crit_edge

sw.epilog.2.for.cond342.preheader.2_crit_edge:    ; preds = %sw.epilog.2
  br label %for.cond342.preheader.2

for.cond342.preheader.2:                          ; preds = %sw.epilog.2.for.cond342.preheader.2_crit_edge
  br label %for.cond342.preheader.2.sw.epilog360.2_crit_edge

for.cond342.preheader.2.sw.epilog360.2_crit_edge: ; preds = %for.cond342.preheader.2
  br label %sw.epilog360.2

sw.epilog360.2:                                   ; preds = %for.cond342.preheader.2.sw.epilog360.2_crit_edge
  %cmp366.not = icmp ult i64 %conv.i96, 0
  br i1 false, label %sw.epilog360.2.if.end418_crit_edge, label %land.lhs.true

sw.epilog360.2.if.end418_crit_edge:               ; preds = %sw.epilog360.2
  br label %if.end418

land.lhs.true:                                    ; preds = %sw.epilog360.2
  %sub369 = sub nsw i32 0, %sub
  br label %land.lhs.true.for.body387_crit_edge

land.lhs.true.for.body387_crit_edge:              ; preds = %land.lhs.true
  br label %for.body387

for.body387:                                      ; preds = %land.lhs.true.for.body387_crit_edge
  br label %if.then402

if.then402:                                       ; preds = %for.body387
  br label %if.then402.cond-add-join312_crit_edge

if.then402.cond-add-join312_crit_edge:            ; preds = %if.then402
  br label %cond-add-join312

cond-add-join312:                                 ; preds = %if.then402.cond-add-join312_crit_edge
  %arrayidx405 = getelementptr inbounds [16 x <4 x i8>], [16 x <4 x i8>]* %stage, i64 0, i64 undef
  %add408 = add nsw i32 %add7, 0
  ret void

if.end418:                                        ; preds = %sw.epilog360.2.if.end418_crit_edge
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %i5)
  ret void
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #1

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = !{void (<4 x i8> addrspace(1)*, <4 x i8> addrspace(1)*, float, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32)* bitcast (void (<4 x i8> addrspace(1)*, <4 x i8> addrspace(1)*, float, i32, <3 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*)* @WaveletDenoise to void (<4 x i8> addrspace(1)*, <4 x i8> addrspace(1)*, float, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32)*), !1}
!1 = !{!2, !3, !16}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8, !9, !10, !11, !12, !14}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 6}
!7 = !{i32 8}
!8 = !{i32 9}
!9 = !{i32 10}
!10 = !{i32 11}
!11 = !{i32 13}
!12 = !{i32 15, !13}
!13 = !{!"explicit_arg_num", i32 0}
!14 = !{i32 15, !15}
!15 = !{!"explicit_arg_num", i32 1}
!16 = !{!"thread_group_size", i32 64, i32 4, i32 1}
