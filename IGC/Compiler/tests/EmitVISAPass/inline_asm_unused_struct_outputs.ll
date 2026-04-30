;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that inline asm with struct return type correctly handles unused outputs.
; When some extractvalue users are optimized away, the emit pass must still
; allocate valid vISA variables for those outputs (not emit "null").

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run --neo \
; RUN:         -platformbmg -igc-emit-visa -regkey DumpVISAASMToConsole -simd-mode 16 %s \
; RUN: | FileCheck %s

; CHECK:     .decl [[V2:inline_asm_unused_out.*]] v_type=G type=d num_elts=16
; CHECK:     .decl [[V3:inline_asm_unused_out.*]] v_type=G type=d num_elts=16

; CHECK-LABEL: .function
; CHECK: mov (M1, 16) {{.*}}(0,0)<1> 0x0:ud
; CHECK: mov (M1, 16) {{.*}}(0,0)<1> 0x0:ud
; CHECK: mov (M1, 16) [[V2]](0,0)<1> 0x0:ud
; CHECK: mov (M1, 16) [[V3]](0,0)<1> 0x0:ud
; CHECK-NOT: null(

%structtype = type { i32, i32, i32, i32 }

define spir_kernel void @test(ptr addrspace(1) align 4 %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %input = load i32, ptr addrspace(1) %out, align 4

  ; Inline asm with 4 output operands and 1 input.
  ; Only outputs 0 and 1 have extractvalue users; outputs 2 and 3 are dead.
  %res = call %structtype asm sideeffect "{\0Amov (M1,16) $0(0,0)<1> 0x0:ud\0Amov (M1,16) $1(0,0)<1> 0x0:ud\0Amov (M1,16) $2(0,0)<1> 0x0:ud\0Amov (M1,16) $3(0,0)<1> 0x0:ud\0A}\0A", "=rw,=rw,=rw,=rw,rw.u"(i32 %input)

  ; Only extract outputs 0 and 1 — outputs 2 and 3 are unused.
  %out0 = extractvalue %structtype %res, 0
  %out1 = extractvalue %structtype %res, 1

  ; Use extracted outputs so they aren't dead.
  store i32 %out0, ptr addrspace(1) %out, align 4
  %ptr1 = getelementptr i32, ptr addrspace(1) %out, i64 1
  store i32 %out1, ptr addrspace(1) %ptr1, align 4

  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!13}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!13 = !{!"ModuleMD", !14}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", ptr @test}
!16 = !{!"FuncMDValue[0]", !90, !100, !226}
!90 = !{!"localOffsets"}
!100 = !{!"resAllocMD", !183, !184, !185, !186}
!183 = !{!"uavsNumType", i32 0}
!184 = !{!"srvsNumType", i32 0}
!185 = !{!"samplersNumType", i32 0}
!186 = !{!"argAllocMDList", !187, !191, !192, !193, !194, !195}
!187 = !{!"argAllocMDListVec[0]", !188, !189, !190}
!188 = !{!"type", i32 0}
!189 = !{!"extensionType", i32 -1}
!190 = !{!"indexType", i32 -1}
!191 = !{!"argAllocMDListVec[1]", !188, !189, !190}
!192 = !{!"argAllocMDListVec[2]", !188, !189, !190}
!193 = !{!"argAllocMDListVec[3]", !188, !189, !190}
!194 = !{!"argAllocMDListVec[4]", !188, !189, !190}
!195 = !{!"argAllocMDListVec[5]", !188, !189, !190}
!226 = !{!"m_OpenCLArgTypeQualifiers", !227}
!227 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
