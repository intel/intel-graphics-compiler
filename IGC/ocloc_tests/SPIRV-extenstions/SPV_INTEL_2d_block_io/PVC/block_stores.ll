;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: pvc-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_2d_block_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %base_address, <2 x i32> addrspace(1)* %coord_ptr, i8* %val_ptr) !intel_reqd_sub_group_size !3 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %coord_ptr

; OpSubgroup2DBlockStoreINTEL Element size: 1, Block Width: 16, 32, Block Height: 1, 2, 4, 8, Block Count: 1
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x1nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x2nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x4nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x8nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x2nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x4nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x8nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)

; OpSubgroup2DBlockStoreINTEL Element size: 2, Block Width: 16, Block Height: 1, 2, 4, 8, Block Count: 1
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x1nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x2nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x4nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x8nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 2, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)

; OpSubgroup2DBlockStoreINTEL Element size: 4, Block Width: 16, Block Height: 1, 2, 4, 8, Block Count: 1
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x1nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x2nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 2, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x4nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 4, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x8nn
call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 4, i32 16, i32 8, i32 1, i8* %val_ptr, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0)

  ret void
}

declare spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32, i32, i32, i32, i8*, i8 addrspace(1)*, i32, i32, i32, <2 x i32>)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!2}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i32 1, i32 2}
!3 = !{i32 16}
