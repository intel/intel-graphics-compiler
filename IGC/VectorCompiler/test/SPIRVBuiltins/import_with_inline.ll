;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=%SPV_CHECK_PREFIX%

declare spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double>)

define spir_func <16 x double> @spirv_log_vec_dbl(<16 x double> %arg) {
  %res = call spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double> %arg)
  ret <16 x double> %res
}

; CHECK-LEGACY-LABEL: define internal spir_func {{(noundef )?}}<16 x double> @_Z15__spirv_ocl_logDv16_d
; CHECK-LEGACY-SAME: (<16 x double> {{(noundef )?}}[[ARG1:%[^ ]+]]) #[[ATTR:[0-9]+]] {
; CHECK-LEGACY: attributes #[[ATTR]] = { {{.*}}alwaysinline{{.*}} }

; COM: FIXME: Not much to check there. Remove the test once the switch is done.
; CHECK-KHR-NOT: define internal spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d
