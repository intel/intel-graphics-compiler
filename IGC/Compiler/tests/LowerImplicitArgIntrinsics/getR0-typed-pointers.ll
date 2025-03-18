;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - --igc-lower-implicit-arg-intrinsic --platformdg2 | FileCheck %s

; Explanation:
; This tests checks if intrinsic calls are being lowered correctly.
; simdSize, simdLaneId and getR0 calls should be moved to the beggining and should not be repeated.
; To reproduce - compile kernel pattern with "-cl-opt-disable" option and flags:
; EnableGlobalStateBuffer=1
; ForceInlineStackCallWithImplArg=0

; Kernel pattern:

; size_t get_global()
; {
;   return get_global_id(0) + get_global_id(1) + get_global_id(2);
; }

; __kernel void test(__global int* out)
; {
;   out[0] = get_global();
; }


define spir_kernel void @test(i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
entry:
  %out.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %out, i32 addrspace(1)** %out.addr, align 8
  %call = call spir_func i64 @get_global() #4
  %conv = trunc i64 %call to i32
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %out.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0
  store i32 %conv, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func i64 @get_global() #1 {
entry:
; CHECK-DAG: [[R0:%.*]] = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
; CHECK-DAG: [[SIMD_SIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-DAG: [[SIMD_LANEID:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

  %0 = call i16 @llvm.genx.GenISA.getLocalID.Z.i16()
  %1 = call i16 @llvm.genx.GenISA.getLocalID.Y.i16()
  %2 = call <8 x i32> @llvm.genx.GenISA.getPayloadHeader.v8i32()
  %scalar111 = extractelement <8 x i32> %2, i32 0
  %scalar112 = extractelement <8 x i32> %2, i32 1
  %scalar113 = extractelement <8 x i32> %2, i32 2
  %scalar114 = extractelement <8 x i32> %2, i32 3
  %scalar115 = extractelement <8 x i32> %2, i32 4
  %scalar116 = extractelement <8 x i32> %2, i32 5
  %scalar117 = extractelement <8 x i32> %2, i32 6
  %scalar118 = extractelement <8 x i32> %2, i32 7
  %3 = call i16 @llvm.genx.GenISA.getLocalID.X.i16()
  %4 = call <3 x i32> @llvm.genx.GenISA.getEnqueuedLocalSize.v3i32()
  %scalar = extractelement <3 x i32> %4, i32 0
  %scalar109 = extractelement <3 x i32> %4, i32 1
  %scalar110 = extractelement <3 x i32> %4, i32 2
  %5 = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
  %cmpDim = icmp eq i32 0, 0
  %tmpOffsetR0 = select i1 %cmpDim, i32 1, i32 5
  %offsetR0 = add i32 0, %tmpOffsetR0
  %groupId = extractelement <8 x i32> %5, i32 %offsetR0
  %mul.i.i.i = mul i32 %scalar, %groupId
  %6 = zext i16 %3 to i32
  %cmp11.i.i.i.i = icmp ult i32 %6, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i) #4
  %add.i.i.i = add i32 %6, %mul.i.i.i
  %add4.i.i.i = add i32 %add.i.i.i, %scalar111
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %cmpDim61 = icmp eq i32 1, 0
  %tmpOffsetR062 = select i1 %cmpDim61, i32 1, i32 5
  %offsetR063 = add i32 1, %tmpOffsetR062
  %groupId64 = extractelement <8 x i32> %5, i32 %offsetR063
  %mul.i7.i.i = mul i32 %scalar109, %groupId64
  %7 = zext i16 %1 to i32
  %cmp11.i.i8.i.i = icmp ult i32 %7, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i) #4
  %add.i9.i.i = add i32 %7, %mul.i7.i.i
  %add4.i11.i.i = add i32 %add.i9.i.i, %scalar112
  %conv.i12.i.i = zext i32 %add4.i11.i.i to i64
  %cmpDim67 = icmp eq i32 2, 0
  %tmpOffsetR068 = select i1 %cmpDim67, i32 1, i32 5
  %offsetR069 = add i32 2, %tmpOffsetR068
  %groupId70 = extractelement <8 x i32> %5, i32 %offsetR069
  %mul.i15.i.i = mul i32 %scalar110, %groupId70
  %8 = zext i16 %0 to i32
  %cmp11.i.i16.i.i = icmp ult i32 %8, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i) #4
  %add.i17.i.i = add i32 %8, %mul.i15.i.i
  %add4.i19.i.i = add i32 %add.i17.i.i, %scalar113
  %conv.i20.i.i = zext i32 %add4.i19.i.i to i64
  %cmpDim73 = icmp eq i32 0, 0
  %tmpOffsetR074 = select i1 %cmpDim73, i32 1, i32 5
  %offsetR075 = add i32 0, %tmpOffsetR074
  %groupId76 = extractelement <8 x i32> %5, i32 %offsetR075
  %mul.i.i.i3 = mul i32 %scalar, %groupId76
  %9 = zext i16 %3 to i32
  %cmp11.i.i.i.i5 = icmp ult i32 %9, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i5) #4
  %add.i.i.i6 = add i32 %9, %mul.i.i.i3
  %add4.i.i.i8 = add i32 %add.i.i.i6, %scalar111
  %conv.i.i.i9 = zext i32 %add4.i.i.i8 to i64
  %cmpDim79 = icmp eq i32 1, 0
  %tmpOffsetR080 = select i1 %cmpDim79, i32 1, i32 5
  %offsetR081 = add i32 1, %tmpOffsetR080
  %groupId82 = extractelement <8 x i32> %5, i32 %offsetR081
  %mul.i7.i.i13 = mul i32 %scalar109, %groupId82
  %10 = zext i16 %1 to i32
  %cmp11.i.i8.i.i15 = icmp ult i32 %10, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i15) #4
  %add.i9.i.i16 = add i32 %10, %mul.i7.i.i13
  %add4.i11.i.i18 = add i32 %add.i9.i.i16, %scalar112
  %conv.i12.i.i19 = zext i32 %add4.i11.i.i18 to i64
  %cmpDim85 = icmp eq i32 2, 0
  %tmpOffsetR086 = select i1 %cmpDim85, i32 1, i32 5
  %offsetR087 = add i32 2, %tmpOffsetR086
  %groupId88 = extractelement <8 x i32> %5, i32 %offsetR087
  %mul.i15.i.i23 = mul i32 %scalar110, %groupId88
  %11 = zext i16 %0 to i32
  %cmp11.i.i16.i.i25 = icmp ult i32 %11, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i25) #4
  %add.i17.i.i26 = add i32 %11, %mul.i15.i.i23
  %add4.i19.i.i28 = add i32 %add.i17.i.i26, %scalar113
  %conv.i20.i.i29 = zext i32 %add4.i19.i.i28 to i64
  %add = add i64 %conv.i.i.i, %conv.i12.i.i19
  %cmpDim91 = icmp eq i32 0, 0
  %tmpOffsetR092 = select i1 %cmpDim91, i32 1, i32 5
  %offsetR093 = add i32 0, %tmpOffsetR092
  %groupId94 = extractelement <8 x i32> %5, i32 %offsetR093
  %mul.i.i.i33 = mul i32 %scalar, %groupId94
  %12 = zext i16 %3 to i32
  %cmp11.i.i.i.i35 = icmp ult i32 %12, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i35) #4
  %add.i.i.i36 = add i32 %12, %mul.i.i.i33
  %add4.i.i.i38 = add i32 %add.i.i.i36, %scalar111
  %conv.i.i.i39 = zext i32 %add4.i.i.i38 to i64
  %cmpDim97 = icmp eq i32 1, 0
  %tmpOffsetR098 = select i1 %cmpDim97, i32 1, i32 5
  %offsetR099 = add i32 1, %tmpOffsetR098
  %groupId100 = extractelement <8 x i32> %5, i32 %offsetR099
  %mul.i7.i.i43 = mul i32 %scalar109, %groupId100
  %13 = zext i16 %1 to i32
  %cmp11.i.i8.i.i45 = icmp ult i32 %13, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i45) #4
  %add.i9.i.i46 = add i32 %13, %mul.i7.i.i43
  %add4.i11.i.i48 = add i32 %add.i9.i.i46, %scalar112
  %conv.i12.i.i49 = zext i32 %add4.i11.i.i48 to i64
  %cmpDim103 = icmp eq i32 2, 0
  %tmpOffsetR0104 = select i1 %cmpDim103, i32 1, i32 5
  %offsetR0105 = add i32 2, %tmpOffsetR0104
  %groupId106 = extractelement <8 x i32> %5, i32 %offsetR0105
  %mul.i15.i.i53 = mul i32 %scalar110, %groupId106
  %14 = zext i16 %0 to i32
  %cmp11.i.i16.i.i55 = icmp ult i32 %14, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i55) #4
  %add.i17.i.i56 = add i32 %14, %mul.i15.i.i53
  %add4.i19.i.i58 = add i32 %add.i17.i.i56, %scalar113
  %conv.i20.i.i59 = zext i32 %add4.i19.i.i58 to i64
  %add3 = add i64 %add, %conv.i20.i.i59
  ret i64 %add3
}

; Function Attrs: nounwind willreturn
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.GenISA.getR0.v8i32() #3

; Function Attrs: nounwind
declare <3 x i32> @llvm.genx.GenISA.getEnqueuedLocalSize.v3i32() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.X.i16() #4

; Function Attrs: nounwind
declare <8 x i32> @llvm.genx.GenISA.getPayloadHeader.v8i32() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.Y.i16() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.Z.i16() #4

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "visaStackCall" }
attributes #2 = { nounwind willreturn }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind }

!igc.functions = !{!0, !4}

!0 = !{i64 ()* @get_global, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 2}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*, i32)* @test, !5}
!5 = !{!6, !7}
!6 = !{!"function_type", i32 0}
!7 = !{!"implicit_arg_desc", !8, !9, !10, !11, !12, !13, !14, !15}
!8 = !{i32 0}
!9 = !{i32 1}
!10 = !{i32 7}
!11 = !{i32 8}
!12 = !{i32 9}
!13 = !{i32 10}
!14 = !{i32 13}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 0}
