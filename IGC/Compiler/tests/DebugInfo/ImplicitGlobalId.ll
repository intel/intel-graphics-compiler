; RUN: igc_opt -igc-add-implicit-gid -S < %s | FileCheck %s
; ------------------------------------------------
; ImplicitGlobalId
; ------------------------------------------------
; This test checks that debug info is properly handled by ImplicitGlobalId pass.
; ImplicitGlobalId:
; Creates implicit global and local id variables and debug info for them for each
; function in module
; FIXME: Check lineinfo for foo function
; ModuleID = 'ImplicitGlobalId.ll'
source_filename = "ImplicitGlobalId.ll"

define spir_kernel void @test_gid(i32 %src1) !dbg !6 {
entry:
  ; Testcase 1:
  ; Checks that global id values are added for @test_gid scope
  ; CHECK: @test_gid
  ; CHECK: %globalId = call <3 x i64> @{{.*Global.*}}
  ; CHECK: [[OCL_GID0:%__ocl_dbg_gid0]] = extractelement {{.*}}
  ; CHECK: [[OCL_GID1:%__ocl_dbg_gid1]] = extractelement {{.*}}
  ; CHECK: [[OCL_GID2:%__ocl_dbg_gid2]] = extractelement {{.*}}
  ; CHECK: [[DBG_VALUE:call void @llvm.dbg.value\(metadata]] i64 [[OCL_GID0]], metadata [[OCL_GID0_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC:![0-9]*]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_GID1]], metadata [[OCL_GID1_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_GID2]], metadata [[OCL_GID2_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ;
  ; Checks that local id values are added for @test_gid scope
  ; CHECK: %globalId1 = call <3 x i64> @{{.*Local.*}}
  ; CHECK: [[OCL_LID0:%__ocl_dbg_lid0]] = extractelement {{.*}}
  ; CHECK: [[OCL_LID1:%__ocl_dbg_lid1]] = extractelement {{.*}}
  ; CHECK: [[OCL_LID2:%__ocl_dbg_lid2]] = extractelement {{.*}}
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_LID0]], metadata [[OCL_LID0_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_LID1]], metadata [[OCL_LID1_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_LID2]], metadata [[OCL_LID2_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ;
  ; Checks that workgroup id values are added for @test_gid scope
  ; CHECK: %globalId2 = call <3 x i64> @{{.*Workgroup.*}}
  ; CHECK: [[OCL_WID0:%__ocl_dbg_grid0]] = extractelement {{.*}}
  ; CHECK: [[OCL_WID1:%__ocl_dbg_grid1]] = extractelement {{.*}}
  ; CHECK: [[OCL_WID2:%__ocl_dbg_grid2]] = extractelement {{.*}}
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_WID0]], metadata [[OCL_WID0_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_WID1]], metadata [[OCL_WID1_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ; CHECK: [[DBG_VALUE]] i64 [[OCL_WID2]], metadata [[OCL_WID2_MD:![0-9]*]], {{.*}}, !dbg [[GID_LOC]]
  ;
  ; Checks that debug info of function is not modified
  ; CHECK: [[FOO_V:%[0-9]*]] = call float @foo(i32 %src1), !dbg [[FOO_CALL_LOC:![0-9]*]]
  ; CHECK: [[DBG_VALUE]] float [[FOO_V]], metadata [[FOO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FOO_CALL_LOC]]
  ;
  ; Checks that next bb is not updated
  ; CHECK-NOT: call
   %0 = call float @foo(i32 %src1), !dbg !11
  call void @llvm.dbg.value(metadata float %0, metadata !9, metadata !DIExpression()), !dbg !11
  br label %exit, !dbg !12

exit:                                             ; preds = %entry
  ret void, !dbg !13
}

define float @foo(i32 %src1) !dbg !14 {
  ; Testcase 2:
  ; Disabled this test case until proper ENV setting is not supported by igc_opt
  ; Checks that global id values are added for @foo scope
  ; CHECK: @foo
  ; cHECK: %globalId = call <3 x i64> @{{.*Global.*}}
  ; cHECK-NEXT: [[FOO_GID0:%__ocl_dbg_gid0]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_GID1:%__ocl_dbg_gid1]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_GID2:%__ocl_dbg_gid2]] = extractelement {{.*}}
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_GID0]], metadata [[FOO_GID0_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC:![0-9]*]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_GID1]], metadata [[FOO_GID1_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_GID2]], metadata [[FOO_GID2_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ;
  ; Checks that local id values are added for @test_gid scope
  ; cHECK-NEXT: %globalId1 = call <3 x i64> @{{.*Local.*}}
  ; cHECK-NEXT: [[FOO_LID0:%__ocl_dbg_lid0]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_LID1:%__ocl_dbg_lid1]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_LID2:%__ocl_dbg_lid2]] = extractelement {{.*}}
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_LID0]], metadata [[FOO_LID0_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_LID1]], metadata [[FOO_LID1_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_LID2]], metadata [[FOO_LID2_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ;
  ; Checks that workgroup id values are added for @test_gid scope
  ; cHECK-NEXT: %globalId2 = call <3 x i64> @{{.*Workgroup.*}}
  ; cHECK-NEXT: [[FOO_WID0:%__ocl_dbg_grid0]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_WID1:%__ocl_dbg_grid1]] = extractelement {{.*}}
  ; cHECK-NEXT: [[FOO_WID2:%__ocl_dbg_grid2]] = extractelement {{.*}}
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_WID0]], metadata [[FOO_WID0_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_WID1]], metadata [[FOO_WID1_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ; cHECK-NEXT: [[DBG_VALUE]] i64 [[FOO_WID2]], metadata [[FOO_WID2_MD:![0-9]*]], {{.*}}, !dbg [[FOO_LOC]]
  ;
  ; Checks that debug info of function is not modified
  ; CHECK: [[CAST_V:%[0-9]*]] = bitcast {{.*}} !dbg [[CAST_LOC:![0-9]*]]
  ; CHECK: [[DBG_VALUE]] float [[CAST_V]], metadata [[CAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CAST_LOC]]
  %1 = bitcast i32 %src1 to float, !dbg !17
  call void @llvm.dbg.value(metadata float %1, metadata !16, metadata !DIExpression()), !dbg !17
  ret float %1, !dbg !18
}

; Testcase 1 MD:
; CHECK-DAG: [[GID_SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_gid"
; CHECK-DAG: [[GID_LOC]] = !DILocation(line: 1, scope: [[GID_SCOPE]])
; CHECK-DAG: [[LONG_LONG_T:![0-9]*]] = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
; CHECK-DAG: [[OCL_GID0_MD]] = !DILocalVariable(name: "__ocl_dbg_gid0", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_GID1_MD]] = !DILocalVariable(name: "__ocl_dbg_gid1", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_GID2_MD]] = !DILocalVariable(name: "__ocl_dbg_gid2", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_LID0_MD]] = !DILocalVariable(name: "__ocl_dbg_lid0", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_LID1_MD]] = !DILocalVariable(name: "__ocl_dbg_lid1", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_LID2_MD]] = !DILocalVariable(name: "__ocl_dbg_lid2", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_WID0_MD]] = !DILocalVariable(name: "__ocl_dbg_grid0", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_WID1_MD]] = !DILocalVariable(name: "__ocl_dbg_grid1", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[OCL_WID2_MD]] = !DILocalVariable(name: "__ocl_dbg_grid2", scope: [[GID_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[FOO_CALL_LOC]] = !DILocation(line: 1, column: 1, scope: [[GID_SCOPE]])
; CHECK-DAG: [[FOO_MD]] = !DILocalVariable(name: "1", scope: [[GID_SCOPE]]

; Testcase 2 MD:
; CHECK-DAG: [[FOO_SCOPE:![0-9]*]] = distinct !DISubprogram(name: "foo"
; cHECK-DAG: [[FOO_LOC]] = !DILocation(line: 4, scope: [[FOO_SCOPE]])
; cHECK-DAG: [[FOO_GID0_MD]] = !DILocalVariable(name: "__ocl_dbg_gid0", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_GID1_MD]] = !DILocalVariable(name: "__ocl_dbg_gid1", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_GID2_MD]] = !DILocalVariable(name: "__ocl_dbg_gid2", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_LID0_MD]] = !DILocalVariable(name: "__ocl_dbg_lid0", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_LID1_MD]] = !DILocalVariable(name: "__ocl_dbg_lid1", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_LID2_MD]] = !DILocalVariable(name: "__ocl_dbg_lid2", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_WID0_MD]] = !DILocalVariable(name: "__ocl_dbg_grid0", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_WID1_MD]] = !DILocalVariable(name: "__ocl_dbg_grid1", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; cHECK-DAG: [[FOO_WID2_MD]] = !DILocalVariable(name: "__ocl_dbg_grid2", scope: [[FOO_SCOPE]], line: 1, type: [[LONG_LONG_T]])
; CHECK-DAG: [[CAST_LOC]] = !DILocation(line: 4, column: 1, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[CAST_MD]] = !DILocalVariable(name: "2", scope: [[FOO_SCOPE]]

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "2.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_gid", linkageName: "test_gid", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 4, type: !7, scopeLine: 4, unit: !0, retainedNodes: !15)
!15 = !{!16}
!16 = !DILocalVariable(name: "2", scope: !14, file: !1, line: 4, type: !10)
!17 = !DILocation(line: 4, column: 1, scope: !14)
!18 = !DILocation(line: 5, column: 1, scope: !14)
