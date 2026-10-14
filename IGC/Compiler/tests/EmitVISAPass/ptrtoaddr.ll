;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-22-plus, regkeys
; RUN: igc_opt --opaque-pointers -platformpvc -gas-info-gen -igc-emit-visa -S %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass - Test ptrtoaddr instruction generation
;
; ptrtoaddr on a generic pointer must clear the address space tag.
; Pointers in other address spaces carry no tag and are copied as is.
; ------------------------------------------------

; Generic pointer which may point to local memory - tag is cleared.
define spir_kernel void @test_generic_tagged(ptr addrspace(4) %p, ptr addrspace(3) %l, ptr addrspace(1) %out) {
; CHECK-LABEL: .kernel "test_generic_tagged"
; CHECK: shl (M1_NM, 1) [[TMP:V[0-9]+]](0,0)<1> p(0,0)<0;1,0> 0x4:d
; CHECK: asr (M1_NM, 1) [[TMP]](0,0)<1> [[TMP]](0,0)<0;1,0> 0x4:d
; CHECK: mov (M1_NM, {{[0-9]+}}) [[DST:V[0-9]+]](0,0)<1> {{V[0-9]+}}(0,0)<{{.*}}>
; CHECK: lsc_store.ugm {{.*}} [[DST]]
  %g = addrspacecast ptr addrspace(3) %l to ptr addrspace(4)
  store ptr addrspace(4) %g, ptr addrspace(1) %out
  %1 = ptrtoaddr ptr addrspace(4) %p to i64
  store i64 %1, ptr addrspace(1) %out
  ret void
}

; Generic pointer which can't point to private or local memory - no tag.
define spir_kernel void @test_generic_untagged(ptr addrspace(4) %p, ptr addrspace(1) %out) {
; CHECK-LABEL: .kernel "test_generic_untagged"
; CHECK-NOT: shl
; CHECK-NOT: asr
; CHECK: mov (M1_NM, 2) [[DST:V[0-9]+]](0,0)<1> p_0v(0,0)<1;1,0>
; CHECK-NOT: shl
; CHECK-NOT: asr
; CHECK: lsc_store.ugm {{.*}} [[DST]]
  %1 = ptrtoaddr ptr addrspace(4) %p to i64
  store i64 %1, ptr addrspace(1) %out
  ret void
}

; Non-generic pointer - no tag, even if generic pointers may be tagged.
define spir_kernel void @test_global(ptr addrspace(1) %p, ptr addrspace(3) %l, ptr addrspace(1) %out) {
; CHECK-LABEL: .kernel "test_global"
; CHECK-NOT: shl
; CHECK-NOT: asr
; CHECK: mov (M1_NM, 2) [[DST:V[0-9]+]](0,0)<1> p_0v(0,0)<1;1,0>
; CHECK-NOT: shl
; CHECK-NOT: asr
; CHECK: lsc_store.ugm {{.*}} [[DST]]
  %g = addrspacecast ptr addrspace(3) %l to ptr addrspace(4)
  store ptr addrspace(4) %g, ptr addrspace(1) %out
  %1 = ptrtoaddr ptr addrspace(1) %p to i64
  store i64 %1, ptr addrspace(1) %out
  ret void
}

!igc.functions = !{!0, !20, !21}
!IGCMetadata = !{!3}
!0 = !{ptr @test_generic_tagged, !1}
!20 = !{ptr @test_generic_untagged, !1}
!21 = !{ptr @test_global, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6, !22, !23, !24, !25}
!5 = !{!"FuncMDMap[0]", ptr @test_generic_tagged}
!6 = !{!"FuncMDValue[0]", !7}
!22 = !{!"FuncMDMap[1]", ptr @test_generic_untagged}
!23 = !{!"FuncMDValue[1]", !26}
!24 = !{!"FuncMDMap[2]", ptr @test_global}
!25 = !{!"FuncMDValue[2]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !13, !14}
!9 = !{!"argAllocMDListVec[0]", !10, !11, !12}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[1]", !10, !11, !12}
!14 = !{!"argAllocMDListVec[2]", !10, !11, !12}
!26 = !{!"resAllocMD", !27}
!27 = !{!"argAllocMDList", !9, !13}
