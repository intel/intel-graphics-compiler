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

  MAD = 0x0c

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x0c(MAD) | Exec_size | Pred | Dst | Src0 | Src1 | Src2 |


## Semantics




                    for (i = 0; < exec_size; ++i){
                      if (ChEn[i]) {
                        dst[n] = src0[i] * src1[i] + src2[i];
                      }
                    }

## Description



    Performs component-wise multiply add of <src0>, <src1>, and <src2> and stores the results in <dst>.

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

- **Dst(vec_operand):** The destination operand. Operand class: general,indirect

- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate16

- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate16

- **Src2(vec_operand):** The third source operand. Operand class: general,indirect,immediate16

#### Properties
- **Supported Types:** B,D,DF,F,HF,UB,UD,UW,W 
- **Saturation:** Only when type is float 
- **Source Modifier:** arithmetic 


## Text
```
    

		[(<P>)] MAD[.sat]  (<exec_size>) <dst> <src0> <src1> <src2>
```



## Notes



		Floating point type MAD is guaranteed to be translated into a HW mad instruction.  Integer type MAD may be translated to either mac or multiply and add; for the latter the intermediate result of the multiply will have the same type of as the final destination.
