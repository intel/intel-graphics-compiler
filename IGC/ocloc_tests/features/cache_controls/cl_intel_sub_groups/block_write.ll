;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroups,+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)*, <2 x i8>)
declare spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)*, <2 x i16>)
declare spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)*, <2 x i32>)
declare spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)*, <2 x i64>)

!opencl.ocl.version = !{!25}

; CHECK: .kernel "test_uncached_uncached"

define spir_kernel void @test_uncached_uncached(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !0
; CHECK: lsc_store.ugm.uc.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !0
; CHECK: lsc_store.ugm.uc.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !0
; CHECK: lsc_store.ugm.uc.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !0
; CHECK: lsc_store.ugm.uc.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!0 = !{!1, !2}
!1 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_uncached_writeback"

define spir_kernel void @test_uncached_writeback(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !3
; CHECK: lsc_store.ugm.uc.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !3
; CHECK: lsc_store.ugm.uc.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !3
; CHECK: lsc_store.ugm.uc.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !3
; CHECK: lsc_store.ugm.uc.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!3 = !{!4, !5}
!4 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!5 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_writethrough_uncached"

define spir_kernel void @test_writethrough_uncached(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !6
; CHECK: lsc_store.ugm.wt.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !6
; CHECK: lsc_store.ugm.wt.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !6
; CHECK: lsc_store.ugm.wt.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !6
; CHECK: lsc_store.ugm.wt.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!6 = !{!7, !8}
!7 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!8 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_writethrough_writeback"

define spir_kernel void @test_writethrough_writeback(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !9
; CHECK: lsc_store.ugm.wt.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !9
; CHECK: lsc_store.ugm.wt.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !9
; CHECK: lsc_store.ugm.wt.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !9
; CHECK: lsc_store.ugm.wt.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!9 = !{!10, !11}
!10 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!11 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_streaming_uncached"

define spir_kernel void @test_streaming_uncached(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !12
; CHECK: lsc_store.ugm.st.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !12
; CHECK: lsc_store.ugm.st.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !12
; CHECK: lsc_store.ugm.st.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !12
; CHECK: lsc_store.ugm.st.uc (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!12 = !{!13, !14}
!13 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!14 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}

; CHECK: .kernel "test_streaming_writeback"

define spir_kernel void @test_streaming_writeback(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !15
; CHECK: lsc_store.ugm.st.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !15
; CHECK: lsc_store.ugm.st.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !15
; CHECK: lsc_store.ugm.st.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !15
; CHECK: lsc_store.ugm.st.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!15 = !{!16, !17}
!16 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!17 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_writeback_writeback"

define spir_kernel void @test_writeback_writeback(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !18
; CHECK: lsc_store.ugm.wb.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !18
; CHECK: lsc_store.ugm.wb.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !18
; CHECK: lsc_store.ugm.wb.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !18
; CHECK: lsc_store.ugm.wb.wb (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!18 = !{!19, !20}
!19 = !{i32 6443, i32 0, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteBack}
!20 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}

; CHECK: .kernel "test_unsupported_cache_controls_config"

define spir_kernel void @test_unsupported_cache_controls_config(i8 addrspace(1)* %uc_p, i16 addrspace(1)* %us_p, i32 addrspace(1)* %ui_p, i64 addrspace(1)* %ul_p, <2 x i8> %uchar_val, <2 x i16> %ushort_val, <2 x i32> %uint_val, <2 x i64> %ulong_val) !intel_reqd_sub_group_size !24 {
entry:
  %decorated_uc_ptr = getelementptr inbounds i8, i8 addrspace(1)* %uc_p, i32 0, !spirv.Decorations !21
; CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x4t
  call spir_func void @_Z31intel_sub_group_block_write_uc2PU3AS1hDv2_h(i8 addrspace(1)* %decorated_uc_ptr, <2 x i8> %uchar_val)

  %decorated_us_ptr = getelementptr inbounds i16, i16 addrspace(1)* %us_p, i32 0, !spirv.Decorations !21
; CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x8t
  call spir_func void @_Z31intel_sub_group_block_write_us2PU3AS1tDv2_t(i16 addrspace(1)* %decorated_us_ptr, <2 x i16> %ushort_val)

  %decorated_ui_ptr = getelementptr inbounds i32, i32 addrspace(1)* %ui_p, i32 0, !spirv.Decorations !21
; CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x16t
  call spir_func void @_Z28intel_sub_group_block_write2PU3AS1jDv2_j(i32 addrspace(1)* %decorated_ui_ptr, <2 x i32> %uint_val)

  %decorated_ul_ptr = getelementptr inbounds i64, i64 addrspace(1)* %ul_p, i32 0, !spirv.Decorations !21
; CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d64x32t
  call spir_func void @_Z31intel_sub_group_block_write_ul2PU3AS1mDv2_m(i64 addrspace(1)* %decorated_ul_ptr, <2 x i64> %ulong_val)

  ret void
}

!21 = !{!22, !23}
!22 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!23 = !{i32 6443, i32 1, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=1, Streaming}

!24 = !{i32 16}
!25 = !{i32 1, i32 2}
