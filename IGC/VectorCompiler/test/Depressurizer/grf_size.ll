;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -vc-grf-size=32 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPG-32
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -vc-grf-size=64 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPG-64
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -vc-grf-size=128 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPG-128
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -vc-grf-size=256 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPG-256
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -vc-auto-large-grf -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-XeHPC

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
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64-NOT: fpext
; CHECK-XeHPG-128-NOT: fpext
; CHECK-XeHPG-256-NOT: fpext
; CHECK-XeHPC-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret void
}

; CHECK-LABEL: @test2
define dllexport <512 x i8> @test2(<512 x i8> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64: fpext
; CHECK-XeHPG-128-NOT: fpext
; CHECK-XeHPG-256-NOT: fpext
; CHECK-XeHPC-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <512 x i8> %pressure
}

; CHECK-LABEL: @test3
define dllexport <2560 x i8> @test3(<2560 x i8> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64: fpext
; CHECK-XeHPG-128: fpext
; CHECK-XeHPG-256-NOT: fpext
; CHECK-XeHPC-NOT: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <2560 x i8> %pressure
}

; CHECK-LABEL: @test4
define dllexport <5120 x i8> @test4(<5120 x i8> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64: fpext
; CHECK-XeHPG-128: fpext
; CHECK-XeHPG-256-NOT: fpext
; CHECK-XeHPC: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <5120 x i8> %pressure
}

; CHECK-LABEL: @test5
define dllexport <6656 x i8> @test5(<6656 x i8> %pressure, <16 x half> %arg) #0 {
entry:
  %fp = fpext <16 x half> %arg to <16 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res = phi <16 x float> [ zeroinitializer, %entry ], [ %res.next, %loop ]
  %i.next = add i32 %i, 1
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64: fpext
; CHECK-XeHPG-128: fpext
; CHECK-XeHPG-256: fpext
; CHECK-XeHPC: fpext
  %res.next = fadd <16 x float> %res, %fp
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <6656 x i8> %pressure
}

; COM: Register pressure is beyond threshold for all platforms
; COM: Check that instructions smaller than a one register are not moved into the loop
; CHECK-LABEL: @test6
define dllexport <6656 x i8> @test6(<6656 x i8> %pressure, <4 x half> %arg.4, <8 x half> %arg.8) #0 {
entry:
  %fp.4 = fpext <4 x half> %arg.4 to <4 x float>
  %fp.8 = fpext <8 x half> %arg.8 to <8 x float>
; CHECK: br label %loop
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %res.4 = phi <4 x float> [ zeroinitializer, %entry ], [ %res.next.4, %loop ]
  %res.8 = phi <8 x float> [ zeroinitializer, %entry ], [ %res.next.8, %loop ]
; CHECK-XeHPG-32: fpext
; CHECK-XeHPG-64: fpext
; CHECK-XeHPG-128: fpext
; CHECK-XeHPG-256: fpext
; CHECK-XeHPC-NOT: fpext
  %res.next.4 = fadd <4 x float> %res.4, %fp.4
  %res.next.8 = fadd <8 x float> %res.8, %fp.8
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, 100
  br i1 %cmp, label %loop, label %end

end:
  ret <6656 x i8> %pressure
}

attributes #0 = { "CMGenxMain" }

