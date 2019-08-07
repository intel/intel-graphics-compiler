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
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "common/Types.hpp"

/***********************************************************************************
This File contains all the helper functions to generate the message descriptor for the different
messages we use for 3D compiler, once the messages are implemented in C-ISA we can get rid of this.
Most likely some messages will stay encoded manually by the code generator

*************************************************************************************/


namespace IGC
{
    EU_SAMPLER_SIMD_MODE samplerSimdMode(SIMDMode simd)
    {
        if (simd == SIMDMode::SIMD8)
        {
            return EU_SAMPLER_SIMD_SIMD8;
        }
        else if (simd == SIMDMode::SIMD16)
        {
            return EU_SAMPLER_SIMD_SIMD16;
        }
        assert(0);
        return EU_SAMPLER_SIMD_SIMD8;
    }

    uint Sampler(
        uint messageLength,
        uint responseLength,
        bool  headerPresent,
        EU_SAMPLER_SIMD_MODE executionMode,
        EU_GEN6_SAMPLER_MESSAGE_TYPE messageType,
        uint samplerIndex,
        uint resourceIndex,
        bool endOfThread,
        bool FP16Input,
        bool FP16Return)
    {
        assert(resourceIndex < 256);
        assert(samplerIndex < 16);
        assert(messageType >= 0 && int(messageType) < 32);
        assert(executionMode >= 0 && int(executionMode) < 4);
        assert(responseLength < 9);
        assert(messageLength > 0 && messageLength < 16);

        // if endOfThread == true, responseLength needs to be 0
        assert(!endOfThread || responseLength == 0);

        SEUSamplerMessageDescriptorGen7 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.BindingTableIndex = resourceIndex;
        messageDescriptor.DW0.All.SamplerIndex = samplerIndex;
        messageDescriptor.DW0.All.MessageType = messageType;
        messageDescriptor.DW0.All.SIMDMode = executionMode;
        messageDescriptor.DW0.All.HeaderPresent = headerPresent;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.FP16Input = FP16Input;
        messageDescriptor.DW0.All.FP16Return = FP16Return;
        messageDescriptor.DW0.All.EndOfThread = endOfThread;

        return messageDescriptor.DW0.Value;
    }

    EU_DATA_PORT_ATOMIC_OPERATION_TYPE getHwAtomicOpEnum(AtomicOp op)
    {
        switch (op)
        {
        case EATOMIC_AND:
            return EU_DATA_PORT_ATOMIC_OPERATION_AND;
        case EATOMIC_DEC:
            return EU_DATA_PORT_ATOMIC_OPERATION_DEC;
        case EATOMIC_IADD:
            return EU_DATA_PORT_ATOMIC_OPERATION_ADD;
        case EATOMIC_IMAX:
            return EU_DATA_PORT_ATOMIC_OPERATION_IMAX;
        case EATOMIC_IMIN:
            return EU_DATA_PORT_ATOMIC_OPERATION_IMIN;
        case EATOMIC_INC:
            return EU_DATA_PORT_ATOMIC_OPERATION_INC;
        case EATOMIC_MAX:
            return EU_DATA_PORT_ATOMIC_OPERATION_IMAX;
        case EATOMIC_MIN:
            return EU_DATA_PORT_ATOMIC_OPERATION_IMIN;
        case EATOMIC_OR:
            return EU_DATA_PORT_ATOMIC_OPERATION_OR;
        case EATOMIC_SUB:
            return EU_DATA_PORT_ATOMIC_OPERATION_SUB;
        case EATOMIC_UMAX:
            return EU_DATA_PORT_ATOMIC_OPERATION_UMAX;
        case EATOMIC_UMIN:
            return EU_DATA_PORT_ATOMIC_OPERATION_UMIN;
        case EATOMIC_CMPXCHG:
            return EU_DATA_PORT_ATOMIC_OPERATION_CMPWR;
        case EATOMIC_XCHG:
            return EU_DATA_PORT_ATOMIC_OPERATION_MOV;
        case EATOMIC_XOR:
            return EU_DATA_PORT_ATOMIC_OPERATION_XOR;
        case EATOMIC_PREDEC:
            return EU_DATA_PORT_ATOMIC_OPERATION_PREDEC;
        case EATOMIC_FMIN:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_FMIN;
        case EATOMIC_FMAX:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_FMAX;
        case EATOMIC_FCMPWR:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_FCMPWR;
            // 64 Bit
        case EATOMIC_AND64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_AND;
        case EATOMIC_DEC64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_DEC;
        case EATOMIC_IADD64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_ADD;
        case EATOMIC_IMAX64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_IMAX;
        case EATOMIC_IMIN64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_IMIN;
        case EATOMIC_INC64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_INC;
        case EATOMIC_OR64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_OR;
        case EATOMIC_SUB64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_SUB;
        case EATOMIC_UMAX64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_UMAX;
        case EATOMIC_UMIN64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_UMIN;
        case EATOMIC_CMPXCHG64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_CMPWR;
        case EATOMIC_XCHG64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_MOV;
        case EATOMIC_XOR64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_XOR;
        case EATOMIC_PREDEC64:
            return EU_DATA_PORT_A64_ATOMIC_OPERATION_PREDEC;
        default:
            assert(0 && "Other atomic operations not implemented");
        }

        return EU_DATA_PORT_ATOMIC_OPERATION_AND;
    }

    uint encodeMessageDescriptorForAtomicUnaryOp(
        const unsigned int  messageLength,
        const unsigned int  responseLength,
        bool headerPresent,
        const uint message_type,
        const bool returnData,
        const SIMDMode simdMode,
        EU_DATA_PORT_ATOMIC_OPERATION_TYPE atomic_op_type,
        uint binding_table_index)
    {
        SEUDataPortMessageDescriptorGen8_0 messageDescriptor = { 0 };

        uint messageSpecificControl = 0;

        messageSpecificControl |= atomic_op_type;

        if ((message_type == EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION ||
            message_type == EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION) &&
            simdMode == SIMDMode::SIMD8)
        {
            messageSpecificControl |= (1 << 4);
        }

        messageDescriptor.DW0.All.BindingTableIndex = binding_table_index;
        messageDescriptor.DW0.All.EndOfThread = false;
        messageDescriptor.DW0.All.HeaderPresent = headerPresent;
        messageDescriptor.DW0.All.MessageLength = messageLength;

        switch (atomic_op_type)
        {

        case EU_DATA_PORT_A64_ATOMIC_OPERATION_FMIN:
        {
            messageDescriptor.DW0.All.MessageSpecificControl = 2;//FMIN
            messageDescriptor.DW0.All.MessageType = EU_GEN8_DATA_PORT_1_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT;
        }
        break;
        case EU_DATA_PORT_A64_ATOMIC_OPERATION_FMAX:
        {
            messageSpecificControl |= (1 << 13);
            messageDescriptor.DW0.All.MessageSpecificControl = 1;//FMAX
            messageDescriptor.DW0.All.MessageType = EU_GEN8_DATA_PORT_1_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT;
        }
        break;
        case EU_DATA_PORT_A64_ATOMIC_OPERATION_FCMPWR:
        {
            messageSpecificControl |= (1 << 13);
            messageDescriptor.DW0.All.MessageSpecificControl = 3;//FCMPWR
            messageDescriptor.DW0.All.MessageType = EU_GEN8_DATA_PORT_1_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT;
        }
        break;
        default:
        {
            messageSpecificControl |= (returnData << 5);
            messageDescriptor.DW0.All.MessageSpecificControl = messageSpecificControl;
            messageDescriptor.DW0.All.MessageType = message_type;
        }
        break;
        }

        messageDescriptor.DW0.All.ResponseLength = responseLength;

        return messageDescriptor.DW0.Value;
    }

    /// Currently this is a bad design since we are overloading both the functions doing the same thing.
    /// Discriminated Unions was an idea but this one needs more thought. Templates are terrible for
    /// dealing with multiple enum types and hence wasn't used.
    uint encodeMessageSpecificControlForReadWrite(
        const EU_DATA_PORT_READ_MESSAGE_TYPE messageType,
        const VISAChannelMask mask,
        const SIMDMode simdMode)
    {
        uint messageSpecificControl = 0;
        // We need only the lowest 4 bits
        messageSpecificControl |= (~mask & 0xF);

        if (messageType == EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ)
        {
            TODO("message encoding for message specific control has to be improved.");
            switch (simdMode)
            {
            case SIMDMode::SIMD8:
                // Hate magic numbers but for now we need a quick solution. Refer to Spec
                messageSpecificControl |= (2 << 4); // 12 is where the bits for SIMD mode start.. So 8 bits
                                                    // are in the structure already so we have to subtract 8
                                                    // for all our calculations
                break;
            case SIMDMode::SIMD16:
                messageSpecificControl |= (1 << 4);
                break;
            default:
                assert(0 && "Other SIMD modes are not allowed");
            }
        }

        return messageSpecificControl;
    }

    uint encodeMessageSpecificControlForReadWrite(
        const EU_DATA_PORT_WRITE_MESSAGE_TYPE messageType,
        const VISAChannelMask mask,
        const SIMDMode simdMode)
    {
        uint messageSpecificControl = 0;
        // We need only the lowest 4 bits
        messageSpecificControl |= (~mask & 0xF);

        if (messageType == EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE)
        {
            TODO("message encoding for message specific control has to be improved.");
            switch (simdMode)
            {
            case SIMDMode::SIMD8:
                // Hate magic numbers but for now we need a quick solution. Refer to Spec
                messageSpecificControl |= (2 << 4); // 12 is where the bits for SIMD mode start.. So 8 bits
                                                    // are in the structure already so we have to subtract 8
                                                    // for all our calculations
                break;
            case SIMDMode::SIMD16:
                messageSpecificControl |= (1 << 4);
                break;
            default:
                assert(0 && "Other SIMD modes are not allowed");
            }
        }
        else
        {
            assert(0 && "Other message types haven't been implemented yet.");
        }

        return messageSpecificControl;
    }

    uint DataPortRead(
        const uint messageLength,
        const uint responseLength,
        const bool headerPresent,
        const EU_DATA_PORT_READ_MESSAGE_TYPE messageType,
        const uint messageSpecificControl,
        const bool invalidateAfterReadEnableHint,
        const DATA_PORT_TARGET_CACHE targetCache,
        const uint bindingTableIndex)
    {
        static_assert(0 == DATA_PORT_TARGET_DATA_CACHE, "Table index order");
        static_assert(1 == DATA_PORT_TARGET_RENDER_CACHE, "Table index order");
        static_assert(2 == DATA_PORT_TARGET_SAMPLER_CACHE, "Table index order");
        static_assert(3 == DATA_PORT_TARGET_CONSTANT_CACHE, "Table index order");
        static_assert(4 == DATA_PORT_TARGET_DATA_CACHE_1, "Table index order");
        assert(0 <= targetCache && targetCache <= 4 && "Table index bounds check");
        static const uint cConvertMessageType[][NUM_EU_DATA_PORT_READ_MESSAGE_TYPES] =
        {
            // DATA_PORT_TARGET_DATA_CACHE
            {
                EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_BLOCK_READ,               // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ
                EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ,          // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_MEDIA_BLOCK_READ
                EU_GEN7_DATA_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_READ,           // EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ
                EU_GEN7_DATA_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ,     // EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ
                EU_GEN7_DATA_CACHE_MESSAGE_TYPE_BYTE_SCATTERED_READ,            // EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ
            },
            // DATA_PORT_TARGET_RENDER_CACHE
            {
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_MEDIA_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ
                EU_GEN9_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_READ,           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ
            },
            // DATA_PORT_TARGET_SAMPLER_CACHE
            {
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ
                EU_GEN7_SAMPLER_CACHE_MESSAGE_TYPE_MEDIA_BLOCK_READ,            // EU_DATA_PORT_READ_MESSAGE_TYPE_MEDIA_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ
                EU_GEN7_SAMPLER_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ,  // EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ
                EU_GEN8_SAMPLER_CACHE_MESSAGE_TYPE_SURFACE_INFO,                // EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ
            },
            // DATA_PORT_TARGET_CONSTANT_CACHE
            {
                EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_OWORD_BLOCK_READ,           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ
                EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ,      // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_MEDIA_BLOCK_READ
                EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_READ,       // EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ
                EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ, // EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ
                EU_GEN9_CONSTANT_CACHE_MESSAGE_SURFACE_INFO,                    // EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ
            },
            // DATA_PORT_TARGET_DATA_CACHE_1
            {
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ
                EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_MEDIA_BLOCK_READ,           // EU_DATA_PORT_READ_MESSAGE_TYPE_MEDIA_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ
                EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_SURFACE_READ,       // EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ
                EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_SURFACE_READ,         // EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ
                EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ,     // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ
                EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_SCATTERED_READ,           // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ
                EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_BLOCK_READ,               // EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ
                INVALID_MESSAGE_TYPE,                                           // EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ
            }
        };


        uint hwMessageType = cConvertMessageType[targetCache][messageType];
        SEUDataPortMessageDescriptorGen8_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.BindingTableIndex = bindingTableIndex;
        messageDescriptor.DW0.All.MessageSpecificControl = messageSpecificControl |
            (invalidateAfterReadEnableHint ? EU_DATA_PORT_INVALIDATE_AFTER_READ_ENABLE : 0);
        messageDescriptor.DW0.All.MessageType = hwMessageType;
        messageDescriptor.DW0.All.HeaderPresent = headerPresent;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.EndOfThread = false;

        return messageDescriptor.DW0.Value;
    }

    uint DataPortWrite(
        const uint   messageLength,
        const uint   responseLength,
        const bool   headerPresent,
        const bool   endOfThread,
        const EU_DATA_PORT_WRITE_MESSAGE_TYPE messageType,
        const uint   messageSpecificControl,
        const bool   invalidateAfterReadEnable,
        const uint   bindingTableIndex)
    {
        assert((messageLength > 0) && (messageLength < 16));
        assert(responseLength < 9);

        SEUDataPortMessageDescriptorGen8_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.BindingTableIndex = bindingTableIndex;
        messageDescriptor.DW0.All.MessageSpecificControl = messageSpecificControl;
        messageDescriptor.DW0.All.MessageType =
            cConvertDataPortWriteMessageType[messageType];
        messageDescriptor.DW0.All.HeaderPresent = headerPresent;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;

        return messageDescriptor.DW0.Value;
    }

    uint PixelDataPort(
        const bool   precisionSubType,
        const uint   messageLength,
        const uint   responseLength,
        const bool   headerPresent,
        const bool   perCoarse,
        const bool   perSample,
        const bool   lastRT,
        const bool   secondHalf,
        const EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL messageSubType,
        const uint   bindingTableIndex)
    {
        assert((messageLength > 0) && (messageLength < 16));
        assert(responseLength < 9);

        SEUPixelDataPortMessageDescriptorGen8_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.BindingTableIndex = bindingTableIndex;
        messageDescriptor.DW0.All.MessageSubType = messageSubType;
        messageDescriptor.DW0.All.Slot = secondHalf;
        messageDescriptor.DW0.All.LastRT = lastRT;
        messageDescriptor.DW0.All.PerSample = perSample;
        messageDescriptor.DW0.All.MessageType = EU_GEN9_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_WRITE;
        messageDescriptor.DW0.All.PerCoarse = perCoarse;
        messageDescriptor.DW0.All.HeaderPresent = headerPresent;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.PrecisionSubType = precisionSubType;

        return messageDescriptor.DW0.Value;
    }

    uint UrbMessage(
        const uint  messageLength,
        const uint  responseLength,
        const bool   endOfThread,
        const bool   perSlotOffset,
        const bool   channelMaskPresent,
        const uint  globalOffset,
        const EU_URB_OPCODE urbOpcode)
    {
        SEUURBMessageDescriptorGen8_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.Simd8.URBOpcode = urbOpcode;
        messageDescriptor.DW0.Simd8.GlobalOffset = globalOffset;
        messageDescriptor.DW0.Simd8.ChannelMaskPresent = channelMaskPresent;
        messageDescriptor.DW0.Simd8.PerSlotOffset = perSlotOffset;
        messageDescriptor.DW0.Simd8.ResponseLength = responseLength;
        messageDescriptor.DW0.Simd8.MessageLength = messageLength;
        messageDescriptor.DW0.Simd8.HeaderPresent = true;

        return messageDescriptor.DW0.Value;
    }

    uint CBTILayout::GetSystemThreadBindingTableIndex(void) const
    {
        return m_pLayout->systemThreadIdx;
    }

    uint CBTILayout::GetBindingTableEntryCount(void) const
    {
        return m_pLayout->maxBTsize;
    }

    uint CBTILayout::GetTextureIndex(uint index) const
    {
        uint idx = m_pLayout->minResourceIdx + index;
        return idx;
    }

    uint CBTILayout::GetNullSurfaceIdx() const
    {
        return m_pLayout->NULLSurfaceIdx;
    }

    uint CBTILayout::GetUavIndex(uint index) const
    {
        uint idx = m_pLayout->minUAVIdx + index;
        return idx;
    }

    uint CBTILayout::GetRenderTargetIndex(uint index) const
    {
        uint idx = m_pLayout->minColorBufferIdx + index;

        assert(m_ColorBufferMappings.size() == 0 || idx < m_ColorBufferMappings.size());
        if (idx < m_ColorBufferMappings.size())
        {
            idx = m_ColorBufferMappings[idx];
        }

        return idx;
    }

    uint CBTILayout::GetConstantBufferIndex(uint index) const
    {
        uint idx = m_pLayout->minConstantBufferIdx + index;
        return idx;
    }

    uint CBTILayout::GetTGSMIndex() const
    {
        return m_pLayout->TGSMIdx;
    }

    uint CBTILayout::GetScratchSurfaceBindingTableIndex() const
    {
        assert(m_pLayout != NULL);
        return SCRATCH_SPACE_BTI;
    }

    uint CBTILayout::GetStatelessBindingTableIndex() const
    {
        assert(m_pLayout != NULL);
        return STATELESS_BTI;
    }

    uint CBTILayout::GetImmediateConstantBufferOffset() const
    {
        assert(m_pLayout != NULL);
        return m_pLayout->immediateConstantBufferOffset;
    }

    uint CBTILayout::GetDrawIndirectBufferIndex() const
    {
        assert(m_pLayout != NULL);
        return m_pLayout->indirectBufferOffset;
    }

    USC::SShaderStageBTLayout* COCLBTILayout::getModifiableLayout()
    {
        return const_cast<USC::SShaderStageBTLayout*>(m_pLayout);
    }

    EU_PIXEL_INTERPOLATOR_SIMD_MODE pixelInterpolatorSimDMode(SIMDMode simd)
    {
        if (simd == SIMDMode::SIMD8)
        {
            return EU_PI_MESSAGE_SIMD8;
        }
        else if (simd == SIMDMode::SIMD16)
        {
            return EU_PI_MESSAGE_SIMD16;
        }
        assert(0);
        return EU_PI_MESSAGE_SIMD8;
    }

    unsigned int PixelInterpolator(
        unsigned int messageLength,
        unsigned int responseLength,
        unsigned int pass,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode,
        const unsigned int sampleindex)
    {
        assert(messageType >= 0 && int(messageType) < 4);
        assert(executionMode >= 0 && int(executionMode) < 4);

        SEUPixelInterpolatorSampleIndexMessageDescriptorGen7_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.SampleIndex = sampleindex;
        messageDescriptor.DW0.All.MessageType = messageType;
        messageDescriptor.DW0.All.InterpolationMode = interpolationMode;
        messageDescriptor.DW0.All.SIMDMode = executionMode;
        messageDescriptor.DW0.All.HeaderPresent = false;
        messageDescriptor.DW0.All.SlotGroupSelect = pass;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.EndOfThread = false;

        return messageDescriptor.DW0.Value;
    }

    unsigned int PixelInterpolator(
        unsigned int messageLength,
        unsigned int responseLength,
        unsigned int pass,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode,
        unsigned int perMessageXOffset,
        unsigned int perMessageYOffset)
    {
        assert(messageType >= 0 && int(messageType) < 4);
        assert(executionMode >= 0 && int(executionMode) < 4);

        SEUPixelInterpolatorOffsetMessageDescriptorGen7_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.MessageType = messageType;
        messageDescriptor.DW0.All.PerMessageXOffset = perMessageXOffset;
        messageDescriptor.DW0.All.PerMessageYOffset = perMessageYOffset;
        messageDescriptor.DW0.All.InterpolationMode = interpolationMode;
        messageDescriptor.DW0.All.SlotGroupSelect = pass;
        messageDescriptor.DW0.All.SIMDMode = executionMode;
        messageDescriptor.DW0.All.HeaderPresent = false;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.EndOfThread = false;

        return messageDescriptor.DW0.Value;
    }

    unsigned int PixelInterpolator(
        const DWORD messageLength,
        const DWORD responseLength,
        const DWORD pass,
        bool  IsCoarse,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode)
    {
        SEUPixelInterpolatorMessageDescriptorGen7_0 messageDescriptor;
        memset(&messageDescriptor, 0, sizeof(messageDescriptor));

        messageDescriptor.DW0.All.MessageType = messageType;
        messageDescriptor.DW0.All.InterpolationMode = interpolationMode;
        messageDescriptor.DW0.All.SlotGroupSelect = pass;
        messageDescriptor.DW0.All.SIMDMode = executionMode;
        messageDescriptor.DW0.All.HeaderPresent = false;
        messageDescriptor.DW0.All.ResponseLength = responseLength;
        messageDescriptor.DW0.All.MessageLength = messageLength;
        messageDescriptor.DW0.All.EndOfThread = false;
        messageDescriptor.DW0.All.ShadingRate = IsCoarse ? 1 : 0;

        return messageDescriptor.DW0.Value;
    }

    uint32_t VMEDescriptor(
        COMMON_ISA_VME_STREAM_MODE streamMode,
        const uint32_t bti,
        const uint32_t msgType,
        const uint32_t regs2snd,
        const uint32_t regs2rcv
    )
    {
        VMEMessageDescriptorGen8_0 messageDescriptor = { 0 };

        messageDescriptor.DW0.All.BindingTableIndex = bti;
        messageDescriptor.DW0.All.MessageType = msgType;
        messageDescriptor.DW0.All.StreamOutEnable = (streamMode == VME_STREAM_IN_OUT) || (streamMode == VME_STREAM_OUT);
        messageDescriptor.DW0.All.StreamInEnable = (streamMode == VME_STREAM_IN_OUT) || (streamMode == VME_STREAM_IN);
        messageDescriptor.DW0.All.HeaderPresent = true;
        messageDescriptor.DW0.All.MessageLength = regs2snd;
        messageDescriptor.DW0.All.ResponseLength = regs2rcv;

        return messageDescriptor.DW0.Value;
    }


    unsigned int PIPullPixelPayload(
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        DWORD responseLength,
        DWORD messageLenght,
        bool inputCoverage,
        bool linearCentroidBary,
        bool linearCenterBary,
        bool perspectiveCentroid,
        bool perspectiveCenter,
        bool OutputCoverageMask)
    {
        union CoarseToPixelMappingMessageDescriptor
        {
            struct
            {
                unsigned int InputConverage : BITFIELD_BIT(0);
                unsigned int LinearCentroidBary : BITFIELD_BIT(1);
                unsigned int LinearCenterBary : BITFIELD_BIT(2);
                unsigned int PerspectiveCentroid : BITFIELD_BIT(3);
                unsigned int PerspectiveCenter : BITFIELD_BIT(4);
                unsigned int OutputCoverageMask : BITFIELD_BIT(5);
                unsigned int                        : BITFIELD_RANGE(6, 7);
                unsigned int PixelShaderPhase : BITFIELD_RANGE(8, 11);
                unsigned int MessageType : BITFIELD_RANGE(12, 13);
                unsigned int InterpolationMode : BITFIELD_BIT(14);
                unsigned int ShadingRate : BITFIELD_BIT(15);
                unsigned int SimdMode : BITFIELD_BIT(16);
                unsigned int                   : BITFIELD_RANGE(17, 18);
                unsigned int HeaderPresent : BITFIELD_BIT(19);
                unsigned int ResponseLength : BITFIELD_RANGE(20, 24);
                unsigned int MessageLength : BITFIELD_RANGE(25, 28);
                unsigned int                   : BITFIELD_RANGE(29, 30);
                unsigned int EndOfThread : BITFIELD_BIT(31);
            } All;

            unsigned int Value;
        };

        CoarseToPixelMappingMessageDescriptor messageDesc;
        messageDesc.Value = 0;
        messageDesc.All.ShadingRate = 1; // Coarse rate
        messageDesc.All.SimdMode = executionMode;
        messageDesc.All.InputConverage = inputCoverage;
        messageDesc.All.LinearCentroidBary = linearCentroidBary;
        messageDesc.All.LinearCenterBary = linearCenterBary;
        messageDesc.All.PerspectiveCentroid = perspectiveCentroid;
        messageDesc.All.PerspectiveCenter = perspectiveCenter;
        messageDesc.All.OutputCoverageMask = OutputCoverageMask;
        messageDesc.All.MessageLength = messageLenght;
        messageDesc.All.ResponseLength = responseLength;

        return messageDesc.Value;
    }



} // namespace IGC
