;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported, opaque-pointers

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'PrintToConsole=1 PrintAfter=Layout EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s

; LSC prefetch args:
;   1. anyptr: memory address
;   2. int: immediate offset (in bytes)
;   3. int: data size (LSC_DATA_SIZE)
;   4. int: vector size (LSC_DATA_ELEMS)
;   5. int: cache controls options (LSC_CACHE_OPTS)
;
; LSC_CACHE_OPTS:
;   1 = L1 uncached, L3 uncached
;   2 = L1 uncached, L3 cached
;   3 = L1 cached, L3 uncached
;   4 = L1 cached, L3 cached
;   ...
;   18 = L1 uncached, L2 uncached, L3 uncached
;   19 = L1 uncached, L2 uncached, L3 cached
;   20 = L1 uncached, L2 cached, L3 uncached
;   21 = L1 uncached, L2 cached, L3 cached
;   22 = L1 cached, L2 uncached, L3 uncached
;   23 = L1 cached, L2 uncached, L3 cached
;   24 = L1 cached, L2 cached, L3 uncached
;   25 = L1 cached, L2 cached, L3 cached

target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z12get_local_idj(i32)
declare spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)*, i64)

define spir_kernel void @test_float_uncached_uncached(float addrspace(1)* %input, i32 %index) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_uncached_uncached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 1)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !0
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_uncached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_uncached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !3
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_cached_uncached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_cached_uncached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 3)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !6
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_cached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_cached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 4)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !9
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_cached_cached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_cached_cached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 25)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !12
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_unsupported_cache_controls_config(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_unsupported_cache_controls_config(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.p1i32(ptr addrspace(1) %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !16
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

!0 = !{!1, !2}
!1 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

!3 = !{!4, !5}
!4 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!5 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

!6 = !{!7, !8}
!7 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!8 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

!9 = !{!10, !11}
!10 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!11 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

!12 = !{!13, !14, !15}
!13 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!14 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
!15 = !{i32 6442, i32 2, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

!16 = !{!17, !18}
!17 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!18 = !{i32 6442, i32 2, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=1, InvalidateAfterRead}

!100 = !{i32 16}
