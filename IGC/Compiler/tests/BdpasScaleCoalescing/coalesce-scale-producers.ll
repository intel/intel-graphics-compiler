;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt --opaque-pointers --regkey EnableFP4Dpas=1 --regkey EnableBdpasScaleCoalescing=1 --igc-bdpas-scale-coalescing -platformCri -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --regkey EnableFP4Dpas=1 --regkey EnableBdpasScaleCoalescing=1 --igc-bdpas-scale-coalescing -platformCri -S < %s | FileCheck %s --check-prefix=I8-NO-TEMP
; RUN: igc_opt --opaque-pointers --regkey EnableFP4Dpas=1 --regkey EnableBdpasScaleCoalescing=1 -debugify --igc-bdpas-scale-coalescing -platformCri -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefix=DEBUGIFY

; A complete two-A by two-B rectangle is coalesced independently of how its
; canonical <2 x i8> scale operands were produced. Oversized ordinary scale
; operands remain unchanged.

; DEBUGIFY: CheckModuleDebugify: PASS

; CHECK-LABEL: define spir_kernel void @from_v4i8(
; CHECK-NOT: @llvm.genx.GenISA.sub.group.bdpas.packed
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8
; CHECK: ret void
define spir_kernel void @from_v4i8(<8 x i16> %a, <8 x i32> %b,
                                   ptr addrspace(1) %sa0.ptr, ptr addrspace(1) %sa1.ptr,
                                   ptr addrspace(1) %sb0.ptr, ptr addrspace(1) %sb1.ptr,
                                   ptr addrspace(1) %out) !intel_reqd_sub_group_size !6 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %sa0.lane = getelementptr i32, ptr addrspace(1) %sa0.ptr, i64 %lane
  %sa1.lane = getelementptr i32, ptr addrspace(1) %sa1.ptr, i64 %lane
  %sb0.lane = getelementptr i32, ptr addrspace(1) %sb0.ptr, i64 %lane
  %sb1.lane = getelementptr i32, ptr addrspace(1) %sb1.ptr, i64 %lane
  %sa0.raw = load i32, ptr addrspace(1) %sa0.lane, align 4
  %sa1.raw = load i32, ptr addrspace(1) %sa1.lane, align 4
  %sb0.raw = load i32, ptr addrspace(1) %sb0.lane, align 4
  %sb1.raw = load i32, ptr addrspace(1) %sb1.lane, align 4
  %sa0 = bitcast i32 %sa0.raw to <4 x i8>
  %sa1 = bitcast i32 %sa1.raw to <4 x i8>
  %sb0 = bitcast i32 %sb0.raw to <4 x i8>
  %sb1 = bitcast i32 %sb1.raw to <4 x i8>
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa0, <4 x i8> %sb0, i32 13, i32 13
  )
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa0, <4 x i8> %sb1, i32 13, i32 13
  )
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa1, <4 x i8> %sb0, i32 13, i32 13
  )
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa1, <4 x i8> %sb1, i32 13, i32 13
  )
  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

; CHECK-LABEL: define spir_kernel void @from_i16(
; CHECK-NOT: bitcast i16
; CHECK-NOT: extractelement <2 x i8>
; CHECK: [[PACKEDA:%.*]] = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %sa0.raw, i16 %sa1.raw, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
; CHECK: [[PACKEDB:%.*]] = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %sb0.raw, i16 %sb1.raw, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 0, i32 0)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 0, i32 16)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 16, i32 0)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 16, i32 16)
define spir_kernel void @from_i16(<8 x i16> %a, <8 x i32> %b,
                                  ptr addrspace(1) %sa0.ptr, ptr addrspace(1) %sa1.ptr,
                                  ptr addrspace(1) %sb0.ptr, ptr addrspace(1) %sb1.ptr,
                                  ptr addrspace(1) %out) !intel_reqd_sub_group_size !6 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %sa0.lane = getelementptr i16, ptr addrspace(1) %sa0.ptr, i64 %lane
  %sa1.lane = getelementptr i16, ptr addrspace(1) %sa1.ptr, i64 %lane
  %sb0.lane = getelementptr i16, ptr addrspace(1) %sb0.ptr, i64 %lane
  %sb1.lane = getelementptr i16, ptr addrspace(1) %sb1.ptr, i64 %lane
  %sa0.raw = load i16, ptr addrspace(1) %sa0.lane, align 2
  %sa1.raw = load i16, ptr addrspace(1) %sa1.lane, align 2
  %sb0.raw = load i16, ptr addrspace(1) %sb0.lane, align 2
  %sb1.raw = load i16, ptr addrspace(1) %sb1.lane, align 2
  %sa0 = bitcast i16 %sa0.raw to <2 x i8>
  %sa1 = bitcast i16 %sa1.raw to <2 x i8>
  %sb0 = bitcast i16 %sb0.raw to <2 x i8>
  %sb1 = bitcast i16 %sb1.raw to <2 x i8>
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa0, <2 x i8> %sb0, i32 13, i32 13
  )
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa0, <2 x i8> %sb1, i32 13, i32 13
  )
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa1, <2 x i8> %sb0, i32 13, i32 13
  )
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa1, <2 x i8> %sb1, i32 13, i32 13
  )
  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

; CHECK-LABEL: define spir_kernel void @from_i8(
; CHECK: [[SPA0:%.*]] = insertelement <4 x i8> poison, i8 %sa00, i32 0
; CHECK: [[SPA1:%.*]] = insertelement <4 x i8> [[SPA0]], i8 %sa10, i32 1
; CHECK: [[SPA2:%.*]] = insertelement <4 x i8> [[SPA1]], i8 %sa01, i32 2
; CHECK: [[SPACKEDA:%.*]] = insertelement <4 x i8> [[SPA2]], i8 %sa11, i32 3
; CHECK: [[SPB0:%.*]] = insertelement <4 x i8> poison, i8 %sb00, i32 0
; CHECK: [[SPB1:%.*]] = insertelement <4 x i8> [[SPB0]], i8 %sb10, i32 1
; CHECK: [[SPB2:%.*]] = insertelement <4 x i8> [[SPB1]], i8 %sb01, i32 2
; CHECK: [[SPACKEDB:%.*]] = insertelement <4 x i8> [[SPB2]], i8 %sb11, i32 3
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}<4 x i8> [[SPACKEDA]], <4 x i8> [[SPACKEDB]]{{.*}}i32 0, i32 0)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 0, i32 16)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 16, i32 0)
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}v4i8.v4i8{{.*}}i32 16, i32 16)
; I8-NO-TEMP-LABEL: define spir_kernel void @from_i8(
; I8-NO-TEMP-NOT: <2 x i8>
; I8-NO-TEMP: ret void
define spir_kernel void @from_i8(<8 x i16> %a, <8 x i32> %b,
                                 ptr addrspace(1) %sa00.ptr, ptr addrspace(1) %sa01.ptr,
                                 ptr addrspace(1) %sa10.ptr, ptr addrspace(1) %sa11.ptr,
                                 ptr addrspace(1) %sb00.ptr, ptr addrspace(1) %sb01.ptr,
                                 ptr addrspace(1) %sb10.ptr, ptr addrspace(1) %sb11.ptr,
                                 ptr addrspace(1) %out) !intel_reqd_sub_group_size !6 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %sa00.lane = getelementptr i8, ptr addrspace(1) %sa00.ptr, i64 %lane
  %sa01.lane = getelementptr i8, ptr addrspace(1) %sa01.ptr, i64 %lane
  %sa10.lane = getelementptr i8, ptr addrspace(1) %sa10.ptr, i64 %lane
  %sa11.lane = getelementptr i8, ptr addrspace(1) %sa11.ptr, i64 %lane
  %sb00.lane = getelementptr i8, ptr addrspace(1) %sb00.ptr, i64 %lane
  %sb01.lane = getelementptr i8, ptr addrspace(1) %sb01.ptr, i64 %lane
  %sb10.lane = getelementptr i8, ptr addrspace(1) %sb10.ptr, i64 %lane
  %sb11.lane = getelementptr i8, ptr addrspace(1) %sb11.ptr, i64 %lane
  %sa00 = load i8, ptr addrspace(1) %sa00.lane, align 1
  %sa01 = load i8, ptr addrspace(1) %sa01.lane, align 1
  %sa10 = load i8, ptr addrspace(1) %sa10.lane, align 1
  %sa11 = load i8, ptr addrspace(1) %sa11.lane, align 1
  %sb00 = load i8, ptr addrspace(1) %sb00.lane, align 1
  %sb01 = load i8, ptr addrspace(1) %sb01.lane, align 1
  %sb10 = load i8, ptr addrspace(1) %sb10.lane, align 1
  %sb11 = load i8, ptr addrspace(1) %sb11.lane, align 1
  %sa0.0 = insertelement <2 x i8> poison, i8 %sa00, i32 0
  %sa0 = insertelement <2 x i8> %sa0.0, i8 %sa01, i32 1
  %sa1.0 = insertelement <2 x i8> poison, i8 %sa10, i32 0
  %sa1 = insertelement <2 x i8> %sa1.0, i8 %sa11, i32 1
  %sb0.0 = insertelement <2 x i8> poison, i8 %sb00, i32 0
  %sb0 = insertelement <2 x i8> %sb0.0, i8 %sb01, i32 1
  %sb1.0 = insertelement <2 x i8> poison, i8 %sb10, i32 0
  %sb1 = insertelement <2 x i8> %sb1.0, i8 %sb11, i32 1
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa0, <2 x i8> %sb0, i32 13, i32 13
  )
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa0, <2 x i8> %sb1, i32 13, i32 13
  )
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa1, <2 x i8> %sb0, i32 13, i32 13
  )
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa1, <2 x i8> %sb1, i32 13, i32 13
  )
  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <2 x i8>, <2 x i8>, i32, i32
)
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <4 x i8>, <4 x i8>, i32, i32
)
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @from_v4i8, !1}
!1 = !{!2, !5}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @from_i16, !1}
!4 = !{ptr @from_i8, !1}
!5 = !{!"sub_group_size", i32 16}
!6 = !{i32 16}
!7 = !{!"requiredSubGroupSize", i32 16}
!8 = !{!"FuncMDMap[0]", ptr @from_v4i8}
!9 = !{!"FuncMDValue[0]", !7}
!10 = !{!"FuncMDMap[1]", ptr @from_i16}
!11 = !{!"FuncMDValue[1]", !7}
!12 = !{!"FuncMDMap[2]", ptr @from_i8}
!13 = !{!"FuncMDValue[2]", !7}
!14 = !{!"FuncMD", !8, !9, !10, !11, !12, !13}
!15 = !{!"ModuleMD", !14}
!IGCMetadata = !{!15}
