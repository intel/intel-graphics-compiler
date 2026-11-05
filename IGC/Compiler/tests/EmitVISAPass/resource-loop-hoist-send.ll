;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink moves a send of another resource, and read-only code, below a run of same-resource
; sends, so two sends of one non-uniform texture that are apart can share a resource loop. Writes and sends that need
; the result of the send in between keep the run apart.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s
; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt -regkey HoistResourceLoopSends=0 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s --check-prefix=OFF

%__2D_DIM_Resource = type opaque

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_hoist_same_resource_send(i32 %u, i32 %v, float addrspace(1)* %dst, float addrspace(1)* %src) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %base = zext i32 %u to i64
  %smp = inttoptr i64 %base to i32 addrspace(2752512)*
  %f = uitofp i32 %v to float
  %g = fadd float %f, 1.000000e+00
  %texA.off = add i32 %nonuniform, 64
  %texA = inttoptr i32 %texA.off to %__2D_DIM_Resource addrspace(2621440)*
  %texB.off = add i32 %nonuniform, 128
  %texB = inttoptr i32 %texB.off to %__2D_DIM_Resource addrspace(2621440)*

; COM: the send of texture B between the two sends of texture A, and the sqrt of its result, move below the second send
; CHECK-LABEL: entry:
; CHECK: %a1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %a2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}!igc.resource.loop.prepared
; CHECK: %b1 = call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK: %q = call float @llvm.sqrt.f32
; COM: with the key off the sends stay apart
; OFF-LABEL: entry:
; OFF: %a1 = call <4 x float> @llvm.genx.GenISA.sampleLptr
; OFF: %b1 = call <4 x float> @llvm.genx.GenISA.sampleLptr
; OFF: %q = call float @llvm.sqrt.f32
; OFF: %a2 = call <4 x float> @llvm.genx.GenISA.sampleLptr
  %a1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %b1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texB, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %b1.x = extractelement <4 x float> %b1, i32 0
  %q = call float @llvm.sqrt.f32(float %b1.x)
  %a2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %a1.x = extractelement <4 x float> %a1, i32 0
  %a2.x = extractelement <4 x float> %a2, i32 0
  %s0 = fadd float %a1.x, %a2.x
  %s1 = fadd float %s0, %q
  br label %write

; COM: the sends are anchored on the end of the line, so metadata on either send fails the match
; COM: a store between the sends of texture A is a barrier: nothing moves
; CHECK-LABEL: write:
; CHECK: %c1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NEXT: store float
; CHECK: %c2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NOT: !igc.resource.loop.prepared
write:
  %c1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  store float %s1, float addrspace(1)* %dst, align 4
  %c2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %c1.x = extractelement <4 x float> %c1, i32 0
  %c2.x = extractelement <4 x float> %c2, i32 0
  %s2 = fadd float %c1.x, %c2.x
  br label %dep

; COM: the second send of texture A takes the result of the send of texture B as a coordinate, so it cannot move up
; CHECK-LABEL: dep:
; CHECK: %d1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK: %e1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK: %d2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NOT: !igc.resource.loop.prepared
dep:
  %d1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %e1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texB, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %e1.x = extractelement <4 x float> %e1, i32 0
  %d2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %e1.x, float %e1.x, float %e1.x, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %d1.x = extractelement <4 x float> %d1, i32 0
  %d2.x = extractelement <4 x float> %d2, i32 0
  %s3 = fadd float %d1.x, %d2.x
  %s4 = fadd float %s3, %s2
  br label %read

; COM: a simple load and a uniform ldraw between the sends of texture A only read, so they move below the second send
; CHECK-LABEL: read:
; CHECK: %r1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %r2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}!igc.resource.loop.prepared
; CHECK: %ld = load float
; CHECK: %ldr = call float @llvm.genx.GenISA.ldraw.indexed
read:
  %r1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %ld = load float, float addrspace(1)* %src, align 4
  %ldr = call float @llvm.genx.GenISA.ldraw.indexed.f32.p1f32(float addrspace(1)* %src, i32 4, i32 4, i1 false)
  %r2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %r1.x = extractelement <4 x float> %r1, i32 0
  %r2.x = extractelement <4 x float> %r2, i32 0
  %s5 = fadd float %r1.x, %r2.x
  %s6 = fadd float %s5, %ld
  %s7 = fadd float %s6, %ldr
  br label %vol

; COM: a volatile load is not a plain read: it keeps the sends apart
; CHECK-LABEL: vol:
; CHECK: %v1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NEXT: %vl = load volatile float
; CHECK-NEXT: %v2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
vol:
  %v1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %vl = load volatile float, float addrspace(1)* %src, align 4
  %v2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %v1.x = extractelement <4 x float> %v1, i32 0
  %v2.x = extractelement <4 x float> %v2, i32 0
  %s8 = fadd float %v1.x, %v2.x
  %s9 = fadd float %s8, %vl
  br label %volraw

; COM: a volatile ldraw, uniform or of another lane-varying resource, is a volatile read like a volatile load: it keeps the sends apart
; CHECK-LABEL: volraw:
; CHECK: %x1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NEXT: %xl = call float @llvm.genx.GenISA.ldraw.indexed{{.*}}i1 true
; CHECK-NEXT: %x2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-LABEL: volraw.varying:
; CHECK: %y1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NEXT: %yl = call float @llvm.genx.GenISA.ldraw.indexed{{.*}}i1 true
; CHECK-NEXT: %y2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
volraw:
  %x1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %xl = call float @llvm.genx.GenISA.ldraw.indexed.f32.p1f32(float addrspace(1)* %src, i32 4, i32 4, i1 true)
  %x2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %x1.x = extractelement <4 x float> %x1, i32 0
  %x2.x = extractelement <4 x float> %x2, i32 0
  %sx = fadd float %x1.x, %x2.x
  %sy0 = fadd float %sx, %xl
  br label %volraw.varying

volraw.varying:
  %rawbuf = inttoptr i32 %texB.off to float addrspace(458753)*
  %y1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %yl = call float @llvm.genx.GenISA.ldraw.indexed.f32.p458753f32(float addrspace(458753)* %rawbuf, i32 4, i32 4, i1 true)
  %y2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %y1.x = extractelement <4 x float> %y1, i32 0
  %y2.x = extractelement <4 x float> %y2, i32 0
  %sy1 = fadd float %y1.x, %y2.x
  %sy2 = fadd float %sy1, %yl
  %sy = fadd float %sy0, %sy2
  br label %conv

; COM: a convergent call depends on the active lanes: it keeps the sends apart
; CHECK-LABEL: conv:
; CHECK: %w1 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
; CHECK-NEXT: %cv = call float @convergent_fn
; CHECK-NEXT: %w2 = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}i32 0){{$}}
conv:
  %w1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %cv = call float @convergent_fn(float %f)
  %w2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %g, float %g, float %g, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %texA, i32 addrspace(2752512)* %smp, i32 0, i32 0, i32 0)
  %w1.x = extractelement <4 x float> %w1, i32 0
  %w2.x = extractelement <4 x float> %w2, i32 0
  %s10 = fadd float %w1.x, %w2.x
  %s11 = fadd float %s10, %cv
  %s12 = fadd float %s4, %s7
  %s13 = fadd float %s12, %s9
  %s14a = fadd float %s13, %s11
  %s14 = fadd float %s14a, %sy
  store float %s14, float addrspace(1)* %dst, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752512)*, i32, i32, i32) #0
declare float @llvm.sqrt.f32(float) #0
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p1f32(float addrspace(1)*, i32, i32, i1) #3
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p458753f32(float addrspace(458753)*, i32, i32, i1) #3
declare float @convergent_fn(float) #2
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #0

attributes #0 = { nounwind readnone }
attributes #2 = { nounwind readnone willreturn convergent }
attributes #3 = { nounwind readonly willreturn argmemonly }
