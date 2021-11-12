/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////
// This file has been split into:
// G4_IR/* (for IR_Builder functions)
// VisaToG4/Translate* for the translate**** functions
//
//   TranslateALU              - move, logic, and basic ALU ops
//   TranslateBranches         - for goto, join, etc...
//   TranslateMath             - for complex math sequences (e.g. IEEE macros)
//   TranslateMisc             - for oddball leftover stuff (e.g. lifetime)
//   TranslateSend3D           - for 3D send operations
//   TranslateSendLdStLegacy   - for other legacy send operations inlcuding:
//                              A64 SVM, A32/SLM, oword block, ...
//   TranslateSendLdStLsc      - for lsc ops (lsc fences go in SendSync)
//   TranslateSendMedia        - media, VA, and sampler VA functions
//   TranslateSendRaw          - for existing raw sends
//   TranslateSendSync         - fences, barriers, etc...

/////////////////////////////////////////////////
// TODO: remove this file after a few months

// non-empty compilation unit
int visa_to_g4_translation_interface_dummy = 0;
