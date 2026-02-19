;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
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

; Check that during SPIRMetaData's propagation into IGCMetaData, 'null'
; kernels are ignored by the translation/serialization routines.

; TODO: Such null entries are typically encountered in partial recompilation
; scenarios as leftovers from the initial compilation. Were the metadata
; cleanup handled by retry manager routines, this test would no longer be
; needed.

declare spir_kernel void @test_spir(i64 addrspace(1)*)

; CHECK: {!"FuncMDMap[0]", void (i64 addrspace(1)*)* @test_spir}
; CHECK-NOT: {!"FuncMDMap[{{0-9}}]",

!opencl.kernels = !{!0, !1, !12}
!opencl.compiler.options = !{!19}

; COM: The following kernel metadata sequence should be omitted by the pass
!0 = distinct !{null, !13, !14, !15, !16, !17, !18}
; COM: End null kernel sequence
!1 = !{void (i64 addrspace(1)*)* @test_spir, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11}
!2 = !{!"kernel_arg_addr_space", i32 1}
!3 = !{!"kernel_arg_access_qual", !"none"}
!4 = !{!"kernel_arg_type", !"long*"}
!5 = !{!"kernel_arg_type_qual", !"volatile"}
!6 = !{!"kernel_arg_base_type", !"long*"}
!7 = !{!"kernel_arg_name", !"dst"}
!8 = !{!"reqd_work_group_size", i32 1, i32 1, i32 16}
!9 = !{!"work_group_size_hint", i32 1, i32 1, i32 4}
!10 = !{!"vec_type_hint", <4 x float> undef, i32 0}
!11 = !{!"intel_reqd_sub_group_size", i32 16}
; COM: The following kernel metadata sequence should be omitted by the pass
!12 = distinct !{null, !13, !14, !15, !16, !17, !18}
!13 = !{!"kernel_arg_addr_space", i32 0}
!14 = !{!"kernel_arg_access_qual", !"none"}
!15 = !{!"kernel_arg_type", !"void*"}
!16 = !{!"kernel_arg_type_qual", !""}
!17 = !{!"kernel_arg_base_type", !"void*"}
!18 = !{!"kernel_arg_name", !"null_arg"}
; COM: End null kernel sequence
!19 = !{!"-cl-std=CL2.0", !"-cl-opt-disable", !"-g", !"-denorms-are-zero", !"-fp32-correctly-rounded-divide-sqrt", !"-mad-enable", !"-no-signed-zeros", !"-unsafe-math-optimizations", !"-finite-math-only", !"-fast-relaxed-math", !"-relaxed-builtins", !"-match-sincospi"}
