;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -platformdg2 --target-addressing-mode bindless | FileCheck %s
;
; // Ensure that no unnecessary "add" is generated when alignment of the load is known (align 4)
; CHECK-NOT: [[ADD:%.*]] = add {{.*}} %bufferOffset

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Image._void_1_0_0_0_0_0_1 = type opaque

; Function Attrs: convergent nounwind
define spir_kernel void @convolve_1d_horizontal(
%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %src,
%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %dst,
float addrspace(1)* %gaussian_kernel,
i32 %diameter,
<8 x i32> %r0, <3 x i32> %globalOffset,
<3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ,
i8* %privateBase, i64 %inlineSampler, i32 %bufferOffset, i32 %bindlessOffset) #0 {
entry:
br label %for.body.lr.ph.new

for.body.lr.ph.new:                               ; preds = %for.body.lr.ph
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.lr.ph.new
  %index = phi i32 [ 0, %for.body.lr.ph.new ], [ %inc.3, %for.body ]
  %iter = phi i32 [ 0, %for.body.lr.ph.new ], [ %iterNext, %for.body ]

  %idxprom = zext i32 %index to i64

; // Random int value 500123
  %ptrInStateFulAddrSpace = inttoptr i32 500123 to float addrspace(131073)*
  %arrayidx = getelementptr inbounds float, float addrspace(131073)* %ptrInStateFulAddrSpace, i64 %idxprom
  %load = load float, float addrspace(131073)* %arrayidx, align 4

  %inc.3 = add nuw nsw i32 %index, 4
  %iterNext = add i32 %iter, 4
  %condition = icmp eq i32 %iterNext, 15
  br i1 %condition, label %cleanup.cont, label %for.body

cleanup.cont:
  ret void
}

!igc.functions = !{!24}
!IGCMetadata = !{!28}

!4 = !{!5}
!5 = !{!"function_type", i32 0}


!24 = !{void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*, float addrspace(1)*, i32, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8*, i64, i32, i32)* @convolve_1d_horizontal, !4}

!28 = !{!"ModuleMD", !30, !163}
!30 = !{!"compOpt", !65, !66, !67}

!65 = !{!"HasPositivePointerOffset", i1 false}
!66 = !{!"HasBufferOffsetArg", i1 true}
!67 = !{!"BufferOffsetArgOptional", i1 true}

!163 = !{!"FuncMD", !299, !300}

!203 = !{!"srvsNumType", i32 0}
!204 = !{!"samplersNumType", i32 0}

!205 = !{!"argAllocMDList", !206, !210, !212, !216, !217, !218, !219, !220, !221, !222, !223, !224, !225, !226}
!206 = !{!"argAllocMDListVec[0]", !207, !208, !209}
!207 = !{!"type", i32 4}
!208 = !{!"extensionType", i32 0}
!209 = !{!"indexType", i32 0}
!210 = !{!"argAllocMDListVec[1]", !207, !208, !211}
!211 = !{!"indexType", i32 1}
!212 = !{!"argAllocMDListVec[2]", !213, !214, !215}
!213 = !{!"type", i32 0}
!214 = !{!"extensionType", i32 -1}
!215 = !{!"indexType", i32 -1}
!216 = !{!"argAllocMDListVec[3]", !213, !214, !215}
!217 = !{!"argAllocMDListVec[4]", !213, !214, !215}
!218 = !{!"argAllocMDListVec[5]", !213, !214, !215}
!219 = !{!"argAllocMDListVec[6]", !213, !214, !215}
!220 = !{!"argAllocMDListVec[7]", !213, !214, !215}
!221 = !{!"argAllocMDListVec[8]", !213, !214, !215}
!222 = !{!"argAllocMDListVec[9]", !213, !214, !215}
!223 = !{!"argAllocMDListVec[10]", !213, !214, !215}
!224 = !{!"argAllocMDListVec[11]", !213, !214, !215}
!225 = !{!"argAllocMDListVec[12]", !213, !214, !215}
!226 = !{!"argAllocMDListVec[13]", !213, !214, !215}

!299 = !{!"FuncMDMap[1]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*, float addrspace(1)*, i32, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8*, i64, i32, i32)* @convolve_1d_horizontal}
!300 = !{!"FuncMDValue[1]", !306, !333, !338}

!306 = !{!"resAllocMD", !203, !204, !205, !307, !308, !309}
!307 = !{!"uavsNumType", i32 0}
!308 = !{!"argAllocMDList"}
!309 = !{!"inlineSamplersMD"}
!310 = !{!"argId", i32 0}
!311 = !{!"implicitArgInfoListVec[0]", !310}
!312 = !{!"argId", i32 2}
!313 = !{!"implicitArgInfoListVec[1]", !312}
!314 = !{!"argId", i32 7}
!315 = !{!"implicitArgInfoListVec[2]", !314}
!316 = !{!"argId", i32 8}
!317 = !{!"implicitArgInfoListVec[3]", !316}
!318 = !{!"argId", i32 9}
!319 = !{!"implicitArgInfoListVec[4]", !318}
!320 = !{!"argId", i32 10}
!321 = !{!"implicitArgInfoListVec[5]", !320}
!322 = !{!"argId", i32 13}
!323 = !{!"implicitArgInfoListVec[6]", !322}
!324 = !{!"argId", i32 33}
!325 = !{!"explicitArgNum", i32 18}
!326 = !{!"implicitArgInfoListVec[7]", !324, !325}
!327 = !{!"argId", i32 15}
!328 = !{!"explicitArgNum", i32 2}
!329 = !{!"implicitArgInfoListVec[8]", !327, !328}
!330 = !{!"argId", i32 59}
!331 = !{!"explicitArgNum", i32 2}
!332 = !{!"implicitArgInfoListVec[9]", !330, !331}
!333 = !{!"implicitArgInfoList", !311, !313, !315, !317, !319, !321, !323, !326, !329, !332}
!334 = !{!"explicitArgNum", i32 0}
!335 = !{!"imgAccessFloatCoords", i32 0}
!336 = !{!"imgAccessIntCoords", i32 1}
!337 = !{!"argInfoListVec[0]", !334, !335, !336}
!338 = !{!"argInfoList", !337}