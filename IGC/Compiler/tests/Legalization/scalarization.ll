;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: igc_opt %s -S -o - -igc-type-legalizer -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f1(<4 x float>* %pc, <4 x float>* %pa, <4 x float>* %pb, i1 %cond) {
  %a = load <4 x float>* %pa
  %b = load <4 x float>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = fadd <4 x float> %a, %b
  br label %exit

F:
  %r2 = fsub <4 x float> %a, %b
  br label %exit

exit:
  %r = phi <4 x float> [ %r1, %T ], [ %r2, %F ]
  store <4 x float> %r, <4 x float>* %pc
  ret void
}

; CHECK-LABEL: define void @f1
; CHECK: %a = load <4 x float>* %pa, align 16
; CHECK: %a.sclr0 = extractelement <4 x float> %a, i32 0
; CHECK: %a.sclr1 = extractelement <4 x float> %a, i32 1
; CHECK: %a.sclr2 = extractelement <4 x float> %a, i32 2
; CHECK: %a.sclr3 = extractelement <4 x float> %a, i32 3
; CHECK: %b = load <4 x float>* %pb, align 16
; CHECK: %b.sclr0 = extractelement <4 x float> %b, i32 0
; CHECK: %b.sclr1 = extractelement <4 x float> %b, i32 1
; CHECK: %b.sclr2 = extractelement <4 x float> %b, i32 2
; CHECK: %b.sclr3 = extractelement <4 x float> %b, i32 3
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %r1.sclr0 = fadd float %a.sclr0, %b.sclr0
; CHECK: %r1.sclr1 = fadd float %a.sclr1, %b.sclr1
; CHECK: %r1.sclr2 = fadd float %a.sclr2, %b.sclr2
; CHECK: %r1.sclr3 = fadd float %a.sclr3, %b.sclr3
; CHECK: br label %exit

; CHECK: %r2.sclr0 = fsub float %a.sclr0, %b.sclr0
; CHECK: %r2.sclr1 = fsub float %a.sclr1, %b.sclr1
; CHECK: %r2.sclr2 = fsub float %a.sclr2, %b.sclr2
; CHECK: %r2.sclr3 = fsub float %a.sclr3, %b.sclr3
; CHECK: br label %exit

; CHECK: %r.sclr0 = phi float [ %r1.sclr0, %T ], [ %r2.sclr0, %F ]
; CHECK: %r.sclr1 = phi float [ %r1.sclr1, %T ], [ %r2.sclr1, %F ]
; CHECK: %r.sclr2 = phi float [ %r1.sclr2, %T ], [ %r2.sclr2, %F ]
; CHECK: %r.sclr3 = phi float [ %r1.sclr3, %T ], [ %r2.sclr3, %F ]
; CHECK: %r.vec0 = insertelement <4 x float> undef, float %r.sclr0, i32 0
; CHECK: %r.vec1 = insertelement <4 x float> %r.vec0, float %r.sclr1, i32 1
; CHECK: %r.vec2 = insertelement <4 x float> %r.vec1, float %r.sclr2, i32 2
; CHECK: %r.vec3 = insertelement <4 x float> %r.vec2, float %r.sclr3, i32 3
; CHECK: store <4 x float> %r.vec3, <4 x float>* %pc, align 16
; CHECK: ret void


define void @f2(<5 x float>* %pc, <5 x float>* %pa, <5 x float>* %pb, i1 %cond) {
  %a = load <5 x float>* %pa
  %b = load <5 x float>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = fadd <5 x float> %a, %b
  br label %exit

F:
  %r2 = fsub <5 x float> %a, %b
  br label %exit

exit:
  %r = phi <5 x float> [ %r1, %T ], [ %r2, %F ]
  store <5 x float> %r, <5 x float>* %pc
  ret void
}

; CHECK-LABEL: define void @f2
; CHECK: %pa.sclr0.ptrcast = bitcast <5 x float>* %pa to <4 x float>*
; CHECK: %a.vec0 = load <4 x float>* %pa.sclr0.ptrcast, align 32
; CHECK: %a.sclr0 = extractelement <4 x float> %a.vec0, i32 0
; CHECK: %a.sclr1 = extractelement <4 x float> %a.vec0, i32 1
; CHECK: %a.sclr2 = extractelement <4 x float> %a.vec0, i32 2
; CHECK: %a.sclr3 = extractelement <4 x float> %a.vec0, i32 3
; CHECK: %pa.sclr4 = getelementptr inbounds <5 x float>* %pa, i32 0, i32 4
; CHECK: %a.sclr4 = load float* %pa.sclr4, align 32
; CHECK: %pb.sclr0.ptrcast = bitcast <5 x float>* %pb to <4 x float>*
; CHECK: %b.vec0 = load <4 x float>* %pb.sclr0.ptrcast, align 32
; CHECK: %b.sclr0 = extractelement <4 x float> %b.vec0, i32 0
; CHECK: %b.sclr1 = extractelement <4 x float> %b.vec0, i32 1
; CHECK: %b.sclr2 = extractelement <4 x float> %b.vec0, i32 2
; CHECK: %b.sclr3 = extractelement <4 x float> %b.vec0, i32 3
; CHECK: %pb.sclr4 = getelementptr inbounds <5 x float>* %pb, i32 0, i32 4
; CHECK: %b.sclr4 = load float* %pb.sclr4, align 32
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %r1.sclr0 = fadd float %a.sclr0, %b.sclr0
; CHECK: %r1.sclr1 = fadd float %a.sclr1, %b.sclr1
; CHECK: %r1.sclr2 = fadd float %a.sclr2, %b.sclr2
; CHECK: %r1.sclr3 = fadd float %a.sclr3, %b.sclr3
; CHECK: %r1.sclr4 = fadd float %a.sclr4, %b.sclr4

; CHECK: %r2.sclr0 = fsub float %a.sclr0, %b.sclr0
; CHECK: %r2.sclr1 = fsub float %a.sclr1, %b.sclr1
; CHECK: %r2.sclr2 = fsub float %a.sclr2, %b.sclr2
; CHECK: %r2.sclr3 = fsub float %a.sclr3, %b.sclr3
; CHECK: %r2.sclr4 = fsub float %a.sclr4, %b.sclr4
; CHECK: br label %exit

; CHECK: %r.sclr0 = phi float [ %r1.sclr0, %T ], [ %r2.sclr0, %F ]
; CHECK: %r.sclr1 = phi float [ %r1.sclr1, %T ], [ %r2.sclr1, %F ]
; CHECK: %r.sclr2 = phi float [ %r1.sclr2, %T ], [ %r2.sclr2, %F ]
; CHECK: %r.sclr3 = phi float [ %r1.sclr3, %T ], [ %r2.sclr3, %F ]
; CHECK: %r.sclr4 = phi float [ %r1.sclr4, %T ], [ %r2.sclr4, %F ]
; CHECK: %pc.sclr0.ptrcast = bitcast <5 x float>* %pc to <4 x float>*
; CHECK: %r.vec3.vec0 = insertelement <4 x float> undef, float %r.sclr0, i32 0
; CHECK: %r.vec3.vec1 = insertelement <4 x float> %r.vec3.vec0, float %r.sclr1, i32 1
; CHECK: %r.vec3.vec2 = insertelement <4 x float> %r.vec3.vec1, float %r.sclr2, i32 2
; CHECK: %r.vec3.vec3 = insertelement <4 x float> %r.vec3.vec2, float %r.sclr3, i32 3
; CHECK: store <4 x float> %r.vec3.vec3, <4 x float>* %pc.sclr0.ptrcast, align 32
; CHECK: %pc.sclr4 = getelementptr inbounds <5 x float>* %pc, i32 0, i32 4
; CHECK: store float %r.sclr4, float* %pc.sclr4, align 32
; CHECK: ret void


define void @f3(<6 x float>* %pc, <6 x float>* %pa, <6 x float>* %pb, i1 %cond) {
  %a = load <6 x float>* %pa
  %b = load <6 x float>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = fadd <6 x float> %a, %b
  br label %exit

F:
  %r2 = fsub <6 x float> %a, %b
  br label %exit

exit:
  %r = phi <6 x float> [ %r1, %T ], [ %r2, %F ]
  store <6 x float> %r, <6 x float>* %pc
  ret void
}

; CHECK-LABEL: define void @f3
; CHECK: %pa.sclr0.ptrcast = bitcast <6 x float>* %pa to <4 x float>*
; CHECK: %a.vec0 = load <4 x float>* %pa.sclr0.ptrcast, align 32
; CHECK: %a.sclr0 = extractelement <4 x float> %a.vec0, i32 0
; CHECK: %a.sclr1 = extractelement <4 x float> %a.vec0, i32 1
; CHECK: %a.sclr2 = extractelement <4 x float> %a.vec0, i32 2
; CHECK: %a.sclr3 = extractelement <4 x float> %a.vec0, i32 3
; CHECK: %pa.sclr4 = getelementptr inbounds <6 x float>* %pa, i32 0, i32 4
; CHECK: %pa.sclr4.ptrcast = bitcast float* %pa.sclr4 to <2 x float>*
; CHECK: %a.vec4 = load <2 x float>* %pa.sclr4.ptrcast, align 32
; CHECK: %a.sclr4 = extractelement <2 x float> %a.vec4, i32 0
; CHECK: %a.sclr5 = extractelement <2 x float> %a.vec4, i32 1
; CHECK: %pb.sclr0.ptrcast = bitcast <6 x float>* %pb to <4 x float>*
; CHECK: %b.vec0 = load <4 x float>* %pb.sclr0.ptrcast, align 32
; CHECK: %b.sclr0 = extractelement <4 x float> %b.vec0, i32 0
; CHECK: %b.sclr1 = extractelement <4 x float> %b.vec0, i32 1
; CHECK: %b.sclr2 = extractelement <4 x float> %b.vec0, i32 2
; CHECK: %b.sclr3 = extractelement <4 x float> %b.vec0, i32 3
; CHECK: %pb.sclr4 = getelementptr inbounds <6 x float>* %pb, i32 0, i32 4
; CHECK: %pb.sclr4.ptrcast = bitcast float* %pb.sclr4 to <2 x float>*
; CHECK: %b.vec4 = load <2 x float>* %pb.sclr4.ptrcast, align 32
; CHECK: %b.sclr4 = extractelement <2 x float> %b.vec4, i32 0
; CHECK: %b.sclr5 = extractelement <2 x float> %b.vec4, i32 1
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %r1.sclr0 = fadd float %a.sclr0, %b.sclr0
; CHECK: %r1.sclr1 = fadd float %a.sclr1, %b.sclr1
; CHECK: %r1.sclr2 = fadd float %a.sclr2, %b.sclr2
; CHECK: %r1.sclr3 = fadd float %a.sclr3, %b.sclr3
; CHECK: %r1.sclr4 = fadd float %a.sclr4, %b.sclr4
; CHECK: %r1.sclr5 = fadd float %a.sclr5, %b.sclr5
; CHECK: br label %exit

; CHECK: %r2.sclr0 = fsub float %a.sclr0, %b.sclr0
; CHECK: %r2.sclr1 = fsub float %a.sclr1, %b.sclr1
; CHECK: %r2.sclr2 = fsub float %a.sclr2, %b.sclr2
; CHECK: %r2.sclr3 = fsub float %a.sclr3, %b.sclr3
; CHECK: %r2.sclr4 = fsub float %a.sclr4, %b.sclr4
; CHECK: %r2.sclr5 = fsub float %a.sclr5, %b.sclr5
; CHECK: br label %exit

; CHECK: %r.sclr0 = phi float [ %r1.sclr0, %T ], [ %r2.sclr0, %F ]
; CHECK: %r.sclr1 = phi float [ %r1.sclr1, %T ], [ %r2.sclr1, %F ]
; CHECK: %r.sclr2 = phi float [ %r1.sclr2, %T ], [ %r2.sclr2, %F ]
; CHECK: %r.sclr3 = phi float [ %r1.sclr3, %T ], [ %r2.sclr3, %F ]
; CHECK: %r.sclr4 = phi float [ %r1.sclr4, %T ], [ %r2.sclr4, %F ]
; CHECK: %r.sclr5 = phi float [ %r1.sclr5, %T ], [ %r2.sclr5, %F ]
; CHECK: %pc.sclr0.ptrcast = bitcast <6 x float>* %pc to <4 x float>*
; CHECK: %r.vec3.vec0 = insertelement <4 x float> undef, float %r.sclr0, i32 0
; CHECK: %r.vec3.vec1 = insertelement <4 x float> %r.vec3.vec0, float %r.sclr1, i32 1
; CHECK: %r.vec3.vec2 = insertelement <4 x float> %r.vec3.vec1, float %r.sclr2, i32 2
; CHECK: %r.vec3.vec3 = insertelement <4 x float> %r.vec3.vec2, float %r.sclr3, i32 3
; CHECK: store <4 x float> %r.vec3.vec3, <4 x float>* %pc.sclr0.ptrcast, align 32
; CHECK: %pc.sclr4 = getelementptr inbounds <6 x float>* %pc, i32 0, i32 4
; CHECK: %pc.sclr4.ptrcast = bitcast float* %pc.sclr4 to <2 x float>*
; CHECK: %r.vec5.vec0 = insertelement <2 x float> undef, float %r.sclr4, i32 0
; CHECK: %r.vec5.vec1 = insertelement <2 x float> %r.vec5.vec0, float %r.sclr5, i32 1
; CHECK: store <2 x float> %r.vec5.vec1, <2 x float>* %pc.sclr4.ptrcast, align 32
; CHECK: ret void


define void @f4(<6 x i1>* %pc, <6 x i1>* %pa, <6 x i1>* %pb, i1 %cond) {
  %a = load <6 x i1>* %pa
  %b = load <6 x i1>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = and <6 x i1> %a, %b
  br label %exit

F:
  %r2 = or <6 x i1> %a, %b
  br label %exit

exit:
  %r = phi <6 x i1> [ %r1, %T ], [ %r2, %F ]
  store <6 x i1> %r, <6 x i1>* %pc
  ret void
}

; CHECK-LABEL: define void @f4
; CHECK: %pa.ptrcast = bitcast <6 x i1>* %pa to i8*
; CHECK: %a.chunk = load i8* %pa.ptrcast, align 8
; CHECK: %pb.ptrcast = bitcast <6 x i1>* %pb to i8*
; CHECK: %b.chunk = load i8* %pb.ptrcast, align 8
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %1 = and i8 %a.chunk, %b.chunk
; CHECK: %2 = and i8 %a.chunk, %b.chunk
; CHECK: %3 = and i8 %a.chunk, %b.chunk
; CHECK: %4 = and i8 %a.chunk, %b.chunk
; CHECK: %5 = and i8 %a.chunk, %b.chunk
; CHECK: %6 = and i8 %a.chunk, %b.chunk
; CHECK: br label %exit

; CHECK: %7 = or i8 %a.chunk, %b.chunk
; CHECK: %8 = or i8 %a.chunk, %b.chunk
; CHECK: %9 = or i8 %a.chunk, %b.chunk
; CHECK: %10 = or i8 %a.chunk, %b.chunk
; CHECK: %11 = or i8 %a.chunk, %b.chunk
; CHECK: %12 = or i8 %a.chunk, %b.chunk
; CHECK: br label %exit

; CHECK: %r.sclr0.in.in = phi i8 [ %1, %T ], [ %7, %F ]
; CHECK: %r.sclr1.in.in = phi i8 [ %2, %T ], [ %8, %F ]
; CHECK: %r.sclr2.in.in = phi i8 [ %3, %T ], [ %9, %F ]
; CHECK: %r.sclr3.in.in = phi i8 [ %4, %T ], [ %10, %F ]
; CHECK: %r.sclr4.in.in = phi i8 [ %5, %T ], [ %11, %F ]
; CHECK: %r.sclr5.in.in = phi i8 [ %6, %T ], [ %12, %F ]
; CHECK: %r.sclr5.in = and i8 %r.sclr5.in.in, 32
; CHECK: %r.sclr4.in = and i8 %r.sclr4.in.in, 16
; CHECK: %r.sclr3.in = and i8 %r.sclr3.in.in, 8
; CHECK: %r.sclr2.in = and i8 %r.sclr2.in.in, 4
; CHECK: %r.sclr1.in = and i8 %r.sclr1.in.in, 2
; CHECK: %r.sclr0.in = and i8 %r.sclr0.in.in, 1
; CHECK: %r.chunk1 = or i8 %r.sclr0.in, %r.sclr1.in
; CHECK: %r.chunk2 = or i8 %r.chunk1, %r.sclr2.in
; CHECK: %r.chunk3 = or i8 %r.chunk2, %r.sclr3.in
; CHECK: %r.chunk4 = or i8 %r.chunk3, %r.sclr4.in
; CHECK: %r.chunk5 = or i8 %r.chunk4, %r.sclr5.in
; CHECK: %pc.ptrcast = bitcast <6 x i1>* %pc to i8*
; CHECK: store i8 %r.chunk5, i8* %pc.ptrcast, align 8
; CHECK: ret void


define void @f5(<6 x i2>* %pc, <6 x i2>* %pa, <6 x i2>* %pb, i1 %cond) {
  %a = load <6 x i2>* %pa
  %b = load <6 x i2>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = and <6 x i2> %a, %b
  br label %exit

F:
  %r2 = or <6 x i2> %a, %b
  br label %exit

exit:
  %r = phi <6 x i2> [ %r1, %T ], [ %r2, %F ]
  store <6 x i2> %r, <6 x i2>* %pc
  ret void
}

; CHECK-LABEL: define void @f5
; CHECK: %pa.ptrcast = bitcast <6 x i2>* %pa to i16*
; CHECK: %a.chunk = load i16* %pa.ptrcast, align 8
; CHECK: %a.lshr1 = lshr i16 %a.chunk, 2
; CHECK: %a.lshr2 = lshr i16 %a.chunk, 4
; CHECK: %a.lshr3 = lshr i16 %a.chunk, 6
; CHECK: %a.lshr4 = lshr i16 %a.chunk, 8
; CHECK: %a.lshr5 = lshr i16 %a.chunk, 10
; CHECK: %pb.ptrcast = bitcast <6 x i2>* %pb to i16*
; CHECK: %b.chunk = load i16* %pb.ptrcast, align 8
; CHECK: %b.lshr1 = lshr i16 %b.chunk, 2
; CHECK: %b.lshr2 = lshr i16 %b.chunk, 4
; CHECK: %b.lshr3 = lshr i16 %b.chunk, 6
; CHECK: %b.lshr4 = lshr i16 %b.chunk, 8
; CHECK: %b.lshr5 = lshr i16 %b.chunk, 10
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %r1.sclr0.promote = and i16 %a.chunk, %b.chunk
; CHECK: %r1.sclr1.promote = and i16 %a.lshr1, %b.lshr1
; CHECK: %r1.sclr2.promote = and i16 %a.lshr2, %b.lshr2
; CHECK: %r1.sclr3.promote = and i16 %a.lshr3, %b.lshr3
; CHECK: %r1.sclr4.promote = and i16 %a.lshr4, %b.lshr4
; CHECK: %r1.sclr5.promote = and i16 %a.lshr5, %b.lshr5
; CHECK: br label %exit

; CHECK: %r2.sclr0.promote1 = or i16 %a.chunk, %b.chunk
; CHECK: %r2.sclr1.promote2 = or i16 %a.lshr1, %b.lshr1
; CHECK: %r2.sclr2.promote3 = or i16 %a.lshr2, %b.lshr2
; CHECK: %r2.sclr3.promote4 = or i16 %a.lshr3, %b.lshr3
; CHECK: %r2.sclr4.promote5 = or i16 %a.lshr4, %b.lshr4
; CHECK: %r2.sclr5.promote6 = or i16 %a.lshr5, %b.lshr5
; CHECK: br label %exit

; CHECK: %r.sclr0.promote = phi i16 [ %r1.sclr0.promote, %T ], [ %r2.sclr0.promote1, %F ]
; CHECK: %r.sclr1.promote = phi i16 [ %r1.sclr1.promote, %T ], [ %r2.sclr1.promote2, %F ]
; CHECK: %r.sclr2.promote = phi i16 [ %r1.sclr2.promote, %T ], [ %r2.sclr2.promote3, %F ]
; CHECK: %r.sclr3.promote = phi i16 [ %r1.sclr3.promote, %T ], [ %r2.sclr3.promote4, %F ]
; CHECK: %r.sclr4.promote = phi i16 [ %r1.sclr4.promote, %T ], [ %r2.sclr4.promote5, %F ]
; CHECK: %r.sclr5.promote = phi i16 [ %r1.sclr5.promote, %T ], [ %r2.sclr5.promote6, %F ]
; CHECK: %r.sclr0.promote.zext = and i16 %r.sclr0.promote, 3
; CHECK: %r.sclr1.promote.zext = shl i16 %r.sclr1.promote, 2
; CHECK: %r.sclr1.zext.shl = and i16 %r.sclr1.promote.zext, 12
; CHECK: %r.chunk1 = or i16 %r.sclr0.promote.zext, %r.sclr1.zext.shl
; CHECK: %r.sclr2.promote.zext = shl i16 %r.sclr2.promote, 4
; CHECK: %r.sclr2.zext.shl = and i16 %r.sclr2.promote.zext, 48
; CHECK: %r.chunk2 = or i16 %r.chunk1, %r.sclr2.zext.shl
; CHECK: %r.sclr3.promote.zext = shl i16 %r.sclr3.promote, 6
; CHECK: %r.sclr3.zext.shl = and i16 %r.sclr3.promote.zext, 192
; CHECK: %r.chunk3 = or i16 %r.chunk2, %r.sclr3.zext.shl
; CHECK: %r.sclr4.promote.zext = shl i16 %r.sclr4.promote, 8
; CHECK: %r.sclr4.zext.shl = and i16 %r.sclr4.promote.zext, 768
; CHECK: %r.chunk4 = or i16 %r.chunk3, %r.sclr4.zext.shl
; CHECK: %r.sclr5.promote.zext = shl i16 %r.sclr5.promote, 10
; CHECK: %r.sclr5.zext.shl = and i16 %r.sclr5.promote.zext, 3072
; CHECK: %r.chunk5 = or i16 %r.chunk4, %r.sclr5.zext.shl
; CHECK: %pc.ptrcast = bitcast <6 x i2>* %pc to i16*
; CHECK: store i16 %r.chunk5, i16* %pc.ptrcast, align 8
; CHECK: ret void


define void @f6(<6 x i33>* %pc, <6 x i33>* %pa, <6 x i33>* %pb, i1 %cond) {
  %a = load <6 x i33>* %pa
  %b = load <6 x i33>* %pb
  br i1 %cond, label %T, label %F

T:
  %r1 = and <6 x i33> %a, %b
  br label %exit

F:
  %r2 = or <6 x i33> %a, %b
  br label %exit

exit:
  %r = phi <6 x i33> [ %r1, %T ], [ %r2, %F ]
  store <6 x i33> %r, <6 x i33>* %pc
  ret void
}

; CHECK-LABEL: define void @f6
; CHECK: %pa.ptrcast.ex0 = bitcast <6 x i33>* %pa to i64*
; CHECK: %a.chunk.ex0 = load i64* %pa.ptrcast.ex0, align 64
; CHECK: %pa.ptrcast.ex1 = getelementptr inbounds i64* %pa.ptrcast.ex0, i32 1
; CHECK: %a.chunk.ex1 = load i64* %pa.ptrcast.ex1, align 8
; CHECK: %pa.ptrcast.ex2 = getelementptr inbounds i64* %pa.ptrcast.ex0, i32 2
; CHECK: %a.chunk.ex2 = load i64* %pa.ptrcast.ex2, align 16
; CHECK: %pa.ptrcast.ex3 = getelementptr inbounds i64* %pa.ptrcast.ex0, i32 3
; CHECK: %pa.ptrcast.ex3.ptrcast = bitcast i64* %pa.ptrcast.ex3 to i8*
; CHECK: %a.chunk.ex3 = load i8* %pa.ptrcast.ex3.ptrcast, align 8
; CHECK: %a.chunk.ex0.lshr = lshr i64 %a.chunk.ex0, 33
; CHECK: %a.chunk.ex1.shl = shl i64 %a.chunk.ex1, 31
; CHECK: %a.lshr1.ex0 = or i64 %a.chunk.ex0.lshr, %a.chunk.ex1.shl
; CHECK: %a.chunk.ex1.lshr2 = lshr i64 %a.chunk.ex1, 2
; CHECK: %a.chunk.ex2.shl3 = shl i64 %a.chunk.ex2, 62
; CHECK: %a.lshr2.ex0 = or i64 %a.chunk.ex1.lshr2, %a.chunk.ex2.shl3
; CHECK: %a.chunk.ex1.lshr6 = lshr i64 %a.chunk.ex1, 35
; CHECK: %a.chunk.ex2.shl7 = shl i64 %a.chunk.ex2, 29
; CHECK: %a.lshr3.ex0 = or i64 %a.chunk.ex1.lshr6, %a.chunk.ex2.shl7
; CHECK: %a.chunk.ex3.zext9 = zext i8 %a.chunk.ex3 to i64
; CHECK: %a.chunk.ex2.lshr10 = lshr i64 %a.chunk.ex2, 4
; CHECK: %a.chunk.ex3.zext9.shl = shl i64 %a.chunk.ex3.zext9, 60
; CHECK: %a.lshr4.ex0 = or i64 %a.chunk.ex2.lshr10, %a.chunk.ex3.zext9.shl
; CHECK: %a.chunk.ex3.zext11 = zext i8 %a.chunk.ex3 to i64
; CHECK: %a.chunk.ex2.lshr12 = lshr i64 %a.chunk.ex2, 37
; CHECK: %a.chunk.ex3.zext11.shl = shl nuw nsw i64 %a.chunk.ex3.zext11, 27
; CHECK: %a.lshr5.ex0 = or i64 %a.chunk.ex2.lshr12, %a.chunk.ex3.zext11.shl
; CHECK: %pb.ptrcast.ex0 = bitcast <6 x i33>* %pb to i64*
; CHECK: %b.chunk.ex0 = load i64* %pb.ptrcast.ex0, align 64
; CHECK: %pb.ptrcast.ex1 = getelementptr inbounds i64* %pb.ptrcast.ex0, i32 1
; CHECK: %b.chunk.ex1 = load i64* %pb.ptrcast.ex1, align 8
; CHECK: %pb.ptrcast.ex2 = getelementptr inbounds i64* %pb.ptrcast.ex0, i32 2
; CHECK: %b.chunk.ex2 = load i64* %pb.ptrcast.ex2, align 16
; CHECK: %pb.ptrcast.ex3 = getelementptr inbounds i64* %pb.ptrcast.ex0, i32 3
; CHECK: %pb.ptrcast.ex3.ptrcast = bitcast i64* %pb.ptrcast.ex3 to i8*
; CHECK: %b.chunk.ex3 = load i8* %pb.ptrcast.ex3.ptrcast, align 8
; CHECK: %b.chunk.ex0.lshr = lshr i64 %b.chunk.ex0, 33
; CHECK: %b.chunk.ex1.shl = shl i64 %b.chunk.ex1, 31
; CHECK: %b.lshr1.ex0 = or i64 %b.chunk.ex0.lshr, %b.chunk.ex1.shl
; CHECK: %b.chunk.ex1.lshr14 = lshr i64 %b.chunk.ex1, 2
; CHECK: %b.chunk.ex2.shl15 = shl i64 %b.chunk.ex2, 62
; CHECK: %b.lshr2.ex0 = or i64 %b.chunk.ex1.lshr14, %b.chunk.ex2.shl15
; CHECK: %b.chunk.ex1.lshr18 = lshr i64 %b.chunk.ex1, 35
; CHECK: %b.chunk.ex2.shl19 = shl i64 %b.chunk.ex2, 29
; CHECK: %b.lshr3.ex0 = or i64 %b.chunk.ex1.lshr18, %b.chunk.ex2.shl19
; CHECK: %b.chunk.ex3.zext21 = zext i8 %b.chunk.ex3 to i64
; CHECK: %b.chunk.ex2.lshr22 = lshr i64 %b.chunk.ex2, 4
; CHECK: %b.chunk.ex3.zext21.shl = shl i64 %b.chunk.ex3.zext21, 60
; CHECK: %b.lshr4.ex0 = or i64 %b.chunk.ex2.lshr22, %b.chunk.ex3.zext21.shl
; CHECK: %b.chunk.ex3.zext23 = zext i8 %b.chunk.ex3 to i64
; CHECK: %b.chunk.ex2.lshr24 = lshr i64 %b.chunk.ex2, 37
; CHECK: %b.chunk.ex3.zext23.shl = shl nuw nsw i64 %b.chunk.ex3.zext23, 27
; CHECK: %b.lshr5.ex0 = or i64 %b.chunk.ex2.lshr24, %b.chunk.ex3.zext23.shl
; CHECK: br i1 %cond, label %T, label %F

; CHECK: %r1.sclr0.promote = and i64 %a.chunk.ex0, %b.chunk.ex0
; CHECK: %r1.sclr1.promote = and i64 %a.lshr1.ex0, %b.lshr1.ex0
; CHECK: %r1.sclr2.promote = and i64 %a.lshr2.ex0, %b.lshr2.ex0
; CHECK: %r1.sclr3.promote = and i64 %a.lshr3.ex0, %b.lshr3.ex0
; CHECK: %r1.sclr4.promote = and i64 %a.lshr4.ex0, %b.lshr4.ex0
; CHECK: %r1.sclr5.promote = and i64 %a.lshr5.ex0, %b.lshr5.ex0
; CHECK: br label %exit

; CHECK: %r2.sclr0.promote = or i64 %a.chunk.ex0, %b.chunk.ex0
; CHECK: %r2.sclr1.promote = or i64 %a.lshr1.ex0, %b.lshr1.ex0
; CHECK: %r2.sclr2.promote = or i64 %a.lshr2.ex0, %b.lshr2.ex0
; CHECK: %r2.sclr3.promote = or i64 %a.lshr3.ex0, %b.lshr3.ex0
; CHECK: %r2.sclr4.promote = or i64 %a.lshr4.ex0, %b.lshr4.ex0
; CHECK: %r2.sclr5.promote = or i64 %a.lshr5.ex0, %b.lshr5.ex0
; CHECK: br label %exit

; CHECK: %r.sclr0.promote = phi i64 [ %r1.sclr0.promote, %T ], [ %r2.sclr0.promote, %F ]
; CHECK: %r.sclr1.promote = phi i64 [ %r1.sclr1.promote, %T ], [ %r2.sclr1.promote, %F ]
; CHECK: %r.sclr2.promote = phi i64 [ %r1.sclr2.promote, %T ], [ %r2.sclr2.promote, %F ]
; CHECK: %r.sclr3.promote = phi i64 [ %r1.sclr3.promote, %T ], [ %r2.sclr3.promote, %F ]
; CHECK: %r.sclr4.promote = phi i64 [ %r1.sclr4.promote, %T ], [ %r2.sclr4.promote, %F ]
; CHECK: %r.sclr5.promote = phi i64 [ %r1.sclr5.promote, %T ], [ %r2.sclr5.promote, %F ]
; CHECK: %r.sclr0.promote.zext = and i64 %r.sclr0.promote, 8589934591
; CHECK: %r.sclr1.zext.shl.ex0 = shl i64 %r.sclr1.promote, 33
; CHECK: %r.sclr1.promote.zext = lshr i64 %r.sclr1.promote, 31
; CHECK: %r.sclr1.promote.zext.rshl = and i64 %r.sclr1.promote.zext, 3
; CHECK: %r.sclr2.promote.zext = shl i64 %r.sclr2.promote, 2
; CHECK: %r.sclr2.promote.zext.shl = and i64 %r.sclr2.promote.zext, 34359738364
; CHECK: %r.chunk2.ex1 = or i64 %r.sclr1.promote.zext.rshl, %r.sclr2.promote.zext.shl
; CHECK: %r.sclr3.promote.zext.shl = shl i64 %r.sclr3.promote, 35
; CHECK: %r.sclr3.promote.zext = lshr i64 %r.sclr3.promote, 29
; CHECK: %r.sclr3.promote.zext.rshl = and i64 %r.sclr3.promote.zext, 15
; CHECK: %r.sclr4.promote.zext = shl i64 %r.sclr4.promote, 4
; CHECK: %r.sclr4.promote.zext.shl = and i64 %r.sclr4.promote.zext, 137438953456
; CHECK: %r.chunk4.ex2 = or i64 %r.sclr3.promote.zext.rshl, %r.sclr4.promote.zext.shl
; CHECK: %r.sclr5.promote.zext.shl = shl i64 %r.sclr5.promote, 37
; CHECK: %r.sclr5.promote.zext = lshr i64 %r.sclr5.promote, 27
; CHECK: %r.sclr5.promote.zext.tr = trunc i64 %r.sclr5.promote.zext to i8
; CHECK: %r.sclr5.promote.zext.rshl.trunc = and i8 %r.sclr5.promote.zext.tr, 63
; CHECK: %r.chunk5.ex0 = or i64 %r.sclr0.promote.zext, %r.sclr1.zext.shl.ex0
; CHECK: %r.chunk5.ex1 = or i64 %r.chunk2.ex1, %r.sclr3.promote.zext.shl
; CHECK: %r.chunk5.ex2 = or i64 %r.chunk4.ex2, %r.sclr5.promote.zext.shl
; CHECK: %pc.ptrcast.ex0 = bitcast <6 x i33>* %pc to i64*
; CHECK: store i64 %r.chunk5.ex0, i64* %pc.ptrcast.ex0, align 64
; CHECK: %pc.ptrcast.ex1 = getelementptr inbounds i64* %pc.ptrcast.ex0, i32 1
; CHECK: store i64 %r.chunk5.ex1, i64* %pc.ptrcast.ex1, align 8
; CHECK: %pc.ptrcast.ex2 = getelementptr inbounds i64* %pc.ptrcast.ex0, i32 2
; CHECK: store i64 %r.chunk5.ex2, i64* %pc.ptrcast.ex2, align 16
; CHECK: %pc.ptrcast.ex3 = getelementptr inbounds i64* %pc.ptrcast.ex0, i32 3
; CHECK: %pc.ptrcast.ex3.ptrcast = bitcast i64* %pc.ptrcast.ex3 to i8*
; CHECK: store i8 %r.sclr5.promote.zext.rshl.trunc, i8* %pc.ptrcast.ex3.ptrcast, align 8
; CHECK: ret void
