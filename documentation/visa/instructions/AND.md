<!---======================= begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ==========================-->

 

## Opcode

  AND = 0x20

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x20(AND) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics




                    for (i = 0; < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = src0[i] & src1[i];
                      }
                    }

## Description


    Performs component-wise bitwise AND of <src0> and <src1> and stores the result into <dst>.

- **Exec_size(ub):** Execution size
 
  - Bit[2..0]: size of the region for source and destination operands
 
    - 0b000:  1 element (scalar) 
    - 0b001:  2 elements 
    - 0b010:  4 elements 
    - 0b011:  8 elements 
    - 0b100:  16 elements 
    - 0b101:  32 elements 
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
- **Pred(uw):** Predication control

- **Dst(vec_operand):** The destination operand. Operand class: general,indirect,predicate

- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,predicate,immediate

- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,predicate,immediate

#### Properties
- **Supported Types:** B,BOOL,D,Q,UB,UD,UQ,UW,W 
- **Source Modifier:** logic 


## Text
```
    

  [(<P>)] AND (<exec_size>) <dst> <src0> <src1>
    //(<P>) must be absent for predicate operands.
```



## Notes



    The instruction may operate on predicate operands. In this mode, all operands must be predicates, and <pred> must be zero (no predication). If the operands are not predicates, all operands must have integer type.
