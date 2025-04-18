;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; This test runs vISA EmitPass and checks if d8/d16 2d block transpose have
; been emulated correctly.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableDebugging \
; RUN:   -simd-mode 16 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

;
; CHECK-LABEL: .kernel
; CHECK: .function
;
; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @test_emu_transpose(i16 addrspace(1)* align 2 %dst, i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) #1 {
entry:
%ibase = ptrtoint i16 addrspace(1)* %dst to i64
  %lid = zext i16 %localIdX to i64
  %tmp = add i64  %ibase, %lid
;
; case 1 u8_m32k4 transpose
; CHECK:  lsc_load_block2d.ugm (M1, 1)  {{.*}}:d8.4x32nt  flat[base,widthm1,heightm1,pitchm1,x,y]
; CHECK:  mov (M1_NM, 8) [[T0:.*]](0,0)<1> [[T1:.*]](0,0)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T0]](0,8)<1>    [[T1]](0,1)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T0]](1,0)<1>    [[T1]](0,2)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T0]](1,8)<1>    [[T1]](0,3)<4;1,0>
;
  %res0 = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 8, i32 4, i32 32, i32 1, i1 true, i1 false, i32 0)
  %tmp0 = add i64 %tmp, 16
  %res00 = bitcast <4 x i16> %res0 to <2 x i32>
  %addr0 = inttoptr i64 %tmp0 to <2 x i32> addrspace(1)*
  store <2 x i32> %res00, <2 x i32> addrspace(1)* %addr0, align 8

;
; case 2 u16_m16k8 transpose
; CHECK:  lsc_load_block2d.ugm (M1, 1)  {{.*}}:d16.2x4x16nt  flat[base,widthm1,heightm1,pitchm1,x,y]
; CHECK:  mov (M1_NM, 8) [[T3:.*]](0,0)<1> [[T1:.*]](0,0)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3:.*]](0,8)<1> [[T1:.*]](0,1)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](1,0)<1>    [[T1]](0,2)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](1,8)<1>    [[T1]](0,3)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](2,0)<1>    [[T1]](2,0)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](2,8)<1>    [[T1]](2,1)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](3,0)<1>    [[T1]](2,2)<4;1,0>
; CHECK:  mov (M1_NM, 8) [[T3]](3,8)<1>    [[T1]](2,3)<4;1,0>
;
  %res1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 16, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %tmp1 = add i64 %tmp, 128
  %res10 = bitcast <8 x i16> %res1 to <4 x i32>
  %addr1 = inttoptr i64 %tmp1 to <4 x i32> addrspace(1)*
  store <4 x i32> %res10, <4 x i32> addrspace(1)* %addr1, align 8

; CHECK: ret
  ret void
}

declare <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

!igc.functions = !{!3}
!IGCMetadata = !{!16}

!3 = !{void (i16 addrspace(1)*, i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_emu_transpose, !4}
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
!132 = !{!"FuncMDMap[0]", void (i16 addrspace(1)*, i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_emu_transpose}
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
