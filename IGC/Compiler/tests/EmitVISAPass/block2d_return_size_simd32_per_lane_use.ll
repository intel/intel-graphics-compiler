;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; A 2d block read is sized from its block geometry, but the result is still an
; ordinary per-lane value to every other consumer.  A 1x16 d32 transposed block
; is one GRF, and at SIMD32 that is only half of what a per-lane use needs.
; This is what an SG16 joint matrix builtin inlined into a SIMD32 kernel looks
; like.  The declare must cover the dispatch width, otherwise the SIMD32 store
; below reads a GRF past the end of it - and because
; G4_InstSend::computeRightBound clamps a send operand to the declare size,
; neither RA nor SWSB would model that extra GRF.

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s \
; RUN:   -simd-mode 32 -regkey DumpVISAASMToConsole | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_block2d_simd32_per_lane_use(ptr addrspace(1) align 4 %dst, i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %ibase = ptrtoint ptr addrspace(1) %dst to i64
  %lid = zext i16 %localIdX to i64
  %off = shl i64 %lid, 2
  %iaddr = add i64 %ibase, %off
  %addr = inttoptr i64 %iaddr to ptr addrspace(1)

; The block is 1x16 d32 == 64B == one GRF, but a SIMD32 d32 payload is two, so
; the declare has to be num_elts=32 and not the num_elts=16 the block implies.
; CHECK: .decl res v_type=G type=d num_elts=32 align=wordx32
  %res = call i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64 %base, i32 %widthm1, i32 %heightm1, i32 %pitchm1, i32 %x, i32 %y, i32 32, i32 1, i32 16, i32 1, i1 true, i1 false, i32 0)

; CHECK: lsc_load_block2d.ugm (M1, 1) res:d32.1x16tn
; CHECK: lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 res:d32
  store i32 %res, ptr addrspace(1) %addr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.LSC2DBlockRead.i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

!igc.functions = !{!3}
!IGCMetadata = !{!16}

!3 = !{void (ptr addrspace(1), i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_block2d_simd32_per_lane_use, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!16 = !{!"ModuleMD", !131}
!131 = !{!"FuncMD", !132, !133}
!132 = !{!"FuncMDMap[0]", void (ptr addrspace(1), i64, i32, i32, i32, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test_block2d_simd32_per_lane_use}
!133 = !{!"FuncMDValue[0]", !166, !260, !261}
!166 = !{!"resAllocMD", !170}
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
!245 = !{!"argId", i32 0}
!246 = !{!"implicitArgInfoListVec[0]", !245}
!247 = !{!"argId", i32 1}
!248 = !{!"implicitArgInfoListVec[1]", !247}
!249 = !{!"argId", i32 7}
!250 = !{!"implicitArgInfoListVec[2]", !249}
!251 = !{!"argId", i32 8}
!252 = !{!"implicitArgInfoListVec[3]", !251}
!253 = !{!"argId", i32 9}
!254 = !{!"implicitArgInfoListVec[4]", !253}
!255 = !{!"argId", i32 10}
!256 = !{!"implicitArgInfoListVec[5]", !255}
!257 = !{!"argId", i32 15}
!258 = !{!"explicitArgNum", i32 0}
!259 = !{!"implicitArgInfoListVec[6]", !257, !258}
!260 = !{!"implicitArgInfoList", !246, !248, !250, !252, !254, !256, !259}
!261 = !{!"requiredSubGroupSize", i32 32}
