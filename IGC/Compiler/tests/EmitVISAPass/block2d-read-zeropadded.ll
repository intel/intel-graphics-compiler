;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test runs vISA EmitPass and checks if 2d block reads with block sizes
; less than device GRF size are moved correctly.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableDebugging \
; RUN:   -simd-mode 16 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"
; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @test_zeropadded(i16 addrspace(1)* align 2 %dst, i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) #1 {
entry:
; elem size = 8 bits, block width 4, height = 2, 4, 8, block count = 2, 4
; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x4x2nn
; CHECK:  mov (M1_NM, 4) [[DST0:.*]](0,0)<1> [[TMP0:.*]](0,0)<16;2,1>
  %res0 = call i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 2, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x4x4nn
; CHECK:  mov (M1_NM, 8) [[DST1:.*]](0,0)<1> [[TMP1:.*]](0,0)<16;4,1>
  %res1 = call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 4, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x4x8nn
; CHECK:  mov (M1_NM, 16) [[DST2:.*]](0,0)<1> [[TMP2:.*]](0,0)<16;8,1>
  %res2 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 8, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x4x2nn
; CHECK:  mov (M1_NM, 4) [[DST3:.*]](0,0)<1> [[TMP3:.*]](0,0)<16;2,1>
; CHECK:  mov (M1_NM, 4) [[DST3]](0,4)<1> [[TMP3]](2,0)<16;2,1>
  %res3 = call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 2, i32 4, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x4x4nn
; CHECK:  mov (M1_NM, 8) [[DST4:.*]](0,0)<1> [[TMP4:.*]](0,0)<16;4,1>
; CHECK:  mov (M1_NM, 8) [[DST4]](0,8)<1> [[TMP4]](2,0)<16;4,1>
  %res4 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 4, i32 4, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x4x8nn
; CHECK:  mov (M1_NM, 16) [[DST5:.*]](0,0)<1> [[TMP5:.*]](0,0)<16;8,1>
; CHECK:  mov (M1_NM, 16) [[DST5]](1,0)<1> [[TMP5]](2,0)<16;8,1>
  %res5 = call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 8, i32 4, i1 false, i1 false, i32 0)


; elem size = 8 bits, block width 8, height = 1, 2, 4, block count = 2, 4
; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x8x1nn
; CHECK:  mov (M1_NM, 4) [[DST6:.*]](0,0)<1> [[TMP6:.*]](0,0)<16;2,1>
  %res6 = call i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 1, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x8x2nn
; CHECK:  mov (M1_NM, 8) [[DST7:.*]](0,0)<1> [[TMP7:.*]](0,0)<16;4,1>
  %res7 = call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 2, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x8x4nn
; CHECK:  mov (M1_NM, 16) [[DST8:.*]](0,0)<1> [[TMP8:.*]](0,0)<16;8,1>
  %res8 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 4, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x8x1nn
; CHECK:  mov (M1_NM, 4) [[DST9:.*]](0,0)<1> [[TMP9:.*]](0,0)<16;2,1>
; CHECK:  mov (M1_NM, 4) [[DST9]](0,4)<1> [[TMP9]](2,0)<16;2,1>
  %res9 = call <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 1, i32 4, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x8x2nn
; CHECK:  mov (M1_NM, 8) [[DST10:.*]](0,0)<1> [[TMP10:.*]](0,0)<16;4,1>
; CHECK:  mov (M1_NM, 8) [[DST10]](0,8)<1> [[TMP10]](2,0)<16;4,1>
  %res10 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 2, i32 4, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x8x4nn
; CHECK:  mov (M1_NM, 16) [[DST11:.*]](0,0)<1> [[TMP11:.*]](0,0)<16;8,1>
; CHECK:  mov (M1_NM, 16) [[DST11]](1,0)<1> [[TMP11]](2,0)<16;8,1>
  %res11 = call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 8, i32 4, i32 4, i1 false, i1 false, i32 0)


; elem size = 8 bits, block width 16, height = 1, 2, 4, block count = 2, 4
; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x16x1nn
; CHECK:  mov (M1_NM, 8) [[DST12:.*]](0,0)<1> [[TMP12:.*]](0,0)<16;4,1>
  %res12 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 16, i32 1, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.2x16x2nn
; CHECK:  mov (M1_NM, 16) [[DST13:.*]](0,0)<1> [[TMP13:.*]](0,0)<16;8,1>
  %res13 = call <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 16, i32 2, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d8.4x16x2nn
; CHECK:  mov (M1_NM, 16) [[DST14:.*]](0,0)<1> [[TMP14:.*]](0,0)<16;8,1>
; CHECK:  mov (M1_NM, 16) [[DST14]](1,0)<1> [[TMP14]](2,0)<16;8,1>
  %res14 = call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 16, i32 2, i32 4, i1 false, i1 false, i32 0)


; elem size = 16 bits, block width 8, height = 2, block count = 2, 4
; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d16.2x8x2nn
; CHECK:  mov (M1_NM, 16) [[DST15:.*]](0,0)<1> [[TMP15:.*]](0,0)<16;8,1>
  %res15 = call <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 16, i32 8, i32 2, i32 2, i1 false, i1 false, i32 0)

; CHECK:  lsc_load_block2d.ugm (M1, 1)  res{{.+}}:d16.4x8x2nn
; CHECK:  mov (M1_NM, 16) [[DST16:.*]](0,0)<1> [[TMP16:.*]](0,0)<16;8,1>
; CHECK:  mov (M1_NM, 16) [[DST16]](1,0)<1> [[TMP16]](2,0)<16;8,1>
  %res16 = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 16, i32 8, i32 2, i32 4, i1 false, i1 false, i32 0)

; CHECK: ret
  ret void
}

declare i8 @llvm.genx.GenISA.LSC2DBlockRead.i8(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <2 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v2i8(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <4 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v4i8(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <2 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v2i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

!igc.functions = !{!3}
!IGCMetadata = !{!16}

!3 = !{void (i16 addrspace(1)*, i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_zeropadded, !4}
!4 = !{!5, !6, !15}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{!"sub_group_size", i32 16}
!16 = !{!"ModuleMD", !131}
!131 = !{!"FuncMD", !132, !133}
!132 = !{!"FuncMDMap[0]", void (i16 addrspace(1)*, i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_zeropadded}
!133 = !{!"FuncMDValue[0]", !166, !237}
!166 = !{!"resAllocMD", !167, !168, !169, !170, !188}
!167 = !{!"uavsNumType", i32 0}
!168 = !{!"srvsNumType", i32 0}
!169 = !{!"samplersNumType", i32 0}
!170 = !{!"argAllocMDList", !171, !175, !176, !177, !178, !179, !180, !181, !182, !183, !184, !185, !186, !187}
!171 = !{!"argAllocMDListVec[0]", !172, !173, !174}
!172 = !{!"type", i32 0}
!173 = !{!"extensionType", i32 -1}
!174 = !{!"indexType", i32 -1}
!175 = !{!"argAllocMDListVec[1]", !172, !173, !174}
!176 = !{!"argAllocMDListVec[2]", !172, !173, !174}
!177 = !{!"argAllocMDListVec[3]", !172, !173, !174}
!178 = !{!"argAllocMDListVec[4]", !172, !173, !174}
!179 = !{!"argAllocMDListVec[5]", !172, !173, !174}
!180 = !{!"argAllocMDListVec[6]", !172, !173, !174}
!181 = !{!"argAllocMDListVec[7]", !172, !173, !174}
!182 = !{!"argAllocMDListVec[8]", !172, !173, !174}
!183 = !{!"argAllocMDListVec[9]", !172, !173, !174}
!184 = !{!"argAllocMDListVec[10]", !172, !173, !174}
!185 = !{!"argAllocMDListVec[11]", !172, !173, !174}
!186 = !{!"argAllocMDListVec[12]", !172, !173, !174}
!187 = !{!"argAllocMDListVec[13]", !172, !173, !174}
!188 = !{!"inlineSamplersMD"}
!237 = !{!"m_OpenCLArgTypeQualifiers", !238, !239, !240, !241, !242, !243, !244}
!238 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!239 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!240 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!241 = !{!"m_OpenCLArgTypeQualifiersVec[3]", !""}
!242 = !{!"m_OpenCLArgTypeQualifiersVec[4]", !""}
!243 = !{!"m_OpenCLArgTypeQualifiersVec[5]", !""}
!244 = !{!"m_OpenCLArgTypeQualifiersVec[6]", !""}
