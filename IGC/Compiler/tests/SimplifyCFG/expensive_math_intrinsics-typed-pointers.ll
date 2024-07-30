;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt %s -S -o - -simplifycfg -gen-tti | FileCheck -check-prefix=CHECK %s

; This test checks that SimplifyCFG do not merge nodes with expensive math intrinsics
; such as sqrt, sin, cos

define internal spir_func i8 @intersectWorldBoolean(i1 %cmp.i, float %sub7.i) {
; CHECK-LABEL: @intersectWorldBoolean(
; CHECK:       if.end.i:
; CHECK-NEXT:    [[CALL_I_I_I2:%.*]] = call float @llvm.sqrt.f32
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  br label %for.body

for.body:                                         ; preds = %for.cond
  br i1 undef, label %if.then9.i.i, label %if.else11.i.i

if.then9.i.i:                                     ; preds = %for.body
  ret i8 0

if.else11.i.i:                                    ; preds = %for.body
  br label %_Z15__spirv_ocl_cosf.exit

_Z15__spirv_ocl_cosf.exit:                        ; preds = %if.else11.i.i
  br i1 %cmp.i, label %if.then.i, label %if.end.i

if.then.i:                                        ; preds = %_Z15__spirv_ocl_cosf.exit
  br label %intersectRaySphere.exit

if.end.i:                                         ; preds = %_Z15__spirv_ocl_cosf.exit
  %call.i.i.i2 = call float @llvm.sqrt.f32(float %sub7.i)
  %i57 = fadd fast float 0.000000e+00, %call.i.i.i2
  %div.i = fmul fast float %i57, 0.000000e+00
  br label %intersectRaySphere.exit

intersectRaySphere.exit:                          ; preds = %if.end.i, %if.then.i
  %tRay.1 = phi float [ 0.000000e+00, %if.then.i ], [ %div.i, %if.end.i ]
  br i1 undef, label %land.lhs.true, label %if.end

land.lhs.true:                                    ; preds = %intersectRaySphere.exit
  %cmp10 = fcmp olt float 0.000000e+00, 0.000000e+00
  br i1 %cmp10, label %if.then, label %if.end

if.then:                                          ; preds = %land.lhs.true
  br i1 undef, label %if.end.i3, label %_Z21__spirv_ocl_normalizeDv4_f.exit

if.end.i3:                                        ; preds = %if.then
  ret i8 0

_Z21__spirv_ocl_normalizeDv4_f.exit:              ; preds = %if.then
  %i98 = insertelement <4 x float> zeroinitializer, float 0.000000e+00, i32 0
  br label %if.end

if.end:                                           ; preds = %_Z21__spirv_ocl_normalizeDv4_f.exit, %land.lhs.true, %intersectRaySphere.exit
  br label %for.inc

for.inc:                                          ; preds = %if.end
  br label %for.cond
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.sqrt.f32(float) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
