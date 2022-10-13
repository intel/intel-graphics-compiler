/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#if !defined(__IGFXHWEUISA_H__)
#define __IGFXHWEUISA_H__

__CODEGEN_FILE_DIRECTIVES_OPEN
__CODEGEN_NAMESPACE_OPEN

typedef enum tagEU_OPCODE {
  EU_OPCODE_ILLEGAL = 0x0,
  EU_OPCODE_MOV = 0x1,
  EU_OPCODE_SEL = 0x2,
  EU_OPCODE_MOVI = 0x3,
  EU_OPCODE_NOT = 0x4,
  EU_OPCODE_AND = 0x5,
  EU_OPCODE_OR = 0x6,
  EU_OPCODE_XOR = 0x7,
  EU_OPCODE_SHR = 0x8,
  EU_OPCODE_SHL = 0x9,
  EU_OPCODE_SMOV = 0xA,
  EU_OPCODE_ASR = 0xC,
  EU_OPCODE_ROR = 0xE, // ICL+
  EU_OPCODE_ROL = 0xF, // ICL+
  EU_OPCODE_CMP = 0x10,
  EU_OPCODE_CMPN = 0x11,
  EU_OPCODE_CSEL = 0x12,
  EU_OPCODE_BFREV = 0x17,
  EU_OPCODE_BFE = 0x18,
  EU_OPCODE_BFI1 = 0x19,
  EU_OPCODE_BFI2 = 0x1A,
  EU_OPCODE_JMPI = 0x20,
  EU_OPCODE_BRD = 0x21,
  EU_OPCODE_IF = 0x22,
  EU_OPCODE_BRC = 0x23,
  EU_OPCODE_ELSE = 0x24,
  EU_OPCODE_ENDIF = 0x25,
  EU_OPCODE_WHILE = 0x27,
  EU_OPCODE_BREAK = 0x28,
  EU_OPCODE_CONT = 0x29,
  EU_OPCODE_HALT = 0x2A,
  EU_OPCODE_CALLA = 0x2B,
  EU_OPCODE_CALL = 0x2C,
  EU_OPCODE_RET = 0x2D,
  EU_OPCODE_GOTO = 0x2E,
  EU_OPCODE_JOIN = 0x2F,
  EU_OPCODE_WAIT = 0x30,
  EU_OPCODE_SEND = 0x31,
  EU_OPCODE_SENDC = 0x32,
  EU_OPCODE_SENDS = 0x33,
  EU_OPCODE_SENDSC = 0x34,
  EU_OPCODE_MATH = 0x38,
  EU_OPCODE_ADD = 0x40,
  EU_OPCODE_MUL = 0x41,
  EU_OPCODE_AVG = 0x42,
  EU_OPCODE_FRC = 0x43,
  EU_OPCODE_RNDU = 0x44,
  EU_OPCODE_RNDD = 0x45,
  EU_OPCODE_RNDE = 0x46,
  EU_OPCODE_RNDZ = 0x47,
  EU_OPCODE_MAC = 0x48,
  EU_OPCODE_MACH = 0x49,
  EU_OPCODE_LZD = 0x4A,
  EU_OPCODE_FBH = 0x4B,
  EU_OPCODE_FBL = 0x4C,
  EU_OPCODE_CBIT = 0x4D,
  EU_OPCODE_ADDC = 0x4E,
  EU_OPCODE_SUBB = 0x4F,
  EU_OPCODE_SAD2 = 0x50,
  EU_OPCODE_SADA2 = 0x51,
  EU_OPCODE_DP4 = 0x54,
  EU_OPCODE_DPH = 0x55,
  EU_OPCODE_DP3 = 0x56,
  EU_OPCODE_DP2 = 0x57,
  EU_OPCODE_DP4A = 0x58,
  EU_OPCODE_LINE = 0x59,
  EU_OPCODE_PLN = 0x5A,
  EU_OPCODE_MAD = 0x5B,
  EU_OPCODE_LRP = 0x5C,
  EU_OPCODE_MADM = 0x5D,
  EU_OPCODE_NOP = 0x7E,
} EU_OPCODE;

typedef enum tagREGFILE {
  REGFILE_ARF = 0x0, // Architecture Register File
  REGFILE_GRF =
      0x1, // General Register File - allowed for any source or destination
  REGFILE_IMM = 0x3, // Immediate operand
} REGFILE;

/*****************************************************************************\
Destination Type
Numeric data type of the destination operand dst. The bits of the destination
operand are interpreted as the identified numeric data type, rather than coerced
into a type implied by the operator. For a send or sendc instruction, this field
applies to CurrDst, the current destination operand. Three source instructions
use a 3-bit encoding that allows fewer data types.
\*****************************************************************************/
typedef enum tagDSTTYPE {
  DSTTYPE_UD = 0x0, // Unsigned Doubleword integer
  DSTTYPE_D = 0x1,  // signed Doubleword integer
  DSTTYPE_UW = 0x2, // Unsigned Word integer
  DSTTYPE_W = 0x3,  // signed Word integer
  DSTTYPE_UB = 0x4, // Unsigned Byte integer
  DSTTYPE_B = 0x5,  // signed Byte integer
  DSTTYPE_DF = 0x6, // Double precision Float (64-bit)
  DSTTYPE_F = 0x7,  // single precision Float (32-bit)
  DSTTYPE_UQ = 0x8, // Unsigned Quadword integer
  DSTTYPE_Q = 0x9,  // signed Quadword integer
  DSTTYPE_HF = 0xA, // Half Float (16-bit)
} DSTTYPE;

/*****************************************************************************\
Specifies the numeric data type of a source operand. In a two-source
instruction, each source operand has its own source type field. In a
three-source instruction, one source type is used for all three source operands.
The bits of a source operand are interpreted as the identified numeric data
type, rather than coerced into a type implied by the operator. Depending on the
RegFile field for the source, this field uses one of two encodings. For a
non-immediate source (from a register file), use the Source Register Type
Encoding, which is identical to the Destination Type encoding. For an immediate
source, use the Source Immediate Type Encoding, which does not support signed or
unsigned byte immediate values and does support the three packed vector types,
V, UV, and VF. Note that three-source instructions do not support immediate
operands, that only the second source (src1) of a two-source instruction can be
immediate, and that 64-bit immediate values (DF, Q, or UQ) can only be used with
one-source instructions. In a two-source instruction with a V (Packed Signed
Half-Byte Integer Vector) or UV (Packed Unsigned Half-Byte Integer Vector)
immediate operand, the other source operand must have a type compatible with
packed word execution mode, one of B, UB, W, or UW. Note that DF (Double Float)
and HF (Half Float) have different encodings in the Source Regster Type Encoding
and the Source Immediate Type Encoding. The Source Register Type Encoding and
Source Immediate Type Encoding lists apply to instructions with one or two
source operands.
\*****************************************************************************/
typedef enum tagSRCTYPE {
  SRCTYPE_UD = 0x0, // Unsigned Doubleword
  SRCTYPE_D = 0x1,  // signed Doubleword
  SRCTYPE_UW = 0x2, // Unsigned Word integer
  SRCTYPE_W = 0x3,  // signed Word integer
  SRCTYPE_UB = 0x4, // unsigned Byte integer
  SRCTYPE_B = 0x5,  // signed Byte integer
  SRCTYPE_DF = 0x6, // Double precision Float (64-bit)
  SRCTYPE_F = 0x7,  // single precision Float (32-bit)
  SRCTYPE_UQ = 0x8, // Unsigned Quadword integer
  SRCTYPE_Q = 0x9,  // signed Quadword integer
  SRCTYPE_HF = 0xA, // Half Float (16-bit)
} SRCTYPE;

/*****************************************************************************\
Specifies the numeric data type of a source operand. In a two-source
instruction, each source operand has its own source type field. In a
three-source instruction, one source type is used for all three source operands.
The bits of a source operand are interpreted as the identified numeric data
type, rather than coerced into a type implied by the operator. Depending on the
RegFile field for the source, this field uses one of two encodings. For a
non-immediate source (from a register file), use the Source Register Type
Encoding, which is identical to the Destination Type encoding. For an immediate
source, use the Source Immediate Type Encoding, which does not support signed or
unsigned byte immediate values and does support the three packed vector types,
V, UV, and VF. Note that three-source instructions do not support immediate
operands, that only the second source (src1) of a two-source instruction can be
immediate, and that 64-bit immediate values (DF, Q, or UQ) can only be used with
one-source instructions. In a two-source instruction with a V (Packed Signed
Half-Byte Integer Vector) or UV (Packed Unsigned Half-Byte Integer Vector)
immediate operand, the other source operand must have a type compatible with
packed word execution mode, one of B, UB, W, or UW. Note that DF (Double Float)
and HF (Half Float) have different encodings in the Source Regster Type Encoding
and the Source Immediate Type Encoding. The Source Register Type Encoding and
Source Immediate Type Encoding lists apply to instructions with one or two
source operands.
\*****************************************************************************/
typedef enum tagSRCIMMTYPE {
  SRCIMMTYPE_UD = 0x0, // Unsigned Doubleword
  SRCIMMTYPE_D = 0x1,  // signed Doubleword
  SRCIMMTYPE_UW = 0x2, // Unsigned Word integer
  SRCIMMTYPE_W = 0x3,  // signed Word integer
  SRCIMMTYPE_UV = 0x4, // Packed Unsigned Half-Byte Integer Vector, 8 x 4-Bit
                       // Unsigned Integer.
  SRCIMMTYPE_VF = 0x5, // Packed Restricted Float Vector, 4 x 8-Bit Restricted
                       // Precision Floating-Point Number
  SRCIMMTYPE_V =
      0x6, // Packed Signed Half-Byte Integer Vector, 8 x 4-Bit Signed Integer
  SRCIMMTYPE_F = 0x7,  // single precision Float (32-bit)
  SRCIMMTYPE_UQ = 0x8, // Unsigned Quadword integer
  SRCIMMTYPE_Q = 0x9,  // signed Quadword integer
  SRCIMMTYPE_DF = 0xA, // Double precision Float (64-bit)
  SRCIMMTYPE_HF = 0xB, // Half Float (16-bit)
} SRCIMMTYPE;

/*****************************************************************************\
Horizontal Stride
This field provides the distance in unit of data elements between two adjacent
data elements within a row (horizontal) in the register region for the operand.
This field applies to both destination and source operands.
This field is not present for an immediate source operand.

A horizontal stride of 0 is used for a row that is one-element wide, useful when
an instruction repeats a column value or repeats a scalar value. For example,
adding a single column to every column in a 2D array or adding a scalar to every
element in a 2D array uses HorzStride of 0. A horizontal stride of 1 indicates
that elements are adjacent within a row. References to HorzStride in this volume
normally reference the value not the encoding, so there are references to
HorzStride of 4, which is encoded as 11b.
\*****************************************************************************/
typedef enum tagHORZSTRIDE {
  HORZSTRIDE_0_ELEMENTS = 0x0,
  HORZSTRIDE_1_ELEMENTS = 0x1,
  HORZSTRIDE_2_ELEMENTS = 0x2,
  HORZSTRIDE_4_ELEMENTS = 0x3,
} HORZSTRIDE;

/*****************************************************************************\
Addressing Mode
This field determines the addressing method of the operand. Normally the
destination operand and each source operand each have a distinct addressing mode
field. When it is cleared, the register address of the operand is directly
provided by bits in the instruction word. It is called a direct register
addressing mode. When it is set, the register address of the operand is computed
based on the address register value and an address immediate field in the
instruction word. This is referred to as a register-indirect register addressing
mode. This field applies to the destination operand and the first source
operand, src0. Support for src1 is device dependent. See Table XX (Indirect
source addressing support available in device hardware) in ISA Execution
Environment for details.

Programming Notes:
Instructions with 3 source operands use Direct Addressing.
\*****************************************************************************/
typedef enum tagADDRMODE {
  ADDRMODE_DIRECT = 0x0,   // 'Direct' register addressing
  ADDRMODE_INDIRECT = 0x1, // 'Register-Indirect' (or in short 'Indirect').
                           // Register-indirect register addressing
} ADDRMODE;

/*****************************************************************************\
Source Modifier
This field specifies the numeric modification of a source operand. The value of
each data element of a source operand can optionally have its absolute value
taken and/or its sign inverted prior to delivery to the execution pipe. The
absolute value is prior to negate such that a guaranteed negative value can be
produced. This field only applies to source operand. It does not apply to
destination. This field is not present for an immediate source operand.

When used with logic instructions (and, not, or, xor), this field indicates
whether the source bits are inverted (bitwise NOT) before delivery to the
execution pipe, regardless of the source type.
\*****************************************************************************/
typedef enum tagSRCMOD {
  SRCMOD_NO_MODIFICATION = 0x0,
  SRCMOD_ABS = 0x1, // Absolute value,Logic instructions: No modification (This
                    // encoding cannot be selected in the assembler syntax)
  SRCMOD_NEGATE =
      0x2, // Negate,Logic instructions: Bitwise NOT, inverting the source bits
  SRCMOD_NEGATE_OF_ABS =
      0x3, // Negate of the absolute (forced negative value),Logic instructions:
           // No modification (This encoding cannot be selected in the assembler
           // syntax)
} SRCMOD;

/*****************************************************************************\
This field specifies the number of elements in the horizontal dimension of the
region for a source operand. This field cannot exceed the ExecSize field of the
instruction. This field only applies to source operand. It does not apply to
destination. This field is not present for an immediate source operand.

Programming Notes:
Note that with ExecSize of 32, because the maximum Width is 16, there are at
least two rows in a source region.
\*****************************************************************************/
typedef enum tagWIDTH {
  WIDTH_1_ELEMENTS = 0x0,
  WIDTH_2_ELEMENTS = 0x1,
  WIDTH_4_ELEMENTS = 0x2,
  WIDTH_8_ELEMENTS = 0x3,
  WIDTH_16_ELEMENTS = 0x4,
} WIDTH;

/*****************************************************************************\
Vertical Stride
The field provides the vertical stride of the register region in unit of data
elements for an operand. Encoding of this field provides values of 0 or powers
of 2, ranging from 1 to 32 elements. Larger values are not supported due to the
restriction that a source operand must reside within two adjacent 256-bit
registers (64 bytes total). Special encoding 1111b (0xF) is only valid when the
operand is in register-indirect addressing mode (AddrMode = 1). If this field is
set to 0xF, one or more sub-registers of the address registers may be used to
compute the addresses. Each address sub-register provides the origin for a row
of data element. The number of address sub-registers used is determined by the
division of ExecSize of the instruction by the Width fields of the operand. This
field only applies to source operand. It does not apply to destination. This
field is not present for an immediate source operand.

Programming Notes:
Note 1: Vertical Stride larger than 32 is not allowed due to the restriction
that a source operand must reside within two adjacent 256-bit registers (64
bytes total).

Note 2: In  Align16 access mode, as encoding 0xF is reserved, only single-index
indirect addressing is supported.

Note 3: If indirect address is supported for src1, encoding 0xF is reserved for
src1 and only single-index indirect addressing is supported.

Note 4: Encoding 0010 applies for QWord-size operands.
\*****************************************************************************/
typedef enum tagVERTSTRIDE {
  VERTSTRIDE_0_ELEMENTS = 0x0,
  VERTSTRIDE_1_ELEMENT = 0x1,
  VERTSTRIDE_2_ELEMENTS = 0x2,
  VERTSTRIDE_4_ELEMENTS = 0x3,
  VERTSTRIDE_8_ELEMENTS = 0x4,
  VERTSTRIDE_16_ELEMENTS = 0x5,
  VERTSTRIDE_32_ELEMENTS = 0x6,
  VERTSTRIDE_VXH_OR_VX1_MODE = 0xF,
} VERTSTRIDE;

/*****************************************************************************\
Replicate Control
This field is only present in three-source instructions, for each of the three
source operands. It controls replication of the starting channel to all channels
in the execution size. This is applicable to 32b datatypes. 16b and 64b
datatypes cannot use the replicate control.
\*****************************************************************************/
typedef enum tagREPCTRL {
  REPCTRL_NO_REPLICATION = 0x0,
  REPCTRL_REPLICATE_ACROSS_ALL_CHANNELS = 0x1,
} REPCTRL;

/*****************************************************************************\
Destination Dependency Control
This field selectively disables destination dependency check and clear for this
instruction. When it is set to 00, normal destination dependency control is
performed for the instruction and hardware checks for destination hazards to
ensure data integrity. Specifically, destination register dependency check is
conducted before the instruction is made ready for execution. After the
instruction is executed, the destination register scoreboard will be cleared
when the destination operands retire. When bit 10 is set (NoDDClr), the
destination register scoreboard will NOT be cleared when the destination
operands retire.  When bit 11 is set (NoDDChk), hardware does not check for
destination register dependency before the instruction is made ready for
execution.  NoDDClr and NoDDChk are not mutual exclusive. When this field is not
all-zero, hardware does not protect against destination hazards for the
instruction.  This is typically used to assemble data in a fine grained fashion
(e.g. matrix-vector compute with dot-product instructions), where the data
integrity is guaranteed by software based on the intended usage of instruction
sequences.
\*****************************************************************************/
typedef enum tagDEPCTRL {
  DEPCTRL_NONE = 0x0,    // Destination dependency checked and cleared (normal)
  DEPCTRL_NODDCLR = 0x1, // Destination dependency checked but not cleared
  DEPCTRL_NODDCHK = 0x2, // Destination dependency not checked but cleared
  DEPCTRL_NODDCLR_NODDCHK =
      0x3, // Destination dependency not checked and not cleared
} DEPCTRL;

/*****************************************************************************\
Thread Control
This field provides explicit control for thread switching.
\*****************************************************************************/
typedef enum tagTHREADCTRL {
  THREADCTRL_NORMAL =
      0x0, // Up to the GEN execution units to manage thread switching. This is
           // the normal (and unnamed) mode. In this mode, for example, if the
           // current instruction cannot proceed due to operand dependencies,
           // the EU switches to the next available thread to fill the compute
           // pipe. In another example, if the current instruction is ready to
           // go, however, there is another thread with higher priority that
           // also has an instruction ready, the EU switches to that thread.,
           // Execution may or may not be preempted by another thread following
           // this instruction.
  THREADCTRL_ATOMIC =
      0x1, // Prevent any thread switch immediately following this instruction.
           // Always execute the next instruction (which may not be next
           // sequentially if the current instruction branches).,            The
           // next instruction gets highest priority in the thread arbitration
           // for the execution pipelines.
  THREADCTRL_SWITCH =
      0x2, // A forced thread switch occurs after the current instruction is
           // executed and before the next instruction. In addition, a long
           // delay (longer than the execution pipe latency) is introduced for
           // the current thread. Particularly, the instruction queue of the
           // current thread is flushed after the current instruction is
           // dispatched for execution. ,            Switch is designed
           // primarily as a safety feature in case there are race conditions
           // for certain instructions.,            Force a switch to another
           // thread after this instruction and before the next instruction.
  THREADCTRL_NOPREEMPT = 0x3, // A thread wont stop on this instruction even
                              // after receiving a pre-emption request.
} THREADCTRL;

/*****************************************************************************\
Execution Size
This field determines the number of channels operating in parallel for this
instruction. The size cannot exceed the maximum number of channels allowed for
the given data type.

Programming Notes:
Restriction : An operand's Width must be less-than-or-equal to ExecSize
\*****************************************************************************/
typedef enum tagEXECSIZE {
  EXECSIZE_1_CHANNEL_SCALAR_OPERATION = 0x0,
  EXECSIZE_2_CHANNELS = 0x1,
  EXECSIZE_4_CHANNELS = 0x2,
  EXECSIZE_8_CHANNELS = 0x3,
  EXECSIZE_16_CHANNELS = 0x4,
  EXECSIZE_32_CHANNELS = 0x5,
} EXECSIZE;

typedef enum tagPREDCTRL {
  PREDCTRL_NO_PREDICATION_NORMAL = 0x0,
  PREDCTRL_SEQUENTIAL_FLAG_CHANNEL_MAPPING = 0x1,
  PREDCTRL_ANYV_ANY_FROM_F0_0_F0_1_ON_THE_SAME_CHANNEL = 0x2,
  PREDCTRL_REPLICATION_SWIZZLE_X = 0x2,
  PREDCTRL_ALLV_ALL_OF_F0_0_F0_1_ON_THE_SAME_CHANNEL = 0x3,
  PREDCTRL_REPLICATION_SWIZZLE_Y = 0x3,
  PREDCTRL_ANY2H_ANY_IN_GROUP_OF_2_CHANNELS = 0x4,
  PREDCTRL_REPLICATION_SWIZZLE_Z = 0x4,
  PREDCTRL_ALL2H_ALL_IN_GROUP_OF_2_CHANNELS = 0x5,
  PREDCTRL_REPLICATION_SWIZZLE_W = 0x5,
  PREDCTRL_ANY4H = 0x6,
  PREDCTRL_ALL4H = 0x7,
  PREDCTRL_ANY8H_ANY_IN_GROUP_OF_8_CHANNELS = 0x8,
  PREDCTRL_ALL8H_ALL_IN_GROUP_OF_8_CHANNELS = 0x9,
  PREDCTRL_ANY16H_ANY_IN_GROUP_OF_16_CHANNELS = 0xA,
  PREDCTRL_ALL16H_ALL_IN_GROUP_OF_16_CHANNELS = 0xB,
  PREDCTRL_ANY32H_ANY_IN_GROUP_OF_32_CHANNELS = 0xC,
  PREDCTRL_ALL32H_ALL_IN_GROUP_OF_32_CHANNELS = 0xD,
} PREDCTRL;

/*****************************************************************************\
Conditional Modifier
This field sets the flag register based on the internal conditional signals
output from the execution pipe such as sign, zero, overflow and NaNs, etc.  If
this field is set to 0000, no flag registers are updated.  Flag registers are
not updated for instructions with embedded compares. This field may also be
referred to as the flag destination control field. This field applies to all
instructions except send, sendc, and math.
\*****************************************************************************/
typedef enum tagCONDMODIFIER {
  CONDMODIFIER_NONE = 0x0, // Do Not modify Flag Register
  CONDMODIFIER_E = 0x1,    // Equal
  CONDMODIFIER_Z = 0x1,    // Zero
  CONDMODIFIER_NE = 0x2,   // NotEqual
  CONDMODIFIER_NZ = 0x2,   // NotZero
  CONDMODIFIER_G = 0x3,    // Greater-than
  CONDMODIFIER_GE = 0x4,   // Greater-than-or-equal
  CONDMODIFIER_L = 0x5,    // Less-than
  CONDMODIFIER_LE = 0x6,   // Less-than-or-equal
  CONDMODIFIER_O = 0x8,    // Overflow
  CONDMODIFIER_U = 0x9,    // Unordered with Computed NaN
} CONDMODIFIER;

/*****************************************************************************\
Quarter Control
This field provides explicit control for ARF selection.
This field combined with ExecSize determines which channels are used for the ARF
registers. Along with NibCtrl, 1/8 DMask/VMask and ARF can be selected.

Programming Notes:
NibCtrl is only allowed for SIMD4 instructions with a DF (Double Float) source
or destination type.
\*****************************************************************************/
typedef enum tagQTRCTRL {
  QTRCTRL_1Q = 0x0, // Use first quarter for DMask/VMask. Use first half for
                    // everything else.
  QTRCTRL_2N = 0x0, // Use second 1/8th for DMask/VMask and ARF.
  QTRCTRL_1N = 0x0, // Use first 1/8th for DMask/VMask and ARF.
  QTRCTRL_1H = 0x0, // Use first half for DMask/VMask. Use all channels for
                    // everything else.
  QTRCTRL_4N = 0x1, // Use fourth 1/8th for DMask/VMask and ARF.
  QTRCTRL_2Q = 0x1, // Use second quarter for DMask/VMask. Use second half for
                    // everything else.
  QTRCTRL_3N = 0x1, // Use third 1/8th for DMask/VMask and ARF.
  QTRCTRL_2H = 0x2, // Use second half for DMask/VMask. Use all channels for
                    // everything else.
  QTRCTRL_6N = 0x2, // Use sixth 1/8th for DMask/VMask and ARF.
  QTRCTRL_5N = 0x2, // Use fifth 1/8th for DMask/VMask and ARF.
  QTRCTRL_3Q = 0x2, // Use third quarter for DMask/VMask. Use first half for
                    // everything else.
  QTRCTRL_8N = 0x3, // Use eighth 1/8th for DMask/VMask and ARF.
  QTRCTRL_4Q = 0x3, // Use fourth quarter for DMask/VMask. Use second half for
                    // everything else.
  QTRCTRL_7N = 0x3, // Use seventh 1/8th for DMask/VMask and ARF.
} QTRCTRL;

/*****************************************************************************\
Math Function Control
\*****************************************************************************/
typedef enum tagFC {
  FC_INV_RECIPROCAL = 0x1,
  FC_LOG = 0x2,
  FC_EXP = 0x3,
  FC_SQRT = 0x4,
  FC_RSQ = 0x5,
  FC_SIN = 0x6,
  FC_COS = 0x7,
  FC_FDIV = 0x9,
  FC_POW = 0xA,
  FC_INT_DIV_BOTH = 0xB,      // Return Quotient and Remainder
  FC_INT_DIV_QUOTIENT = 0xC,  // Return Quotient Only
  FC_INT_DIV_REMAINDER = 0xD, // Return Remainder
  FC_INVM = 0xE,
  FC_RSQRTM = 0xF,
} FC;

/*****************************************************************************\
The following table lists the assignments (encodings) of the Shared Function and
Fixed Function IDs used within the GPE. A Shared Function is a valid target of a
message initiated via a 'send' instruction. A Fixed Function is an identifiable
unit of the 3D or Media pipeline. Note that the Thread Spawner is both a Shared
Function and Fixed Function. Note: The initial intention was to combine these
two ID namespaces, so that (theoretically) an agent (such as the Thread Spawner)
that served both as a Shared Function and Fixed Function would have a single,
unique 4-bit ID encoding. However, this combination is not a requirement of the
architecture.

Programming Notes:
SFID_DP_DC1 is an extension of SFID_DP_DC0 to allow for more message types. They
act as a single logical entity.

SFID_DP_DC1, SFID_DP_DC2, SFID_DP_DC3 are extensions of SFID_DP_DC0 to allow for
more message types. They act as a single logical entity.
\*****************************************************************************/
typedef enum tagSFID {
  SFID_SFID_NULL = 0x0,    // Null
  SFID_SFID_SAMPLER = 0x2, // Sampler
  SFID_SFID_GATEWAY = 0x3, // Message Gateway
  SFID_SFID_DP_DC2 = 0x4,  // Data Cache Data Port 2
  SFID_SFID_DP_RC = 0x5,   // Render Cache Data Port
  SFID_SFID_URB = 0x6,     // URB
  SFID_SFID_SPAWNER = 0x7, // Thread Spawner
  SFID_SFID_VME = 0x8,     // Video Motion Estimation
  SFID_SFID_DP_DCRO = 0x9, // Data Cache Read Only Data Port
  SFID_SFID_DP_DC0 = 0xA,  // Data Cache Data Port
  SFID_SFID_PI = 0xB,      // Pixel Interpolator
  SFID_SFID_DP_DC1 = 0xC,  // Data Cache Data Port 1
  SFID_SFID_CRE = 0xD,     // Check and Refinement Engine
} SFID;

/*****************************************************************************\
Datatype for Ternary Align1 instruction.
\*****************************************************************************/
typedef enum tagTERNARYALIGN1DATATYPE {
  TERNARYALIGN1DATATYPE_UD = 0x0, // Unsigned Doubleword Integer (32-bit).
  TERNARYALIGN1DATATYPE_HF = 0x0, // Half Precision Float (16-bit).
  TERNARYALIGN1DATATYPE_D =
      0x1, // <span style="color: rgb(0, 0, 0); font-family: Arial, sans-serif;
           // line-height: normal;">Signed Doubleword Integer (32-bit).</span>
  TERNARYALIGN1DATATYPE_F = 0x1,  // Single Precision Float (32-bit).
  TERNARYALIGN1DATATYPE_UW = 0x2, // Unsigned Word Integer (16-bit).
  TERNARYALIGN1DATATYPE_DF = 0x2, // Double precision Float (64-bit).
  TERNARYALIGN1DATATYPE_W = 0x3,  // Signed Word Integer (16-bit).
  TERNARYALIGN1DATATYPE_NF = 0x3, // Native Precision Float (66-bit)
  TERNARYALIGN1DATATYPE_UB = 0x4, // Unsigned Byte Integer (8-bit).
  TERNARYALIGN1DATATYPE_B = 0x5,  // Signed Byte Integer (8-bit).
} TERNARYALIGN1DATATYPE;

/*****************************************************************************\
Source Vertical Stride is required for regioning/accessing datatypes of varied
size. It is one way to obtain a vector of scalars.
\*****************************************************************************/
typedef enum tagTERNARYALIGN1VERTSTRIDE {
  TERNARYALIGN1VERTSTRIDE_0_ELEMENTS = 0x0,
  TERNARYALIGN1VERTSTRIDE_2_ELEMENTS = 0x1,
  TERNARYALIGN1VERTSTRIDE_4_ELEMENTS = 0x2,
  TERNARYALIGN1VERTSTRIDE_8_ELEMENTS = 0x3,
} TERNARYALIGN1VERTSTRIDE;

typedef enum tagSATURATE {
  SATURATE_NO_DESTINATION_MODIFICATION = 0x0,
  SATURATE_SAT = 0x1, // Saturate the output
} SATURATE;

typedef enum tagACCWRCTRL {
  ACCWRCTRL_DON_T_WRITE_TO_ACC = 0x0,
  ACCWRCTRL_UPDATE_ACC = 0x1, // Write result to the ACC, and destination
} ACCWRCTRL;

typedef enum tagACCESSMODE {
  ACCESSMODE_ALIGN1 = 0x0,
  ACCESSMODE_ALIGN16 = 0x1,
} ACCESSMODE;

typedef enum tagNIBCTRL {
  NIBCTRL_ODD = 0x0,  // Use an odd 1/8th for DMask/VMask and ARF (first, third,
                      // fifth, or seventh depending on QtrCtrl).
  NIBCTRL_EVEN = 0x1, // Use an even 1/8th for DMask/VMask and ARF (second,
                      // fourth, sixth, or eighth depending on QtrCtrl).
} NIBCTRL;

typedef enum tagPREDINV {
  PREDINV_POSITIVE = 0x0, // Positive polarity of predication. Use the
                          // predication mask produced by PredCtrl
  PREDINV_NEGATIVE = 0x1, // Negative polarity of predication. If PredCtrl is
                          // nonzero, invert the predication mask.
} PREDINV;

typedef enum tagCMPTCTRL {
  CMPTCTRL_NOCOMPACTION = 0x0, // No compaction. 128-bit native instruction
                               // supporting all instruction options.
  CMPTCTRL_COMPACTED = 0x1, // Compaction is enabled. 64-bit compact instruction
                            // supporting only some instruction variations.
} CMPTCTRL;

typedef enum tagDEBUGCTRL {
  DEBUGCTRL_NO_BREAKPOINT = 0x0,
  DEBUGCTRL_BREAKPOINT = 0x1,
} DEBUGCTRL;

typedef enum tagEOT {
  EOT_THREAD_IS_NOT_TERMINATED = 0x0,
  EOT_EOT = 0x1,
} EOT;

typedef enum tagMASKCTRL {
  MASKCTRL_NORMAL = 0x0,
  MASKCTRL_WRITE_ALL_CHANNELS =
      0x1, // Except channels killed with predication control
} MASKCTRL;

typedef struct tagEU_INSTRUCTION_CONTROLS_A {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {

      /*****************************************************************************\
      Access Mode. This field determines the operand access for the instruction.
      It applies to all source and destination operands. When it is cleared
      (Align1), the instruction uses byte-aligned addressing for source and
      destination operands. Source swizzle control and destination mask control
      are not supported. When it is set (Align16), the instruction uses
      16-byte-aligned addressing for all source and destination operands. Source
      swizzle control and destination mask control are supported in this mode.
      \*****************************************************************************/
      BYTE Accessmode : BITFIELD_BIT(0); // ACCESSMODE

      /*****************************************************************************\
      Destination Dependency Control. This field selectively disables
      destination dependency check and clear for this instruction. When it is
      set to 00, normal destination dependency control is performed for the
      instruction  - hardware checks for destination hazards to ensure data
      integrity. Specifically, destination register dependency check is
      conducted before the instruction is made ready for execution. After the
      instruction is executed, the destination register scoreboard will be
      cleared when the destination operands retire. When bit 10 is set
      (NoDDClr), the destination register scoreboard will NOT be cleared when
      the destination operands retire.  When bit 11 is set (NoDDChk), hardware
      does not check for destination register dependency before the instruction
      is made ready for execution.  NoDDClr and NoDDChk are not mutual
      exclusive. When this field is not all-zero, hardware does not protect
      against destination hazards for the instruction.  This is typically used
      to assemble data in a fine grained fashion (e.g. matrix-vector compute
      with dot-product instructions), where the data integrity is guaranteed by
      software based on the intended usage of instruction sequences.
      \*****************************************************************************/
      BYTE Depctrl : BITFIELD_RANGE(1, 2); // DEPCTRL

      /*****************************************************************************\
      Nibble Control. This field is used in some instructions along with
      QtrCtrl. See the description of QtrCtrl below. NibCtrl is only used for
      SIMD4 instructions with a DF (Double Float) source or destination.

      Programming Notes:
      Note that if eighths are given zero-based indices from 0 to 7, then
      NibCtrl = 0 indicates even indices and NibCtrl = 1 indicates odd indices.
      \*****************************************************************************/
      BYTE Nibctrl : BITFIELD_BIT(3); // NIBCTRL

      /*****************************************************************************\
       - Quarter Control. This field provides explicit control for ARF
      selection. This field combined with NibCtrl and ExecSize determines which
      channels are used for the ARF registers.
      \*****************************************************************************/
      BYTE Qtrctrl : BITFIELD_RANGE(4, 5); // QTRCTRL

      /*****************************************************************************\
      Thread Control. This field provides explicit control for thread switching.
      If this field is set to 00b, it is up to the GEN execution units to manage
      thread switching. This is the normal (and unnamed) mode. In this mode, for
      example, if the current instruction cannot proceed due to operand
      dependencies, the EU switches to the next available thread to fill the
      compute pipe. In another example, if the current instruction is ready to
      go, however, there is another thread with higher priority that also has an
      instruction ready, the EU switches to that thread. If this field is set to
      Switch, a forced thread switch occurs after the current instruction is
      executed and before the next instruction. In addition, a long delay
      (longer than the execution pipe latency) is introduced for the current
      thread. Particularly, the instruction queue of the current thread is
      flushed after the current instruction is dispatched for execution. Switch
      is designed primarily as a safety feature in case there are race
      conditions for certain instructions.
      \*****************************************************************************/
      BYTE ThreadControl : BITFIELD_RANGE(6, 7);                 // THREADCTRL
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(8, 12); // Override

      /*****************************************************************************\
      This field determines the number of channels operating in parallel for
      this instruction.  The size cannot exceed the maximum number of channels
      allowed for the given data type.
      \*****************************************************************************/
      BYTE Execsize : BITFIELD_RANGE(13, 15); // EXECSIZE
    } Common;
    struct tagPropertyPredicationIsFalse {
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);   // Override
      BYTE _ : BITFIELD_RANGE(8, 11);                             // PREDCTRL
      BYTE __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(12);         //
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } PropertyPredicationIsFalse;
    struct tagPropertyPredicationIsTrue {
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7); // Override

      /*****************************************************************************\
      This field, together with PredInv, enables and controls the generation of
      the predication mask for the instruction.  It allows per-channel
      conditional execution of the instruction based on the content of the
      selected flag register.  Encoding depends on the access mode. In Align16
      access mode, there are eight encodings (including no predication). All
      encodings are based on group-of-4 predicate bits, including channel
      sequential, replication swizzles and horizontal any|all operations.  The
      same configuration is repeated for each group-of-4 execution channels.
      \*****************************************************************************/
      BYTE Predctrl : BITFIELD_RANGE(8, 11); // PREDCTRL

      /*****************************************************************************\
      This field, together with PredCtrl, enables and controls the generation of
      the predication mask for the instruction.  When it is set, the predication
      uses the inverse of the predication bits generated according to setting of
      Predicate Control. In other BYTEs, effect of PredInv happens after
      PredCtrl. This field is ignored by hardware if Predicate Control is set to
      0000  - there is no predication. PMask is the final predication mask
      produced by the effects of both fields.
      \*****************************************************************************/
      BYTE Predinv : BITFIELD_BIT(12);                            // PREDINV
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } PropertyPredicationIsTrue;
    BYTE RawData[1];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.Accessmode = ACCESSMODE_ALIGN1;
    TheStructure.Common.Depctrl = 0x0;
    TheStructure.Common.Nibctrl = NIBCTRL_ODD;
    TheStructure.Common.Qtrctrl = 0x0;
    TheStructure.Common.ThreadControl = 0x0;
    TheStructure.Common.Execsize = 0x0;
  }

  static tagEU_INSTRUCTION_CONTROLS_A sInit() {
    tagEU_INSTRUCTION_CONTROLS_A state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE BYTE &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE ACCESSMODE GetAccessmode() const {
    __CODEGEN_GET_MACRO();
    return (ACCESSMODE)TheStructure.Common.Accessmode;
  }

  __CODEGEN_INLINE void SetAccessmode(ACCESSMODE AccessmodeValue) {
    __CODEGEN_SET_MACRO(AccessmodeValue);
    TheStructure.Common.Accessmode = AccessmodeValue;
  }

  __CODEGEN_INLINE DEPCTRL GetDepctrl() const {
    __CODEGEN_GET_MACRO();
    return (DEPCTRL)TheStructure.Common.Depctrl;
  }

  __CODEGEN_INLINE void SetDepctrl(DEPCTRL DepctrlValue) {
    __CODEGEN_SET_MACRO(DepctrlValue);
    TheStructure.Common.Depctrl = DepctrlValue;
  }

  __CODEGEN_INLINE NIBCTRL GetNibctrl() const {
    __CODEGEN_GET_MACRO();
    return (NIBCTRL)TheStructure.Common.Nibctrl;
  }

  __CODEGEN_INLINE void SetNibctrl(NIBCTRL NibctrlValue) {
    __CODEGEN_SET_MACRO(NibctrlValue);
    TheStructure.Common.Nibctrl = NibctrlValue;
  }

  __CODEGEN_INLINE QTRCTRL GetQtrctrl() const {
    __CODEGEN_GET_MACRO();
    return (QTRCTRL)TheStructure.Common.Qtrctrl;
  }

  __CODEGEN_INLINE void SetQtrctrl(QTRCTRL QtrctrlValue) {
    __CODEGEN_SET_MACRO(QtrctrlValue);
    TheStructure.Common.Qtrctrl = QtrctrlValue;
  }

  __CODEGEN_INLINE THREADCTRL GetThreadControl() const {
    __CODEGEN_GET_MACRO();
    return (THREADCTRL)TheStructure.Common.ThreadControl;
  }

  __CODEGEN_INLINE void SetThreadControl(THREADCTRL ThreadControlValue) {
    __CODEGEN_SET_MACRO(ThreadControlValue);
    TheStructure.Common.ThreadControl = ThreadControlValue;
  }

  __CODEGEN_INLINE EXECSIZE GetExecsize() const {
    __CODEGEN_GET_MACRO();
    return (EXECSIZE)TheStructure.Common.Execsize;
  }

  __CODEGEN_INLINE void SetExecsize(EXECSIZE ExecsizeValue) {
    __CODEGEN_SET_MACRO(ExecsizeValue);
    TheStructure.Common.Execsize = ExecsizeValue;
  }

  __CODEGEN_INLINE PREDCTRL Get_() const {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)TheStructure.PropertyPredicationIsFalse._;
  }

  __CODEGEN_INLINE void Set_(PREDCTRL _Value) {
    __CODEGEN_SET_MACRO(_Value);
    TheStructure.PropertyPredicationIsFalse._ = _Value;
  }

  __CODEGEN_INLINE PREDCTRL GetPredctrl() const {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)TheStructure.PropertyPredicationIsTrue.Predctrl;
  }

  __CODEGEN_INLINE void SetPredctrl(PREDCTRL PredctrlValue) {
    __CODEGEN_SET_MACRO(PredctrlValue);
    TheStructure.PropertyPredicationIsTrue.Predctrl = PredctrlValue;
  }

  __CODEGEN_INLINE PREDINV GetPredinv() const {
    __CODEGEN_GET_MACRO();
    return (PREDINV)TheStructure.PropertyPredicationIsTrue.Predinv;
  }

  __CODEGEN_INLINE void SetPredinv(PREDINV PredinvValue) {
    __CODEGEN_SET_MACRO(PredinvValue);
    TheStructure.PropertyPredicationIsTrue.Predinv = PredinvValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_CONTROLS_A;

typedef struct tagEU_INSTRUCTION_CONTROLS {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      EU_INSTRUCTION_CONTROLS_A ControlsA;
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(16, 19); // Override

      /*****************************************************************************\
      AccWrCtrl. This field allows per instruction accumulator write control.
      \*****************************************************************************/
      BYTE ControlsB_Accwrctrl : BITFIELD_BIT(20); // ACCWRCTRL

      /*****************************************************************************\
      Compaction Control
      Indicates whether the instruction is compacted to the 64-bit compact
      instruction format. When this bit is set, the 64-bit compact instruction
      format is used. The EU decodes the compact format using lookup tables
      internal to the hardware, but documented for use by software tools. Only
      some instruction variations can be compacted, the variations supported by
      those lookup tables and the compact format. See EU Compact Instruction
      Format [DevBDW+] for more information.
      \*****************************************************************************/
      BYTE ControlsB_Cmptctrl : BITFIELD_BIT(21); // CMPTCTRL

      /*****************************************************************************\
      This field allows the insertion of a breakpoint at the current
      instruction. When the bit is set, hardware automatically stores the
      current IP in CR register and jumps to the System IP (SIP) BEFORE
      executing the current instruction.
      \*****************************************************************************/
      BYTE ControlsB_Debugctrl : BITFIELD_BIT(22);          // DEBUGCTRL
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(23); // Override
    } Common;
    struct tagPropertyConditionalModifierIsFalse {
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);   // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(8, 15);  // Override
      BYTE __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(16, 19);   // MBZ
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(20, 23); // Override
    } PropertyConditionalModifierIsFalse;
    struct tagPropertyConditionalModifierIsTrue {
      //            BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(  0, 15);
      //            // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);  // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(8, 15); // Override
      /*****************************************************************************\
      Does not exist for send/sendc/math/branch/break-continue opcodes
      \*****************************************************************************/
      BYTE Condmodifier : BITFIELD_RANGE(16, 19); // CONDMODIFIER
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(20, 23); // Override
    } PropertyConditionalModifierIsTrue;
    struct tagPropertySaturationIsFalse {
      // BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(  0, 15); //
      // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);   // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(8, 15);  // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(16, 22); // Override
      BYTE __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(23);         // MBZ
    } PropertySaturationIsFalse;
    struct tagPropertySaturationIsTrue {
      // BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(  0, 15); //
      // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);   // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(8, 15);  // Override
      BYTE __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(16, 22); // Override

      /*****************************************************************************\
      Enables or disables destination saturation.
      When it is set, output values to the destination register are saturated.
      The saturation operation depends on the destination data type. Saturation
      is the operation that converts any value outside the saturation target
      range for the data type to the closest value in the target range. For a
      floating-point destination type, the saturation target range is
      [0.0, 1.0]. For a floating-point NaN, there is no  -closest value -; any
      NaN saturates to 0.0. Note that enabling Saturate overrides all of the NaN
      propagation behaviors described for various numeric instructions. Any
      floating-point number greater than 1.0, including +INF, saturates to 1.0.
      Any negative floating-point number, including -INF, saturates to 0.0. Any
      floating-point number in the range 0.0 to 1.0 is not changed by
      saturation. For an integer destination type, the maximum range for that
      type is the saturation target range. For example, the saturation range for
      B (Signed Byte Integer) is [-128, 127]. When Saturate is clear,
      destination values are not saturated. For example, a wrapped result
      (modulo) is output to the destination for an overflowed integer value. See
      the Numeric Data Typessection for information about data types and their
      ranges.
      \*****************************************************************************/
      BYTE ControlsB_Saturate : BITFIELD_BIT(23); // SATURATE
    } PropertySaturationIsTrue;
    BYTE RawData[3];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.ControlsA.Init();
    TheStructure.Common.ControlsB_Accwrctrl = ACCWRCTRL_DON_T_WRITE_TO_ACC;
    TheStructure.Common.ControlsB_Cmptctrl = CMPTCTRL_NOCOMPACTION;
    TheStructure.Common.ControlsB_Debugctrl = DEBUGCTRL_NO_BREAKPOINT;
  }

  static tagEU_INSTRUCTION_CONTROLS sInit() {
    tagEU_INSTRUCTION_CONTROLS state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE BYTE &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE EU_INSTRUCTION_CONTROLS_A &GetControlsA() {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.ControlsA;
  }

  __CODEGEN_INLINE ACCWRCTRL GetControlsB_Accwrctrl() const {
    __CODEGEN_GET_MACRO();
    return (ACCWRCTRL)TheStructure.Common.ControlsB_Accwrctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Accwrctrl(ACCWRCTRL ControlsB_AccwrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_AccwrctrlValue);
    TheStructure.Common.ControlsB_Accwrctrl = ControlsB_AccwrctrlValue;
  }

  __CODEGEN_INLINE CMPTCTRL GetControlsB_Cmptctrl() const {
    __CODEGEN_GET_MACRO();
    return (CMPTCTRL)TheStructure.Common.ControlsB_Cmptctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Cmptctrl(CMPTCTRL ControlsB_CmptctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_CmptctrlValue);
    TheStructure.Common.ControlsB_Cmptctrl = ControlsB_CmptctrlValue;
  }

  __CODEGEN_INLINE DEBUGCTRL GetControlsB_Debugctrl() const {
    __CODEGEN_GET_MACRO();
    return (DEBUGCTRL)TheStructure.Common.ControlsB_Debugctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Debugctrl(DEBUGCTRL ControlsB_DebugctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_DebugctrlValue);
    TheStructure.Common.ControlsB_Debugctrl = ControlsB_DebugctrlValue;
  }

  __CODEGEN_INLINE CONDMODIFIER GetCondmodifier() const {
    __CODEGEN_GET_MACRO();
    return (CONDMODIFIER)
        TheStructure.PropertyConditionalModifierIsTrue.Condmodifier;
  }

  __CODEGEN_INLINE void SetCondmodifier(CONDMODIFIER CondmodifierValue) {
    __CODEGEN_SET_MACRO(CondmodifierValue);
    TheStructure.PropertyConditionalModifierIsTrue.Condmodifier =
        CondmodifierValue;
  }

  __CODEGEN_INLINE SATURATE GetControlsB_Saturate() const {
    __CODEGEN_GET_MACRO();
    return (SATURATE)TheStructure.PropertySaturationIsTrue.ControlsB_Saturate;
  }

  __CODEGEN_INLINE void
  SetControlsB_Saturate(SATURATE ControlsB_SaturateValue) {
    __CODEGEN_SET_MACRO(ControlsB_SaturateValue);
    TheStructure.PropertySaturationIsTrue.ControlsB_Saturate =
        ControlsB_SaturateValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_CONTROLS;

static_assert(3 == sizeof(EU_INSTRUCTION_CONTROLS));

typedef struct tagEU_INSTRUCTION_HEADER {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      BYTE Opcode : BITFIELD_RANGE(0, 6);                // EU_OPCODE
      BYTE __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(7); // MBZ
      EU_INSTRUCTION_CONTROLS Control;
    } Common;
    BYTE RawData[4];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.Opcode = 0x0;
    TheStructure.RawData[1] = 0x0;
    TheStructure.Common.Control.Init();
  }

  static tagEU_INSTRUCTION_HEADER sInit() {
    tagEU_INSTRUCTION_HEADER state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE BYTE &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() const {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)TheStructure.Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    TheStructure.Common.Opcode = OpcodeValue;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_CONTROLS &GetControl() {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.Control;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_HEADER;

static_assert(4 == sizeof(EU_INSTRUCTION_HEADER));

typedef struct tagEU_INSTRUCTION_OPERAND_DST_ALIGN1 {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12); // Override

      /*****************************************************************************\
      For a send instruction, this field applies to CurrDst. PostDst only uses
      the register number.
      \*****************************************************************************/
      WORD DestinationHorizontalStride : BITFIELD_RANGE(13, 14); // HORZSTRIDE

      /*****************************************************************************\
      For a send instruction, this field applies to PostDst - the post
      destination operand. Addressing mode for CurrDst (current destination
      operand) is fixed as Direct. (See Instruction Reference chapter for
      CurrDst and PostDst.)
      \*****************************************************************************/
      WORD DestinationAddressingMode : BITFIELD_BIT(15); // ADDRMODE
    } Common;
    struct tagDestinationAddressingModeIsDirect {
      WORD DestinationSubregisterNumber_DestinationSubRegisterNumber
          : BITFIELD_RANGE(0, 4); //
      WORD DestinationRegisterNumber_DestinationRegisterNumber
          : BITFIELD_RANGE(5, 12);                                //
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } DestinationAddressingModeIsDirect;
    struct tagDestinationAddressingModeIsIndirect {

      /*****************************************************************************\
      For a send instruction, this field applies to PostDst.
      \*****************************************************************************/
      WORD DestinationAddressImmediate : BITFIELD_RANGE(0, 8); // S8
      WORD DestinationAddressSubregisterNumber_AddressSubregisterNumber
          : BITFIELD_RANGE(9, 12);                                //
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } DestinationAddressingModeIsIndirect;
    WORD RawData[1];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.DestinationHorizontalStride = 0x0;
    TheStructure.Common.DestinationAddressingMode = 0x0;
  }

  static tagEU_INSTRUCTION_OPERAND_DST_ALIGN1 sInit() {
    tagEU_INSTRUCTION_OPERAND_DST_ALIGN1 state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED WORD &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE HORZSTRIDE GetDestinationHorizontalStride() const {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)TheStructure.Common.DestinationHorizontalStride;
  }

  __CODEGEN_INLINE void
  SetDestinationHorizontalStride(HORZSTRIDE DestinationHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(DestinationHorizontalStrideValue);
    TheStructure.Common.DestinationHorizontalStride =
        DestinationHorizontalStrideValue;
  }

  __CODEGEN_INLINE ADDRMODE GetDestinationAddressingMode() const {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)TheStructure.Common.DestinationAddressingMode;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressingMode(ADDRMODE DestinationAddressingModeValue) {
    __CODEGEN_SET_MACRO(DestinationAddressingModeValue);
    TheStructure.Common.DestinationAddressingMode =
        DestinationAddressingModeValue;
  }

  __CODEGEN_INLINE WORD
  GetDestinationSubregisterNumber_DestinationSubRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsDirect
        .DestinationSubregisterNumber_DestinationSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetDestinationSpecialAcc(WORD SpecialAccValue) {
    __CODEGEN_SET_MACRO(
        DestinationSubregisterNumber_DestinationSubRegisterNumberValue);
    TheStructure.DestinationAddressingModeIsDirect
        .DestinationSubregisterNumber_DestinationSubRegisterNumber =
        SpecialAccValue;
  }

  __CODEGEN_INLINE void
  SetDestinationSubregisterNumber_DestinationSubRegisterNumber(
      WORD DestinationSubregisterNumber_DestinationSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationSubregisterNumber_DestinationSubRegisterNumberValue);
    TheStructure.DestinationAddressingModeIsDirect
        .DestinationSubregisterNumber_DestinationSubRegisterNumber =
        DestinationSubregisterNumber_DestinationSubRegisterNumberValue;
  }

  __CODEGEN_INLINE WORD
  GetDestinationRegisterNumber_DestinationRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsDirect
        .DestinationRegisterNumber_DestinationRegisterNumber;
  }

  __CODEGEN_INLINE void SetDestinationRegisterNumber_DestinationRegisterNumber(
      WORD DestinationRegisterNumber_DestinationRegisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationRegisterNumber_DestinationRegisterNumberValue);
    TheStructure.DestinationAddressingModeIsDirect
        .DestinationRegisterNumber_DestinationRegisterNumber =
        DestinationRegisterNumber_DestinationRegisterNumberValue;
  }

  __CODEGEN_INLINE WORD GetDestinationAddressImmediate() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressImmediate;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressImmediate(WORD DestinationAddressImmediateValue) {
    __CODEGEN_SET_MACRO(DestinationAddressImmediateValue);
    TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressImmediate = DestinationAddressImmediateValue;
  }

  __CODEGEN_INLINE WORD
  GetDestinationAddressSubregisterNumber_AddressSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressSubregisterNumber_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressSubregisterNumber_AddressSubregisterNumber(
      WORD DestinationAddressSubregisterNumber_AddressSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationAddressSubregisterNumber_AddressSubregisterNumberValue);
    TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressSubregisterNumber_AddressSubregisterNumber =
        DestinationAddressSubregisterNumber_AddressSubregisterNumberValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_OPERAND_DST_ALIGN1;

static_assert(2 == sizeof(EU_INSTRUCTION_OPERAND_DST_ALIGN1));

typedef struct tagEU_INSTRUCTION_OPERAND_DST_ALIGN16 {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {

      /*****************************************************************************\
      For a send instruction, this field applies to the CurrDst
      \*****************************************************************************/
      WORD DestinationChannelEnable : BITFIELD_RANGE(0, 3);      // ChanEn[4]
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(4, 12); // Override

      /*****************************************************************************\


      Programming Notes:
      Although Dst.HorzStride is a don?t care for Align16, HW needs this to be
      programmed as ?01?.
      \*****************************************************************************/
      WORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14); //

      /*****************************************************************************\
      For a send instruction, this field applies to PostDst - the post
      destination operand. Addressing mode for CurrDst (current destination
      operand) is fixed as Direct. (See Instruction Reference chapter for
      CurrDst and PostDst.)
      \*****************************************************************************/
      WORD DestinationAddressingMode : BITFIELD_BIT(15); // ADDRMODE
    } Common;
    struct tagDestinationAddressingModeIsDirect {
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override

      /*****************************************************************************\
      For a send instruction, this field applies to CurrDst.
      \*****************************************************************************/
      WORD DestinationSubregisterNumber : BITFIELD_BIT(4); // DstSubRegNum[4:4]
      WORD DestinationRegisterNumber_DestinationRegisterNumber
          : BITFIELD_RANGE(5, 12);                                //
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } DestinationAddressingModeIsDirect;
    struct tagDestinationAddressingModeIsIndirect {
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override

      /*****************************************************************************\
      For a send instruction, this field applies to PostDst
      \*****************************************************************************/
      WORD DestinationAddressImmediate84 : BITFIELD_RANGE(4, 8); // S8[8:4]
      WORD DestinationAddressSubregisterNumber_AddressSubregisterNumber
          : BITFIELD_RANGE(9, 12);                                //
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 15); // Override
    } DestinationAddressingModeIsIndirect;
    WORD RawData[1];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.DestinationChannelEnable = 0x0;
    TheStructure.Common.DestinationAddressingMode = 0x0;
  }

  static tagEU_INSTRUCTION_OPERAND_DST_ALIGN16 sInit() {
    tagEU_INSTRUCTION_OPERAND_DST_ALIGN16 state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED WORD &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE WORD GetDestinationChannelEnable() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.Common.DestinationChannelEnable;
  }

  __CODEGEN_INLINE void
  SetDestinationChannelEnable(WORD DestinationChannelEnableValue) {
    __CODEGEN_SET_MACRO(DestinationChannelEnableValue);
    TheStructure.Common.DestinationChannelEnable =
        DestinationChannelEnableValue;
  }

  __CODEGEN_INLINE ADDRMODE GetDestinationAddressingMode() const {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)TheStructure.Common.DestinationAddressingMode;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressingMode(ADDRMODE DestinationAddressingModeValue) {
    __CODEGEN_SET_MACRO(DestinationAddressingModeValue);
    TheStructure.Common.DestinationAddressingMode =
        DestinationAddressingModeValue;
  }

  typedef enum tagDESTINATIONSUBREGISTERNUMBER {
    DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT = 0x4,
    DESTINATIONSUBREGISTERNUMBER_ALIGN_SIZE = 0x10,
  } DESTINATIONSUBREGISTERNUMBER;

  __CODEGEN_INLINE WORD GetDestinationSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsDirect
               .DestinationSubregisterNumber
           << DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT;
  }

  __CODEGEN_INLINE void
  SetDestinationSubregisterNumber(WORD DestinationSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationSubregisterNumberValue);
    TheStructure.DestinationAddressingModeIsDirect
        .DestinationSubregisterNumber = DestinationSubregisterNumberValue >>
                                        DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT;
  }

  __CODEGEN_INLINE WORD
  GetDestinationRegisterNumber_DestinationRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsDirect
        .DestinationRegisterNumber_DestinationRegisterNumber;
  }

  __CODEGEN_INLINE void SetDestinationRegisterNumber_DestinationRegisterNumber(
      WORD DestinationRegisterNumber_DestinationRegisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationRegisterNumber_DestinationRegisterNumberValue);
    TheStructure.DestinationAddressingModeIsDirect
        .DestinationRegisterNumber_DestinationRegisterNumber =
        DestinationRegisterNumber_DestinationRegisterNumberValue;
  }

  typedef enum tagDESTINATIONADDRESSIMMEDIATE84 {
    DESTINATIONADDRESSIMMEDIATE84_BIT_SHIFT = 0x4,
    DESTINATIONADDRESSIMMEDIATE84_ALIGN_SIZE = 0x10,
  } DESTINATIONADDRESSIMMEDIATE84;

  __CODEGEN_INLINE WORD GetDestinationAddressImmediate84() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsIndirect
               .DestinationAddressImmediate84
           << DESTINATIONADDRESSIMMEDIATE84_BIT_SHIFT;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressImmediate84(WORD DestinationAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(DestinationAddressImmediate84Value);
    TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressImmediate84 =
        DestinationAddressImmediate84Value >>
        DESTINATIONADDRESSIMMEDIATE84_BIT_SHIFT;
  }

  __CODEGEN_INLINE WORD
  GetDestinationAddressSubregisterNumber_AddressSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (WORD)TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressSubregisterNumber_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressSubregisterNumber_AddressSubregisterNumber(
      WORD DestinationAddressSubregisterNumber_AddressSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationAddressSubregisterNumber_AddressSubregisterNumberValue);
    TheStructure.DestinationAddressingModeIsIndirect
        .DestinationAddressSubregisterNumber_AddressSubregisterNumber =
        DestinationAddressSubregisterNumber_AddressSubregisterNumberValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_OPERAND_DST_ALIGN16;

static_assert(2 == sizeof(EU_INSTRUCTION_OPERAND_DST_ALIGN16));

typedef struct tagEU_INSTRUCTION_OPERAND_CONTROLS {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      DWORD FlagRegisterNumberSubregisterNumber : BITFIELD_RANGE(0, 1); //

      /*****************************************************************************\
      Mask Control (formerly Write Enable Control). This field determines if the
      the per channel write enables are used to generate the final write enable.
      This field should be normally "0".

      Programming Notes:
      MaskCtrl = NoMask skips the check for PcIP[n] == ExIP before enabling a
      channel, as described in the Evaluate Write Enable section.
      \*****************************************************************************/
      DWORD Maskctrl : BITFIELD_BIT(2);                     // MASKCTRL
      DWORD DestinationRegisterFile : BITFIELD_RANGE(3, 4); // REGFILE

      /*****************************************************************************\
      This field specifies the numeric data type of the destination operand dst.
      The bits of the destination operand are interpreted as the identified
      numeric data type, rather than coerced into a type implied by the
      operator. For a send instruction, this field applies to the CurrDst  - the
      current destination operand.
      \*****************************************************************************/
      DWORD DestinationDataType : BITFIELD_RANGE(5, 8);            // DSTTYPE
      DWORD Src0Regfile : BITFIELD_RANGE(9, 10);                   // REGFILE
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(11, 31); // Override
    } Common;
    struct tagSrc0RegfileNotImm {
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 10);  // Override
      DWORD Src0Srctype : BITFIELD_RANGE(11, 14);                  // SRCTYPE
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 31); // Override
    } Src0RegfileNotImm;
    struct tagSrc0RegfileIsImm {
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 10);  // Override
      DWORD Src0Srctype : BITFIELD_RANGE(11, 14);                  // SRCIMMTYPE
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 31); // Override
    } Src0RegfileIsImm;
    struct tagDestinationRegisterRegionDestinationAddressingModeIsDirect {
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);  // Override
      DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(15);         // MBZ
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(16, 31); // Override
    } DestinationRegisterRegionDestinationAddressingModeIsDirect;
    struct tagDestinationRegisterRegionDestinationAddressingModeIsIndirect {
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);  // Override
      DWORD DestinationAddressImmediate99 : BITFIELD_BIT(15);      // U1
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(16, 31); // Override
    } DestinationRegisterRegionDestinationAddressingModeIsIndirect;
    struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16 {
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 15); // Override
      EU_INSTRUCTION_OPERAND_DST_ALIGN16 DestinationRegisterRegion;
      // EU_INSTRUCTION_OPERAND_DST_ALIGN16 __CODEGEN_UNIQUE(Overridden) :
      // BITFIELD_RANGE( 16, 31); // Override
    } StructureEu_Instruction_Controls_AAccessmodeIsAlign16;
    struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1 {
      WORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 15); // Override
      EU_INSTRUCTION_OPERAND_DST_ALIGN1 DestinationRegisterRegion;
      // EU_INSTRUCTION_OPERAND_DST_ALIGN1 __CODEGEN_UNIQUE(Overridden) :
      // BITFIELD_RANGE( 16, 31); // Override
    } StructureEu_Instruction_Controls_AAccessmodeIsAlign1;
    DWORD RawData[1];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.FlagRegisterNumberSubregisterNumber = 0x0;
    TheStructure.Common.Maskctrl = MASKCTRL_NORMAL;
    TheStructure.Common.DestinationRegisterFile = 0x0;
    TheStructure.Common.DestinationDataType = 0x0;
    TheStructure.Common.Src0Regfile = 0x0;
  }

  static tagEU_INSTRUCTION_OPERAND_CONTROLS sInit() {
    tagEU_INSTRUCTION_OPERAND_CONTROLS state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE DWORD GetFlagRegisterNumberSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.FlagRegisterNumberSubregisterNumber;
  }

  __CODEGEN_INLINE void SetFlagRegisterNumberSubregisterNumber(
      DWORD FlagRegisterNumberSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagRegisterNumberSubregisterNumberValue);
    TheStructure.Common.FlagRegisterNumberSubregisterNumber =
        FlagRegisterNumberSubregisterNumberValue;
  }

  __CODEGEN_INLINE MASKCTRL GetMaskctrl() const {
    __CODEGEN_GET_MACRO();
    return (MASKCTRL)TheStructure.Common.Maskctrl;
  }

  __CODEGEN_INLINE void SetMaskctrl(MASKCTRL MaskctrlValue) {
    __CODEGEN_SET_MACRO(MaskctrlValue);
    TheStructure.Common.Maskctrl = MaskctrlValue;
  }

  __CODEGEN_INLINE REGFILE GetDestinationRegisterFile() const {
    __CODEGEN_GET_MACRO();
    return (REGFILE)TheStructure.Common.DestinationRegisterFile;
  }

  __CODEGEN_INLINE void
  SetDestinationRegisterFile(REGFILE DestinationRegisterFileValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterFileValue);
    TheStructure.Common.DestinationRegisterFile = DestinationRegisterFileValue;
  }

  __CODEGEN_INLINE int GetDestinationDataType() const {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.DestinationDataType;
  }

  __CODEGEN_INLINE void SetDestinationDataType(int DestinationDataTypeValue) {
    __CODEGEN_SET_MACRO(DestinationDataTypeValue);
    TheStructure.Common.DestinationDataType = DestinationDataTypeValue;
  }

  __CODEGEN_INLINE REGFILE GetSrc0Regfile() const {
    __CODEGEN_GET_MACRO();
    return (REGFILE)TheStructure.Common.Src0Regfile;
  }

  __CODEGEN_INLINE void SetSrc0Regfile(REGFILE Src0RegfileValue) {
    __CODEGEN_SET_MACRO(Src0RegfileValue);
    TheStructure.Common.Src0Regfile = Src0RegfileValue;
  }

  __CODEGEN_INLINE int GetSrc0Srctype() const {
    __CODEGEN_GET_MACRO();
    return TheStructure.Src0RegfileNotImm.Src0Srctype;
  }

  __CODEGEN_INLINE void SetSrc0Srctype(int Src0SrctypeValue) {
    __CODEGEN_SET_MACRO(Src0SrctypeValue);
    TheStructure.Src0RegfileNotImm.Src0Srctype = Src0SrctypeValue;
  }

  __CODEGEN_INLINE int GetSrc0Srctype_Imm() const {
    __CODEGEN_GET_MACRO();
    return TheStructure.Src0RegfileIsImm.Src0Srctype;
  }

  __CODEGEN_INLINE void SetSrc0Srctype_Imm(int Src0SrctypeValue) {
    __CODEGEN_SET_MACRO(Src0SrctypeValue);
    TheStructure.Src0RegfileIsImm.Src0Srctype = Src0SrctypeValue;
  }

  __CODEGEN_INLINE DWORD GetDestinationAddressImmediate99() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .DestinationRegisterRegionDestinationAddressingModeIsIndirect
        .DestinationAddressImmediate99;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressImmediate99(DWORD DestinationAddressImmediate99Value) {
    __CODEGEN_SET_MACRO(DestinationAddressImmediate99Value);
    TheStructure.DestinationRegisterRegionDestinationAddressingModeIsIndirect
        .DestinationAddressImmediate99 = DestinationAddressImmediate99Value;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_DST_ALIGN16 &
  GetDestinationRegisterRegion_Align16() {
    __CODEGEN_GET_MACRO();
    return TheStructure.StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .DestinationRegisterRegion;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_DST_ALIGN1 &
  GetDestinationRegisterRegion_Align1() {
    __CODEGEN_GET_MACRO();
    return TheStructure.StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .DestinationRegisterRegion;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressingMode(ADDRMODE DestinationAddressingModeValue) {

    GetDestinationRegisterRegion_Align1().SetDestinationAddressingMode(
        DestinationAddressingModeValue);
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_OPERAND_CONTROLS;

static_assert(4 == sizeof(EU_INSTRUCTION_OPERAND_CONTROLS));

/*****************************************************************************\
Single source, immediate
\*****************************************************************************/
typedef union tagEU_INSTRUCTION_SOURCES_IMM32 {
  struct tagCommon {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 24); // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(25, 31);  // MBZ
    QWORD Source0Immediate : BITFIELD_RANGE(32, 63);            //
  } Common;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD Source0_SourceSubRegisterNumber : BITFIELD_RANGE(0, 4); //
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);   //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD Source0_SourceAddressImmediate80 : BITFIELD_RANGE(0, 8);  // S9[8:0]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 30);    // Override
    QWORD Source0_SourceAddressImmediate9 : BITFIELD_RANGE(31, 31); // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);    // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);         // ADDRMODE
    QWORD Source0_SourceHorizontalStride : BITFIELD_RANGE(16, 17); // HORZSTRIDE
    QWORD Source0_SourceWidth : BITFIELD_RANGE(18, 20);            // WIDTH
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63);   // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD Source0_SourceChannelSelect30
        : BITFIELD_RANGE(0, 3); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(4, 14); // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);      // ADDRMODE
    QWORD Source0_SourceChannelSelect74
        : BITFIELD_RANGE(16, 19);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(20); // MBZ
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24); // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override
    QWORD Source0_SourceSubregisterNumber44
        : BITFIELD_BIT(4); // SrcSubRegNum[4:4]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);  //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3);      // Override
    QWORD Source0_SourceAddressImmediate84 : BITFIELD_RANGE(4, 8);  // S9[8:4]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  DWORD RawData[2];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    RawData[1] = 0x0;
    Common.Source0Immediate = 0x0;
  }

  static tagEU_INSTRUCTION_SOURCES_IMM32 sInit() {
    tagEU_INSTRUCTION_SOURCES_IMM32 state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE QWORD GetSource0Immediate() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Source0Immediate;
  }

  __CODEGEN_INLINE void SetSource0Immediate(QWORD Source0ImmediateValue) {
    __CODEGEN_SET_MACRO(Source0ImmediateValue);
    Common.Source0Immediate = Source0ImmediateValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubRegisterNumber(
      QWORD Source0_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubRegisterNumber = Source0_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceRegisterNumber = Source0_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate80(
      QWORD Source0_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate80 =
        Source0_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate9(
      QWORD Source0_SourceAddressImmediate9Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate9 = Source0_SourceAddressImmediate9Value;
  }
  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_0(
      QWORD Source0_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_0Value;
  }

  // TODO: disabled, does not compile
  //__CODEGEN_INLINE SRCMOD GetSource0_SourceModifier_0() {
  //     __CODEGEN_GET_MACRO();
  //     return
  //     (SRCMOD)StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue.Source0_SourceModifier_0;
  // }
  //
  //__CODEGEN_INLINE void SetSource0_SourceModifier_0(SRCMOD
  //Source0_SourceModifier_0Value) {
  //     __CODEGEN_SET_MACRO(Source0_SourceModifier_0Value);
  //     StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue.Source0_SourceModifier
  //     = Source0_SourceModifier_0Value;
  // }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_0(
      ADDRMODE Source0_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource0_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceHorizontalStride(
      HORZSTRIDE Source0_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceHorizontalStride = Source0_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource0_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource0_SourceWidth(WIDTH Source0_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source0_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceWidth = Source0_SourceWidthValue;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceVerticalStride(
      VERTSTRIDE Source0_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceVerticalStride = Source0_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect30(QWORD Source0_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect30 = Source0_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_1() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_1(
      ADDRMODE Source0_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
                   .Source0_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect74(QWORD Source0_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect74 =
        Source0_SourceChannelSelect74Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
                   .Source0_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber44(
      QWORD Source0_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubregisterNumber44 =
        Source0_SourceSubregisterNumber44Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
                   .Source0_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate84(
      QWORD Source0_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate84 =
        Source0_SourceAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE SRCMOD GetSource0_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
            .Source0_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceModifier(SRCMOD Source0_SourceModifier_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceModifier_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
        .Source0_SourceModifier = Source0_SourceModifier_1Value;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SOURCES_IMM32;

static_assert(8 == sizeof(EU_INSTRUCTION_SOURCES_IMM32));

/*****************************************************************************\
Single source, register
\*****************************************************************************/
typedef union tagEU_INSTRUCTION_SOURCES_REG {
  struct tagCommon {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 24); // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(25, 63);  // MBZ
  } Common;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD Source0_SourceSubRegisterNumber : BITFIELD_RANGE(0, 4); //
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);   //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD Source0_SourceAddressImmediate80 : BITFIELD_RANGE(0, 8);  // S9[8:0]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 30);    // Override
    QWORD Source0_SourceAddressImmediate9 : BITFIELD_RANGE(31, 31); // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);    // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);         // ADDRMODE
    QWORD Source0_SourceHorizontalStride : BITFIELD_RANGE(16, 17); // HORZSTRIDE
    QWORD Source0_SourceWidth : BITFIELD_RANGE(18, 20);            // WIDTH
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63);   // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD Source0_SourceChannelSelect30
        : BITFIELD_RANGE(0, 3); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(4, 14); // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);      // ADDRMODE
    QWORD Source0_SourceChannelSelect74
        : BITFIELD_RANGE(16, 19);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(20); // MBZ
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24); // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override
    QWORD Source0_SourceSubregisterNumber44
        : BITFIELD_BIT(4); // SrcSubRegNum[4:4]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);  //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3);      // Override
    QWORD Source0_SourceAddressImmediate84 : BITFIELD_RANGE(4, 8);  // S9[8:4]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  DWORD RawData[2];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    RawData[1] = 0x0;
  }

  static tagEU_INSTRUCTION_SOURCES_REG sInit() {
    tagEU_INSTRUCTION_SOURCES_REG state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubRegisterNumber(
      QWORD Source0_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubRegisterNumber = Source0_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceRegisterNumber = Source0_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate80(
      QWORD Source0_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate80 =
        Source0_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate9(
      QWORD Source0_SourceAddressImmediate9Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate9 = Source0_SourceAddressImmediate9Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_0(
      QWORD Source0_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_0Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource0_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
            .Source0_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceModifier(SRCMOD Source0_SourceModifier_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceModifier_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
        .Source0_SourceModifier = Source0_SourceModifier_0Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode(
      ADDRMODE Source0_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource0_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceHorizontalStride(
      HORZSTRIDE Source0_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceHorizontalStride = Source0_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource0_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource0_SourceWidth(WIDTH Source0_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source0_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceWidth = Source0_SourceWidthValue;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceVerticalStride(
      VERTSTRIDE Source0_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceVerticalStride = Source0_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect30(QWORD Source0_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect30 = Source0_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_1() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_1(
      ADDRMODE Source0_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
                   .Source0_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect74(QWORD Source0_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect74 =
        Source0_SourceChannelSelect74Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
                   .Source0_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber44(
      QWORD Source0_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubregisterNumber44 =
        Source0_SourceSubregisterNumber44Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
                   .Source0_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate84(
      QWORD Source0_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate84 =
        Source0_SourceAddressImmediate84Value >> 4;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SOURCES_REG;

static_assert(8 == sizeof(EU_INSTRUCTION_SOURCES_REG));

/*****************************************************************************\
Dual source, register and immediate
\*****************************************************************************/
typedef union tagEU_INSTRUCTION_SOURCES_REG_IMM {
  struct tagCommon {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 24); // Override
    QWORD Src1Regfile : BITFIELD_RANGE(25, 26);                 // REGFILE
    QWORD Src1Srctype : BITFIELD_RANGE(27, 30);                 // SRCIMMTYPE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(31);      // Override
    QWORD Source1Immediate : BITFIELD_RANGE(32, 63);            //
  } Common;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD Source0_SourceSubRegisterNumber : BITFIELD_RANGE(0, 4); //
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);   //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD Source0_SourceAddressImmediate80 : BITFIELD_RANGE(0, 8);  // S9[8:0]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 30);    // Override
    QWORD Source0_SourceAddressImmediate9 : BITFIELD_RANGE(31, 31); // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);    // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);         // ADDRMODE
    QWORD Source0_SourceHorizontalStride : BITFIELD_RANGE(16, 17); // HORZSTRIDE
    QWORD Source0_SourceWidth : BITFIELD_RANGE(18, 20);            // WIDTH
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63);   // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD Source0_SourceChannelSelect30
        : BITFIELD_RANGE(0, 3); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(4, 14); // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);      // ADDRMODE
    QWORD Source0_SourceChannelSelect74
        : BITFIELD_RANGE(16, 19);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(20); // MBZ
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24); // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override
    QWORD Source0_SourceSubregisterNumber44
        : BITFIELD_BIT(4); // SrcSubRegNum[4:4]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);  //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3);      // Override
    QWORD Source0_SourceAddressImmediate84 : BITFIELD_RANGE(4, 8);  // S9[8:4]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  struct tagSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30);  // Override
    QWORD Source0AddressImmediate9SignBit : BITFIELD_BIT(31);    // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63); // Override
  } Source0SourceAddressingModeIsIndirect;
  struct tagSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(31);         // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63); // Override
  } Source0SourceAddressingModeIsDirect;
  DWORD RawData[2];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Src1Regfile = 0x0;
    Common.Src1Srctype = 0x0;
    RawData[1] = 0x0;
    Common.Source1Immediate = 0x0;
  }

  static tagEU_INSTRUCTION_SOURCES_REG_IMM sInit() {
    tagEU_INSTRUCTION_SOURCES_REG_IMM state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE REGFILE GetSrc1Regfile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.Src1Regfile;
  }

  __CODEGEN_INLINE void SetSrc1Regfile(REGFILE Src1RegfileValue) {
    __CODEGEN_SET_MACRO(Src1RegfileValue);
    Common.Src1Regfile = Src1RegfileValue;
  }

  __CODEGEN_INLINE int GetSrc1Srctype() {
    __CODEGEN_GET_MACRO();
    return Common.Src1Srctype;
  }

  __CODEGEN_INLINE void SetSrc1Srctype(int Src1SrctypeValue) {
    __CODEGEN_SET_MACRO(Src1SrctypeValue);
    Common.Src1Srctype = Src1SrctypeValue;
  }

  __CODEGEN_INLINE QWORD GetSource1Immediate() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Source1Immediate;
  }

  __CODEGEN_INLINE void SetSource1Immediate(QWORD Source1ImmediateValue) {
    __CODEGEN_SET_MACRO(Source1ImmediateValue);
    Common.Source1Immediate = Source1ImmediateValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubRegisterNumber(
      QWORD Source0_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubRegisterNumber = Source0_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceRegisterNumber = Source0_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate80(
      QWORD Source0_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate80 =
        Source0_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate9(
      QWORD Source0_SourceAddressImmediate9Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate9 = Source0_SourceAddressImmediate9Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_0(
      QWORD Source0_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_0Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource0_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
            .Source0_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceModifier(SRCMOD Source0_SourceModifier_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceModifier_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
        .Source0_SourceModifier = Source0_SourceModifier_0Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_0(
      ADDRMODE Source0_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource0_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceHorizontalStride(
      HORZSTRIDE Source0_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceHorizontalStride = Source0_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource0_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource0_SourceWidth(WIDTH Source0_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source0_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceWidth = Source0_SourceWidthValue;
  }
  //
  __CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceVerticalStride(
      VERTSTRIDE Source0_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceVerticalStride = Source0_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect30(QWORD Source0_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect30 = Source0_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_1() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_1(
      ADDRMODE Source0_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
                   .Source0_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect74(QWORD Source0_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect74 =
        Source0_SourceChannelSelect74Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
                   .Source0_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber44(
      QWORD Source0_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubregisterNumber44 =
        Source0_SourceSubregisterNumber44Value >> 4;
  }

  //__CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber_1() {
  //    __CODEGEN_GET_MACRO();
  //    return
  //    (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber;
  //}

  //__CODEGEN_INLINE void SetSource0_SourceRegisterNumber_1(QWORD
  //Source0_SourceRegisterNumber_1Value) {
  //    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_1Value);
  //    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber
  //    = Source0_SourceRegisterNumber_1Value;
  //}
  //
  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
                   .Source0_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate84(
      QWORD Source0_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate84 =
        Source0_SourceAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0AddressImmediate9SignBit() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        Source0SourceAddressingModeIsIndirect.Source0AddressImmediate9SignBit;
  }

  __CODEGEN_INLINE void SetSource0AddressImmediate9SignBit(
      QWORD Source0AddressImmediate9SignBitValue) {
    __CODEGEN_SET_MACRO(Source0AddressImmediate9SignBitValue);
    Source0SourceAddressingModeIsIndirect.Source0AddressImmediate9SignBit =
        Source0AddressImmediate9SignBitValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SOURCES_REG_IMM;

static_assert(8 == sizeof(EU_INSTRUCTION_SOURCES_REG_IMM));

/*****************************************************************************\
Dual source, both registers
\*****************************************************************************/
typedef union tagEU_INSTRUCTION_SOURCES_REG_REG {
  struct tagCommon {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 24); // Override
    QWORD Src1Regfile : BITFIELD_RANGE(25, 26);                 // REGFILE

    /*****************************************************************************\
    This field specifies the numeric data type of the source operand src1. The
    bits of a source operand are interpreted as the identified numeric data
    type, rather than coerced into a type implied by the operator. Depending on
    RegFile field of the source operand, there are two different encoding for
    this field. If a source is a register operand, this field follows the Source
    Register Type Encoding. If a source is an immediate operand, this field
    follows the Source Immediate Type Encoding.

    Programming Notes:
    Both source operands, src0 and src1, support immediate types, but only one
    immediate is allowed for a given instruction and it must be the last
    operand.

    Halfbyte integer vector (v) type can only be used in instructions in
    packed-word execution mode. Therefore, in a two-source instruction where
    src1 is of type :v, src0 must be of type :b, :ub, :w, or :uw.
    \*****************************************************************************/
    QWORD Src1Srctype : BITFIELD_RANGE(27, 30);                  // SRCTYPE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(31, 57); // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(58, 63);   // MBZ
  } Common;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD Source0_SourceChannelSelect30
        : BITFIELD_RANGE(0, 3); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(4, 14); // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);      // ADDRMODE
    QWORD Source0_SourceChannelSelect74
        : BITFIELD_RANGE(16, 19);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(20); // MBZ
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24); // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3); // Override
    QWORD Source0_SourceSubregisterNumber44
        : BITFIELD_BIT(4); // SrcSubRegNum[4:4]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);  //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 3);      // Override
    QWORD Source0_SourceAddressImmediate84 : BITFIELD_RANGE(4, 8);  // S9[8:4]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect {
    QWORD Source0_SourceSubRegisterNumber : BITFIELD_RANGE(0, 4); //
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(5, 12);   //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect {
    QWORD Source0_SourceAddressImmediate80 : BITFIELD_RANGE(0, 8);  // S9[8:0]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(9, 12); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 30);    // Override
    QWORD Source0_SourceAddressImmediate9 : BITFIELD_RANGE(31, 31); // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(13, 14);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 12);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(13, 14);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(15, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue;

  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 14);    // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(15);         // ADDRMODE
    QWORD Source0_SourceHorizontalStride : BITFIELD_RANGE(16, 17); // HORZSTRIDE
    QWORD Source0_SourceWidth : BITFIELD_RANGE(18, 20);            // WIDTH
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(21, 24);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(25, 63);   // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm;
  struct tagSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(31);         // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63); // Override
  } Source0SourceAddressingModeIsDirect;
  struct tagSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30);  // Override
    QWORD Source0AddressImmediate9SignBit : BITFIELD_BIT(31);    // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(32, 63); // Override
  } Source0SourceAddressingModeIsIndirect;
  struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16 {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 31); // Override
    QWORD Source1_SourceChannelSelect30
        : BITFIELD_RANGE(32, 35); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(36, 46); // Override
    QWORD Source1_SourceAddressingMode : BITFIELD_BIT(47);       // ADDRMODE
    QWORD Source1_SourceChannelSelect74
        : BITFIELD_RANGE(48, 51);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(52); // MBZ
    QWORD Source1_SourceVerticalStride : BITFIELD_RANGE(53, 56); // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(57, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 35); // Override
    QWORD Source1_SourceSubregisterNumber44
        : BITFIELD_BIT(36); // SrcSubRegNum[4:4]
    QWORD Source1_SourceRegisterNumber : BITFIELD_RANGE(37, 44); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(45, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 35);      // Override
    QWORD Source1_SourceAddressImmediate84 : BITFIELD_RANGE(36, 40); // S9[8:4]
    QWORD Source1_AddressSubregisterNumber : BITFIELD_RANGE(41, 44); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(45, 63);     // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 44);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(45, 46);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(47, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 44);  // Override
    QWORD Source1_SourceModifier : BITFIELD_RANGE(45, 46);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(47, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 31);     // Override
    QWORD Source1_SourceSubRegisterNumber : BITFIELD_RANGE(32, 36); //
    QWORD Source1_SourceRegisterNumber : BITFIELD_RANGE(37, 44);    //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(45, 63);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 31);      // Override
    QWORD Source1_SourceAddressImmediate80 : BITFIELD_RANGE(32, 40); // S9[8:0]
    QWORD Source1_AddressSubregisterNumber : BITFIELD_RANGE(41, 44); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(45, 56);     // Override
    QWORD Source1_SourceAddressImmediate9 : BITFIELD_RANGE(57, 57);  // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(58, 63);     // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 44);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(45, 46);   // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(47, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 44);  // Override
    QWORD Source1_SourceModifier : BITFIELD_RANGE(45, 46);       // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(47, 63); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue;
  struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1 {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 46);    // Override
    QWORD Source1_SourceAddressingMode : BITFIELD_BIT(47);         // ADDRMODE
    QWORD Source1_SourceHorizontalStride : BITFIELD_RANGE(48, 49); // HORZSTRIDE
    QWORD Source1_SourceWidth : BITFIELD_RANGE(50, 52);            // WIDTH
    QWORD Source1_SourceVerticalStride : BITFIELD_RANGE(53, 56);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(57, 63);   // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1;
  struct tagSource1SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 56);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(57);         // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(58, 63); // Override
  } Source1SourceAddressingModeIsDirect;
  struct tagSource1SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 56);  // Override
    QWORD Source1AddressImmediate9SignBit : BITFIELD_BIT(57);    // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(58, 63); // Override
  } Source1SourceAddressingModeIsIndirect;
  QWORD RawData[1];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Src1Regfile = 0x0;
    Common.Src1Srctype = 0x0;
  }

  static tagEU_INSTRUCTION_SOURCES_REG_REG sInit() {
    tagEU_INSTRUCTION_SOURCES_REG_REG state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  //__CODEGEN_INLINE QWORD& GetDWord(UINT const index) {
  //    return RawData[index];
  //}

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE REGFILE GetSrc1Regfile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.Src1Regfile;
  }

  __CODEGEN_INLINE void SetSrc1Regfile(REGFILE Src1RegfileValue) {
    __CODEGEN_SET_MACRO(Src1RegfileValue);
    Common.Src1Regfile = Src1RegfileValue;
  }

  __CODEGEN_INLINE int GetSrc1Srctype() {
    __CODEGEN_GET_MACRO();
    return Common.Src1Srctype;
  }

  __CODEGEN_INLINE void SetSrc1Srctype(int Src1SrctypeValue) {
    __CODEGEN_SET_MACRO(Src1SrctypeValue);
    Common.Src1Srctype = Src1SrctypeValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect30(QWORD Source0_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect30 = Source0_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_0(
      ADDRMODE Source0_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
                   .Source0_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect74(QWORD Source0_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceChannelSelect74 =
        Source0_SourceChannelSelect74Value >> 4;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceVerticalStride(
      VERTSTRIDE Source0_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceVerticalStride = Source0_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
                   .Source0_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber44(
      QWORD Source0_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubregisterNumber44 =
        Source0_SourceSubregisterNumber44Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceRegisterNumber = Source0_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
                   .Source0_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate84(
      QWORD Source0_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate84 =
        Source0_SourceAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_0(
      QWORD Source0_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_0Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource0_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
            .Source0_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceModifier(SRCMOD Source0_SourceModifier_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceModifier_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndPropertySourceModifierIsTrue
        .Source0_SourceModifier = Source0_SourceModifier_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
            .Source0_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubRegisterNumber(
      QWORD Source0_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubRegisterNumber = Source0_SourceSubRegisterNumberValue;
  }

  //__CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber_1() {
  //    __CODEGEN_GET_MACRO();
  //    return
  //    (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber;
  //}
  //
  //__CODEGEN_INLINE void SetSource0_SourceRegisterNumber_1(QWORD
  //Source0_SourceRegisterNumber_1Value) {
  //    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_1Value);
  //    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber
  //    = Source0_SourceRegisterNumber_1Value;
  //}

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
            .Source0_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate80(
      QWORD Source0_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate80 =
        Source0_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate9(
      QWORD Source0_SourceAddressImmediate9Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImmAndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate9 = Source0_SourceAddressImmediate9Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode(
      ADDRMODE Source0_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource0_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceHorizontalStride(
      HORZSTRIDE Source0_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceHorizontalStride = Source0_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource0_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
            .Source0_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource0_SourceWidth(WIDTH Source0_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source0_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm
        .Source0_SourceWidth = Source0_SourceWidthValue;
  }

  //__CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
  //    __CODEGEN_GET_MACRO();
  //    return
  //    (VERTSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm.Source0_SourceVerticalStride;
  //}
  //
  //__CODEGEN_INLINE void SetSource0_SourceVerticalStride(VERTSTRIDE
  //Source0_SourceVerticalStrideValue) {
  //    __CODEGEN_SET_MACRO(Source0_SourceVerticalStrideValue);
  //    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndStructureEu_Instruction_Operand_ControlsSrc0RegfileNotImm.Source0_SourceVerticalStride
  //    = Source0_SourceVerticalStrideValue;
  //}

  __CODEGEN_INLINE QWORD GetSource0AddressImmediate9SignBit() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        Source0SourceAddressingModeIsIndirect.Source0AddressImmediate9SignBit;
  }

  __CODEGEN_INLINE void SetSource0AddressImmediate9SignBit(
      QWORD Source0AddressImmediate9SignBitValue) {
    __CODEGEN_SET_MACRO(Source0AddressImmediate9SignBitValue);
    Source0SourceAddressingModeIsIndirect.Source0AddressImmediate9SignBit =
        Source0AddressImmediate9SignBitValue;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceChannelSelect30(QWORD Source1_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source1_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceChannelSelect30 = Source1_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource1_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressingMode_0(
      ADDRMODE Source1_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source1_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceAddressingMode = Source1_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
               .Source1_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceChannelSelect74(QWORD Source1_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source1_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceChannelSelect74 =
        Source1_SourceChannelSelect74Value >> 4;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource1_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource1_SourceVerticalStride(
      VERTSTRIDE Source1_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source1_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source1_SourceVerticalStride = Source1_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect
                   .Source1_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource1_SourceSubregisterNumber44(
      QWORD Source1_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source1_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect
        .Source1_SourceSubregisterNumber44 =
        Source1_SourceSubregisterNumber44Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect
            .Source1_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceRegisterNumber(QWORD Source1_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source1_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsDirect
        .Source1_SourceRegisterNumber = Source1_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect
                   .Source1_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressImmediate84(
      QWORD Source1_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source1_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect
        .Source1_SourceAddressImmediate84 =
        Source1_SourceAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource1_AddressSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect
            .Source1_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource1_AddressSubregisterNumber(
      QWORD Source1_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source1_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource1SourceAddressingModeIsIndirect
        .Source1_AddressSubregisterNumber =
        Source1_AddressSubregisterNumber_0Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource1_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue
            .Source1_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceModifier(SRCMOD Source1_SourceModifier_0Value) {
    __CODEGEN_SET_MACRO(Source1_SourceModifier_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue
        .Source1_SourceModifier = Source1_SourceModifier_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect
            .Source1_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource1_SourceSubRegisterNumber(
      QWORD Source1_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect
        .Source1_SourceSubRegisterNumber = Source1_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceRegisterNumber_1() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect
            .Source1_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceRegisterNumber_1(QWORD Source1_SourceRegisterNumber_1Value) {
    __CODEGEN_SET_MACRO(Source1_SourceRegisterNumber_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsDirect
        .Source1_SourceRegisterNumber = Source1_SourceRegisterNumber_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect
            .Source1_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressImmediate80(
      QWORD Source1_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source1_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect
        .Source1_SourceAddressImmediate80 =
        Source1_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressImmediate9(
      QWORD Source1_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source1_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect
        .Source1_SourceAddressImmediate9 =
        Source1_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_AddressSubregisterNumber_1() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect
            .Source1_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource1_AddressSubregisterNumber_1(
      QWORD Source1_AddressSubregisterNumber_1Value) {
    __CODEGEN_SET_MACRO(Source1_AddressSubregisterNumber_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource1SourceAddressingModeIsIndirect
        .Source1_AddressSubregisterNumber =
        Source1_AddressSubregisterNumber_1Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource1_SourceModifier_1() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue
            .Source1_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceModifier_1(SRCMOD Source1_SourceModifier_1Value) {
    __CODEGEN_SET_MACRO(Source1_SourceModifier_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue
        .Source1_SourceModifier = Source1_SourceModifier_1Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource1_SourceAddressingMode() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressingMode(
      ADDRMODE Source1_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source1_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceAddressingMode = Source1_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource1_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource1_SourceHorizontalStride(
      HORZSTRIDE Source1_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source1_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceHorizontalStride = Source1_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource1_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource1_SourceWidth(WIDTH Source1_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source1_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1.Source1_SourceWidth =
        Source1_SourceWidthValue;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource1_SourceVerticalStride_1() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource1_SourceVerticalStride_1(
      VERTSTRIDE Source1_SourceVerticalStride_1Value) {
    __CODEGEN_SET_MACRO(Source1_SourceVerticalStride_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source1_SourceVerticalStride = Source1_SourceVerticalStride_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource1AddressImmediate9SignBit() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        Source1SourceAddressingModeIsIndirect.Source1AddressImmediate9SignBit;
  }

  __CODEGEN_INLINE void SetSource1AddressImmediate9SignBit(
      QWORD Source1AddressImmediate9SignBitValue) {
    __CODEGEN_SET_MACRO(Source1AddressImmediate9SignBitValue);
    Source1SourceAddressingModeIsIndirect.Source1AddressImmediate9SignBit =
        Source1AddressImmediate9SignBitValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SOURCES_REG_REG;

static_assert(8 == sizeof(EU_INSTRUCTION_SOURCES_REG_REG));

typedef union tagEU_INSTRUCTION_BASIC_ONE_SRC {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControls;
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
  } Common;
  struct tagOperandControlsSrc0RegfileIsImm {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    EU_INSTRUCTION_SOURCES_IMM32 Immsource;
  } OperandControlsSrc0RegfileIsImm;
  struct tagOperandControlsSrc0RegfileNotImm {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    EU_INSTRUCTION_SOURCES_REG Regsource;
  } OperandControlsSrc0RegfileNotImm;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.OperandControls.Init();
    RawData[2] = 0x0;
    RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_BASIC_ONE_SRC sInit() {
    tagEU_INSTRUCTION_BASIC_ONE_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControls() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControls;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_IMM32 &GetImmsource() {
    __CODEGEN_GET_MACRO();
    return OperandControlsSrc0RegfileIsImm.Immsource;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG &GetRegsource() {
    __CODEGEN_GET_MACRO();
    return OperandControlsSrc0RegfileNotImm.Regsource;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BASIC_ONE_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_BASIC_ONE_SRC));

//////

typedef struct tagEU_INSTRUCTION_IMM64_SRC {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      EU_INSTRUCTION_HEADER Header;
      EU_INSTRUCTION_OPERAND_CONTROLS OperandControls;
      QWORD Source : BITFIELD_RANGE(64, 127); //
    } Common;
    DWORD RawData[4];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.Common.Header.Init();
    TheStructure.RawData[1] = 0x0;
    TheStructure.Common.OperandControls.Init();
    TheStructure.RawData[2] = 0x0;
    TheStructure.RawData[3] = 0x0;
    TheStructure.Common.Source = 0x0;
  }

  static tagEU_INSTRUCTION_IMM64_SRC sInit() {
    tagEU_INSTRUCTION_IMM64_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.Header;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControls() {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.OperandControls;
  }

  __CODEGEN_INLINE QWORD GetSource() const {
    __CODEGEN_GET_MACRO();
    return (QWORD)TheStructure.Common.Source;
  }

  __CODEGEN_INLINE void SetSource(QWORD SourceValue) {
    __CODEGEN_SET_MACRO(SourceValue);
    TheStructure.Common.Source = SourceValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_IMM64_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_IMM64_SRC));

typedef union tagEU_INSTRUCTION_BASIC_TWO_SRC {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControls;
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
  } Common;

  struct tagImmsourceSrc1RegfileIsImm {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    EU_INSTRUCTION_SOURCES_REG_IMM Immsource;
  } ImmsourceSrc1RegfileIsImm;

  struct tagRegsourceSrc1RegfileNotImm {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    EU_INSTRUCTION_SOURCES_REG_REG Regsource;
  } RegsourceSrc1RegfileNotImm;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.OperandControls.Init();
    RawData[2] = 0x0;
    RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_BASIC_TWO_SRC sInit() {
    tagEU_INSTRUCTION_BASIC_TWO_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControls() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControls;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG_IMM &GetImmsource() {
    __CODEGEN_GET_MACRO();
    return ImmsourceSrc1RegfileIsImm.Immsource;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG_REG &GetRegsource() {
    __CODEGEN_GET_MACRO();
    return RegsourceSrc1RegfileNotImm.Regsource;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BASIC_TWO_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_BASIC_TWO_SRC));

typedef union tagEU_INSTRUCTION_BASIC_THREE_SRC {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;

    /*****************************************************************************\
    This field contains the flag subregister number for instructions with a
    non-zero Conditional Modifier.
    \*****************************************************************************/
    DWORD FlagSubregisterNumber : BITFIELD_BIT(32); //

    /*****************************************************************************\
    This field contains the flag register number for instructions with a
    non-zero Conditional Modifier.
    \*****************************************************************************/
    DWORD FlagRegisterNumber : BITFIELD_BIT(33); //

    /*****************************************************************************\
    (formerly WECtrl/Write Enable Control). This flag disables the normal write
    enables; it should normally be 0.

    Programming Notes:
    MaskCtrl = NoMask also skips the check for PcIP[n] == ExIP before enabling a
    channel, as described in the Evaluate Write Enable section.
    \*****************************************************************************/
    DWORD Maskctrl : BITFIELD_BIT(34);                           // MASKCTRL
    DWORD Source2Type : BITFIELD_BIT(35);                        // U1
    DWORD Source1Type : BITFIELD_BIT(36);                        // U1
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(37, 42); // Override
    DWORD SourceDataType : BITFIELD_RANGE(43, 45);      // SOURCE_DATA_TYPE
    DWORD DestinationDataType : BITFIELD_RANGE(46, 48); // DESTINATION_DATA_TYPE

    /*****************************************************************************\
    Four channel enables are defined for controlling which channels are written
    into the destination region. These channel mask bits are applied in a
    modulo-four manner to all ExecSize channels. There is 1-bit Channel Enable
    for each channel within the group of 4. If the bit is cleared, the write for
    the corresponding channel is disabled. If the bit is set, the write is
    enabled. Mnemonics for the bit being set for the group of 4 are  -x -,  -y
    -,  -z -, and  -w -, respectively, where  -x - corresponds to Channel 0 in
    the group and  -w - corresponds to channel 3 in the group
    \*****************************************************************************/
    DWORD DestinationChannelEnable : BITFIELD_RANGE(49, 52);     // ChanEn[4]
    DWORD DestinationSubregisterNumber : BITFIELD_RANGE(53, 55); //
    DWORD DestinationRegisterNumber_DestinationRegisterNumber
        : BITFIELD_RANGE(0, 7);                              //
    QWORD Source0_SourceReplicateControl : BITFIELD_BIT(64); // REPCTRL
    QWORD Source0_SourceSwizzle : BITFIELD_RANGE(65, 72);    // ChanSel[4]
    QWORD Source0_SourceSubregisterNumber42
        : BITFIELD_RANGE(73, 75); // SrcSubRegNum[4:2]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(76, 83); //
    QWORD Source0_SourceSubregisterNumber1
        : BITFIELD_BIT(84);                                  // SrcSubRegNum[1]
    QWORD Source1_SourceReplicateControl : BITFIELD_BIT(85); // REPCTRL
    QWORD Source1_SourceSwizzle : BITFIELD_RANGE(86, 93);    // ChanSel[4]
    QWORD Source1_SourceSubregisterNumber42
        : BITFIELD_RANGE(94, 96); // SrcSubRegNum[4:2]
    QWORD Source1_SourceRegisterNumber : BITFIELD_RANGE(97, 104); //
    QWORD Source1_SourceSubregisterNumber1
        : BITFIELD_BIT(105);                                  // SrcSubRegNum[1]
    QWORD Source2_SourceReplicateControl : BITFIELD_BIT(106); // REPCTRL
    QWORD Source2_SourceSwizzle : BITFIELD_RANGE(107, 114);   // ChanSel[4]
    QWORD Source2_SourceSubregisterNumber42
        : BITFIELD_RANGE(115, 117); // SrcSubRegNum[4:2]
    QWORD Source2_SourceRegisterNumber : BITFIELD_RANGE(118, 125); //
    QWORD Source2_SourceSubregisterNumber1
        : BITFIELD_BIT(126);                              // SrcSubRegNum[1]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(127); // MBZ
  } Common;
  struct tagPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 36);   // Override
    QWORD Source0Modifier : BITFIELD_RANGE(37, 38);               // SRCMOD
    QWORD Source1Modifier : BITFIELD_RANGE(39, 40);               // SRCMOD
    QWORD Source2Modifier : BITFIELD_RANGE(41, 42);               // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override

  } PropertySourceModifierIsTrue;
  struct tagPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 36);   // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(37, 42);    // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } PropertySourceModifierIsFalse;
  DWORD RawData[4];

  typedef enum tagSOURCE_DATA_TYPE {
    SOURCE_DATA_TYPE_F = 0x0,  // single precision Float (32-bit)
    SOURCE_DATA_TYPE_D = 0x1,  // signed Doubleword integer
    SOURCE_DATA_TYPE_UD = 0x2, // Unsigned Doubleword integer
    SOURCE_DATA_TYPE_DF = 0x3, // Double precision Float (64-bit)
    SOURCE_DATA_TYPE_HF = 0x4, // Half Float (16-bit)
  } SOURCE_DATA_TYPE;

  typedef enum tagDESTINATION_DATA_TYPE {
    DESTINATION_DATA_TYPE_F = 0x0,  // single precision Float (32-bit)
    DESTINATION_DATA_TYPE_D = 0x1,  // signed Doubleword integer
    DESTINATION_DATA_TYPE_UD = 0x2, // Unsigned Doubleword integer
    DESTINATION_DATA_TYPE_DF = 0x3, // Double precision Float (64-bit)
    DESTINATION_DATA_TYPE_HF = 0x4, // Half Float (16-bit)
  } DESTINATION_DATA_TYPE;

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.FlagSubregisterNumber = 0x0;
    Common.FlagRegisterNumber = 0x0;
    Common.Maskctrl = MASKCTRL_NORMAL;
    Common.Source2Type = 0x0;
    Common.Source1Type = 0x0;
    Common.SourceDataType = SOURCE_DATA_TYPE_F;
    Common.DestinationDataType = DESTINATION_DATA_TYPE_F;
    Common.DestinationChannelEnable = 0x0;
    Common.DestinationSubregisterNumber = 0x0;
    RawData[2] = 0x0;
    Common.DestinationRegisterNumber_DestinationRegisterNumber = 0x0;
    RawData[3] = 0x0;
    Common.Source0_SourceReplicateControl = 0x0;
    Common.Source0_SourceSwizzle = 0x0;
    Common.Source0_SourceSubregisterNumber42 = 0x0;
    Common.Source0_SourceRegisterNumber = 0x0;
    Common.Source0_SourceSubregisterNumber1 = 0x0;
    Common.Source1_SourceReplicateControl = 0x0;
    Common.Source1_SourceSwizzle = 0x0;
    // RawData[4]                                                 = 0x0;
    Common.Source1_SourceSubregisterNumber42 = 0x0;
    Common.Source1_SourceRegisterNumber = 0x0;
    Common.Source1_SourceSubregisterNumber1 = 0x0;
    Common.Source2_SourceReplicateControl = 0x0;
    Common.Source2_SourceSwizzle = 0x0;
    Common.Source2_SourceSubregisterNumber42 = 0x0;
    Common.Source2_SourceRegisterNumber = 0x0;
    Common.Source2_SourceSubregisterNumber1 = 0x0;
  }

  static tagEU_INSTRUCTION_BASIC_THREE_SRC sInit() {
    tagEU_INSTRUCTION_BASIC_THREE_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS

  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE DWORD GetFlagRegisterNumberSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.FlagSubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetFlagRegisterNumberSubregisterNumber(DWORD FlagSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagSubregisterNumberValue);
    Common.FlagSubregisterNumber = FlagSubregisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetFlagRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.FlagRegisterNumber;
  }

  __CODEGEN_INLINE void SetFlagRegisterNumber(DWORD FlagRegisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagRegisterNumberValue);
    Common.FlagRegisterNumber = FlagRegisterNumberValue;
  }

  __CODEGEN_INLINE MASKCTRL GetMaskctrl() {
    __CODEGEN_GET_MACRO();
    return (MASKCTRL)Common.Maskctrl;
  }

  __CODEGEN_INLINE void SetMaskctrl(MASKCTRL MaskctrlValue) {
    __CODEGEN_SET_MACRO(MaskctrlValue);
    Common.Maskctrl = MaskctrlValue;
  }

  __CODEGEN_INLINE DWORD GetSource2Type() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source2Type;
  }

  __CODEGEN_INLINE void SetSource2Type(DWORD Source2TypeValue) {
    __CODEGEN_SET_MACRO(Source2TypeValue);
    Common.Source2Type = Source2TypeValue;
  }

  __CODEGEN_INLINE DWORD GetSource1Type() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source1Type;
  }

  __CODEGEN_INLINE void SetSource1Type(DWORD Source1TypeValue) {
    __CODEGEN_SET_MACRO(Source1TypeValue);
    Common.Source1Type = Source1TypeValue;
  }

  __CODEGEN_INLINE SOURCE_DATA_TYPE GetSourceDataType() {
    __CODEGEN_GET_MACRO();
    return (SOURCE_DATA_TYPE)Common.SourceDataType;
  }

  __CODEGEN_INLINE void
  SetSourceDataType(SOURCE_DATA_TYPE SourceDataTypeValue) {
    __CODEGEN_SET_MACRO(SourceDataTypeValue);
    Common.SourceDataType = SourceDataTypeValue;
  }

  __CODEGEN_INLINE DESTINATION_DATA_TYPE GetDestinationDataType() {
    __CODEGEN_GET_MACRO();
    return (DESTINATION_DATA_TYPE)Common.DestinationDataType;
  }

  __CODEGEN_INLINE void
  SetDestinationDataType(DESTINATION_DATA_TYPE DestinationDataTypeValue) {
    __CODEGEN_SET_MACRO(DestinationDataTypeValue);
    Common.DestinationDataType = DestinationDataTypeValue;
  }

  __CODEGEN_INLINE DWORD GetDestinationChannelEnable() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.DestinationChannelEnable;
  }

  __CODEGEN_INLINE void
  SetDestinationChannelEnable(DWORD DestinationChannelEnableValue) {
    __CODEGEN_SET_MACRO(DestinationChannelEnableValue);
    Common.DestinationChannelEnable = DestinationChannelEnableValue;
  }

  __CODEGEN_INLINE DWORD GetDestinationSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.DestinationSubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetDestinationSubregisterNumber(DWORD DestinationSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationSubregisterNumberValue);
    Common.DestinationSubregisterNumber = DestinationSubregisterNumberValue;
  }

  __CODEGEN_INLINE DWORD
  GetDestinationRegisterNumber_DestinationRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.DestinationRegisterNumber_DestinationRegisterNumber;
  }

  __CODEGEN_INLINE void SetDestinationRegisterNumber_DestinationRegisterNumber(
      DWORD DestinationRegisterNumber_DestinationRegisterNumberValue) {
    __CODEGEN_SET_MACRO(
        DestinationRegisterNumber_DestinationRegisterNumberValue);
    Common.DestinationRegisterNumber_DestinationRegisterNumber =
        DestinationRegisterNumber_DestinationRegisterNumberValue;
  }

  __CODEGEN_INLINE REPCTRL GetSource0_SourceReplicateControl() {
    __CODEGEN_GET_MACRO();
    return (REPCTRL)Common.Source0_SourceReplicateControl;
  }

  __CODEGEN_INLINE void SetSource0_SourceReplicateControl(
      REPCTRL Source0_SourceReplicateControlValue) {
    __CODEGEN_SET_MACRO(Source0_SourceReplicateControlValue);
    Common.Source0_SourceReplicateControl = Source0_SourceReplicateControlValue;
  }

  __CODEGEN_INLINE DWORD GetSource0_SourceSwizzle() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source0_SourceSwizzle;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceSwizzle(DWORD Source0_SourceSwizzleValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSwizzleValue);
    Common.Source0_SourceSwizzle = Source0_SourceSwizzleValue;
  }

  __CODEGEN_INLINE DWORD GetSource0_SourceSubregisterNumber42() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source0_SourceSubregisterNumber42 << 2;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber42(
      DWORD Source0_SourceSubregisterNumber42Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber42Value);
    Common.Source0_SourceSubregisterNumber42 =
        Source0_SourceSubregisterNumber42Value >> 2;
  }

  __CODEGEN_INLINE DWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(DWORD Source0_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumberValue);
    Common.Source0_SourceRegisterNumber = Source0_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource0_SourceSubregisterNumber1() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source0_SourceSubregisterNumber1;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber1(
      DWORD Source0_SourceSubregisterNumber1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber1Value);
    Common.Source0_SourceSubregisterNumber1 =
        Source0_SourceSubregisterNumber1Value;
  }

  __CODEGEN_INLINE REPCTRL GetSource1_SourceReplicateControl() {
    __CODEGEN_GET_MACRO();
    return (REPCTRL)Common.Source1_SourceReplicateControl;
  }

  __CODEGEN_INLINE void SetSource1_SourceReplicateControl(
      REPCTRL Source1_SourceReplicateControlValue) {
    __CODEGEN_SET_MACRO(Source1_SourceReplicateControlValue);
    Common.Source1_SourceReplicateControl = Source1_SourceReplicateControlValue;
  }

  __CODEGEN_INLINE DWORD GetSource1_SourceSwizzle() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source1_SourceSwizzle;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceSwizzle(DWORD Source1_SourceSwizzleValue) {
    __CODEGEN_SET_MACRO(Source1_SourceSwizzleValue);
    Common.Source1_SourceSwizzle = Source1_SourceSwizzleValue;
  }

  __CODEGEN_INLINE DWORD GetSource1_SourceSubregisterNumber42() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source1_SourceSubregisterNumber42 << 2;
  }

  __CODEGEN_INLINE void SetSource1_SourceSubregisterNumber42(
      DWORD Source1_SourceSubregisterNumber42Value) {
    __CODEGEN_SET_MACRO(Source1_SourceSubregisterNumber42Value);
    Common.Source1_SourceSubregisterNumber42 =
        Source1_SourceSubregisterNumber42Value >> 2;
  }

  __CODEGEN_INLINE DWORD GetSource1_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source1_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceRegisterNumber(DWORD Source1_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1_SourceRegisterNumberValue);
    Common.Source1_SourceRegisterNumber = Source1_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource1_SourceSubregisterNumber1() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source1_SourceSubregisterNumber1;
  }

  __CODEGEN_INLINE void SetSource1_SourceSubregisterNumber1(
      DWORD Source1_SourceSubregisterNumber1Value) {
    __CODEGEN_SET_MACRO(Source1_SourceSubregisterNumber1Value);
    Common.Source1_SourceSubregisterNumber1 =
        Source1_SourceSubregisterNumber1Value;
  }

  __CODEGEN_INLINE REPCTRL GetSource2_SourceReplicateControl() {
    __CODEGEN_GET_MACRO();
    return (REPCTRL)Common.Source2_SourceReplicateControl;
  }

  __CODEGEN_INLINE void SetSource2_SourceReplicateControl(
      REPCTRL Source2_SourceReplicateControlValue) {
    __CODEGEN_SET_MACRO(Source2_SourceReplicateControlValue);
    Common.Source2_SourceReplicateControl = Source2_SourceReplicateControlValue;
  }

  __CODEGEN_INLINE DWORD GetSource2_SourceSwizzle() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source2_SourceSwizzle;
  }

  __CODEGEN_INLINE void
  SetSource2_SourceSwizzle(DWORD Source2_SourceSwizzleValue) {
    __CODEGEN_SET_MACRO(Source2_SourceSwizzleValue);
    Common.Source2_SourceSwizzle = Source2_SourceSwizzleValue;
  }

  __CODEGEN_INLINE DWORD GetSource2_SourceSubregisterNumber42() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source2_SourceSubregisterNumber42 << 2;
  }

  __CODEGEN_INLINE void SetSource2_SourceSubregisterNumber42(
      DWORD Source2_SourceSubregisterNumber42Value) {
    __CODEGEN_SET_MACRO(Source2_SourceSubregisterNumber42Value);
    Common.Source2_SourceSubregisterNumber42 =
        Source2_SourceSubregisterNumber42Value >> 2;
  }

  __CODEGEN_INLINE DWORD GetSource2_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source2_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource2_SourceRegisterNumber(DWORD Source2_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source2_SourceRegisterNumberValue);
    Common.Source2_SourceRegisterNumber = Source2_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource2_SourceSubregisterNumber1() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Source2_SourceSubregisterNumber1;
  }

  __CODEGEN_INLINE void SetSource2_SourceSubregisterNumber1(
      DWORD Source2_SourceSubregisterNumber1Value) {
    __CODEGEN_SET_MACRO(Source2_SourceSubregisterNumber1Value);
    Common.Source2_SourceSubregisterNumber1 =
        Source2_SourceSubregisterNumber1Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource0Modifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)PropertySourceModifierIsTrue.Source0Modifier;
  }

  __CODEGEN_INLINE void SetSource0_SourceModifier(SRCMOD Source0ModifierValue) {
    __CODEGEN_SET_MACRO(Source0ModifierValue);
    PropertySourceModifierIsTrue.Source0Modifier = Source0ModifierValue;
  }

  __CODEGEN_INLINE SRCMOD GetSource1Modifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)PropertySourceModifierIsTrue.Source1Modifier;
  }

  __CODEGEN_INLINE void SetSource1_SourceModifier(SRCMOD Source1ModifierValue) {
    __CODEGEN_SET_MACRO(Source1ModifierValue);
    PropertySourceModifierIsTrue.Source1Modifier = Source1ModifierValue;
  }

  __CODEGEN_INLINE SRCMOD GetSource2Modifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)PropertySourceModifierIsTrue.Source2Modifier;
  }

  __CODEGEN_INLINE void SetSource2_SourceModifier(SRCMOD Source2ModifierValue) {
    __CODEGEN_SET_MACRO(Source2ModifierValue);
    PropertySourceModifierIsTrue.Source2Modifier = Source2ModifierValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BASIC_THREE_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_BASIC_THREE_SRC));

typedef struct tagEU_INSTRUCTION_ALIGN1_THREE_SRC {
  __CODEGEN_ACCESS_SPECIFIER_DEFINITION
  union tagTheStructure {
    struct tagCommon {
      EU_INSTRUCTION_HEADER Header;

      /*****************************************************************************\
      This field contains the flag subregister number for instructions with a
      non-zero Conditional Modifier.
      \*****************************************************************************/
      DWORD FlagSubregisterNumber : BITFIELD_BIT(32); //

      /*****************************************************************************\
      This field contains the flag register number for instructions with a
      non-zero Conditional Modifier.
      \*****************************************************************************/
      DWORD FlagRegisterNumber : BITFIELD_BIT(33); //

      /*****************************************************************************\
      This flag disables the normal write enables; it should normally be 0.

      Programming Notes:
      MaskCtrl = NoMask also skips the check for PcIP[n] == ExIP before enabling
      a channel, as described in the Evaluate Write Enable section.
      \*****************************************************************************/
      DWORD Maskctrl : BITFIELD_BIT(34); // MASKCTRL

      /*****************************************************************************\
      This field defines common data type for all sources and destination
      operands.
      \*****************************************************************************/
      DWORD ExecutionDatatype : BITFIELD_BIT(35); // EXECUTION_DATATYPE

      /*****************************************************************************\
      Selects destination register file.
      \*****************************************************************************/
      DWORD DestinationRegisterFile
          : BITFIELD_BIT(36); // DESTINATION_REGISTER_FILE
      DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(37, 42); // Override

      /*****************************************************************************\
      Selects source 0 register file.
      \*****************************************************************************/
      DWORD Source0RegisterFile : BITFIELD_BIT(43); // SOURCE_0_REGISTER_FILE

      /*****************************************************************************\
      Selects source 1 register file.
      \*****************************************************************************/
      DWORD Source1RegisterFile : BITFIELD_BIT(44); // SOURCE_1_REGISTER_FILE

      /*****************************************************************************\
      Selects source 2 register file.
      \*****************************************************************************/
      DWORD Source2RegisterFile : BITFIELD_BIT(45); // SOURCE_2_REGISTER_FILE

      /*****************************************************************************\
      Selects destination datatype.
      \*****************************************************************************/
      DWORD DestinationDatatype
          : BITFIELD_RANGE(46, 48); // TernaryAlign1DataType

      /*****************************************************************************\
      Selects destination horizontal stride.Destination horizontal stride is
      required for striding based on execution size or packing the destination
      datatype.
      \*****************************************************************************/
      DWORD DestinationHorizontalStride
          : BITFIELD_BIT(49); // DESTINATION_HORIZONTAL_STRIDE
      DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(50, 51); // MBZ
      DWORD DestinationAccRegLowBits : BITFIELD_RANGE(52, 53); // AccRegSel[0:1]
      DWORD DestinationSubregisterNumber
          : BITFIELD_RANGE(54, 55); // (DstSubRegNum[4:3])
      DWORD DestinationRegisterNumber : BITFIELD_RANGE(56, 63); // DstRegNum

      /*****************************************************************************\
      Selects source 0 datatype.
      \*****************************************************************************/
      QWORD Source0Datatype : BITFIELD_RANGE(64, 66); // TernaryAlign1DataType
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(67, 83); // Override
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(84);         // MBZ

      /*****************************************************************************\
      Selects source 1 datatype.
      \*****************************************************************************/
      QWORD Source1Datatype : BITFIELD_RANGE(85, 87); // TernaryAlign1DataType
      QWORD Source1VerticalStride
          : BITFIELD_RANGE(88, 89); // TernaryAlign1VertStride
      QWORD Source1HorizontalStride
          : BITFIELD_RANGE(90, 91); // TernaryAlign1HorzStride
      QWORD Source1SubregisterNumber_SourceSubRegisterNumber
          : BITFIELD_RANGE(92, 96); //
      QWORD Source1RegisterNumber_SourceRegisterNumber
          : BITFIELD_RANGE(97, 104);                        //
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(105); // MBZ

      /*****************************************************************************\
      Selects source 2 datatype.
      \*****************************************************************************/
      QWORD Source2Datatype : BITFIELD_RANGE(106, 108); // TernaryAlign1DataType
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(109, 125); // Override
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(126, 127);   // MBZ
    } Common;

    struct tagPropertySourceModifierIsTrue {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 36);   // Override
      QWORD Source0Modifier : BITFIELD_RANGE(37, 38);               // SRCMOD
      QWORD Source1Modifier : BITFIELD_RANGE(39, 40);               // SRCMOD
      QWORD Source2Modifier : BITFIELD_RANGE(41, 42);               // SRCMOD
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 63);  // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
      // QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 127); //
      // Override
    } PropertySourceModifierIsTrue;
    struct tagPropertySourceModifierIsFalse {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 36);   // Override
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(37, 42);    // MBZ
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 63);  // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
      // QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 127); //
      // Override
    } PropertySourceModifierIsFalse;
    struct tagStructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);  // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 66); // Override
      QWORD Source0VerticalStride
          : BITFIELD_RANGE(67, 68); // TernaryAlign1VertStride
      QWORD Source0HorizontalStride
          : BITFIELD_RANGE(69, 70); // TernaryAlign1HorzStride
      QWORD Source0SubregisterNumber_SourceSubRegisterNumber
          : BITFIELD_RANGE(71, 75); //
      QWORD Source0RegisterNumber_SourceRegisterNumber
          : BITFIELD_RANGE(76, 83);                                 //
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(84, 127); // Override
    } StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf;
    struct tagStructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsImm {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 66);  // Override
      QWORD Source0ImmediateValue : BITFIELD_RANGE(67, 82);         //
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(83);          // MBZ
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(84, 127); // Override
    } StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsImm;
    struct tagStructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 108); // Override
      // QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 108); //
      // Override
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(109, 110); // MBZ
      QWORD Source2HorizontalStride
          : BITFIELD_RANGE(111, 112); // TernaryAlign1HorzStride
      QWORD Source2SubregisterNumber_SourceSubRegisterNumber
          : BITFIELD_RANGE(113, 117); //
      QWORD Source2RegisterNumber_SourceRegisterNumber
          : BITFIELD_RANGE(118, 125);                                //
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(126, 127); // Override
    } StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf;
    struct tagStructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsImm {
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 108); // Override
      // QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 108); //
      // Override
      QWORD Source2ImmediateValue : BITFIELD_RANGE(109, 124);        //
      QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(125);          // MBZ
      QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(126, 127); // Override
    } StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsImm;
    DWORD RawData[4];
  } TheStructure;

  __CODEGEN_ACCESS_SPECIFIER_METHODS

  typedef enum tagEXECUTION_DATATYPE {
    EXECUTION_DATATYPE_INTEGER = 0x0, // Integer datatypes.
    EXECUTION_DATATYPE_FLOAT = 0x1,   // Floating point datatype.
  } EXECUTION_DATATYPE;

  typedef enum tagDESTINATION_REGISTER_FILE {
    DESTINATION_REGISTER_FILE_GRF =
        0x0, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects General Register File
             // as Destination</span>.
    DESTINATION_REGISTER_FILE_ARF =
        0x1, // Selects Architectural Register File as Destination. Only
             // Accumulator is allowed.
  } DESTINATION_REGISTER_FILE;

  typedef enum tagSOURCE_0_REGISTER_FILE {
    SOURCE_0_REGISTER_FILE_GRF =
        0x0, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects General Register File
             // as source 0.</span>
    SOURCE_0_REGISTER_FILE_IMM =
        0x1, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects Immediate Register
             // File as source 0.</span>
    SOURCE_0_REGISTER_FILE_ARF = 0x1, // when src0 is accumulator with NF type
  } SOURCE_0_REGISTER_FILE;

  typedef enum tagSOURCE_1_REGISTER_FILE {
    SOURCE_1_REGISTER_FILE_GRF =
        0x0, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects General Register File
             // as source 1.</span>
    SOURCE_1_REGISTER_FILE_ARF =
        0x1, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects Architectural Register
             // File as source 1. Only Accumulator is allowed.</span>
  } SOURCE_1_REGISTER_FILE;

  typedef enum tagSOURCE_2_REGISTER_FILE {
    SOURCE_2_REGISTER_FILE_GRF =
        0x0, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects General Register File
             // as source 2.</span>
    SOURCE_2_REGISTER_FILE_IMM =
        0x1, // <span style="color: rgb(0, 0, 0); font-family: Arial,
             // sans-serif; line-height: normal;">Selects Immediate Register
             // File as source 2.</span>
  } SOURCE_2_REGISTER_FILE;

  typedef enum tagDESTINATION_HORIZONTAL_STRIDE {
    DESTINATION_HORIZONTAL_STRIDE_1_ELEMENT = 0x0,
    DESTINATION_HORIZONTAL_STRIDE_2_ELEMENT = 0x1,
  } DESTINATION_HORIZONTAL_STRIDE;

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    TheStructure.RawData[0] = 0x0;
    TheStructure.RawData[1] = 0x0;
    TheStructure.RawData[2] = 0x0;
    TheStructure.RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_ALIGN1_THREE_SRC sInit() {
    tagEU_INSTRUCTION_ALIGN1_THREE_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS

  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetRawData(UINT const index) {
    return TheStructure.RawData[index];
  }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return TheStructure.Common.Header;
  }

  __CODEGEN_INLINE DWORD GetFlagSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.FlagSubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetFlagSubregisterNumber(DWORD FlagSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagSubregisterNumberValue);
    TheStructure.Common.FlagSubregisterNumber = FlagSubregisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetFlagRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.FlagRegisterNumber;
  }

  __CODEGEN_INLINE void SetFlagRegisterNumber(DWORD FlagRegisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagRegisterNumberValue);
    TheStructure.Common.FlagRegisterNumber = FlagRegisterNumberValue;
  }

  __CODEGEN_INLINE MASKCTRL GetMaskctrl() const {
    __CODEGEN_GET_MACRO();
    return (MASKCTRL)TheStructure.Common.Maskctrl;
  }

  __CODEGEN_INLINE void SetMaskctrl(MASKCTRL MaskctrlValue) {
    __CODEGEN_SET_MACRO(MaskctrlValue);
    TheStructure.Common.Maskctrl = MaskctrlValue;
  }

  __CODEGEN_INLINE EXECUTION_DATATYPE GetExecutionDatatype() const {
    __CODEGEN_GET_MACRO();
    return (EXECUTION_DATATYPE)TheStructure.Common.ExecutionDatatype;
  }

  __CODEGEN_INLINE void
  SetExecutionDatatype(EXECUTION_DATATYPE ExecutionDatatypeValue) {
    __CODEGEN_SET_MACRO(ExecutionDatatypeValue);
    TheStructure.Common.ExecutionDatatype = ExecutionDatatypeValue;
  }

  __CODEGEN_INLINE DESTINATION_REGISTER_FILE
  GetDestinationRegisterFile() const {
    __CODEGEN_GET_MACRO();
    return (DESTINATION_REGISTER_FILE)
        TheStructure.Common.DestinationRegisterFile;
  }

  __CODEGEN_INLINE void SetDestinationRegisterFile(
      DESTINATION_REGISTER_FILE DestinationRegisterFileValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterFileValue);
    TheStructure.Common.DestinationRegisterFile = DestinationRegisterFileValue;
  }

  __CODEGEN_INLINE SOURCE_0_REGISTER_FILE GetSource0RegisterFile() const {
    __CODEGEN_GET_MACRO();
    return (SOURCE_0_REGISTER_FILE)TheStructure.Common.Source0RegisterFile;
  }

  __CODEGEN_INLINE void
  SetSource0RegisterFile(SOURCE_0_REGISTER_FILE Source0RegisterFileValue) {
    __CODEGEN_SET_MACRO(Source0RegisterFileValue);
    TheStructure.Common.Source0RegisterFile = Source0RegisterFileValue;
  }

  __CODEGEN_INLINE SOURCE_1_REGISTER_FILE GetSource1RegisterFile() const {
    __CODEGEN_GET_MACRO();
    return (SOURCE_1_REGISTER_FILE)TheStructure.Common.Source1RegisterFile;
  }

  __CODEGEN_INLINE void
  SetSource1RegisterFile(SOURCE_1_REGISTER_FILE Source1RegisterFileValue) {
    __CODEGEN_SET_MACRO(Source1RegisterFileValue);
    TheStructure.Common.Source1RegisterFile = Source1RegisterFileValue;
  }

  __CODEGEN_INLINE SOURCE_2_REGISTER_FILE GetSource2RegisterFile() const {
    __CODEGEN_GET_MACRO();
    return (SOURCE_2_REGISTER_FILE)TheStructure.Common.Source2RegisterFile;
  }

  __CODEGEN_INLINE void
  SetSource2RegisterFile(SOURCE_2_REGISTER_FILE Source2RegisterFileValue) {
    __CODEGEN_SET_MACRO(Source2RegisterFileValue);
    TheStructure.Common.Source2RegisterFile = Source2RegisterFileValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1DATATYPE GetDestinationDatatype() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1DATATYPE)TheStructure.Common.DestinationDatatype;
  }

  __CODEGEN_INLINE void
  SetDestinationDatatype(TERNARYALIGN1DATATYPE DestinationDatatypeValue) {
    __CODEGEN_SET_MACRO(DestinationDatatypeValue);
    TheStructure.Common.DestinationDatatype = DestinationDatatypeValue;
  }

  __CODEGEN_INLINE DESTINATION_HORIZONTAL_STRIDE
  GetDestinationHorizontalStride() const {
    __CODEGEN_GET_MACRO();
    return (DESTINATION_HORIZONTAL_STRIDE)
        TheStructure.Common.DestinationHorizontalStride;
  }

  __CODEGEN_INLINE void SetDestinationHorizontalStride(
      DESTINATION_HORIZONTAL_STRIDE DestinationHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(DestinationHorizontalStrideValue);
    TheStructure.Common.DestinationHorizontalStride =
        DestinationHorizontalStrideValue;
  }

  typedef enum tagDESTINATIONSUBREGISTERNUMBER {
    DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT = 0x3,
    DESTINATIONSUBREGISTERNUMBER_ALIGN_SIZE = 0x8,
  } DESTINATIONSUBREGISTERNUMBER;

  __CODEGEN_INLINE DWORD GetDestinationSubregisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.DestinationSubregisterNumber
           << DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT;
  }

  __CODEGEN_INLINE void
  SetDestinationSubregisterNumber(DWORD DestinationSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationSubregisterNumberValue);
    TheStructure.Common.DestinationSubregisterNumber =
        DestinationSubregisterNumberValue >>
        DESTINATIONSUBREGISTERNUMBER_BIT_SHIFT;
  }

  __CODEGEN_INLINE void SetDestinationSpecialAcc(DWORD DestinationSpecialAcc) {
    __CODEGEN_SET_MACRO(DestinationAccRegSel);
    TheStructure.Common.DestinationAccRegLowBits = DestinationSpecialAcc & 0x3;
    TheStructure.Common.DestinationSubregisterNumber =
        (DestinationSpecialAcc >> 2) & 0x3;
  }

  __CODEGEN_INLINE DWORD GetDestinationRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.DestinationRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetDestinationRegisterNumber(DWORD DestinationRegisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterNumberValue);
    TheStructure.Common.DestinationRegisterNumber =
        DestinationRegisterNumberValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1DATATYPE GetSource0Datatype() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1DATATYPE)TheStructure.Common.Source0Datatype;
  }

  __CODEGEN_INLINE void
  SetSource0Datatype(TERNARYALIGN1DATATYPE Source0DatatypeValue) {
    __CODEGEN_SET_MACRO(Source0DatatypeValue);
    TheStructure.Common.Source0Datatype = Source0DatatypeValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1DATATYPE GetSource1Datatype() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1DATATYPE)TheStructure.Common.Source1Datatype;
  }

  __CODEGEN_INLINE void
  SetSource1Datatype(TERNARYALIGN1DATATYPE Source1DatatypeValue) {
    __CODEGEN_SET_MACRO(Source1DatatypeValue);
    TheStructure.Common.Source1Datatype = Source1DatatypeValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1VERTSTRIDE GetSource1VerticalStride() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1VERTSTRIDE)TheStructure.Common.Source1VerticalStride;
  }

  __CODEGEN_INLINE void
  SetSource1VerticalStride(TERNARYALIGN1VERTSTRIDE Source1VerticalStrideValue) {
    __CODEGEN_SET_MACRO(Source1VerticalStrideValue);
    TheStructure.Common.Source1VerticalStride = Source1VerticalStrideValue;
  }

  __CODEGEN_INLINE DWORD GetSource1HorizontalStride() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure.Common.Source1HorizontalStride;
  }

  __CODEGEN_INLINE void
  SetSource1HorizontalStride(DWORD Source1HorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source1HorizontalStrideValue);
    TheStructure.Common.Source1HorizontalStride = Source1HorizontalStrideValue;
  }

  __CODEGEN_INLINE DWORD
  GetSource1SubregisterNumber_SourceSubRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)
        TheStructure.Common.Source1SubregisterNumber_SourceSubRegisterNumber;
  }

  // SubRegNum[4:1] for special acc
  __CODEGEN_INLINE void SetSource1SpecialAcc(DWORD SpecialAccValue) {
    __CODEGEN_SET_MACRO(Source1SubregisterNumber_SourceSubRegisterNumberValue);
    DWORD V =
        ((SpecialAccValue << 1) |
         (TheStructure.Common.Source1SubregisterNumber_SourceSubRegisterNumber &
          0x1));
    TheStructure.Common.Source1SubregisterNumber_SourceSubRegisterNumber =
        (V & 0x1F);
  }

  __CODEGEN_INLINE void SetSource1SubregisterNumber_SourceSubRegisterNumber(
      DWORD Source1SubregisterNumber_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1SubregisterNumber_SourceSubRegisterNumberValue);
    TheStructure.Common.Source1SubregisterNumber_SourceSubRegisterNumber =
        Source1SubregisterNumber_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource1RegisterNumber_SourceRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)
        TheStructure.Common.Source1RegisterNumber_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource1RegisterNumber_SourceRegisterNumber(
      DWORD Source1RegisterNumber_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1RegisterNumber_SourceRegisterNumberValue);
    TheStructure.Common.Source1RegisterNumber_SourceRegisterNumber =
        Source1RegisterNumber_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1DATATYPE GetSource2Datatype() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1DATATYPE)TheStructure.Common.Source2Datatype;
  }

  __CODEGEN_INLINE void
  SetSource2Datatype(TERNARYALIGN1DATATYPE Source2DatatypeValue) {
    __CODEGEN_SET_MACRO(Source2DatatypeValue);
    TheStructure.Common.Source2Datatype = Source2DatatypeValue;
  }

  __CODEGEN_INLINE SRCMOD GetSource0Modifier() const {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)TheStructure.PropertySourceModifierIsTrue.Source0Modifier;
  }

  __CODEGEN_INLINE void SetSource0Modifier(SRCMOD Source0ModifierValue) {
    __CODEGEN_SET_MACRO(Source0ModifierValue);
    TheStructure.PropertySourceModifierIsTrue.Source0Modifier =
        Source0ModifierValue;
  }

  __CODEGEN_INLINE SRCMOD GetSource1Modifier() const {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)TheStructure.PropertySourceModifierIsTrue.Source1Modifier;
  }

  __CODEGEN_INLINE void SetSource1Modifier(SRCMOD Source1ModifierValue) {
    __CODEGEN_SET_MACRO(Source1ModifierValue);
    TheStructure.PropertySourceModifierIsTrue.Source1Modifier =
        Source1ModifierValue;
  }

  __CODEGEN_INLINE SRCMOD GetSource2Modifier() const {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)TheStructure.PropertySourceModifierIsTrue.Source2Modifier;
  }

  __CODEGEN_INLINE void SetSource2Modifier(SRCMOD Source2ModifierValue) {
    __CODEGEN_SET_MACRO(Source2ModifierValue);
    TheStructure.PropertySourceModifierIsTrue.Source2Modifier =
        Source2ModifierValue;
  }

  __CODEGEN_INLINE TERNARYALIGN1VERTSTRIDE GetSource0VerticalStride() const {
    __CODEGEN_GET_MACRO();
    return (TERNARYALIGN1VERTSTRIDE)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0VerticalStride;
  }

  __CODEGEN_INLINE void
  SetSource0VerticalStride(TERNARYALIGN1VERTSTRIDE Source0VerticalStrideValue) {
    __CODEGEN_SET_MACRO(Source0VerticalStrideValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0VerticalStride = Source0VerticalStrideValue;
  }

  __CODEGEN_INLINE DWORD GetSource0HorizontalStride() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0HorizontalStride;
  }

  __CODEGEN_INLINE void
  SetSource0HorizontalStride(DWORD Source0HorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0HorizontalStrideValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0HorizontalStride = Source0HorizontalStrideValue;
  }

  __CODEGEN_INLINE DWORD
  GetSource0SubregisterNumber_SourceSubRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0SubregisterNumber_SourceSubRegisterNumber;
  }

  // SubRegNum[4:1] for special acc
  __CODEGEN_INLINE void SetSource0SpecialAcc(DWORD SpecialAccValue) {
    __CODEGEN_SET_MACRO(Source0SubregisterNumber_SourceSubRegisterNumberValue);
    DWORD V =
        ((SpecialAccValue << 1) |
         (TheStructure
              .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
              .Source0SubregisterNumber_SourceSubRegisterNumber &
          0x1));
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0SubregisterNumber_SourceSubRegisterNumber = (V & 0x1F);
  }

  __CODEGEN_INLINE void SetSource0SubregisterNumber_SourceSubRegisterNumber(
      DWORD Source0SubregisterNumber_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0SubregisterNumber_SourceSubRegisterNumberValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0SubregisterNumber_SourceSubRegisterNumber =
        Source0SubregisterNumber_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource0RegisterNumber_SourceRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0RegisterNumber_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0RegisterNumber_SourceRegisterNumber(
      DWORD Source0RegisterNumber_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0RegisterNumber_SourceRegisterNumberValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsGrf
        .Source0RegisterNumber_SourceRegisterNumber =
        Source0RegisterNumber_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource0ImmediateValue() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsImm
        .Source0ImmediateValue;
  }

  __CODEGEN_INLINE void
  SetSource0ImmediateValue(DWORD Source0ImmediateValueValue) {
    __CODEGEN_SET_MACRO(Source0ImmediateValueValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource0RegisterFileIsImm
        .Source0ImmediateValue = Source0ImmediateValueValue;
  }

  __CODEGEN_INLINE DWORD GetSource2HorizontalStride() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2HorizontalStride;
  }

  __CODEGEN_INLINE void
  SetSource2HorizontalStride(DWORD Source2HorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source2HorizontalStrideValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2HorizontalStride = Source2HorizontalStrideValue;
  }

  __CODEGEN_INLINE DWORD
  GetSource2SubregisterNumber_SourceSubRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2SubregisterNumber_SourceSubRegisterNumber;
  }

  // SubRegNum[4:1] for special acc
  __CODEGEN_INLINE void SetSource2SpecialAcc(DWORD SpecialAccValue) {
    __CODEGEN_SET_MACRO(Source2SubregisterNumber_SourceSubRegisterNumberValue);
    DWORD V =
        ((SpecialAccValue << 1) |
         (TheStructure
              .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
              .Source2SubregisterNumber_SourceSubRegisterNumber &
          0x1));
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2SubregisterNumber_SourceSubRegisterNumber = (V & 0x1F);
  }

  __CODEGEN_INLINE void SetSource2SubregisterNumber_SourceSubRegisterNumber(
      DWORD Source2SubregisterNumber_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source2SubregisterNumber_SourceSubRegisterNumberValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2SubregisterNumber_SourceSubRegisterNumber =
        Source2SubregisterNumber_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource2RegisterNumber_SourceRegisterNumber() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2RegisterNumber_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource2RegisterNumber_SourceRegisterNumber(
      DWORD Source2RegisterNumber_SourceRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source2RegisterNumber_SourceRegisterNumberValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsGrf
        .Source2RegisterNumber_SourceRegisterNumber =
        Source2RegisterNumber_SourceRegisterNumberValue;
  }

  __CODEGEN_INLINE DWORD GetSource2ImmediateValue() const {
    __CODEGEN_GET_MACRO();
    return (DWORD)TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsImm
        .Source2ImmediateValue;
  }

  __CODEGEN_INLINE void
  SetSource2ImmediateValue(DWORD Source2ImmediateValueValue) {
    __CODEGEN_SET_MACRO(Source2ImmediateValueValue);
    TheStructure
        .StructureEu_Instruction_Align1_Three_SrcSource2RegisterFileIsImm
        .Source2ImmediateValue = Source2ImmediateValueValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_ALIGN1_THREE_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_ALIGN1_THREE_SRC));

typedef union tagEU_INSTRUCTION_BRANCH_CONDITIONAL {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;
    DWORD DestinationRegisterFile : BITFIELD_RANGE(32, 33); // REGFILE

    /*****************************************************************************\
    This field specifies the numeric data type of the destination operand dst.
    The bits of the destination operand are interpreted as the identified
    numeric data type, rather than coerced into a type implied by the operator.
    For a send instruction, this field applies to the CurrDst ? the current
    destination operand.
    \*****************************************************************************/
    DWORD DestinationDataType : BITFIELD_RANGE(34, 36); // DataType
    DWORD Src0Regfile : BITFIELD_RANGE(37, 38);         // REGFILE
    DWORD Src0Srctype : BITFIELD_RANGE(39, 41);         // DataType
    DWORD Src1Regfile : BITFIELD_RANGE(42, 43);         // REGFILE

    /*****************************************************************************\
    This field specifies the numeric data type of the source operand src1. The
    bits of a source operand are interpreted as the identified numeric data
    type, rather than coerced into a type implied by the operator. Depending on
    RegFile field of the source operand, there are two different encoding for
    this field. If a source is a register operand, this field follows the Source
    Register Type Encoding. If a source is an immediate operand, this field
    follows the Source Immediate Type Encoding.

    Programming Notes:
    Both source operands, src0 and src1, support immediate types, but only one
    immediate is allowed for a given instruction and it must be the last
    operand.

    Halfbyte integer vector (v) type can only be used in instructions in
    packed-word execution mode. Therefore, in a two-source instruction where
    src1 is of type :v, src0 must be of type :b, :ub, :w, or :uw.
    \*****************************************************************************/
    DWORD Src1Srctype : BITFIELD_RANGE(44, 46);          // DataType
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(47); // MBZ

    /*****************************************************************************\
    Jump Target Offset. The jump distance in number of eight-byte units if a
    jump is taken for the instruction.
    \*****************************************************************************/
    DWORD Jip : BITFIELD_RANGE(48, 63); // S15
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
  } Common;
  struct tagSrc1RegfileIsImm {
    EU_INSTRUCTION_SOURCES_REG_IMM Sources;
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
  } Src1RegfileIsImm;
  struct tagSrc1RegfileNotImm {
    EU_INSTRUCTION_SOURCES_REG_REG Sources;
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
  } Src1RegfileNotImm;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.DestinationRegisterFile = 0x0;
    Common.DestinationDataType = 0x0;
    Common.Src0Regfile = 0x0;
    Common.Src0Srctype = 0x0;
    Common.Src1Regfile = 0x0;
    Common.Src1Srctype = 0x0;
    Common.Jip = 0x0;
    RawData[2] = 0x0;
    RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_BRANCH_CONDITIONAL sInit() {
    tagEU_INSTRUCTION_BRANCH_CONDITIONAL state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE REGFILE GetDestinationRegisterFile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.DestinationRegisterFile;
  }

  __CODEGEN_INLINE void
  SetDestinationRegisterFile(REGFILE DestinationRegisterFileValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterFileValue);
    Common.DestinationRegisterFile = DestinationRegisterFileValue;
  }

  __CODEGEN_INLINE DWORD GetDestinationDataType() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.DestinationDataType;
  }

  __CODEGEN_INLINE void SetDestinationDataType(DWORD DestinationDataTypeValue) {
    __CODEGEN_SET_MACRO(DestinationDataTypeValue);
    Common.DestinationDataType = DestinationDataTypeValue;
  }

  __CODEGEN_INLINE REGFILE GetSrc0Regfile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.Src0Regfile;
  }

  __CODEGEN_INLINE void SetSrc0Regfile(REGFILE Src0RegfileValue) {
    __CODEGEN_SET_MACRO(Src0RegfileValue);
    Common.Src0Regfile = Src0RegfileValue;
  }

  __CODEGEN_INLINE DWORD GetSrc0Srctype() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Src0Srctype;
  }

  __CODEGEN_INLINE void SetSrc0Srctype(DWORD Src0SrctypeValue) {
    __CODEGEN_SET_MACRO(Src0SrctypeValue);
    Common.Src0Srctype = Src0SrctypeValue;
  }

  __CODEGEN_INLINE REGFILE GetSrc1Regfile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.Src1Regfile;
  }

  __CODEGEN_INLINE void SetSrc1Regfile(REGFILE Src1RegfileValue) {
    __CODEGEN_SET_MACRO(Src1RegfileValue);
    Common.Src1Regfile = Src1RegfileValue;
  }

  __CODEGEN_INLINE DWORD GetSrc1Srctype() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Src1Srctype;
  }

  __CODEGEN_INLINE void SetSrc1Srctype(DWORD Src1SrctypeValue) {
    __CODEGEN_SET_MACRO(Src1SrctypeValue);
    Common.Src1Srctype = Src1SrctypeValue;
  }

  __CODEGEN_INLINE DWORD GetJip() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Jip;
  }

  __CODEGEN_INLINE void SetJip(DWORD JipValue) {
    __CODEGEN_SET_MACRO(JipValue);
    Common.Jip = JipValue;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG_IMM &GetSources_0() {
    __CODEGEN_GET_MACRO();
    return Src1RegfileIsImm.Sources;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG_REG &GetSources_1() {
    __CODEGEN_GET_MACRO();
    return Src1RegfileNotImm.Sources;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BRANCH_CONDITIONAL;

static_assert(16 == sizeof(EU_INSTRUCTION_BRANCH_CONDITIONAL));

typedef union tagEU_INSTRUCTION_BRANCH_ONE_SRC {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControl;
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 88); // Override
    QWORD Src1Regfile : BITFIELD_RANGE(89, 90);                  // REGFILE
    QWORD Src1Srctype : BITFIELD_RANGE(91, 94);                  // SRCTYPE
    QWORD Source0AddressImmediate9SignBit : BITFIELD_BIT(95);    //

    /*****************************************************************************\
    Jump Target Offset. The relative offset in bytes if a jump is taken for the
    instruction.
    \*****************************************************************************/
    QWORD Jip : BITFIELD_RANGE(96, 127); // S31
  } Common;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    QWORD Source0_SourceSubRegisterNumber : BITFIELD_RANGE(64, 68); //
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(69, 76);    //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(77, 97);    // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(98, 127);   // Override

  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    QWORD Source0_SourceAddressImmediate80 : BITFIELD_RANGE(64, 72); // S9[8:0]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(73, 76); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(77, 94);     // Override
    QWORD Source0_SourceAddressImmediate9 : BITFIELD_RANGE(95, 95);  // S9[9:9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(96, 127);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 76);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(77, 78);    // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 97);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(98, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 76);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(77, 78);        // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 97);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(98, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue;
  struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign1 {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);    // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 78);   // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(79);         // ADDRMODE
    QWORD Source0_SourceHorizontalStride : BITFIELD_RANGE(80, 81); // HORZSTRIDE
    QWORD Source0_SourceWidth : BITFIELD_RANGE(82, 84);            // WIDTH
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(85, 88);   // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(89, 97);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(98, 127);  // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign1;
  struct tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16 {
    DWORD __CODEGEN_UNIQUE(Overridden)[2];
    QWORD Source0_SourceChannelSelect30
        : BITFIELD_RANGE(64, 67); // ChanSel[4][3:0]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(68, 78); // Override
    QWORD Source0_SourceAddressingMode : BITFIELD_BIT(79);       // ADDRMODE
    QWORD Source0_SourceChannelSelect74
        : BITFIELD_RANGE(80, 83);                        // ChanSel[4][7:4]
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(84); // MBZ
    QWORD Source0_SourceVerticalStride : BITFIELD_RANGE(85, 88);  // VERTSTRIDE
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(89, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 67); // Override
    QWORD Source0_SourceSubregisterNumber44
        : BITFIELD_BIT(68); // SrcSubRegNum[4:4]
    QWORD Source0_SourceRegisterNumber : BITFIELD_RANGE(69, 76);  //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(77, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);      // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 67);     // Override
    QWORD Source0_SourceAddressImmediate84 : BITFIELD_RANGE(68, 72); // S9[8:4]
    QWORD Source0_AddressSubregisterNumber : BITFIELD_RANGE(73, 76); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(77, 127);    // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 76);  // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(77, 78);    // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsFalse;
  struct
      tagStructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 76);  // Override
    QWORD Source0_SourceModifier : BITFIELD_RANGE(77, 78);        // SRCMOD
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 127); // Override
  } StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndPropertySourceModifierIsTrue;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.OperandControl.Init();
    RawData[2] = 0x0;
    Common.Src1Regfile = 0x0;
    Common.Src1Srctype = 0x0;
    Common.Source0AddressImmediate9SignBit = 0x0;
    RawData[3] = 0x0;
    Common.Jip = 0x0;
  }

  static tagEU_INSTRUCTION_BRANCH_ONE_SRC sInit() {
    tagEU_INSTRUCTION_BRANCH_ONE_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControl() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControl;
  }

  __CODEGEN_INLINE REGFILE GetSrc1Regfile() {
    __CODEGEN_GET_MACRO();
    return (REGFILE)Common.Src1Regfile;
  }

  __CODEGEN_INLINE void SetSrc1Regfile(REGFILE Src1RegfileValue) {
    __CODEGEN_SET_MACRO(Src1RegfileValue);
    Common.Src1Regfile = Src1RegfileValue;
  }

  __CODEGEN_INLINE int GetSrc1Srctype() {
    __CODEGEN_GET_MACRO();
    return Common.Src1Srctype;
  }

  __CODEGEN_INLINE void SetSrc1Srctype(int Src1SrctypeValue) {
    __CODEGEN_SET_MACRO(Src1SrctypeValue);
    Common.Src1Srctype = Src1SrctypeValue;
  }

  __CODEGEN_INLINE QWORD GetSource0AddressImmediate9SignBit() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Source0AddressImmediate9SignBit;
  }

  __CODEGEN_INLINE void SetSource0AddressImmediate9SignBit(
      QWORD Source0AddressImmediate9SignBitValue) {
    __CODEGEN_SET_MACRO(Source0AddressImmediate9SignBitValue);
    Common.Source0AddressImmediate9SignBit =
        Source0AddressImmediate9SignBitValue;
  }

  __CODEGEN_INLINE QWORD GetJip() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Jip;
  }

  __CODEGEN_INLINE void SetJip(QWORD JipValue) {
    __CODEGEN_SET_MACRO(JipValue);
    Common.Jip = JipValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect
            .Source0_SourceSubRegisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubRegisterNumber(
      QWORD Source0_SourceSubRegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0_SourceSubRegisterNumberValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubRegisterNumber = Source0_SourceSubRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect
            .Source0_SourceRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0_SourceRegisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsDirect
        .Source0_SourceRegisterNumber = Source0_SourceRegisterNumber_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate80() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect
            .Source0_SourceAddressImmediate80;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate80(
      QWORD Source0_SourceAddressImmediate80Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate80Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate80 =
        Source0_SourceAddressImmediate80Value;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate9(
      QWORD Source0_SourceAddressImmediate9Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate9Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate9 = Source0_SourceAddressImmediate9Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_0(
      QWORD Source0_AddressSubregisterNumber_0Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_0Value;
  }

  __CODEGEN_INLINE SRCMOD GetSource0_SourceModifier() {
    __CODEGEN_GET_MACRO();
    return (SRCMOD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue
            .Source0_SourceModifier;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceModifier(SRCMOD Source0_SourceModifier_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceModifier_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1AndPropertySourceModifierIsTrue
        .Source0_SourceModifier = Source0_SourceModifier_0Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_0() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode(
      ADDRMODE Source0_SourceAddressingMode_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_0Value;
  }

  __CODEGEN_INLINE HORZSTRIDE GetSource0_SourceHorizontalStride() {
    __CODEGEN_GET_MACRO();
    return (HORZSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceHorizontalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceHorizontalStride(
      HORZSTRIDE Source0_SourceHorizontalStrideValue) {
    __CODEGEN_SET_MACRO(Source0_SourceHorizontalStrideValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceHorizontalStride = Source0_SourceHorizontalStrideValue;
  }

  __CODEGEN_INLINE WIDTH GetSource0_SourceWidth() {
    __CODEGEN_GET_MACRO();
    return (WIDTH)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceWidth;
  }

  __CODEGEN_INLINE void SetSource0_SourceWidth(WIDTH Source0_SourceWidthValue) {
    __CODEGEN_SET_MACRO(Source0_SourceWidthValue);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1.Source0_SourceWidth =
        Source0_SourceWidthValue;
  }

  __CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride() {
    __CODEGEN_GET_MACRO();
    return (VERTSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceVerticalStride;
  }

  __CODEGEN_INLINE void SetSource0_SourceVerticalStride(
      VERTSTRIDE Source0_SourceVerticalStride_0Value) {
    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_0Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign1
        .Source0_SourceVerticalStride = Source0_SourceVerticalStride_0Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect30() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source0_SourceChannelSelect30;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect30(QWORD Source0_SourceChannelSelect30Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect30Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source0_SourceChannelSelect30 = Source0_SourceChannelSelect30Value;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode_1() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source0_SourceAddressingMode;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressingMode_1(
      ADDRMODE Source0_SourceAddressingMode_1Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressingMode_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source0_SourceAddressingMode = Source0_SourceAddressingMode_1Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceChannelSelect74() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16
               .Source0_SourceChannelSelect74
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceChannelSelect74(QWORD Source0_SourceChannelSelect74Value) {
    __CODEGEN_SET_MACRO(Source0_SourceChannelSelect74Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16
        .Source0_SourceChannelSelect74 =
        Source0_SourceChannelSelect74Value >> 4;
  }

  //__CODEGEN_INLINE VERTSTRIDE GetSource0_SourceVerticalStride_1() {
  //    __CODEGEN_GET_MACRO();
  //    return
  //    (VERTSTRIDE)StructureEu_Instruction_Controls_AAccessmodeIsAlign16.Source0_SourceVerticalStride;
  //}
  //
  //__CODEGEN_INLINE void SetSource0_SourceVerticalStride_1(VERTSTRIDE
  //Source0_SourceVerticalStride_1Value) {
  //    __CODEGEN_SET_MACRO(Source0_SourceVerticalStride_1Value);
  //    StructureEu_Instruction_Controls_AAccessmodeIsAlign16.Source0_SourceVerticalStride
  //    = Source0_SourceVerticalStride_1Value;
  //}

  __CODEGEN_INLINE QWORD GetSource0_SourceSubregisterNumber44() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect
                   .Source0_SourceSubregisterNumber44
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceSubregisterNumber44(
      QWORD Source0_SourceSubregisterNumber44Value) {
    __CODEGEN_SET_MACRO(Source0_SourceSubregisterNumber44Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect
        .Source0_SourceSubregisterNumber44 =
        Source0_SourceSubregisterNumber44Value >> 4;
  }

  //__CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber_1() {
  //    __CODEGEN_GET_MACRO();
  //    return
  //    (QWORD)StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber;
  //}
  //
  //__CODEGEN_INLINE void SetSource0_SourceRegisterNumber_1(QWORD
  //Source0_SourceRegisterNumber_1Value) {
  //    __CODEGEN_SET_MACRO(Source0_SourceRegisterNumber_1Value);
  //    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsDirect.Source0_SourceRegisterNumber
  //    = Source0_SourceRegisterNumber_1Value;
  //}

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect
                   .Source0_SourceAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediate84(
      QWORD Source0_SourceAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0_SourceAddressImmediate84Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect
        .Source0_SourceAddressImmediate84 =
        Source0_SourceAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_AddressSubregisterNumber_1() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect
            .Source0_AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_AddressSubregisterNumber_1(
      QWORD Source0_AddressSubregisterNumber_1Value) {
    __CODEGEN_SET_MACRO(Source0_AddressSubregisterNumber_1Value);
    StructureEu_Instruction_Controls_AAccessmodeIsAlign16AndSource0SourceAddressingModeIsIndirect
        .Source0_AddressSubregisterNumber =
        Source0_AddressSubregisterNumber_1Value;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BRANCH_ONE_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_BRANCH_ONE_SRC));

typedef union tagEU_INSTRUCTION_BRANCH_TWO_SRC {
  struct tagCommon {
    EU_INSTRUCTION_HEADER Header;
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControl;

    /*****************************************************************************\
    The byte aligned jump distance if a jump is taken for the instruction.
    \*****************************************************************************/
    DWORD Uip : BITFIELD_RANGE(64, 95); // S31

    /*****************************************************************************\
    The byte-aligned jump distance if a jump is taken for the channel.
    \*****************************************************************************/
    DWORD Jip : BITFIELD_RANGE(96, 127); // S31
  } Common;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Header.Init();
    RawData[1] = 0x0;
    Common.OperandControl.Init();
    RawData[2] = 0x0;
    Common.Uip = 0x0;
    RawData[3] = 0x0;
    Common.Jip = 0x0;
  }

  static tagEU_INSTRUCTION_BRANCH_TWO_SRC sInit() {
    tagEU_INSTRUCTION_BRANCH_TWO_SRC state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWord(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_INSTRUCTION_HEADER &GetHeader() {
    __CODEGEN_GET_MACRO();
    return Common.Header;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControl() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControl;
  }

  __CODEGEN_INLINE DWORD GetUip() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Uip;
  }

  __CODEGEN_INLINE void SetUip(DWORD UipValue) {
    __CODEGEN_SET_MACRO(UipValue);
    Common.Uip = UipValue;
  }

  __CODEGEN_INLINE DWORD GetJip() {
    __CODEGEN_GET_MACRO();
    return (DWORD)Common.Jip;
  }

  __CODEGEN_INLINE void SetJip(DWORD JipValue) {
    __CODEGEN_SET_MACRO(JipValue);
    Common.Jip = JipValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_BRANCH_TWO_SRC;

static_assert(16 == sizeof(EU_INSTRUCTION_BRANCH_TWO_SRC));

typedef union tagEU_INSTRUCTION_ILLEGAL {
  struct tagCommon {
    QWORD Opcode : BITFIELD_RANGE(0, 6);                        // EU_OPCODE
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(7, 63);   // MBZ
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(64, 127); // MBZ
  } Common;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Opcode = 0x0;
    RawData[1] = 0x0;
    RawData[2] = 0x0;
    RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_ILLEGAL sInit() {
    tagEU_INSTRUCTION_ILLEGAL state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    Common.Opcode = OpcodeValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_ILLEGAL;

static_assert(16 == sizeof(EU_INSTRUCTION_ILLEGAL));

typedef union tagEU_INSTRUCTION_MATH {
  struct tagCommon {
    DWORD Opcode : BITFIELD_RANGE(0, 6);                // EU_OPCODE
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(7); // MBZ

    /*****************************************************************************\
    Access Mode. This field determines the operand access for the instruction.
    It applies to all source and destination operands. When it is cleared
    (Align1), the instruction uses byte-aligned addressing for source and
    destination operands. Source swizzle control and destination mask control
    are not supported. When it is set (Align16), the instruction uses
    16-byte-aligned addressing for all source and destination operands. Source
    swizzle control and destination mask control are supported in this mode.
    \*****************************************************************************/
    DWORD ControlsA_Accessmode : BITFIELD_BIT(0); // ACCESSMODE

    /*****************************************************************************\
    Destination Dependency Control. This field selectively disables destination
    dependency check and clear for this instruction. When it is set to 00,
    normal destination dependency control is performed for the instruction  -
    hardware checks for destination hazards to ensure data integrity.
    Specifically, destination register dependency check is conducted before the
    instruction is made ready for execution. After the instruction is executed,
    the destination register scoreboard will be cleared when the destination
    operands retire. When bit 10 is set (NoDDClr), the destination register
    scoreboard will NOT be cleared when the destination operands retire.  When
    bit 11 is set (NoDDChk), hardware does not check for destination register
    dependency before the instruction is made ready for execution.  NoDDClr and
    NoDDChk are not mutual exclusive. When this field is not all-zero, hardware
    does not protect against destination hazards for the instruction.  This is
    typically used to assemble data in a fine grained fashion (e.g.
    matrix-vector compute with dot-product instructions), where the data
    integrity is guaranteed by software based on the intended usage of
    instruction sequences.
    \*****************************************************************************/
    DWORD ControlsA_Depctrl : BITFIELD_RANGE(1, 2); // DEPCTRL

    /*****************************************************************************\
    Nibble Control. This field is used in some instructions along with QtrCtrl.
    See the description of QtrCtrl below. NibCtrl is only used for SIMD4
    instructions with a DF (Double Float) source or destination.

    Programming Notes:
    Note that if eighths are given zero-based indices from 0 to 7, then NibCtrl
    = 0 indicates even indices and NibCtrl = 1 indicates odd indices.
    \*****************************************************************************/
    DWORD ControlsA_Nibctrl : BITFIELD_BIT(3); // NIBCTRL

    /*****************************************************************************\
     - Quarter Control. This field provides explicit control for ARF selection.
    This field combined with NibCtrl and ExecSize determines which channels are
    used for the ARF registers.
    \*****************************************************************************/
    DWORD ControlsA_Qtrctrl : BITFIELD_RANGE(4, 5); // QTRCTRL

    /*****************************************************************************\
    Thread Control. This field provides explicit control for thread switching.
    If this field is set to 00b, it is up to the GEN execution units to manage
    thread switching. This is the normal (and unnamed) mode. In this mode, for
    example, if the current instruction cannot proceed due to operand
    dependencies, the EU switches to the next available thread to fill the
    compute pipe. In another example, if the current instruction is ready to go,
    however, there is another thread with higher priority that also has an
    instruction ready, the EU switches to that thread. If this field is set to
    Switch, a forced thread switch occurs after the current instruction is
    executed and before the next instruction. In addition, a long delay (longer
    than the execution pipe latency) is introduced for the current thread.
    Particularly, the instruction queue of the current thread is flushed after
    the current instruction is dispatched for execution. Switch is designed
    primarily as a safety feature in case there are race conditions for certain
    instructions.
    \*****************************************************************************/
    DWORD ControlsA_ThreadControl : BITFIELD_RANGE(6, 7); // THREADCTRL

    /*****************************************************************************\
    This field determines the number of channels operating in parallel for this
    instruction.  The size cannot exceed the maximum number of channels allowed
    for the given data type.
    \*****************************************************************************/
    DWORD ControlsA_Execsize : BITFIELD_RANGE(13, 15);           // EXECSIZE
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(19, 23); // Override
    DWORD FunctionControlFc : BITFIELD_RANGE(24, 27);            // FC

    /*****************************************************************************\
    AccWrCtrl. This field allows per instruction accumulator write control.
    \*****************************************************************************/
    DWORD ControlsB_Accwrctrl : BITFIELD_BIT(28); // ACCWRCTRL

    /*****************************************************************************\
    Compaction Control
    Indicates whether the instruction is compacted to the 64-bit compact
    instruction format. When this bit is set, the 64-bit compact instruction
    format is used. The EU decodes the compact format using lookup tables
    internal to the hardware, but documented for use by software tools. Only
    some instruction variations can be compacted, the variations supported by
    those lookup tables and the compact format. See EU Compact Instruction
    Format [DevBDW+] for more information.
    \*****************************************************************************/
    DWORD ControlsB_Cmptctrl : BITFIELD_BIT(29); // CMPTCTRL

    /*****************************************************************************\
    This field allows the insertion of a breakpoint at the current instruction.
    When the bit is set, hardware automatically stores the current IP in CR
    register and jumps to the System IP (SIP) BEFORE executing the current
    instruction.
    \*****************************************************************************/
    DWORD ControlsB_Debugctrl : BITFIELD_BIT(30); // DEBUGCTRL
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControl;
    EU_INSTRUCTION_SOURCES_REG_REG Regsource;
    // EU_INSTRUCTION_SOURCES_REG_REG __CODEGEN_UNIQUE(Overridden) :
    // BITFIELD_BIT(      127); // Override
  } Common;
  struct tagPropertyPredicationIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7); // Override

    /*****************************************************************************\
    This field, together with PredInv, enables and controls the generation of
    the predication mask for the instruction.  It allows per-channel conditional
    execution of the instruction based on the content of the selected flag
    register.  Encoding depends on the access mode. In Align16 access mode,
    there are eight encodings (including no predication). All encodings are
    based on group-of-4 predicate bits, including channel sequential,
    replication swizzles and horizontal any|all operations.  The same
    configuration is repeated for each group-of-4 execution channels.
    \*****************************************************************************/
    QWORD ControlsA_Predctrl : BITFIELD_RANGE(8, 11); // PREDCTRL

    /*****************************************************************************\
    This field, together with PredCtrl, enables and controls the generation of
    the predication mask for the instruction.  When it is set, the predication
    uses the inverse of the predication bits generated according to setting of
    Predicate Control. In other words, effect of PredInv happens after PredCtrl.
    This field is ignored by hardware if Predicate Control is set to 0000  -
    there is no predication. PMask is the final predication mask produced by the
    effects of both fields.
    \*****************************************************************************/
    QWORD ControlsA_Predinv : BITFIELD_BIT(12);                   // PREDINV
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override

  } PropertyPredicationIsTrue;
  struct tagPropertyPredicationIsFalse {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);   // Override
    DWORD ControlsA__ : BITFIELD_RANGE(8, 11);                   // PREDCTRL
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(12);         //
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 31); // Override
    // QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE( 64,127); // Override
    DWORD __CODEGEN_UNIQUE(Overridden)[3];

  } PropertyPredicationIsFalse;
  struct tagPropertySaturationIsTrue {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override

    /*****************************************************************************\
    Enables or disables destination saturation.
    When it is set, output values to the destination register are saturated. The
    saturation operation depends on the destination data type. Saturation is the
    operation that converts any value outside the saturation target range for
    the data type to the closest value in the target range. For a floating-point
    destination type, the saturation target range is [0.0, 1.0]. For a
    floating-point NaN, there is no  -closest value -; any NaN saturates to 0.0.
    Note that enabling Saturate overrides all of the NaN propagation behaviors
    described for various numeric instructions. Any floating-point number
    greater than 1.0, including +INF, saturates to 1.0. Any negative
    floating-point number, including -INF, saturates to 0.0. Any floating-point
    number in the range 0.0 to 1.0 is not changed by saturation. For an integer
    destination type, the maximum range for that type is the saturation target
    range. For example, the saturation range for B (Signed Byte Integer) is
    [-128, 127]. When Saturate is clear, destination values are not saturated.
    For example, a wrapped result (modulo) is output to the destination for an
    overflowed integer value. See the Numeric Data Typessection for information
    about data types and their ranges.
    \*****************************************************************************/
    DWORD ControlsB_Saturate : BITFIELD_BIT(31); // SATURATE
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsTrue;
  struct tagPropertySaturationIsFalse {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(31);        // MBZ
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsFalse;
  DWORD RawData[4];

  // LOCAL ENUMERATIONS
  //
  typedef enum tagACCESSMODE {
    ACCESSMODE_ALIGN1 = 0x0,
    ACCESSMODE_ALIGN16 = 0x1,
  } ACCESSMODE;

  typedef enum tagNIBCTRL {
    NIBCTRL_ODD = 0x0,  // Use an odd 1/8th for DMask/VMask and ARF (first,
                        // third, fifth, or seventh depending on QtrCtrl).
    NIBCTRL_EVEN = 0x1, // Use an even 1/8th for DMask/VMask and ARF (second,
                        // fourth, sixth, or eighth depending on QtrCtrl).
  } NIBCTRL;

  // typedef enum tagACCWRCTRL {
  //     ACCWRCTRL_DON_T_WRITE_TO_ACC = 0x0,
  //     ACCWRCTRL_UPDATE_ACC         = 0x1,    // Write result to the ACC, and
  //     destination
  // } ACCWRCTRL;

  typedef enum tagCMPTCTRL {
    CMPTCTRL_NOCOMPACTION = 0x0, // No compaction. 128-bit native instruction
                                 // supporting all instruction options.
    CMPTCTRL_COMPACTED =
        0x1, // Compaction is enabled. 64-bit compact instruction supporting
             // only some instruction variations.
  } CMPTCTRL;

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Opcode = 0x0;
    RawData[1] = 0x0;
    Common.ControlsA_Accessmode = ACCESSMODE_ALIGN1;
    Common.ControlsA_Depctrl = 0x0;
    Common.ControlsA_Nibctrl = NIBCTRL_ODD;
    Common.ControlsA_Qtrctrl = 0x0;
    Common.ControlsA_ThreadControl = 0x0;
    Common.ControlsA_Execsize = 0x0;
    Common.FunctionControlFc = 0x0;
    Common.ControlsB_Accwrctrl = ACCWRCTRL_DON_T_WRITE_TO_ACC;
    Common.ControlsB_Cmptctrl = CMPTCTRL_NOCOMPACTION;
    Common.ControlsB_Debugctrl = DEBUGCTRL_NO_BREAKPOINT;
    RawData[2] = 0x0;
    Common.OperandControl.Init();
    RawData[3] = 0x0;
    Common.Regsource.Init();
  }

  static tagEU_INSTRUCTION_MATH sInit() {
    tagEU_INSTRUCTION_MATH state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    Common.Opcode = OpcodeValue;
  }

  __CODEGEN_INLINE ACCESSMODE GetControlsA_Accessmode() {
    __CODEGEN_GET_MACRO();
    return (ACCESSMODE)Common.ControlsA_Accessmode;
  }

  __CODEGEN_INLINE void
  SetControlsA_Accessmode(ACCESSMODE ControlsA_AccessmodeValue) {
    __CODEGEN_SET_MACRO(ControlsA_AccessmodeValue);
    Common.ControlsA_Accessmode = ControlsA_AccessmodeValue;
  }

  __CODEGEN_INLINE DEPCTRL GetControlsA_Depctrl() {
    __CODEGEN_GET_MACRO();
    return (DEPCTRL)Common.ControlsA_Depctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Depctrl(DEPCTRL ControlsA_DepctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_DepctrlValue);
    Common.ControlsA_Depctrl = ControlsA_DepctrlValue;
  }

  __CODEGEN_INLINE NIBCTRL GetControlsA_Nibctrl() {
    __CODEGEN_GET_MACRO();
    return (NIBCTRL)Common.ControlsA_Nibctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Nibctrl(NIBCTRL ControlsA_NibctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_NibctrlValue);
    Common.ControlsA_Nibctrl = ControlsA_NibctrlValue;
  }

  __CODEGEN_INLINE QTRCTRL GetControlsA_Qtrctrl() {
    __CODEGEN_GET_MACRO();
    return (QTRCTRL)Common.ControlsA_Qtrctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Qtrctrl(QTRCTRL ControlsA_QtrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_QtrctrlValue);
    Common.ControlsA_Qtrctrl = ControlsA_QtrctrlValue;
  }

  __CODEGEN_INLINE THREADCTRL GetControlsA_ThreadControl() {
    __CODEGEN_GET_MACRO();
    return (THREADCTRL)Common.ControlsA_ThreadControl;
  }

  __CODEGEN_INLINE void
  SetControlsA_ThreadControl(THREADCTRL ControlsA_ThreadControlValue) {
    __CODEGEN_SET_MACRO(ControlsA_ThreadControlValue);
    Common.ControlsA_ThreadControl = ControlsA_ThreadControlValue;
  }

  __CODEGEN_INLINE EXECSIZE GetControlsA_Execsize() {
    __CODEGEN_GET_MACRO();
    return (EXECSIZE)Common.ControlsA_Execsize;
  }

  __CODEGEN_INLINE void
  SetControlsA_Execsize(EXECSIZE ControlsA_ExecsizeValue) {
    __CODEGEN_SET_MACRO(ControlsA_ExecsizeValue);
    Common.ControlsA_Execsize = ControlsA_ExecsizeValue;
  }

  __CODEGEN_INLINE FC GetFunctionControlFc() {
    __CODEGEN_GET_MACRO();
    return (FC)Common.FunctionControlFc;
  }

  __CODEGEN_INLINE void SetFunctionControlFc(FC FunctionControlFcValue) {
    __CODEGEN_SET_MACRO(FunctionControlFcValue);
    Common.FunctionControlFc = FunctionControlFcValue;
  }

  __CODEGEN_INLINE ACCWRCTRL GetControlsB_Accwrctrl() {
    __CODEGEN_GET_MACRO();
    return (ACCWRCTRL)Common.ControlsB_Accwrctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Accwrctrl(ACCWRCTRL ControlsB_AccwrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_AccwrctrlValue);
    Common.ControlsB_Accwrctrl = ControlsB_AccwrctrlValue;
  }

  __CODEGEN_INLINE CMPTCTRL GetControlsB_Cmptctrl() {
    __CODEGEN_GET_MACRO();
    return (CMPTCTRL)Common.ControlsB_Cmptctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Cmptctrl(CMPTCTRL ControlsB_CmptctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_CmptctrlValue);
    Common.ControlsB_Cmptctrl = ControlsB_CmptctrlValue;
  }

  __CODEGEN_INLINE DEBUGCTRL GetControlsB_Debugctrl() {
    __CODEGEN_GET_MACRO();
    return (DEBUGCTRL)Common.ControlsB_Debugctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Debugctrl(DEBUGCTRL ControlsB_DebugctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_DebugctrlValue);
    Common.ControlsB_Debugctrl = ControlsB_DebugctrlValue;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControl() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControl;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_SOURCES_REG_REG &GetRegsource() {
    __CODEGEN_GET_MACRO();
    return Common.Regsource;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA_Predctrl() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsTrue.ControlsA_Predctrl;
  }

  __CODEGEN_INLINE void
  SetControlsA_Predctrl(PREDCTRL ControlsA_PredctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredctrlValue);
    PropertyPredicationIsTrue.ControlsA_Predctrl = ControlsA_PredctrlValue;
  }

  __CODEGEN_INLINE PREDINV GetControlsA_Predinv() {
    __CODEGEN_GET_MACRO();
    return (PREDINV)PropertyPredicationIsTrue.ControlsA_Predinv;
  }

  __CODEGEN_INLINE void SetControlsA_Predinv(PREDINV ControlsA_PredinvValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredinvValue);
    PropertyPredicationIsTrue.ControlsA_Predinv = ControlsA_PredinvValue;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA__() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsFalse.ControlsA__;
  }

  __CODEGEN_INLINE void SetControlsA__(PREDCTRL ControlsA__Value) {
    __CODEGEN_SET_MACRO(ControlsA__Value);
    PropertyPredicationIsFalse.ControlsA__ = ControlsA__Value;
  }

  __CODEGEN_INLINE SATURATE GetControlsB_Saturate() {
    __CODEGEN_GET_MACRO();
    return (SATURATE)PropertySaturationIsTrue.ControlsB_Saturate;
  }

  __CODEGEN_INLINE void
  SetControlsB_Saturate(SATURATE ControlsB_SaturateValue) {
    __CODEGEN_SET_MACRO(ControlsB_SaturateValue);
    PropertySaturationIsTrue.ControlsB_Saturate = ControlsB_SaturateValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_MATH;

static_assert(16 == sizeof(EU_INSTRUCTION_MATH));

typedef union tagEU_INSTRUCTION_NOP {
  struct tagCommon {
    QWORD Opcode : BITFIELD_RANGE(0, 6);                      // EU_OPCODE
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(7, 29); // MBZ

    /*****************************************************************************\
    This field allows the insertion of a breakpoint at the current instruction.
    When the bit is set, hardware automatically stores the current IP in CR
    register and jumps to the System IP (SIP) BEFORE executing the current
    instruction.
    \*****************************************************************************/
    QWORD Debugctrl : BITFIELD_BIT(30);                         // DEBUGCTRL
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(31, 63);  // MBZ
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(64, 127); // MBZ

  } Common;
  DWORD RawData[4];

  // LOCAL ENUMERATIONS
  //
  typedef enum tagDEBUGCTRL {
    DEBUGCTRL_NO_BREAKPOINT = 0x0,
    DEBUGCTRL_BREAKPOINT = 0x1,
  } DEBUGCTRL;

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Opcode = 0x0;
    Common.Debugctrl = DEBUGCTRL_NO_BREAKPOINT;
    RawData[1] = 0x0;
    RawData[2] = 0x0;
    RawData[3] = 0x0;
  }

  static tagEU_INSTRUCTION_NOP sInit() {
    tagEU_INSTRUCTION_NOP state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    Common.Opcode = OpcodeValue;
  }

  __CODEGEN_INLINE DEBUGCTRL GetDebugctrl() {
    __CODEGEN_GET_MACRO();
    return (DEBUGCTRL)Common.Debugctrl;
  }

  __CODEGEN_INLINE void SetDebugctrl(DEBUGCTRL DebugctrlValue) {
    __CODEGEN_SET_MACRO(DebugctrlValue);
    Common.Debugctrl = DebugctrlValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_NOP;

static_assert(16 == sizeof(EU_INSTRUCTION_NOP));

typedef union tagEU_INSTRUCTION_OPERAND_SEND_MSG {
  struct tagCommon {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 28); // Override
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(29, 30);  // MBZ

    /*****************************************************************************\
    This field controls the termination of the thread. For a send instruction,
    if this field is set, EU will terminate the thread and also set the EOT bit
    in the message sideband. This field only applies to the send instruction. It
    is not present for other instructions.
    \*****************************************************************************/
    DWORD Eot : BITFIELD_BIT(31); // EOT
  } Common;
  struct tagStructureEu_Instruction_SendSrc1RegfileIsImm {

    /*****************************************************************************\
    This field is intended to control the target function unit. Refer to the
    section on the specific target function unit for details on the contents of
    this field.
    \*****************************************************************************/
    DWORD MessageDescriptor_FunctionControl : BITFIELD_RANGE(0, 18); //

    /*****************************************************************************\
    If set, indicates that the message includes a header. Depending on the
    target shared function, this field may be restricted to either enabled or
    disabled. Refer to the specific shared function section for details.
    \*****************************************************************************/
    DWORD MessageDescriptor_HeaderPresent : BITFIELD_BIT(19); // bool

    /*****************************************************************************\
    This field indicates the number of 256-bit registers expected in the message
    response. The valid value ranges from 0 to 16. A value 0 indicates that the
    request message does not expect any response. The largest response supported
    is 16 GRF registers.
    \*****************************************************************************/
    DWORD MessageDescriptor_ResponseLength : BITFIELD_RANGE(20, 24); //

    /*****************************************************************************\
    This field specifies the number of 256-bit registers starting from
    &lt;curr_dest&gt; to be sent out on the request message payload. Valid value
    ranges from 1 to 15. A value of 0 is considered erroneous.
    \*****************************************************************************/
    DWORD MessageDescriptor_MessageLength : BITFIELD_RANGE(25, 28); //
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(29, 31);    // Override
  } StructureEu_Instruction_SendSrc1RegfileIsImm;
  struct tagStructureEu_Instruction_SendSrc1RegfileNotImm {

    /*****************************************************************************\
    In a send or sendc instruction refers to the option of providing the message
    descriptor field QWORD, of which bits 28:0 are used, in the first two words
    of the Address Register rather than as an immediate operand.
    \*****************************************************************************/
    DWORD Reg32 : BITFIELD_RANGE(0, 28);                         //
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(29, 31); // Override
  } StructureEu_Instruction_SendSrc1RegfileNotImm;
  DWORD RawData[1];

  // LOCAL ENUMERATIONS
  //
  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Eot = EOT_THREAD_IS_NOT_TERMINATED;
  }

  static tagEU_INSTRUCTION_OPERAND_SEND_MSG sInit() {
    tagEU_INSTRUCTION_OPERAND_SEND_MSG state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EOT GetEot() {
    __CODEGEN_GET_MACRO();
    return (EOT)Common.Eot;
  }

  __CODEGEN_INLINE void SetEot(EOT EotValue) {
    __CODEGEN_SET_MACRO(EotValue);
    Common.Eot = EotValue;
  }

  __CODEGEN_INLINE QWORD GetMessageDescriptor_FunctionControl() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_FunctionControl;
  }

  __CODEGEN_INLINE void SetMessageDescriptor_FunctionControl(
      QWORD MessageDescriptor_FunctionControlValue) {
    __CODEGEN_SET_MACRO(MessageDescriptor_FunctionControlValue);
    StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_FunctionControl =
        MessageDescriptor_FunctionControlValue;
  }

  __CODEGEN_INLINE QWORD GetMessageDescriptor_HeaderPresent() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_HeaderPresent;
  }

  __CODEGEN_INLINE void SetMessageDescriptor_HeaderPresent(
      QWORD MessageDescriptor_HeaderPresentValue) {
    __CODEGEN_SET_MACRO(MessageDescriptor_HeaderPresentValue);
    StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_HeaderPresent = MessageDescriptor_HeaderPresentValue;
  }

  __CODEGEN_INLINE QWORD GetMessageDescriptor_ResponseLength() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_ResponseLength;
  }

  __CODEGEN_INLINE void SetMessageDescriptor_ResponseLength(
      QWORD MessageDescriptor_ResponseLengthValue) {
    __CODEGEN_SET_MACRO(MessageDescriptor_ResponseLengthValue);
    StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_ResponseLength =
        MessageDescriptor_ResponseLengthValue;
  }

  __CODEGEN_INLINE QWORD GetMessageDescriptor_MessageLength() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_MessageLength;
  }

  __CODEGEN_INLINE void SetMessageDescriptor_MessageLength(
      QWORD MessageDescriptor_MessageLengthValue) {
    __CODEGEN_SET_MACRO(MessageDescriptor_MessageLengthValue);
    StructureEu_Instruction_SendSrc1RegfileIsImm
        .MessageDescriptor_MessageLength = MessageDescriptor_MessageLengthValue;
  }

  __CODEGEN_INLINE QWORD GetReg32() {
    __CODEGEN_GET_MACRO();
    return (QWORD)StructureEu_Instruction_SendSrc1RegfileNotImm.Reg32;
  }

  __CODEGEN_INLINE void SetReg32(QWORD Reg32Value) {
    __CODEGEN_SET_MACRO(Reg32Value);
    StructureEu_Instruction_SendSrc1RegfileNotImm.Reg32 = Reg32Value;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_OPERAND_SEND_MSG;

static_assert(4 == sizeof(EU_INSTRUCTION_OPERAND_SEND_MSG));

typedef union tagEU_INSTRUCTION_SEND {
  struct tagCommon {
    DWORD Opcode : BITFIELD_RANGE(0, 6);                // EU_OPCODE
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(7); // MBZ

    /*****************************************************************************\
    Access Mode. This field determines the operand access for the instruction.
    It applies to all source and destination operands. When it is cleared
    (Align1), the instruction uses byte-aligned addressing for source and
    destination operands. Source swizzle control and destination mask control
    are not supported. When it is set (Align16), the instruction uses
    16-byte-aligned addressing for all source and destination operands. Source
    swizzle control and destination mask control are supported in this mode.
    \*****************************************************************************/
    DWORD ControlsA_Accessmode : BITFIELD_BIT(0); // ACCESSMODE

    /*****************************************************************************\
    Destination Dependency Control. This field selectively disables destination
    dependency check and clear for this instruction. When it is set to 00,
    normal destination dependency control is performed for the instruction  -
    hardware checks for destination hazards to ensure data integrity.
    Specifically, destination register dependency check is conducted before the
    instruction is made ready for execution. After the instruction is executed,
    the destination register scoreboard will be cleared when the destination
    operands retire. When bit 10 is set (NoDDClr), the destination register
    scoreboard will NOT be cleared when the destination operands retire.  When
    bit 11 is set (NoDDChk), hardware does not check for destination register
    dependency before the instruction is made ready for execution.  NoDDClr and
    NoDDChk are not mutual exclusive. When this field is not all-zero, hardware
    does not protect against destination hazards for the instruction.  This is
    typically used to assemble data in a fine grained fashion (e.g.
    matrix-vector compute with dot-product instructions), where the data
    integrity is guaranteed by software based on the intended usage of
    instruction sequences.
    \*****************************************************************************/
    DWORD ControlsA_Depctrl : BITFIELD_RANGE(1, 2); // DEPCTRL

    /*****************************************************************************\
    Nibble Control. This field is used in some instructions along with QtrCtrl.
    See the description of QtrCtrl below. NibCtrl is only used for SIMD4
    instructions with a DF (Double Float) source or destination.

    Programming Notes:
    Note that if eighths are given zero-based indices from 0 to 7, then NibCtrl
    = 0 indicates even indices and NibCtrl = 1 indicates odd indices.
    \*****************************************************************************/
    DWORD ControlsA_Nibctrl : BITFIELD_BIT(3); // NIBCTRL

    /*****************************************************************************\
     - Quarter Control. This field provides explicit control for ARF selection.
    This field combined with NibCtrl and ExecSize determines which channels are
    used for the ARF registers.
    \*****************************************************************************/
    DWORD ControlsA_Qtrctrl : BITFIELD_RANGE(4, 5); // QTRCTRL

    /*****************************************************************************\
    Thread Control. This field provides explicit control for thread switching.
    If this field is set to 00b, it is up to the GEN execution units to manage
    thread switching. This is the normal (and unnamed) mode. In this mode, for
    example, if the current instruction cannot proceed due to operand
    dependencies, the EU switches to the next available thread to fill the
    compute pipe. In another example, if the current instruction is ready to go,
    however, there is another thread with higher priority that also has an
    instruction ready, the EU switches to that thread. If this field is set to
    Switch, a forced thread switch occurs after the current instruction is
    executed and before the next instruction. In addition, a long delay (longer
    than the execution pipe latency) is introduced for the current thread.
    Particularly, the instruction queue of the current thread is flushed after
    the current instruction is dispatched for execution. Switch is designed
    primarily as a safety feature in case there are race conditions for certain
    instructions.
    \*****************************************************************************/
    DWORD ControlsA_ThreadControl : BITFIELD_RANGE(6, 7); // THREADCTRL

    /*****************************************************************************\
    This field determines the number of channels operating in parallel for this
    instruction.  The size cannot exceed the maximum number of channels allowed
    for the given data type.
    \*****************************************************************************/
    DWORD ControlsA_Execsize : BITFIELD_RANGE(13, 15);           // EXECSIZE
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(19, 23); // Override
    DWORD SharedFunctionIdSfid : BITFIELD_RANGE(24, 27);         // SFID

    /*****************************************************************************\
    AccWrCtrl. This field allows per instruction accumulator write control.
    \*****************************************************************************/
    DWORD ControlsB_Accwrctrl : BITFIELD_BIT(28); // ACCWRCTRL

    /*****************************************************************************\
    Compaction Control
    Indicates whether the instruction is compacted to the 64-bit compact
    instruction format. When this bit is set, the 64-bit compact instruction
    format is used. The EU decodes the compact format using lookup tables
    internal to the hardware, but documented for use by software tools. Only
    some instruction variations can be compacted, the variations supported by
    those lookup tables and the compact format. See EU Compact Instruction
    Format [DevBDW+] for more information.
    \*****************************************************************************/
    DWORD ControlsB_Cmptctrl : BITFIELD_BIT(29); // CMPTCTRL

    /*****************************************************************************\
    This field allows the insertion of a breakpoint at the current instruction.
    When the bit is set, hardware automatically stores the current IP in CR
    register and jumps to the System IP (SIP) BEFORE executing the current
    instruction.
    \*****************************************************************************/
    DWORD ControlsB_Debugctrl : BITFIELD_BIT(30); // DEBUGCTRL
    EU_INSTRUCTION_OPERAND_CONTROLS OperandControl;
    DWORD Exdesc1916 : BITFIELD_RANGE(64, 67); // ExMsgDescpt[19:16]
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(68, 79); // MBZ
    DWORD Exdesc2320 : BITFIELD_RANGE(80, 83);           // ExtMsgDescpt[23:20]
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(84); // MBZ
    DWORD Exdesc2724 : BITFIELD_RANGE(85, 88);           // ExtMsgDescpt[27:24]
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_RANGE(89, 90); // MBZ
    DWORD Exdesc3028 : BITFIELD_RANGE(91, 93); // ExtMsgDescpt[31:28]
    DWORD Exdesc1111 : BITFIELD_BIT(94); // LOD COMPENSATION BIT, MANUALLY ADDED
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(95); // MBZ
    EU_INSTRUCTION_OPERAND_SEND_MSG Message;
  } Common;
  struct tagPropertyPredicationIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7); // Override

    /*****************************************************************************\
    This field, together with PredInv, enables and controls the generation of
    the predication mask for the instruction.  It allows per-channel conditional
    execution of the instruction based on the content of the selected flag
    register.  Encoding depends on the access mode. In Align16 access mode,
    there are eight encodings (including no predication). All encodings are
    based on group-of-4 predicate bits, including channel sequential,
    replication swizzles and horizontal any|all operations.  The same
    configuration is repeated for each group-of-4 execution channels.
    \*****************************************************************************/
    QWORD ControlsA_Predctrl : BITFIELD_RANGE(8, 11); // PREDCTRL

    /*****************************************************************************\
    This field, together with PredCtrl, enables and controls the generation of
    the predication mask for the instruction.  When it is set, the predication
    uses the inverse of the predication bits generated according to setting of
    Predicate Control. In other words, effect of PredInv happens after PredCtrl.
    This field is ignored by hardware if Predicate Control is set to 0000  -
    there is no predication. PMask is the final predication mask produced by the
    effects of both fields.
    \*****************************************************************************/
    QWORD ControlsA_Predinv : BITFIELD_BIT(12);                   // PREDINV
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override

  } PropertyPredicationIsTrue;
  struct tagPropertyPredicationIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);    // Override
    QWORD ControlsA__ : BITFIELD_RANGE(8, 11);                    // PREDCTRL
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(12);          //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } PropertyPredicationIsFalse;
  struct tagPropertySaturationIsTrue {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override

    /*****************************************************************************\
    Enables or disables destination saturation.
    When it is set, output values to the destination register are saturated. The
    saturation operation depends on the destination data type. Saturation is the
    operation that converts any value outside the saturation target range for
    the data type to the closest value in the target range. For a floating-point
    destination type, the saturation target range is [0.0, 1.0]. For a
    floating-point NaN, there is no  -closest value -; any NaN saturates to 0.0.
    Note that enabling Saturate overrides all of the NaN propagation behaviors
    described for various numeric instructions. Any floating-point number
    greater than 1.0, including +INF, saturates to 1.0. Any negative
    floating-point number, including -INF, saturates to 0.0. Any floating-point
    number in the range 0.0 to 1.0 is not changed by saturation. For an integer
    destination type, the maximum range for that type is the saturation target
    range. For example, the saturation range for B (Signed Byte Integer) is
    [-128, 127]. When Saturate is clear, destination values are not saturated.
    For example, a wrapped result (modulo) is output to the destination for an
    overflowed integer value. See the Numeric Data Typessection for information
    about data types and their ranges.
    \*****************************************************************************/
    DWORD ControlsB_Saturate : BITFIELD_BIT(31); // SATURATE
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsTrue;
  struct tagPropertySaturationIsFalse {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(31);        // MBZ
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsFalse;
  DWORD RawData[4];

  // STRUCTURE DEBUG
  //
  //__CODEGEN_DebugType(EU_INSTRUCTION_SEND) {
  //    __CODEGEN_DebugAttributeEnum(Opcode, 0, 6, EU_OPCODE);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_Accessmode, 0, 0, ACCESSMODE);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_Depctrl, 1, 2, DEPCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_Nibctrl, 3, 3, NIBCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_Qtrctrl, 4, 5, QTRCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_ThreadControl, 6, 7, THREADCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsA_Execsize, 13, 15, EXECSIZE);
  //    __CODEGEN_DebugAttributeEnum(SharedFunctionIdSfid, 24, 27, SFID);
  //    __CODEGEN_DebugAttributeEnum(ControlsB_Accwrctrl, 28, 28, ACCWRCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsB_Cmptctrl, 29, 29, CMPTCTRL);
  //    __CODEGEN_DebugAttributeEnum(ControlsB_Debugctrl, 30, 30, DEBUGCTRL);
  //    __CODEGEN_DebugAttributeStruct(OperandControl);
  //    __CODEGEN_DebugAttributeUInt(Exdesc1916, 64, 67);
  //    __CODEGEN_DebugAttributeUInt(Exdesc2320, 80, 83);
  //    __CODEGEN_DebugAttributeUInt(Exdesc2724, 85, 88);
  //    __CODEGEN_DebugAttributeUInt(Exdesc3128, 91, 94);
  //    __CODEGEN_DebugAttributeStruct(Message);
  //}

  // LOCAL ENUMERATIONS
  //
  typedef enum tagACCESSMODE {
    ACCESSMODE_ALIGN1 = 0x0,
    ACCESSMODE_ALIGN16 = 0x1,
  } ACCESSMODE;

  typedef enum tagNIBCTRL {
    NIBCTRL_ODD = 0x0,  // Use an odd 1/8th for DMask/VMask and ARF (first,
                        // third, fifth, or seventh depending on QtrCtrl).
    NIBCTRL_EVEN = 0x1, // Use an even 1/8th for DMask/VMask and ARF (second,
                        // fourth, sixth, or eighth depending on QtrCtrl).
  } NIBCTRL;

  // typedef enum tagACCWRCTRL {
  //     ACCWRCTRL_DON_T_WRITE_TO_ACC = 0x0,
  //     ACCWRCTRL_UPDATE_ACC         = 0x1,    // Write result to the ACC, and
  //     destination
  // } ACCWRCTRL;

  typedef enum tagCMPTCTRL {
    CMPTCTRL_NOCOMPACTION = 0x0, // No compaction. 128-bit native instruction
                                 // supporting all instruction options.
    CMPTCTRL_COMPACTED =
        0x1, // Compaction is enabled. 64-bit compact instruction supporting
             // only some instruction variations.
  } CMPTCTRL;

  typedef enum tagDEBUGCTRL {
    DEBUGCTRL_NO_BREAKPOINT = 0x0,
    DEBUGCTRL_BREAKPOINT = 0x1,
  } DEBUGCTRL;

  typedef enum tagPREDINV {
    PREDINV_POSITIVE = 0x0, // Positive polarity of predication. Use the
                            // predication mask produced by PredCtrl
    PREDINV_NEGATIVE = 0x1, // Negative polarity of predication. If PredCtrl is
                            // nonzero, invert the predication mask.
  } PREDINV;

  // typedef enum tagSATURATE {
  //     SATURATE_NO_DESTINATION_MODIFICATION = 0x0,
  //     SATURATE_SAT                         = 0x1,    // Saturate the output
  // } SATURATE;

  // ENUMERATION DEBUG
  //
  //__CODEGEN_DebugEnum(ACCESSMODE) {
  //    __CODEGEN_DebugEnumValue(ACCESSMODE_ALIGN1);
  //    __CODEGEN_DebugEnumValue(ACCESSMODE_ALIGN16);
  //}
  //
  //__CODEGEN_DebugEnum(NIBCTRL) {
  //    __CODEGEN_DebugEnumValue(NIBCTRL_ODD);
  //    __CODEGEN_DebugEnumValue(NIBCTRL_EVEN);
  //}
  //
  //__CODEGEN_DebugEnum(ACCWRCTRL) {
  //    __CODEGEN_DebugEnumValue(ACCWRCTRL_DON_T_WRITE_TO_ACC);
  //    __CODEGEN_DebugEnumValue(ACCWRCTRL_UPDATE_ACC);
  //}
  //
  //__CODEGEN_DebugEnum(CMPTCTRL) {
  //    __CODEGEN_DebugEnumValue(CMPTCTRL_NOCOMPACTION);
  //    __CODEGEN_DebugEnumValue(CMPTCTRL_COMPACTED);
  //}
  //
  //__CODEGEN_DebugEnum(DEBUGCTRL) {
  //    __CODEGEN_DebugEnumValue(DEBUGCTRL_NO_BREAKPOINT);
  //    __CODEGEN_DebugEnumValue(DEBUGCTRL_BREAKPOINT);
  //}
  //
  //__CODEGEN_DebugEnum(PREDINV) {
  //    __CODEGEN_DebugEnumValue(PREDINV_POSITIVE);
  //    __CODEGEN_DebugEnumValue(PREDINV_NEGATIVE);
  //}
  //
  //__CODEGEN_DebugEnum(SATURATE) {
  //    __CODEGEN_DebugEnumValue(SATURATE_NO_DESTINATION_MODIFICATION);
  //    __CODEGEN_DebugEnumValue(SATURATE_SAT);
  //}

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Opcode = 0x0;
    RawData[1] = 0x0;
    Common.ControlsA_Accessmode = ACCESSMODE_ALIGN1;
    Common.ControlsA_Depctrl = 0x0;
    Common.ControlsA_Nibctrl = NIBCTRL_ODD;
    Common.ControlsA_Qtrctrl = 0x0;
    Common.ControlsA_ThreadControl = 0x0;
    Common.ControlsA_Execsize = 0x0;
    Common.SharedFunctionIdSfid = 0x0;
    Common.ControlsB_Accwrctrl = ACCWRCTRL_DON_T_WRITE_TO_ACC;
    Common.ControlsB_Cmptctrl = CMPTCTRL_NOCOMPACTION;
    Common.ControlsB_Debugctrl = DEBUGCTRL_NO_BREAKPOINT;
    RawData[2] = 0x0;
    Common.OperandControl.Init();
    RawData[3] = 0x0;
    Common.Exdesc1916 = 0x0;
    Common.Exdesc2320 = 0x0;
    Common.Exdesc2724 = 0x0;
    Common.Exdesc3028 = 0x0;
    Common.Exdesc1111 = 0x0;
    Common.Message.Init();
  }

  static tagEU_INSTRUCTION_SEND sInit() {
    tagEU_INSTRUCTION_SEND state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    Common.Opcode = OpcodeValue;
  }

  __CODEGEN_INLINE ACCESSMODE GetControlsA_Accessmode() {
    __CODEGEN_GET_MACRO();
    return (ACCESSMODE)Common.ControlsA_Accessmode;
  }

  __CODEGEN_INLINE void
  SetControlsA_Accessmode(ACCESSMODE ControlsA_AccessmodeValue) {
    __CODEGEN_SET_MACRO(ControlsA_AccessmodeValue);
    Common.ControlsA_Accessmode = ControlsA_AccessmodeValue;
  }

  __CODEGEN_INLINE DEPCTRL GetControlsA_Depctrl() {
    __CODEGEN_GET_MACRO();
    return (DEPCTRL)Common.ControlsA_Depctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Depctrl(DEPCTRL ControlsA_DepctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_DepctrlValue);
    Common.ControlsA_Depctrl = ControlsA_DepctrlValue;
  }

  __CODEGEN_INLINE NIBCTRL GetControlsA_Nibctrl() {
    __CODEGEN_GET_MACRO();
    return (NIBCTRL)Common.ControlsA_Nibctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Nibctrl(NIBCTRL ControlsA_NibctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_NibctrlValue);
    Common.ControlsA_Nibctrl = ControlsA_NibctrlValue;
  }

  __CODEGEN_INLINE QTRCTRL GetControlsA_Qtrctrl() {
    __CODEGEN_GET_MACRO();
    return (QTRCTRL)Common.ControlsA_Qtrctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Qtrctrl(QTRCTRL ControlsA_QtrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_QtrctrlValue);
    Common.ControlsA_Qtrctrl = ControlsA_QtrctrlValue;
  }

  __CODEGEN_INLINE THREADCTRL GetControlsA_ThreadControl() {
    __CODEGEN_GET_MACRO();
    return (THREADCTRL)Common.ControlsA_ThreadControl;
  }

  __CODEGEN_INLINE void
  SetControlsA_ThreadControl(THREADCTRL ControlsA_ThreadControlValue) {
    __CODEGEN_SET_MACRO(ControlsA_ThreadControlValue);
    Common.ControlsA_ThreadControl = ControlsA_ThreadControlValue;
  }

  __CODEGEN_INLINE EXECSIZE GetControlsA_Execsize() {
    __CODEGEN_GET_MACRO();
    return (EXECSIZE)Common.ControlsA_Execsize;
  }

  __CODEGEN_INLINE void
  SetControlsA_Execsize(EXECSIZE ControlsA_ExecsizeValue) {
    __CODEGEN_SET_MACRO(ControlsA_ExecsizeValue);
    Common.ControlsA_Execsize = ControlsA_ExecsizeValue;
  }

  __CODEGEN_INLINE SFID GetSharedFunctionIdSfid() {
    __CODEGEN_GET_MACRO();
    return (SFID)Common.SharedFunctionIdSfid;
  }

  __CODEGEN_INLINE void
  SetSharedFunctionIdSfid(SFID SharedFunctionIdSfidValue) {
    __CODEGEN_SET_MACRO(SharedFunctionIdSfidValue);
    Common.SharedFunctionIdSfid = SharedFunctionIdSfidValue;
  }

  __CODEGEN_INLINE ACCWRCTRL GetControlsB_Accwrctrl() {
    __CODEGEN_GET_MACRO();
    return (ACCWRCTRL)Common.ControlsB_Accwrctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Accwrctrl(ACCWRCTRL ControlsB_AccwrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_AccwrctrlValue);
    Common.ControlsB_Accwrctrl = ControlsB_AccwrctrlValue;
  }

  __CODEGEN_INLINE CMPTCTRL GetControlsB_Cmptctrl() {
    __CODEGEN_GET_MACRO();
    return (CMPTCTRL)Common.ControlsB_Cmptctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Cmptctrl(CMPTCTRL ControlsB_CmptctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_CmptctrlValue);
    Common.ControlsB_Cmptctrl = ControlsB_CmptctrlValue;
  }

  __CODEGEN_INLINE DEBUGCTRL GetControlsB_Debugctrl() {
    __CODEGEN_GET_MACRO();
    return (DEBUGCTRL)Common.ControlsB_Debugctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Debugctrl(DEBUGCTRL ControlsB_DebugctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_DebugctrlValue);
    Common.ControlsB_Debugctrl = ControlsB_DebugctrlValue;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_CONTROLS &GetOperandControl() {
    __CODEGEN_GET_MACRO();
    return Common.OperandControl;
  }

  __CODEGEN_INLINE QWORD GetExdesc1916() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc1916 << 16;
  }

  __CODEGEN_INLINE void SetExdesc1916(QWORD Exdesc1916Value) {
    __CODEGEN_SET_MACRO(Exdesc1916Value);
    Common.Exdesc1916 = Exdesc1916Value >> 16;
  }

  __CODEGEN_INLINE QWORD GetExdesc2320() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc2320 << 20;
  }

  __CODEGEN_INLINE void SetExdesc2320(QWORD Exdesc2320Value) {
    __CODEGEN_SET_MACRO(Exdesc2320Value);
    Common.Exdesc2320 = Exdesc2320Value >> 20;
  }

  __CODEGEN_INLINE QWORD GetExdesc2724() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc2724 << 24;
  }

  __CODEGEN_INLINE void SetExdesc2724(QWORD Exdesc2724Value) {
    __CODEGEN_SET_MACRO(Exdesc2724Value);
    Common.Exdesc2724 = Exdesc2724Value >> 24;
  }

  __CODEGEN_INLINE QWORD GetExdesc3028() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc3028 << 28;
  }

  __CODEGEN_INLINE void SetExdesc3028(QWORD Exdesc3028Value) {
    __CODEGEN_SET_MACRO(Exdesc3028Value);
    Common.Exdesc3028 = Exdesc3028Value >> 28;
  }

  __CODEGEN_INLINE void SetExdesc1111(QWORD Exdesc1111Value) {
    __CODEGEN_SET_MACRO(Exdesc1111Value);
    Common.Exdesc1111 = Exdesc1111Value >> 11;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_SEND_MSG &GetMessage() {
    __CODEGEN_GET_MACRO();
    return Common.Message;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA_Predctrl() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsTrue.ControlsA_Predctrl;
  }

  __CODEGEN_INLINE void
  SetControlsA_Predctrl(PREDCTRL ControlsA_PredctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredctrlValue);
    PropertyPredicationIsTrue.ControlsA_Predctrl = ControlsA_PredctrlValue;
  }

  __CODEGEN_INLINE PREDINV GetControlsA_Predinv() {
    __CODEGEN_GET_MACRO();
    return (PREDINV)PropertyPredicationIsTrue.ControlsA_Predinv;
  }

  __CODEGEN_INLINE void SetControlsA_Predinv(PREDINV ControlsA_PredinvValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredinvValue);
    PropertyPredicationIsTrue.ControlsA_Predinv = ControlsA_PredinvValue;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA__() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsFalse.ControlsA__;
  }

  __CODEGEN_INLINE void SetControlsA__(PREDCTRL ControlsA__Value) {
    __CODEGEN_SET_MACRO(ControlsA__Value);
    PropertyPredicationIsFalse.ControlsA__ = ControlsA__Value;
  }

  __CODEGEN_INLINE SATURATE GetControlsB_Saturate() {
    __CODEGEN_GET_MACRO();
    return (SATURATE)PropertySaturationIsTrue.ControlsB_Saturate;
  }

  __CODEGEN_INLINE void
  SetControlsB_Saturate(SATURATE ControlsB_SaturateValue) {
    __CODEGEN_SET_MACRO(ControlsB_SaturateValue);
    PropertySaturationIsTrue.ControlsB_Saturate = ControlsB_SaturateValue;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SEND;

static_assert(16 == sizeof(EU_INSTRUCTION_SEND));

typedef union tagEU_INSTRUCTION_SENDS {
  struct tagCommon {
    DWORD Opcode : BITFIELD_RANGE(0, 6);                // EU_OPCODE
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(7); // MBZ

    /*****************************************************************************\
    Access Mode. This field determines the operand access for the instruction.
    It applies to all source and destination operands. When it is cleared
    (Align1), the instruction uses byte-aligned addressing for source and
    destination operands. Source swizzle control and destination mask control
    are not supported. When it is set (Align16), the instruction uses
    16-byte-aligned addressing for all source and destination operands. Source
    swizzle control and destination mask control are supported in this mode.
    \*****************************************************************************/
    DWORD ControlsA_Accessmode : BITFIELD_BIT(0); // ACCESSMODE

    /*****************************************************************************\
    Destination Dependency Control. This field selectively disables destination
    dependency check and clear for this instruction. When it is set to 00,
    normal destination dependency control is performed for the instruction  -
    hardware checks for destination hazards to ensure data integrity.
    Specifically, destination register dependency check is conducted before the
    instruction is made ready for execution. After the instruction is executed,
    the destination register scoreboard will be cleared when the destination
    operands retire. When bit 10 is set (NoDDClr), the destination register
    scoreboard will NOT be cleared when the destination operands retire.  When
    bit 11 is set (NoDDChk), hardware does not check for destination register
    dependency before the instruction is made ready for execution.  NoDDClr and
    NoDDChk are not mutual exclusive. When this field is not all-zero, hardware
    does not protect against destination hazards for the instruction.  This is
    typically used to assemble data in a fine grained fashion (e.g.
    matrix-vector compute with dot-product instructions), where the data
    integrity is guaranteed by software based on the intended usage of
    instruction sequences.
    \*****************************************************************************/
    DWORD ControlsA_Depctrl : BITFIELD_RANGE(1, 2); // DEPCTRL

    /*****************************************************************************\
    Nibble Control. This field is used in some instructions along with QtrCtrl.
    See the description of QtrCtrl below. NibCtrl is only used for SIMD4
    instructions with a DF (Double Float) source or destination.

    Programming Notes:
    Note that if eighths are given zero-based indices from 0 to 7, then NibCtrl
    = 0 indicates even indices and NibCtrl = 1 indicates odd indices.
    \*****************************************************************************/
    DWORD ControlsA_Nibctrl : BITFIELD_BIT(3); // NIBCTRL

    /*****************************************************************************\
     - Quarter Control. This field provides explicit control for ARF selection.
    This field combined with NibCtrl and ExecSize determines which channels are
    used for the ARF registers.
    \*****************************************************************************/
    DWORD ControlsA_Qtrctrl : BITFIELD_RANGE(4, 5); // QTRCTRL

    /*****************************************************************************\
    Thread Control. This field provides explicit control for thread switching.
    If this field is set to 00b, it is up to the GEN execution units to manage
    thread switching. This is the normal (and unnamed) mode. In this mode, for
    example, if the current instruction cannot proceed due to operand
    dependencies, the EU switches to the next available thread to fill the
    compute pipe. In another example, if the current instruction is ready to go,
    however, there is another thread with higher priority that also has an
    instruction ready, the EU switches to that thread. If this field is set to
    Switch, a forced thread switch occurs after the current instruction is
    executed and before the next instruction. In addition, a long delay (longer
    than the execution pipe latency) is introduced for the current thread.
    Particularly, the instruction queue of the current thread is flushed after
    the current instruction is dispatched for execution. Switch is designed
    primarily as a safety feature in case there are race conditions for certain
    instructions.
    \*****************************************************************************/
    DWORD ControlsA_ThreadControl : BITFIELD_RANGE(6, 7); // THREADCTRL

    /*****************************************************************************\
    This field determines the number of channels operating in parallel for this
    instruction.  The size cannot exceed the maximum number of channels allowed
    for the given data type.
    \*****************************************************************************/
    DWORD ControlsA_Execsize : BITFIELD_RANGE(13, 15);           // EXECSIZE
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(19, 23); // Override
    DWORD SharedFunctionIdSfid : BITFIELD_RANGE(24, 27);         // SFID

    /*****************************************************************************\
    AccWrCtrl. This field allows per instruction accumulator write control.
    \*****************************************************************************/
    DWORD ControlsB_Accwrctrl : BITFIELD_BIT(28); // ACCWRCTRL

    /*****************************************************************************\
    Compaction Control
    Indicates whether the instruction is compacted to the 64-bit compact
    instruction format. When this bit is set, the 64-bit compact instruction
    format is used. The EU decodes the compact format using lookup tables
    internal to the hardware, but documented for use by software tools. Only
    some instruction variations can be compacted, the variations supported by
    those lookup tables and the compact format. See EU Compact Instruction
    Format [DevBDW+] for more information.
    \*****************************************************************************/
    DWORD ControlsB_Cmptctrl : BITFIELD_BIT(29); // CMPTCTRL

    /*****************************************************************************\
    This field allows the insertion of a breakpoint at the current instruction.
    When the bit is set, hardware automatically stores the current IP in CR
    register and jumps to the System IP (SIP) BEFORE executing the current
    instruction.
    \*****************************************************************************/
    DWORD ControlsB_Debugctrl : BITFIELD_BIT(30); // DEBUGCTRL
    DWORD Exdesc1111 : BITFIELD_BIT(31); // LOD COMPENSATION BIT, MANUALLY ADDED
    DWORD FlagRegisterNumberSubregisterNumber : BITFIELD_RANGE(32, 33); //
    DWORD Maskctrl : BITFIELD_BIT(34);                                  //
    DWORD DestinationRegisterFile : BITFIELD_BIT(35);            // RegFile[0]
    DWORD Source1RegisterFile : BITFIELD_BIT(36);                // RegFile[0]
    DWORD DestinationType : BITFIELD_RANGE(37, 40);              //
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(41);       // Override
    DWORD Source1AddressingMode : BITFIELD_BIT(42);              // ADDRMODE
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(43, 60); // Override
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(61);         // MBZ
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(62);       // Override
    DWORD DestinationAddressingMode : BITFIELD_BIT(63);          // ADDRMODE
    DWORD Exdesc96 : BITFIELD_RANGE(64, 67); // ExtMsgDescpt[9:6]
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(68, 76); // Override
    DWORD Selreg32desc : BITFIELD_BIT(77);                       //
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(78);       // Override
    DWORD Source0AddressingMode : BITFIELD_BIT(79);              // ADDRMODE
    DWORD Exdesc3116 : BITFIELD_RANGE(80, 95); // ExtMsgDescpt[31:16]
    EU_INSTRUCTION_OPERAND_SEND_MSG Message;
  } Common;
  struct tagPropertyPredicationIsTrue {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7); // Override

    /*****************************************************************************\
    This field, together with PredInv, enables and controls the generation of
    the predication mask for the instruction.  It allows per-channel conditional
    execution of the instruction based on the content of the selected flag
    register.  Encoding depends on the access mode. In Align16 access mode,
    there are eight encodings (including no predication). All encodings are
    based on group-of-4 predicate bits, including channel sequential,
    replication swizzles and horizontal any|all operations.  The same
    configuration is repeated for each group-of-4 execution channels.
    \*****************************************************************************/
    QWORD ControlsA_Predctrl : BITFIELD_RANGE(8, 11); // PREDCTRL

    /*****************************************************************************\
    This field, together with PredCtrl, enables and controls the generation of
    the predication mask for the instruction.  When it is set, the predication
    uses the inverse of the predication bits generated according to setting of
    Predicate Control. In other words, effect of PredInv happens after PredCtrl.
    This field is ignored by hardware if Predicate Control is set to 0000  -
    there is no predication. PMask is the final predication mask produced by the
    effects of both fields.
    \*****************************************************************************/
    QWORD ControlsA_Predinv : BITFIELD_BIT(12);                   // PREDINV
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } PropertyPredicationIsTrue;
  struct tagPropertyPredicationIsFalse {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 7);    // Override
    QWORD ControlsA__ : BITFIELD_RANGE(8, 11);                    // PREDCTRL
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(12);          //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(13, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } PropertyPredicationIsFalse;
  struct tagPropertySaturationIsTrue {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override

    /*****************************************************************************\
    Enables or disables destination saturation.
    When it is set, output values to the destination register are saturated. The
    saturation operation depends on the destination data type. Saturation is the
    operation that converts any value outside the saturation target range for
    the data type to the closest value in the target range. For a floating-point
    destination type, the saturation target range is [0.0, 1.0]. For a
    floating-point NaN, there is no  -closest value -; any NaN saturates to 0.0.
    Note that enabling Saturate overrides all of the NaN propagation behaviors
    described for various numeric instructions. Any floating-point number
    greater than 1.0, including +INF, saturates to 1.0. Any negative
    floating-point number, including -INF, saturates to 0.0. Any floating-point
    number in the range 0.0 to 1.0 is not changed by saturation. For an integer
    destination type, the maximum range for that type is the saturation target
    range. For example, the saturation range for B (Signed Byte Integer) is
    [-128, 127]. When Saturate is clear, destination values are not saturated.
    For example, a wrapped result (modulo) is output to the destination for an
    overflowed integer value. See the Numeric Data Typessection for information
    about data types and their ranges.
    \*****************************************************************************/
    DWORD ControlsB_Saturate : BITFIELD_BIT(31); // SATURATE
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsTrue;
  struct tagPropertySaturationIsFalse {
    DWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 30); // Override
    DWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(31);        // MBZ
    DWORD __CODEGEN_UNIQUE(Overridden)[3];
  } PropertySaturationIsFalse;
  struct tagSource1AddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 40);     // Override
    QWORD Source1AddressImmediateSign9 : BITFIELD_BIT(41);          // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(42);          // Override
    QWORD Source1AddressImmediate84 : BITFIELD_RANGE(43, 47);       // S9[8:4]
    QWORD Source1AddressSubregisterNumber : BITFIELD_RANGE(48, 51); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(52, 63);    // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127);   // Override
  } Source1AddressingModeIsIndirect;
  struct tagSource1AddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 40);   // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(41);          // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(42);        // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(43);          // MBZ
    QWORD Source1RegisterNumber : BITFIELD_RANGE(44, 51);         //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(52, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } Source1AddressingModeIsDirect;
  struct tagDestinationAddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 51);   // Override
    QWORD DestinationSubregisterNumber4 : BITFIELD_BIT(52);       //
    QWORD DestinationRegisterNumber : BITFIELD_RANGE(53, 60);     //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(61);        // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(62);          // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(63, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } DestinationAddressingModeIsDirect;
  struct tagDestinationAddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 51);   // Override
    QWORD DestinationAddressImmediate84 : BITFIELD_RANGE(52, 56); // S9[8:4]
    QWORD DestinationAddressSubregisterNumber : BITFIELD_RANGE(57, 60); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(61);        // Override
    QWORD DestinationAddressImmediateSign9 : BITFIELD_BIT(62);    // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(63, 63);  // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 127); // Override
  } DestinationAddressingModeIsIndirect;
  struct tagSource0AddressingModeIsDirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);   // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 67);  // Override
    QWORD Source0SubregisterNumber : BITFIELD_BIT(68);            //
    QWORD Source0RegisterNumber : BITFIELD_RANGE(69, 76);         //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(77);        // Override
    QWORD __CODEGEN_UNIQUE(Reserved) : BITFIELD_BIT(78);          // MBZ
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 127); // Override
  } Source0AddressingModeIsDirect;
  struct tagSource0AddressingModeIsIndirect {
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(0, 63);     // Override
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(64, 67);    // Override
    QWORD Source0AddressImmediate84 : BITFIELD_RANGE(68, 72);       // S9[8:4]
    QWORD Source0AddressSubregisterNumber : BITFIELD_RANGE(73, 76); //
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_BIT(77);          // Override
    QWORD Source0AddressImmediateSign9 : BITFIELD_BIT(78);          // S9[9]
    QWORD __CODEGEN_UNIQUE(Overridden) : BITFIELD_RANGE(79, 127);   // Override
  } Source0AddressingModeIsIndirect;
  DWORD RawData[4];

  // INITIALIZATION
  //
  __CODEGEN_INLINE void Init() {
    RawData[0] = 0x0;
    Common.Opcode = 0x0;
    RawData[1] = 0x0;
    Common.ControlsA_Accessmode = ACCESSMODE_ALIGN1;
    Common.ControlsA_Depctrl = 0x0;
    Common.ControlsA_Nibctrl = NIBCTRL_ODD;
    Common.ControlsA_Qtrctrl = 0x0;
    Common.ControlsA_ThreadControl = 0x0;
    Common.ControlsA_Execsize = 0x0;
    Common.SharedFunctionIdSfid = 0x0;
    Common.ControlsB_Accwrctrl = ACCWRCTRL_DON_T_WRITE_TO_ACC;
    Common.ControlsB_Cmptctrl = CMPTCTRL_NOCOMPACTION;
    Common.ControlsB_Debugctrl = DEBUGCTRL_NO_BREAKPOINT;
    RawData[2] = 0x0;
    Common.Exdesc1111 = 0x0;
    Common.FlagRegisterNumberSubregisterNumber = 0x0;
    Common.Maskctrl = 0x0;
    Common.DestinationRegisterFile = 0x0;
    Common.Source1RegisterFile = 0x0;
    Common.DestinationType = 0x0;
    Common.Source1AddressingMode = 0x0;
    Common.DestinationAddressingMode = 0x0;
    RawData[3] = 0x0;
    Common.Exdesc96 = 0x0;
    Common.Selreg32desc = 0x0;
    Common.Source0AddressingMode = 0x0;
    Common.Exdesc3116 = 0x0;
    Common.Message.Init();
  }

  static tagEU_INSTRUCTION_SENDS sInit() {
    tagEU_INSTRUCTION_SENDS state;
    state.Init();
    return state;
  }

  // ACCESSORS
  //
  __CODEGEN_INLINE __CODEGEN_PACKED DWORD &GetDWORD(UINT const index) {
    return RawData[index];
  }

  static size_t CalculateSize(UINT const entryCount = 0) { return 0; }

  __CODEGEN_INLINE size_t GetSize() { return 0; }

  __CODEGEN_INLINE EU_OPCODE GetOpcode() {
    __CODEGEN_GET_MACRO();
    return (EU_OPCODE)Common.Opcode;
  }

  __CODEGEN_INLINE void SetOpcode(EU_OPCODE OpcodeValue) {
    __CODEGEN_SET_MACRO(OpcodeValue);
    Common.Opcode = OpcodeValue;
  }

  __CODEGEN_INLINE ACCESSMODE GetControlsA_Accessmode() {
    __CODEGEN_GET_MACRO();
    return (ACCESSMODE)Common.ControlsA_Accessmode;
  }

  __CODEGEN_INLINE void
  SetControlsA_Accessmode(ACCESSMODE ControlsA_AccessmodeValue) {
    __CODEGEN_SET_MACRO(ControlsA_AccessmodeValue);
    Common.ControlsA_Accessmode = ControlsA_AccessmodeValue;
  }

  __CODEGEN_INLINE DEPCTRL GetControlsA_Depctrl() {
    __CODEGEN_GET_MACRO();
    return (DEPCTRL)Common.ControlsA_Depctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Depctrl(DEPCTRL ControlsA_DepctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_DepctrlValue);
    Common.ControlsA_Depctrl = ControlsA_DepctrlValue;
  }

  __CODEGEN_INLINE NIBCTRL GetControlsA_Nibctrl() {
    __CODEGEN_GET_MACRO();
    return (NIBCTRL)Common.ControlsA_Nibctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Nibctrl(NIBCTRL ControlsA_NibctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_NibctrlValue);
    Common.ControlsA_Nibctrl = ControlsA_NibctrlValue;
  }

  __CODEGEN_INLINE QTRCTRL GetControlsA_Qtrctrl() {
    __CODEGEN_GET_MACRO();
    return (QTRCTRL)Common.ControlsA_Qtrctrl;
  }

  __CODEGEN_INLINE void SetControlsA_Qtrctrl(QTRCTRL ControlsA_QtrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_QtrctrlValue);
    Common.ControlsA_Qtrctrl = ControlsA_QtrctrlValue;
  }

  __CODEGEN_INLINE THREADCTRL GetControlsA_ThreadControl() {
    __CODEGEN_GET_MACRO();
    return (THREADCTRL)Common.ControlsA_ThreadControl;
  }

  __CODEGEN_INLINE void
  SetControlsA_ThreadControl(THREADCTRL ControlsA_ThreadControlValue) {
    __CODEGEN_SET_MACRO(ControlsA_ThreadControlValue);
    Common.ControlsA_ThreadControl = ControlsA_ThreadControlValue;
  }

  __CODEGEN_INLINE EXECSIZE GetControlsA_Execsize() {
    __CODEGEN_GET_MACRO();
    return (EXECSIZE)Common.ControlsA_Execsize;
  }

  __CODEGEN_INLINE void
  SetControlsA_Execsize(EXECSIZE ControlsA_ExecsizeValue) {
    __CODEGEN_SET_MACRO(ControlsA_ExecsizeValue);
    Common.ControlsA_Execsize = ControlsA_ExecsizeValue;
  }

  __CODEGEN_INLINE SFID GetSharedFunctionIdSfid() {
    __CODEGEN_GET_MACRO();
    return (SFID)Common.SharedFunctionIdSfid;
  }

  __CODEGEN_INLINE void
  SetSharedFunctionIdSfid(SFID SharedFunctionIdSfidValue) {
    __CODEGEN_SET_MACRO(SharedFunctionIdSfidValue);
    Common.SharedFunctionIdSfid = SharedFunctionIdSfidValue;
  }

  __CODEGEN_INLINE ACCWRCTRL GetControlsB_Accwrctrl() {
    __CODEGEN_GET_MACRO();
    return (ACCWRCTRL)Common.ControlsB_Accwrctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Accwrctrl(ACCWRCTRL ControlsB_AccwrctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_AccwrctrlValue);
    Common.ControlsB_Accwrctrl = ControlsB_AccwrctrlValue;
  }

  __CODEGEN_INLINE CMPTCTRL GetControlsB_Cmptctrl() {
    __CODEGEN_GET_MACRO();
    return (CMPTCTRL)Common.ControlsB_Cmptctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Cmptctrl(CMPTCTRL ControlsB_CmptctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_CmptctrlValue);
    Common.ControlsB_Cmptctrl = ControlsB_CmptctrlValue;
  }

  __CODEGEN_INLINE DEBUGCTRL GetControlsB_Debugctrl() {
    __CODEGEN_GET_MACRO();
    return (DEBUGCTRL)Common.ControlsB_Debugctrl;
  }

  __CODEGEN_INLINE void
  SetControlsB_Debugctrl(DEBUGCTRL ControlsB_DebugctrlValue) {
    __CODEGEN_SET_MACRO(ControlsB_DebugctrlValue);
    Common.ControlsB_Debugctrl = ControlsB_DebugctrlValue;
  }

  __CODEGEN_INLINE QWORD GetFlagRegisterNumberSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.FlagRegisterNumberSubregisterNumber;
  }

  __CODEGEN_INLINE void SetFlagRegisterNumberSubregisterNumber(
      QWORD FlagRegisterNumberSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(FlagRegisterNumberSubregisterNumberValue);
    Common.FlagRegisterNumberSubregisterNumber =
        FlagRegisterNumberSubregisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetMaskctrl() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Maskctrl;
  }

  __CODEGEN_INLINE void SetMaskctrl(QWORD MaskctrlValue) {
    __CODEGEN_SET_MACRO(MaskctrlValue);
    Common.Maskctrl = MaskctrlValue;
  }

  __CODEGEN_INLINE QWORD GetDestinationRegisterFile() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.DestinationRegisterFile;
  }

  __CODEGEN_INLINE void
  SetDestinationRegisterFile(QWORD DestinationRegisterFileValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterFileValue);
    Common.DestinationRegisterFile = DestinationRegisterFileValue;
  }

  __CODEGEN_INLINE QWORD GetSrc1Regfile() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Source1RegisterFile;
  }

  __CODEGEN_INLINE void SetSrc1Regfile(QWORD Source1RegisterFileValue) {
    __CODEGEN_SET_MACRO(Source1RegisterFileValue);
    Common.Source1RegisterFile = Source1RegisterFileValue;
  }

  __CODEGEN_INLINE QWORD GetDestinationDataType() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.DestinationType;
  }

  __CODEGEN_INLINE void SetDestinationDataType(QWORD DestinationTypeValue) {
    __CODEGEN_SET_MACRO(DestinationTypeValue);
    Common.DestinationType = DestinationTypeValue;
  }

  __CODEGEN_INLINE ADDRMODE GetSource1_SourceAddressingMode() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)Common.Source1AddressingMode;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceAddressingMode(ADDRMODE Source1AddressingModeValue) {
    __CODEGEN_SET_MACRO(Source1AddressingModeValue);
    Common.Source1AddressingMode = Source1AddressingModeValue;
  }

  __CODEGEN_INLINE ADDRMODE GetDestinationAddressingMode() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)Common.DestinationAddressingMode;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressingMode(ADDRMODE DestinationAddressingModeValue) {
    __CODEGEN_SET_MACRO(DestinationAddressingModeValue);
    Common.DestinationAddressingMode = DestinationAddressingModeValue;
  }

  __CODEGEN_INLINE QWORD GetExdesc96() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc96 << 6;
  }

  __CODEGEN_INLINE void SetExdesc96(QWORD Exdesc96Value) {
    __CODEGEN_SET_MACRO(Exdesc96Value);
    Common.Exdesc96 = Exdesc96Value >> 6;
  }

  __CODEGEN_INLINE QWORD GetSelreg32desc() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Selreg32desc;
  }

  __CODEGEN_INLINE void SetSelreg32desc(QWORD Selreg32descValue) {
    __CODEGEN_SET_MACRO(Selreg32descValue);
    Common.Selreg32desc = Selreg32descValue;
  }

  __CODEGEN_INLINE ADDRMODE GetSource0_SourceAddressingMode() {
    __CODEGEN_GET_MACRO();
    return (ADDRMODE)Common.Source0AddressingMode;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceAddressingMode(ADDRMODE Source0AddressingModeValue) {
    __CODEGEN_SET_MACRO(Source0AddressingModeValue);
    Common.Source0AddressingMode = Source0AddressingModeValue;
  }

  __CODEGEN_INLINE QWORD GetExdesc3116() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Common.Exdesc3116 << 16;
  }

  __CODEGEN_INLINE void SetExdesc3116(QWORD Exdesc3116Value) {
    __CODEGEN_SET_MACRO(Exdesc3116Value);
    Common.Exdesc3116 = Exdesc3116Value >> 16;
  }

  __CODEGEN_INLINE void SetExdesc1111(QWORD Exdesc1111Value) {
    __CODEGEN_SET_MACRO(Exdesc1111Value);
    Common.Exdesc1111 = Exdesc1111Value >> 11;
  }

  __CODEGEN_INLINE EU_INSTRUCTION_OPERAND_SEND_MSG &GetMessage() {
    __CODEGEN_GET_MACRO();
    return Common.Message;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA_Predctrl() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsTrue.ControlsA_Predctrl;
  }

  __CODEGEN_INLINE void
  SetControlsA_Predctrl(PREDCTRL ControlsA_PredctrlValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredctrlValue);
    PropertyPredicationIsTrue.ControlsA_Predctrl = ControlsA_PredctrlValue;
  }

  __CODEGEN_INLINE PREDINV GetControlsA_Predinv() {
    __CODEGEN_GET_MACRO();
    return (PREDINV)PropertyPredicationIsTrue.ControlsA_Predinv;
  }

  __CODEGEN_INLINE void SetControlsA_Predinv(PREDINV ControlsA_PredinvValue) {
    __CODEGEN_SET_MACRO(ControlsA_PredinvValue);
    PropertyPredicationIsTrue.ControlsA_Predinv = ControlsA_PredinvValue;
  }

  __CODEGEN_INLINE PREDCTRL GetControlsA__() {
    __CODEGEN_GET_MACRO();
    return (PREDCTRL)PropertyPredicationIsFalse.ControlsA__;
  }

  __CODEGEN_INLINE void SetControlsA__(PREDCTRL ControlsA__Value) {
    __CODEGEN_SET_MACRO(ControlsA__Value);
    PropertyPredicationIsFalse.ControlsA__ = ControlsA__Value;
  }

  __CODEGEN_INLINE SATURATE GetControlsB_Saturate() {
    __CODEGEN_GET_MACRO();
    return (SATURATE)PropertySaturationIsTrue.ControlsB_Saturate;
  }

  __CODEGEN_INLINE void
  SetControlsB_Saturate(SATURATE ControlsB_SaturateValue) {
    __CODEGEN_SET_MACRO(ControlsB_SaturateValue);
    PropertySaturationIsTrue.ControlsB_Saturate = ControlsB_SaturateValue;
  }

  // note:  unique for sends encoding
  __CODEGEN_INLINE QWORD GetSource1_SourceAddressImmediateSign9() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source1AddressingModeIsIndirect.Source1AddressImmediateSign9;
  }

  // note: unique for sends encoding
  __CODEGEN_INLINE void SetSource1_SourceAddressImmediateSign9(
      QWORD Source1AddressImmediateSign9Value) {
    __CODEGEN_SET_MACRO(Source1AddressImmediateSign9Value);
    Source1AddressingModeIsIndirect.Source1AddressImmediateSign9 =
        Source1AddressImmediateSign9Value;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source1AddressingModeIsIndirect.Source1AddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceAddressImmediate84(QWORD Source1AddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source1AddressImmediate84Value);
    Source1AddressingModeIsIndirect.Source1AddressImmediate84 =
        Source1AddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceAddressSubregisterNumber_0() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        Source1AddressingModeIsIndirect.Source1AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource1_SourceAddressSubregisterNumber_0(
      QWORD Source1AddressSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1AddressSubregisterNumberValue);
    Source1AddressingModeIsIndirect.Source1AddressSubregisterNumber =
        Source1AddressSubregisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource1_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source1AddressingModeIsDirect.Source1RegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource1_SourceRegisterNumber(QWORD Source1RegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source1RegisterNumberValue);
    Source1AddressingModeIsDirect.Source1RegisterNumber =
        Source1RegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetDestinationSubregisterNumber4() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        DestinationAddressingModeIsDirect.DestinationSubregisterNumber4;
  }

  __CODEGEN_INLINE void
  SetDestinationSubregisterNumber4(QWORD DestinationSubregisterNumber4Value) {
    __CODEGEN_SET_MACRO(DestinationSubregisterNumber4Value);
    DestinationAddressingModeIsDirect.DestinationSubregisterNumber4 =
        DestinationSubregisterNumber4Value;
  }

  __CODEGEN_INLINE QWORD GetDestinationRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)DestinationAddressingModeIsDirect.DestinationRegisterNumber;
  }

  __CODEGEN_INLINE void
  SetDestinationRegisterNumber(QWORD DestinationRegisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationRegisterNumberValue);
    DestinationAddressingModeIsDirect.DestinationRegisterNumber =
        DestinationRegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetDestinationAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
               DestinationAddressingModeIsIndirect.DestinationAddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void
  SetDestinationAddressImmediate84(QWORD DestinationAddressImmediate84Value) {
    __CODEGEN_SET_MACRO(DestinationAddressImmediate84Value);
    DestinationAddressingModeIsIndirect.DestinationAddressImmediate84 =
        DestinationAddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetDestinationAddressSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        DestinationAddressingModeIsIndirect.DestinationAddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetDestinationAddressSubregisterNumber(
      QWORD DestinationAddressSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(DestinationAddressSubregisterNumberValue);
    DestinationAddressingModeIsIndirect.DestinationAddressSubregisterNumber =
        DestinationAddressSubregisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetDestinationAddressImmediateSign9() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        DestinationAddressingModeIsIndirect.DestinationAddressImmediateSign9;
  }

  __CODEGEN_INLINE void SetDestinationAddressImmediateSign9(
      QWORD DestinationAddressImmediateSign9Value) {
    __CODEGEN_SET_MACRO(DestinationAddressImmediateSign9Value);
    DestinationAddressingModeIsIndirect.DestinationAddressImmediateSign9 =
        DestinationAddressImmediateSign9Value;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceSubRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source0AddressingModeIsDirect.Source0SubregisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceSubRegisterNumber(QWORD Source0SubregisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0SubregisterNumberValue);
    Source0AddressingModeIsDirect.Source0SubregisterNumber =
        Source0SubregisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceRegisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source0AddressingModeIsDirect.Source0RegisterNumber;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceRegisterNumber(QWORD Source0RegisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0RegisterNumberValue);
    Source0AddressingModeIsDirect.Source0RegisterNumber =
        Source0RegisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediate84() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source0AddressingModeIsIndirect.Source0AddressImmediate84
           << 4;
  }

  __CODEGEN_INLINE void
  SetSource0_SourceAddressImmediate84(QWORD Source0AddressImmediate84Value) {
    __CODEGEN_SET_MACRO(Source0AddressImmediate84Value);
    Source0AddressingModeIsIndirect.Source0AddressImmediate84 =
        Source0AddressImmediate84Value >> 4;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressSubregisterNumber() {
    __CODEGEN_GET_MACRO();
    return (QWORD)
        Source0AddressingModeIsIndirect.Source0AddressSubregisterNumber;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressSubregisterNumber(
      QWORD Source0AddressSubregisterNumberValue) {
    __CODEGEN_SET_MACRO(Source0AddressSubregisterNumberValue);
    Source0AddressingModeIsIndirect.Source0AddressSubregisterNumber =
        Source0AddressSubregisterNumberValue;
  }

  __CODEGEN_INLINE QWORD GetSource0_SourceAddressImmediateSign9() {
    __CODEGEN_GET_MACRO();
    return (QWORD)Source0AddressingModeIsIndirect.Source0AddressImmediateSign9;
  }

  __CODEGEN_INLINE void SetSource0_SourceAddressImmediateSign9(
      QWORD Source0AddressImmediateSign9Value) {
    __CODEGEN_SET_MACRO(Source0AddressImmediateSign9Value);
    Source0AddressingModeIsIndirect.Source0AddressImmediateSign9 =
        Source0AddressImmediateSign9Value;
  }

} __CODEGEN_ATTRIBUTES_STRUCTURE EU_INSTRUCTION_SENDS;

static_assert(16 == sizeof(EU_INSTRUCTION_SENDS));

__CODEGEN_NAMESPACE_CLOSE
__CODEGEN_FILE_DIRECTIVES_CLOSE

#endif // __IGFXHWEUISA_H__
