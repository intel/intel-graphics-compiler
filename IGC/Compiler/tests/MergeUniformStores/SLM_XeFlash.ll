;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-merge-uniform-stores -S < %s | FileCheck %s
; run: igc_opt -debugify -igc-merge-uniform-stores -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; MergeUniformStores
; ------------------------------------------------

; Debug-info check fails
;
; COM: check-not WARNING
; COM: check CheckModuleDebugify: PASS

%__2D_DIM_Resource = type opaque
%"class.RWTexture2D<unsigned int>" = type { i32 }

@"\01?tgsm@@3PAHA" = external addrspace(3) global [16 x i32], align 4
@ThreadGroupSize_X = constant i32 16
@ThreadGroupSize_Y = constant i32 16
@ThreadGroupSize_Z = constant i32 1

define void @main(<8 x i32> %r0) {
._crit_edge:
  %0 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %0 to i32
  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
  %GroupID_Y = bitcast float %1 to i32
  %2 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %u0 = inttoptr i32 %2 to %__2D_DIM_Resource addrspace(2490368)*
  %3 = shl i32 %GroupID_X, 4
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ThreadID_X = add i32 %3, %LocalID_X
  %4 = load i32, i32 addrspace(3)* null, align 2147483648
  %5 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* nonnull inttoptr (i32 4 to i32 addrspace(3)*), i32 addrspace(3)* nonnull inttoptr (i32 4 to i32 addrspace(3)*), i32 %4, i32 0)
; CHECK:  %WaveBallot = call i32 @llvm.genx.GenISA.WaveBallot.i1.i32(i1 true, i32 0)
; CHECK:  %FirstBitHi = call i32 @llvm.genx.GenISA.firstbitHi(i32 %WaveBallot)
; CHECK:  %WaveShufIdx = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %5, i32 %FirstBitHi, i32 0)
; CHECK:  store i32 %WaveShufIdx, i32 addrspace(3)* inttoptr (i32 8 to i32 addrspace(3)*), align 8
  store i32 %5, i32 addrspace(3)* inttoptr (i32 8 to i32 addrspace(3)*), align 8
  %6 = load i32, i32 addrspace(3)* inttoptr (i32 4 to i32 addrspace(3)*), align 4
  %7 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* nonnull inttoptr (i32 8 to i32 addrspace(3)*), i32 addrspace(3)* nonnull inttoptr (i32 8 to i32 addrspace(3)*), i32 %6, i32 0)
; CHECK:  %WaveShufIdx1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %7, i32 %FirstBitHi, i32 0)
; CHECK:  store i32 %WaveShufIdx1, i32 addrspace(3)* inttoptr (i32 12 to i32 addrspace(3)*), align 4
  store i32 %7, i32 addrspace(3)* inttoptr (i32 12 to i32 addrspace(3)*), align 4
  %8 = load i32, i32 addrspace(3)* inttoptr (i32 8 to i32 addrspace(3)*), align 8
  %9 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* nonnull inttoptr (i32 12 to i32 addrspace(3)*), i32 addrspace(3)* nonnull inttoptr (i32 12 to i32 addrspace(3)*), i32 %8, i32 0)
; CHECK:  %WaveShufIdx2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %9, i32 %FirstBitHi, i32 0)
; CHECK:  store i32 %WaveShufIdx2, i32 addrspace(3)* inttoptr (i32 16 to i32 addrspace(3)*), align 16
  store i32 %9, i32 addrspace(3)* inttoptr (i32 16 to i32 addrspace(3)*), align 16
  %10 = shl i32 %GroupID_Y, 4
  %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
  %ThreadID_Y = add i32 %10, %LocalID_Y
  %11 = load float, float addrspace(3)* inttoptr (i32 16 to float addrspace(3)*), align 16
  call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %u0, i32 %ThreadID_X, i32 %ThreadID_Y, i32 0, i32 0, float %11, float %11, float %11, float %11)
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #0

; Function Attrs: argmemonly nounwind
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)*, i32 addrspace(3)*, i32, i32) #1

; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)*, i32, i32, i32, i32, float, float, float, float) #2

; Function Attrs: nounwind readnone
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #0

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

; Function Attrs: convergent inaccessiblememonly nounwind
declare i32 @llvm.genx.GenISA.WaveBallot.i1.i32(i1, i32) #3

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.firstbitHi(i32) #0

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32, i32, i32) #4

attributes #0 = { nounwind readnone }
attributes #1 = { argmemonly nounwind }
attributes #2 = { argmemonly nounwind writeonly }
attributes #3 = { convergent inaccessiblememonly nounwind }
attributes #4 = { convergent nounwind readnone }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!8}
!igc.functions = !{!11}

!0 = !{!"dxcoob 1.7.2212.40 (e043f4a12)"}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 7}
!3 = !{!"cs", i32 6, i32 0}
!4 = !{null, !5, null, null}
!5 = !{!6}
!6 = !{i32 0, %"class.RWTexture2D<unsigned int>"* undef, !"", i32 0, i32 0, i32 1, i32 2, i1 false, i1 false, i1 false, !7}
!7 = !{i32 0, i32 5}
!8 = distinct !{null, !"main", null, !4, !9}
!9 = !{i32 4, !10}
!10 = !{i32 16, i32 16, i32 1}
!11 = !{void (<8 x i32>)* @main, !12}
!12 = !{!13, !14}
!13 = !{!"function_type", i32 0}
!14 = !{!"implicit_arg_desc", !15}
!15 = !{i32 0}
