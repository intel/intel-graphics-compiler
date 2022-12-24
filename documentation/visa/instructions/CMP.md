<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  CMP = 0x2c

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x2c(CMP) | Exec_size | Rel_op | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        if (dst is predicate) {
                          dst[i] = src0[i] rel_op src[i];
                        }
                        else { // dst is general operand
                          dst[i] = src0[i] rel_op src[i] ? -1 : 0;
                        }
                    }
```

## Description





```
    Performs component-wise comparison of <src0> and <src1> according to the relational operator and stores the results into <dst>.
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

- **Rel_op(ub):** Relational operator for the comparison

  - Bit[2..0]: encode the relational operator

    - 0b000:  EQ - equal to
    - 0b001:  NE - not equal
    - 0b010:  GT - greater than
    - 0b011:  GE - greater than or equal to
    - 0b100:  LT - less than
    - 0b101:  LE - less than or equal to

- **Dst(vec_operand):** The destination operand. Operand class: predicate,general


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** B,D,DF,F,HF,Q,UB,UD,UQ,UW,W.{XEHP+}BF
- **Source Modifier:** arithmetic


#### Operand type maps
- **Type map**
  -  **Dst types:** UD, D, UW, W, UB, B
  -  **Src types:** UD, D, UW, W, UB, B
- **Type map**
  -  **Dst types:** F
  -  **Src types:** UD, D, UW, W, UB, B
- **Type map**
  -  **Dst types:** F, HF
  -  **Src types:** F, HF
- **Type map{XEHP+}**
  -  **Dst types:** F, BF
  -  **Src types:** F, BF
- **Type map**
  -  **Dst types:** DF
  -  **Src types:** DF
- **Type map**
  -  **Dst types:** HF
  -  **Src types:** UD, D, UW, W, UB, B


## Text
```



    CMP.rel_op  (<exec_size>) <dst> <src0> <src1>

    //rel_op is one of "eq", "ne", "gt", "ge", "lt", "le"
```
## Notes





```
    Supported Types:
        - Source: BF, HF, F, DF, B, UB, W, UW, D, UD, Q, UQ.
        - Destination (if general): BF, HF, F, DF, B, UB, UW, D, UD, Q, UQ.

    Predication is not allowed for this instruction.If destination is a general operand:
        - Either all ones (0xFF, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFF based on type size) or all zeros will be assigned to it based on the comparison results.
        - If both sources have float type, destintation must have the same type as source

    CMP and SETP instructions are the only way to create a new predicate value. Floating point comparison for special values follows the rules below:
        - Less than. src0 < src1 and neither source is NaN.
        - Equal. src0 = src1 and neither source is NaN.
        - Greater than. src0 > src1 and neither source is NaN.
        - Unordered. Any source is NaN.

    Any NaN compares unordered to any value, including itself. Specifically, if one of the operands is NaN, NE comparison returns true while all other comparisons return false.
    Infinities of the same sign compare as equal.
    Zeros compare as equal regardless of sign: -0 = +0.
```

