;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported, llvm-16-plus, opaque-pointers

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1, PrintToConsole=1, PrintAfter=Layout'" 2>&1 | FileCheck %s

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
;
; For PVC, cache to L1 is disabled; L1 cache control options are ignored.

target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z12get_local_idj(i32)
declare spir_func void @_Z20__spirv_ocl_prefetchPU3AS1cl(i8 addrspace(1)*, i64) #0
declare spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)*, i64)

define spir_kernel void @test_i8_uncached_cached(i8 addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_i8_uncached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i8|p1}}({{i8|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 5, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i64 %i, !spirv.Decorations !3
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1cl(i8 addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_i8v16_uncached_cached(i8 addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; COM: i8 type can be unaligned, vector data type can't be used, i8v16 is broken into two i64 messages.
; CHECK-LABEL: @test_i8v16_uncached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i64|p1}}({{i64|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 4, i32 1, i32 2)
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i64|p1}}({{i64|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 4, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i64 %i, !spirv.Decorations !3
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1cl(i8 addrspace(1)* %decorated_ptr, i64 16)
  ret void
}

define spir_kernel void @test_float_uncached_uncached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_uncached_uncached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i32|p1}}({{i32|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 {{1|2}})
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !0
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_uncached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_float_uncached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i32|p1}}({{i32|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !3
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_cached_uncached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; COM: Cache to L1 unsupported, ignore L1 cache control options.
; CHECK-LABEL: @test_float_cached_uncached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i32|p1}}({{i32|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 {{1|2}})
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !6
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_float_cached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; COM: Cache to L1 unsupported, ignore L1 cache control options.
; CHECK-LABEL: @test_float_cached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1i32|p1}}({{i32|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 3, i32 1, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !9
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 1)
  ret void
}

define spir_kernel void @test_floatv8_uncached_cached(float addrspace(1)* %input) !intel_reqd_sub_group_size !100 {
entry:
; COM: Float type is aligned, vector data type can be used.
; CHECK-LABEL: @test_floatv8_uncached_cached(
; CHECK:         call void @llvm.genx.GenISA.LSCPrefetch.{{p1v8i32|p1}}({{<8 x i32>|ptr}} addrspace(1){{.*}} %{{[0-9]+}}, i32 0, i32 3, i32 5, i32 2)
  %i = call spir_func i64 @_Z12get_local_idj(i32 0)
  %decorated_ptr = getelementptr inbounds float, float addrspace(1)* %input, i64 %i, !spirv.Decorations !3
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1fl(float addrspace(1)* %decorated_ptr, i64 8)
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

!100 = !{i32 16}
