<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  DPAS = 0x83

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x83(DPAS) | Exec_size | Dst | Src0 | Src1 | Src2 | W |
|            | A         | SD  | RC   |      |      |   |


## Semantics


```
   DPAS is a matrix multiply-add operation as follows:

       D = C + A x B

       where
          D (Dst)  : MxN
          C (Src0) : MxN
          A (Src2) : MxK
          B (Src1) : KxN

          M : repeat count
          N : fixed execution size, either 8 or 16
          K : depth * OPS_PER_CHAN
              OPS_PER_CHAN
                 1 : for TF32
                 2 : for 16-bit precision(BF, HF)
                 4 : for 8-bit precision (FP8, UB, B)
                 8 : for less-then 8 bit precision (U4/S4, U2/S2).

              If depth is 8, K would be 8, 16, 32, or 64 (basd on OPS_PER_CHAN).

       Note that Src2 is A, and Src1 is B.

   Conceptually, DPAS instruction is simple. As it requires its operands to have a special layout in GRF
   for matrix B, this makes DPAS a little complicated to understand. The following might help undertand
   it better.

       If we view GRFs as a 1-D memory space in the increasing order of GRF numbers, Dst, Src0, Src2
       are laid out in row-major in this 1-D memory space. But Src1 needs a special layout, neither
       row-major nor column major. To visualize the layout for Src1,  we can view entire GRFs as a 2-D
       memory space with each row being an exactly one whole GRF. For example,  128 GRFs of 16 DWs each
       can be viewed as 128x16 of DW (it would be 128x8 if GRF size is 8 DWs), which means N must be 16.
       Under this view, each column of Src1 (matrix B) of total 16 columns are packed into its corresponding
       column of this 2-D memory space, that is, Column 0 of Src1 to Column 0 of 2-D memory space, column 1
       of Src1 to Column 1 of 2-D memory space, and so on. Take 8-bit precision for example, K is 32. This
       32 elements of 8-bit precision data is packed into 8 DWs (4 for each DW). For all 16 columns, it
       takes 8 rows in this 2-D memory space, which is 8 GRFs.

   With this, the following detailed description will be not hard to follow.

    1) The semantics for interger DPAS and float(bf/hf) DPAS:

       The semantics is described using the following notation:

           Let dot4(X.a[3:0], Y.b[3:0]) be a 4-element dot product of X and Y, starting
           at element offset a and b respectively. Similary, let dot2(X.a[1:0], Y.b[1:0])
           be a 2-element dot product of X and Y, and dot8(X.a[7:0], Y.b[7:0]) be a 8-element
           dot product of X and Y. Note that the size of element in X and Y may be different.
           For example, if X's precision is 4 bit, Y's 8 bits, X's element
           size shall be 4 bits, y's shall be 8 bits. Also, X.DW[j] is used to denote
           the j+1'th DW of X; X.R[j] for denoting the j+1'th GRF. Similarly, X.R[1].DW[2]
           will refer to the 3rd DW of the 2nd GRF of X. Note that index always starts from zero.
           GRF size would be 8 pre PVC and 16 DW in PVC or later.


       // 1) Src1PrecisionInBits and Src2PrecisionInBits are 16 for both bfloat(bf) and half(hf) dpas.
       // 2) Element size for Src0 and Dst are 4 bytes (either int or fp32)
       // 3) Element size for src1 and src2 could be 2 bits, 4 bits, 8 bits, or 16 bits (bf/hf),
       //    which are given as precisons.

       // OPS_PER_CHAN
       if (Src1PrecisionInBits == 16) {
         // BF/HF dpas. Src2PrecisionInBits must be 16
         OPS_PER_CHAN = 2;
       } else if (Src1Precision == 8 || Src2Precision == 8) {
         // integer dpas : dot4 per DW
         OPS_PER_CHAN = 4;
       } else {
         // integer dpas : dot8 per DW
         OPS_PER_CHAN = 8;
       }

       // Src1's DW of each channel might be used in more than one depth (based on its precision).
       // For example, an integer dpas with src1's precision being 2-bits, one DW has data for
       // 4 depths (assuming dot4 per DW. Each dot4 consumes 8 bits, a DW can do 4 dot4 operations).
       // Under SD=8, the first four depths will use the entire DW of the first GRF holding src1;
       // and the second 4 depths will use the entire DW of the second GRF. If src1's precision is
       // 8 bits (dot4 per DW), each DW will be used in an exact one depth; and the next depth will
       // use the DW of the next GRF. The following variable is used in the pseudo code:
       //   SRC1_OPERANDS_PER_CHAN :
       //     how many operands per each DW (each channel is DW-wide) for src1.
       //     Each operand is defined as data that is used for a single dot4/dot8(int) or dot2(bf/bf).
       //     This means that for float DPAS, SRC1_OPERANDS_PER_CHAN = 1; and for int DPAS,
       //       if OPS_PER_CHAN = 4
       //          SRC1_OPERANDS_PER_CHAN = 1 (8-bit precision), 2 (4-bit), or 4 (2-bit)
       //       else // OPS_PER_CHAN = 8
       //          SRC1_OPERANDS_PER_CHAN = 1 (4-bit), or 2 (2-bit)
       SRC1_OPERANDS_PER_CHAN = 32 / (OPS_PER_CHAN * Src1PrecisionInBits)

       Exec_size = isPrePVC ? 8 : 16;

       k = 0;
       for (r = 0; i < RC; ++r)
       {
         temp = Src0.R[r];
         for (d = 0; d < SD; ++d )
         {
           m = d / SRC1_OPERANDS_PER_CHAN;  // to select GRF
           n = (d % SRC1_OPERANDS_PER_CHAN) * OPS_PER_CHAN; // in unit of element
           for ( i = 0; i < Exec_size; i++ )
           { // for each channel
             if OPS_PER_CHAN == 4  // int dpas
                temp.DW[i] += dot4(Src1.R[m].DW[i].n[3:0], Src2.k[3:0]);
             else if OPS_PER_CHAN == 8 // int dpas
                temp.DW[i] += dot8(Src1.R[m].DW[i].n[7:0], Src2.k[7:0]);
             else // float DPAS
                temp.F[i] += dot2(Src1.R[m].DW[i].n[1:0], Src2.k[1:0]);
           }
           k += OPS_PER_CHAN;
         }

         // update dst
         dst.R[r] = temp;
       }

       Dst, Src0 are advanced one GRF for each repeat advance; Src2 is advanced 8*OPS_PER_CHAN for
       each repeat advance. Src1 stays the same for each repeat count advance.








```

## Description





    Integer DPAS is a element wise multiply add and accumulate operation of multiple elements
    in a systolic pipeline with low precision (<= 8 bits) inputs. Src1 is also refered to
    as Weights, and Src2 as Activation. Src1 and Src2's element types are defined by
    Precision W and A, respectively. Src1 is divided into elements along each 32-bit SIMD channel.

    Float DPAS is the same, except its elements are always 16 bits. So each 32-bit channel has
    exactly two elements. And element type is either half float (hf) or bfloat16 (bf).


    The operand precision refers to a type of elements that make up an operand, and generally,
    it is no larger than 8 bits in size (subbyte type) for integer, 16 bits for both bf and hf,
    Currently, the precision is used only in DPAS instruction. And the operand that a precision applies
    to must be of either D or UD type. The following table lists all possible precisions and their text
    and binary formats:

    .. table:: The following table lists all operand precisions:
      :align: center

      +-------------------+-------------+--------+--------+
      | Operand Precision |  Range      | Binary | Text   |
      |                   |             | Format | Format |
      +-------------------+-------------+--------+--------+
      | unused            |             | 0000b  |        |
      +-------------------+-------------+--------+--------+
      | Unsigned 1-bit    | [0, 1]      | 0001b  | u1     |
      +-------------------+-------------+--------+--------+
      | Signed 1-bit      | [-1, 0]     | 0010b  | s1     |
      +-------------------+-------------+--------+--------+
      | Unsigned 2-bits   | [0, 3]      | 0011b  | u2     |
      +-------------------+-------------+--------+--------+
      | Signed 2-bits     | [-2, 1]     | 0100b  | s2     |
      +-------------------+-------------+--------+--------+
      | Unsigned 4-bits   | [0, 15]     | 0101b  | u4     |
      +-------------------+-------------+--------+--------+
      | Signed 4-bits     | [-8, 7]     | 0110b  | s4     |
      +-------------------+-------------+--------+--------+
      | Unsigned 8-bits   | [0, 255]    | 0111b  | u8     |
      +-------------------+-------------+--------+--------+
      | Signed 8-bits     | [-128, 127] | 1000b  | s8     |
      +-------------------+-------------+--------+--------+
      | bfloat            | bfloat16    | 1001b  | bf     |
      +-------------------+-------------+--------+--------+
      | half              | fp16        | 1010b  | hf     |
      +-------------------+-------------+--------+--------+
      | tf32              | tf32        | 1100b  | tf32   |
      +-------------------+-------------+--------+--------+


    The bfloat16 is a 16-bit float type (E8M7, aka truncated IEEE 754 single-precision 32-bit float,
    1-bit sign, 8-bit exponent, 7-bit mantissa). and fp16 is the IEEE 754 half.



    The TF32 is 19-bit tensor float type (E8M10), which has 1-bit sign, 8-bit exponent, and 10-bit mantissa.


    For integer type, the precision of the sources can vary per src1 and src2. For float type, src1
    and src2 must be the same precision (either bf or hf, not mixed). For df type, dst/src0/src1/srcs must
    have same df precision.

    The dst and src0 take a regular integer type (e.g. D or UD) or float type which is 32 bits in size,
    Src0 is used as an accumulator to add operands to. The SD parameter is the systolic depth of the operation,
    meaning we perform a sequence of these operations advancing over successive inputs. The output of each stage
    is a dword (integer or float) or df which is the accumulated input to the next systolic stage. The first stage
    accumulation input is defined via Src0. The last stage accumulated output is written to Dst.

    Not all combinations of operand types are allowed. The following table gives all the legal combinations.
    Note that particular platform might have additional restriction and this spec will follow that
    additional restriction.

    .. table:: All legal combinations of types and precisions.
      :align: center

      +--------+---------+-----------+----------+
      | Dst    |  Src0   | Src1      | Src2     |
      |        |         | Precision | Preision |
      +--------+---------+-----------+----------+
      | UD, D  |  UD,D   | int       | int      |
      +--------+---------+-----------+----------+
      | F, BF  |  F, BF  | BF        | BF       |
      +--------+---------+-----------+----------+
      | F, HF  |  F, HF  | HF        | HF       |
      +--------+---------+-----------+----------+
      | F      |  F      | TF32      | TF32     |
      +--------+---------+-----------+----------+
      | F      |  F      | BF8, HF8  | BF8, HF8 |
      +--------+---------+-----------+----------+


    SD can be encoded as 1, 2, 4, and 8. XEHP+ only supports a systolic depth of 8.



- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - {XEHP}0b011:  8 elements
    - {PVC}0b100:  16 elements
  - Bit[7..4]: execution mask (explicit control over the enabled channels)

    - 0b0000:  M1
    - 0b0001:  M2
    - 0b0010:  M3
    - 0b0011:  M4
    - 0b0100:  M5
    - 0b0101:  M6
    - 0b0110:  M7
    - 0b0111:  M8
    - 0b1000:  M1_NM
    - 0b1001:  M2_NM
    - 0b1010:  M3_NM
    - 0b1011:  M4_NM
    - 0b1100:  M5_NM
    - 0b1101:  M6_NM
    - 0b1110:  M7_NM
    - 0b1111:  M8_NM

- **Dst(raw_operand):** The destination operand.. Must have type D,UD,F,BF,HF,DF


- **Src0(raw_operand):** The source 0 operand. It could be a null operand, meaning it is zero.. Must have type D,UD,F,BF,HF,DF


- **Src1(raw_operand):** The field **W** further defines its element precision. Must have type D,UD,DF


- **Src2(vec_operand):** The field **A** defines its  element precision. Must have type D,UD,DF. Operand class: general


- **W(ub):** The precision of Src1, shown in the table above


- **A(ub):** The precision of Src2, shown in the table above


- **SD(ub):** Systolic depth


- **RC(ub):** Repeat Count


#### Properties
- **Supported Types:** D,F,UD
- **Source Modifier:** false




## Text
```




      DPAS.W.A.SD.RC    (Exec_size) <dst> <src0> <src1> <src2>
```
## Notes





```

      - **Register region:** No
      - **Alignment:** Dst, Src0, and Src1 are GRF aligned; for interger or float(bf, hf) DPAS, Src2 is
        (SD/(32/(Src2PrecisionInBits * OPS_PER_CHAN))) DWORD aligned. That is, for SD=8 and OPS_PER_CHAN=4,
        src2 would be 8 DWORD aligned for s8/u8 precision; 4 DWORD aligned for s4/u4; and 2 DWORD aligned
        for s2/u2.

      Src2 type should be consistent with Src1's, that is, if Src1 is an integer type, Src2 must be an integer
      type; if Src1 is bf, Src2 must be bf; if Src1 is hf, Src2 must be hf too.
      Here are some examples:

      -  DPAS.u4.s8.8.8  (Exec_size) <dst> <src0> <src1> <src2>   // int DPAS with u4 for src1 and s8 for src1
      -  DPAS.bf.bf.8.8  (Exec_size) <dst> <src0> <src1> <src2>   // float DPAS with bfloat as element type
      -  DPAS.hf.hf.8.8  (Exec_size) <dst> <src0> <src1> <src2>   // float DPAS with half as element type

      Also note that Src2 Should be uniform, meaning the entire Src2 is used by every simd lanes.

      Exec_size is 16 for PVC and 8 otherwise for int or float DPAS.

```

