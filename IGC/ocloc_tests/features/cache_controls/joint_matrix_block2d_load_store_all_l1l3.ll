;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, pvc-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_joint_matrix -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC
; REQUIRES: xe2-hpg-supported
; RUN: ocloc compile -spirv_input -file %t.spv -device xe2-hpg -options " -igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-BMG

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_joint_matrix -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC
; REQUIRES: xe2-hpg-supported
; RUN: ocloc compile -spirv_input -file %t.spv -device xe2-hpg -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-BMG

target triple = "spir64-unknown-unknown"

%spirv.JointMatrixINTEL._float_8_16_3_3_2 = type opaque
declare spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)*, i64, i32, i32, i32) #0
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)*, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)*, i64, i32, i32, i32) #0


; CHECK: .kernel "TestLoad_L1_Uncached__L3_Uncached"
; CHECK: lsc_load_block2d.ugm.uc.uc ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Uncached__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!0 = !{!1, !2}
!1 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L1_Uncached__L3_Cached"
; CHECK: lsc_load_block2d.ugm.uc.ca ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Uncached__L3_Cached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !10
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!10 = !{!11, !12}
!11 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!12 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L1_Cached__L3_Uncached"
; CHECK: lsc_load_block2d.ugm.ca.uc ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Cached__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !20
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!20 = !{!21, !22}
!21 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!22 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L1_Cached__L3_Cached"
; CHECK: lsc_load_block2d.ugm.ca.ca ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Cached__L3_Cached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !30
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!30 = !{!31, !32}
!31 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!32 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L1_Streaming__L3_Uncached"
; CHECK: lsc_load_block2d.ugm.st.uc ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Streaming__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !40
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!40 = !{!41, !42}
!41 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!42 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L1_Streaming__L3_Cached"
; CHECK: lsc_load_block2d.ugm.st.ca ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Streaming__L3_Cached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !50
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!50 = !{!51, !52}
!51 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!52 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L1_InvalidateAfterRead__L3_Cached"
; CHECK: lsc_load_block2d.ugm.ri.ca ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_InvalidateAfterRead__L3_Cached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !60
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!60 = !{!61, !62}
!61 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!62 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L1_Uncached__L3_ConstCached"
; CHECK-BMG: lsc_load_block2d.ugm.uc.cc ({{.*}})
; CHECK-PVC: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Uncached__L3_ConstCached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !70
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!70 = !{!71, !72}
!71 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!72 = !{i32 6442, i32 1, i32 4}  ; {CacheControlLoadINTEL, CacheLevel=1, ConstCached}


; CHECK: .kernel "TestLoad_L1_Cached__L3_ConstCached"
; CHECK-BMG: lsc_load_block2d.ugm.ca.cc ({{.*}})
; CHECK-PVC: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_Cached__L3_ConstCached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !80
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!80 = !{!81, !82}
!81 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!82 = !{i32 6442, i32 1, i32 4}  ; {CacheControlLoadINTEL, CacheLevel=1, ConstCached}


; CHECK: .kernel "TestLoad_L1_InvalidateAfterRead__L3_InvalidateAfterRead"
; CHECK-BMG: lsc_load_block2d.ugm.ri.ri ({{.*}})
; CHECK-PVC: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm ({{.*}})
define spir_kernel void @TestLoad_L1_InvalidateAfterRead__L3_InvalidateAfterRead(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0, !spirv.Decorations !90
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!90 = !{!91, !92}
!91 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!92 = !{i32 6442, i32 1, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=1, InvalidateAfterRead}


; CHECK: .kernel "TestStore_L1_Uncached__L3_Uncached"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.uc.uc ({{.*}})
define spir_kernel void @TestStore_L1_Uncached__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !100
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!100 = !{!101, !102}
!101 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!102 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L1_Uncached__L3_WriteBack"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.uc.wb ({{.*}})
define spir_kernel void @TestStore_L1_Uncached__L3_WriteBack(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !110
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!110 = !{!111, !112}
!111 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!112 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L1_WriteThrough__L3_Uncached"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.wt.uc ({{.*}})
define spir_kernel void @TestStore_L1_WriteThrough__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !120
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!120 = !{!121, !122}
!121 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!122 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L1_WriteThrough__L3_WriteBack"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.wt.wb ({{.*}})
define spir_kernel void @TestStore_L1_WriteThrough__L3_WriteBack(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !130
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!130 = !{!131, !132}
!131 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!132 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L1_Streaming__L3_Uncached"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.st.uc ({{.*}})
define spir_kernel void @TestStore_L1_Streaming__L3_Uncached(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !140
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!140 = !{!141, !142}
!141 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!142 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L1_Streaming__L3_WriteBack"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.st.wb ({{.*}})
define spir_kernel void @TestStore_L1_Streaming__L3_WriteBack(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !150
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!150 = !{!151, !152}
!151 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!152 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L1_WriteBack__L3_WriteBack"
; CHECK: lsc_load_block2d.ugm ({{.*}})
; CHECK: lsc_store_block2d.ugm.wb.wb ({{.*}})
define spir_kernel void @TestStore_L1_WriteBack__L3_WriteBack(float addrspace(1)* %src, float addrspace(1)* %dst) {
entry:
%decoratedsrc = getelementptr inbounds float, float addrspace(1)* %src, i64 0
%0 =  call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2PU3AS4fliii(float addrspace(1)* %decoratedsrc, i64 64, i32 0, i32 3, i32 0) #0
%decorateddst = getelementptr inbounds float, float addrspace(1)* %dst, i64 0, !spirv.Decorations !160
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(float addrspace(1)* %decorateddst, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %0, i64 64, i32 0, i32 3, i32 0) #0
ret void
}
!160 = !{!161, !162}
!161 = !{i32 6443, i32 0, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteBack}
!162 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}
