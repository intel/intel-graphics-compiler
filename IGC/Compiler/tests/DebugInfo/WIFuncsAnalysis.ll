;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-wi-func-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; WIFuncsAnalysis
; ------------------------------------------------
; This test checks that WIFuncsAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_wi(__global uint* dst)
; {
;   int lid  = get_local_id(0);
;   int gid  = get_group_id(0);
;   int gsize  = get_global_size(0);
;   int lsize = get_local_size(0);
;   int goff  = get_global_offset(0);
;   int dim  = get_work_dim();
;   int numgroups  = get_num_groups(0);
;   int elsize  = get_enqueued_local_size(0);
;   dst[0] = lid + gid + gsize + lsize + goff + dim + numgroups + elsize;
; }
;
; ------------------------------------------------
;
; This is an analysis pass so this test only checks
; that debug metadata is not changed

; Check IR:
;
; CHECK: define spir_kernel void @test_wi
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: addrspace{{.*}}, metadata [[DST_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %lid, metadata [[LID_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[LID_LOC:![0-9]*]]
; CHECK: [[LID_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LIDC_LOC:![0-9]*]]
; CHECK: store i32 [[LID_V]], i32* %lid, align 4, !dbg [[LID_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %gid, metadata [[GID_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[GID_LOC:![0-9]*]]
; CHECK: [[GID_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[GIDC_LOC:![0-9]*]]
; CHECK: store i32 [[GID_V]], i32* %gid, align 4, !dbg [[GID_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %gsize, metadata [[GSIZE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[GSIZE_LOC:![0-9]*]]
; CHECK: [[GSIZE_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[GSIZEC_LOC:![0-9]*]]
; CHECK: store i32 [[GSIZE_V]], i32* %gsize, align 4, !dbg [[GSIZE_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %lsize, metadata [[LSIZE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[LSIZE_LOC:![0-9]*]]
; CHECK: [[LSIZE_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LSIZEC_LOC:![0-9]*]]
; CHECK: store i32 [[LSIZE_V]], i32* %lsize, align 4, !dbg [[LSIZE_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %goff, metadata [[GOFF_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[GOFF_LOC:![0-9]*]]
; CHECK: [[GOFF_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[GOFFC_LOC:![0-9]*]]
; CHECK: store i32 [[GOFF_V]], i32* %goff, align 4, !dbg [[GOFF_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %dim, metadata [[DIM_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DIM_LOC:![0-9]*]]
; CHECK: [[DIM_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[DIMC_LOC:![0-9]*]]
; CHECK: store i32 [[DIM_V]], i32* %dim, align 4, !dbg [[DIM_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %numgroups, metadata [[NUMGROUPS_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[NUMGROUPS_LOC:![0-9]*]]
; CHECK: [[NUMGROUPS_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[NUMGROUPSC_LOC:![0-9]*]]
; CHECK: store i32 [[NUMGROUPS_V]], i32* %numgroups, align 4, !dbg [[NUMGROUPS_LOC]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %elsize, metadata [[ELSIZE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[ELSIZE_LOC:![0-9]*]]
; CHECK: [[ELSIZE_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[ELSIZEC_LOC:![0-9]*]]
; CHECK: store i32 [[ELSIZE_V]], i32* %elsize, align 4, !dbg [[ELSIZE_LOC]]

define spir_kernel void @test_wi(i32 addrspace(1)* %dst) #0 !dbg !9 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %lid = alloca i32, align 4
  %gid = alloca i32, align 4
  %gsize = alloca i32, align 4
  %lsize = alloca i32, align 4
  %goff = alloca i32, align 4
  %dim = alloca i32, align 4
  %numgroups = alloca i32, align 4
  %elsize = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !18, metadata !DIExpression()), !dbg !19
  call void @llvm.dbg.declare(metadata i32* %lid, metadata !20, metadata !DIExpression()), !dbg !22
  %call.i = call spir_func i32 @__builtin_IB_get_local_id_x(), !dbg !23
  store i32 %call.i, i32* %lid, align 4, !dbg !22
  call void @llvm.dbg.declare(metadata i32* %gid, metadata !24, metadata !DIExpression()), !dbg !25
  %call.i2 = call spir_func i32 @__builtin_IB_get_group_id(i32 0), !dbg !26
  store i32 %call.i2, i32* %gid, align 4, !dbg !25
  call void @llvm.dbg.declare(metadata i32* %gsize, metadata !27, metadata !DIExpression()), !dbg !28
  %call.i12 = call spir_func i32 @__builtin_IB_get_global_size(i32 0), !dbg !29
  store i32 %call.i12, i32* %gsize, align 4, !dbg !28
  call void @llvm.dbg.declare(metadata i32* %lsize, metadata !30, metadata !DIExpression()), !dbg !31
  %call.i22 = call spir_func i32 @__builtin_IB_get_local_size(i32 0), !dbg !32
  store i32 %call.i22, i32* %lsize, align 4, !dbg !31
  call void @llvm.dbg.declare(metadata i32* %goff, metadata !33, metadata !DIExpression()), !dbg !34
  %call.i32 = call spir_func i32 @__builtin_IB_get_global_offset(i32 0), !dbg !35
  store i32 %call.i32, i32* %goff, align 4, !dbg !34
  call void @llvm.dbg.declare(metadata i32* %dim, metadata !36, metadata !DIExpression()), !dbg !37
  %call.i42 = call spir_func i32 @__builtin_IB_get_work_dim(), !dbg !38
  store i32 %call.i42, i32* %dim, align 4, !dbg !37
  call void @llvm.dbg.declare(metadata i32* %numgroups, metadata !39, metadata !DIExpression()), !dbg !40
  %call.i43 = call spir_func i32 @__builtin_IB_get_num_groups(i32 0), !dbg !41
  store i32 %call.i43, i32* %numgroups, align 4, !dbg !40
  call void @llvm.dbg.declare(metadata i32* %elsize, metadata !42, metadata !DIExpression()), !dbg !43
  %call.i53 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0), !dbg !44
  store i32 %call.i53, i32* %elsize, align 4, !dbg !43
  %0 = load i32, i32* %lid, align 4, !dbg !45
  %1 = load i32, i32* %gid, align 4, !dbg !46
  %add = add nsw i32 %0, %1, !dbg !47
  %2 = load i32, i32* %gsize, align 4, !dbg !48
  %add14 = add nsw i32 %add, %2, !dbg !49
  %3 = load i32, i32* %lsize, align 4, !dbg !50
  %add15 = add nsw i32 %add14, %3, !dbg !51
  %4 = load i32, i32* %goff, align 4, !dbg !52
  %add16 = add nsw i32 %add15, %4, !dbg !53
  %5 = load i32, i32* %dim, align 4, !dbg !54
  %add17 = add nsw i32 %add16, %5, !dbg !55
  %6 = load i32, i32* %numgroups, align 4, !dbg !56
  %add18 = add nsw i32 %add17, %6, !dbg !57
  %7 = load i32, i32* %elsize, align 4, !dbg !58
  %add19 = add nsw i32 %add18, %7, !dbg !59
  %8 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !60
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %8, i64 0, !dbg !60
  store i32 %add19, i32 addrspace(1)* %arrayidx, align 4, !dbg !61
  ret void, !dbg !62
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "WIFuncsAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_wi", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 38, scope: [[SCOPE]])
;
; CHECK-DAG: [[LID_MD]] = !DILocalVariable(name: "lid", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LID_LOC]] = !DILocation(line: 3, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[LIDC_LOC]] = !DILocation(line: 3, column: 14, scope: [[SCOPE]])
;
; CHECK-DAG: [[GID_MD]] = !DILocalVariable(name: "gid", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[GID_LOC]] = !DILocation(line: 4, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[GIDC_LOC]] = !DILocation(line: 4, column: 14, scope: [[SCOPE]])
;
; CHECK-DAG: [[GSIZE_MD]] = !DILocalVariable(name: "gsize", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[GSIZE_LOC]] = !DILocation(line: 5, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[GSIZEC_LOC]] = !DILocation(line: 5, column: 16, scope: [[SCOPE]])
;
; CHECK-DAG: [[LSIZE_MD]] = !DILocalVariable(name: "lsize", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[LSIZE_LOC]] = !DILocation(line: 6, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[LSIZEC_LOC]] = !DILocation(line: 6, column: 15, scope: [[SCOPE]])
;
; CHECK-DAG: [[GOFF_MD]] = !DILocalVariable(name: "goff", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[GOFF_LOC]] = !DILocation(line: 7, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[GOFFC_LOC]] = !DILocation(line: 7, column: 15, scope: [[SCOPE]])
;
; CHECK-DAG: [[DIM_MD]] = !DILocalVariable(name: "dim", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[DIM_LOC]] = !DILocation(line: 8, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[DIMC_LOC]] = !DILocation(line: 8, column: 14, scope: [[SCOPE]])
;
; CHECK-DAG: [[NUMGROUPS_MD]] = !DILocalVariable(name: "numgroups", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[NUMGROUPS_LOC]] = !DILocation(line: 9, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[NUMGROUPSC_LOC]] = !DILocation(line: 9, column: 20, scope: [[SCOPE]])
;
; CHECK-DAG: [[ELSIZE_MD]] = !DILocalVariable(name: "elsize", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[ELSIZE_LOC]] = !DILocation(line: 10, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[ELSIZEC_LOC]] = !DILocation(line: 10, column: 17, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_num_groups(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_group_id(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_y() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_z() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_work_dim() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_enqueued_local_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_offset(i32) local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (i32 addrspace(1)*)* @test_wi, !7}
!7 = !{!8}
!8 = !{!"function_type", i32 0}
!9 = distinct !DISubprogram(name: "test_wi", scope: null, file: !10, line: 1, type: !11, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!10 = !DIFile(filename: "WIFuncsAnalysis.ll", directory: "/")
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !14}
!13 = !DIBasicType(name: "int", size: 4)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !16, baseType: !17)
!16 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!17 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "dst", arg: 1, scope: !9, file: !10, line: 1, type: !14)
!19 = !DILocation(line: 1, column: 38, scope: !9)
!20 = !DILocalVariable(name: "lid", scope: !9, file: !10, line: 3, type: !21)
!21 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!22 = !DILocation(line: 3, column: 7, scope: !9)
!23 = !DILocation(line: 3, column: 14, scope: !9)
!24 = !DILocalVariable(name: "gid", scope: !9, file: !10, line: 4, type: !21)
!25 = !DILocation(line: 4, column: 7, scope: !9)
!26 = !DILocation(line: 4, column: 14, scope: !9)
!27 = !DILocalVariable(name: "gsize", scope: !9, file: !10, line: 5, type: !21)
!28 = !DILocation(line: 5, column: 7, scope: !9)
!29 = !DILocation(line: 5, column: 16, scope: !9)
!30 = !DILocalVariable(name: "lsize", scope: !9, file: !10, line: 6, type: !21)
!31 = !DILocation(line: 6, column: 7, scope: !9)
!32 = !DILocation(line: 6, column: 15, scope: !9)
!33 = !DILocalVariable(name: "goff", scope: !9, file: !10, line: 7, type: !21)
!34 = !DILocation(line: 7, column: 7, scope: !9)
!35 = !DILocation(line: 7, column: 15, scope: !9)
!36 = !DILocalVariable(name: "dim", scope: !9, file: !10, line: 8, type: !21)
!37 = !DILocation(line: 8, column: 7, scope: !9)
!38 = !DILocation(line: 8, column: 14, scope: !9)
!39 = !DILocalVariable(name: "numgroups", scope: !9, file: !10, line: 9, type: !21)
!40 = !DILocation(line: 9, column: 7, scope: !9)
!41 = !DILocation(line: 9, column: 20, scope: !9)
!42 = !DILocalVariable(name: "elsize", scope: !9, file: !10, line: 10, type: !21)
!43 = !DILocation(line: 10, column: 7, scope: !9)
!44 = !DILocation(line: 10, column: 17, scope: !9)
!45 = !DILocation(line: 11, column: 12, scope: !9)
!46 = !DILocation(line: 11, column: 18, scope: !9)
!47 = !DILocation(line: 11, column: 16, scope: !9)
!48 = !DILocation(line: 11, column: 24, scope: !9)
!49 = !DILocation(line: 11, column: 22, scope: !9)
!50 = !DILocation(line: 11, column: 32, scope: !9)
!51 = !DILocation(line: 11, column: 30, scope: !9)
!52 = !DILocation(line: 11, column: 40, scope: !9)
!53 = !DILocation(line: 11, column: 38, scope: !9)
!54 = !DILocation(line: 11, column: 47, scope: !9)
!55 = !DILocation(line: 11, column: 45, scope: !9)
!56 = !DILocation(line: 11, column: 53, scope: !9)
!57 = !DILocation(line: 11, column: 51, scope: !9)
!58 = !DILocation(line: 11, column: 65, scope: !9)
!59 = !DILocation(line: 11, column: 63, scope: !9)
!60 = !DILocation(line: 11, column: 3, scope: !9)
!61 = !DILocation(line: 11, column: 10, scope: !9)
!62 = !DILocation(line: 12, column: 1, scope: !9)
