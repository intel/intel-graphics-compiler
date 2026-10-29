;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 32 -regkey DumpVISAASMToConsole,VectorAlias=2 | FileCheck %s --check-prefixes=CHECK,CHECK-BMG
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformpvc -simd-mode 32 -regkey DumpVISAASMToConsole,VectorAlias=2 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC

; Byte load -> zext to i32 -> bitcast to float -> insertelement chain.
; DeSSA coalesces the bitcast with the zext, so VRA aliases the zext into the
; vector slot. EmitZExtByteLoad must not remap the zext to the d8u32 load
; result, otherwise the alias is broken and the vector slot is never written.
;
; Xe2+ (BMG): d8u32 zero-extends the loaded byte to a dword, so the load result is
; copied directly into the vector slot, with no narrow byte copy in between.
; Pre-Xe2 (PVC): d8u32 zero-extension is not guaranteed, so the byte is first
; copied to a narrow variable (stride <4;1,0>) and then into the vector.

; CHECK-LABEL: .kernel "test"

; CHECK-BMG:       lsc_load.ugm (M1, 32) [[W0:[A-Za-z0-9_]+]]:d8u32
; CHECK-BMG-NEXT:  mov (M1, 32) [[V:[A-Za-z0-9]+]]_0v(0,0)<1> [[W0]](0,0)<1;1,0>
; CHECK-BMG-NOT:   <4;1,0>
; CHECK-BMG:       lsc_load.ugm (M1, 32) [[W1:[A-Za-z0-9_]+]]:d8u32
; CHECK-BMG-NEXT:  mov (M1, 32) [[V]]_0v(2,0)<1> [[W1]](0,0)<1;1,0>
; CHECK-BMG-NOT:   <4;1,0>
; CHECK-BMG:       lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32x2

; CHECK-PVC:       lsc_load.ugm (M1, 32) {{.*}}:d8u32
; CHECK-PVC-NEXT:  mov (M1, 32) [[B0:[A-Za-z0-9_]+]](0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<4;1,0>
; CHECK-PVC-NEXT:  mov (M1, 32) [[V:[A-Za-z0-9]+]]_0v(0,0)<1> [[B0]]_0v(0,0)<1;1,0>
; CHECK-PVC:       lsc_load.ugm (M1, 32) {{.*}}:d8u32
; CHECK-PVC-NEXT:  mov (M1, 32) [[B1:[A-Za-z0-9_]+]](0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<4;1,0>
; CHECK-PVC-NEXT:  mov (M1, 32) [[V]]_0v(2,0)<1> [[B1]]_0v(0,0)<1;1,0>
; CHECK-PVC:       lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32x2

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %lid = zext i16 %localIdX to i64
  %p0 = getelementptr i8, ptr addrspace(1) %src, i64 %lid
  %b0 = load i8, ptr addrspace(1) %p0, align 1
  %z0 = zext i8 %b0 to i32
  %f0 = bitcast i32 %z0 to float
  %p1 = getelementptr i8, ptr addrspace(1) %p0, i64 64
  %b1 = load i8, ptr addrspace(1) %p1, align 1
  %z1 = zext i8 %b1 to i32
  %f1 = bitcast i32 %z1 to float
  %v0 = insertelement <2 x float> undef, float %f0, i32 0
  %v1 = insertelement <2 x float> %v0, float %f1, i32 1
  %off = shl i64 %lid, 3
  %q = getelementptr i8, ptr addrspace(1) %dst, i64 %off
  store <2 x float> %v1, ptr addrspace(1) %q, align 8
  ret void
}

attributes #0 = { nounwind null_pointer_is_valid }

!igc.functions = !{!0}
!IGCMetadata = !{!20}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23}
!22 = !{!"FuncMDMap[0]", ptr @test}
!23 = !{!"FuncMDValue[0]", !24, !60}
!24 = !{!"resAllocMD", !25}
!25 = !{!"argAllocMDList", !26, !27, !28, !29, !30, !31, !32, !33}
!26 = !{!"argAllocMDListVec[0]", !40, !41, !42}
!27 = !{!"argAllocMDListVec[1]", !40, !41, !42}
!28 = !{!"argAllocMDListVec[2]", !40, !41, !42}
!29 = !{!"argAllocMDListVec[3]", !40, !41, !42}
!30 = !{!"argAllocMDListVec[4]", !40, !41, !42}
!31 = !{!"argAllocMDListVec[5]", !40, !41, !42}
!32 = !{!"argAllocMDListVec[6]", !40, !41, !42}
!33 = !{!"argAllocMDListVec[7]", !40, !41, !42}
!40 = !{!"type", i32 0}
!41 = !{!"extensionType", i32 -1}
!42 = !{!"indexType", i32 -1}
!43 = !{!"argId", i32 0}
!44 = !{!"implicitArgInfoListVec[0]", !43}
!45 = !{!"argId", i32 1}
!46 = !{!"implicitArgInfoListVec[1]", !45}
!47 = !{!"argId", i32 7}
!48 = !{!"implicitArgInfoListVec[2]", !47}
!49 = !{!"argId", i32 8}
!50 = !{!"implicitArgInfoListVec[3]", !49}
!51 = !{!"argId", i32 9}
!52 = !{!"implicitArgInfoListVec[4]", !51}
!53 = !{!"argId", i32 10}
!54 = !{!"implicitArgInfoListVec[5]", !53}
!60 = !{!"implicitArgInfoList", !44, !46, !48, !50, !52, !54}
