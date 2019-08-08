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

// enums for various fields in message-specific descriptors
typedef enum {
    DC_OWORD_BLOCK_READ = 0,
    DC_ALIGNED_OWORD_BLOCK_READ = 1,
    DC_DWORD_SCATTERED_READ = 3,
    DC_BYTE_SCATTERED_READ = 4,
    DC_UNTYPED_ATOMIC = 6,
    DC_MEMORY_FENCE = 7,
    DC_OWORD_BLOCK_WRITE = 8,
    DC_DWORD_SCATTERED_WRITE = 11,
    DC_BYTE_SCATTERED_WRITE = 12,
} DATA_CACHE0_MESSAGES;

typedef enum {
    DC1_UNTYPED_SURFACE_READ = 0x1,
    DC1_UNTYPED_ATOMIC = 0x2,
    DC1_MEDIA_BLOCK_READ = 0x4,
    DC1_TYPED_SURFACE_READ = 0x5,
    DC1_TYPED_ATOMIC = 0x6,
    DC1_UNTYPED_SURFACE_WRITE = 0x9,
    DC1_MEDIA_BLOCK_WRITE = 0xA,
    DC1_TYPED_SURFACE_WRITE = 0xD,
    DC1_A64_SCATTERED_READ  = 0x10,
    DC1_A64_UNTYPED_SURFACE_READ = 0x11,
    DC1_A64_ATOMIC = 0x12,
    DC1_A64_BLOCK_READ = 0x14,
    DC1_A64_BLOCK_WRITE = 0x15,
    DC1_A64_UNTYPED_SURFACE_WRITE = 0x19,
    DC1_A64_SCATTERED_WRITE = 0x1A,
    DC1_UNTYPED_FLOAT_ATOMIC = 0x1B,
    DC1_A64_UNTYPED_FLOAT_ATOMIC = 0x1D,
} DATA_CACHE1_MESSAGES;

typedef enum 
{
    A64_BLOCK_MSG_OWORD_RW = 0x0,
    A64_BLOCK_MSG_OWORD_UNALIGNED_READ = 0x1
} A64_BLOCK_MSG_SUBTYPE;

typedef enum {
    DC2_UNTYPED_SURFACE_READ        = (0x01) << 1, // MT2R_US
    DC2_A64_SCATTERED_READ          = (0x02) << 1, // MT2R_A64_SB
    DC2_A64_UNTYPED_SURFACE_READ    = (0x03) << 1, // MT2R_A64_US
    DC2_BYTE_SCATTERED_READ         = (0x04) << 1, // MT2R_BS
    DC2_UNTYPED_SURFACE_WRITE       = (0x09) << 1, // MT2W_US
    DC2_A64_UNTYPED_SURFACE_WRITE   = (0x0A) << 1, // MT2W_A64_US
    DC2_A64_SCATTERED_WRITE         = (0x0B) << 1, // MT2W_A64_SB
    DC2_BYTE_SCATTERED_WRITE        = (0x0C) << 1  // MT2W_BS
} DATA_CACHE2_MESSAGES;

typedef enum {
    DC1_HWORD_BLOCK_READ              = 0x0,
    DC1_HWORD_ALIGNED_BLOCK_READ      = 0x1,
    DC1_HWORD_BLOCK_WRITE             = 0x8,
    DC1_HWORD_ALIGNED_BLOCK_WRITE     = 0x9
} HWORD_DATA_CACHE1_MESSAGES;

typedef enum 
{
    URB_WRITE_HWORD = 0,
    URB_WRITE_OWORD = 1,
    URB_READ_HWORD = 2,
    URB_READ_OWORD = 3,
    URB_ATOMIC_MOV = 4,
    URB_ATOMIC_INC = 5,
    URB_ATOMIC_ADD = 6,
    URB_SIMD8_WRITE = 7,
    URB_SIMD8_READ = 8
} URB_MESSAGES;

// SIMD Mode 2 Message Descriptor Control Field
typedef enum {
    MDC_SM2_SIMD8  = 0,
    MDC_SM2_SIMD16 = 1
} MDC_SM2;

// Reversed SIMD Mode 2 Message Descriptor Control Field
typedef enum {
    MDC_SM2R_SIMD8  = 1,
    MDC_SM2R_SIMD16 = 0
} MDC_SM2R;

// SIMD Mode 3 Message Descriptor Control Field
typedef enum {
    MDC_SM3_SIMD4x2 = 0,
    MDC_SM3_SIMD16  = 1,
    MDC_SM3_SIMD8   = 2
} MDC_SM3;

typedef enum {
    MDC_GW_OPEN_GATEWAY         = 0,
    MDC_GW_CLOSE_GATEWAY        = 1,
    MDC_GW_FORWARG_MSG          = 2,
    MDC_GW_GET_TIMESTAMP        = 3,
    MDC_GW_BARRIER_MSG          = 4,
    MDC_GW_UPDATE_GATEWAY_STATE = 5
} MDC_GATEWAY_SUBFUNC;

typedef enum {
    MDC_SG3_SG4x2   = 0,
    MDC_SG3_SG8L    = 1,
    MDC_SG3_SG8U    = 2,
} MDC_SG3;

enum SamplerSIMDMode
{
    SIMD8 = 1,
    SIMD16 = 2,
    SIMD32 = 3
};

#define A64_BLOCK_MSG_SUBTYPE_OFFSET 11
