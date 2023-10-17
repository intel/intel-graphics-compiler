;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; Function Attrs: nofree nosync nounwind readnone
declare i32 @llvm.genx.lane.id() #0

; Function Attrs: nofree nosync nounwind readnone
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v12i32.i16(<12 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nofree nosync nounwind readnone
declare <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.i16.i1(<1 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nofree nosync nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind
declare <8 x i32> @llvm.genx.vload.v8i32.p0v8i32(<8 x i32>*) #1

; Function Attrs: nounwind
declare void @llvm.genx.vstore.v8i32.p0v8i32(<8 x i32>, <8 x i32>*) #1

; Function Attrs: noinline nounwind
define internal spir_func void @foo(i64 %BUFF, <8 x i32>* %v, <12 x i32> %a) #4 {
entry:
  %0 = call i32 @llvm.genx.lane.id()
  %conv = trunc i32 %0 to i16
  %1 = mul i16 %conv, 4
  %rdr = call <1 x i32> @llvm.genx.rdregioni.v1i32.v12i32.i16(<12 x i32> %a, i32 0, i32 1, i32 0, i16 %1, i32 undef)
  %ar0.0.init = extractelement <1 x i32> %rdr, i64 0
  %add = add i32 %0, 1
  %conv1 = trunc i32 %add to i16
  %2 = mul i16 %conv1, 4
  %rdr2 = call <1 x i32> @llvm.genx.rdregioni.v1i32.v12i32.i16(<12 x i32> %a, i32 0, i32 1, i32 0, i16 %2, i32 undef)
  %ar0.0.end = extractelement <1 x i32> %rdr2, i64 0
  %cmp = icmp ult i32 %ar0.0.init, %ar0.0.end
  br i1 %cmp, label %do.body, label %if.end

do.body:
  %ar0.0 = phi i32 [ %ar0.0.init, %entry ], [ %add, %do.body ]
  %data = phi i32 [ undef, %entry ], [ %res, %do.body ]
  %conv.i = sext i32 %ar0.0 to i64
  %mul.i = mul i64 %conv.i, 4
  %add.i = add i64 %BUFF, %mul.i
  %i2ptr.i = inttoptr i64 %add.i to i32 addrspace(1)*
  %ld = load i32, i32 addrspace(1)* %i2ptr.i, align 4
  %data.v = insertelement <1 x i32> undef, i32 %data, i64 0
  %ld.v = insertelement <1 x i32> undef, i32 %ld, i64 0

  ; CHECK: [[LD:%[A-z0-9.]*]] = call <8 x i32> @llvm.masked.gather.v8i32.v8p1i32(<8 x i32 addrspace(1)*> {{.*}}, i32 4, <8 x i1> {{.*}}, <8 x i32> undef)
  ; CHECK-NEXT: call <8 x i32> @llvm.genx.wrregioni.v8i32.v8i32.i16.i1(<8 x i32> {{.*}}, <8 x i32> [[LD]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NOT: call <1 x i32> @llvm.genx.wrregioni.v1i32.v8i32.i16.i1(<1 x i32> undef, <8 x i32> [[LD]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %res.v = call <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.i16.i1(<1 x i32> %data.v, <1 x i32> %ld.v, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %res = extractelement <1 x i32> %res.v, i64 0
  %ref.load = call <8 x i32> @llvm.genx.vload.v8i32.p0v8i32(<8 x i32>* %v)
  %res.v.up = insertelement <1 x i32> undef, i32 %res, i64 0

  ; CHECK: call <8 x i32> @llvm.genx.wrregioni.v8i32.v8i32.v8i16.v8i1(<8 x i32> {{.*}}, <8 x i32> %{{.*}}, i32 0, i32 1
  ; CHECK-NOT: call <8 x i32> @llvm.genx.wrregionf.v8f32.v8f32.v8i16.v8i1(<8 x i32> {{.*}}, <8 x i32> %{{.*}}, i32 0, i32 8
  %wrregion = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %ref.load, <1 x i32> %res.v.up, i32 0, i32 1, i32 0, i16 %1, i32 undef, i1 true)
  call void @llvm.genx.vstore.v8i32.p0v8i32(<8 x i32> %wrregion, <8 x i32>* %v)
  %add.new = add i32 %ar0.0, 1
  %cmp.i = icmp ult i32 %add.new, %ar0.0.end
  br i1 %cmp.i, label %do.body, label %if.end

if.end:
  ret void
}

attributes #0 = { nofree nosync nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { nofree nounwind readonly }
attributes #4 = { noinline nounwind "CMGenxSIMT"="8" }
