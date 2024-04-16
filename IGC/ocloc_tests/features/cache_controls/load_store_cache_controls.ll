;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC

target triple = "spir64-unknown-unknown"


; CHECK: .kernel "TestLoad_L0_Uncached__L3_Uncached"
; CHECK: lsc_load.ugm.uc.uc ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Uncached__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !0
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!0 = !{!1, !2}
!1 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!2 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L0_Uncached__L3_Cached"
; CHECK: lsc_load.ugm.uc.ca ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Uncached__L3_Cached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !10
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!10 = !{!11, !12}
!11 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!12 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L0_Cached__L3_Uncached"
; CHECK: lsc_load.ugm.ca.uc ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Cached__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !20
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!20 = !{!21, !22}
!21 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!22 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L0_Cached__L3_Cached"
; CHECK: lsc_load.ugm.ca.ca ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Cached__L3_Cached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !30
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!30 = !{!31, !32}
!31 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!32 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L0_Streaming__L3_Uncached"
; CHECK: lsc_load.ugm.st.uc ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Streaming__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !40
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!40 = !{!41, !42}
!41 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!42 = !{i32 6442, i32 1, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestLoad_L0_Streaming__L3_Cached"
; CHECK: lsc_load.ugm.st.ca ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Streaming__L3_Cached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !50
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!50 = !{!51, !52}
!51 = !{i32 6442, i32 0, i32 2}  ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!52 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L0_InvalidateAfterRead__L3_Cached"
; CHECK: lsc_load.ugm.ri.ca ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_InvalidateAfterRead__L3_Cached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !60
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!60 = !{!61, !62}
!61 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!62 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}


; CHECK: .kernel "TestLoad_L0_Uncached__L3_ConstCached"
; CHECK-PVC: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Uncached__L3_ConstCached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !70
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!70 = !{!71, !72}
!71 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!72 = !{i32 6442, i32 1, i32 4}  ; {CacheControlLoadINTEL, CacheLevel=1, ConstCached}


; CHECK: .kernel "TestLoad_L0_Cached__L3_ConstCached"
; CHECK-PVC: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_Cached__L3_ConstCached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !80
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!80 = !{!81, !82}
!81 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!82 = !{i32 6442, i32 1, i32 4}  ; {CacheControlLoadINTEL, CacheLevel=1, ConstCached}


; CHECK: .kernel "TestLoad_L0_InvalidateAfterRead__L3_InvalidateAfterRead"
; CHECK-PVC: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm ({{.*}})
define spir_kernel void @TestLoad_L0_InvalidateAfterRead__L3_InvalidateAfterRead(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1, !spirv.Decorations !90
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!90 = !{!91, !92}
!91 = !{i32 6442, i32 0, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!92 = !{i32 6442, i32 1, i32 3}  ; {CacheControlLoadINTEL, CacheLevel=1, InvalidateAfterRead}


; CHECK: .kernel "TestStore_L0_Uncached__L3_Uncached"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.uc.uc ({{.*}})
define spir_kernel void @TestStore_L0_Uncached__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !100
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!100 = !{!101, !102}
!101 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!102 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L0_Uncached__L3_WriteBack"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.uc.wb ({{.*}})
define spir_kernel void @TestStore_L0_Uncached__L3_WriteBack(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !110
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!110 = !{!111, !112}
!111 = !{i32 6443, i32 0, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=0, Uncached}
!112 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L0_WriteThrough__L3_Uncached"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.wt.uc ({{.*}})
define spir_kernel void @TestStore_L0_WriteThrough__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !120
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!120 = !{!121, !122}
!121 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!122 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L0_WriteThrough__L3_WriteBack"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.wt.wb ({{.*}})
define spir_kernel void @TestStore_L0_WriteThrough__L3_WriteBack(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !130
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!130 = !{!131, !132}
!131 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!132 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L0_Streaming__L3_Uncached"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.st.uc ({{.*}})
define spir_kernel void @TestStore_L0_Streaming__L3_Uncached(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !140
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!140 = !{!141, !142}
!141 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!142 = !{i32 6443, i32 1, i32 0}  ; {CacheControlStoreINTEL, CacheLevel=1, Uncached}


; CHECK: .kernel "TestStore_L0_Streaming__L3_WriteBack"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.st.wb ({{.*}})
define spir_kernel void @TestStore_L0_Streaming__L3_WriteBack(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !150
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!150 = !{!151, !152}
!151 = !{i32 6443, i32 0, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=0, Streaming}
!152 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}


; CHECK: .kernel "TestStore_L0_WriteBack__L3_WriteBack"
; CHECK: lsc_load.ugm ({{.*}})
; CHECK: lsc_store.ugm.wb.wb ({{.*}})
define spir_kernel void @TestStore_L0_WriteBack__L3_WriteBack(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 0, !spirv.Decorations !160
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}
!160 = !{!161, !162}
!161 = !{i32 6443, i32 0, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteBack}
!162 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}
