;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_2d_block_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s --check-prefix=CHECK-VISAASM
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %base_address, <2 x i32> addrspace(1)* %coord_ptr, i8* %val_ptr) !intel_reqd_sub_group_size !3 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %coord_ptr
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.4x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 4, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.8x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 8, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.64x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 64, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.4x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 4, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.8x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 8, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0, <2 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.64x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 2, i32 1, i1 false, i1 false, i32 0, <2 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 64, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.4x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 4, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.8x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 8, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0, <2 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0, <4 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.64x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 4, i32 1, i1 false, i1 false, i32 0, <4 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 64, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.4x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 4, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.8x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0, <2 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 8, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i8> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.64x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 64, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.2x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 2, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.4x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 4, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.8x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 8, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.32x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.2x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 2, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.4x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 4, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.8x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 8, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.32x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0, <2 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 32, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.2x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 2, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.4x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 4, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.8x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 8, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0, <2 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.32x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0, <4 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 32, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.2x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 2, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.4x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 4, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.8x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0, <2 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 8, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.32x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 32, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.1x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 1, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.2x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 2, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.4x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 4, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.8x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 8, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.1x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 1, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.2x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 2, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.4x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 4, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.8x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 8, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.1x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 1, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.2x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 2, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.4x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 4, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.8x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 8, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0, <2 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.1x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 1, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.2x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 2, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.4x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 4, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.8x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0, <2 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 8, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i32> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.1x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 1, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.2x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 2, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.4x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 4, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.8x1nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 8, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.1x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 1, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.2x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 2, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.4x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 4, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.8x2nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 8, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.1x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 1, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.2x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 2, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.4x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 4, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.8x4nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 8, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.1x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 1, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.2x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 2, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.4x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0, <1 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 4, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ; CHECK-VISAASM: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d64.8x8nn
  ; CHECK-GENISA:  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0, <2 x i64> %{{.*}})
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 8, i32 8, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
  ret void
}

declare spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32, i32, i32, i32, i8*, i8 addrspace(1)*, i32, i32, i32, <2 x i32>)

!spirv.MemoryModel = ! { !0 }
!spirv.Source = ! { !1 }
!opencl.spir.version = ! { !2 }
!opencl.ocl.version = ! { !2 }

!0 = ! { i32 2, i32 2 }
!1 = ! { i32 3, i32 102000 }
!2 = ! { i32 1, i32 2 }
!3 = ! { i32 32 }
