;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported, llvm-15-or-older
; UNSUPPORTED: sys32

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; COM: Replace SPV_INTEL_subgroups with SPV_INTEL_subgroup_buffer_prefetch once support for prefetches is implemented in the KHR Translator
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_subgroups,+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; COM: Replace SPV_INTEL_subgroups with SPV_INTEL_subgroup_buffer_prefetch once support for prefetches is implemented in the KHR Translator
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_subgroups,+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)*, i32)
declare spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)*, i32)

!opencl.ocl.version = !{!25}

; CHECK .kernel "test_uncached_uncached"

define spir_kernel void @test_uncached_uncached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !0
; CHECK: lsc_load.ugm.uc.uc (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !0
; CHECK: lsc_load.ugm.uc.uc (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !0
; CHECK: lsc_load.ugm.uc.uc (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !0
; CHECK: lsc_load.ugm.uc.uc (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!0 = !{!1, !2}
!1 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_uncached_cached"

define spir_kernel void @test_uncached_cached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !3
; CHECK: lsc_load.ugm.uc.ca (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !3
; CHECK: lsc_load.ugm.uc.ca (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !3
; CHECK: lsc_load.ugm.uc.ca (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !3
; CHECK: lsc_load.ugm.uc.ca (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!3 = !{!4, !5}
!4 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!5 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_cached_uncached"

define spir_kernel void @test_cached_uncached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !6
; CHECK: lsc_load.ugm.ca.uc (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !6
; CHECK: lsc_load.ugm.ca.uc (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !6
; CHECK: lsc_load.ugm.ca.uc (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !6
; CHECK: lsc_load.ugm.ca.uc (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!6 = !{!7, !8}
!7 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!8 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_cached_cached"

define spir_kernel void @test_cached_cached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !9
; CHECK: lsc_load.ugm.ca.ca (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !9
; CHECK: lsc_load.ugm.ca.ca (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !9
; CHECK: lsc_load.ugm.ca.ca (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !9
; CHECK: lsc_load.ugm.ca.ca (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!9 = !{!10, !11}
!10 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!11 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_streaming_uncached"

define spir_kernel void @test_streaming_uncached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !12
; CHECK: lsc_load.ugm.st.uc (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !12
; CHECK: lsc_load.ugm.st.uc (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !12
; CHECK: lsc_load.ugm.st.uc (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !12
; CHECK: lsc_load.ugm.st.uc (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!12 = !{!13, !14}
!13 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!14 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_streaming_cached"

define spir_kernel void @test_streaming_cached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !15
; CHECK: lsc_load.ugm.st.ca (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !15
; CHECK: lsc_load.ugm.st.ca (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !15
; CHECK: lsc_load.ugm.st.ca (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !15
; CHECK: lsc_load.ugm.st.ca (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!15 = !{!16, !17}
!16 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!17 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_iar_cached"

define spir_kernel void @test_iar_cached(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !18
; CHECK: lsc_load.ugm.ri.ca (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !18
; CHECK: lsc_load.ugm.ri.ca (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !18
; CHECK: lsc_load.ugm.ri.ca (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !18
; CHECK: lsc_load.ugm.ri.ca (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!18 = !{!19, !20}
!19 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!20 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_unsupported_cache_controls_config"

define spir_kernel void @test_unsupported_cache_controls_config(i8 addrspace(1)* %uc_in ,i16 addrspace(1)* %us_in, i32 addrspace(1)* %ui_in, i64 addrspace(1)* %ul_in, <2 x i8> addrspace(1)* %uc_out, <2 x i16> addrspace(1)* %us_out, <2 x i32> addrspace(1)* %ui_out, <2 x i64> addrspace(1)* %ul_out) !intel_reqd_sub_group_size !24 {
entry:

  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_in, i32 0, !spirv.Decorations !21
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x8t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Khj(i8 addrspace(1)* %decorated_uc_ptr, i32 2)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_in, i32 0, !spirv.Decorations !21
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x16t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Ktj(i16 addrspace(1)* %decorated_us_ptr, i32 4)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_in, i32 0, !spirv.Decorations !21
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d32x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kjj(i32 addrspace(1)* %decorated_ui_ptr, i32 8)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_in, i32 0, !spirv.Decorations !21
; CHECK: lsc_load.ugm (M1_NM, 1)  %null:d64x32t  flat[{{.*}}]:a64
  call spir_func void @_Z34__spirv_SubgroupBlockPrefetchINTELPU3AS1Kmj(i64 addrspace(1)* %decorated_ul_ptr, i32 16)

  ret void
}

!21 = !{!22, !23}
!22 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!23 = !{i32 6442, i32 1, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=1, InvalidateAfterRead}

!24 = !{i32 16}
!25 = !{i32 1, i32 2}
