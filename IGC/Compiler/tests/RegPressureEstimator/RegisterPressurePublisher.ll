;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --opaque-pointers -igc-pressure-publisher -igc-serialize-metadata -S %s | FileCheck %s
; RUN: igc_opt --typed-pointers -igc-pressure-publisher -igc-serialize-metadata -S %s | FileCheck %s

; Test checks that RegisterPressurePublisher writes maxRegPressure and the per-SIMD
; maxRegPressureSimd16 / maxRegPressureSimd32 entries in FuncMD metadata.

; CHECK: "maxRegPressureSimd16", i32 {{[0-9]+}}
; CHECK: "maxRegPressureSimd32", i32 {{[0-9]+}}

define spir_kernel void @testNoUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = extractelement <3 x i32> %localSize, i64 0
  %1 = extractelement <3 x i32> %localSize, i64 1
  %localIdZ2 = zext i16 %localIdZ to i32
  %mul.i.i = mul i32 %1, %localIdZ2
  %localIdY4 = zext i16 %localIdY to i32
  %add.i.i = add i32 %mul.i.i, %localIdY4
  %mul4.i.i = mul i32 %0, %add.i.i
  %localIdX6 = zext i16 %localIdX to i32
  %add6.i.i = add i32 %mul4.i.i, %localIdX6
  %conv.i = zext i32 %add6.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %conv.i
  %2 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4
  ret void
}

!igc.functions = !{!1}

!1 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!17 = !{!"argId", i32 0}
!18 = !{!"implicitArgInfoListVec[0]", !17}
!19 = !{!"argId", i32 1}
!20 = !{!"implicitArgInfoListVec[1]", !19}
!21 = !{!"argId", i32 6}
!22 = !{!"implicitArgInfoListVec[2]", !21}
!23 = !{!"argId", i32 8}
!24 = !{!"implicitArgInfoListVec[3]", !23}
!25 = !{!"argId", i32 9}
!26 = !{!"implicitArgInfoListVec[4]", !25}
!27 = !{!"argId", i32 10}
!28 = !{!"implicitArgInfoListVec[5]", !27}
!29 = !{!"argId", i32 15}
!30 = !{!"explicitArgNum", i32 0}
!31 = !{!"implicitArgInfoListVec[6]", !29, !30}
!32 = !{!"argId", i32 15}
!33 = !{!"explicitArgNum", i32 1}
!34 = !{!"implicitArgInfoListVec[7]", !32, !33}
!35 = !{!"implicitArgInfoList", !18, !20, !22, !24, !26, !28, !31, !34}
!36 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif}
!37 = !{!"FuncMDValue[0]", !35}
!38 = !{!"FuncMD", !36, !37}
!39 = !{!"ModuleMD", !38}
!IGCMetadata = !{!39}
