;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

declare spir_func float @_Z16__spirv_ocl_ceilf(float) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z16__spirv_ocl_ceilf
; CHECK-SAME: (float {{(noundef )?}}%[[ARG1:[^ ]+]])
; CHECK: %[[CALL1:[^ ]+]] = tail call float @llvm.ceil.f32(float %[[ARG1]])
; CHECK-NEXT: ret float %[[CALL1]]

declare spir_func float @_Z17__spirv_ocl_floorf(float) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z17__spirv_ocl_floorf
; CHECK-SAME: (float {{(noundef )?}}%[[ARG2:[^ ]+]])
; CHECK: %[[CALL2:[^ ]+]] = tail call float @llvm.floor.f32(float %[[ARG2]])
; CHECK-NEXT: ret float %[[CALL2]]

declare spir_func float @_Z17__spirv_ocl_truncf(float) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z17__spirv_ocl_truncf
; CHECK-SAME: (float {{(noundef )?}}%[[ARG3:[^ ]+]])
; CHECK: %[[CALL3:[^ ]+]] = tail call float @llvm.trunc.f32(float %[[ARG3]])
; CHECK-NEXT: ret float %[[CALL3]]

declare spir_func float @_Z19__spirv_ocl_roundnef(float) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z19__spirv_ocl_roundnef
; CHECK-SAME: (float {{(noundef )?}}%[[ARG4:[^ ]+]])
; CHECK: %[[CALL4:[^ ]+]] = tail call float @llvm.genx.rnde.f32(float %[[ARG4]])
; CHECK-NEXT: ret float %[[CALL4]]

declare spir_func <2 x float> @_Z16__spirv_ocl_ceilDv2_f(<2 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x float> @_Z16__spirv_ocl_ceilDv2_f
; CHECK-SAME: (<2 x float> {{(noundef )?}}%[[ARG5:[^ ]+]])
; CHECK: %[[CALL5:[^ ]+]] = tail call <2 x float> @llvm.ceil.v2f32(<2 x float> %[[ARG5]])
; CHECK-NEXT: ret <2 x float> %[[CALL5]]

declare spir_func <16 x float> @_Z16__spirv_ocl_ceilDv16_f(<16 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z16__spirv_ocl_ceilDv16_f
; CHECK-SAME: (<16 x float> {{(noundef )?}}%[[ARG6:[^ ]+]])
; CHECK: %[[CALL6:[^ ]+]] = tail call <16 x float> @llvm.ceil.v16f32(<16 x float> %[[ARG6]])
; CHECK-NEXT: ret <16 x float> %[[CALL6]]

declare spir_func <2 x float> @_Z17__spirv_ocl_floorDv2_f(<2 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x float> @_Z17__spirv_ocl_floorDv2_f
; CHECK-SAME: (<2 x float> {{(noundef )?}}%[[ARG7:[^ ]+]])
; CHECK: %[[CALL7:[^ ]+]] = tail call <2 x float> @llvm.floor.v2f32(<2 x float> %[[ARG7]])
; CHECK-NEXT: ret <2 x float> %[[CALL7]]

declare spir_func <16 x float> @_Z17__spirv_ocl_floorDv16_f(<16 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z17__spirv_ocl_floorDv16_f
; CHECK-SAME: (<16 x float> {{(noundef )?}}%[[ARG8:[^ ]+]])
; CHECK: %[[CALL8:[^ ]+]] = tail call <16 x float> @llvm.floor.v16f32(<16 x float> %[[ARG8]])
; CHECK-NEXT: ret <16 x float> %[[CALL8]]

declare spir_func <2 x float> @_Z17__spirv_ocl_truncDv2_f(<2 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x float> @_Z17__spirv_ocl_truncDv2_f
; CHECK-SAME: (<2 x float> {{(noundef )?}}%[[ARG9:[^ ]+]])
; CHECK: %[[CALL9:[^ ]+]] = tail call <2 x float> @llvm.trunc.v2f32(<2 x float> %[[ARG9]])
; CHECK-NEXT: ret <2 x float> %[[CALL9]]

declare spir_func <16 x float> @_Z17__spirv_ocl_truncDv16_f(<16 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z17__spirv_ocl_truncDv16_f
; CHECK-SAME: (<16 x float> {{(noundef )?}}%[[ARG10:[^ ]+]])
; CHECK: %[[CALL10:[^ ]+]] = tail call <16 x float> @llvm.trunc.v16f32(<16 x float> %[[ARG10]])
; CHECK-NEXT: ret <16 x float> %[[CALL10]]

declare spir_func <2 x float> @_Z19__spirv_ocl_roundneDv2_f(<2 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<2 x float> @_Z19__spirv_ocl_roundneDv2_f
; CHECK-SAME: (<2 x float> {{(noundef )?}}%[[ARG11:[^ ]+]])
; CHECK: %[[CALL11:[^ ]+]] = tail call <2 x float> @llvm.genx.rnde.v2f32(<2 x float> %[[ARG11]])
; CHECK-NEXT: ret <2 x float> %[[CALL11]]

declare spir_func <16 x float> @_Z19__spirv_ocl_roundneDv16_f(<16 x float>) #0
; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z19__spirv_ocl_roundneDv16_f
; CHECK-SAME: (<16 x float> {{(noundef )?}}%[[ARG12:[^ ]+]])
; CHECK: %[[CALL12:[^ ]+]] = tail call <16 x float> @llvm.genx.rnde.v16f32(<16 x float> %[[ARG12]])
; CHECK-NEXT: ret <16 x float> %[[CALL12]]

attributes #0 = { nounwind }
