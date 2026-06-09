;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check __builtin_IB_is_normalized_coords is replaced with implicit arg %smpNormalized
; for bindless sampler (offset is or'ed with 1).

; RUN: igc_opt -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque
%spirv.Sampler = type opaque

declare i32 @__builtin_IB_is_normalized_coords(i32 %sampler)

define i32 @foo(%spirv.Sampler addrspace(2)* %sampler, i32 %smpNormalized) nounwind {
  %1 = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
  %2 = or i64 %1, 1
  %3 = trunc i64 %2 to i32
  %id = call i32 @__builtin_IB_is_normalized_coords(i32 %3)
  ret i32 %id
}

!igc.input.ir = !{!100}
!100 = !{!"ocl", i32 1, i32 2}

!igc.functions = !{!0}
!0 = !{i32 (%spirv.Sampler addrspace(2)*, i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!101 = !{!"argId", i32 30}
!102 = !{!"explicitArgNum", i32 0}
!103 = !{!"implicitArgInfoListVec[0]", !101, !102}
!104 = !{!"implicitArgInfoList", !103}
!105 = !{!"FuncMDMap[0]", i32 (%spirv.Sampler addrspace(2)*, i32)* @foo}
!106 = !{!"FuncMDValue[0]", !104}
!107 = !{!"FuncMD", !105, !106}
!108 = !{!"ModuleMD", !107}
!IGCMetadata = !{!108}

; CHECK:         ret i32 %smpNormalized

; CHECK-NOT:     call i32 @__builtin_IB_is_normalized_coords(i32 %sampler)
