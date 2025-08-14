;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; To test inline as on constraint "P" : plain immediate

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run -inputocl --neo \
; RUN:         -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole -simd-mode 16 %s  \
; RUN: | FileCheck %s

; CHECK-LABEL: .function
; CHECK:  dpas.bf.bf.8.2 (M1, 16)
; CHECK: ret (M1, 1)

; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @test(i32 addrspace(1)* align 4 %a, i32 addrspace(1)* align 4 %b, <8 x i32> %r0, <8 x i32> %payloadHeader,
       i16 %localIdX, i16 %localIdY, i16 %localIdZ ) {
entry:
  %lid = zext i16 %localIdX to i64
  %base = ptrtoint i32 addrspace(1)* %a to i64
  %bbase = ptrtoint i32 addrspace(1)* %b to i64
  %tmp0 = add i64  %base, %lid
  %a_addr = inttoptr i64 %tmp0 to i16 addrspace(1)*
  %tmp1 = add i64  %bbase, %lid
  %b_addr = inttoptr i64 %tmp1 to i16 addrspace(1)*
  %ma = load i32, ptr addrspace(1) %a, align 4
  %mb = load i32, ptr addrspace(1) %b, align 4

  %r = call i32 asm "dpas.bf.bf.8.$4 (M1, 16) $0.0  $1.0  $2.0  $3(0,0)" , "=rw,rw,rw,rw,P"(i32 0, i32 %ma, i32 %mb, i32 2)

  %tmp3 = add i64  %tmp0, 1024
  %d_addr = inttoptr i64 %tmp3 to i32 addrspace(1)*
  store i32 %r, i32 addrspace(1)* %d_addr
  ret void
}


!igc.functions = !{!0}
!IGCMetadata = !{!13}

!0 = !{void (i16 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!13 = !{!"ModuleMD", !14}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", void (i16 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16)* @test}
!16 = !{!"FuncMDValue[0]", !90, !100, !226}
!90 = !{!"localOffsets"}
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

