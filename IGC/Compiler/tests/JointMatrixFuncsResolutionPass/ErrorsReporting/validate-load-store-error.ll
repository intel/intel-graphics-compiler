;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -igc-joint-matrix-resolution -dce -S --platformdg2 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------


; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @load_store_legacy_error(ptr %a, ptr %dst) {
; CHECK-LABEL: define spir_kernel void @load_store_legacy_error(
; CHECK: [[TMP4:%.*]] = alloca <16 x i32>
; CHECK: [[PTR:%.*]] = alloca <16 x i32>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x16_i32_16_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %a, i64 16, i32 0), !dbg [[DBG2:![0-9]*]]
; CHECK: [[MATRIX:%.*]] = load <16 x i32>, ptr [[PTR]]
; CHECK: store <16 x i32> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i32_16_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0), !dbg [[DBG3:![0-9]*]]
; CHECK: ret void
; CHECK: error: in kernel 'load_store_legacy_error': Unsupported JointMatrix operation: load matrix A <8 x 16 x i32> with row major layout
; CHECK: error: in kernel 'load_store_legacy_error': Unsupported JointMatrix operation: store matrix A <8 x 16 x i32> with row major layout
;
  %1 = call spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 0) @_Z28__spirv_JointMatrixLoadINTELPU3AS4iliii(ptr %a, i64 16, i32 0, i32 3, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS4iliii(ptr %dst, target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 0) %1, i64 8, i32 0, i32 3, i32 0)
  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 0) @_Z28__spirv_JointMatrixLoadINTELPU3AS4iliii(ptr, i64, i32, i32, i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS4iliii(ptr, target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 0), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{ptr @load_store_legacy_error, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
