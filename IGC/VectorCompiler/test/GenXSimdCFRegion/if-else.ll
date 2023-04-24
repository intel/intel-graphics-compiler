;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -simdcf-region -enable-simdcf-transform -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion generate
; correct if-then-else simd llvm-ir (based on ispc test)

; CHECK: foo
define void @foo(i32 %In, i32 %Out) #0 {
if.entry:
  %x = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %y = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %mask = icmp slt <16 x i32> %x, zeroinitializer
  %not.mask = xor <16 x i1> %mask, <i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1>
  %any = call i1 @llvm.genx.any.v16i1(<16 x i1> %mask)
  br i1 %any, label %if.then, label %if.else

; CHECK: [[IF_AFTER_ENTRY:[A-z0-9.]*]]:
; CHECK: [[GOTO:%[A-z0-9.]*]] = call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto
; CHECK: {{.*}} = extractvalue { <32 x i1>, <16 x i1>, i1 } [[GOTO]]

; CHECK: [[IF_THEN:[A-z0-9.]*]]:      ; preds = %[[IF_AFTER_ENTRY]]
if.then:                                          ; preds = %entry
  %mul = mul <16 x i32> %x, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %mul2 = mul <16 x i32> %y, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %add = add <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, %mul
  %add.masked = select <16 x i1> %mask, <16 x i32> %add, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %add2.masked = select <16 x i1> %mask, <16 x i32> %mul2, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %any.not = call i1 @llvm.genx.any.v16i1(<16 x i1> %not.mask)
  br i1 %any.not, label %if.else, label %if.end

; CHECK: [[AFTERTHEN:[A-z0-9.]*]]: ; preds = %[[IF_THEN]], %[[IF_THEN]], %[[IF_AFTER_ENTRY]]
; CHECK-DAG: [[AFTERTHEN_PHI_X:%[A-z0-9.]*]] = phi <16 x i32> [ %add.masked{{.*}}, %[[IF_THEN]] ], [ %add.masked{{.*}}, %[[IF_THEN]] ]
; CHECK-DAG: [[AFTERTHEN_PHI_Y:%[A-z0-9.]*]] = phi <16 x i32> [ %add2.masked{{.*}}, %[[IF_THEN]] ], [ %add2.masked{{.*}}, %[[IF_THEN]] ]
; CHECK-SAME: <i32 1, i32 1, i32 1, i32 1,
; CHECK: [[JOIN_CALL:%[A-z0-9.]*]] = call { <32 x i1>, i1 } @llvm.genx.simdcf.join
; CHECK: %{{.*}} = extractvalue { <32 x i1>, i1 } [[JOIN_CALL]]

; CHECK: [[IF_ELSE:[A-z0-9.]*]]:      ; preds = %[[AFTERTHEN]]
if.else:
  %div = mul <16 x i32> %x, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %div2 = mul <16 x i32> %y, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %add.1 = add <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, %div
  %add.1.masked = select <16 x i1> %not.mask, <16 x i32> %add.1, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %add2.1.masked = select <16 x i1> %not.mask, <16 x i32> %div2, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  br label %if.end

; CHECK: if.end: ; preds = %[[AFTERTHEN]], %[[IF_ELSE]]
; C HECK-DAG: %{{[A-z0-9.]*}} = phi <16 x i32> [ [[AFTERTHEN_PHI_X]], {{.*}}afterthen ], [ %{{[A-z0-9.]*}}, %[[IF_ELSE]] ]
; C HECK-DAG: %{{[A-z0-9.]*}} = phi <16 x i32> [ [[AFTERTHEN_PHI_Y]], {{.*}}afterthen ], [ %{{[A-z0-9.]*}}, %[[IF_ELSE]] ]
if.end:                                           ; preds = %if.then, %entry
  %a = phi <16 x i32> [ %add.masked, %if.then ], [ %add.1.masked, %if.else ]
  %b = phi <16 x i32> [ %add2.masked, %if.then ], [ %add2.1.masked, %if.else ]
  tail call void @llvm.genx.oword.st.v16i32(i32 2, i32 0, <16 x i32> %a)
  tail call void @llvm.genx.oword.st.v16i32(i32 2, i32 0, <16 x i32> %b)
  ret void
}

declare !genx_intrinsic_id !6 i1 @llvm.genx.any.v16i1(<16 x i1>) #3
declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>) #1

attributes #0 = { noinline nounwind optnone "CMGenxMain" "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="256" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "oclrt"="1" "stack-protector-buffer-size"="8" "target-cpu"="TGLLP" "target-features"="+ocl_runtime" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!genx.kernels = !{!0}
!llvm.module.flags = !{!4}
!llvm.ident = !{!5}

!0 = !{void (i32, i32)* @foo, !"foo", !1, i32 0, i32 0, !2, !3, i32 0, i32 0}
!1 = !{i32 2, i32 2}
!2 = !{i32 0, i32 0}
!3 = !{!"", !""}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{!"clang version 8.0.1 \0A(VC-Intrinsics sources: 1e2562d87528e1870ea2ad7197e4cc0346192e22)"}
!6 = !{i32 7022}
!7 = !{i32 6977}
!8 = !{i32 7195}
!9 = !{i32 6975}
!10 = !{i32 6979}
