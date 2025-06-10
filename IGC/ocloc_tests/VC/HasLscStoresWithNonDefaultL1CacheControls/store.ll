;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, ptl-supported
; RUN: llvm-as %s -opaque-pointers=0 -o %t.bc
; RUN: ocloc -device ptl -llvm_input -internal_options "-vc-report-lsc-stores-with-non-default-l1-cache-controls" -options "-vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_opt'" -output_no_suffix -file %t.bc -output %t_opt
; RUN: ocloc -device ptl -llvm_input -options "-vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_no_opt'" -output_no_suffix -file %t.bc -output %t_no_opt
; RUN: FileCheck %s --check-prefixes CHECK,OPT-SET --input-file %t_opt/*.zeinfo
; RUN: FileCheck %s --check-prefixes CHECK,NO-OPT-SET --input-file %t_no_opt/*.zeinfo

declare void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i64>, i16, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)

; CHECK-LABEL: - name: test_uncached
; CHECK: execution_env:
; OPT-SET:   has_lsc_stores_with_non_default_l1_cache_controls: true
; NO-OPT-SET-NOT:   has_lsc_stores_with_non_default_l1_cache_controls: true

define dllexport spir_kernel void @test_uncached() #0 {
  tail call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 3>, i64 0, <8 x i64> zeroinitializer, i16 1, i32 0, <8 x i32> zeroinitializer)
  ret void
}

; CHECK-LABEL: - name: test_default
; CHECK: execution_env:
; OPT-SET-NOT:   has_lsc_stores_with_non_default_l1_cache_controls: true
; NO-OPT-SET-NOT:   has_lsc_stores_with_non_default_l1_cache_controls: true

define dllexport spir_kernel void @test_default() #0 {
  tail call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> zeroinitializer, i16 1, i32 0, <8 x i32> zeroinitializer)
  ret void
}

; CHECK-LABEL: - name: test_both
; CHECK: execution_env:
; OPT-SET:   has_lsc_stores_with_non_default_l1_cache_controls: true
; NO-OPT-SET-NOT:   has_lsc_stores_with_non_default_l1_cache_controls: true

define dllexport spir_kernel void @test_both() #0 {
  %ret = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1><i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 12, i8 3, i8 6, <2 x i8> <i8 1, i8 3>, i32 0, <16 x i64> zeroinitializer, i16 1, i32 0, <16 x i32> zeroinitializer, <16 x i32> undef, <16 x i32> zeroinitializer)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1><i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> zeroinitializer, i16 1, i32 0, <16 x i32> %ret)
  ret void
}

attributes #0 = { noinline "VCFunction" }

!spirv.Source = !{!0}

!0 = !{i32 0, i32 100000}
