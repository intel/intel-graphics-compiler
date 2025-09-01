;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --igc-code-scheduling \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --regkey "CodeSchedulingConfig=10;1;0;30000;100;0;100000;1000;6000;200;10;20;500;50;0;1;1;0;1;1;8;1;32;1;200;64;0;1;20;200;200;5;16;16;34;200;1;0;128;256" \
; RUN:         --regkey ForceOCLSIMDWidth=32 -S %s 2>&1 | FileCheck %s


; Checks that the register pressure is estimated correctly for the special cases related to vector shuffles.

define spir_kernel void @vector_shuffle_no_op(ptr addrspace(1) %A) {
; CHECK: Function vector_shuffle_no_op
; CHECK: Greedy MW attempt

; CHECK: {{([0-9]+,[ ]*[0-9]+[ ]*).*[ ]*}}        [[BASE_ADDR:%.*]] = ptrtoint ptr addrspace(1) [[A:%.*]] to i64

;  (6, 512     ) MW:         Node #1, MW: 3000        %load2d = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
;                      adds 512 bytes
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}        [[LOAD2D:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

;                      the EE and IE instructions are marked as NOP and don't add regpressure
; (22, 0      ) Im:   NOP   Node #2, MW: 3000            %EE1.0 = extractelement <8 x i16> %load2d, i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_0:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_0:%.*]] = insertelement <4 x i16> undef, i16 [[EE1_0]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_1:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_1:%.*]] = insertelement <4 x i16> [[IE1_0]], i16 [[EE1_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_2:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_2:%.*]] = insertelement <4 x i16> [[IE1_1]], i16 [[EE1_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE1_3:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE1_3:%.*]] = insertelement <4 x i16> [[IE1_2]], i16 [[EE1_3]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_0:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_0:%.*]] = insertelement <4 x i16> undef, i16 [[EE2_0]], i32 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_1:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_1:%.*]] = insertelement <4 x i16> [[IE2_0]], i16 [[EE2_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_2:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_2:%.*]] = insertelement <4 x i16> [[IE2_1]], i16 [[EE2_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[EE2_3:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 7
; CHECK: {{([0-9]+,[ ]*0[ ]*).*NOP.*[ ]*}}        [[IE2_3:%.*]] = insertelement <4 x i16> [[IE2_2]], i16 [[EE2_3]], i32 3

;  (22, 512    ) MW:         Node #34, MW: 0          %dpas1 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE1.7, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
;  (38, 0      ) MW:         Node #35, MW: 0          %dpas2 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE2.7, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; first DPAS increases the regpressure by 512 bytes, the second one doesn't add any regpressure because the whole vector dies
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}        [[DPAS1:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> [[IE1_3]], <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}          [[DPAS2:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> [[IE2_3]], <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)

; CHECK:         ret void
;
entry:
  %base_addr = ptrtoint ptr addrspace(1) %A to i64
  %load2d = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %EE1.0 = extractelement <8 x i16> %load2d, i32 0
  %IE1.0 = insertelement <4 x i16> undef, i16 %EE1.0, i32 0
  %EE1.1 = extractelement <8 x i16> %load2d, i32 1
  %IE1.1 = insertelement <4 x i16> %IE1.0, i16 %EE1.1, i32 1
  %EE1.2 = extractelement <8 x i16> %load2d, i32 2
  %IE1.2 = insertelement <4 x i16> %IE1.1, i16 %EE1.2, i32 2
  %EE1.3 = extractelement <8 x i16> %load2d, i32 3
  %IE1.3 = insertelement <4 x i16> %IE1.2, i16 %EE1.3, i32 3

  %EE2.0 = extractelement <8 x i16> %load2d, i32 4
  %IE2.0 = insertelement <4 x i16> undef, i16 %EE2.0, i32 0
  %EE2.1 = extractelement <8 x i16> %load2d, i32 5
  %IE2.1 = insertelement <4 x i16> %IE2.0, i16 %EE2.1, i32 1
  %EE2.2 = extractelement <8 x i16> %load2d, i32 6
  %IE2.2 = insertelement <4 x i16> %IE2.1, i16 %EE2.2, i32 2
  %EE2.3 = extractelement <8 x i16> %load2d, i32 7
  %IE2.3 = insertelement <4 x i16> %IE2.2, i16 %EE2.3, i32 3

  %dpas1 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE1.3, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE2.3, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}


define spir_kernel void @vector_shuffle(ptr addrspace(1) %A) {
; CHECK: Function vector_shuffle
; CHECK: Greedy MW attempt

; CHECK: {{([0-9]+,[ ]*[0-9]+[ ]*).*[ ]*}}        [[BASE_ADDR:%.*]] = ptrtoint ptr addrspace(1) [[A:%.*]] to i64

;  (6, 512     ) MW:         Node #1, MW: 3000        %load2d = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
;                      adds 512 bytes, but we also estimate the regpressure burst from the shuffles, so use 2x (1024 bytes) when making the decision
; CHECK: {{([0-9]+,[ ]*1024[ ]*).*[ ]*}}        [[LOAD2D:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)


;                      the EE and IE instructions are marked as VS. IEs add regpressure. The last IE kills the original vector

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_0:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 0
; CHECK: {{([0-9]+,[ ]*256[ ]*).*VS.*[ ]*}}       [[IE1_0:%.*]] = insertelement <4 x i16> undef, i16 [[EE1_0]], i32 0

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_1:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_1:%.*]] = insertelement <4 x i16> [[IE1_0]], i16 [[EE1_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_2:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 4
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_2:%.*]] = insertelement <4 x i16> [[IE1_1]], i16 [[EE1_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE1_3:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 6
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE1_3:%.*]] = insertelement <4 x i16> [[IE1_2]], i16 [[EE1_3]], i32 3

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_0:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 1
; CHECK: {{([0-9]+,[ ]*256[ ]*).*VS.*[ ]*}}       [[IE2_0:%.*]] = insertelement <4 x i16> undef, i16 [[EE2_0]], i32 0

; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_1:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 3
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_1:%.*]] = insertelement <4 x i16> [[IE2_0]], i16 [[EE2_1]], i32 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_2:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 5
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[IE2_2:%.*]] = insertelement <4 x i16> [[IE2_1]], i16 [[EE2_2]], i32 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*VS.*[ ]*}}         [[EE2_3:%.*]] = extractelement <8 x i16> [[LOAD2D]], i32 7

; CHECK: {{([0-9]+,[ ]*-512[ ]*).*VS.*[ ]*}}      [[IE2_3:%.*]] = insertelement <4 x i16> [[IE2_2]], i16 [[EE2_3]], i32 3

;                       both DPAS increase the regpressure by 256 (sub vector of 256 dies, 512 created)
; CHECK: {{([0-9]+,[ ]*256[ ]*).*[ ]*}}          [[DPAS1:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> [[IE1_3]], <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK: {{([0-9]+,[ ]*256[ ]*).*[ ]*}}          [[DPAS2:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> [[IE2_3]], <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK:         ret void
;
entry:
  %base_addr = ptrtoint ptr addrspace(1) %A to i64
  %load2d = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %EE1.0 = extractelement <8 x i16> %load2d, i32 0
  %IE1.0 = insertelement <4 x i16> undef, i16 %EE1.0, i32 0
  %EE1.1 = extractelement <8 x i16> %load2d, i32 2
  %IE1.1 = insertelement <4 x i16> %IE1.0, i16 %EE1.1, i32 1
  %EE1.2 = extractelement <8 x i16> %load2d, i32 4
  %IE1.2 = insertelement <4 x i16> %IE1.1, i16 %EE1.2, i32 2
  %EE1.3 = extractelement <8 x i16> %load2d, i32 6
  %IE1.3 = insertelement <4 x i16> %IE1.2, i16 %EE1.3, i32 3
  %EE2.0 = extractelement <8 x i16> %load2d, i32 1
  %IE2.0 = insertelement <4 x i16> undef, i16 %EE2.0, i32 0
  %EE2.1 = extractelement <8 x i16> %load2d, i32 3
  %IE2.1 = insertelement <4 x i16> %IE2.0, i16 %EE2.1, i32 1
  %EE2.2 = extractelement <8 x i16> %load2d, i32 5
  %IE2.2 = insertelement <4 x i16> %IE2.1, i16 %EE2.2, i32 2
  %EE2.3 = extractelement <8 x i16> %load2d, i32 7
  %IE2.3 = insertelement <4 x i16> %IE2.2, i16 %EE2.3, i32 3
  %dpas1 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE1.3, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> %IE2.3, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}


define spir_kernel void @coalesced_scalars(ptr addrspace(1) %0) {
; CHECK: Function coalesced_scalars
; CHECK: Greedy MW attempt

;               the IE instructions are marked as SCA. First IE adds regpressure
;               then the last usage of the scalar (fadd) kills the hanging values

; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP17:%.*]] = fmul fast float [[TMP9:%.*]], [[TMP1:%.*]]
; CHECK: {{([0-9]+,[ ]*512[ ]*).*SCA.*[ ]*}}            [[TMP19:%.*]] = insertelement <4 x float> zeroinitializer, float [[TMP17]], i64 0
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP18:%.*]] = fmul fast float [[TMP10:%.*]], [[TMP2:%.*]]
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*SCA.*[ ]*}}        [[TMP21:%.*]] = insertelement <4 x float> [[TMP19]], float [[TMP18]], i64 1
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP20:%.*]] = fmul fast float [[TMP11:%.*]], [[TMP3:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP23:%.*]] = insertelement <4 x float> [[TMP21]], float [[TMP20]], i64 2
; CHECK: {{([0-9]+,[ ]*[0-9-]+[ ]*).*[ ]*}}             [[TMP22:%.*]] = fmul fast float [[TMP12:%.*]], [[TMP4:%.*]]
; CHECK: {{([0-9]+,[ ]*0[ ]*).*SCA.*[ ]*}}              [[TMP25:%.*]] = insertelement <4 x float> [[TMP23]], float [[TMP22]], i64 3

;            dpas don't add any regpressure, they reuse the registers of the created vector
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}                   [[TMP33:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> [[TMP25]], <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
; CHECK: {{([0-9]+,[ ]*0[ ]*).*[ ]*}}                   [[TMP34:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> [[TMP33]], <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

;            extract the values from the vector. The extractelement instructions are marked as V2S and don't increase regpressure
;            the vector hangs, so the last EE doesn't reduce regpressure. In this case the EEs are used in PHI nodes so these hanging vals are not killed
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP35:%.*]] = extractelement <4 x float> [[TMP34]], i64 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP36:%.*]] = extractelement <4 x float> [[TMP34]], i64 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP37:%.*]] = extractelement <4 x float> [[TMP34]], i64 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}              [[TMP38:%.*]] = extractelement <4 x float> [[TMP34]], i64 3

;            fadd kills the hanging values from SCA and creates one float -> -384 (3 * 32 * 4)
; CHECK: {{([0-9]+,[ ]*-384[ ]*).*[ ]*}}                [[TMP50:%.*]] = fadd fast float [[TMP37:%.*]], [[TMP22]]
;            this fadd doesn't kill the hanging EEs (they are used in PHI nodes), so it increases regpressure by 128 (32 * 4)
; CHECK: {{([0-9]+,[ ]*128[ ]*).*[ ]*}}                  [[TMP51:%.*]] = fadd fast float [[TMP38:%.*]], 4.000000e+00

  entry:
  br label %._crit_edge

  ._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi float [ 0.000000e+00, %entry ], [ %19, %._crit_edge ]
  %2 = phi float [ 0.000000e+00, %entry ], [ %20, %._crit_edge ]
  %3 = phi float [ 0.000000e+00, %entry ], [ %21, %._crit_edge ]
  %4 = phi float [ 0.000000e+00, %entry ], [ %22, %._crit_edge ]
  %5 = call float @llvm.exp2.f32(float 0.000000e+00)
  %6 = call float @llvm.exp2.f32(float 0.000000e+00)
  %7 = call float @llvm.exp2.f32(float 0.000000e+00)
  %8 = call float @llvm.exp2.f32(float 0.000000e+00)
  %9 = fmul fast float %5, %1
  %10 = fmul fast float %6, %2
  %11 = fmul fast float %7, %3
  %12 = fmul fast float %8, %4
  %13 = insertelement <4 x float> zeroinitializer, float %9, i64 0
  %14 = insertelement <4 x float> %13, float %10, i64 1
  %15 = insertelement <4 x float> %14, float %11, i64 2
  %16 = insertelement <4 x float> %15, float %12, i64 3
  %17 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %16, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %18 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %17, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %19 = extractelement <4 x float> %18, i64 0
  %20 = extractelement <4 x float> %18, i64 1
  %21 = extractelement <4 x float> %18, i64 2
  %22 = extractelement <4 x float> %18, i64 3
  %23 = fadd fast float %21, %12
  %24 = fadd fast float %22, 4.000000e+00

  br label %._crit_edge
}


define spir_kernel void @vector_to_scalars_pattern(ptr addrspace(1) %A) {
; CHECK: Function vector_to_scalars_pattern
; CHECK: Greedy MW attempt

;           DPAS increases regpressure
; CHECK: {{([0-9]+,[ ]*512[ ]*).*[ ]*}}           [[DPAS:%.*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> undef, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)

;           EE don't increase regpressure
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE0:%.*]] = extractelement <4 x float> [[DPAS]], i64 0
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE1:%.*]] = extractelement <4 x float> [[DPAS]], i64 1
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE2:%.*]] = extractelement <4 x float> [[DPAS]], i64 2
; CHECK: {{([0-9]+,[ ]*0[ ]*).*V2S.*[ ]*}}        [[EE3:%.*]] = extractelement <4 x float> [[DPAS]], i64 3

;           The vector doesn't die on the last EE, it hangs.
;           The uses of the EEs increase regpressure
; CHECK: {{([0-9]+,[ ]*128[ ]*).*[ ]*}}            [[USE0:%.*]] = fadd fast float [[EE0]], 1.0
; CHECK: {{([0-9]+,[ ]*128[ ]*).*[ ]*}}            [[USE1:%.*]] = fadd fast float [[EE1]], 2.0
; CHECK: {{([0-9]+,[ ]*128[ ]*).*[ ]*}}            [[USE2:%.*]] = fadd fast float [[EE2]], 3.0
;           The vector dies on the last EE usage
; CHECK: {{([0-9]+,[ ]*-384[ ]*).*[ ]*}}          [[USE3:%.*]] = fadd fast float [[EE3]], 4.0

; CHECK:         ret void

entry:
  %dpas = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> zeroinitializer, <4 x i16> undef, <4 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %ee0 = extractelement <4 x float> %dpas, i64 0
  %ee1 = extractelement <4 x float> %dpas, i64 1
  %ee2 = extractelement <4 x float> %dpas, i64 2
  %ee3 = extractelement <4 x float> %dpas, i64 3
  %use0 = fadd fast float %ee0, 1.0
  %use1 = fadd fast float %ee1, 2.0
  %use2 = fadd fast float %ee2, 3.0
  %use3 = fadd fast float %ee3, 4.0
  ret void
}



declare <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(
  <4 x float>, <4 x i16>, <4 x i32>, i32, i32, i32, i32, i1) #1

declare float @llvm.exp2.f32(float) #2

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1
attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
