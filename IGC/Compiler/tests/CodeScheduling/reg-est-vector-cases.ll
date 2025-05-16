;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --igc-code-scheduling -S %s 2>&1 | FileCheck %s


; Checks that the register pressure is estimated correctly for the special cases related to vector shuffles.

define spir_kernel void @vector_shuffle_no_op(ptr addrspace(1) %A) {
; CHECK: Function vector_shuffle_no_op
; CHECK: Greedy MW attempt
; CHECK: {{([0-9]+,[ ]*[0-9]+[ ]*).*[ ]*}}        [[BASE_ADDR:%.*]] = ptrtoint ptr addrspace(1) [[A:%.*]] to i64

;  (6, 512     ) MW:         Node #1, MW: 3000        %load2d = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
;                      adds 512 bytes
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}        [[LOAD2D:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

;                      the EE and IE instructions are marked as NOP and don't add regpressure
; (22, 0      ) Im:   NOP   Node #2, MW: 3000            %EE1.0 = extractelement <16 x i16> %load2d, i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_0:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_0:%.*]] = insertelement <8 x i16> undef, i16 [[EE1_0]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_1:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_1:%.*]] = insertelement <8 x i16> [[IE1_0]], i16 [[EE1_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_2:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_2:%.*]] = insertelement <8 x i16> [[IE1_1]], i16 [[EE1_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_3:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_3:%.*]] = insertelement <8 x i16> [[IE1_2]], i16 [[EE1_3]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_4:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_4:%.*]] = insertelement <8 x i16> [[IE1_3]], i16 [[EE1_4]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_5:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_5:%.*]] = insertelement <8 x i16> [[IE1_4]], i16 [[EE1_5]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_6:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_6:%.*]] = insertelement <8 x i16> [[IE1_5]], i16 [[EE1_6]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_7:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 7
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_7:%.*]] = insertelement <8 x i16> [[IE1_6]], i16 [[EE1_7]], i32 7
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_0:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 8
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_0:%.*]] = insertelement <8 x i16> undef, i16 [[EE2_0]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_1:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 9
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_1:%.*]] = insertelement <8 x i16> [[IE2_0]], i16 [[EE2_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_2:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 10
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_2:%.*]] = insertelement <8 x i16> [[IE2_1]], i16 [[EE2_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_3:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 11
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_3:%.*]] = insertelement <8 x i16> [[IE2_2]], i16 [[EE2_3]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_4:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 12
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_4:%.*]] = insertelement <8 x i16> [[IE2_3]], i16 [[EE2_4]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_5:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 13
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_5:%.*]] = insertelement <8 x i16> [[IE2_4]], i16 [[EE2_5]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_6:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 14
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_6:%.*]] = insertelement <8 x i16> [[IE2_5]], i16 [[EE2_6]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_7:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 15
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_7:%.*]] = insertelement <8 x i16> [[IE2_6]], i16 [[EE2_7]], i32 7

;  (22, 512    ) MW:         Node #34, MW: 0          %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE1.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
;  (38, 0      ) MW:         Node #35, MW: 0          %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE2.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; first DPAS increases the regpressure by 512 bytes, the second one doesn't add any regpressure because the whole vector dies
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}        [[DPAS1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[IE1_7]], <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}          [[DPAS2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[IE2_7]], <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)

; CHECK:         ret void
;
entry:
  %base_addr = ptrtoint ptr addrspace(1) %A to i64
  %load2d = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %EE1.0 = extractelement <16 x i16> %load2d, i32 0
  %IE1.0 = insertelement <8 x i16> undef, i16 %EE1.0, i32 0
  %EE1.1 = extractelement <16 x i16> %load2d, i32 1
  %IE1.1 = insertelement <8 x i16> %IE1.0, i16 %EE1.1, i32 1
  %EE1.2 = extractelement <16 x i16> %load2d, i32 2
  %IE1.2 = insertelement <8 x i16> %IE1.1, i16 %EE1.2, i32 2
  %EE1.3 = extractelement <16 x i16> %load2d, i32 3
  %IE1.3 = insertelement <8 x i16> %IE1.2, i16 %EE1.3, i32 3
  %EE1.4 = extractelement <16 x i16> %load2d, i32 4
  %IE1.4 = insertelement <8 x i16> %IE1.3, i16 %EE1.4, i32 4
  %EE1.5 = extractelement <16 x i16> %load2d, i32 5
  %IE1.5 = insertelement <8 x i16> %IE1.4, i16 %EE1.5, i32 5
  %EE1.6 = extractelement <16 x i16> %load2d, i32 6
  %IE1.6 = insertelement <8 x i16> %IE1.5, i16 %EE1.6, i32 6
  %EE1.7 = extractelement <16 x i16> %load2d, i32 7
  %IE1.7 = insertelement <8 x i16> %IE1.6, i16 %EE1.7, i32 7

  %EE2.0 = extractelement <16 x i16> %load2d, i32 8
  %IE2.0 = insertelement <8 x i16> undef, i16 %EE2.0, i32 0
  %EE2.1 = extractelement <16 x i16> %load2d, i32 9
  %IE2.1 = insertelement <8 x i16> %IE2.0, i16 %EE2.1, i32 1
  %EE2.2 = extractelement <16 x i16> %load2d, i32 10
  %IE2.2 = insertelement <8 x i16> %IE2.1, i16 %EE2.2, i32 2
  %EE2.3 = extractelement <16 x i16> %load2d, i32 11
  %IE2.3 = insertelement <8 x i16> %IE2.2, i16 %EE2.3, i32 3
  %EE2.4 = extractelement <16 x i16> %load2d, i32 12
  %IE2.4 = insertelement <8 x i16> %IE2.3, i16 %EE2.4, i32 4
  %EE2.5 = extractelement <16 x i16> %load2d, i32 13
  %IE2.5 = insertelement <8 x i16> %IE2.4, i16 %EE2.5, i32 5
  %EE2.6 = extractelement <16 x i16> %load2d, i32 14
  %IE2.6 = insertelement <8 x i16> %IE2.5, i16 %EE2.6, i32 6
  %EE2.7 = extractelement <16 x i16> %load2d, i32 15
  %IE2.7 = insertelement <8 x i16> %IE2.6, i16 %EE2.7, i32 7

  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE1.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE2.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}


define spir_kernel void @vector_shuffle(ptr addrspace(1) %A) {
; CHECK: Function vector_shuffle
; CHECK: {{([0-9]+,[ ]*[0-9]+[ ]*).*[ ]*}}        [[BASE_ADDR:%.*]] = ptrtoint ptr addrspace(1) [[A:%.*]] to i64

;  (6, 512     ) MW:         Node #1, MW: 3000        %load2d = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
;                      adds 512 bytes
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}        [[LOAD2D:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)


;                      the EE and IE instructions are marked as VS. IEs add regpressure. The last IE kills the original vector

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_0:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 0
; CHECK: {{([0-9]+,[ ]*256[ ]*).*VS.*[ ]*}}       [[IE1_0:%.*]] = insertelement <8 x i16> undef, i16 [[EE1_0]], i32 0

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_1:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_1:%.*]] = insertelement <8 x i16> [[IE1_0]], i16 [[EE1_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_2:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_2:%.*]] = insertelement <8 x i16> [[IE1_1]], i16 [[EE1_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_3:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_3:%.*]] = insertelement <8 x i16> [[IE1_2]], i16 [[EE1_3]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_4:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 8
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_4:%.*]] = insertelement <8 x i16> [[IE1_3]], i16 [[EE1_4]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_5:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 10
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_5:%.*]] = insertelement <8 x i16> [[IE1_4]], i16 [[EE1_5]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_6:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 12
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_6:%.*]] = insertelement <8 x i16> [[IE1_5]], i16 [[EE1_6]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_7:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 14
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_7:%.*]] = insertelement <8 x i16> [[IE1_6]], i16 [[EE1_7]], i32 7

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_0:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 1
; CHECK: {{([0-9]+,[ ]*256[ ]*).*VS.*[ ]*}}       [[IE2_0:%.*]] = insertelement <8 x i16> undef, i16 [[EE2_0]], i32 0

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_1:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_1:%.*]] = insertelement <8 x i16> [[IE2_0]], i16 [[EE2_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_2:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_2:%.*]] = insertelement <8 x i16> [[IE2_1]], i16 [[EE2_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_3:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 7
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_3:%.*]] = insertelement <8 x i16> [[IE2_2]], i16 [[EE2_3]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_4:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 9
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_4:%.*]] = insertelement <8 x i16> [[IE2_3]], i16 [[EE2_4]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_5:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 11
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_5:%.*]] = insertelement <8 x i16> [[IE2_4]], i16 [[EE2_5]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_6:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 13
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_6:%.*]] = insertelement <8 x i16> [[IE2_5]], i16 [[EE2_6]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_7:%.*]] = extractelement <16 x i16> [[LOAD2D]], i32 15

; CHECK: {{([0-9]+,[ ]*-512[ ]*).*VS.*[ ]*}}      [[IE2_7:%.*]] = insertelement <8 x i16> [[IE2_6]], i16 [[EE2_7]], i32 7

;                       both DPAS increase the regpressure by 256 (sub vector of 256 dies, 512 created)
; CHECK: {{([0-9]+,[ ]*256[ ]*).*[ ]*}}          [[DPAS1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[IE1_7]], <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK: {{([0-9]+,[ ]*256[ ]*).*[ ]*}}          [[DPAS2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[IE2_7]], <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK:         ret void
;
entry:
  %base_addr = ptrtoint ptr addrspace(1) %A to i64
  %load2d = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %EE1.0 = extractelement <16 x i16> %load2d, i32 0
  %IE1.0 = insertelement <8 x i16> undef, i16 %EE1.0, i32 0
  %EE1.1 = extractelement <16 x i16> %load2d, i32 2
  %IE1.1 = insertelement <8 x i16> %IE1.0, i16 %EE1.1, i32 1
  %EE1.2 = extractelement <16 x i16> %load2d, i32 4
  %IE1.2 = insertelement <8 x i16> %IE1.1, i16 %EE1.2, i32 2
  %EE1.3 = extractelement <16 x i16> %load2d, i32 6
  %IE1.3 = insertelement <8 x i16> %IE1.2, i16 %EE1.3, i32 3
  %EE1.4 = extractelement <16 x i16> %load2d, i32 8
  %IE1.4 = insertelement <8 x i16> %IE1.3, i16 %EE1.4, i32 4
  %EE1.5 = extractelement <16 x i16> %load2d, i32 10
  %IE1.5 = insertelement <8 x i16> %IE1.4, i16 %EE1.5, i32 5
  %EE1.6 = extractelement <16 x i16> %load2d, i32 12
  %IE1.6 = insertelement <8 x i16> %IE1.5, i16 %EE1.6, i32 6
  %EE1.7 = extractelement <16 x i16> %load2d, i32 14
  %IE1.7 = insertelement <8 x i16> %IE1.6, i16 %EE1.7, i32 7
  %EE2.0 = extractelement <16 x i16> %load2d, i32 1
  %IE2.0 = insertelement <8 x i16> undef, i16 %EE2.0, i32 0
  %EE2.1 = extractelement <16 x i16> %load2d, i32 3
  %IE2.1 = insertelement <8 x i16> %IE2.0, i16 %EE2.1, i32 1
  %EE2.2 = extractelement <16 x i16> %load2d, i32 5
  %IE2.2 = insertelement <8 x i16> %IE2.1, i16 %EE2.2, i32 2
  %EE2.3 = extractelement <16 x i16> %load2d, i32 7
  %IE2.3 = insertelement <8 x i16> %IE2.2, i16 %EE2.3, i32 3
  %EE2.4 = extractelement <16 x i16> %load2d, i32 9
  %IE2.4 = insertelement <8 x i16> %IE2.3, i16 %EE2.4, i32 4
  %EE2.5 = extractelement <16 x i16> %load2d, i32 11
  %IE2.5 = insertelement <8 x i16> %IE2.4, i16 %EE2.5, i32 5
  %EE2.6 = extractelement <16 x i16> %load2d, i32 13
  %IE2.6 = insertelement <8 x i16> %IE2.5, i16 %EE2.6, i32 6
  %EE2.7 = extractelement <16 x i16> %load2d, i32 15
  %IE2.7 = insertelement <8 x i16> %IE2.6, i16 %EE2.7, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE1.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %IE2.7, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}


define spir_kernel void @coalesced_scalars(ptr addrspace(1) %0) {
; CHECK: Function coalesced_scalars

;               the IE instructions are marked as SCA. First IE adds regpressure
;               then the last usage of the scalar (fadd) kills the hanging values

; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP17:%.*]] = fmul fast float [[TMP9:%.*]], [[TMP1:%.*]]
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP18:%.*]] = fmul fast float [[TMP10:%.*]], [[TMP2:%.*]]
; CHECK: {{([0-9]+,[ ]*512[ ]*).*SCA.*[ ]*}}            [[TMP19:%.*]] = insertelement <8 x float> zeroinitializer, float [[TMP17]], i64 0
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP20:%.*]] = fmul fast float [[TMP11:%.*]], [[TMP3:%.*]]
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*SCA.*[ ]*}}        [[TMP21:%.*]] = insertelement <8 x float> [[TMP19]], float [[TMP18]], i64 1
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP22:%.*]] = fmul fast float [[TMP12:%.*]], [[TMP4:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP23:%.*]] = insertelement <8 x float> [[TMP21]], float [[TMP20]], i64 2
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP24:%.*]] = fmul fast float [[TMP13:%.*]], [[TMP5:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP25:%.*]] = insertelement <8 x float> [[TMP23]], float [[TMP22]], i64 3
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP26:%.*]] = fmul fast float [[TMP14:%.*]], [[TMP6:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP27:%.*]] = insertelement <8 x float> [[TMP25]], float [[TMP24]], i64 4
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP28:%.*]] = fmul fast float [[TMP15:%.*]], [[TMP7:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP29:%.*]] = insertelement <8 x float> [[TMP27]], float [[TMP26]], i64 5
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP30:%.*]] = fmul fast float [[TMP16:%.*]], [[TMP8:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP31:%.*]] = insertelement <8 x float> [[TMP29]], float [[TMP28]], i64 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP32:%.*]] = insertelement <8 x float> [[TMP31]], float [[TMP30]], i64 7

;            dpas don't add any regpressure, they reuse the registers of the created vector
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}                   [[TMP33:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP32]], <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}                   [[TMP34:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP33]], <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

;            extract the values from the vector. The extractelement instructions are marked as V2S and don't increase regpressure
;            the vector hangs, so the last EE doesn't reduce regpressure. In this case the EEs are used in PHI nodes so these hanging vals are not killed
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP35:%.*]] = extractelement <8 x float> [[TMP34]], i64 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP36:%.*]] = extractelement <8 x float> [[TMP34]], i64 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP37:%.*]] = extractelement <8 x float> [[TMP34]], i64 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP38:%.*]] = extractelement <8 x float> [[TMP34]], i64 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP39:%.*]] = extractelement <8 x float> [[TMP34]], i64 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP40:%.*]] = extractelement <8 x float> [[TMP34]], i64 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP41:%.*]] = extractelement <8 x float> [[TMP34]], i64 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP42:%.*]] = extractelement <8 x float> [[TMP34]], i64 7

;            fadd kills the hanging values from SCA and creates one float -> -448
; CHECK: {{([0-9]+,[ ]*-448[ ]*).*[ ]*}}                [[TMP50:%.*]] = fadd fast float [[TMP37:%.*]], [[TMP26]]
;            this fadd doesn't kill the hanging EEs (they are used in PHI nodes), so it increases regpressure by 64
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}                  [[TMP51:%.*]] = fadd fast float [[TMP42:%.*]], 4.000000e+00

  entry:
  br label %._crit_edge

  ._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi float [ 0.000000e+00, %entry ], [ %35, %._crit_edge ]
  %2 = phi float [ 0.000000e+00, %entry ], [ %36, %._crit_edge ]
  %3 = phi float [ 0.000000e+00, %entry ], [ %37, %._crit_edge ]
  %4 = phi float [ 0.000000e+00, %entry ], [ %38, %._crit_edge ]
  %5 = phi float [ 0.000000e+00, %entry ], [ %39, %._crit_edge ]
  %6 = phi float [ 0.000000e+00, %entry ], [ %40, %._crit_edge ]
  %7 = phi float [ 0.000000e+00, %entry ], [ %41, %._crit_edge ]
  %8 = phi float [ 0.000000e+00, %entry ], [ %42, %._crit_edge ]
  %9 = call float @llvm.exp2.f32(float 0.000000e+00)
  %10 = call float @llvm.exp2.f32(float 0.000000e+00)
  %11 = call float @llvm.exp2.f32(float 0.000000e+00)
  %12 = call float @llvm.exp2.f32(float 0.000000e+00)
  %13 = call float @llvm.exp2.f32(float 0.000000e+00)
  %14 = call float @llvm.exp2.f32(float 0.000000e+00)
  %15 = call float @llvm.exp2.f32(float 0.000000e+00)
  %16 = call float @llvm.exp2.f32(float 0.000000e+00)
  %17 = fmul fast float %9, %1
  %18 = fmul fast float %10, %2
  %19 = fmul fast float %11, %3
  %20 = fmul fast float %12, %4
  %21 = fmul fast float %13, %5
  %22 = fmul fast float %14, %6
  %23 = fmul fast float %15, %7
  %24 = fmul fast float %16, %8
  %25 = insertelement <8 x float> zeroinitializer, float %17, i64 0
  %26 = insertelement <8 x float> %25, float %18, i64 1
  %27 = insertelement <8 x float> %26, float %19, i64 2
  %28 = insertelement <8 x float> %27, float %20, i64 3
  %29 = insertelement <8 x float> %28, float %21, i64 4
  %30 = insertelement <8 x float> %29, float %22, i64 5
  %31 = insertelement <8 x float> %30, float %23, i64 6
  %32 = insertelement <8 x float> %31, float %24, i64 7
  %33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %33, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %35 = extractelement <8 x float> %34, i64 0
  %36 = extractelement <8 x float> %34, i64 1
  %37 = extractelement <8 x float> %34, i64 2
  %38 = extractelement <8 x float> %34, i64 3
  %39 = extractelement <8 x float> %34, i64 4
  %40 = extractelement <8 x float> %34, i64 5
  %41 = extractelement <8 x float> %34, i64 6
  %42 = extractelement <8 x float> %34, i64 7
  %43 = fadd fast float %37, %22
  %44 = fadd fast float %42, 4.000000e+00

  br label %._crit_edge
}


define spir_kernel void @vector_to_scalars_pattern(ptr addrspace(1) %A) {
; CHECK: Function vector_to_scalars_pattern

;           DPAS increases regpressure
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}           [[DPAS:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)

;           EE don't increase regpressure
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE0:%.*]] = extractelement <8 x float> [[DPAS]], i64 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE1:%.*]] = extractelement <8 x float> [[DPAS]], i64 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE2:%.*]] = extractelement <8 x float> [[DPAS]], i64 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE3:%.*]] = extractelement <8 x float> [[DPAS]], i64 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE4:%.*]] = extractelement <8 x float> [[DPAS]], i64 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE5:%.*]] = extractelement <8 x float> [[DPAS]], i64 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE6:%.*]] = extractelement <8 x float> [[DPAS]], i64 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE7:%.*]] = extractelement <8 x float> [[DPAS]], i64 7

;           The vector doesn't die on the last EE, it hangs.
;           The uses of the EEs increase regpressure
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE0:%.*]] = fadd fast float [[EE0]], 1.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE1:%.*]] = fadd fast float [[EE1]], 2.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE2:%.*]] = fadd fast float [[EE2]], 3.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE3:%.*]] = fadd fast float [[EE3]], 4.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE4:%.*]] = fadd fast float [[EE4]], 5.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE5:%.*]] = fadd fast float [[EE5]], 6.0
; CHECK: {{([0-9]+,[ ]*64[ ]*).*[ ]*}}            [[USE6:%.*]] = fadd fast float [[EE6]], 7.0
;           The vector dies on the last EE usage
; CHECK: {{([0-9]+,[ ]*-448[ ]*).*[ ]*}}          [[USE7:%.*]] = fadd fast float [[EE7]], 8.0

; CHECK:         ret void

entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %ee0 = extractelement <8 x float> %dpas, i64 0
  %ee1 = extractelement <8 x float> %dpas, i64 1
  %ee2 = extractelement <8 x float> %dpas, i64 2
  %ee3 = extractelement <8 x float> %dpas, i64 3
  %ee4 = extractelement <8 x float> %dpas, i64 4
  %ee5 = extractelement <8 x float> %dpas, i64 5
  %ee6 = extractelement <8 x float> %dpas, i64 6
  %ee7 = extractelement <8 x float> %dpas, i64 7
  %use0 = fadd fast float %ee0, 1.0
  %use1 = fadd fast float %ee1, 2.0
  %use2 = fadd fast float %ee2, 3.0
  %use3 = fadd fast float %ee3, 4.0
  %use4 = fadd fast float %ee4, 5.0
  %use5 = fadd fast float %ee5, 6.0
  %use6 = fadd fast float %ee6, 7.0
  %use7 = fadd fast float %ee7, 8.0
  ret void
}



declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

declare float @llvm.exp2.f32(float) #2

declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1
attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
