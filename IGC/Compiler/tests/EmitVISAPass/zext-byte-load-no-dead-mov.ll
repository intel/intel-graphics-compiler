;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt -opaque-pointers -platformPtl -igc-emit-visa -simd-mode 32 -regkey DumpVISAASMToConsole -S %s | FileCheck %s
;
; Verify that when a byte load's only uses are zext-to-i32 and byte
; stores, the dead byte-extraction mov (stride <4;1,0>) is eliminated.
; EmitZExtByteLoad remaps the zext to use the d8u32 gatherDst directly,
; and emitLSCVectorStore_subDW uses m_SubDWLoadWideDst, so the narrow
; byte-typed destination is never read.
;
; CHECK-LABEL: .kernel "test_zext_byte_load"
; Expect the lsc_load with d8u32 NOT followed by a stride-4 mov.
; CHECK: lsc_load.ugm {{.*}}:d8u32
; CHECK-NOT: <4;1,0>
; CHECK: add

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_zext_byte_load(ptr addrspace(1) %buf, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) #0 {
entry:
  %lid = zext i16 %localIdX to i32
  %lid64 = zext i32 %lid to i64
  %buf_i = ptrtoint ptr addrspace(1) %buf to i64
  %src_addr = add i64 %buf_i, %lid64
  %src_ptr = inttoptr i64 %src_addr to ptr addrspace(1)

  ; Byte load
  %val = load i8, ptr addrspace(1) %src_ptr, align 1

  ; Use 1: zext to i32 (EmitZExtByteLoad path)
  %ext = zext i8 %val to i32

  ; Use 2: store byte (m_SubDWLoadWideDst path)
  %dst_off = add i64 %lid64, 256
  %dst_addr = add i64 %buf_i, %dst_off
  %dst_ptr = inttoptr i64 %dst_addr to ptr addrspace(1)
  store i8 %val, ptr addrspace(1) %dst_ptr, align 1

  ; Use the i32 result to prevent DCE
  %sum = add i32 %ext, %lid
  %out_off = shl i64 %lid64, 2
  %out_addr = add i64 %buf_i, %out_off
  %out_ptr = inttoptr i64 %out_addr to ptr addrspace(1)
  %out_ptr_i32 = bitcast ptr addrspace(1) %out_ptr to ptr addrspace(1)
  store i32 %sum, ptr addrspace(1) %out_ptr_i32, align 4
  ret void
}

attributes #0 = { nounwind null_pointer_is_valid }

!igc.functions = !{!0}
!IGCMetadata = !{!20}

!0 = !{ptr @test_zext_byte_load, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8, !9, !10, !11}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 7}
!7 = !{i32 8}
!8 = !{i32 9}
!9 = !{i32 10}
!10 = !{i32 11}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 0}

!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23}
!22 = !{!"FuncMDMap[0]", ptr @test_zext_byte_load}
!23 = !{!"FuncMDValue[0]", !24}
!24 = !{!"resAllocMD", !25}
!25 = !{!"argAllocMDList", !26, !27, !28, !29, !30, !31, !32, !33}
!26 = !{!"argAllocMDListVec[0]", !40, !41, !42}
!27 = !{!"argAllocMDListVec[1]", !40, !41, !42}
!28 = !{!"argAllocMDListVec[2]", !40, !41, !42}
!29 = !{!"argAllocMDListVec[3]", !40, !41, !42}
!30 = !{!"argAllocMDListVec[4]", !40, !41, !42}
!31 = !{!"argAllocMDListVec[5]", !40, !41, !42}
!32 = !{!"argAllocMDListVec[6]", !40, !41, !42}
!33 = !{!"argAllocMDListVec[7]", !40, !41, !42}
!40 = !{!"type", i32 0}
!41 = !{!"extensionType", i32 -1}
!42 = !{!"indexType", i32 -1}
