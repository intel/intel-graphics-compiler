;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

;
; Variants with output seed pointer
;

; CHECK-LABEL: .kernel "Test_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=32
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=32
declare spir_func signext i8 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDhiPi(half, i32, i32*)
define spir_kernel void @Test_ClampStochasticRoundFP16ToE5M2(half addrspace(1)* %inbuf, i8 addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds half, half addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load half, half addrspace(1)* %inputAddr, align 2
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func signext i8 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDhiPi(half %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4
  %outputAddr = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %globalId0
  store i8 %result, i8 addrspace(1)* %outputAddr, align 1
  ret void
}

; CHECK-LABEL: .kernel "Test2_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=64
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=64
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=64
declare spir_func <2 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv2_DhiPi(<2 x half>, i32, i32*)
define spir_kernel void @Test2_ClampStochasticRoundFP16ToE5M2(<2 x half> addrspace(1)* %inbuf, <2 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <2 x half>, <2 x half> addrspace(1)* %inputAddr, align 4
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <2 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv2_DhiPi(<2 x half> %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4

  %outputAddr = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <2 x i8> %result, <2 x i8> addrspace(1)* %outputAddr, align 2
  ret void
}

; CHECK-LABEL: .kernel "Test3_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=96
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=96
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=96
declare spir_func <3 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv3_DhiPi(<3 x half>, i32, i32*)
define spir_kernel void @Test3_ClampStochasticRoundFP16ToE5M2(<3 x half> addrspace(1)* %inbuf, <3 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <3 x half>, <3 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <3 x half>, <3 x half> addrspace(1)* %inputAddr, align 8
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <3 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv3_DhiPi(<3 x half> %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4

  %outputAddr = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <3 x i8> %result, <3 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test4_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=128
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=128
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=128
declare spir_func <4 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv4_DhiPi(<4 x half>, i32, i32*)
define spir_kernel void @Test4_ClampStochasticRoundFP16ToE5M2(<4 x half> addrspace(1)* %inbuf, <4 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <4 x half>, <4 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <4 x half>, <4 x half> addrspace(1)* %inputAddr, align 8
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <4 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv4_DhiPi(<4 x half> %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4

  %outputAddr = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <4 x i8> %result, <4 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test8_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0> [[SEED]](2,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0> [[SEED]](2,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0> [[SEED]](3,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0> [[SEED]](3,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=256
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=256
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=256
declare spir_func <8 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv8_DhiPi(<8 x half>, i32, i32*)
define spir_kernel void @Test8_ClampStochasticRoundFP16ToE5M2(<8 x half> addrspace(1)* %inbuf, <8 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <8 x half>, <8 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <8 x half>, <8 x half> addrspace(1)* %inputAddr, align 16
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <8 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv8_DhiPi(<8 x half> %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4

  %outputAddr = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <8 x i8> %result, <8 x i8> addrspace(1)* %outputAddr, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test16_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0> [[SEED]](2,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0> [[SEED]](2,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0> [[SEED]](3,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0> [[SEED]](3,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](4,0)<1> [[IN]](8,0)<1;1,0> [[SEED]](4,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](4,32)<1> [[IN]](9,0)<1;1,0> [[SEED]](4,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](5,0)<1> [[IN]](10,0)<1;1,0> [[SEED]](5,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](5,32)<1> [[IN]](11,0)<1;1,0> [[SEED]](5,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](6,0)<1> [[IN]](12,0)<1;1,0> [[SEED]](6,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](6,32)<1> [[IN]](13,0)<1;1,0> [[SEED]](6,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](7,0)<1> [[IN]](14,0)<1;1,0> [[SEED]](7,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](7,32)<1> [[IN]](15,0)<1;1,0> [[SEED]](7,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=512
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=512
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=512
declare spir_func <16 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv16_DhiPi(<16 x half>, i32, i32*)
define spir_kernel void @Test16_ClampStochasticRoundFP16ToE5M2(<16 x half> addrspace(1)* %inbuf, <16 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed, i32 addrspace(1)* %outseed) {
entry:
  %privateSeed = alloca i32, align 4
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <16 x half>, <16 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <16 x half>, <16 x half> addrspace(1)* %inputAddr, align 32
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <16 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv16_DhiPi(<16 x half> %inputLoaded, i32 %seedLoaded, i32* %privateSeed)

  %loadedPrivateSeed = load i32, i32* %privateSeed, align 4
  %outSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %outseed, i64 %globalId0
  store i32 %loadedPrivateSeed, i32 addrspace(1)* %outSeedAddr, align 4

  %outputAddr = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <16 x i8> %result, <16 x i8> addrspace(1)* %outputAddr, align 16
  ret void
}

;
; Variants without output seed pointer
;

; CHECK-LABEL: .kernel "TestNoSeedPtr_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=32
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=32
declare spir_func signext i8 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDhi(half, i32)
define spir_kernel void @TestNoSeedPtr_ClampStochasticRoundFP16ToE5M2(half addrspace(1)* %inbuf, i8 addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds half, half addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load half, half addrspace(1)* %inputAddr, align 2
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func signext i8 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDhi(half %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %globalId0
  store i8 %result, i8 addrspace(1)* %outputAddr, align 1
  ret void
}

; CHECK-LABEL: .kernel "TestNoSeedPtr2_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=64
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=64
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=64
declare spir_func <2 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv2_Dhi(<2 x half>, i32)
define spir_kernel void @TestNoSeedPtr2_ClampStochasticRoundFP16ToE5M2(<2 x half> addrspace(1)* %inbuf, <2 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <2 x half>, <2 x half> addrspace(1)* %inputAddr, align 4
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <2 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv2_Dhi(<2 x half> %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <2 x i8> %result, <2 x i8> addrspace(1)* %outputAddr, align 2
  ret void
}

; CHECK-LABEL: .kernel "TestNoSeedPtr3_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=96
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=96
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=96
declare spir_func <3 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv3_Dhi(<3 x half>, i32)
define spir_kernel void @TestNoSeedPtr3_ClampStochasticRoundFP16ToE5M2(<3 x half> addrspace(1)* %inbuf, <3 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <3 x half>, <3 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <3 x half>, <3 x half> addrspace(1)* %inputAddr, align 8
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <3 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv3_Dhi(<3 x half> %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <3 x i8> %result, <3 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}

; CHECK-LABEL: .kernel "TestNoSeedPtr4_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=128
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=128
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=128
declare spir_func <4 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv4_Dhi(<4 x half>, i32)
define spir_kernel void @TestNoSeedPtr4_ClampStochasticRoundFP16ToE5M2(<4 x half> addrspace(1)* %inbuf, <4 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <4 x half>, <4 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <4 x half>, <4 x half> addrspace(1)* %inputAddr, align 8
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <4 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv4_Dhi(<4 x half> %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <4 x i8> %result, <4 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}

; CHECK-LABEL: .kernel "TestNoSeedPtr8_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0> [[SEED]](2,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0> [[SEED]](2,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0> [[SEED]](3,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0> [[SEED]](3,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=256
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=256
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=256
declare spir_func <8 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv8_Dhi(<8 x half>, i32)
define spir_kernel void @TestNoSeedPtr8_ClampStochasticRoundFP16ToE5M2(<8 x half> addrspace(1)* %inbuf, <8 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <8 x half>, <8 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <8 x half>, <8 x half> addrspace(1)* %inputAddr, align 16
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <8 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv8_Dhi(<8 x half> %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <8 x i8> %result, <8 x i8> addrspace(1)* %outputAddr, align 8
  ret void
}

; CHECK-LABEL: .kernel "TestNoSeedPtr16_ClampStochasticRoundFP16ToE5M2"
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: lfsr.b8v4 (M1, 32)
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0> [[SEED:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0> [[SEED]](0,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0> [[SEED]](1,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0> [[SEED]](1,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0> [[SEED]](2,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0> [[SEED]](2,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0> [[SEED]](3,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0> [[SEED]](3,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](4,0)<1> [[IN]](8,0)<1;1,0> [[SEED]](4,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](4,32)<1> [[IN]](9,0)<1;1,0> [[SEED]](4,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](5,0)<1> [[IN]](10,0)<1;1,0> [[SEED]](5,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](5,32)<1> [[IN]](11,0)<1;1,0> [[SEED]](5,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](6,0)<1> [[IN]](12,0)<1;1,0> [[SEED]](6,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](6,32)<1> [[IN]](13,0)<1;1,0> [[SEED]](6,32)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](7,0)<1> [[IN]](14,0)<1;1,0> [[SEED]](7,0)<1;1,0>
; CHECK-DAG: srnd.sat (M1_NM, 32) [[OUT]](7,32)<1> [[IN]](15,0)<1;1,0> [[SEED]](7,32)<1;1,0>
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=512
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=512
; CHECK-DAG: .decl [[SEED]] v_type=G type=ub num_elts=512
declare spir_func <16 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv16_Dhi(<16 x half>, i32)
define spir_kernel void @TestNoSeedPtr16_ClampStochasticRoundFP16ToE5M2(<16 x half> addrspace(1)* %inbuf, <16 x i8> addrspace(1)* %outbuf, i32 addrspace(1)* %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <16 x half>, <16 x half> addrspace(1)* %inbuf, i64 %globalId0
  %inSeedAddr = getelementptr inbounds i32, i32 addrspace(1)* %inseed, i64 %globalId0

  %inputLoaded = load <16 x half>, <16 x half> addrspace(1)* %inputAddr, align 32
  %seedLoaded = load i32, i32 addrspace(1)* %inSeedAddr, align 4
  %result = call spir_func <16 x i8> @_Z51__builtin_spirv_ClampStochasticRoundFP16ToE5M2INTELDv16_Dhi(<16 x half> %inputLoaded, i32 %seedLoaded)

  %outputAddr = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %outbuf, i64 %globalId0
  store <16 x i8> %result, <16 x i8> addrspace(1)* %outputAddr, align 16
  ret void
}
