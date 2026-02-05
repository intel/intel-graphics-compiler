;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt  --opaque-pointers -igc-runtimevalue-legalization-pass -S %s | FileCheck %s

target datalayout = "e-p0:32:32-p2:64:64"

define void @main(i32 %idx) #0 {
entry:
  ; verify that overlapping float scalar and int vector instructions are merged
  %0 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 0)
  %1 = extractelement <4 x i32> %0, i32 %idx
  call void @foo(i32 %1)
  ; CHECK: [[VALUE5:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 0)
  ; CHECK-NEXT: [[VALUE6:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE5]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE6]])
  %2 = call float @llvm.genx.GenISA.RuntimeValue.float(i32 1)
  call void @bar(float %2)
  ; CHECK: [[VALUE7:%[a-zA-Z0-9_.%-]+]] = call <4 x float> @llvm.genx.GenISA.RuntimeValue.v4f32(i32 0)
  ; CHECK-NEXT: [[VALUE8:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x float> [[VALUE7]], i32 1
  ; CHECK-NEXT: call void @bar(float [[VALUE8]])

  ; verify that overlapping i64 scalar and int vector instructions are merged
  %3 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  %4 = extractelement <4 x i32> %3, i32 %idx
  call void @foo(i32 %4)
  ; CHECK: [[VALUE9:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  ; CHECK-NEXT: [[VALUE10:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE9]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE10]])
  %5 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 4)
  call void @baz(i64 %5)
  ; CHECK: [[VALUE11:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  ; CHECK-NEXT: [[VALUE12:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE11]], i32 0
  ; CHECK-NEXT: [[VALUE13:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE12]], i32 0
  ; CHECK-NEXT: [[VALUE14:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE11]], i32 1
  ; CHECK-NEXT: [[VALUE15:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE13]], i32 [[VALUE14]], i32 1
  ; CHECK-NEXT: [[VALUE16:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE15]] to i64
  ; CHECK-NEXT: call void @baz(i64 [[VALUE16]])

  %6 = call ptr addrspace(2) @llvm.genx.GenISA.RuntimeValue.p2(i32 6)
  call void @bap2(ptr addrspace(2) %6)
  ; CHECK: [[VALUE17:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  ; CHECK-NEXT: [[VALUE18:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE17]], i32 2
  ; CHECK-NEXT: [[VALUE19:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE18]], i32 0
  ; CHECK-NEXT: [[VALUE20:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE17]], i32 3
  ; CHECK-NEXT: [[VALUE21:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE19]], i32 [[VALUE20]], i32 1
  ; CHECK-NEXT: [[VALUE22:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE21]] to i64
  ; CHECK-NEXT: [[VALUE23:%[a-zA-Z0-9_.%-]+]] = inttoptr i64 [[VALUE22]] to ptr addrspace(2)
  ; CHECK-NEXT: call void @bap2(ptr addrspace(2) [[VALUE23]])

  %7 = call ptr @llvm.genx.GenISA.RuntimeValue.p0(i32 1)
  call void @bap0(ptr %7)
  ; CHECK: [[VALUE24:%[a-zA-Z0-9_.%-]+]] = call <4 x ptr> @llvm.genx.GenISA.RuntimeValue.v4p0(i32 0)
  ; CHECK-NEXT: [[VALUE25:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x ptr> [[VALUE24]], i32 1
  ; CHECK-NEXT: call void @bap0(ptr [[VALUE25]])

  %8 = call <2 x i64> @llvm.genx.GenISA.RuntimeValue.v2i64(i32 8)
  %9 = extractelement <2 x i64> %8, i32 %idx
  call void @baz(i64 %9)
  ; CHECK: [[VALUE26:%[a-zA-Z0-9_.%-]+]] = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 8)
  ; CHECK-NEXT: [[VALUE27:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE26]], i32 0
  ; CHECK-NEXT: [[VALUE28:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE27]], i32 0
  ; CHECK-NEXT: [[VALUE29:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE26]], i32 1
  ; CHECK-NEXT: [[VALUE30:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE28]], i32 [[VALUE29]], i32 1
  ; CHECK-NEXT: [[VALUE31:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE30]] to i64
  ; CHECK-NEXT: [[VALUE32:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i64> undef, i64 [[VALUE31]], i32 0
  ; CHECK-NEXT: [[VALUE33:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE26]], i32 2
  ; CHECK-NEXT: [[VALUE34:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE33]], i32 0
  ; CHECK-NEXT: [[VALUE35:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE26]], i32 3
  ; CHECK-NEXT: [[VALUE36:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE34]], i32 [[VALUE35]], i32 1
  ; CHECK-NEXT: [[VALUE37:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE36]] to i64
  ; CHECK-NEXT: [[VALUE38:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i64> [[VALUE32]], i64 [[VALUE37]], i32 1
  ; CHECK-NEXT: [[VALUE39:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x i64> [[VALUE38]], i32 %idx
  ; CHECK-NEXT: call void @baz(i64 [[VALUE39]])

  %10 = call <2 x double> @llvm.genx.GenISA.RuntimeValue.v2f64(i32 10)
  %11 = extractelement <2 x double> %10, i32 %idx
  call void @baf(double %11)
  ; CHECK: [[VALUE40:%[a-zA-Z0-9_.%-]+]] = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 8)
  ; CHECK-NEXT: [[VALUE41:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE40]], i32 2
  ; CHECK-NEXT: [[VALUE42:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE41]], i32 0
  ; CHECK-NEXT: [[VALUE43:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE40]], i32 3
  ; CHECK-NEXT: [[VALUE44:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE42]], i32 [[VALUE43]], i32 1
  ; CHECK-NEXT: [[VALUE45:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE44]] to double
  ; CHECK-NEXT: [[VALUE46:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x double> undef, double [[VALUE45]], i32 0
  ; CHECK-NEXT: [[VALUE47:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE40]], i32 4
  ; CHECK-NEXT: [[VALUE48:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE47]], i32 0
  ; CHECK-NEXT: [[VALUE49:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE40]], i32 5
  ; CHECK-NEXT: [[VALUE50:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE48]], i32 [[VALUE49]], i32 1
  ; CHECK-NEXT: [[VALUE51:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE50]] to double
  ; CHECK-NEXT: [[VALUE52:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x double> [[VALUE46]], double [[VALUE51]], i32 1
  ; CHECK-NEXT: [[VALUE53:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x double> [[VALUE52]], i32 %idx
  ; CHECK-NEXT: call void @baf(double [[VALUE53]])

  %12 = call <2 x ptr addrspace(2)> @llvm.genx.GenISA.RuntimeValue.v2p2(i32 8)
  %13 = extractelement <2 x ptr addrspace(2)> %12, i32 %idx
  call void @bap2(ptr addrspace(2) %13)
  ; CHECK: [[VALUE54:%[a-zA-Z0-9_.%-]+]] = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 8)
  ; CHECK-NEXT: [[VALUE55:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE54]], i32 0
  ; CHECK-NEXT: [[VALUE56:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE55]], i32 0
  ; CHECK-NEXT: [[VALUE57:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE54]], i32 1
  ; CHECK-NEXT: [[VALUE58:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE56]], i32 [[VALUE57]], i32 1
  ; CHECK-NEXT: [[VALUE59:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE58]] to i64
  ; CHECK-NEXT: [[VALUE60:%[a-zA-Z0-9_.%-]+]] = inttoptr i64 [[VALUE59]] to ptr addrspace(2)
  ; CHECK-NEXT: [[VALUE61:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x ptr addrspace(2)> undef, ptr addrspace(2) [[VALUE60]], i32 0
  ; CHECK-NEXT: [[VALUE62:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE54]], i32 2
  ; CHECK-NEXT: [[VALUE63:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE62]], i32 0
  ; CHECK-NEXT: [[VALUE64:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE54]], i32 3
  ; CHECK-NEXT: [[VALUE65:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE63]], i32 [[VALUE64]], i32 1
  ; CHECK-NEXT: [[VALUE66:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE65]] to i64
  ; CHECK-NEXT: [[VALUE67:%[a-zA-Z0-9_.%-]+]] = inttoptr i64 [[VALUE66]] to ptr addrspace(2)
  ; CHECK-NEXT: [[VALUE68:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x ptr addrspace(2)> [[VALUE61]], ptr addrspace(2) [[VALUE67]], i32 1
  ; CHECK-NEXT: [[VALUE69:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x ptr addrspace(2)> [[VALUE68]], i32 %idx
  ; CHECK-NEXT: call void @bap2(ptr addrspace(2) [[VALUE69]])

  %14 = call <2 x float> @llvm.genx.GenISA.RuntimeValue.v2f32(i32 9)
  call void @bar2(<2 x float> %14)
  ; CHECK: [[VALUE70:%[a-zA-Z0-9_.%-]+]] = call <6 x float> @llvm.genx.GenISA.RuntimeValue.v6f32(i32 8)
  ; CHECK-NEXT: [[VALUE71:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x float> [[VALUE70]], i32 1
  ; CHECK-NEXT: [[VALUE72:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x float> undef, float [[VALUE71]], i32 0
  ; CHECK-NEXT: [[VALUE73:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x float> [[VALUE70]], i32 2
  ; CHECK-NEXT: [[VALUE74:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x float> [[VALUE72]], float [[VALUE73]], i32 1
  ; CHECK-NEXT: call void @bar2(<2 x float> [[VALUE74]])

  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #1

; Function Attrs: nounwind readnone
declare ptr @llvm.genx.GenISA.RuntimeValue.p0(i32) #1

; Function Attrs: nounwind readnone
declare ptr addrspace(2) @llvm.genx.GenISA.RuntimeValue.p2(i32) #1

; Function Attrs: nounwind readnone
declare float @llvm.genx.GenISA.RuntimeValue.float(i32) #1

; Function Attrs: nounwind readnone
declare <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32) #1

; Function Attrs: nounwind readnone
declare <2 x float> @llvm.genx.GenISA.RuntimeValue.v2f32(i32) #1

; Function Attrs: nounwind readnone
declare <2 x i64> @llvm.genx.GenISA.RuntimeValue.v2i64(i32) #1

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32) #1

; Function Attrs: nounwind readnone
declare <4 x float> @llvm.genx.GenISA.RuntimeValue.v4f32(i32) #1

; Function Attrs: noduplicate nounwind
declare void @foo(i32) #2

; Function Attrs: noduplicate nounwind
declare void @bar(float) #2

; Function Attrs: noduplicate nounwind
declare void @bar2(<2 x float>) #2

; Function Attrs: noduplicate nounwind
declare void @baz(i64) #2

; Function Attrs: noduplicate nounwind
declare void @bap0(ptr) #2

; Function Attrs: noduplicate nounwind
declare void @bap2(ptr addrspace(2)) #2

; Function Attrs: noduplicate nounwind
declare void @baf(double) #2

; Function Attrs: nounwind readnone
declare <2 x double> @llvm.genx.GenISA.RuntimeValue.v2f64(i32) #1

; Function Attrs: nounwind readnone
declare <2 x ptr addrspace(2)> @llvm.genx.GenISA.RuntimeValue.v2p2(i32) #1

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readnone }
attributes #2 = { noduplicate nounwind }

