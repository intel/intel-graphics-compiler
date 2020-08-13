/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
//   TranslateSendMedia        - media, VA, and sampler VA functions
//   TranslateSendRaw          - for existing raw sends
//   TranslateSendSync         - fences, barriers, etc...

/////////////////////////////////////////////////
// TODO: remove this file after a few months

// non-empty compilation unit
int visa_to_g4_translation_interface_dummy = 0;
