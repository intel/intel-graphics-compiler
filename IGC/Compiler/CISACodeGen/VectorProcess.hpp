/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
            MESSAGE_A32_QWORD_SCATTERED_RW,
            MESSAGE_A32_LSC_RW,
            MESSAGE_A64_LSC_RW
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
            //    TGL : no 8DW scattered message.
            //          use A64_SCATTERED_MAX_BYTES_4DW
            A32_UNTYPED_MAX_BYTES = 16,
            A32_BYTE_SCATTERED_MAX_BYTES = 4,
            A64_UNTYPED_MAX_BYTES = 16,
            A64_SCATTERED_MAX_BYTES_8DW_SIMD8 = 32,
            A64_SCATTERED_MAX_BYTES_4DW = 16,
            A64_BYTE_SCATTERED_MAX_BYTES_8B = 8,
            A64_BYTE_SCATTERED_MAX_BYTES = 4,
            A32_A64_BYTE_LSC_LOAD_STORE_MAX_BYTES_SIMD32 = 16,
            A32_A64_BYTE_LSC_LOAD_STORE_MAX_BYTES_SIMD16 = 32,
            A32_A64_BYTE_LSC_LOAD_STORE_MAX_BYTES = 64,
            A32_A64_DATA_SIZE_LSC_LOAD_STORE_MAX_BYTES = 64
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


        void getLSCInfo(llvm::Type* Ty, uint32_t Align, CodeGenContext* ctx, bool useA32, bool transpose);

    private:
        const EmitPass* m_emitter;

        VectorMessage(const VectorMessage&);   // not implemented
        void operator=(const VectorMessage&);  // not implemented
    };
}
