;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPG
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPC
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe2 -vc-grf-size=64 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-Xe2-64
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe2 -vc-grf-size=128 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-Xe2-128
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe2 -vc-grf-size=256 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-Xe2-256

; CHECK-LABEL: @test1
define dllexport void @test1(<16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG-NOT: fpext
; CHECK-XeHPC-NOT: fpext
; CHECK-Xe2-64-NOT: fpext
; CHECK-Xe2-128-NOT: fpext
; CHECK-Xe2-256-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret void
}

; CHECK-LABEL: @test2
define dllexport <1024 x i32> @test2(<1024 x i32> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG: fpext
; CHECK-XeHPC-NOT: fpext
; CHECK-Xe2-64: fpext
; CHECK-Xe2-128-NOT: fpext
; CHECK-Xe2-256-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <1024 x i32> %pressure
}

; CHECK-LABEL: @test3
define dllexport <2048 x i32> @test3(<2048 x i32> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG: fpext
; CHECK-XeHPC: fpext
; CHECK-Xe2-64: fpext
; CHECK-Xe2-128: fpext
; CHECK-Xe2-256-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <2048 x i32> %pressure
}

; CHECK-LABEL: @test4
define dllexport <4096 x i32> @test4(<4096 x i32> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG: fpext
; CHECK-XeHPC: fpext
; CHECK-Xe2-64: fpext
; CHECK-Xe2-128: fpext
; CHECK-Xe2-256: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <4096 x i32> %pressure
}

; COM: Register pressure is beyond threshold for all platforms
; COM: Check that instructions smaller than a one register are not moved into the loop
; CHECK-LABEL: @test5
define dllexport <4096 x i32> @test5(<4096 x i32> %pressure, <4 x half> %arg.4, <8 x half> %arg.8) #0 {
entry:
  %fp.4 = fpext <4 x half> %arg.4 to <4 x float>
  %fp.8 = fpext <8 x half> %arg.8 to <8 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res.4 = phi <4 x float> [ zeroinitializer, %entry ], [ %res.next.4, %loop ]
  %res.8 = phi <8 x float> [ zeroinitializer, %entry ], [ %res.next.8, %loop ]
; CHECK-XeHPG-NOT: fpext <4 x half>
; CHECK-XeHPG: fpext <8 x half>
; CHECK-XeHPC-NOT: fpext
; CHECK-Xe2-64-NOT: fpext
; CHECK-Xe2-128-NOT: fpext
; CHECK-Xe2-256-NOT: fpext
  %res.next.4 = fadd <4 x float> %res.4, %fp.4
  %res.next.8 = fadd <8 x float> %res.8, %fp.8
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <4096 x i32> %pressure
}

attributes #0 = { "CMGenxMain" }

