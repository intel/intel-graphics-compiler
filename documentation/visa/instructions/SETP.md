<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SETP = 0x2b

## Format

| | | | |
| --- | --- | --- | --- |
| 0x2b(SETP) | Exec_size | Dst | Src0 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (src0 is scalar) {
                        dst[i] = src0 & (1 << i); //one bit
                      }
                      else{
                        if(ChEn[i]) {
                          dst[i] = src0[i] & 1; // one bit
                        }
                      }
                    }
```

## Description





```
    Sets component-wise the predicate <dst> based on the values of <src0>
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

- **Dst(vec_operand):** The destination operand. Operand class: predicate


- **Src0(vec_operand):** The first source operand. Must have type UB,UW,UD. Operand class: general,indirect,immediate


#### Properties




## Text
```
SETP (<exec_size>) <dst> <src0>
```

## Notes





```
    - If <src0> is a scalar operand: Each of src0's bits from the LSB to MSB is used to set the BOOL value in the corresponding dst element.
    - If <exec_size> is 32: Instruction must be {M1_NoMask}, and all 32 dst elements are updated.
    - If <exec_size> is less than 32: Instruction must be either {M1_NoMask} or {M5_NoMask}, and the lower/upper 16 dst elements are updated based on the mask control.
    - If <src0> is general or indirect: The LSB in each src0 element determines the corresponding dst element's BOOL value.

    This instruction is intended to enable initialization of predicate variable to a constant bit stream.
```

