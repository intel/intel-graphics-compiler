;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; This test doesn't contain vla (variable length arrays).
; It checks if we remove stack overflow detection subroutines in generated vISA.
;
; This .ll is based on a simplified output of this .cl code:
;
; kernel void test_simple(global uint* out, global uint *sidemem, uint n) {
;   sidemem[0] = n;
;   for (uint i = 1; i < n; i++) {
;     sidemem[n] = i*n - sidemem[n - 1];
;   }
;
;   for (uint i = 0; i < n; i++) {
;     uint load_from = sidemem[i] % n;
;     out[i] = sidemem[load_from];
;   }
; }

; REQUIRES: llvm-spirv, regkeys, pvc-supported
;
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options " -igc_opts 'DumpVISAASMToConsole=1,StackOverflowDetection=1'" -device pvc | FileCheck %s --check-prefix=CHECK-VISA

; CHECK-VISA: .kernel "test_simple"
; CHECK-VISA-NOT: call (M1, {{(8)|(16)}}) __stackoverflow_init{{.*}}
; CHECK-VISA-NOT: call (M1, {{(8)|(16)}}) __stackoverflow_detection{{.*}}


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test_simple(i32 addrspace(1)* %out, i32 addrspace(1)* %sidemem, i32 %n) #0 {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %sidemem, i64 0
  store i32 %n, i32 addrspace(1)* %arrayidx, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 1, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i32 %i.0, %n
  %sub = sub i32 %n, 1
  %idxprom = zext i32 %sub to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %sidemem, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %sub2 = sub i32 %mul, %0
  %idxprom3 = zext i32 %n to i64
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %sidemem, i64 %idxprom3
  store i32 %sub2, i32 addrspace(1)* %arrayidx4, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc16, %for.end
  %i5.0 = phi i32 [ 0, %for.end ], [ %inc17, %for.inc16 ]
  %cmp7 = icmp ult i32 %i5.0, %n
  br i1 %cmp7, label %for.body9, label %for.cond.cleanup8

for.cond.cleanup8:                                ; preds = %for.cond6
  br label %for.end18

for.body9:                                        ; preds = %for.cond6
  %idxprom10 = zext i32 %i5.0 to i64
  %arrayidx11 = getelementptr inbounds i32, i32 addrspace(1)* %sidemem, i64 %idxprom10
  %1 = load i32, i32 addrspace(1)* %arrayidx11, align 4
  %rem = urem i32 %1, %n
  %idxprom12 = zext i32 %rem to i64
  %arrayidx13 = getelementptr inbounds i32, i32 addrspace(1)* %sidemem, i64 %idxprom12
  %2 = load i32, i32 addrspace(1)* %arrayidx13, align 4
  %idxprom14 = zext i32 %i5.0 to i64
  %arrayidx15 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom14
  store i32 %2, i32 addrspace(1)* %arrayidx15, align 4
  br label %for.inc16

for.inc16:                                        ; preds = %for.body9
  %inc17 = add i32 %i5.0, 1
  br label %for.cond6

for.end18:                                        ; preds = %for.cond.cleanup8
  ret void
}

attributes #0 = { nounwind }
