<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SEL = 0x2a

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x2a(SEL) | Exec_size | Pred | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (EM[i]) {
                        dst[i] = Pred[i] ? src0[i] : src1[i];
                      }
                    }
```

## Description





```
    Copies data component-wise from <src0> if the predicate bit is set, else copies data component-wise from <src1> and stores the result into <dst>.
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


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


- **Src1(vec_operand):** The second source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** B,D,DF,F,HF,Q,UB,UD,UQ,UW,W.{XEHP+}BF
- **Saturation:** Yes
- **Source Modifier:** arithmetic


#### Operand type maps
- **Type map**
  -  **Dst types:** UD, D, UW, W, UB, B, UQ, Q
  -  **Src types:** UD, D, UW, W, UB, B, UQ, Q
- **Type map{XEHP+}**
  -  **Dst types:** F, BF
  -  **Src types:** F, BF
- **Type map**
  -  **Dst types:** DF
  -  **Src types:** DF
- **Type map**
  -  **Dst types:** F, HF
  -  **Src types:** F, HF


## Text
```
[(<P>)] SEL[.sat] (<exec_size>) <dst> <src0> <src1>
```

## Notes





    Q/UQ is not supported on all platforms without int64 (ICL, TGL, DG2, MTL, etc.)

