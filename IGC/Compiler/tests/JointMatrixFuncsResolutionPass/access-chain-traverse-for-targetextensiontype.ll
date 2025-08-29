;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; Walk thru uses in order to figure out the TET type

%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.3" = type { target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0) }

; CHECK:       define spir_kernel void @test()
; CHECK:       %addressCast = addrspacecast ptr %gep3 to ptr addrspace(4)
; CHECK:       %bitcast2 = bitcast ptr addrspace(4) %addressCast to ptr addrspace(4)
; CHECK-NEXT:  %0 = load <8 x i16>, ptr addrspace(4) %bitcast2, align 8

; Resolution flow: Call operand0 -> Bitcast -> Address Space Cast -> GEP -> TET

; Function Attrs: nounwind
define spir_kernel void @test() {
entry:
    %alloca = alloca target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0), align 8
    %gep = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.3", ptr %alloca, i64 0, i32 0
    %addressCast = addrspacecast ptr %gep to ptr addrspace(4)
    %bitcast2 = bitcast ptr addrspace(4) %addressCast to ptr addrspace(4)
    %call = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0l(ptr addrspace(4) %bitcast2, i64 0)

    store i16 5, ptr addrspace(4) %call, align 2
    ret void
}

; Function Attrs: nounwind
declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0l(ptr addrspace(4), i64) #0
