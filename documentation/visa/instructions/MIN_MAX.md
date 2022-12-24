<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  MIN_MAX = 0x45

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x45(MIN_MAX) | Exec_size | Op | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = min/max(src0[i], src1[i]);
                      }
                    }
```

## Description





```
    Selects component-wise either the minimum or the maximum of the two sources to <dst>.
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

- **Op(ub):**

  - Bit[0]: controls the operations

    - 0b0:  min
    - 0b1:  max

- **Dst(vec_operand):** The destination operand. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** B,D,DF,F,HF,Q,UB,UD,UQ,UW,W
- **Saturation:** Yes
- **Source Modifier:** arithmetic




## Text
```




MIN[.sat] (<exec_size>) <dst> <src0> <src1> |
MAX[.sat] (<exec_size>) <dst> <src0> <src1>
```
## Notes





    For floating point values this instruction implements the IEEE min_max operation. To compute the MIN or MAX of two floating-point numbers, if one of the numbers is NaN and the other is not, MIN or MAX of the two numbers returns the one that is not NaN. When both numbers are NaN, MIN or MAX of the two numbers returns source1.

