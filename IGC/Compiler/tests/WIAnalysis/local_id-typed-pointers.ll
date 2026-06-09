;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: regkeys
; RUN: igc_opt --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

define spir_kernel void @testYZUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
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

define spir_kernel void @testZUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
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

;CHECK: block: entry function: testYZUnif
;CHECK: U: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: U: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

;CHECK: block: entry function: testNoUnif
;CHECK: N: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: N: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

;CHECK: block: entry function: testZUnif
;CHECK: U: {{.*}} [[localIdZ:%.*]] = zext i16 %localIdZ to i32
;CHECK: N: {{.*}} [[localIdY:%.*]] = zext i16 %localIdY to i32
;CHECK: N: {{.*}} [[localIdX:%.*]] = zext i16 %localIdX to i32

!igc.functions = !{!1, !2, !3}
!IGCMetadata = !{!19}

!1 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif, !41}
!2 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif, !42}
!3 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testZUnif, !43}
!41 = !{!5}
!42 = !{!5}
!43 = !{!5}
!5 = !{!"function_type", i32 0}
!19 = !{!"ModuleMD", !112}
!112 = !{!"FuncMD", !113, !114, !333, !334, !444, !445}
!113 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testYZUnif}
!114 = !{!"FuncMDValue[0]", !116, !120, !464}
!333 = !{!"FuncMDMap[1]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif}
!334 = !{!"FuncMDValue[1]", !116, !483}
!444 = !{!"FuncMDMap[2]", void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testZUnif}
!445 = !{!"FuncMDValue[2]", !116, !124, !502}
!116 = !{!"workGroupWalkOrder", !117, !118, !119}
!117 = !{!"dim0", i32 0}
!118 = !{!"dim1", i32 1}
!119 = !{!"dim2", i32 2}
!120 = !{!"threadGroupSize", !121, !122, !123}
!121 = !{!"dim0", i32 32}
!122 = !{!"dim1", i32 32}
!123 = !{!"dim2", i32 32}
!124 = !{!"threadGroupSize", !125, !126, !127}
!125 = !{!"dim0", i32 16}
!126 = !{!"dim1", i32 32}
!127 = !{!"dim2", i32 32}
!446 = !{!"argId", i32 0}
!447 = !{!"implicitArgInfoListVec[0]", !446}
!448 = !{!"argId", i32 1}
!449 = !{!"implicitArgInfoListVec[1]", !448}
!450 = !{!"argId", i32 6}
!451 = !{!"implicitArgInfoListVec[2]", !450}
!452 = !{!"argId", i32 8}
!453 = !{!"implicitArgInfoListVec[3]", !452}
!454 = !{!"argId", i32 9}
!455 = !{!"implicitArgInfoListVec[4]", !454}
!456 = !{!"argId", i32 10}
!457 = !{!"implicitArgInfoListVec[5]", !456}
!458 = !{!"argId", i32 15}
!459 = !{!"explicitArgNum", i32 0}
!460 = !{!"implicitArgInfoListVec[6]", !458, !459}
!461 = !{!"argId", i32 15}
!462 = !{!"explicitArgNum", i32 1}
!463 = !{!"implicitArgInfoListVec[7]", !461, !462}
!464 = !{!"implicitArgInfoList", !447, !449, !451, !453, !455, !457, !460, !463}
!465 = !{!"argId", i32 0}
!466 = !{!"implicitArgInfoListVec[0]", !465}
!467 = !{!"argId", i32 1}
!468 = !{!"implicitArgInfoListVec[1]", !467}
!469 = !{!"argId", i32 6}
!470 = !{!"implicitArgInfoListVec[2]", !469}
!471 = !{!"argId", i32 8}
!472 = !{!"implicitArgInfoListVec[3]", !471}
!473 = !{!"argId", i32 9}
!474 = !{!"implicitArgInfoListVec[4]", !473}
!475 = !{!"argId", i32 10}
!476 = !{!"implicitArgInfoListVec[5]", !475}
!477 = !{!"argId", i32 15}
!478 = !{!"explicitArgNum", i32 0}
!479 = !{!"implicitArgInfoListVec[6]", !477, !478}
!480 = !{!"argId", i32 15}
!481 = !{!"explicitArgNum", i32 1}
!482 = !{!"implicitArgInfoListVec[7]", !480, !481}
!483 = !{!"implicitArgInfoList", !466, !468, !470, !472, !474, !476, !479, !482}
!484 = !{!"argId", i32 0}
!485 = !{!"implicitArgInfoListVec[0]", !484}
!486 = !{!"argId", i32 1}
!487 = !{!"implicitArgInfoListVec[1]", !486}
!488 = !{!"argId", i32 6}
!489 = !{!"implicitArgInfoListVec[2]", !488}
!490 = !{!"argId", i32 8}
!491 = !{!"implicitArgInfoListVec[3]", !490}
!492 = !{!"argId", i32 9}
!493 = !{!"implicitArgInfoListVec[4]", !492}
!494 = !{!"argId", i32 10}
!495 = !{!"implicitArgInfoListVec[5]", !494}
!496 = !{!"argId", i32 15}
!497 = !{!"explicitArgNum", i32 0}
!498 = !{!"implicitArgInfoListVec[6]", !496, !497}
!499 = !{!"argId", i32 15}
!500 = !{!"explicitArgNum", i32 1}
!501 = !{!"implicitArgInfoListVec[7]", !499, !500}
!502 = !{!"implicitArgInfoList", !485, !487, !489, !491, !493, !495, !498, !501}
