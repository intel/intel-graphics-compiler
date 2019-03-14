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

//*****************************************************************************/
// Generic Header
//*****************************************************************************/
#include "IBiF_Header.cl"

//*****************************************************************************/
// Math builtin functions
//*****************************************************************************/
#include "IBiF_Math_Common.cl"

//*****************************************************************************/
// Arithmetic opcodes
//*****************************************************************************/
#include "arithmetic.cl"

//*****************************************************************************/
// Atomics opcodes
//*****************************************************************************/
#include "atomics.cl"

//*****************************************************************************/
// Barrier opcodes
//*****************************************************************************/
#include "barrier.cl"


//*****************************************************************************/
// Bits opcodes
//*****************************************************************************/
#include "bits.cl"

//*****************************************************************************/
// Bits opcodes
//*****************************************************************************/
#include "conversions.cl"

//*****************************************************************************/
// Device-Side Enqueue opcodes
//*****************************************************************************/
#include "device_side_enqueue.cl"

//*****************************************************************************/
// Group opcodes
//*****************************************************************************/
#include "group.cl"

#ifndef OMIT_IMAGES_CL_INCLUDE
//*****************************************************************************/
// Image opcodes
//*****************************************************************************/
#include "images.cl"
#endif // OMIT_IMAGES_CL_INCLUDE

//*****************************************************************************/
// Pipe opcodes
//*****************************************************************************/
#include "pipe.cl"

//*****************************************************************************/
// Prefetch opcodes
//*****************************************************************************/
#include "prefetch.cl"

//*****************************************************************************/
// Relational opcodes
//*****************************************************************************/
#include "relational.cl"

//*****************************************************************************/
// Shuffle opcodes
//*****************************************************************************/
#include "shuffle.cl"

//*****************************************************************************/
// Vload/Vstore opcodes
//*****************************************************************************/
#include "vloadvstore.cl"

//*****************************************************************************/
// 64bit Math Emulation
//*****************************************************************************/
#ifdef __IGC_BUILD__
#include "IGCBiF_Math_64bitDiv.cl"
#endif

//*****************************************************************************/
// Read Clock Extension Opcodes
//*****************************************************************************/
#include "clock.cl"

