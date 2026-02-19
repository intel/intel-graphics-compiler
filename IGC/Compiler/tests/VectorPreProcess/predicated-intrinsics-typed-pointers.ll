;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-vectorpreprocess -S < %s 2>&1 | FileCheck %s

; ------------------------------------------------
; VectorPreProcess : predicated load/store intrinsics
; ------------------------------------------------

; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(<13 x i32> addrspace(1)* %src, <13 x i32> addrspace(1)* %dst, <13 x i32> %mrg, i48 addrspace(1)* %src1, i48 %mrg1, i48 addrspace(1)* %dst1) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC:%.*]] to i32 addrspace(1)*
; CHECK:    [[TMP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = bitcast i32 addrspace(1)* [[TMP2]] to <8 x i32> addrspace(1)*
; CHECK:    [[TMP4:%.*]] = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP3]], i64 4, i1 true, <8 x i32> zeroinitializer)
; CHECK:    [[TMP5:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP6:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP5]], i32 8
; CHECK:    [[TMP7:%.*]] = bitcast i32 addrspace(1)* [[TMP6]] to <4 x i32> addrspace(1)*
; CHECK:    [[TMP8:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP7]], i64 4, i1 true, <4 x i32> zeroinitializer)
; CHECK:    [[TMP9:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP10:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP9]], i32 12
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[TMP10]], i64 4, i1 true, i32 0)
; CHECK:    [[SPLIT:%.*]] = extractelement <8 x i32> [[TMP4]], i32 0
; CHECK:    [[SPLIT1:%.*]] = extractelement <8 x i32> [[TMP4]], i32 1
; CHECK:    [[SPLIT2:%.*]] = extractelement <8 x i32> [[TMP4]], i32 2
; CHECK:    [[SPLIT3:%.*]] = extractelement <8 x i32> [[TMP4]], i32 3
; CHECK:    [[SPLIT4:%.*]] = extractelement <8 x i32> [[TMP4]], i32 4
; CHECK:    [[SPLIT5:%.*]] = extractelement <8 x i32> [[TMP4]], i32 5
; CHECK:    [[SPLIT6:%.*]] = extractelement <8 x i32> [[TMP4]], i32 6
; CHECK:    [[SPLIT7:%.*]] = extractelement <8 x i32> [[TMP4]], i32 7
; CHECK:    [[SPLIT8:%.*]] = extractelement <4 x i32> [[TMP8]], i32 0
; CHECK:    [[SPLIT9:%.*]] = extractelement <4 x i32> [[TMP8]], i32 1
; CHECK:    [[SPLIT10:%.*]] = extractelement <4 x i32> [[TMP8]], i32 2
; CHECK:    [[SPLIT11:%.*]] = extractelement <4 x i32> [[TMP8]], i32 3
; CHECK:    [[TMP12:%.*]] = insertelement <8 x i32> undef, i32 [[SPLIT]], i32 0
; CHECK:    [[TMP13:%.*]] = insertelement <8 x i32> [[TMP12]], i32 [[SPLIT1]], i32 1
; CHECK:    [[TMP14:%.*]] = insertelement <8 x i32> [[TMP13]], i32 [[SPLIT2]], i32 2
; CHECK:    [[TMP15:%.*]] = insertelement <8 x i32> [[TMP14]], i32 [[SPLIT3]], i32 3
; CHECK:    [[TMP16:%.*]] = insertelement <8 x i32> [[TMP15]], i32 [[SPLIT4]], i32 4
; CHECK:    [[TMP17:%.*]] = insertelement <8 x i32> [[TMP16]], i32 [[SPLIT5]], i32 5
; CHECK:    [[TMP18:%.*]] = insertelement <8 x i32> [[TMP17]], i32 [[SPLIT6]], i32 6
; CHECK:    [[TMP19:%.*]] = insertelement <8 x i32> [[TMP18]], i32 [[SPLIT7]], i32 7
; CHECK:    [[TMP20:%.*]] = insertelement <4 x i32> undef, i32 [[SPLIT8]], i32 0
; CHECK:    [[TMP21:%.*]] = insertelement <4 x i32> [[TMP20]], i32 [[SPLIT9]], i32 1
; CHECK:    [[TMP22:%.*]] = insertelement <4 x i32> [[TMP21]], i32 [[SPLIT10]], i32 2
; CHECK:    [[TMP23:%.*]] = insertelement <4 x i32> [[TMP22]], i32 [[SPLIT11]], i32 3
; CHECK:    [[TMP24:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST:%.*]] to i32 addrspace(1)*
; CHECK:    [[TMP25:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP24]], i32 0
; CHECK:    [[TMP26:%.*]] = bitcast i32 addrspace(1)* [[TMP25]] to <8 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP26]], <8 x i32> [[TMP19]], i64 4, i1 true)
; CHECK:    [[TMP27:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP28:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP27]], i32 8
; CHECK:    [[TMP29:%.*]] = bitcast i32 addrspace(1)* [[TMP28]] to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP29]], <4 x i32> [[TMP23]], i64 4, i1 true)
; CHECK:    [[TMP30:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP31:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP30]], i32 12
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP31]], i32 [[TMP11]], i64 4, i1 true)
  %1 = call <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %src, i64 4, i1 true, <13 x i32> zeroinitializer)
  call void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %dst, <13 x i32> %1, i64 4, i1 true)

; CHECK:    [[TMP32:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP33:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP32]], i32 0
; CHECK:    [[TMP34:%.*]] = bitcast i32 addrspace(1)* [[TMP33]] to <8 x i32> addrspace(1)*
; CHECK:    [[TMP35:%.*]] = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP34]], i64 4, i1 true, <8 x i32> undef)
; CHECK:    [[TMP36:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP37:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP36]], i32 8
; CHECK:    [[TMP38:%.*]] = bitcast i32 addrspace(1)* [[TMP37]] to <4 x i32> addrspace(1)*
; CHECK:    [[TMP39:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP38]], i64 4, i1 true, <4 x i32> undef)
; CHECK:    [[TMP40:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP41:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP40]], i32 12
; CHECK:    [[TMP42:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[TMP41]], i64 4, i1 true, i32 undef)
; CHECK:    [[SPLIT12:%.*]] = extractelement <8 x i32> [[TMP35]], i32 0
; CHECK:    [[SPLIT13:%.*]] = extractelement <8 x i32> [[TMP35]], i32 1
; CHECK:    [[SPLIT14:%.*]] = extractelement <8 x i32> [[TMP35]], i32 2
; CHECK:    [[SPLIT15:%.*]] = extractelement <8 x i32> [[TMP35]], i32 3
; CHECK:    [[SPLIT16:%.*]] = extractelement <8 x i32> [[TMP35]], i32 4
; CHECK:    [[SPLIT17:%.*]] = extractelement <8 x i32> [[TMP35]], i32 5
; CHECK:    [[SPLIT18:%.*]] = extractelement <8 x i32> [[TMP35]], i32 6
; CHECK:    [[SPLIT19:%.*]] = extractelement <8 x i32> [[TMP35]], i32 7
; CHECK:    [[SPLIT20:%.*]] = extractelement <4 x i32> [[TMP39]], i32 0
; CHECK:    [[SPLIT21:%.*]] = extractelement <4 x i32> [[TMP39]], i32 1
; CHECK:    [[SPLIT22:%.*]] = extractelement <4 x i32> [[TMP39]], i32 2
; CHECK:    [[SPLIT23:%.*]] = extractelement <4 x i32> [[TMP39]], i32 3
; CHECK:    [[TMP43:%.*]] = insertelement <8 x i32> undef, i32 [[SPLIT12]], i32 0
; CHECK:    [[TMP44:%.*]] = insertelement <8 x i32> [[TMP43]], i32 [[SPLIT13]], i32 1
; CHECK:    [[TMP45:%.*]] = insertelement <8 x i32> [[TMP44]], i32 [[SPLIT14]], i32 2
; CHECK:    [[TMP46:%.*]] = insertelement <8 x i32> [[TMP45]], i32 [[SPLIT15]], i32 3
; CHECK:    [[TMP47:%.*]] = insertelement <8 x i32> [[TMP46]], i32 [[SPLIT16]], i32 4
; CHECK:    [[TMP48:%.*]] = insertelement <8 x i32> [[TMP47]], i32 [[SPLIT17]], i32 5
; CHECK:    [[TMP49:%.*]] = insertelement <8 x i32> [[TMP48]], i32 [[SPLIT18]], i32 6
; CHECK:    [[TMP50:%.*]] = insertelement <8 x i32> [[TMP49]], i32 [[SPLIT19]], i32 7
; CHECK:    [[TMP51:%.*]] = insertelement <4 x i32> undef, i32 [[SPLIT20]], i32 0
; CHECK:    [[TMP52:%.*]] = insertelement <4 x i32> [[TMP51]], i32 [[SPLIT21]], i32 1
; CHECK:    [[TMP53:%.*]] = insertelement <4 x i32> [[TMP52]], i32 [[SPLIT22]], i32 2
; CHECK:    [[TMP54:%.*]] = insertelement <4 x i32> [[TMP53]], i32 [[SPLIT23]], i32 3
; CHECK:    [[TMP55:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP56:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP55]], i32 0
; CHECK:    [[TMP57:%.*]] = bitcast i32 addrspace(1)* [[TMP56]] to <8 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP57]], <8 x i32> [[TMP50]], i64 4, i1 true)
; CHECK:    [[TMP58:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP59:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP58]], i32 8
; CHECK:    [[TMP60:%.*]] = bitcast i32 addrspace(1)* [[TMP59]] to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP60]], <4 x i32> [[TMP54]], i64 4, i1 true)
; CHECK:    [[TMP61:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP62:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP61]], i32 12
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP62]], i32 [[TMP42]], i64 4, i1 true)
  %2 = call <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %src, i64 4, i1 true, <13 x i32> undef)
  call void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %dst, <13 x i32> %2, i64 4, i1 true)

; CHECK:    [[TMP63:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP64:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP63]], i32 0
; CHECK:    [[TMP65:%.*]] = bitcast i32 addrspace(1)* [[TMP64]] to <8 x i32> addrspace(1)*
; CHECK:    [[TMP66:%.*]] = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP65]], i64 4, i1 true, <8 x i32> poison)
; CHECK:    [[TMP67:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP68:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP67]], i32 8
; CHECK:    [[TMP69:%.*]] = bitcast i32 addrspace(1)* [[TMP68]] to <4 x i32> addrspace(1)*
; CHECK:    [[TMP70:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP69]], i64 4, i1 true, <4 x i32> poison)
; CHECK:    [[TMP71:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP72:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP71]], i32 12
; CHECK:    [[TMP73:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[TMP72]], i64 4, i1 true, i32 poison)
; CHECK:    [[SPLIT24:%.*]] = extractelement <8 x i32> [[TMP66]], i32 0
; CHECK:    [[SPLIT25:%.*]] = extractelement <8 x i32> [[TMP66]], i32 1
; CHECK:    [[SPLIT26:%.*]] = extractelement <8 x i32> [[TMP66]], i32 2
; CHECK:    [[SPLIT27:%.*]] = extractelement <8 x i32> [[TMP66]], i32 3
; CHECK:    [[SPLIT28:%.*]] = extractelement <8 x i32> [[TMP66]], i32 4
; CHECK:    [[SPLIT29:%.*]] = extractelement <8 x i32> [[TMP66]], i32 5
; CHECK:    [[SPLIT30:%.*]] = extractelement <8 x i32> [[TMP66]], i32 6
; CHECK:    [[SPLIT31:%.*]] = extractelement <8 x i32> [[TMP66]], i32 7
; CHECK:    [[SPLIT32:%.*]] = extractelement <4 x i32> [[TMP70]], i32 0
; CHECK:    [[SPLIT33:%.*]] = extractelement <4 x i32> [[TMP70]], i32 1
; CHECK:    [[SPLIT34:%.*]] = extractelement <4 x i32> [[TMP70]], i32 2
; CHECK:    [[SPLIT35:%.*]] = extractelement <4 x i32> [[TMP70]], i32 3
; CHECK:    [[TMP74:%.*]] = insertelement <8 x i32> undef, i32 [[SPLIT24]], i32 0
; CHECK:    [[TMP75:%.*]] = insertelement <8 x i32> [[TMP74]], i32 [[SPLIT25]], i32 1
; CHECK:    [[TMP76:%.*]] = insertelement <8 x i32> [[TMP75]], i32 [[SPLIT26]], i32 2
; CHECK:    [[TMP77:%.*]] = insertelement <8 x i32> [[TMP76]], i32 [[SPLIT27]], i32 3
; CHECK:    [[TMP78:%.*]] = insertelement <8 x i32> [[TMP77]], i32 [[SPLIT28]], i32 4
; CHECK:    [[TMP79:%.*]] = insertelement <8 x i32> [[TMP78]], i32 [[SPLIT29]], i32 5
; CHECK:    [[TMP80:%.*]] = insertelement <8 x i32> [[TMP79]], i32 [[SPLIT30]], i32 6
; CHECK:    [[TMP81:%.*]] = insertelement <8 x i32> [[TMP80]], i32 [[SPLIT31]], i32 7
; CHECK:    [[TMP82:%.*]] = insertelement <4 x i32> undef, i32 [[SPLIT32]], i32 0
; CHECK:    [[TMP83:%.*]] = insertelement <4 x i32> [[TMP82]], i32 [[SPLIT33]], i32 1
; CHECK:    [[TMP84:%.*]] = insertelement <4 x i32> [[TMP83]], i32 [[SPLIT34]], i32 2
; CHECK:    [[TMP85:%.*]] = insertelement <4 x i32> [[TMP84]], i32 [[SPLIT35]], i32 3
; CHECK:    [[TMP86:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP87:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP86]], i32 0
; CHECK:    [[TMP88:%.*]] = bitcast i32 addrspace(1)* [[TMP87]] to <8 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP88]], <8 x i32> [[TMP81]], i64 4, i1 true)
; CHECK:    [[TMP89:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP90:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP89]], i32 8
; CHECK:    [[TMP91:%.*]] = bitcast i32 addrspace(1)* [[TMP90]] to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP91]], <4 x i32> [[TMP85]], i64 4, i1 true)
; CHECK:    [[TMP92:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP93:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP92]], i32 12
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP93]], i32 [[TMP73]], i64 4, i1 true)
  %3 = call <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %src, i64 4, i1 true, <13 x i32> poison)
  call void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %dst, <13 x i32> %3, i64 4, i1 true)

; CHECK:    [[TMP94:%.*]] = extractelement <13 x i32> [[MRG:%.*]], i32 0
; CHECK:    [[TMP95:%.*]] = insertelement <8 x i32> undef, i32 [[TMP94]], i32 0
; CHECK:    [[TMP96:%.*]] = extractelement <13 x i32> [[MRG]], i32 1
; CHECK:    [[TMP97:%.*]] = insertelement <8 x i32> [[TMP95]], i32 [[TMP96]], i32 1
; CHECK:    [[TMP98:%.*]] = extractelement <13 x i32> [[MRG]], i32 2
; CHECK:    [[TMP99:%.*]] = insertelement <8 x i32> [[TMP97]], i32 [[TMP98]], i32 2
; CHECK:    [[TMP100:%.*]] = extractelement <13 x i32> [[MRG]], i32 3
; CHECK:    [[TMP101:%.*]] = insertelement <8 x i32> [[TMP99]], i32 [[TMP100]], i32 3
; CHECK:    [[TMP102:%.*]] = extractelement <13 x i32> [[MRG]], i32 4
; CHECK:    [[TMP103:%.*]] = insertelement <8 x i32> [[TMP101]], i32 [[TMP102]], i32 4
; CHECK:    [[TMP104:%.*]] = extractelement <13 x i32> [[MRG]], i32 5
; CHECK:    [[TMP105:%.*]] = insertelement <8 x i32> [[TMP103]], i32 [[TMP104]], i32 5
; CHECK:    [[TMP106:%.*]] = extractelement <13 x i32> [[MRG]], i32 6
; CHECK:    [[TMP107:%.*]] = insertelement <8 x i32> [[TMP105]], i32 [[TMP106]], i32 6
; CHECK:    [[TMP108:%.*]] = extractelement <13 x i32> [[MRG]], i32 7
; CHECK:    [[TMP109:%.*]] = insertelement <8 x i32> [[TMP107]], i32 [[TMP108]], i32 7
; CHECK:    [[TMP110:%.*]] = extractelement <13 x i32> [[MRG]], i32 8
; CHECK:    [[TMP111:%.*]] = insertelement <4 x i32> undef, i32 [[TMP110]], i32 0
; CHECK:    [[TMP112:%.*]] = extractelement <13 x i32> [[MRG]], i32 9
; CHECK:    [[TMP113:%.*]] = insertelement <4 x i32> [[TMP111]], i32 [[TMP112]], i32 1
; CHECK:    [[TMP114:%.*]] = extractelement <13 x i32> [[MRG]], i32 10
; CHECK:    [[TMP115:%.*]] = insertelement <4 x i32> [[TMP113]], i32 [[TMP114]], i32 2
; CHECK:    [[TMP116:%.*]] = extractelement <13 x i32> [[MRG]], i32 11
; CHECK:    [[TMP117:%.*]] = insertelement <4 x i32> [[TMP115]], i32 [[TMP116]], i32 3
; CHECK:    [[TMP118:%.*]] = extractelement <13 x i32> [[MRG]], i32 12
; CHECK:    [[TMP119:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP120:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP119]], i32 0
; CHECK:    [[TMP121:%.*]] = bitcast i32 addrspace(1)* [[TMP120]] to <8 x i32> addrspace(1)*
; CHECK:    [[TMP122:%.*]] = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP121]], i64 4, i1 true, <8 x i32> [[TMP109]])
; CHECK:    [[TMP123:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP124:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP123]], i32 8
; CHECK:    [[TMP125:%.*]] = bitcast i32 addrspace(1)* [[TMP124]] to <4 x i32> addrspace(1)*
; CHECK:    [[TMP126:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP125]], i64 4, i1 true, <4 x i32> [[TMP117]])
; CHECK:    [[TMP127:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP128:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP127]], i32 12
; CHECK:    [[TMP129:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[TMP128]], i64 4, i1 true, i32 [[TMP118]])
; CHECK:    [[SPLIT36:%.*]] = extractelement <8 x i32> [[TMP122]], i32 0
; CHECK:    [[SPLIT37:%.*]] = extractelement <8 x i32> [[TMP122]], i32 1
; CHECK:    [[SPLIT38:%.*]] = extractelement <8 x i32> [[TMP122]], i32 2
; CHECK:    [[SPLIT39:%.*]] = extractelement <8 x i32> [[TMP122]], i32 3
; CHECK:    [[SPLIT40:%.*]] = extractelement <8 x i32> [[TMP122]], i32 4
; CHECK:    [[SPLIT41:%.*]] = extractelement <8 x i32> [[TMP122]], i32 5
; CHECK:    [[SPLIT42:%.*]] = extractelement <8 x i32> [[TMP122]], i32 6
; CHECK:    [[SPLIT43:%.*]] = extractelement <8 x i32> [[TMP122]], i32 7
; CHECK:    [[SPLIT44:%.*]] = extractelement <4 x i32> [[TMP126]], i32 0
; CHECK:    [[SPLIT45:%.*]] = extractelement <4 x i32> [[TMP126]], i32 1
; CHECK:    [[SPLIT46:%.*]] = extractelement <4 x i32> [[TMP126]], i32 2
; CHECK:    [[SPLIT47:%.*]] = extractelement <4 x i32> [[TMP126]], i32 3
; CHECK:    [[TMP130:%.*]] = insertelement <8 x i32> undef, i32 [[SPLIT36]], i32 0
; CHECK:    [[TMP131:%.*]] = insertelement <8 x i32> [[TMP130]], i32 [[SPLIT37]], i32 1
; CHECK:    [[TMP132:%.*]] = insertelement <8 x i32> [[TMP131]], i32 [[SPLIT38]], i32 2
; CHECK:    [[TMP133:%.*]] = insertelement <8 x i32> [[TMP132]], i32 [[SPLIT39]], i32 3
; CHECK:    [[TMP134:%.*]] = insertelement <8 x i32> [[TMP133]], i32 [[SPLIT40]], i32 4
; CHECK:    [[TMP135:%.*]] = insertelement <8 x i32> [[TMP134]], i32 [[SPLIT41]], i32 5
; CHECK:    [[TMP136:%.*]] = insertelement <8 x i32> [[TMP135]], i32 [[SPLIT42]], i32 6
; CHECK:    [[TMP137:%.*]] = insertelement <8 x i32> [[TMP136]], i32 [[SPLIT43]], i32 7
; CHECK:    [[TMP138:%.*]] = insertelement <4 x i32> undef, i32 [[SPLIT44]], i32 0
; CHECK:    [[TMP139:%.*]] = insertelement <4 x i32> [[TMP138]], i32 [[SPLIT45]], i32 1
; CHECK:    [[TMP140:%.*]] = insertelement <4 x i32> [[TMP139]], i32 [[SPLIT46]], i32 2
; CHECK:    [[TMP141:%.*]] = insertelement <4 x i32> [[TMP140]], i32 [[SPLIT47]], i32 3
; CHECK:    [[TMP142:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP143:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP142]], i32 0
; CHECK:    [[TMP144:%.*]] = bitcast i32 addrspace(1)* [[TMP143]] to <8 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP144]], <8 x i32> [[TMP137]], i64 4, i1 true)
; CHECK:    [[TMP145:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP146:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP145]], i32 8
; CHECK:    [[TMP147:%.*]] = bitcast i32 addrspace(1)* [[TMP146]] to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP147]], <4 x i32> [[TMP141]], i64 4, i1 true)
; CHECK:    [[TMP148:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP149:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP148]], i32 12
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP149]], i32 [[TMP129]], i64 4, i1 true)
  %4 = call <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %src, i64 4, i1 true, <13 x i32> %mrg)
  call void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %dst, <13 x i32> %4, i64 4, i1 true)

; CHECK:    [[TMP150:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP151:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP150]], i32 0
; CHECK:    [[TMP152:%.*]] = bitcast i32 addrspace(1)* [[TMP151]] to <8 x i32> addrspace(1)*
; CHECK:    [[TMP153:%.*]] = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP152]], i64 4, i1 true, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>)
; CHECK:    [[TMP154:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP155:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP154]], i32 8
; CHECK:    [[TMP156:%.*]] = bitcast i32 addrspace(1)* [[TMP155]] to <4 x i32> addrspace(1)*
; CHECK:    [[TMP157:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP156]], i64 4, i1 true, <4 x i32> <i32 9, i32 10, i32 11, i32 12>)
; CHECK:    [[TMP158:%.*]] = bitcast <13 x i32> addrspace(1)* [[SRC]] to i32 addrspace(1)*
; CHECK:    [[TMP159:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP158]], i32 12
; CHECK:    [[TMP160:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[TMP159]], i64 4, i1 true, i32 13)
; CHECK:    [[SPLIT48:%.*]] = extractelement <8 x i32> [[TMP153]], i32 0
; CHECK:    [[SPLIT49:%.*]] = extractelement <8 x i32> [[TMP153]], i32 1
; CHECK:    [[SPLIT50:%.*]] = extractelement <8 x i32> [[TMP153]], i32 2
; CHECK:    [[SPLIT51:%.*]] = extractelement <8 x i32> [[TMP153]], i32 3
; CHECK:    [[SPLIT52:%.*]] = extractelement <8 x i32> [[TMP153]], i32 4
; CHECK:    [[SPLIT53:%.*]] = extractelement <8 x i32> [[TMP153]], i32 5
; CHECK:    [[SPLIT54:%.*]] = extractelement <8 x i32> [[TMP153]], i32 6
; CHECK:    [[SPLIT55:%.*]] = extractelement <8 x i32> [[TMP153]], i32 7
; CHECK:    [[SPLIT56:%.*]] = extractelement <4 x i32> [[TMP157]], i32 0
; CHECK:    [[SPLIT57:%.*]] = extractelement <4 x i32> [[TMP157]], i32 1
; CHECK:    [[SPLIT58:%.*]] = extractelement <4 x i32> [[TMP157]], i32 2
; CHECK:    [[SPLIT59:%.*]] = extractelement <4 x i32> [[TMP157]], i32 3
; CHECK:    [[TMP161:%.*]] = insertelement <8 x i32> undef, i32 [[SPLIT48]], i32 0
; CHECK:    [[TMP162:%.*]] = insertelement <8 x i32> [[TMP161]], i32 [[SPLIT49]], i32 1
; CHECK:    [[TMP163:%.*]] = insertelement <8 x i32> [[TMP162]], i32 [[SPLIT50]], i32 2
; CHECK:    [[TMP164:%.*]] = insertelement <8 x i32> [[TMP163]], i32 [[SPLIT51]], i32 3
; CHECK:    [[TMP165:%.*]] = insertelement <8 x i32> [[TMP164]], i32 [[SPLIT52]], i32 4
; CHECK:    [[TMP166:%.*]] = insertelement <8 x i32> [[TMP165]], i32 [[SPLIT53]], i32 5
; CHECK:    [[TMP167:%.*]] = insertelement <8 x i32> [[TMP166]], i32 [[SPLIT54]], i32 6
; CHECK:    [[TMP168:%.*]] = insertelement <8 x i32> [[TMP167]], i32 [[SPLIT55]], i32 7
; CHECK:    [[TMP169:%.*]] = insertelement <4 x i32> undef, i32 [[SPLIT56]], i32 0
; CHECK:    [[TMP170:%.*]] = insertelement <4 x i32> [[TMP169]], i32 [[SPLIT57]], i32 1
; CHECK:    [[TMP171:%.*]] = insertelement <4 x i32> [[TMP170]], i32 [[SPLIT58]], i32 2
; CHECK:    [[TMP172:%.*]] = insertelement <4 x i32> [[TMP171]], i32 [[SPLIT59]], i32 3
; CHECK:    [[TMP173:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP174:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP173]], i32 0
; CHECK:    [[TMP175:%.*]] = bitcast i32 addrspace(1)* [[TMP174]] to <8 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v8i32.v8i32(<8 x i32> addrspace(1)* [[TMP175]], <8 x i32> [[TMP168]], i64 4, i1 true)
; CHECK:    [[TMP176:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP177:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP176]], i32 8
; CHECK:    [[TMP178:%.*]] = bitcast i32 addrspace(1)* [[TMP177]] to <4 x i32> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[TMP178]], <4 x i32> [[TMP172]], i64 4, i1 true)
; CHECK:    [[TMP179:%.*]] = bitcast <13 x i32> addrspace(1)* [[DST]] to i32 addrspace(1)*
; CHECK:    [[TMP180:%.*]] = getelementptr i32, i32 addrspace(1)* [[TMP179]], i32 12
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP180]], i32 [[TMP160]], i64 4, i1 true)
  %5 = call <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %src, i64 4, i1 true, <13 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13>)
  call void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)* %dst, <13 x i32> %5, i64 4, i1 true)

; CHECK:    [[TMP181:%.*]] = bitcast i48 [[MRG1:%.*]] to <3 x i16>
; CHECK:    [[TMP182:%.*]] = bitcast i48 addrspace(1)* [[SRC1:%.*]] to <3 x i16> addrspace(1)*
; CHECK:    [[TMP183:%.*]] = bitcast <3 x i16> addrspace(1)* [[TMP182]] to i16 addrspace(1)*
; CHECK:    [[TMP184:%.*]] = getelementptr i16, i16 addrspace(1)* [[TMP183]], i32 2
; CHECK:    [[TMP185:%.*]] = extractelement <3 x i16> [[TMP181]], i32 0
; CHECK:    [[TMP186:%.*]] = insertelement <2 x i16> undef, i16 [[TMP185]], i32 0
; CHECK:    [[TMP187:%.*]] = extractelement <3 x i16> [[TMP181]], i32 1
; CHECK:    [[TMP188:%.*]] = insertelement <2 x i16> [[TMP186]], i16 [[TMP187]], i32 1
; CHECK:    [[TMP189:%.*]] = extractelement <3 x i16> [[TMP181]], i32 2
; CHECK:    [[TMP190:%.*]] = bitcast <3 x i16> addrspace(1)* [[TMP182]] to <2 x i16> addrspace(1)*
; CHECK:    [[TMP191:%.*]] = call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)* [[TMP190]], i64 4, i1 true, <2 x i16> [[TMP188]])
; CHECK:    [[ELT0:%.*]] = extractelement <2 x i16> [[TMP191]], i32 0
; CHECK:    [[ELT1:%.*]] = extractelement <2 x i16> [[TMP191]], i32 1
; CHECK:    [[TMP192:%.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* [[TMP184]], i64 4, i1 true, i16 [[TMP189]])
; CHECK:    [[TMP193:%.*]] = insertelement <3 x i16> undef, i16 [[ELT0]], i32 0
; CHECK:    [[TMP194:%.*]] = insertelement <3 x i16> [[TMP193]], i16 [[ELT1]], i32 1
; CHECK:    [[TMP195:%.*]] = insertelement <3 x i16> [[TMP194]], i16 [[TMP192]], i32 2
; CHECK:    [[TMP196:%.*]] = bitcast <3 x i16> [[TMP195]] to i48
  %6 = call i48 @llvm.genx.GenISA.PredicatedLoad.i48.p1i48.i48(i48 addrspace(1)* %src1, i64 4, i1 true, i48 %mrg1)
; CHECK:    [[TMP197:%.*]] = bitcast i48 [[TMP196]] to <3 x i16>
; CHECK:    [[TMP198:%.*]] = bitcast i48 addrspace(1)* [[DST1:%.*]] to <3 x i16> addrspace(1)*
; CHECK:    [[TMP199:%.*]] = bitcast <3 x i16> addrspace(1)* [[TMP198]] to i16 addrspace(1)*
; CHECK:    [[TMP200:%.*]] = getelementptr i16, i16 addrspace(1)* [[TMP199]], i32 2
; CHECK:    [[ELT0:%.*]] = extractelement <3 x i16> [[TMP197]], i32 0
; CHECK:    [[ELT1:%.*]] = extractelement <3 x i16> [[TMP197]], i32 1
; CHECK:    [[ELT2:%.*]] = extractelement <3 x i16> [[TMP197]], i32 2
; CHECK:    [[TMP201:%.*]] = insertelement <2 x i16> undef, i16 [[ELT0]], i32 0
; CHECK:    [[TMP202:%.*]] = insertelement <2 x i16> [[TMP201]], i16 [[ELT1]], i32 1
; CHECK:    [[TMP203:%.*]] = bitcast <3 x i16> addrspace(1)* [[TMP198]] to <2 x i16> addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1v2i16.v2i16(<2 x i16> addrspace(1)* [[TMP203]], <2 x i16> [[TMP202]], i64 4, i1 true)
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i16.i16(i16 addrspace(1)* [[TMP200]], i16 [[ELT2]], i64 4, i1 true)
  call void @llvm.genx.GenISA.PredicatedStore.i48.p1i48.i48(i48 addrspace(1)* %dst1, i48 %6, i64 4, i1 true)

  ret void
}

declare <13 x i32> @llvm.genx.GenISA.PredicatedLoad.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)*, i64, i1, <13 x i32>)
declare void @llvm.genx.GenISA.PredicatedStore.v13i32.p1v13i32.v13i32(<13 x i32> addrspace(1)*, <13 x i32>, i64, i1)
declare i48 @llvm.genx.GenISA.PredicatedLoad.i48.p1i48.i48(i48 addrspace(1)*, i64, i1, i48)
declare void @llvm.genx.GenISA.PredicatedStore.i48.p1i48.i48(i48 addrspace(1)*, i48, i64, i1)
