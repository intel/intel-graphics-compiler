;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
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

define spir_kernel void @test(i8 addrspace(1)* %base_address, <2 x i32> addrspace(1)* %coord_ptr, i8* %dst_pointer) !intel_reqd_sub_group_size !3 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %coord_ptr

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 8, Block Height: 8, 16, 32, Block Count: 1, 2, 4
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 16,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 32,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2, 4
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; this case generates [128 x i8], so we store it to [128 x i8]* %1 instead of i8* %dst_pointer to avoid generating bitcast [128 x i8]* to i8*.
; Verification Pass complains about such bitcasts.
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  %1 = alloca [128 x i8]
  %2 = getelementptr inbounds [128 x i8], [128 x i8]* %1, i64 0, i64 0
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %2)

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 32, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 64, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 8, Block Height: 2, 4, 8, 16, 32, Block Count: 1, 2, 4
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 32, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL Element size: 4, Block Width: 8, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL Element size: 4, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransformINTEL Element size: 1, Block Width: 16, Block Height: 32, Block Count: 1, 2, 4
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransformINTEL Element size: 2, Block Width: 16, Block Height: 16, 32, Block Count: 1, 2
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 2, 4, 8, Block Height: 8, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 1, 2, 4, 8, Block Height: 16, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 4, 8, Block Height: 32, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x32tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 8, Block Width: 1, 2, 4, Block Height: 8, Block Count: 1
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.1x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.2x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

  ret void
}

declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)
declare spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)
declare spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!2}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i32 1, i32 2}
!3 = !{i32 16}
