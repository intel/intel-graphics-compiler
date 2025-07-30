;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: pvc-supported, regkeys, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-intel-enable-auto-large-GRF-mode -igc_opts 'DumpVISAASMToConsole=1, EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-VISAASM

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-intel-enable-auto-large-GRF-mode -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-VISAASM

target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %src_a, i8 addrspace(1)* %src_b, float addrspace(1)* %src_c) !intel_reqd_sub_group_size !100 {
entry:
  %ma1x32 = alloca <2 x i16>
  %ma32x32 = alloca <64 x i16>
  %mb = alloca <64 x i32>
  %mc1x64 = alloca <4 x float>
  %mc32x64 = alloca { <64 x float>, <64 x float> }

  %a1 = bitcast <2 x i16>* %ma1x32 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedA_RowMajor_SG16_1x32_i16_2_v8i8_pi32_i32(i8* %a1, i8 addrspace(1)* %src_a, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

  %a = bitcast <64 x i16>* %ma32x32 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedA_RowMajor_SG16_32x32_i16_64_v8i8_pi32_i32(i8* %a, i8 addrspace(1)* %src_a, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

  %b = bitcast <64 x i32>* %mb to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x1,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x11,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x21,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x31,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_PackedB_SG16_32x64_i16_64_v8i8_pi32_i32(i8* %b, i8 addrspace(1)* %src_b, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x22,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x32,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_32x64_i16_64_v8i8_pi32_i32(i8* %b, i8 addrspace(1)* %src_b, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

  %c1 = bitcast <4 x float>* %mc1x64 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_v8i8_pi32_i32(i8* %c1, float addrspace(1)* %src_c, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

  %c = bitcast { <64 x float>, <64 x float> }* %mc32x64 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x1]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x1]
  call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_v8i8_pi32_i32(i8* %c, float addrspace(1)* %src_c, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x1]  V{{[0-9]+}}:d16.32x1nn
  call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedA_RowMajor_SG16_1x32_i16_2_pi64_v8i8(i8 addrspace(1)* %src_a, i8* %a1, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x1]  V{{[0-9]+}}:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x9]  V{{[0-9]+}}.256:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x11]  V{{[0-9]+}}.512:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x2,0x19]  V{{[0-9]+}}.768:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x1]  V{{[0-9]+}}:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x9]  V{{[0-9]+}}.256:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x11]  V{{[0-9]+}}.512:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x12,0x19]  V{{[0-9]+}}.768:d16.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedA_RowMajor_SG16_32x32_i16_64_pi64_v8i8(i8 addrspace(1)* %src_a, i8* %a, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x1,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x1,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x11,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x11,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x21,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x21,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x31,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0x7,0x2,0x9,0x31,0x9]  V{{[0-9]+}}.512:d32.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedB_PackedB_SG16_32x64_i16_64_pi64_v8i8(i8 addrspace(1)* %src_b, i8* %b, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x1]  {{[_A-Za-z0-9]+}}:d32.16x1nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x1]  {{[_A-Za-z0-9]+}}:d32.16x1nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x1]  {{[_A-Za-z0-9]+}}:d32.16x1nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x1]  {{[_A-Za-z0-9]+}}:d32.16x1nn
  call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_pi64_v8i8(float addrspace(1)* %src_c, i8* %c1, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x11]  V{{[0-9]+}}.1024:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x2,0x19]  V{{[0-9]+}}.1536:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x11]  V{{[0-9]+}}.1024:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x12,0x19]  V{{[0-9]+}}.1536:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x11]  V{{[0-9]+}}.1024:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x22,0x19]  V{{[0-9]+}}.1536:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x1]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x9]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x11]  V{{[0-9]+}}.1024:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{[_A-Za-z0-9]+}},0xF,0x2,0x13,0x32,0x19]  V{{[0-9]+}}.1536:d32.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_pi64_v8i8(float addrspace(1)* %src_c, i8* %c, i32 1, i32 2, i32 3, i32 4, i64 5, i32 0)

  ret void
}

declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedA_RowMajor_SG16_1x32_i16_2_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedA_RowMajor_SG16_32x32_i16_64_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_PackedB_SG16_32x64_i16_64_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_32x64_i16_64_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_v8i8_pi32_i32(i8*, float addrspace(1)*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_v8i8_pi32_i32(i8*, float addrspace(1)*, i32, i32, i32, i32, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedA_RowMajor_SG16_1x32_i16_2_pi64_v8i8(i8 addrspace(1)*, i8*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedA_RowMajor_SG16_32x32_i16_64_pi64_v8i8(i8 addrspace(1)*, i8*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_PackedB_PackedB_SG16_32x64_i16_64_pi64_v8i8(i8 addrspace(1)*, i8*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_pi64_v8i8(float addrspace(1)*, i8*, i32, i32, i32, i32, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_pi64_v8i8(float addrspace(1)*, i8*, i32, i32, i32, i32, i64, i32)

!100 = !{i32 16}
