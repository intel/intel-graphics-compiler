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

!4 = !{!5, !6, !11}
!5 = !{!"function_type", i32 0}
!6 = !{!"arg_desc", !7}
!7 = !{null, !8, !9, !10}
!8 = !{!"explicit_arg_num", i32 0}
!9 = !{!"img_access_float_coords", i1 false}
!10 = !{!"img_access_int_coords", i1 true}

!11 = !{!"implicit_arg_desc", !12, !13, !14, !15, !16, !17, !18, !19, !21, !23}
!12 = !{i32 0}
!13 = !{i32 2}
!14 = !{i32 7}
!15 = !{i32 8}
!16 = !{i32 9}
!17 = !{i32 10}
!18 = !{i32 13}
!19 = !{i32 33, !20}
!20 = !{!"explicit_arg_num", i32 18}
!21 = !{i32 15, !22}
!22 = !{!"explicit_arg_num", i32 2}
!23 = !{i32 59, !22}

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
!300 = !{!"FuncMDValue[1]", !306}

!306 = !{!"resAllocMD", !203, !204, !205, !307, !308, !309}
!307 = !{!"uavsNumType", i32 0}
!308 = !{!"argAllocMDList"}
!309 = !{!"inlineSamplersMD"}