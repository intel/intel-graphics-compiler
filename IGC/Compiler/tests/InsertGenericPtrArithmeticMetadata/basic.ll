;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -insert-generic-ptr-arithmetic-metadata -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; InsertGenericPtrArithmeticMetadata
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(i1 %src1, i32* %src2) {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = addrspacecast i32* [[SRC2:%.*]] to i32 addrspace(4)*,{{.*}} !generic.arith [[GEN_MD:![0-9]*]]
; CHECK:    br i1 [[SRC1:%[A-z0-9]*]], label [[BB1:%[A-z0-9]*]], label [[BB2:%[A-z0-9]*]]
; CHECK:  bb1:
; CHECK:    [[TMP1:%.*]] = getelementptr i32, i32 addrspace(4)* [[TMP0]], i32 4
; CHECK:    br label [[END:%[A-z0-9]*]]
; CHECK:  bb2:
; CHECK:    [[TMP2:%.*]] = getelementptr i32, i32 addrspace(4)* [[TMP0]], i32 2
; CHECK:    br label [[END]]
; CHECK:  end:
; CHECK:    [[TMP3:%.*]] = phi i32 addrspace(4)* [ [[TMP1]], [[BB1]] ], [ [[TMP2]], [[BB2]] ]
; CHECK:    [[TMP4:%.*]] = ptrtoint i32 addrspace(4)* [[TMP3]] to i32
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP4]], 14
; CHECK:    store i32 [[TMP5]], i32* [[SRC2]]
; CHECK:    ret void
;
entry:
  %0 = addrspacecast i32* %src2 to i32 addrspace(4)*
  br i1 %src1, label %bb1, label %bb2

bb1:                                              ; preds = %entry
  %1 = getelementptr i32, i32 addrspace(4)* %0, i32 4
  br label %end

bb2:                                              ; preds = %entry
  %2 = getelementptr i32, i32 addrspace(4)* %0, i32 2
  br label %end

end:                                              ; preds = %bb2, %bb1
  %3 = phi i32 addrspace(4)* [ %1, %bb1 ], [ %2, %bb2 ]
  %4 = ptrtoint i32 addrspace(4)* %3 to i32
  %5 = add i32 %4, 14
  store i32 %5, i32* %src2
  ret void
}

; CHECK-DAG: [[GEN_MD]] = !{!"generic.arith"}
