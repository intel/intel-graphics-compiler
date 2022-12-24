<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  ADD3O = 0x92

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x92(ADD3O) | Exec_size | Pred | Dst | Src0 | Src1 | Src2 |


## Semantics


```
                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = src0[i] + src1[i] + src2[i];
                        P[i] = overflow(src0[i] + src1[i] + src2[i]);
                      }
                    }
```

## Description





```
    Performs component-wise addition of <src0>, <src1> and <src2> and stores the result into <dst> and overflow-bit into <P>. Overflow-bit will be either 0 or 1.

    It will be mapped into GEN add3. Specifically:
    (W)add3 (16|M0) (ov)f0.0 dst<1>:ud src0<1;0>:ud src1<1;0>:ud src2<1;0>:ud
```


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

- **Pred(uw):** Variable for overflow condition modifier


- **Dst(vec_operand):** The destination operand. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


- **Src2(vec_operand):** The third source operand. Operand class: general,indirect,immediate16


#### Properties
- **Supported Types:** D,UD,UW,W
- **Source Modifier:** arithmetic


#### Operand type maps
- **Type map**
  -  **Dst types:** UD, D, UW, W
  -  **Src types:** UD, D, UW, W


## Text
```



    (<P>) ADD3O (<exec_size>) <dst> <src0> <src1> <src2>
```
## Notes





