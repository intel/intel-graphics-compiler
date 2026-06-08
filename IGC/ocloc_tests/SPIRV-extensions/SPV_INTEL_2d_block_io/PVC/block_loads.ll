;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: pvc-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_2d_block_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s --check-prefix=CHECK-VISAASM
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %base_address, <2 x i32> addrspace(1)* %coord_ptr, i8* %dst_pointer) !intel_reqd_sub_group_size !3 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %coord_ptr
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 2, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v1i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v64i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 4, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v64i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v64i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <128 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v128i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v64i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 1, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 2, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v1i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 8, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 2, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 2, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 4, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 4, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v64i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 4, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <64 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v64i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 2, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x4x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 4, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x2x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v1i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x1x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 16, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x1x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 32, i32 2, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; This should fail
  ;  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.2x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v4i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.1x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <1 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v1i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.2x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v4i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v8i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.1x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <2 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.2x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <4 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v4i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <8 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v8i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  ; CHECK-GENISA:  call <16 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v16i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
  ret void
}

declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

!spirv.MemoryModel = ! { !0 }
!spirv.Source = ! { !1 }
!opencl.spir.version = ! { !2 }
!opencl.ocl.version = ! { !2 }

!0 = ! { i32 2, i32 2 }
!1 = ! { i32 3, i32 102000 }
!2 = ! { i32 1, i32 2 }
!3 = ! { i32 16 }
