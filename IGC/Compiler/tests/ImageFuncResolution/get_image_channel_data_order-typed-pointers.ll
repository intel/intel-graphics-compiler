;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque

declare i32 @__builtin_IB_get_image_channel_order(i32 %img)

define i32 @foo(i32 %img, i32 %imageOrder) nounwind {
  %id = call i32 @__builtin_IB_get_image_channel_order(i32 %img)
  ret i32 %id
}

!igc.input.ir = !{!100}
!100 = !{!"ocl", i32 1, i32 2}

!igc.functions = !{!0}
!0 = !{i32 (i32, i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!101 = !{!"argId", i32 26}
!102 = !{!"explicitArgNum", i32 0}
!103 = !{!"implicitArgInfoListVec[0]", !101, !102}
!104 = !{!"implicitArgInfoList", !103}
!105 = !{!"FuncMDMap[0]", i32 (i32, i32)* @foo}
!106 = !{!"FuncMDValue[0]", !104}
!107 = !{!"FuncMD", !105, !106}
!108 = !{!"ModuleMD", !107}
!IGCMetadata = !{!108}

; CHECK:         ret i32 %imageOrder

; CHECK-NOT:     call i32 @__builtin_IB_get_image_channel_order(i32 %img)

