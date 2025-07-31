;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, bmg-supported
; RUN: llvm-as %s -opaque-pointers=0 -o %t.bc
; RUN: ocloc -device bmg -llvm_input -options "-ze-exp-register-file-size=64 -vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_opt_64'" -output_no_suffix -file %t.bc -output %t_opt_64
; RUN: FileCheck %s --check-prefixes CHECK,CHECK-128 --input-file %t_opt_64/*.zeinfo
; RUN: ocloc -device bmg -llvm_input -options "-ze-exp-register-file-size=128 -vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_opt_128'" -output_no_suffix -file %t.bc -output %t_opt_128
; RUN: FileCheck %s --check-prefixes CHECK,CHECK-128 --input-file %t_opt_128/*.zeinfo
; RUN: ocloc -device bmg -llvm_input -options "-ze-exp-register-file-size=160 -vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_opt_160'" -output_no_suffix -file %t.bc -output %t_opt_160
; RUN: FileCheck %s --check-prefixes CHECK,CHECK-256 --input-file %t_opt_160/*.zeinfo
; RUN: ocloc -device bmg -llvm_input -options "-ze-exp-register-file-size=256 -vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_opt_256'" -output_no_suffix -file %t.bc -output %t_opt_256
; RUN: FileCheck %s --check-prefixes CHECK,CHECK-256 --input-file %t_opt_256/*.zeinfo

declare void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)

; CHECK-LABEL: - name: test
; CHECK: execution_env:
; CHECK-128: grf_count:       128
; CHECK-256: grf_count:       256

define dllexport spir_kernel void @test() #0 {
  tail call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> zeroinitializer, i16 1, i32 0, <8 x i32> zeroinitializer)
  ret void
}
attributes #0 = { noinline "VCFunction" }

!spirv.Source = !{!0}

!0 = !{i32 0, i32 100000}
