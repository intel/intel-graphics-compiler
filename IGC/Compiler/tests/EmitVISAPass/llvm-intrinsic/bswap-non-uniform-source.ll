;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole %s | FileCheck %s

; Checks that a 64-bit byte swap on a non-uniform (per-work-item) source is
; emitted correctly at SIMD16, verifying correct region offsets.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @bswap64_simd16(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
  %lid = zext i16 %localIdX to i64
  %off = shl i64 %lid, 3
  %srcbase = ptrtoint ptr addrspace(1) %src to i64
  %srcaddr = add i64 %srcbase, %off
  %srcptr = inttoptr i64 %srcaddr to ptr addrspace(1)
  %val = load i64, ptr addrspace(1) %srcptr, align 8
  ; CHECK: lsc_load.ugm (M1, 16)  [[VAL:[A-z0-9]+]]:d64  flat[[[SRCADDR:[A-z0-9_]+]]]:a64
  %bswap = call i64 @llvm.bswap.i64(i64 %val)
  ; CHECK-DAG: mov (M1, 8) [[V1:[A-z0-9]+]](0,3)<4> [[VAL]]_{{.*}}(0,0)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V1]](0,35)<4> [[VAL]]_{{.*}}(1,0)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V1]](0,2)<4> [[VAL]]_{{.*}}(0,1)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V1]](0,34)<4> [[VAL]]_{{.*}}(1,1)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V1]](0,1)<4> [[VAL]]_{{.*}}(0,2)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V1]](0,33)<4> [[VAL]]_{{.*}}(1,2)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V1]](0,0)<4> [[VAL]]_{{.*}}(0,3)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V1]](0,32)<4> [[VAL]]_{{.*}}(1,3)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V2:[A-z0-9]+]](0,3)<4> [[VAL]]_{{.*}}(0,4)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V2]](0,35)<4> [[VAL]]_{{.*}}(1,4)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V2]](0,2)<4> [[VAL]]_{{.*}}(0,5)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V2]](0,34)<4> [[VAL]]_{{.*}}(1,5)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V2]](0,1)<4> [[VAL]]_{{.*}}(0,6)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V2]](0,33)<4> [[VAL]]_{{.*}}(1,6)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[V2]](0,0)<4> [[VAL]]_{{.*}}(0,7)<8;1,0>
  ; CHECK-DAG: mov (M3, 8) [[V2]](0,32)<4> [[VAL]]_{{.*}}(1,7)<8;1,0>
  ; CHECK-DAG: mov (M1, 8) [[BSWAP:[A-z0-9]+]]_{{.*}}(0,0)<2> [[V3:[A-z0-9]+]](0,0)<1;1,0>
  ; CHECK-DAG: mov (M3, 8) [[BSWAP]]_{{.*}}(1,0)<2> [[V3]](0,8)<1;1,0>
  ; CHECK-DAG: mov (M1, 8) [[BSWAP]]_{{.*}}(0,1)<2> [[V4:[A-z0-9]+]](0,0)<1;1,0>
  ; CHECK-DAG: mov (M3, 8) [[BSWAP]]_{{.*}}(1,1)<2> [[V4]](0,8)<1;1,0>
  %dstbase = ptrtoint ptr addrspace(1) %dst to i64
  %dstaddr = add i64 %dstbase, %off
  %dstptr = inttoptr i64 %dstaddr to ptr addrspace(1)
  store i64 %bswap, ptr addrspace(1) %dstptr, align 8
  ; CHECK: lsc_store.ugm (M1, 16)  flat[[[DSTADDR:[A-z0-9_]+]]]:a64  [[BSWAP]]:d64
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn readnone
declare i64 @llvm.bswap.i64(i64) #1

attributes #0 = { convergent nounwind }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn readnone }

!igc.functions = !{!3}
!IGCMetadata = !{!6}

!3 = !{ptr @bswap64_simd16, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{!"ModuleMD", !153}
!153 = !{!"FuncMD", !154, !155}
!154 = !{!"FuncMDMap[0]", ptr @bswap64_simd16}
!155 = !{!"FuncMDValue[0]", !156, !157, !180}
!156 = !{!"functionType", !"KernelFunction"}
!157 = !{!"resAllocMD", !158}
!158 = !{!"argAllocMDList", !159, !163, !164, !165, !166, !167, !168, !169, !170}
!159 = !{!"argAllocMDListVec[0]", !160, !161, !162}
!160 = !{!"type", i32 0}
!161 = !{!"extensionType", i32 -1}
!162 = !{!"indexType", i32 -1}
!163 = !{!"argAllocMDListVec[1]", !160, !161, !162}
!164 = !{!"argAllocMDListVec[2]", !160, !161, !162}
!165 = !{!"argAllocMDListVec[3]", !160, !161, !162}
!166 = !{!"argAllocMDListVec[4]", !160, !161, !162}
!167 = !{!"argAllocMDListVec[5]", !160, !161, !162}
!168 = !{!"argAllocMDListVec[6]", !160, !161, !162}
!169 = !{!"argAllocMDListVec[7]", !160, !161, !162}
!170 = !{!"argAllocMDListVec[8]", !160, !161, !162}
!180 = !{!"implicitArgInfoList", !181, !183, !185, !187, !189, !191, !193}
!181 = !{!"implicitArgInfoListVec[0]", !182}
!182 = !{!"argId", i32 0}
!183 = !{!"implicitArgInfoListVec[1]", !184}
!184 = !{!"argId", i32 1}
!185 = !{!"implicitArgInfoListVec[2]", !186}
!186 = !{!"argId", i32 7}
!187 = !{!"implicitArgInfoListVec[3]", !188}
!188 = !{!"argId", i32 8}
!189 = !{!"implicitArgInfoListVec[4]", !190}
!190 = !{!"argId", i32 9}
!191 = !{!"implicitArgInfoListVec[5]", !192}
!192 = !{!"argId", i32 10}
!193 = !{!"implicitArgInfoListVec[6]", !194, !195}
!194 = !{!"argId", i32 15}
!195 = !{!"explicitArgNum", i32 0}
