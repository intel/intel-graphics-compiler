;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -GenXImportOCLBiF -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -GenXImportOCLBiF -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; TODO: %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins,GenXImportOCLBiF -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXTranslateSPIRVBuiltins,GenXImportOCLBiF -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double>)

define spir_func <16 x double> @spirv_log_vec_dbl(<16 x double> %arg) {
  %res = call spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double> %arg)
  ret <16 x double> %res
}

; CHECK: define internal spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double> {{(noundef )?}}[[ARG:%[^ ]+]]) [[ATTR:#[0-9]+]]
; CHECK: attributes [[ATTR]] = { {{.*}}alwaysinline{{.*}} }
