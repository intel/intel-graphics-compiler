;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole -regkey AddVISADumpDeclarationsToEnd < %s | FileCheck %s
; UNSUPPORTED: sys32

; Verify that scalar atomics return-value reconstruction for BF16 atomic add
; uses BF-typed operands (not W) in the add instructions.

; Verify the sequence after the scalar atomic:
;
; 1. SIMD1 scalar atomic bfloat add (all lanes collapsed into one).
; CHECK:     lsc_atomic_bfadd.ugm (M1_NM, 1) [[RET:.*]]:d16u32
;
; 2. Add atomic return value (old memory value) into every lane of the prefix sum.
; CHECK:     add (M1_NM, {{[0-9]+}}) [[PREFIXSUM:.*]](0,0)<1> [[PREFIXSUM]](0,0)<1;1,0> [[RET]](0,0)<0;1,0>
;
; 3. Subtract each lane's own source to get per-lane "old value before my add".
;    Both dst and (-)src must be bf-typed (not w/uw) for legal BF mixed mode.
; CHECK:     add (M1, {{[0-9]+}}) [[DST:.*]](0,0)<1> [[PREFIXSUM]](0,0)<1;1,0> (-)[[SRC:.*]](0,0)<1;1,0>
; CHECK-DAG: .decl [[DST]] v_type=G type=bf
; CHECK-DAG: .decl [[SRC]] v_type=G type=bf

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Kernel: atomic_fadd_bf16_with_return
;   %object:  uniform pointer - target of atomic fadd
;   %values:  pointer to per-lane bfloat values (non-uniform source)
;   %results: pointer to store per-lane return values
define spir_kernel void @atomic_fadd_bf16_with_return(
    i16 addrspace(1)* %object,
    i16 addrspace(1)* %values,
    i16 addrspace(1)* %results,
    <8 x i32> %r0,
    <8 x i32> %payloadHeader,
    i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i64
  %val_base = ptrtoint i16 addrspace(1)* %values to i64
  %offset = shl nuw nsw i64 %lid, 1
  %val_addr = add i64 %val_base, %offset
  %val_ptr = inttoptr i64 %val_addr to i16 addrspace(1)*

  %src_val = load i16, i16 addrspace(1)* %val_ptr, align 2

  ; atomic_op = 44 = EATOMIC_FADDBF16
  %old_val = call i16 @llvm.genx.GenISA.intatomicrawA64.i16.p1i16.p1i16(
      i16 addrspace(1)* %object,
      i16 addrspace(1)* %object,
      i16 %src_val,
      i32 44)

  %res_base = ptrtoint i16 addrspace(1)* %results to i64
  %res_addr = add i64 %res_base, %offset
  %res_ptr = inttoptr i64 %res_addr to i16 addrspace(1)*
  store i16 %old_val, i16 addrspace(1)* %res_ptr, align 2

  ret void
}

declare i16 @llvm.genx.GenISA.intatomicrawA64.i16.p1i16.p1i16(i16 addrspace(1)*, i16 addrspace(1)*, i16, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!30}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i16 addrspace(1)*, i16 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i16)* @atomic_fadd_bf16_with_return}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !12, !13, !14}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !7, !8, !9}
!12 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!13 = !{!"argAllocMDListVec[4]", !7, !8, !9}
!14 = !{!"argAllocMDListVec[5]", !7, !8, !9}

!30 = !{void (i16 addrspace(1)*, i16 addrspace(1)*, i16 addrspace(1)*, <8 x i32>, <8 x i32>, i16)* @atomic_fadd_bf16_with_return, !31}
!31 = !{!32, !33}
!32 = !{!"function_type", i32 0}
!33 = !{!"implicit_arg_desc", !34, !35, !36}
!34 = !{i32 0}
!35 = !{i32 1}
!36 = !{i32 8}
