;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: pvc-supported, regkeys, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

target triple = "spir64-unknown-unknown"

define spir_kernel void @test64(i64 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, <2 x i32> %vec, <64 x i16>* %output) {
entry:
  %data = call spir_func <64 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v2(i64 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, <2 x i32> %vec)
  ; CHECK-ASM: load_block2d.ugm.d16.a64 (1|M0)  r{{[0-9]*}}:31   [r{{[0-9]*}}:1]
  store <64 x i16> %data, <64 x i16>* %output, align 128
  ret void
}

define spir_kernel void @test128(i64 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, <2 x i32> %vec, <128 x i8>* %output) {
entry:
  %data = call spir_func <128 x i8> @__builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v4(i64 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, <2 x i32> %vec, i32 0)
  ; CHECK-ASM: load_block2d.ugm.d8.a64 (1|M0)  r{{[0-9]*}}:31    [r{{[0-9]*}}:1]
  store <128 x i8> %data, <128 x i8>* %output, align 128
  ret void
}

declare spir_func <64 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v2(i64, i32, i32, i32, <2 x i32>)
declare spir_func <128 x i8> @__builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v4(i64, i32, i32, i32, <2 x i32>, i32)
