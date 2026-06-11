;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
;
; Test that llvm.uadd/usub.with.overflow are lowered to genx.addc/subb.
; Covers scalar i32, vector <16 x i32>, and vector <16 x i64>.

; CHECK-LABEL: @test_uadd_i32
; CHECK: [[ADDC:%[^ ]*]] = call { i32, i32 } @llvm.genx.addc.i32.i32(i32 %a, i32 %b)
; CHECK: [[RES:%[^ ]*]] = extractvalue { i32, i32 } [[ADDC]], 1
; CHECK: [[CARRY:%[^ ]*]] = extractvalue { i32, i32 } [[ADDC]], 0
; CHECK: [[OVF:%[^ ]*]] = icmp ne i32 [[CARRY]], 0
; CHECK: insertvalue { i32, i1 } poison, i32 [[RES]], 0
; CHECK: insertvalue { i32, i1 } {{.*}}, i1 [[OVF]], 1
define spir_kernel void @test_uadd_i32(i32 %a, i32 %b, i32 addrspace(1)* %out) #0 {
  %r = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %r, 0
  store i32 %val, i32 addrspace(1)* %out
  ret void
}

; CHECK-LABEL: @test_usub_i32
; CHECK: [[SUBB:%[^ ]*]] = call { i32, i32 } @llvm.genx.subb.i32.i32(i32 %a, i32 %b)
; CHECK: [[RES:%[^ ]*]] = extractvalue { i32, i32 } [[SUBB]], 1
; CHECK: [[BORROW:%[^ ]*]] = extractvalue { i32, i32 } [[SUBB]], 0
; CHECK: [[OVF:%[^ ]*]] = icmp ne i32 [[BORROW]], 0
; CHECK: insertvalue { i32, i1 } poison, i32 [[RES]], 0
; CHECK: insertvalue { i32, i1 } {{.*}}, i1 [[OVF]], 1
define spir_kernel void @test_usub_i32(i32 %a, i32 %b, i32 addrspace(1)* %out) #0 {
  %r = call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %r, 0
  store i32 %val, i32 addrspace(1)* %out
  ret void
}

; CHECK-LABEL: @test_uadd_v16i32
; CHECK: [[ADDC:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> %a, <16 x i32> %b)
; CHECK: [[RES:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC]], 1
; CHECK: [[CARRY:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC]], 0
; CHECK: [[OVF:%[^ ]*]] = icmp ne <16 x i32> [[CARRY]], zeroinitializer
; CHECK: insertvalue { <16 x i32>, <16 x i1> } poison, <16 x i32> [[RES]], 0
; CHECK: insertvalue { <16 x i32>, <16 x i1> } {{.*}}, <16 x i1> [[OVF]], 1
define spir_kernel void @test_uadd_v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> addrspace(1)* %out) #0 {
  %r = call { <16 x i32>, <16 x i1> } @llvm.uadd.with.overflow.v16i32(<16 x i32> %a, <16 x i32> %b)
  %val = extractvalue { <16 x i32>, <16 x i1> } %r, 0
  store <16 x i32> %val, <16 x i32> addrspace(1)* %out
  ret void
}

; CHECK-LABEL: @test_usub_v16i32
; CHECK: [[SUBB:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.subb.v16i32.v16i32(<16 x i32> %a, <16 x i32> %b)
; CHECK: [[RES:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[SUBB]], 1
; CHECK: [[BORROW:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[SUBB]], 0
; CHECK: [[OVF:%[^ ]*]] = icmp ne <16 x i32> [[BORROW]], zeroinitializer
; CHECK: insertvalue { <16 x i32>, <16 x i1> } poison, <16 x i32> [[RES]], 0
; CHECK: insertvalue { <16 x i32>, <16 x i1> } {{.*}}, <16 x i1> [[OVF]], 1
define spir_kernel void @test_usub_v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> addrspace(1)* %out) #0 {
  %r = call { <16 x i32>, <16 x i1> } @llvm.usub.with.overflow.v16i32(<16 x i32> %a, <16 x i32> %b)
  %val = extractvalue { <16 x i32>, <16 x i1> } %r, 0
  store <16 x i32> %val, <16 x i32> addrspace(1)* %out
  ret void
}

; Test that both result and zext(overflow) are used — exercises the path where
; lowerCast(zext <16xi1>) fires before lowerOverflowIntrinsic reaches the same zext.
; Optimal: store sees GenXCarry directly (borrow), no select.
; CHECK-LABEL: @test_usub_v16i32_with_overflow_zext
; CHECK: [[SUBB:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.subb.v16i32.v16i32(<16 x i32> %a, <16 x i32> %b)
; CHECK: [[BORROW:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[SUBB]], 0
; CHECK-NOT: select
; CHECK: store <16 x i32> [[BORROW]]
define spir_kernel void @test_usub_v16i32_with_overflow_zext(<16 x i32> %a, <16 x i32> %b, <16 x i32> addrspace(1)* %out0, <16 x i32> addrspace(1)* %out1) #0 {
  %r = call { <16 x i32>, <16 x i1> } @llvm.usub.with.overflow.v16i32(<16 x i32> %a, <16 x i32> %b)
  %val = extractvalue { <16 x i32>, <16 x i1> } %r, 0
  %ovf = extractvalue { <16 x i32>, <16 x i1> } %r, 1
  %ovf_ext = zext <16 x i1> %ovf to <16 x i32>
  store <16 x i32> %val, <16 x i32> addrspace(1)* %out0
  store <16 x i32> %ovf_ext, <16 x i32> addrspace(1)* %out1
  ret void
}

; CHECK-LABEL: @test_usub_v16i64
; CHECK: {{%[^ ]*}} = bitcast <16 x i64> %a to <32 x i32>
; CHECK: [[ALO:%[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni{{.*}}(<32 x i32> {{.*}}, i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK: [[AHI:%[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni{{.*}}(<32 x i32> {{.*}}, i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK: {{%[^ ]*}} = bitcast <16 x i64> %b to <32 x i32>
; CHECK: [[BLO:%[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni{{.*}}(<32 x i32> {{.*}}, i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK: [[BHI:%[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni{{.*}}(<32 x i32> {{.*}}, i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK: [[SUBBLO:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.subb.v16i32.v16i32(<16 x i32> [[ALO]], <16 x i32> [[BLO]])
; CHECK: extractvalue { <16 x i32>, <16 x i32> } [[SUBBLO]], 1
; CHECK: extractvalue { <16 x i32>, <16 x i32> } [[SUBBLO]], 0
; CHECK: [[ADD3:%[^ ]*]] = call { <16 x i1>, <16 x i32> } @llvm.genx.add3c{{.*}}(<16 x i32> [[AHI]], <16 x i32> {{.*}}, <16 x i32> {{.*}})
; CHECK: extractvalue { <16 x i1>, <16 x i32> } [[ADD3]], 1
; CHECK: extractvalue { <16 x i1>, <16 x i32> } [[ADD3]], 0
; CHECK: insertvalue { <16 x i64>, <16 x i1> } poison, <16 x i64> {{.*}}, 0
; CHECK: insertvalue { <16 x i64>, <16 x i1> } {{.*}}, <16 x i1> {{.*}}, 1
define spir_kernel void @test_usub_v16i64(<16 x i64> %a, <16 x i64> %b, <16 x i64> addrspace(1)* %out) #0 {
  %r = call { <16 x i64>, <16 x i1> } @llvm.usub.with.overflow.v16i64(<16 x i64> %a, <16 x i64> %b)
  %val = extractvalue { <16 x i64>, <16 x i1> } %r, 0
  store <16 x i64> %val, <16 x i64> addrspace(1)* %out
  ret void
}

declare { i32, i1 }               @llvm.uadd.with.overflow.i32(i32, i32)
declare { i32, i1 }               @llvm.usub.with.overflow.i32(i32, i32)
declare { <16 x i32>, <16 x i1> } @llvm.uadd.with.overflow.v16i32(<16 x i32>, <16 x i32>)
declare { <16 x i32>, <16 x i1> } @llvm.usub.with.overflow.v16i32(<16 x i32>, <16 x i32>)
declare { <16 x i64>, <16 x i1> } @llvm.usub.with.overflow.v16i64(<16 x i64>, <16 x i64>)

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }
