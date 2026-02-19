;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-memopt -remove-red-blockreads | FileCheck %s


target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i64 %i64input, i32 addrspace(1)* %i32addr, i8 addrspace(1)* %i8addr) {
entry:
  %0 = inttoptr i64 %i64input to i32 addrspace(1)*
  %1 = inttoptr i64 %i64input to i8 addrspace(1)*
  %2 = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* %0)
  %3 = call i8 @llvm.genx.GenISA.simdBlockRead.i8.p1i8(i8 addrspace(1)* %1)
  store i32 %2, i32 addrspace(1)* %i32addr, align 4
  store i8 %3, i8 addrspace(1)* %i8addr, align 1
  ret void
}

; CHECK-LABEL: define void @f0
; CHECK: %0 = inttoptr i64 %i64input to i32 addrspace(1)*
; CHECK: %1 = inttoptr i64 %i64input to i8 addrspace(1)*
; CHECK: %2 = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* %0)
; CHECK: %3 = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: %4 = zext i16 %3 to i32
; CHECK: %5 = lshr i32 %4, 2
; CHECK: %6 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %5, i32 0)
; CHECK: %7 = and i32 %4, 3
; CHECK: %8 = mul i32 %7, 8
; CHECK: %9 = lshr i32 %6, %8
; CHECK: %10 = trunc i32 %9 to i8
; CHECK: store i32 %2, i32 addrspace(1)* %i32addr, align 4
; CHECK: store i8 %10, i8 addrspace(1)* %i8addr, align 1
; CHECK: ret void


define void @f1(i64 %i64input, i16 addrspace(1)* %i16addr, i8 addrspace(1)* %i8addr) {
entry:
  %0 = inttoptr i64 %i64input to i16 addrspace(1)*
  %1 = inttoptr i64 %i64input to i8 addrspace(1)*
  %2 = call i8 @llvm.genx.GenISA.simdBlockRead.i8.p1i8(i8 addrspace(1)* %1)
  %3 = call i16 @llvm.genx.GenISA.simdBlockRead.i16.p1i16(i16 addrspace(1)* %0)
  store i16 %3, i16 addrspace(1)* %i16addr, align 2
  store i8 %2, i8 addrspace(1)* %i8addr, align 1
  ret void
}

; CHECK-LABEL: define void @f1
; CHECK: %0 = inttoptr i64 %i64input to i16 addrspace(1)*
; CHECK: %1 = inttoptr i64 %i64input to i8 addrspace(1)*
; CHECK: %2 = bitcast i8 addrspace(1)* %1 to i16 addrspace(1)*
; CHECK: %3 = call i16 @llvm.genx.GenISA.simdBlockRead.i16.p1i16(i16 addrspace(1)* %2)
; CHECK: %4 = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: %5 = zext i16 %4 to i32
; CHECK: %6 = lshr i32 %5, 1
; CHECK: %7 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %3, i32 %6, i32 0)
; CHECK: %8 = and i32 %5, 1
; CHECK: %9 = mul i32 %8, 8
; CHECK: %10 = trunc i32 %9 to i16
; CHECK: %11 = lshr i16 %7, %10
; CHECK: %12 = trunc i16 %11 to i8
; CHECK: store i16 %3, i16 addrspace(1)* %i16addr, align 2
; CHECK: store i8 %12, i8 addrspace(1)* %i8addr, align 1
; CHECK: ret void

define void @f2(i64 %i64input, i64 addrspace(1)* %i64addr, i8 addrspace(1)* %i8addr) {
entry:
  %0 = inttoptr i64 %i64input to i64 addrspace(1)*
  %1 = inttoptr i64 %i64input to i8 addrspace(1)*
  %2 = call i8 @llvm.genx.GenISA.simdBlockRead.i8.p1i8(i8 addrspace(1)* %1)
  %3 = call i64 @llvm.genx.GenISA.simdBlockRead.i64.p1i64(i64 addrspace(1)* %0)
  store i64 %3, i64 addrspace(1)* %i64addr, align 8
  store i8 %2, i8 addrspace(1)* %i8addr, align 1
  ret void
}

; CHECK-LABEL: define void @f2
; CHECK: %0 = inttoptr i64 %i64input to i64 addrspace(1)*
; CHECK: %1 = inttoptr i64 %i64input to i8 addrspace(1)*
; CHECK: %2 = bitcast i8 addrspace(1)* %1 to i64 addrspace(1)*
; CHECK: %3 = call i64 @llvm.genx.GenISA.simdBlockRead.i64.p1i64(i64 addrspace(1)* %2)
; CHECK: %4 = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: %5 = zext i16 %4 to i32
; CHECK: %6 = lshr i32 %5, 4
; CHECK: %7 = bitcast i64 %3 to <2 x i32>
; CHECK: %8 = extractelement <2 x i32> %7, i32 0
; CHECK: %9 = extractelement <2 x i32> %7, i32 1
; CHECK: %10 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %8, i32 %6, i32 0)
; CHECK: %11 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %9, i32 %6, i32 0)
; CHECK: %12 = insertelement <2 x i32> undef, i32 %10, i64 0
; CHECK: %13 = insertelement <2 x i32> %12, i32 %11, i64 1
; CHECK: %14 = bitcast <2 x i32> %13 to i64
; CHECK: %15 = and i32 %5, 7
; CHECK: %16 = mul i32 %15, 8
; CHECK: %17 = zext i32 %16 to i64
; CHECK: %18 = lshr i64 %14, %17
; CHECK: %19 = trunc i64 %18 to i8
; CHECK: store i64 %3, i64 addrspace(1)* %i64addr, align 8
; CHECK: store i8 %19, i8 addrspace(1)* %i8addr, align 1
; CHECK: ret void

define void @f3(i64 %i64input, i16 addrspace(1)* %i16addr, i32 addrspace(1)* %i32addr) {
entry:
  %0 = inttoptr i64 %i64input to i16 addrspace(1)*
  %1 = inttoptr i64 %i64input to i32 addrspace(1)*
  %2 = call <8 x i16> @llvm.genx.GenISA.simdBlockRead.v8i16.p1i16(i16 addrspace(1)* %0)
  %3 = call <2 x i32> @llvm.genx.GenISA.simdBlockRead.v2i32.p1i32(i32 addrspace(1)* %1)
  %4 = extractelement <8 x i16> %2, i32 3
  %5 = extractelement <2 x i32> %3, i32 1
  store i16 %4, i16 addrspace(1)* %i16addr, align 2
  store i32 %5, i32 addrspace(1)* %i32addr, align 4
  ret void
}

; CHECK-LABEL: define void @f3
; CHECK: %0 = inttoptr i64 %i64input to i16 addrspace(1)*
; CHECK: %1 = inttoptr i64 %i64input to i32 addrspace(1)*
; CHECK: %2 = call <8 x i16> @llvm.genx.GenISA.simdBlockRead.v8i16.p1i16(i16 addrspace(1)* %0)
; CHECK: %3 = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: %4 = zext i16 %3 to i32
; CHECK: %5 = extractelement <8 x i16> %2, i32 0
; CHECK: %6 = extractelement <8 x i16> %2, i32 1
; CHECK: %7 = extractelement <8 x i16> %2, i32 2
; CHECK: %8 = extractelement <8 x i16> %2, i32 3
; CHECK: %9 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 0, i32 0)
; CHECK: %10 = insertelement <32 x i16> undef, i16 %9, i64 0
; CHECK: %11 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 1, i32 0)
; CHECK: %12 = insertelement <32 x i16> %10, i16 %11, i64 1
; CHECK: %13 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 2, i32 0)
; CHECK: %14 = insertelement <32 x i16> %12, i16 %13, i64 2
; CHECK: %15 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 3, i32 0)
; CHECK: %16 = insertelement <32 x i16> %14, i16 %15, i64 3
; CHECK: %17 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 4, i32 0)
; CHECK: %18 = insertelement <32 x i16> %16, i16 %17, i64 4
; CHECK: %19 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 5, i32 0)
; CHECK: %20 = insertelement <32 x i16> %18, i16 %19, i64 5
; CHECK: %21 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 6, i32 0)
; CHECK: %22 = insertelement <32 x i16> %20, i16 %21, i64 6
; CHECK: %23 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %5, i32 7, i32 0)
; CHECK: %24 = insertelement <32 x i16> %22, i16 %23, i64 7
; CHECK: %25 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 0, i32 0)
; CHECK: %26 = insertelement <32 x i16> %24, i16 %25, i64 8
; CHECK: %27 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 1, i32 0)
; CHECK: %28 = insertelement <32 x i16> %26, i16 %27, i64 9
; CHECK: %29 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 2, i32 0)
; CHECK: %30 = insertelement <32 x i16> %28, i16 %29, i64 10
; CHECK: %31 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 3, i32 0)
; CHECK: %32 = insertelement <32 x i16> %30, i16 %31, i64 11
; CHECK: %33 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 4, i32 0)
; CHECK: %34 = insertelement <32 x i16> %32, i16 %33, i64 12
; CHECK: %35 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 5, i32 0)
; CHECK: %36 = insertelement <32 x i16> %34, i16 %35, i64 13
; CHECK: %37 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 6, i32 0)
; CHECK: %38 = insertelement <32 x i16> %36, i16 %37, i64 14
; CHECK: %39 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %6, i32 7, i32 0)
; CHECK: %40 = insertelement <32 x i16> %38, i16 %39, i64 15
; CHECK: %41 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 0, i32 0)
; CHECK: %42 = insertelement <32 x i16> %40, i16 %41, i64 16
; CHECK: %43 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 1, i32 0)
; CHECK: %44 = insertelement <32 x i16> %42, i16 %43, i64 17
; CHECK: %45 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 2, i32 0)
; CHECK: %46 = insertelement <32 x i16> %44, i16 %45, i64 18
; CHECK: %47 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 3, i32 0)
; CHECK: %48 = insertelement <32 x i16> %46, i16 %47, i64 19
; CHECK: %49 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 4, i32 0)
; CHECK: %50 = insertelement <32 x i16> %48, i16 %49, i64 20
; CHECK: %51 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 5, i32 0)
; CHECK: %52 = insertelement <32 x i16> %50, i16 %51, i64 21
; CHECK: %53 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 6, i32 0)
; CHECK: %54 = insertelement <32 x i16> %52, i16 %53, i64 22
; CHECK: %55 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %7, i32 7, i32 0)
; CHECK: %56 = insertelement <32 x i16> %54, i16 %55, i64 23
; CHECK: %57 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 0, i32 0)
; CHECK: %58 = insertelement <32 x i16> %56, i16 %57, i64 24
; CHECK: %59 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 1, i32 0)
; CHECK: %60 = insertelement <32 x i16> %58, i16 %59, i64 25
; CHECK: %61 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 2, i32 0)
; CHECK: %62 = insertelement <32 x i16> %60, i16 %61, i64 26
; CHECK: %63 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 3, i32 0)
; CHECK: %64 = insertelement <32 x i16> %62, i16 %63, i64 27
; CHECK: %65 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 4, i32 0)
; CHECK: %66 = insertelement <32 x i16> %64, i16 %65, i64 28
; CHECK: %67 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 5, i32 0)
; CHECK: %68 = insertelement <32 x i16> %66, i16 %67, i64 29
; CHECK: %69 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 6, i32 0)
; CHECK: %70 = insertelement <32 x i16> %68, i16 %69, i64 30
; CHECK: %71 = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %8, i32 7, i32 0)
; CHECK: %72 = insertelement <32 x i16> %70, i16 %71, i64 31
; CHECK: %73 = mul i32 %4, 2
; CHECK: %74 = add i32 %73, 0
; CHECK: %75 = add i32 %74, 0
; CHECK: %76 = extractelement <32 x i16> %72, i32 %75
; CHECK: %77 = insertelement <2 x i16> undef, i16 %76, i64 0
; CHECK: %78 = add i32 %74, 1
; CHECK: %79 = extractelement <32 x i16> %72, i32 %78
; CHECK: %80 = insertelement <2 x i16> %77, i16 %79, i64 1
; CHECK: %81 = bitcast <2 x i16> %80 to i32
; CHECK: %82 = insertelement <2 x i32> undef, i32 %81, i64 0
; CHECK: %83 = add i32 %73, 16
; CHECK: %84 = add i32 %83, 0
; CHECK: %85 = extractelement <32 x i16> %72, i32 %84
; CHECK: %86 = insertelement <2 x i16> undef, i16 %85, i64 0
; CHECK: %87 = add i32 %83, 1
; CHECK: %88 = extractelement <32 x i16> %72, i32 %87
; CHECK: %89 = insertelement <2 x i16> %86, i16 %88, i64 1
; CHECK: %90 = bitcast <2 x i16> %89 to i32
; CHECK: %91 = insertelement <2 x i32> %82, i32 %90, i64 1
; CHECK: %92 = extractelement <8 x i16> %2, i32 3
; CHECK: %93 = extractelement <2 x i32> %91, i32 1
; CHECK: store i16 %92, i16 addrspace(1)* %i16addr, align 2
; CHECK: store i32 %93, i32 addrspace(1)* %i32addr, align 4
; CHECK: ret void

define void @f4(i64 %i64input, i32 addrspace(1)* %i32addr, i8 addrspace(1)* %i8addr) {
entry:
  %0 = inttoptr i64 %i64input to i32 addrspace(1)*
  %1 = inttoptr i64 %i64input to i8 addrspace(1)*
  %2 = call <2 x i32> @llvm.genx.GenISA.simdBlockRead.v2i32.p1i32(i32 addrspace(1)* %0)
  %3 = call <2 x i8> @llvm.genx.GenISA.simdBlockRead.v2i8.p1i8(i8 addrspace(1)* %1)
  %4 = extractelement <2 x i32> %2, i32 0
  %5 = extractelement <2 x i8> %3, i32 1
  store i32 %4, i32 addrspace(1)* %i32addr, align 4
  store i8 %5, i8 addrspace(1)* %i8addr, align 1
  ret void
}

; CHECK-LABEL: define void @f4
; CHECK: %0 = inttoptr i64 %i64input to i32 addrspace(1)*
; CHECK: %1 = inttoptr i64 %i64input to i8 addrspace(1)*
; CHECK: %2 = call <2 x i32> @llvm.genx.GenISA.simdBlockRead.v2i32.p1i32(i32 addrspace(1)* %0)
; CHECK: %3 = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: %4 = zext i16 %3 to i32
; CHECK: %5 = lshr i32 %4, 2
; CHECK: %6 = extractelement <2 x i32> %2, i32 0
; CHECK: %7 = add i32 %5, 0
; CHECK: %8 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %6, i32 %7, i32 0)
; CHECK: %9 = add i32 %5, 2
; CHECK: %10 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %6, i32 %9, i32 0)
; CHECK: %11 = bitcast i32 %8 to <4 x i8>
; CHECK: %12 = and i32 %4, 3
; CHECK: %13 = extractelement <4 x i8> %11, i32 %12
; CHECK: %14 = insertelement <2 x i8> undef, i8 %13, i32 0
; CHECK: %15 = bitcast i32 %10 to <4 x i8>
; CHECK: %16 = and i32 %4, 3
; CHECK: %17 = extractelement <4 x i8> %15, i32 %16
; CHECK: %18 = insertelement <2 x i8> %14, i8 %17, i32 1
; CHECK: %19 = extractelement <2 x i32> %2, i32 0
; CHECK: %20 = extractelement <2 x i8> %18, i32 1
; CHECK: store i32 %19, i32 addrspace(1)* %i32addr, align 4
; CHECK: store i8 %20, i8 addrspace(1)* %i8addr, align 1
; CHECK: ret void

declare i64 @llvm.genx.GenISA.simdBlockRead.i64.p1i64(i64 addrspace(1)*)
declare i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)*)
declare <2 x i32> @llvm.genx.GenISA.simdBlockRead.v2i32.p1i32(i32 addrspace(1)*)
declare i16 @llvm.genx.GenISA.simdBlockRead.i16.p1i16(i16 addrspace(1)*)
declare <8 x i16> @llvm.genx.GenISA.simdBlockRead.v8i16.p1i16(i16 addrspace(1)*)
declare i8 @llvm.genx.GenISA.simdBlockRead.i8.p1i8(i8 addrspace(1)*)
declare <2 x i8> @llvm.genx.GenISA.simdBlockRead.v2i8.p1i8(i8 addrspace(1)*)

!igc.functions = !{!0, !4, !5, !6, !7}

!0 = !{void (i64, i32 addrspace(1)*, i8 addrspace(1)*)* @f0, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
!4 = !{void (i64, i16 addrspace(1)*, i8 addrspace(1)*)* @f1, !1}
!5 = !{void (i64, i64 addrspace(1)*, i8 addrspace(1)*)* @f2, !1}
!6 = !{void (i64, i16 addrspace(1)*, i32 addrspace(1)*)* @f3, !1}
!7 = !{void (i64, i32 addrspace(1)*, i8 addrspace(1)*)* @f4, !1}
