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
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-intel-enable-auto-large-GRF-mode -igc_opts 'DumpVISAASMToConsole=1,DisableCodeScheduling=1,EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-VISAASM

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-intel-enable-auto-large-GRF-mode -igc_opts 'DumpVISAASMToConsole=1,DisableCodeScheduling=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-VISAASM

target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %src_a, i8 addrspace(1)* %src_b, float addrspace(1)* %src_c) !intel_reqd_sub_group_size !100 {
entry:
  %ma1x32 = alloca <2 x i16>
  %ma32x32 = alloca <64 x i16>
  %ma8x16 = alloca <8 x i16>

  %mb = alloca <64 x i32>
  %mb16x16 = alloca <8 x i32>

  %mc1x64 = alloca <4 x float>
  %mc32x64 = alloca { <64 x float>, <64 x float> }

  %md1x64 = alloca <4 x float>
  %md32x64 = alloca { <64 x float>, <64 x float> }

  %a = bitcast <64 x i16>* %ma32x32 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[V{{[0-9]+}},0x3F,0x1F,0x3F,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[V{{[0-9]+}},0x3F,0x1F,0x3F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_v8i8_pi32_i32(i8* %a, i8 addrspace(1)* %src_a, i64 32, i32 0)

  %b = bitcast <64 x i32>* %mb to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[V{{[0-9]+}},0xFF,0xF,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[V{{[0-9]+}},0xFF,0xF,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[V{{[0-9]+}},0xFF,0xF,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[V{{[0-9]+}},0xFF,0xF,0xFF,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8* %b, i8 addrspace(1)* %src_b, i64 128, i32 0)

; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[V{{[0-9]+}},0x7F,0x1F,0x7F,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[V{{[0-9]+}},0x7F,0x1F,0x7F,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[V{{[0-9]+}},0x7F,0x1F,0x7F,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[V{{[0-9]+}},0x7F,0x1F,0x7F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8* %b, i8 addrspace(1)* %src_b, i64 64, i32 0)

  %c = bitcast { <64 x float>, <64 x float> }* %mc32x64 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[V{{[0-9]+}},0xFF,0x1F,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[V{{[0-9]+}},0xFF,0x1F,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[V{{[0-9]+}},0xFF,0x1F,0xFF,V{{[0-9]+}},0x0]
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[V{{[0-9]+}},0xFF,0x1F,0xFF,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8* %c, float addrspace(1)* %src_c, i64 64, i32 0)

  %d = bitcast { <64 x float>, <64 x float> }* %md32x64 to i8*
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}.0 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}.0 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}.0 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}.0 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(4,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1024 V{{[0-9]+}}.0 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1024 V{{[0-9]+}}.0 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1024 V{{[0-9]+}}.0 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1024 V{{[0-9]+}}.0 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(8,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1536 V{{[0-9]+}}.0 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1536 V{{[0-9]+}}.0 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1536 V{{[0-9]+}}.0 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.1536 V{{[0-9]+}}.0 V{{[0-9]+}}(12,0)
; CHECK-VISAASM: dpas.bf.bf.8.8 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(12,0)
  call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x32_bf16_bf16_fp32_fp32(i8* %a, i8* %b, i8* %c, i8* %d)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  {{[A-z0-9_]*}}:d32.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)* %src_c, i8* %d, i64 64, i32 0)

  %a1 = bitcast <2 x i16>* %ma1x32 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[V{{[0-9]+}},0x3F,0x0,0x3F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_v8i8_pi32_i32(i8* %a1, i8 addrspace(1)* %src_a, i64 32, i32 0)

  %c1 = bitcast <4 x float>* %mc1x64 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x4nn  flat[V{{[0-9]+}},0x3F,0x3,0x3F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_v8i8_pi32_i32(i8* %c1, float addrspace(1)* %src_c, i64 64, i32 0)

  %d1 = bitcast <4 x float>* %md1x64 to i8*
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.64 V{{[0-9]+}}.0 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.128 V{{[0-9]+}}.0 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 V{{[0-9]+}}.192 V{{[0-9]+}}.0 V{{[0-9]+}}(0,
; CHECK-VISAASM: dpas.bf.bf.8.1 (M1, 16) {{[A-z0-9_]*}}.0 {{[A-z0-9_]*}}.0 V{{[0-9]+}}.512 V{{[0-9]+}}(0,
  call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x32_bf16_bf16_fp32_fp32(i8* %a1, i8* %b, i8* %c1, i8* %d1)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x3,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d32.16x4nn
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)* %src_c, i8* %d1, i64 64, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x0,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d16.32x1nn
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_pi64_v8i8(i8 addrspace(1)* %src_a, i8* %a1, i64 32, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.256:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.768:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.256:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d16.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0x3F,0x7,0x3F,V{{[0-9]+}},0x0]  V{{[0-9]+}}.768:d16.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_pi64_v8i8(i8 addrspace(1)* %src_a, i8* %a, i64 32, i32 0)

; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}:d32.16x8nn
; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[V{{[0-9]+}},0xFF,0x7,0xFF,V{{[0-9]+}},0x0]  V{{[0-9]+}}.512:d32.16x8nn
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_pi64_v8i8(i8 addrspace(1)* %src_b, i8* %b, i64 128, i32 0)

; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
; CHECK-VISAASM: lsc_store.ugm (M1_NM, 1)  flat[{{[_0-9A-Za-z]+}}]:a64  V{{[0-9]+}}:d64x16t
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_pi64_v8i8(i8 addrspace(1)* %src_b, i8* %b, i64 64, i32 0)

  %a2 = bitcast <8 x i16>* %ma8x16 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x16tn  flat[V{{[0-9]+}},0x1F,0xF,0x1F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_8x16_i16_8_global_v8i8_pi32_i32(i8* %a2, i8 addrspace(1)* %src_a, i64 16, i32 0)

  %b1 = bitcast <8 x i32>* %mb16x16 to i8*
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16tn  flat[V{{[0-9]+}},0x3F,0xF,0x3F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_16x16_i16_8_global_v8i8_pi32_i32(i8* %b1, i8 addrspace(1)* %src_b, i64 32, i32 0)

; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x32tn  flat[V{{[0-9]+}},0xF,0x1F,0xF,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_8x32_i8_8_global_v8i8_pi32_i32(i8* %a2, i8 addrspace(1)* %src_a, i64 16, i32 0)

; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16tn  flat[V{{[0-9]+}},0x1F,0xF,0x1F,V{{[0-9]+}},0x0]
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_32x16_i8_8_global_v8i8_pi32_i32(i8* %b1, i8 addrspace(1)* %src_b, i64 32, i32 0)

  ret void
}

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_8x16_i16_8_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_8x32_i8_8_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_16x16_i16_8_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_32x16_i8_8_global_v8i8_pi32_i32(i8*, i8 addrspace(1)*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_v8i8_pi32_i32(i8*, float addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8*, float addrspace(1)*, i64, i32)

declare spir_func void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x32_bf16_bf16_fp32_fp32(i8*, i8*, i8*, i8*)
declare spir_func void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x32_bf16_bf16_fp32_fp32(i8*, i8*, i8*, i8*)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_pi64_v8i8(i8 addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_pi64_v8i8(i8 addrspace(1)*, i8*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_pi64_v8i8(i8 addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_pi64_v8i8(i8 addrspace(1)*, i8*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)*, i8*, i64, i32)

!100 = !{i32 16}
