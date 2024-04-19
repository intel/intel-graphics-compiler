;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt  --regkey DumpLoopSink=1 --regkey PrintToConsole=1 --regkey CodeSinkingLoadSchedulingInstr=1 --regkey LoopSinkMinSave=0 --regkey ForceLoadsLoopSink=1 --regkey ForceLoopSink=1 %enable-basic-aa% --igc-code-loop-sinking -disable-output -S %s 2>&1 | FileCheck %s

; CHECK: Loop pressure increased after sinking.
; CHECK: >> Reverting the changes.


define spir_kernel void @test(<8 x i32> %arg, <8 x i32> %arg1, i16 %arg2, i32 %arg3, i32 %arg4, <8 x i32> addrspace(131074)* %arg5, <8 x i32> %arg6, i32 %arg7, i32 %arg8, i32 %arg9, i32 %arg10, <2 x i32> addrspace(131072)* %arg11, i32 %arg12, i32 %arg13, i32 %arg14, i32 %arg15, <2 x i32> addrspace(131072)* %arg16, i32 %arg17, i32 %arg18, i32 %arg19, i32 %arg20, <2 x i32> addrspace(131072)* %arg21, <2 x i32> %arg22, i32 %arg23, i32 %arg24, i32 %arg25, i32 %arg26, <2 x i32> addrspace(131072)* %arg27, <8 x i32> addrspace(131074)* %arg28, <8 x i32> %arg29, i32 %arg30, i32 %arg31, i32 %arg32, i32 %arg33, <2 x i32> addrspace(131072)* %arg34, i32 %arg35, i32 %arg36, i32 %arg37, i32 %arg38, <2 x i32> addrspace(131072)* %arg39, <2 x i32> %arg40, i32 %arg41, i32 %arg42, i32 %arg43, i32 %arg44, <2 x i32> addrspace(131072)* %arg45, <2 x i32> %arg46, i32 %arg47, i32 %arg48, i32 %arg49, i32 %arg50, <2 x i32> addrspace(131072)* %arg51, <8 x i32> addrspace(131074)* %arg52, <8 x i32> %arg53, i32 %arg54, i32 %arg55, i32 %arg56, i32 %arg57, <2 x i32> addrspace(131072)* %arg58, i32 %arg59, i32 %arg60, i32 %arg61, i32 %arg62, <2 x i32> addrspace(131072)* %arg63, i32 %arg64, i32 %arg65, i32 %arg66, i32 %arg67, <2 x i32> addrspace(131072)* %arg68, i32 %arg69, i32 %arg70, i32 %arg71, i32 %arg72, <2 x i32> addrspace(131072)* %arg73, <2 x i32> %arg74, <8 x i32> addrspace(131074)* %arg75, <8 x i32> %arg76, i32 %arg77, i32 %arg78, i32 %arg79, i32 %arg80, <2 x i32> addrspace(131072)* %arg81, i32 %arg82, i32 %arg83, i32 %arg84, i32 %arg85, <2 x i32> addrspace(131072)* %arg86, i32 %arg87, i32 %arg88, i32 %arg89, i32 %arg90, <2 x i32> addrspace(131072)* %arg91, i32 %arg92, i32 %arg93, i32 %arg94, i32 %arg95, <2 x i32> addrspace(131072)* %arg96, <8 x i32> addrspace(131074)* %arg97, <8 x i32> %arg98, i32 %arg99, i32 %arg100, i32 %arg101, i32 %arg102, <2 x i32> addrspace(131072)* %arg103, <2 x i32> %arg104, i32 %arg105, i32 %arg106, i32 %arg107, i32 %arg108, <2 x i32> addrspace(131072)* %arg109, <2 x i32> %arg110, i32 %arg111, i32 %arg112, i32 %arg113, i32 %arg114, <2 x i32> addrspace(131072)* %arg115, <2 x i32> %arg116, i32 %arg117, i32 %arg118, i32 %arg119, i32 %arg120, <2 x i32> addrspace(131072)* %arg121, <2 x i32> %arg122, <8 x i32> addrspace(131074)* %arg123, <8 x i32> %arg124, i32 %arg125, i32 %arg126, i32 %arg127, i32 %arg128, <2 x i32> addrspace(131072)* %arg129, i32 %arg130, i32 %arg131, i32 %arg132, i32 %arg133, <2 x i32> addrspace(131072)* %arg134, i32 %arg135, i32 %arg136, i32 %arg137, i32 %arg138, <2 x i32> addrspace(131072)* %arg139, <2 x i32> %arg140, i32 %arg141, i32 %arg142, i32 %arg143, i32 %arg144, <2 x i32> addrspace(131072)* %arg145, <8 x i32> addrspace(131074)* %arg146, <8 x i32> %arg147, i32 %arg148, i32 %arg149, i32 %arg150, i32 %arg151, <2 x i32> addrspace(131072)* %arg152, i32 %arg153, i32 %arg154, i32 %arg155, i32 %arg156, <2 x i32> addrspace(131072)* %arg157, <2 x i32> %arg158, i32 %arg159, i32 %arg160, i32 %arg161, i32 %arg162, <2 x i32> addrspace(131072)* %arg163, <2 x i32> %arg164, i32 %arg165, i32 %arg166, i32 %arg167, i32 %arg168, <2 x i32> addrspace(131072)* %arg169, <2 x i32> %arg170, <8 x i32> addrspace(131074)* %arg171, <8 x i32> %arg172, i32 %arg173, i32 %arg174, i32 %arg175, i32 %arg176, <2 x i32> addrspace(131072)* %arg177, <2 x i32> %arg178, i32 %arg179, i32 %arg180, i32 %arg181, i32 %arg182, <2 x i32> addrspace(131072)* %arg183, i32 %arg184, i32 %arg185, i32 %arg186, i32 %arg187, <2 x i32> addrspace(131072)* %arg188, i32 %arg189, i32 %arg190, i32 %arg191, i32 %arg192, <2 x i32> addrspace(131072)* %arg193, <8 x i32> addrspace(131074)* %arg194, <8 x i32> %arg195, i32 %arg196, i32 %arg197, i32 %arg198, i32 %arg199, <2 x i32> addrspace(131072)* %arg200, i32 %arg201, i32 %arg202, i32 %arg203, i32 %arg204, <2 x i32> addrspace(131072)* %arg205, <2 x i32> %arg206, i32 %arg207, i32 %arg208, i32 %arg209, i32 %arg210, <2 x i32> addrspace(131072)* %arg211, i32 %arg212, i32 %arg213, i32 %arg214, i32 %arg215, <2 x i32> addrspace(131072)* %arg216, <8 x i32> addrspace(131074)* %arg217, <8 x i32> %arg218, i32 %arg219, i32 %arg220, i32 %arg221, i32 %arg222, <2 x i32> addrspace(131072)* %arg223, i32 %arg224, i32 %arg225, i32 %arg226, i32 %arg227, <2 x i32> addrspace(131072)* %arg228, i32 %arg229, i32 %arg230, i32 %arg231, i32 %arg232, <2 x i32> addrspace(131072)* %arg233, <2 x i32> %arg234, i32 %arg235, i32 %arg236, i32 %arg237, i32 %arg238, <2 x i32> addrspace(131072)* %arg239, <2 x i32> %arg240, i32 %arg241, i32 %arg242, i32 %arg243, <2 x i32> addrspace(131072)* %arg244, <8 x i32> %arg245, i32 %arg246, i32 %arg247, i32 %arg248, i32 %arg249, <2 x i32> addrspace(131072)* %arg250, i32 %arg251, i32 %arg252, i32 %arg253, i32 %arg254, <2 x i32> addrspace(131072)* %arg255, <2 x i32> %arg256, i32 %arg257, i32 %arg258, i32 %arg259, i32 %arg260, <2 x i32> addrspace(131072)* %arg261, <8 x i32> addrspace(131074)* %arg262, <8 x i32> %arg263, i32 %arg264, i32 %arg265, i32 %arg266, i32 %arg267, <2 x i32> addrspace(131072)* %arg268, i32 %arg269, i32 %arg270, i32 %arg271, i32 %arg272, <2 x i32> addrspace(131072)* %arg273, i32 %arg274, i32 %arg275, i32 %arg276, i32 %arg277, <2 x i32> addrspace(131072)* %arg278, i32 %arg279, i32 %arg280, i32 %arg281, i32 %arg282, <2 x i32> addrspace(131072)* %arg283, <2 x i32> %arg284, <8 x i32> addrspace(131074)* %arg285, <8 x i32> %arg286, i32 %arg287, i32 %arg288, i32 %arg289, i32 %arg290, <2 x i32> addrspace(131072)* %arg291, <2 x i32> %arg292, i32 %arg293, <2 x i32> addrspace(131072)* %arg294, <2 x i32> %arg295, i32 %arg296, i32 %arg297, i32 %arg298, i32 %arg299, <2 x i32> addrspace(131072)* %arg300, <2 x i32> %arg301, i32 %arg302, i32 %arg303, i32 %arg304, i32 %arg305, <2 x i32> addrspace(131072)* %arg306, <8 x i32> addrspace(131074)* %arg307, <8 x i32> %arg308, i32 %arg309, i32 %arg310, i32 %arg311, i32 %arg312, <2 x i32> addrspace(131072)* %arg313, i32 %arg314, i32 %arg315, i32 %arg316, i32 %arg317, <2 x i32> addrspace(131072)* %arg318, i32 %arg319, i32 %arg320, i32 %arg321, i32 %arg322, <2 x i32> addrspace(131072)* %arg323, i32 %arg324, i32 %arg325, i32 %arg326, i32 %arg327, <2 x i32> addrspace(131072)* %arg328, <2 x i32> %arg329, <8 x i32> %arg330, i32 %arg331, i32 %arg332, i32 %arg333, i32 %arg334, <2 x i32> addrspace(131072)* %arg335, i32 %arg336, i32 %arg337, i32 %arg338, i32 %arg339, <2 x i32> addrspace(131072)* %arg340, i32 %arg341, i32 %arg342, i32 %arg343, i32 %arg344, <2 x i32> addrspace(131072)* %arg345, <2 x i32> %arg346, i32 %arg347, i32 %arg348, i32 %arg349, i32 %arg350, <2 x i32> addrspace(131072)* %arg351, <2 x i32> %arg352, i32 %arg353, i32 %arg354, i32 %arg355, i32 %arg356, <2 x i32> addrspace(131072)* %arg357, <8 x i32> %arg358, i32 %arg359, i32 %arg360, i32 %arg361, i32 %arg362, <2 x i32> addrspace(131072)* %arg363, i32 %arg364, i32 %arg365, i32 %arg366, <2 x i32> addrspace(131072)* %arg367, <8 x i32> addrspace(131074)* %arg368, <8 x i32> %arg369, i32 %arg370, i32 %arg371, i32 %arg372, i32 %arg373, <2 x i32> addrspace(131072)* %arg374, <2 x i32> %arg375, i32 %arg376, i32 %arg377, i32 %arg378, i32 %arg379, <2 x i32> addrspace(131072)* %arg380, i32 %arg381, i32 %arg382, i32 %arg383, i32 %arg384, <2 x i32> addrspace(131072)* %arg385, i32 %arg386, i32 %arg387, i32 %arg388, i32 %arg389, <2 x i32> addrspace(131072)* %arg390, <8 x i32> addrspace(131074)* %arg391, <8 x i32> %arg392, i32 %arg393, i32 %arg394, i32 %arg395, i32 %arg396, <2 x i32> addrspace(131072)* %arg397, <2 x i32> addrspace(131072)* %arg398, <2 x i32> %arg399, i32 %arg400, i32 %arg401, i32 %arg402, i32 %arg403, <2 x i32> addrspace(131072)* %arg404, <2 x i32> %arg405, i32 %arg406, i32 %arg407, i32 %arg408, i32 %arg409, <2 x i32> addrspace(131072)* %arg410, <2 x i32> %arg411, <2 x i32> %arg412, <8 x i32> %arg413, i32 %arg414, i32 %arg415, i32 %arg416, i32 %arg417, <2 x i32> addrspace(131072)* %arg418, <2 x i32> %arg419, i32 %arg420, i32 %arg421, i32 %arg422, i32 %arg423, <2 x i32> addrspace(131072)* %arg424, <2 x i32> %arg425, i32 %arg426, i32 %arg427, i32 %arg428, i32 %arg429, <2 x i32> addrspace(131072)* %arg430, <2 x i32> %arg431, i32 %arg432, i32 %arg433, i32 %arg434, <2 x i32> addrspace(131072)* %arg435, <2 x i32> %arg436, <8 x i32> %arg437, i32 %arg438, i32 %arg439, i32 %arg440, i32 %arg441, <2 x i32> addrspace(131072)* %arg442, <2 x i32> %arg443, i32 %arg444, i32 %arg445, i32 %arg446, i32 %arg447, <2 x i32> addrspace(131072)* %arg448, i32 %arg449, i32 %arg450, i32 %arg451, i32 %arg452, <2 x i32> addrspace(131072)* %arg453, <8 x i32> %arg454, i32 %arg455, i32 %arg456, i32 %arg457, i32 %arg458, <2 x i32> addrspace(131072)* %arg459, <2 x i32> %arg460, i32 %arg461, <2 x i32> addrspace(131072)* %arg462, <2 x i32> %arg463, <2 x i32> addrspace(131072)* %arg464, <2 x i32> %arg465, i32 %arg466, <2 x i32> addrspace(131072)* %arg467, i32 %arg468, i32 %arg469, i32 %arg470, <2 x i32> addrspace(131072)* %arg471, <2 x i32> %arg472, i32 %arg473, <2 x i32> addrspace(131072)* %arg474, <2 x i32> %arg475, i32 %arg476, i32 %arg477, <2 x i32> addrspace(131072)* %arg478, i64 %arg479, i64 %arg480, i64 %arg481, i64 %arg482, i64 %arg483, i64 %arg484, i64 %arg485, i64 %arg486, i64 %arg487, i64 %arg488, i64 %arg489, i64 %arg490, i64 %arg491, i64 %arg492, i64 %arg493, i64 %arg494, i64 %arg495, i64 %arg496, i64 %arg497, i64 %arg498, i64 %arg499, i64 %arg500, i64 %arg501, i64 %arg502, i64 %arg503, i64 %arg504, i64 %arg505, i64 %arg506, i64 %arg507, i64 %arg508, i64 %arg509, i64 %arg510, i64 %arg511, i64 %arg512, i64 %arg513, i64 %arg514, i64 %arg515, i64 %arg516, i64 %arg517, i64 %arg518, i64 %arg519, i64 %arg520, i64 %arg521, i64 %arg522, i64 %arg523, i64 %arg524, i64 %arg525, i64 %arg526, i64 %arg527) {
bb:
  %tmp = load <2 x i32>, <2 x i32> addrspace(131072)* %arg11, align 8
  %tmp528 = bitcast <2 x i32> %tmp to i64
  %tmp529 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg16, align 8
  %tmp530 = bitcast <2 x i32> %tmp529 to i64
  %tmp531 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg27, align 8
  %tmp532 = bitcast <2 x i32> %tmp531 to i64
  %tmp533 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg51, align 8
  %tmp534 = bitcast <2 x i32> %tmp533 to i64
  %tmp535 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg58, align 8
  %tmp536 = bitcast <2 x i32> %tmp535 to i64
  %tmp537 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg63, align 8
  %tmp538 = bitcast <2 x i32> %tmp537 to i64
  %tmp539 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg68, align 8
  %tmp540 = bitcast <2 x i32> %tmp539 to i64
  %tmp541 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg81, align 8
  %tmp542 = bitcast <2 x i32> %tmp541 to i64
  %tmp543 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg86, align 8
  %tmp544 = bitcast <2 x i32> %tmp543 to i64
  %tmp545 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg91, align 8
  %tmp546 = bitcast <2 x i32> %tmp545 to i64
  %tmp547 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg96, align 8
  %tmp548 = bitcast <2 x i32> %tmp547 to i64
  %tmp549 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg129, align 8
  %tmp550 = bitcast <2 x i32> %tmp549 to i64
  %tmp551 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg134, align 8
  %tmp552 = bitcast <2 x i32> %tmp551 to i64
  %tmp553 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg145, align 8
  %tmp554 = bitcast <2 x i32> %tmp553 to i64
  %tmp555 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg188, align 8
  %tmp556 = bitcast <2 x i32> %tmp555 to i64
  %tmp557 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg200, align 8
  %tmp558 = bitcast <2 x i32> %tmp557 to i64
  %tmp559 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg211, align 8
  %tmp560 = bitcast <2 x i32> %tmp559 to i64
  %tmp561 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg121, align 8
  %tmp562 = bitcast <2 x i32> %tmp561 to i64
  %tmp563 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg34, align 8
  %tmp564 = bitcast <2 x i32> %tmp563 to i64
  %tmp565 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg163, align 8
  %tmp566 = bitcast <2 x i32> %tmp565 to i64
  %tmp567 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg216, align 8
  %tmp568 = bitcast <2 x i32> %tmp567 to i64
  %tmp569 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg45, align 8
  %tmp570 = bitcast <2 x i32> %tmp569 to i64
  %tmp571 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg169, align 8
  %tmp572 = bitcast <2 x i32> %tmp571 to i64
  %tmp573 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg115, align 8
  %tmp574 = bitcast <2 x i32> %tmp573 to i64
  %tmp575 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg103, align 8
  %tmp576 = bitcast <2 x i32> %tmp575 to i64
  %tmp577 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg223, align 8
  %tmp578 = bitcast <2 x i32> %tmp577 to i64
  %tmp579 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg205, align 8
  %tmp580 = bitcast <2 x i32> %tmp579 to i64
  %tmp581 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg109, align 8
  %tmp582 = bitcast <2 x i32> %tmp581 to i64
  %tmp583 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg39, align 8
  %tmp584 = bitcast <2 x i32> %tmp583 to i64
  %tmp585 = load <2 x i32>, <2 x i32> addrspace(131072)* null, align 8
  %tmp586 = bitcast <2 x i32> %tmp585 to i64
  %tmp587 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg177, align 8
  %tmp588 = bitcast <2 x i32> %tmp587 to i64
  %tmp589 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg183, align 8
  %tmp590 = bitcast <2 x i32> %tmp589 to i64
  %tmp591 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg21, align 8
  %tmp592 = bitcast <2 x i32> %tmp591 to i64
  %tmp593 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg193, align 8
  %tmp594 = bitcast <2 x i32> %tmp593 to i64
  %tmp595 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg73, align 8
  %tmp596 = bitcast <2 x i32> %tmp595 to i64
  %tmp597 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg228, align 8
  %tmp598 = bitcast <2 x i32> %tmp597 to i64
  %tmp599 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg139, align 8
  %tmp600 = bitcast <2 x i32> %tmp599 to i64
  %tmp601 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg152, align 8
  %tmp602 = bitcast <2 x i32> %tmp601 to i64
  %tmp603 = load <2 x i32>, <2 x i32> addrspace(131072)* %arg157, align 8
  %tmp604 = bitcast <2 x i32> %tmp603 to i64
  br label %bb605

bb605:                                            ; preds = %bb683, %bb
  %tmp606 = mul i64 %tmp604, 44
  %tmp607 = zext i32 %arg3 to i64
  store <2 x i32> zeroinitializer, <2 x i32> addrspace(131073)* null, align 16
  %tmp608 = mul i64 %tmp528, %tmp528
  %tmp609 = mul i64 %tmp530, %tmp530
  %tmp610 = mul i64 %tmp532, %tmp532
  %tmp611 = mul i64 %arg525, %arg479
  %tmp612 = mul i64 %tmp534, %tmp534
  %tmp613 = mul i64 %tmp536, %tmp536
  %tmp614 = mul i64 %tmp538, %tmp538
  %tmp615 = mul i64 %tmp540, %tmp540
  %tmp616 = mul i64 %tmp542, %tmp542
  %tmp617 = mul i64 %tmp544, %tmp544
  %tmp618 = mul i64 %tmp546, %tmp546
  %tmp619 = mul i64 %tmp548, %tmp548
  %tmp620 = mul i64 %arg479, %arg485
  %tmp621 = mul i64 %tmp550, %tmp550
  %tmp622 = mul i64 %tmp552, %tmp552
  %tmp623 = mul i64 %arg487, %arg482
  %tmp624 = mul i64 %tmp554, %tmp554
  %tmp625 = mul i64 %arg520, %arg479
  %tmp626 = mul i64 %arg488, %arg481
  %tmp627 = mul i64 %arg489, %arg480
  %tmp628 = mul i64 %arg479, %arg490
  %tmp629 = mul i64 %arg491, %arg479
  %tmp630 = mul i64 %arg513, %arg479
  %tmp631 = mul i64 %tmp556, %tmp556
  %tmp632 = mul i64 %arg519, %arg479
  %tmp633 = mul i64 %tmp558, %tmp558
  %tmp634 = mul i64 %tmp560, %tmp560
  %tmp635 = mul i64 %tmp562, %tmp562
  %tmp636 = mul i64 %tmp564, %tmp564
  %tmp637 = mul i64 %tmp566, %tmp566
  %tmp638 = mul i64 %arg493, %arg483
  %tmp639 = mul i64 %tmp568, %tmp568
  %tmp640 = mul i64 %tmp570, %tmp570
  %tmp641 = mul i64 %arg495, %arg484
  %tmp642 = mul i64 %tmp572, %tmp572
  %tmp643 = mul i64 %arg523, %arg479
  %tmp644 = mul i64 %arg526, %arg479
  %tmp645 = mul i64 %tmp574, %tmp574
  %tmp646 = mul i64 %arg496, %arg479
  %tmp647 = mul i64 %arg497, %arg479
  %tmp648 = mul i64 %arg498, %arg479
  %tmp649 = mul i64 %arg479, %arg499
  %tmp650 = mul i64 %tmp576, %tmp576
  %tmp651 = mul i64 %tmp578, %tmp578
  %tmp652 = mul i64 %arg515, %arg479
  %tmp653 = mul i64 %tmp580, %tmp580
  %tmp654 = mul i64 %arg500, %arg479
  %tmp655 = mul i64 %tmp582, %tmp582
  %tmp656 = mul i64 %tmp584, %tmp584
  %tmp657 = mul i64 %arg501, %arg479
  %tmp658 = mul i64 %arg479, %arg502
  %tmp659 = mul i64 %tmp586, %tmp586
  %tmp660 = mul i64 %tmp588, %tmp588
  %tmp661 = mul i64 %tmp590, %tmp590
  %tmp662 = mul i64 %tmp592, %tmp592
  %tmp663 = mul i64 %arg479, %arg503
  %tmp664 = mul i64 %tmp594, %tmp594
  %tmp665 = mul i64 %tmp596, %tmp596
  %tmp666 = mul i64 %tmp598, %tmp598
  %tmp667 = mul i64 %tmp600, %tmp600
  %tmp668 = mul i64 %arg479, %arg504
  %tmp669 = mul i64 %arg479, %arg505
  %tmp670 = add i64 %arg507, %arg508
  %tmp671 = add i64 %tmp670, %arg510
  %tmp672 = mul i64 %arg511, %arg486
  %tmp673 = mul i64 %arg514, %arg506
  %tmp674 = add i64 1, %arg516
  %tmp675 = mul i64 %arg517, %arg494
  %tmp676 = mul i64 %arg524, %arg479
  %tmp677 = mul i64 %tmp602, %tmp602
  %tmp678 = mul i64 %arg518, %arg492
  %tmp679 = mul i64 %arg479, %arg521
  %tmp680 = add i64 %arg522, %tmp604
  %tmp681 = add i64 %arg512, %arg527
  %tmp682 = mul i64 %arg509, %arg479
  br label %bb683

bb683:                                            ; preds = %bb605
  br label %bb605
}

!igc.functions = !{}
