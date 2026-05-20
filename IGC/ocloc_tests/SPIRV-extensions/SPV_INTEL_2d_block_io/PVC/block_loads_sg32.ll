;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_2d_block_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'"             2>&1 | FileCheck %s --check-prefix=CHECK-VISAASM

; Test for 2D block loads for subgroup-size=32. Compared to subgroup-size=16 (test block_loads.ll),
; the GenISA calls use different data type per work-item, but VISA ASM is exactly the same.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i8 addrspace(1)* %base_address, <2 x i32> addrspace(1)* %coord_ptr, i8* %dst_pointer) !intel_reqd_sub_group_size !3 {
entry:
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %coord_ptr

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 8, Block Height: 8, 16, 32 Block Count: 1, 2, 4
; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 8, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32  8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 16, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 16,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 8, i32 32, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 8, i32 32,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)


; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2, 4
; CHECK-GENISA:  call i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 1, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 2, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 1, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 1,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 2, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 2,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 4, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 4,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 8, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v32i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 16, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 16,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <64 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v64i8(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 32, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 2, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 2, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 4, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 4, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 8, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 16, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 32, i32 32, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 1, Block Width: 64, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 1, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 2, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 4, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 64, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 64, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 8, Block Height: 2, 4, 8, 16, 32, Block Count: 1, 2, 4
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 2, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 2, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 2,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 4, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 4,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 8, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 8,  i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 16, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 16, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 8, i32 32, i32 4, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.4x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 8, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 2, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 4, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL, Element size: 2, Block Width: 32, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 32, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL Element size: 4, Block Width: 8, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1, 2
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 1, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 1,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 2, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 2,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 4, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 4,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8,  i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 16, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 32, i32 2, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadINTEL Element size: 4, Block Width: 16, Block Height: 1, 2, 4, 8, 16, 32, Block Count: 1
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 1,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 2,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 4,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 8,  i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransformINTEL Element size: 1, Block Width: 16, Block Height: 32, Block Count: 1, 2, 4
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 1, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 8, i32 16, i32 32, i32 4, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 16, i32 32, i32 4, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransformINTEL Element size: 2, Block Width: 16, Block Height: 16, 32, Block Count: 1, 2
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 1, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 1, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 16, i32 2, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 16, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 16, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransformINTELiiiiPU3AS1KviiiDv2_iPv(i32 2, i32 16, i32 32, i32 2, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 2, 4, 8, Block Height: 8, Block Count: 1
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 1, 2, 4, 8, Block Height: 16, Block Count: 1
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 1, i32 16, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.1x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 1, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 2, i32 16, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 2, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 16, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 16, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 4, Block Width: 4, 8, Block Height: 32, Block Count: 1
; CHECK-GENISA:  call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 4, i32 32, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.4x32tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 4, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 32, i32 8, i32 32, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 4, i32 8, i32 32, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)

; OpSubgroup2DBlockLoadTransposeINTEL Element size: 8, Block Width: 1, 2, 4, Block Height: 8, Block Count: 1
; CHECK-GENISA:  call i64 @llvm.genx.GenISA.LSC2DBlockRead.i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 1, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.1x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 1, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call i64 @llvm.genx.GenISA.LSC2DBlockRead.i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 2, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.2x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
  call spir_func void @_Z41__spirv_Subgroup2DBlockLoadTransposeINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 2, i32 8, i32 1, i8 addrspace(1)* %base_address, i32 512, i32 46, i32 512, <2 x i32> %0, i8* %dst_pointer)
; CHECK-GENISA:  call <2 x i64> @llvm.genx.GenISA.LSC2DBlockRead.v2i64(i64 %{{[0-9]+}}, i32 511, i32 45, i32 511, i32 %{{[0-9]+}}, i32 %{{[0-9]+}}, i32 64, i32 4, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK-VISAASM: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d64.4x8tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}
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
!3 = !{i32 32}
