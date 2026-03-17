;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported
; UNSUPPORTED: sys32

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc \
; COM: Uncomment below line when support for SPV_INTEL_subgroup_buffer_prefetch is implemented in the Khronos SPIRV-LLVM Translator
; COM: --spirv-ext=+SPV_INTEL_subgroup_buffer_prefetch
; RUN: -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)*, i32)

!opencl.ocl.version = !{!1}

; CHECK .kernel "test_all_variants"

define spir_kernel void @test_all_variants(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in) !intel_reqd_sub_group_size !0 {
entry:

; ------------------char variants--------------------

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x4t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %uc_in, i32 1)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %uc_in, i32 2)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %uc_in, i32 4)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %uc_in, i32 8)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %uc_in, i32 16)

; ------------------short variants--------------------

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %us_in, i32 2)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %us_in, i32 4)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %us_in, i32 8)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %us_in, i32 16)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}]:a64
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}+0x100]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %us_in, i32 32)

; ------------------int variants--------------------

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %ui_in, i32 4)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %ui_in, i32 8)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %ui_in, i32 16)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}]:a64
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x64t  flat[{{.*}}+0x100]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %ui_in, i32 32)

; ------------------long variants--------------------

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %ul_in, i32 8)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %ul_in, i32 16)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x64t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %ul_in, i32 32)

; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x64t  flat[{{.*}}]:a64
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x64t  flat[{{.*}}+0x200]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %ul_in, i32 64)

  ret void
}

!0 = !{i32 16}
!1 = !{i32 2, i32 0}
