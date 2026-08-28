;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -simd-mode 32 -regkey DumpVISAASMToConsole,VectorAlias=1 | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; VariableReuseAnalysis aliases the vec3 predicated load result into the padded
; vec4 store payload and drops the re-pack copies, so the load has to keep
; writing that payload. The merge value must not take the destination over: the
; store reads back the very register the load wrote.

; CHECK: ({{P[0-9]+}}) lsc_load{{.*}} [[PAYLOAD:[^ ]+]]:d64x2
; CHECK: lsc_store{{.*}} [[PAYLOAD]]:d64x2

define spir_kernel void @test_kernel(ptr addrspace(1) %input, ptr addrspace(1) %def, ptr addrspace(1) %predicate, ptr addrspace(1) %output, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %tid = zext i16 %localIdX to i64
  %offset = shl i64 %tid, 5

  %input.base = ptrtoint ptr addrspace(1) %input to i64
  %input.off = add i64 %offset, %input.base
  %input.addr = inttoptr i64 %input.off to ptr addrspace(1)

  %def.base = ptrtoint ptr addrspace(1) %def to i64
  %def.off = add i64 %offset, %def.base
  %def.addr = inttoptr i64 %def.off to ptr addrspace(1)

  %pred.base = ptrtoint ptr addrspace(1) %predicate to i64
  %pred.off = add i64 %tid, %pred.base
  %pred.addr = inttoptr i64 %pred.off to ptr addrspace(1)

  %output.base = ptrtoint ptr addrspace(1) %output to i64
  %output.off = add i64 %offset, %output.base
  %output.addr = inttoptr i64 %output.off to ptr addrspace(1)

  %pred.byte = load i8, ptr addrspace(1) %pred.addr, align 1
  %pred.bit = icmp ne i8 %pred.byte, 0

  ; merge value, a vec3 unpacked from the padded vec4 load
  %def.vec4 = load <4 x i64>, ptr addrspace(1) %def.addr, align 32
  %def.0 = extractelement <4 x i64> %def.vec4, i32 0
  %merge.0 = insertelement <3 x i64> undef, i64 %def.0, i32 0
  %def.1 = extractelement <4 x i64> %def.vec4, i32 1
  %merge.1 = insertelement <3 x i64> %merge.0, i64 %def.1, i32 1
  %def.2 = extractelement <4 x i64> %def.vec4, i32 2
  %merge.2 = insertelement <3 x i64> %merge.1, i64 %def.2, i32 2

  %loaded = call <3 x i64> @llvm.genx.GenISA.PredicatedLoad.v3i64.p1.v3i64(ptr addrspace(1) %input.addr, i64 32, i1 %pred.bit, <3 x i64> %merge.2)

  ; the vec3 result is re-packed into the padded vec4 store payload
  %loaded.0 = extractelement <3 x i64> %loaded, i32 0
  %payload.0 = insertelement <4 x i64> undef, i64 %loaded.0, i32 0
  %loaded.1 = extractelement <3 x i64> %loaded, i32 1
  %payload.1 = insertelement <4 x i64> %payload.0, i64 %loaded.1, i32 1
  %loaded.2 = extractelement <3 x i64> %loaded, i32 2
  %payload.2 = insertelement <4 x i64> %payload.1, i64 %loaded.2, i32 2
  %payload = insertelement <4 x i64> %payload.2, i64 undef, i64 3

  store <4 x i64> %payload, ptr addrspace(1) %output.addr, align 32
  ret void
}

declare <3 x i64> @llvm.genx.GenISA.PredicatedLoad.v3i64.p1.v3i64(ptr addrspace(1), i64, i1, <3 x i64>)

!IGCMetadata = !{!0}
!igc.functions = !{!14}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test_kernel}
!3 = !{!"FuncMDValue[0]", !4, !10}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !9, !9, !9}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !8}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"argAllocMDListVec[1]", !7, !8, !8}
!10 = !{!"implicitArgInfoList", !11, !12, !13}
!11 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!12 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!13 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!14 = !{ptr @test_kernel, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
