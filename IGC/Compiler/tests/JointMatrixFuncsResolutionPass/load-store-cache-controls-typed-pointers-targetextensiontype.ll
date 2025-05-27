;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --typed-pointers -igc-joint-matrix-resolution -S --platformpvc 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

define spir_kernel void @test_jm(
  float addrspace(1)* %t1_src, float addrspace(1)* %t1_dst,
  float addrspace(1)* %t2_src, float addrspace(1)* %t2_dst,
  float addrspace(1)* %t3_src, float addrspace(1)* %t3_dst,
  float addrspace(1)* %t4_src, float addrspace(1)* %t4_dst) {
  call void @load_store_partial_cache_controls(float addrspace(1)* %t1_src, float addrspace(1)* %t1_dst)
  call void @load_store_without_cache_controls(float addrspace(1)* %t2_src, float addrspace(1)* %t2_dst)
  call void @load_store_l1l3_cache_controls(float addrspace(1)* %t3_src, float addrspace(1)* %t3_dst)
  call void @load_store_invalid_cache_controls(float addrspace(1)* %t4_src, float addrspace(1)* %t4_dst)
  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)*, i64, i32, i32, i32) #0
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)*, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2), i64, i32, i32, i32) #0



;
; This test sets cache controls only for L1 or only for L3. It checks that we fill missing information with defaults to produce valid cacheopt.
;
define void @load_store_partial_cache_controls(float addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define void @load_store_partial_cache_controls(
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_v8i8_pi32_i32(i8* %{{.*}}, float addrspace(1)* %decoratedsrc, i64 64, i32 3)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_pi64_v8i8(float addrspace(1)* %decorateddst, i8* {{.*}}, i64 64, i32 6)
; CHECK: ret void
;
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !100
%1 =  call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !105
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i64 64, i32 0, i32 3, i32 0) #0
ret void
}


;
; This test sets cache controls for both L1 and L3.
;
define void @load_store_l1l3_cache_controls(float addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define void @load_store_l1l3_cache_controls(
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_v8i8_pi32_i32(i8* {{.*}}, float addrspace(1)* %decoratedsrc, i64 64, i32 5)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_pi64_v8i8(float addrspace(1)* %decorateddst, i8* {{.*}}, i64 64, i32 3)
; CHECK: ret void
;
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !110
%1 = call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !115
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i64 64, i32 0, i32 3, i32 0) #0
ret void
}


;
; This test doesn't set cache controls. It checks that we use defaults (0).
;
define void @load_store_without_cache_controls(float addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define void @load_store_without_cache_controls(
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_v8i8_pi32_i32(i8* {{.*}}, float addrspace(1)* %src, i64 64, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_pi64_v8i8(float addrspace(1)* %dst, i8* {{.*}}, i64 64, i32 0)
; CHECK: ret void
;
%1 =  call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %src, i64 64, i32 0, i32 3, i32 0) #0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %dst, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i64 64, i32 0, i32 3, i32 0) #0
ret void
}


;
; This tests sets invalid cache controls configuration. It checks that we fallback to defaults.
;
define void @load_store_invalid_cache_controls(float addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define void @load_store_invalid_cache_controls(
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_v8i8_pi32_i32(i8* {{.*}}, float addrspace(1)* %decoratedsrc, i64 64, i32 4)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_global_pi64_v8i8(float addrspace(1)* %decorateddst, i8* {{.*}}, i64 64, i32 7)
; CHECK: ret void
; CHECK: Unsupported cache controls configuration requested. Applying default configuration.
; CHECK: Unsupported cache controls configuration requested. Applying default configuration.
; CHECK-NOT: Unsupported cache controls configuration requested. Applying default configuration.
; CHECK-NOT: error
;
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !120
%1 =  call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !125
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i64 64, i32 0, i32 3, i32 0) #0
ret void
}



!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}

!IGCMetadata = !{!50}
!50 = !{!"ModuleMD", !51}
!51 = !{!"compOpt", !52, !53}

; Below values represent the default cache controls that in real compilation are passed
; from OCL/L0 Runtime as internal options and may vary depending on the target platform.
!52 = !{!"LoadCacheDefault", i32 4}   ; load  - LSC_L1C_WT_L3C_WB   (4) - L1 cached,     L3 cached
!53 = !{!"StoreCacheDefault", i32 7}  ; store - LSC_L1IAR_WB_L3C_WB (7) - L1 write-back, L3 write-back

; Cache controls for load_store_partial_cache_controls
!100 = !{!101}
!101 = !{i32 6442, i32 1, i32 0} ; {CacheControlLoadINTEL, CacheLevel=1, Uncached} = L3 uncached
; DEFAULT L1 cached + L3 uncached = LSC_L1C_WT_L3UC (3)
!105 = !{!106}
!106 = !{i32 6443, i32 0, i32 3} ; {CacheControlStoreINTEL, CacheLevel=0, Streaming} = L1 streaming
; L1 streaming + DEFAULT L3 write-back = LSC_L1S_L3C_WB (6)

; Cache controls for load_store_l1l3_cache_controls
!110 = !{!111, !112}
!111 = !{i32 6442, i32 0, i32 2} ; {CacheControlLoadINTEL, CacheLevel=0, Streaming} = L1 streaming
!112 = !{i32 6442, i32 1, i32 0} ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}  = L3 uncached
; L1 streaming + L3 uncached = LSC_L1S_L3UC (5)
!115 = !{!116, !117}
!116 = !{i32 6443, i32 0, i32 1} ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough} = L1 write-through
!117 = !{i32 6443, i32 1, i32 0} ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}     = L3 uncached
; L1 write-through + L3 uncached = LSC_L1C_WT_L3UC (3)

; Cache controls for load_store_invalid_cache_controls
!120 = !{!121, !122}
!121 = !{i32 6442, i32 0, i32 3} ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead} = L1 invalidate-after-read
!122 = !{i32 6442, i32 1, i32 0} ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}            = L3 uncached
; L1 invalidate-after-read + L3 uncached = INVALID
!125 = !{!126}
!126 = !{i32 6443, i32 1, i32 3} ; {CacheControlStoreINTEL, CacheLevel=1, Streaming} = L3 streaming
; DEFAULT L1 write-back + L3 streaming = INVALID
