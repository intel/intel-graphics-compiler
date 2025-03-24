;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_2d_block_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32, i32, i32, i32, i8*, i8 addrspace(1)*, i32, i32, i32, <2 x i32>)

; CHECK: .kernel "test_uncached_uncached"

define spir_kernel void @test_uncached_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !0
; CHECK: lsc_store_block2d.ugm.uc.uc (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)

  ret void
}

!0 = !{!1, !2}
!1 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_uncached_writeback"

define spir_kernel void @test_uncached_writeback(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !3
; CHECK: lsc_store_block2d.ugm.uc.wb (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!3 = !{!4, !5}
!4 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!5 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_writethrough_uncached"

define spir_kernel void @test_writethrough_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !6
; CHECK: lsc_store_block2d.ugm.wt.uc (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!6 = !{!7, !8}
!7 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!8 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_writethrough_writeback"

define spir_kernel void @test_writethrough_writeback(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !9
; CHECK: lsc_store_block2d.ugm.wt.wb (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!9 = !{!10, !11}
!10 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!11 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_streaming_uncached"

define spir_kernel void @test_streaming_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !12
; CHECK: lsc_store_block2d.ugm.st.uc (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!12 = !{!13, !14}
!13 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!14 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_streaming_writeback"

define spir_kernel void @test_streaming_writeback(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !15
; CHECK: lsc_store_block2d.ugm.st.wb (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!15 = !{!16, !17}
!16 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!17 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_writeback_writeback"

define spir_kernel void @test_writeback_writeback(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !18
; CHECK: lsc_store_block2d.ugm.wb.wb (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!18 = !{!19, !20}
!19 = !{i32 6443, i32 0, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteBack}
!20 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_unsupported_cache_controls_config"

define spir_kernel void @test_unsupported_cache_controls_config(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %val_ptr) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !21
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn
  call spir_func void @_Z33__spirv_Subgroup2DBlockStoreINTELiiiiPKvPU3AS1viiiDv2_i(i32 1, i32 32, i32 1, i32 1, i8* %val_ptr, i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord)
  ret void
}

!21 = !{!22, !23}
!22 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!23 = !{i32 6443, i32 1, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=1, Streaming}

!24 = !{i32 16}
