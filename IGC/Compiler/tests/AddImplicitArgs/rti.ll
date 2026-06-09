;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-add-implicit-args -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK:     define spir_kernel void @test_rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_kernel void @test_rti()
; CHECK:     call spir_func void @foo(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)

define spir_kernel void @test_rti() {
  call spir_func void @foo()
  ret void
}

; CHECK:     define spir_func void @foo(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_func void @foo()
; CHECK:     call spir_func void @rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)

define spir_func void @foo() {
  call spir_func void @rti()
  ret void
}

; CHECK:     define spir_func void @rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_func void @rti()

define spir_func void @rti() {
  %1 = call ptr addrspace(1) @llvm.genx.GenISA.GlobalBufferPointer()
  %2 = call ptr addrspace(1) @llvm.genx.GenISA.LocalBufferPointer()
  %3 = call i16 @llvm.genx.GenISA.AsyncStackID()
  %stackid_ext = zext i16 %3 to i32
  %4 = call ptr addrspace(1) @llvm.genx.GenISA.InlinedData(i32 %stackid_ext)
  ret void
}

declare ptr addrspace(1) @llvm.genx.GenISA.GlobalBufferPointer()

declare ptr addrspace(1) @llvm.genx.GenISA.LocalBufferPointer()

declare i16 @llvm.genx.GenISA.AsyncStackID()

declare ptr addrspace(1) @llvm.genx.GenISA.InlinedData(i32)

!igc.functions = !{!0, !8, !9}

!0 = !{ptr @test_rti, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!8 = !{ptr @rti, !1}
!9 = !{ptr @foo, !1}
!10 = !{!"argId", i32 50}
!11 = !{!"implicitArgInfoListVec[0]", !10}
!12 = !{!"argId", i32 51}
!13 = !{!"implicitArgInfoListVec[1]", !12}
!14 = !{!"argId", i32 53}
!15 = !{!"implicitArgInfoListVec[2]", !14}
!16 = !{!"argId", i32 52}
!17 = !{!"implicitArgInfoListVec[3]", !16}
!18 = !{!"implicitArgInfoList", !11, !13, !15, !17}
!19 = !{!"argId", i32 50}
!20 = !{!"implicitArgInfoListVec[0]", !19}
!21 = !{!"argId", i32 51}
!22 = !{!"implicitArgInfoListVec[1]", !21}
!23 = !{!"argId", i32 53}
!24 = !{!"implicitArgInfoListVec[2]", !23}
!25 = !{!"argId", i32 52}
!26 = !{!"implicitArgInfoListVec[3]", !25}
!27 = !{!"implicitArgInfoList", !20, !22, !24, !26}
!28 = !{!"argId", i32 50}
!29 = !{!"implicitArgInfoListVec[0]", !28}
!30 = !{!"argId", i32 51}
!31 = !{!"implicitArgInfoListVec[1]", !30}
!32 = !{!"argId", i32 53}
!33 = !{!"implicitArgInfoListVec[2]", !32}
!34 = !{!"argId", i32 52}
!35 = !{!"implicitArgInfoListVec[3]", !34}
!36 = !{!"implicitArgInfoList", !29, !31, !33, !35}
!37 = !{!"FuncMDMap[0]", ptr @test_rti}
!38 = !{!"FuncMDValue[0]", !18}
!39 = !{!"FuncMDMap[1]", ptr @rti}
!40 = !{!"FuncMDValue[1]", !27}
!41 = !{!"FuncMDMap[2]", ptr @foo}
!42 = !{!"FuncMDValue[2]", !36}
!43 = !{!"FuncMD", !37, !38, !39, !40, !41, !42}
!44 = !{!"ModuleMD", !43}
!IGCMetadata = !{!44}
