;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; XFAIL: vc-disable-bif

; Signed tests

declare spir_func i8 @_Z17__spirv_ocl_s_absc(i8) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}signext i8 @_Z17__spirv_ocl_s_absc
; CHECK-SAME: (i8 {{(noundef )?}}signext %[[ARG7:[^ ]+]])
; CHECK: %[[CALL7:[^ ]+]] = tail call i8 @llvm.genx.absi.i8(i8 %[[ARG7]])
; CHECK-NEXT: ret i8 %[[CALL7]]

declare spir_func i16 @_Z17__spirv_ocl_s_abss(i16) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}signext i16 @_Z17__spirv_ocl_s_abss
; CHECK-SAME: (i16 {{(noundef )?}}signext %[[ARG8:[^ ]+]])
; CHECK: %[[CALL8:[^ ]+]] = tail call i16 @llvm.genx.absi.i16(i16 %[[ARG8]])
; CHECK-NEXT: ret i16 %[[CALL8]]

declare spir_func i32 @_Z17__spirv_ocl_s_absi(i32) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}i32 @_Z17__spirv_ocl_s_absi
; CHECK-SAME: (i32 {{(noundef )?}}%[[ARG9:[^ ]+]])
; CHECK: %[[CALL9:[^ ]+]] = tail call i32 @llvm.genx.absi.i32(i32 %[[ARG9]])
; CHECK-NEXT: ret i32 %[[CALL9]]

declare spir_func i64 @_Z17__spirv_ocl_s_absl(i64) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}i64 @_Z17__spirv_ocl_s_absl
; CHECK-SAME: (i64 {{(noundef )?}}%[[ARG10:[^ ]+]])
; CHECK: %[[CALL10:[^ ]+]] = tail call i64 @llvm.genx.absi.i64(i64 %[[ARG10]])
; CHECK-NEXT: ret i64 %[[CALL10]]

; Float scalar tests

declare spir_func float @_Z16__spirv_ocl_fabsf(float) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z16__spirv_ocl_fabsf
; CHECK-SAME: (float {{(noundef )?}}%[[ARG1:[^ ]+]])
; CHECK: %[[CALL1:[^ ]+]] = tail call float @llvm.fabs.f32(float %[[ARG1]])
; CHECK-NEXT: ret float %[[CALL1]]

declare spir_func double @_Z16__spirv_ocl_fabsd(double) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}double @_Z16__spirv_ocl_fabsd
; CHECK-SAME: (double {{(noundef )?}}%[[ARG4:[^ ]+]])
; CHECK: %[[CALL4:[^ ]+]] = tail call double @llvm.fabs.f64(double %[[ARG4]])
; CHECK-NEXT: ret double %[[CALL4]]

; Unsigned tests

declare spir_func i8 @_Z17__spirv_ocl_s_absh(i8) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}zeroext i8 @_Z17__spirv_ocl_s_absh
; CHECK-SAME: (i8 {{(noundef )?}}zeroext %[[ARG11:[^ ]+]])
; CHECK: %[[CALL11:[^ ]+]] = tail call i8 @llvm.genx.absi.i8(i8 %[[ARG11]])
; CHECK-NEXT: ret i8 %[[CALL11]]

declare spir_func i16 @_Z17__spirv_ocl_s_abst(i16) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}zeroext i16 @_Z17__spirv_ocl_s_abst
; CHECK-SAME: (i16 {{(noundef )?}}zeroext %[[ARG12:[^ ]+]])
; CHECK: %[[CALL12:[^ ]+]] = tail call i16 @llvm.genx.absi.i16(i16 %[[ARG12]])
; CHECK-NEXT: ret i16 %[[CALL12]]

declare spir_func i32 @_Z17__spirv_ocl_s_absj(i32) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}i32 @_Z17__spirv_ocl_s_absj
; CHECK-SAME: (i32 {{(noundef )?}}%[[ARG13:[^ ]+]])
; CHECK: %[[CALL13:[^ ]+]] = tail call i32 @llvm.genx.absi.i32(i32 %[[ARG13]])
; CHECK-NEXT: ret i32 %[[CALL13]]

declare spir_func i64 @_Z17__spirv_ocl_s_absm(i64) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}i64 @_Z17__spirv_ocl_s_absm
; CHECK-SAME: (i64 {{(noundef )?}}%[[ARG14:[^ ]+]])
; CHECK: %[[CALL14:[^ ]+]] = tail call i64 @llvm.genx.absi.i64(i64 %[[ARG14]])
; CHECK-NEXT: ret i64 %[[CALL14]]

; Signed vector test

declare spir_func <2 x i32> @_Z17__spirv_ocl_s_absDv2_i(<2 x i32>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x i32> @_Z17__spirv_ocl_s_absDv2_i
; CHECK-SAME: (<2 x i32> {{(noundef )?}}%[[ARG15:[^ ]+]])
; CHECK: %[[CALL15:[^ ]+]] = tail call <2 x i32> @llvm.genx.absi.v2i32(<2 x i32> %[[ARG15]])
; CHECK-NEXT: ret <2 x i32> %[[CALL15]]

; Float vector tests

declare spir_func <2 x float> @_Z16__spirv_ocl_fabsDv2_f(<2 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x float> @_Z16__spirv_ocl_fabsDv2_f
; CHECK-SAME: (<2 x float> {{(noundef )?}}%[[ARG2:[^ ]+]])
; CHECK: %[[CALL2:[^ ]+]] = tail call <2 x float> @llvm.fabs.v2f32(<2 x float> %[[ARG2]])
; CHECK-NEXT: ret <2 x float> %[[CALL2]]

declare spir_func <16 x float> @_Z16__spirv_ocl_fabsDv16_f(<16 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z16__spirv_ocl_fabsDv16_f
; CHECK-SAME: (<16 x float> {{(noundef )?}}%[[ARG3:[^ ]+]])
; CHECK: %[[CALL3:[^ ]+]] = tail call <16 x float> @llvm.fabs.v16f32(<16 x float> %[[ARG3]])
; CHECK-NEXT: ret <16 x float> %[[CALL3]]

declare spir_func <2 x double> @_Z16__spirv_ocl_fabsDv2_d(<2 x double>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x double> @_Z16__spirv_ocl_fabsDv2_d
; CHECK-SAME: (<2 x double> {{(noundef )?}}%[[ARG5:[^ ]+]])
; CHECK: %[[CALL5:[^ ]+]] = tail call <2 x double> @llvm.fabs.v2f64(<2 x double> %[[ARG5]])
; CHECK-NEXT: ret <2 x double> %[[CALL5]]

declare spir_func <16 x double> @_Z16__spirv_ocl_fabsDv16_d(<16 x double>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x double> @_Z16__spirv_ocl_fabsDv16_d
; CHECK-SAME: (<16 x double> {{(noundef )?}}%[[ARG6:[^ ]+]])
; CHECK: %[[CALL6:[^ ]+]] = tail call <16 x double> @llvm.fabs.v16f64(<16 x double> %[[ARG6]])
; CHECK-NEXT: ret <16 x double> %[[CALL6]]

; Unsigned vector test

declare spir_func <2 x i32> @_Z17__spirv_ocl_s_absDv2_j(<2 x i32>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x i32> @_Z17__spirv_ocl_s_absDv2_j
; CHECK-SAME: (<2 x i32> {{(noundef )?}}%[[ARG15:[^ ]+]])
; CHECK: %[[CALL15:[^ ]+]] = tail call <2 x i32> @llvm.genx.absi.v2i32(<2 x i32> %[[ARG15]])
; CHECK-NEXT: ret <2 x i32> %[[CALL15]]

attributes #0 = { nounwind }
