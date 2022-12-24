<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  BFE = 0x46

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x46(BFE) | Exec_size | Pred | Dst | Src0 | Src1 | Src2 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        UD width = src0[i] & 1F;
                        UD offset = src1[i] & 1F;
                        dst[i] = (src2[i] >> offset) & ((1 << width) - 1);
                      }
                    }
```

## Description





```
    Component-wise extract a bit field from <src2> using the bit field width from <src0> and the bit field offset from <src1>.
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

- **Pred(uw):** Predication control


- **Dst(vec_operand):** The destination operand. Operand class: general,indirect


- **Src0(vec_operand):** Bit field width. Operand class: general,indirect,immediate


- **Src1(vec_operand):** Bit field offset. Operand class: general,indirect,immediate


- **Src2(vec_operand):** Bit field to be extracted. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** D,UD
- **Source Modifier:** false


#### Operand type maps
- **Type map**
  -  **Dst types:** UD
  -  **Src types:** UD
- **Type map**
  -  **Dst types:** D
  -  **Src types:** D


## Text
```
[(<P>)] BFE (<exec_size>) <dst> <src0> <src1> <src2>
```

## Notes





```
    If <dst> is type D, the results are sign-extended, otherwise they are zero-extended.

Restriction:
    - Dst, Src0, Src1, and Src2 must be 16 byte aligned, except when <exec size> is 1.
    - <exec_size> must not be 2.
```

