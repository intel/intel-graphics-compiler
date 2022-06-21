; RUN: igc_opt --igc-raytracing-shader-lowering -S < %s | FileCheck %s
; ------------------------------------------------
; RayTracingShaderLowering
; ------------------------------------------------
; This test checks that debug info is properly handled by RayTracingShaderLowering pass
;
; Checks:
; 1) Pass adds fences for calls
; it's good practice to copy debug info for them, but overall this is excessive check
; 2) Replaces AsyncStackPtr
; 3) Reduces redundant converts, check that no info is lost. <- This fails

; CHECK: @rtlow{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call {{.*}} !dbg [[TRACE_LOC:![0-9]*]]
; CHECK: call {{.*}} !dbg [[TRACE_LOC]]
; CHECK: call {{.*}} !dbg [[DISPATCH_LOC:![0-9]*]]
; CHECK: call {{.*}} !dbg [[DISPATCH_LOC]]
; CHECK: [[APTR_V:%[A-z0-9]*]] = {{.*}}i32 addrspace(1)*{{.*}} !dbg [[APTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 addrspace(1)* [[APTR_V]], metadata [[APTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[APTR_LOC]]
;
; redundant convert is removed here
; CHECK: @llvm.dbg.value(metadata i64 %{{.*}}, metadata [[IPTR64_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IPTR64_LOC:![0-9]*]]
;
; Note this can optimized too, if it will replace this check with commented
; CHECK: [[PTR32_V:%[A-z0-9]*]] = {{.*}}i32 addrspace(1)*{{.*}} !dbg [[PTR32_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 addrspace(1)* [[PTR32_V]], metadata [[PTR32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTR32_LOC]]
;
; check: @llvm.dbg.value(metadata i32 addrspace(1)* %{{.*}}, metadata [[PTR32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTR32_LOC:![0-9]*]]
;
; Fails here
; CHECK-DAG: @llvm.dbg.value(metadata float addrspace(1)* [[FPTR_V:%[A-z0-9]*]], metadata [[FPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FPTR_LOC:![0-9]*]]
; CHECK-DAG: [[FPTR_V]] = {{.*}}float addrspace(1)*{{.*}} !dbg [[FPTR_LOC]]
;
; CHECK: [[I32_V:%[A-z0-9]*]] = {{.*}} !dbg [[I32_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[I32_V]], metadata [[I32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[I32_LOC]]
;

define void @rtlow(i32 addrspace(1)* %src1, i64 %src2) !dbg !6 {
  call void @llvm.genx.GenISA.TraceRayAsync.p1i32(i32 addrspace(1)* %src1, i32 2), !dbg !17
  call void @llvm.genx.GenISA.BindlessThreadDispatch.p1i32(i32 addrspace(1)* %src1, i16 3, i64 %src2), !dbg !18
  %1 = call i32 addrspace(1)* @llvm.genx.GenISA.AsyncStackPtr.p1i32.i64(i64 %src2), !dbg !19
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %1, metadata !9, metadata !DIExpression()), !dbg !19
  %2 = ptrtoint i32 addrspace(1)* %1 to i64, !dbg !20
  call void @llvm.dbg.value(metadata i64 %2, metadata !11, metadata !DIExpression()), !dbg !20
  %3 = inttoptr i64 %2 to i32 addrspace(1)*, !dbg !21
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !21
  store i32 14, i32 addrspace(1)* %3, !dbg !22
  %4 = bitcast i32 addrspace(1)* %1 to float addrspace(1)*, !dbg !23
  call void @llvm.dbg.value(metadata float addrspace(1)* %4, metadata !13, metadata !DIExpression()), !dbg !23
  %5 = ptrtoint float addrspace(1)* %4 to i32, !dbg !24
  call void @llvm.dbg.value(metadata i32 %5, metadata !14, metadata !DIExpression()), !dbg !24
  %6 = bitcast i32 %5 to float, !dbg !25
  call void @llvm.dbg.value(metadata float %6, metadata !16, metadata !DIExpression()), !dbg !25
  store float %6, float addrspace(1)* %4, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "RayTracingShaderLowering.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "rtlow", linkageName: "rtlow", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[TRACE_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DISPATCH_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[APTR_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[APTR_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IPTR64_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[IPTR64_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTR32_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[PTR32_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FPTR_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[FPTR_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[I32_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[I32_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])



declare i32 addrspace(1)* @llvm.genx.GenISA.AsyncStackPtr.p1i32.i64(i64)

declare void @llvm.genx.GenISA.TraceRayAsync.p1i32(i32 addrspace(1)*, i32)

declare void @llvm.genx.GenISA.BindlessThreadDispatch.p1i32(i32 addrspace(1)*, i16, i64)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "RayTracingShaderLowering.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "rtlow", linkageName: "rtlow", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 3, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 4, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 7, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 8, type: !15)
!15 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 9, type: !15)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
