;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; To vector alias on inline asm

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run -inputocl --neo \
; RUN:         -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole,VectorAlias=1 -simd-mode 16 %s  \
; RUN: | FileCheck %s

; CHECK-LABEL: .function
; CHECK: lsc_load_block2d.ugm (M1, 1)  [[INPUT:.*]]:d8.16x8nt  flat[{{.+}}]
; CHECK: mov (M1_NM, 16) [[OUTPUT:.*]](0,0)<2> [[INPUT]](0,0)<4;1,0>
; CHECK: mov (M1_NM, 16) [[OUTPUT]](0,1)<2> [[INPUT]](0,1)<4;1,0>
; CHECK: mov (M1_NM, 16) [[OUTPUT]](1,0)<2> [[INPUT]](0,2)<4;1,0>
; CHECK: mov (M1_NM, 16) [[OUTPUT]](1,1)<2> [[INPUT]](0,3)<4;1,0>
; CHECK: mov (M1, 16) [[OUTPUT]](2,0)<1> 0x0:w
; CHECK: mov (M1, 16) [[OUTPUT]](2,16)<1> 0x0:w
; CHECK: mov (M1, 16) [[OUTPUT]](3,0)<1> 0x0:w
; CHECK: mov (M1, 16) [[OUTPUT]](3,16)<1> 0x0:w
; CHECK: lsc_store_block2d.ugm (M1, 1)  flat[{{.+}}]  [[OUTPUT]]:d16.16x8nn
; CHECK: ret (M1, 1)

; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @test(i8 addrspace(1)* align 1 %a, i16 addrspace(1)* align 2 %b, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = call <8 x i8> asm "lsc_load_block2d.ugm (M1, 1) $0:d8.16x8nt flat[$1,15,15,15,0,0]", "=rw,rw.u"(i8 addrspace(1)* %a)
  %1 = extractelement <8 x i8> %0, i32 0
  %2 = insertelement <4 x i8> undef, i8 %1, i32 0
  %3 = extractelement <8 x i8> %0, i32 1
  %4 = insertelement <4 x i8> %2, i8 %3, i32 1
  %5 = extractelement <8 x i8> %0, i32 2
  %6 = insertelement <4 x i8> %4, i8 %5, i32 2
  %7 = extractelement <8 x i8> %0, i32 3
  %8 = insertelement <4 x i8> %6, i8 %7, i32 3
  %9 = call <4 x i16> asm "mov (M1_NM, 16) $0(0,0)<2> $1(0,0)<4;1,0>\0Amov (M1_NM, 16) $0(0,1)<2> $1(0,1)<4;1,0>\0Amov (M1_NM, 16) $0(1,0)<2> $1(0,2)<4;1,0>\0Amov (M1_NM, 16) $0(1,1)<2> $1(0,3)<4;1,0>\0A", "=rw,rw"(<4 x i8> %8)
  %10 = extractelement <4 x i16> %9, i32 0
  %11 = extractelement <4 x i16> %9, i32 1
  %12 = extractelement <4 x i16> %9, i32 2
  %13 = extractelement <4 x i16> %9, i32 3
  %14 = insertelement <8 x i16> undef, i16 %10, i32 0
  %15 = insertelement <8 x i16> %14, i16 %11, i32 1
  %16 = insertelement <8 x i16> %15, i16 %12, i32 2
  %17 = insertelement <8 x i16> %16, i16 %13, i32 3
  %18 = insertelement <8 x i16> %17, i16 0, i32 4
  %19 = insertelement <8 x i16> %18, i16 0, i32 5
  %20 = insertelement <8 x i16> %19, i16 0, i32 6
  %21 = insertelement <8 x i16> %20, i16 0, i32 7
  call void asm sideeffect "lsc_store_block2d.ugm (M1, 1) flat[$1,15,15,15,0,0] $0:d16.16x8nn", "rw,rw.u"(<8 x i16> %21, i16 addrspace(1)* %b)
  ret void
}


!igc.functions = !{!0}
!IGCMetadata = !{!13}

!0 = !{void (i8 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i32, i32)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!13 = !{!"ModuleMD", !14}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i32, i32)* @test}
!16 = !{!"FuncMDValue[0]", !100, !226}
!100 = !{!"resAllocMD", !183, !184, !185, !186}
!183 = !{!"uavsNumType", i32 0}
!184 = !{!"srvsNumType", i32 0}
!185 = !{!"samplersNumType", i32 0}
!186 = !{!"argAllocMDList", !187, !191, !192, !193, !194, !195, !196}
!187 = !{!"argAllocMDListVec[0]", !188, !189, !190}
!188 = !{!"type", i32 0}
!189 = !{!"extensionType", i32 -1}
!190 = !{!"indexType", i32 -1}
!191 = !{!"argAllocMDListVec[1]", !188, !189, !190}
!192 = !{!"argAllocMDListVec[2]", !188, !189, !190}
!193 = !{!"argAllocMDListVec[3]", !188, !189, !190}
!194 = !{!"argAllocMDListVec[4]", !188, !189, !190}
!195 = !{!"argAllocMDListVec[5]", !188, !189, !190}
!196 = !{!"argAllocMDListVec[6]", !188, !189, !190}
!226 = !{!"m_OpenCLArgTypeQualifiers", !227, !228}
!227 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!228 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}

