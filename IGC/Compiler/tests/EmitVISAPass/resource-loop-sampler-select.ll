;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopSamplerSelect merges the two arms of an if/else that sample the same non-uniform texture with
; different samplers into one sample in the dominating block, with a per-lane select of the sampler, so that the arms
; need one resource loop instead of two.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt --resource-loop-sampler-select -S %s 2>&1 | FileCheck %s

%__2D_DIM_Resource = type opaque

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_sampler_select(i32 %u, i32 %v, float addrspace(1)* %dst) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %base = zext i32 %u to i64
  %cond = icmp ult i32 %nonuniform, 8
  %f = uitofp i32 %v to float

; COM: the descriptor and sampler address computations of both arms move to the head, the samplers are selected per lane
; COM: and there is a single sample (it takes the sampler address space of the then arm)
; CHECK-LABEL: entry:
; CHECK: [[TEX:%.*]] = inttoptr i32 {{.*}} to %__2D_DIM_Resource{{[.0-9]*}} addrspace(2621440)*
; CHECK: [[SEL:%samplerSelect]] = select i1 %cond, i64 {{.*}}, i64
; CHECK: [[SAMP:%.*]] = inttoptr i64 [[SEL]] to i32 addrspace(2752512)*
; CHECK: [[S:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr.{{.*}}p2752512i32({{.*}}%__2D_DIM_Resource{{[.0-9]*}} addrspace(2621440)* [[TEX]], i32 addrspace(2752512)* [[SAMP]],
; CHECK: br i1 %cond, label %thenA, label %elseA
; CHECK: thenA:
; CHECK-NOT: GenISA.sample
; CHECK: elseA:
; CHECK-NOT: GenISA.sample
; CHECK: joinA:
  br i1 %cond, label %thenA, label %elseA

thenA:
  %ta.off = add i32 %nonuniform, 64
  %ta = inttoptr i32 %ta.off to %__2D_DIM_Resource addrspace(2621440)*
  %sa.addr = add i64 %base, 672
  %sa = inttoptr i64 %sa.addr to i32 addrspace(2752512)*
  %xa = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %ta, i32 addrspace(2752512)* %sa, i32 0, i32 0, i32 0)
  %va = extractelement <4 x float> %xa, i32 0
  br label %joinA

elseA:
  %eb.off = add i32 %nonuniform, 64
  %eb = inttoptr i32 %eb.off to %__2D_DIM_Resource addrspace(2621440)*
  %sb.addr = add i64 %base, 704
  %sb = inttoptr i64 %sb.addr to i32 addrspace(2752513)*
  %xb = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %eb, i32 addrspace(2752513)* %sb, i32 0, i32 0, i32 0)
  %vb = extractelement <4 x float> %xb, i32 0
  br label %joinA

joinA:
  %rA = phi float [ %va, %thenA ], [ %vb, %elseA ]
  %tb = inttoptr i32 %u to %__2D_DIM_Resource addrspace(2621440)*

; COM: a uniform texture needs no resource loop, and a per-lane sampler would add one: both arms stay
; CHECK: br i1 %cond, label %thenB, label %elseB
; CHECK: thenB:
; CHECK: GenISA.sampleLptr{{.*}}p2752512i32
; CHECK: elseB:
; CHECK: GenISA.sampleLptr{{.*}}p2752513i32
; CHECK: joinB:
  br i1 %cond, label %thenB, label %elseB

thenB:
  %sc = inttoptr i64 %base to i32 addrspace(2752512)*
  %xc = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tb, i32 addrspace(2752512)* %sc, i32 0, i32 0, i32 0)
  %vc = extractelement <4 x float> %xc, i32 0
  br label %joinB

elseB:
  %sd.addr = add i64 %base, 32
  %sd = inttoptr i64 %sd.addr to i32 addrspace(2752513)*
  %xd = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tb, i32 addrspace(2752513)* %sd, i32 0, i32 0, i32 0)
  %vd = extractelement <4 x float> %xd, i32 0
  br label %joinB

joinB:
  %rB = phi float [ %vc, %thenB ], [ %vd, %elseB ]

; COM: a sample that depends on the neighbouring lanes (implicit LOD) is not executed for the lanes of both arms
; CHECK: br i1 %cond, label %thenC, label %elseC
; CHECK: thenC:
; CHECK: GenISA.sampleptr{{.*}}p2752512i32
; CHECK: elseC:
; CHECK: GenISA.sampleptr{{.*}}p2752513i32
; CHECK: joinC:
  br i1 %cond, label %thenC, label %elseC

thenC:
  %tc.off = add i32 %nonuniform, 128
  %tc = inttoptr i32 %tc.off to %__2D_DIM_Resource addrspace(2621440)*
  %se = inttoptr i64 %base to i32 addrspace(2752512)*
  %xe = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tc, i32 addrspace(2752512)* %se, i32 0, i32 0, i32 0)
  %ve = extractelement <4 x float> %xe, i32 0
  br label %joinC

elseC:
  %td.off = add i32 %nonuniform, 128
  %td = inttoptr i32 %td.off to %__2D_DIM_Resource addrspace(2621440)*
  %sf.addr = add i64 %base, 64
  %sf = inttoptr i64 %sf.addr to i32 addrspace(2752513)*
  %xf = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %td, i32 addrspace(2752513)* %sf, i32 0, i32 0, i32 0)
  %vf = extractelement <4 x float> %xf, i32 0
  br label %joinC

joinC:
  %rC = phi float [ %ve, %thenC ], [ %vf, %elseC ]

; COM: arms that sample at different coordinates are not the same sample: they stay
; CHECK: br i1 %cond, label %thenD, label %elseD
; CHECK: thenD:
; CHECK: GenISA.sampleLptr{{.*}}p2752512i32
; CHECK: elseD:
; CHECK: GenISA.sampleLptr{{.*}}p2752513i32
; CHECK: joinD:
  br i1 %cond, label %thenD, label %elseD

thenD:
  %te.off = add i32 %nonuniform, 192
  %te = inttoptr i32 %te.off to %__2D_DIM_Resource addrspace(2621440)*
  %sg = inttoptr i64 %base to i32 addrspace(2752512)*
  %xg = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %te, i32 addrspace(2752512)* %sg, i32 0, i32 0, i32 0)
  %vg = extractelement <4 x float> %xg, i32 0
  br label %joinD

elseD:
  %tf.off = add i32 %nonuniform, 192
  %tf = inttoptr i32 %tf.off to %__2D_DIM_Resource addrspace(2621440)*
  %sh.addr = add i64 %base, 96
  %sh = inttoptr i64 %sh.addr to i32 addrspace(2752513)*
  %xh = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %f, float %f, float 0.000000e+00, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tf, i32 addrspace(2752513)* %sh, i32 0, i32 0, i32 0)
  %vh = extractelement <4 x float> %xh, i32 0
  br label %joinD

joinD:
  %rD = phi float [ %vg, %thenD ], [ %vh, %elseD ]

; COM: a direct sampler keeps its index in the address space, so it cannot be selected per lane: the arms stay
; COM: (sampler buffer type SAMPLER: indirect in the then arm, direct in the else arm)
; CHECK: br i1 %cond, label %thenE, label %elseE
; CHECK: thenE:
; CHECK: GenISA.sampleLptr{{.*}}p2686976i32
; CHECK: elseE:
; CHECK: GenISA.sampleLptr{{.*}}p589827i32
; CHECK: joinE:
  br i1 %cond, label %thenE, label %elseE

thenE:
  %ti.off = add i32 %nonuniform, 256
  %ti = inttoptr i32 %ti.off to %__2D_DIM_Resource addrspace(2621440)*
  %si = inttoptr i64 %base to i32 addrspace(2686976)*
  %xi = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2686976i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %ti, i32 addrspace(2686976)* %si, i32 0, i32 0, i32 0)
  %vi = extractelement <4 x float> %xi, i32 0
  br label %joinE

elseE:
  %tj.off = add i32 %nonuniform, 256
  %tj = inttoptr i32 %tj.off to %__2D_DIM_Resource addrspace(2621440)*
  %sj = inttoptr i64 %base to i32 addrspace(589827)*
  %xj = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p589827i32(float %f, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tj, i32 addrspace(589827)* %sj, i32 0, i32 0, i32 0)
  %vj = extractelement <4 x float> %xj, i32 0
  br label %joinE

joinE:
  %rE = phi float [ %vi, %thenE ], [ %vj, %elseE ]

; COM: the arms compute the coordinates the same way, one of them feeds the other and both are used twice: the else
; COM: arm's copies are dead after the merge, and deleting one must not free another that is still to be deleted
; CHECK: [[C:%.*]] = fmul float
; CHECK: [[D:%.*]] = fadd float [[C]], 1.000000e+00
; CHECK: %samplerSelect{{[0-9]*}} = select i1 %cond
; CHECK: call <4 x float> @llvm.genx.GenISA.sampleLptr.{{.*}}(float [[C]], float [[D]], float [[C]], float [[D]],
; CHECK: br i1 %cond, label %thenF, label %elseF
; CHECK: thenF:
; CHECK-NOT: GenISA.sample
; CHECK: elseF:
; CHECK-NOT: GenISA.sample
; CHECK-NOT: fmul
; CHECK: joinF:
  br i1 %cond, label %thenF, label %elseF

thenF:
  %tk.off = add i32 %nonuniform, 320
  %tk = inttoptr i32 %tk.off to %__2D_DIM_Resource addrspace(2621440)*
  %ck = fmul float %f, %f
  %dk = fadd float %ck, 1.000000e+00
  %sk = inttoptr i64 %base to i32 addrspace(2752512)*
  %xk = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %ck, float %dk, float %ck, float %dk, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tk, i32 addrspace(2752512)* %sk, i32 0, i32 0, i32 0)
  %vk = extractelement <4 x float> %xk, i32 0
  br label %joinF

elseF:
  %tl.off = add i32 %nonuniform, 320
  %tl = inttoptr i32 %tl.off to %__2D_DIM_Resource addrspace(2621440)*
  %cl = fmul float %f, %f
  %dl = fadd float %cl, 1.000000e+00
  %sl.addr = add i64 %base, 128
  %sl = inttoptr i64 %sl.addr to i32 addrspace(2752513)*
  %xl = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %cl, float %dl, float %cl, float %dl, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tl, i32 addrspace(2752513)* %sl, i32 0, i32 0, i32 0)
  %vl = extractelement <4 x float> %xl, i32 0
  br label %joinF

joinF:
  %rF = phi float [ %vk, %thenF ], [ %vl, %elseF ]

; COM: the same coordinate computation with different flags (nsw only in the then arm): only the then arm's copy would
; COM: be kept, so the else lanes would compute with flags they do not have: the arms stay
; CHECK: br i1 %cond, label %thenG, label %elseG
; CHECK: thenG:
; CHECK: GenISA.sampleLptr{{.*}}p2752512i32
; CHECK: elseG:
; CHECK: GenISA.sampleLptr{{.*}}p2752513i32
; CHECK: joinG:
  br i1 %cond, label %thenG, label %elseG

thenG:
  %tm.off = add i32 %nonuniform, 384
  %tm = inttoptr i32 %tm.off to %__2D_DIM_Resource addrspace(2621440)*
  %cm.i = add nsw i32 %u, 1
  %cm = sitofp i32 %cm.i to float
  %sm = inttoptr i64 %base to i32 addrspace(2752512)*
  %xm = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %cm, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tm, i32 addrspace(2752512)* %sm, i32 0, i32 0, i32 0)
  %vm = extractelement <4 x float> %xm, i32 0
  br label %joinG

elseG:
  %tn.off = add i32 %nonuniform, 384
  %tn = inttoptr i32 %tn.off to %__2D_DIM_Resource addrspace(2621440)*
  %cn.i = add i32 %u, 1
  %cn = sitofp i32 %cn.i to float
  %sn.addr = add i64 %base, 160
  %sn = inttoptr i64 %sn.addr to i32 addrspace(2752513)*
  %xn = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %cn, float %f, float %f, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tn, i32 addrspace(2752513)* %sn, i32 0, i32 0, i32 0)
  %vn = extractelement <4 x float> %xn, i32 0
  br label %joinG

joinG:
  %rG = phi float [ %vm, %thenG ], [ %vn, %elseG ]

; COM: two samples per arm that share the texture and the coordinate code: both pairs merge, and the code
; COM: shared between the pairs is hoisted and deleted once
; CHECK: [[HC:%.*]] = fmul float
; CHECK: %samplerSelect{{[0-9]*}} = select i1 %cond
; CHECK: GenISA.sampleLptr{{.*}}(float [[HC]], float [[HC]], float [[HC]]
; CHECK: [[HD:%.*]] = fadd float [[HC]], 1.000000e+00
; CHECK: %samplerSelect{{[0-9]*}} = select i1 %cond
; CHECK: GenISA.sampleLptr{{.*}}(float [[HD]], float [[HD]], float [[HD]]
; CHECK: br i1 %cond, label %thenH, label %elseH
; CHECK: thenH:
; CHECK-NOT: GenISA.sample
; CHECK: elseH:
; CHECK-NOT: GenISA.sample
; CHECK: joinH:
  br i1 %cond, label %thenH, label %elseH

thenH:
  %thT.off = add i32 %nonuniform, 448
  %thT = inttoptr i32 %thT.off to %__2D_DIM_Resource addrspace(2621440)*
  %chT = fmul float %f, %f
  %dhT = fadd float %chT, 1.000000e+00
  %shT = inttoptr i64 %base to i32 addrspace(2752512)*
  %x0hT = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %chT, float %chT, float %chT, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %thT, i32 addrspace(2752512)* %shT, i32 0, i32 0, i32 0)
  %v0hT = extractelement <4 x float> %x0hT, i32 0
  %x1hT = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %dhT, float %dhT, float %dhT, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %thT, i32 addrspace(2752512)* %shT, i32 0, i32 0, i32 0)
  %v1hT = extractelement <4 x float> %x1hT, i32 0
  br label %joinH

elseH:
  %thE.off = add i32 %nonuniform, 448
  %thE = inttoptr i32 %thE.off to %__2D_DIM_Resource addrspace(2621440)*
  %chE = fmul float %f, %f
  %dhE = fadd float %chE, 1.000000e+00
  %shE.addr = add i64 %base, 192
  %shE = inttoptr i64 %shE.addr to i32 addrspace(2752513)*
  %x0hE = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %chE, float %chE, float %chE, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %thE, i32 addrspace(2752513)* %shE, i32 0, i32 0, i32 0)
  %v0hE = extractelement <4 x float> %x0hE, i32 0
  %x1hE = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %dhE, float %dhE, float %dhE, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %thE, i32 addrspace(2752513)* %shE, i32 0, i32 0, i32 0)
  %v1hE = extractelement <4 x float> %x1hE, i32 0
  br label %joinH

joinH:
  %rH0 = phi float [ %v0hT, %thenH ], [ %v0hE, %elseH ]
  %rH1 = phi float [ %v1hT, %thenH ], [ %v1hE, %elseH ]
  %rH = fadd float %rH0, %rH1

; COM: bindless samplers with the same (direct-flagged) address space are interchangeable: the sampler is
; COM: the pointer value
; CHECK: %samplerSelect{{[0-9]*}} = select i1 %cond
; CHECK: GenISA.sampleLptr{{.*}}p655363i32
; CHECK: br i1 %cond, label %thenI, label %elseI
; CHECK: thenI:
; CHECK-NOT: GenISA.sample
; CHECK: elseI:
; CHECK-NOT: GenISA.sample
; CHECK: joinI:
  br i1 %cond, label %thenI, label %elseI

thenI:
  %tiT.off = add i32 %nonuniform, 512
  %tiT = inttoptr i32 %tiT.off to %__2D_DIM_Resource addrspace(2621440)*
  %ciT = fmul float %f, %f
  %diT = fadd float %ciT, 1.000000e+00
  %siT = inttoptr i64 %base to i32 addrspace(655363)*
  %x0iT = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p655363i32(float %ciT, float %ciT, float %ciT, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tiT, i32 addrspace(655363)* %siT, i32 0, i32 0, i32 0)
  %v0iT = extractelement <4 x float> %x0iT, i32 0
  br label %joinI

elseI:
  %tiE.off = add i32 %nonuniform, 512
  %tiE = inttoptr i32 %tiE.off to %__2D_DIM_Resource addrspace(2621440)*
  %ciE = fmul float %f, %f
  %diE = fadd float %ciE, 1.000000e+00
  %siE.addr = add i64 %base, 64
  %siE = inttoptr i64 %siE.addr to i32 addrspace(655363)*
  %x0iE = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p655363i32(float %ciE, float %ciE, float %ciE, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tiE, i32 addrspace(655363)* %siE, i32 0, i32 0, i32 0)
  %v0iE = extractelement <4 x float> %x0iE, i32 0
  br label %joinI

joinI:
  %rI = phi float [ %v0iT, %thenI ], [ %v0iE, %elseI ]

; COM: the arms take the same coordinates in a different order: the samples do not pair up, so the arms stay
; CHECK: br i1 %cond, label %thenJ, label %elseJ
; CHECK: thenJ:
; CHECK: GenISA.sampleLptr{{.*}}p2752512i32
; CHECK: GenISA.sampleLptr{{.*}}p2752512i32
; CHECK: elseJ:
; CHECK: GenISA.sampleLptr{{.*}}p2752513i32
; CHECK: GenISA.sampleLptr{{.*}}p2752513i32
; CHECK: joinJ:
  br i1 %cond, label %thenJ, label %elseJ

thenJ:
  %tjT.off = add i32 %nonuniform, 576
  %tjT = inttoptr i32 %tjT.off to %__2D_DIM_Resource addrspace(2621440)*
  %cjT = fmul float %f, %f
  %djT = fadd float %cjT, 1.000000e+00
  %sjT = inttoptr i64 %base to i32 addrspace(2752512)*
  %x0jT = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %cjT, float %cjT, float %cjT, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tjT, i32 addrspace(2752512)* %sjT, i32 0, i32 0, i32 0)
  %v0jT = extractelement <4 x float> %x0jT, i32 0
  %x1jT = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %djT, float %djT, float %djT, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tjT, i32 addrspace(2752512)* %sjT, i32 0, i32 0, i32 0)
  %v1jT = extractelement <4 x float> %x1jT, i32 0
  br label %joinJ

elseJ:
  %tjE.off = add i32 %nonuniform, 576
  %tjE = inttoptr i32 %tjE.off to %__2D_DIM_Resource addrspace(2621440)*
  %cjE = fmul float %f, %f
  %djE = fadd float %cjE, 1.000000e+00
  %sjE.addr = add i64 %base, 256
  %sjE = inttoptr i64 %sjE.addr to i32 addrspace(2752513)*
  %x0jE = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %djE, float %djE, float %djE, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tjE, i32 addrspace(2752513)* %sjE, i32 0, i32 0, i32 0)
  %v0jE = extractelement <4 x float> %x0jE, i32 0
  %x1jE = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float %cjE, float %cjE, float %cjE, float %f, float %f, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %tjE, i32 addrspace(2752513)* %sjE, i32 0, i32 0, i32 0)
  %v1jE = extractelement <4 x float> %x1jE, i32 0
  br label %joinJ

joinJ:
  %rJ0 = phi float [ %v0jT, %thenJ ], [ %v0jE, %elseJ ]
  %rJ1 = phi float [ %v1jT, %thenJ ], [ %v1jE, %elseJ ]
  %rJ = fadd float %rJ0, %rJ1
  %s0 = fadd float %rA, %rB
  %s1 = fadd float %s0, %rC
  %s2 = fadd float %s1, %rD
  %s3 = fadd float %s2, %rE
  %s4 = fadd float %s3, %rF
  %s5 = fadd float %s4, %rG
  %s6 = fadd float %s5, %rH
  %s7 = fadd float %s6, %rI
  %s8 = fadd float %s7, %rJ
  store float %s8, float addrspace(1)* %dst, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752512)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752513)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p655363i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(655363)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2686976i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2686976)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p589827i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(589827)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752512)*, i32, i32, i32) #0
declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752513i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752513)*, i32, i32, i32) #0
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32, float addrspace(1)*)* @test_sampler_select}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!17 = !{!"m_OpenCLArgTypeQualifiers", !18, !19, !20}
!18 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!19 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!21 = !{void (i32, i32, float addrspace(1)*)* @test_sampler_select, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
