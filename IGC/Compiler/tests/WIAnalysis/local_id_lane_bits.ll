;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

; In a 1-D work-group, lane i of thread t has the local id x = t * SIMD size + i, so only the
; low log2(SIMD size) bits of x differ between the lanes, and x < X clears the bits above those of
; X - 1. A value computed from x, constants and uniform arguments that depends on none of these
; bits is uniform.

; Work-group 256x1x1, SIMD16: x differs in bits 0-3.
; CHECK-LABEL: block: entry function: testX256
; CHECK: N: {{.*}} %lx = zext i16 %localIdX to i32
; CHECK: U: {{.*}} %hi = lshr i32 %lx, 4
; CHECK: N: {{.*}} %mid = lshr i32 %lx, 3
; CHECK: N: {{.*}} %lane = and i32 %lx, 15
; CHECK: U: {{.*}} %clr = and i32 %lx, -16
; CHECK: U: {{.*}} %set = or i32 %lx, 15
; CHECK: N: {{.*}} %s = shl i32 %lx, 1
; CHECK: U: {{.*}} %col = and i32 %s, 224
; CHECK: N: {{.*}} %col2 = and i32 %s, 30
; CHECK: U: {{.*}} %h16 = lshr i16 %localIdX, 4
; CHECK: U: {{.*}} %sx = sext i16 %h16 to i32
; CHECK: U: {{.*}} %as = ashr i16 %localIdX, 4
; CHECK: N: {{.*}} %as2 = ashr i16 %localIdX, 2
; CHECK: U: {{.*}} %tr = trunc i32 %hi to i8
; CHECK: U: {{.*}} %div = udiv i32 %lx, 16
; CHECK: N: {{.*}} %rem = urem i32 %lx, 16
; CHECK: N: {{.*}} %m32 = mul i32 %lx, 32
; CHECK: U: {{.*}} %m32h = lshr i32 %m32, 9
; CHECK: N: {{.*}} %m48 = mul i32 %lx, 48
; CHECK: N: {{.*}} %m48h = lshr i32 %m48, 9
; CHECK: N: {{.*}} %al = add i32 %lx, 16
; CHECK: U: {{.*}} %alh = lshr i32 %al, 4
; CHECK: N: {{.*}} %un = add i32 %lx, 8
; CHECK: N: {{.*}} %unh = lshr i32 %un, 4
; CHECK: N: {{.*}} %sb = sub i32 %lx, 32
; CHECK: U: {{.*}} %sbh = lshr i32 %sb, 4
; CHECK: N: {{.*}} %ux = xor i32 %lx, %u
; CHECK: U: {{.*}} %uxh = lshr i32 %ux, 4
; CHECK: N: {{.*}} %comb = or i32 %hi, %lane
; CHECK: U: {{.*}} %us = shl i32 %u, 4
; CHECK: N: {{.*}} %ua = add i32 %lx, %us
; CHECK: U: {{.*}} %uah = lshr i32 %ua, 4
; CHECK: N: {{.*}} %hs = shl i16 %localIdX, 12
; CHECK: N: {{.*}} %hsx = sext i16 %hs to i32
; CHECK: N: {{.*}} %hsxh = lshr i32 %hsx, 16
; CHECK: N: {{.*}} %ha = ashr i16 %hs, 4
; CHECK: N: {{.*}} %hah = and i16 %ha, -4096
; CHECK: N: {{.*}} %ru = sub i32 64, %lx
; CHECK: N: {{.*}} %ruh = and i32 %ru, -16
; CHECK: N: {{.*}} %x64 = zext i16 %localIdX to i64
; CHECK: U: {{.*}} %h64 = lshr i64 %x64, 4
; CHECK: U: {{.*}} %z63 = lshr i64 %x64, 63
; CHECK: N: {{.*}} %top = shl i64 %x64, 60
; CHECK: N: {{.*}} %topl = lshr i64 %top, 63
; CHECK: N: {{.*}} %topa = ashr i64 %top, 63
; CHECK: N: {{.*}} %topah = and i64 %topa, -4294967296
; CHECK: N: {{.*}} %mid64 = shl i64 %x64, 59
; CHECK: U: {{.*}} %mida = ashr i64 %mid64, 63
; CHECK: N: {{.*}} %sx64 = sext i16 %hs to i64
; CHECK: N: {{.*}} %sx64h = and i64 %sx64, -65536
; CHECK: N: {{.*}} %zx64 = zext i16 %hs to i64
; CHECK: U: {{.*}} %zx64h = and i64 %zx64, -65536
define spir_kernel void @testX256(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %hi = lshr i32 %lx, 4
  %mid = lshr i32 %lx, 3
  %lane = and i32 %lx, 15
  %clr = and i32 %lx, -16
  %set = or i32 %lx, 15
  %s = shl i32 %lx, 1
  %col = and i32 %s, 224
  %col2 = and i32 %s, 30
  %h16 = lshr i16 %localIdX, 4
  %sx = sext i16 %h16 to i32
  %as = ashr i16 %localIdX, 4
  %as2 = ashr i16 %localIdX, 2
  %tr = trunc i32 %hi to i8
  %div = udiv i32 %lx, 16
  %rem = urem i32 %lx, 16
  ; x * 32 differs in bits 5-8, x * 48 from bit 4 up.
  %m32 = mul i32 %lx, 32
  %m32h = lshr i32 %m32, 9
  %m48 = mul i32 %lx, 48
  %m48h = lshr i32 %m48, 9
  ; Adding a multiple of 16 carries nothing out of bits 0-3; adding 8 may.
  %al = add i32 %lx, 16
  %alh = lshr i32 %al, 4
  %un = add i32 %lx, 8
  %unh = lshr i32 %un, 4
  %sb = sub i32 %lx, 32
  %sbh = lshr i32 %sb, 4
  ; %u is a uniform kernel argument.
  %ux = xor i32 %lx, %u
  %uxh = lshr i32 %ux, 4
  %comb = or i32 %hi, %lane
  ; A multiple of 16 computed from a uniform argument.
  %us = shl i32 %u, 4
  %ua = add i32 %lx, %us
  %uah = lshr i32 %ua, 4
  ; x << 12 has a variant sign bit, which sext and ashr copy into the high bits.
  %hs = shl i16 %localIdX, 12
  %hsx = sext i16 %hs to i32
  %hsxh = lshr i32 %hsx, 16
  %ha = ashr i16 %hs, 4
  %hah = and i16 %ha, -4096
  ; 64 - x borrows out of bits 0-3 unless they are zero.
  %ru = sub i32 64, %lx
  %ruh = and i32 %ru, -16
  ; i64: shifts by 63, and a sign bit that differs between the lanes (x << 60) or does not (x << 59).
  %x64 = zext i16 %localIdX to i64
  %h64 = lshr i64 %x64, 4
  %z63 = lshr i64 %x64, 63
  %top = shl i64 %x64, 60
  %topl = lshr i64 %top, 63
  %topa = ashr i64 %top, 63
  %topah = and i64 %topa, -4294967296
  %mid64 = shl i64 %x64, 59
  %mida = ashr i64 %mid64, 63
  ; sext of x << 12 copies its variant sign bit into bits 16-63; zext does not.
  %sx64 = sext i16 %hs to i64
  %sx64h = and i64 %sx64, -65536
  %zx64 = zext i16 %hs to i64
  %zx64h = and i64 %zx64, -65536
  ret void
}

; The same through the getLocalID.X intrinsic.
; CHECK-LABEL: block: entry function: testIntrinsic
; CHECK: U: {{.*}} %ih = lshr i16 %ix, 4
; CHECK: N: {{.*}} %il = and i16 %ix, 15
define spir_kernel void @testIntrinsic(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %ix = call i16 @llvm.genx.GenISA.getLocalID.X()
  %ih = lshr i16 %ix, 4
  %il = and i16 %ix, 15
  ret void
}

; Work-group 8x1x1, SIMD16: x < 8, so it differs in bits 0-2 only.
; CHECK-LABEL: block: entry function: testX8
; CHECK: U: {{.*}} %xh = lshr i32 %lx, 3
; CHECK: N: {{.*}} %x4 = and i32 %lx, 4
define spir_kernel void @testX8(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %xh = lshr i32 %lx, 3
  %x4 = and i32 %lx, 4
  ret void
}

; Work-group 24x1x1, SIMD16: X is not a power of two; x still differs in bits 0-3 only.
; CHECK-LABEL: block: entry function: testX24
; CHECK: U: {{.*}} %hi = lshr i32 %lx, 4
; CHECK: N: {{.*}} %lane = and i32 %lx, 15
define spir_kernel void @testX24(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %hi = lshr i32 %lx, 4
  %lane = and i32 %lx, 15
  ret void
}

; Work-group 256x1x1 without a required sub-group size: the SIMD size is assumed to be 32.
; CHECK-LABEL: block: entry function: testNoSubGroupSize
; CHECK: U: {{.*}} %hi = lshr i32 %lx, 5
; CHECK: N: {{.*}} %mid = lshr i32 %lx, 4
define spir_kernel void @testNoSubGroupSize(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %hi = lshr i32 %lx, 5
  %mid = lshr i32 %lx, 4
  ret void
}

; Work-group 32x2x1: not 1-D. Without uniform work-groups, an edge work-group 24x2 holds
; x = 16..23, 0..7 in its second thread, so x >> 4 differs between the lanes.
; CHECK-LABEL: block: entry function: test2D
; CHECK: N: {{.*}} %hi = lshr i32 %lx, 4
define spir_kernel void @test2D(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %hi = lshr i32 %lx, 4
  ret void
}

declare i16 @llvm.genx.GenISA.getLocalID.X()

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!0 = !{ptr @testX256, !926}
!1 = !{ptr @testIntrinsic, !926}
!2 = !{ptr @testX8, !926}
!3 = !{ptr @testX24, !926}
!4 = !{ptr @testNoSubGroupSize, !926}
!5 = !{ptr @test2D, !926}
!IGCMetadata = !{!100}
!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !200, !201, !210, !211, !220, !221, !230, !231, !240, !241, !250, !251}
!200 = !{!"FuncMDMap[0]", ptr @testX256}
!201 = !{!"FuncMDValue[0]", !920, !202, !924, !917}
!202 = !{!"threadGroupSize", !203, !204, !205}
!203 = !{!"dim0", i32 256}
!204 = !{!"dim1", i32 1}
!205 = !{!"dim2", i32 1}
!210 = !{!"FuncMDMap[1]", ptr @testIntrinsic}
!211 = !{!"FuncMDValue[1]", !920, !212, !924, !917}
!212 = !{!"threadGroupSize", !213, !214, !215}
!213 = !{!"dim0", i32 256}
!214 = !{!"dim1", i32 1}
!215 = !{!"dim2", i32 1}
!220 = !{!"FuncMDMap[2]", ptr @testX8}
!221 = !{!"FuncMDValue[2]", !920, !222, !924, !917}
!222 = !{!"threadGroupSize", !223, !224, !225}
!223 = !{!"dim0", i32 8}
!224 = !{!"dim1", i32 1}
!225 = !{!"dim2", i32 1}
!230 = !{!"FuncMDMap[3]", ptr @testX24}
!231 = !{!"FuncMDValue[3]", !920, !232, !924, !917}
!232 = !{!"threadGroupSize", !233, !234, !235}
!233 = !{!"dim0", i32 24}
!234 = !{!"dim1", i32 1}
!235 = !{!"dim2", i32 1}
!240 = !{!"FuncMDMap[4]", ptr @testNoSubGroupSize}
!241 = !{!"FuncMDValue[4]", !920, !242, !917}
!242 = !{!"threadGroupSize", !243, !244, !245}
!243 = !{!"dim0", i32 256}
!244 = !{!"dim1", i32 1}
!245 = !{!"dim2", i32 1}
!250 = !{!"FuncMDMap[5]", ptr @test2D}
!251 = !{!"FuncMDValue[5]", !920, !252, !924, !917}
!252 = !{!"threadGroupSize", !253, !254, !255}
!253 = !{!"dim0", i32 32}
!254 = !{!"dim1", i32 2}
!255 = !{!"dim2", i32 1}
!900 = !{!"argId", i32 0}
!901 = !{!"implicitArgInfoListVec[0]", !900}
!902 = !{!"argId", i32 1}
!903 = !{!"implicitArgInfoListVec[1]", !902}
!904 = !{!"argId", i32 6}
!905 = !{!"implicitArgInfoListVec[2]", !904}
!906 = !{!"argId", i32 8}
!907 = !{!"implicitArgInfoListVec[3]", !906}
!908 = !{!"argId", i32 9}
!909 = !{!"implicitArgInfoListVec[4]", !908}
!910 = !{!"argId", i32 10}
!911 = !{!"implicitArgInfoListVec[5]", !910}
!912 = !{!"argId", i32 15}
!913 = !{!"explicitArgNum", i32 0}
!914 = !{!"implicitArgInfoListVec[6]", !912, !913}
!917 = !{!"implicitArgInfoList", !901, !903, !905, !907, !909, !911, !914}
!920 = !{!"workGroupWalkOrder", !921, !922, !923}
!921 = !{!"dim0", i32 0}
!922 = !{!"dim1", i32 1}
!923 = !{!"dim2", i32 2}
!924 = !{!"requiredSubGroupSize", i32 16}
!925 = !{!"function_type", i32 0}
!926 = !{!925}
