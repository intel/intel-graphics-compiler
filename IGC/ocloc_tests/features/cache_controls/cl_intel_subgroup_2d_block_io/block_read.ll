;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i16*)

; CHECK: .kernel "test_uncached_uncached"

define spir_kernel void @test_uncached_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !0

; CHECK: lsc_load_block2d.ugm.uc.uc (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!0 = !{!1, !2}
!1 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_uncached_cached"

define spir_kernel void @test_uncached_cached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !3

; CHECK: lsc_load_block2d.ugm.uc.ca (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!3 = !{!4, !5}
!4 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!5 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_cached_uncached"

define spir_kernel void @test_cached_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !6

; CHECK: lsc_load_block2d.ugm.ca.uc (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!6 = !{!7, !8}
!7 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!8 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_cached_cached"

define spir_kernel void @test_cached_cached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !9

; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!9 = !{!10, !11}
!10 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!11 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_streaming_uncached"

define spir_kernel void @test_streaming_uncached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !12

; CHECK: lsc_load_block2d.ugm.st.uc (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!12 = !{!13, !14}
!13 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!14 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_streaming_cached"

define spir_kernel void @test_streaming_cached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !15

; CHECK: lsc_load_block2d.ugm.st.ca (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!15 = !{!16, !17}
!16 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!17 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_iar_cached"

define spir_kernel void @test_iar_cached(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !18

; CHECK: lsc_load_block2d.ugm.ri.ca (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!18 = !{!19, !20}
!19 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!20 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}

; CHECK: .kernel "test_unsupported_cache_controls_config"

define spir_kernel void @test_unsupported_cache_controls_config(i8 addrspace(1)* %input, <2 x i32> %coord, i16 addrspace(1)* %output) !intel_reqd_sub_group_size !24 {
entry:
  %alloca = alloca [2 x i16], align 2
  %dst = getelementptr inbounds [2 x i16], [2 x i16]* %alloca, i64 0, i64 0
  %decorated_ptr = getelementptr inbounds i8, i8 addrspace(1)* %input, i32 0, !spirv.Decorations !21

; CHECK: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.64x1nn  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]
  call spir_func void @_Z40intel_sub_group_2d_block_read_8b_1r32x2cPU3AS1viiiDv2_iPt(i8 addrspace(1)* %decorated_ptr, i32 512, i32 46, i32 512, <2 x i32> %coord, i16* %dst)

  %value = load i16, i16* %dst, align 2
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %out_ptr = getelementptr inbounds i16, i16 addrspace(1)* %output, i64 %gid
  store i16 %value, i16 addrspace(1)* %out_ptr, align 2
  ret void
}

!21 = !{!22, !23}
!22 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!23 = !{i32 6442, i32 1, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=1, InvalidateAfterRead}

!24 = !{i32 16}
