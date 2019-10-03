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

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"

namespace llvm
{
    class DataLayout;
    class Type;
    class FunctionPass;
}

namespace IGC
{
    llvm::FunctionPass* createVectorPreProcessPass();
    llvm::FunctionPass* createVectorProcessPass();

    class EmitPass;

    // Used to map vector to its corresponding messages
    class VectorMessage
    {
    public:
        enum MESSAGE_KIND
        {
            MESSAGE_A32_UNTYPED_SURFACE_RW,
            MESSAGE_A32_BYTE_SCATTERED_RW,
            MESSAGE_A64_UNTYPED_SURFACE_RW,
            MESSAGE_A64_SCATTERED_RW,
        };

        // VECMESSAGEINFO_MAX_LEN:
        //   VectorPreProcess splits larger vectors. After that, the
        //   max vector would be <8 x i32>, which would be 8 insts at
        //   most (when it is unaligned) using byte scattered messages.
        //   Therefore, using 16 would be enough.
        enum
        {
            VECMESSAGEINFO_MAX_LEN = 16,

            // SKL+
            // Exception
            //    BDW : use A64_BYTE_SCATTERED_MAX_BYTES_8B
            A32_UNTYPED_MAX_BYTES = 16,
            A32_BYTE_SCATTERED_MAX_BYTES = 4,
            A64_UNTYPED_MAX_BYTES = 16,
            A64_SCATTERED_MAX_BYTES_8DW_SIMD8 = 32,
            A64_SCATTERED_MAX_BYTES_4DW = 16,
            A64_BYTE_SCATTERED_MAX_BYTES_8B = 8,
            A64_BYTE_SCATTERED_MAX_BYTES = 4,
        };

        // Calculated by getInfo().
        struct
        {
            MESSAGE_KIND kind;
            uint16_t  startByte;
            VISA_Type blkType;      // type of a block (B, D, Q, etc)
            uint16_t  blkInBytes;   // the block size in bytes
            uint16_t  numBlks;      // the number of blocks
        } insts[VECMESSAGEINFO_MAX_LEN];
        uint16_t  numInsts;

        VectorMessage(EmitPass* emitter) : m_emitter(emitter) {}
        void getInfo(llvm::Type* Ty, uint32_t Align, bool useA32,
            bool forceByteScatteredRW = false);



    private:
        const EmitPass* m_emitter;

        VectorMessage(const VectorMessage&);   // not implemented
        void operator=(const VectorMessage&);  // not implemented
    };
}
