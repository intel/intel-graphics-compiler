;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-spir-metadata-translation -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; SPIRMetaDataTranslation
; ------------------------------------------------

; Verifies the OpenCL vec_type_hint reduction: SPIRMetaDataTranslation turns the
; SPIR-V !vec_type_hint LLVM type into its ZEBinary string form and stores it in
; ModuleMetaData::FuncMD[F].vecTypeHint (serialized as {!"vecTypeHint", !"<str>"}).
; Each kernel exercises a distinct branch of the type->string reduction:
;   <4 x i8>  -> "uchar4"  (integer 'u' prefix, 8-bit, vector suffix)
;   <2 x i32> -> "uint2"   (integer 'u' prefix, 32-bit, vector suffix)
;   double    -> "double"  (non-integer scalar, no prefix, no suffix)
;   <3 x half>-> "half3"   (non-integer, 16-bit float, vector suffix)

declare spir_kernel void @k_uchar4()
declare spir_kernel void @k_uint2()
declare spir_kernel void @k_double()
declare spir_kernel void @k_half3()

; CHECK-DAG: {!"vecTypeHint", !"uchar4"}
; CHECK-DAG: {!"vecTypeHint", !"uint2"}
; CHECK-DAG: {!"vecTypeHint", !"double"}
; CHECK-DAG: {!"vecTypeHint", !"half3"}

!opencl.kernels = !{!0, !2, !4, !6}

!0 = !{void ()* @k_uchar4, !1}
!1 = !{!"vec_type_hint", <4 x i8> undef, i32 0}
!2 = !{void ()* @k_uint2, !3}
!3 = !{!"vec_type_hint", <2 x i32> undef, i32 0}
!4 = !{void ()* @k_double, !5}
!5 = !{!"vec_type_hint", double undef, i32 0}
!6 = !{void ()* @k_half3, !7}
!7 = !{!"vec_type_hint", <3 x half> undef, i32 0}
