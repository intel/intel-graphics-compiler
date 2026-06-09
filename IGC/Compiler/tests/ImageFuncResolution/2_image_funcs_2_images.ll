;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque

declare i32 @__builtin_IB_get_image_height(i32 %img)
declare i32 @__builtin_IB_get_image_width(i32 %img)

define i32 @foo(i32 %img1, i32 %img2, i32 %imageHeigt, i32 %imageWidth) nounwind {
  %id1 = call i32 @__builtin_IB_get_image_width(i32 %img1)
  %id2 = call i32 @__builtin_IB_get_image_height(i32 %img2)
  %res = add i32 %id1, %id2
  ret i32 %res
}

!igc.input.ir = !{!100}
!100 = !{!"ocl", i32 1, i32 2}

!igc.functions = !{!0}
!0 = !{i32 (i32, i32, i32, i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!101 = !{!"argId", i32 21}
!102 = !{!"explicitArgNum", i32 1}
!103 = !{!"implicitArgInfoListVec[0]", !101, !102}
!104 = !{!"argId", i32 22}
!105 = !{!"explicitArgNum", i32 0}
!106 = !{!"implicitArgInfoListVec[1]", !104, !105}
!107 = !{!"implicitArgInfoList", !103, !106}
!108 = !{!"FuncMDMap[0]", i32 (i32, i32, i32, i32)* @foo}
!109 = !{!"FuncMDValue[0]", !107}
!110 = !{!"FuncMD", !108, !109}
!111 = !{!"ModuleMD", !110}
!IGCMetadata = !{!111}

; CHECK:         %res = add i32 %imageWidth, %imageHeigt

; CHECK-NOT:     call i32 @__builtin_IB_get_image_width(i32 %img1)
; CHECK-NOT:     call i32 @__builtin_IB_get_image_height(i32 %img2)

