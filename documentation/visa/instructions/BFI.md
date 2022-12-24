<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  BFI = 0x47

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x47(BFI) | Exec_size | Pred | Dst | Src0 | Src1 | Src2 |
|           | Src3      |      |     |      |      |      |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        UD width = src0[i] & 1F;
                        UD offset = src1[i] & 1F;
                        UD bitmask = (((1 << width) - 1) << offset);
                        dst[i] = ((src2[i] << offset) & bitmask) | (src3[i] & ~bitmask);
                      }
                    }
```

## Description





```
    Given a bit range from the LSB of a number (<src2>), component-wise place that number of bits (<src0>) in another number (<src3>) at any offset (<src1>).
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


- **Src2(vec_operand):** Bit field value to insert. Operand class: general,indirect,immediate


- **Src3(vec_operand):** Overall value into which the bit ield is inserted. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** D,UD
- **Source Modifier:** false




## Text
```
[(<P>)] BFI (<exec_size>) <dst> <src0> <src1> <src2> <src3>
```

## Notes





```
    Restriction:

    - All operands must be 16 byte aligned, except when <exec_size> is 1.
    - <exec_size> must not be 2.
```

