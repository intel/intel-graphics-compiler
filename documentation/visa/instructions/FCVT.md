<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FCVT = 0x1d

## Format

| | | | |
| --- | --- | --- | --- |
| 0x1d(FCVT) | Exec_size | Dst | Src0 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {    // ChEn[i] is always true if dst has FP8 type
                        dst[i] = src0[i];
                      }
                    }
```

## Description





```
    Perform type conversion between FP8 and HF from <src0> to <dst>. FP8 here is BF8, it is an 8-bit float with 1-bit sign, 5-bit exponent, and 2-bit mantissa, aka E5M2.  HF-to-BF8 conversion uses the RTE rounding mode (round-to-nearest-even), and denoms are retained. FP8-to-HF is a precise conversion, thus no rounding is involved. BF8 is denoted by type UB as visa has no BF8 type.

    {PVC_XT+}It also performs conversion between float and TF32 (tensorfloat, 1-bit sign, 8-bit exponent, and 10-bit mantissa). It also uses RTE for float to TF32. Denorms are flushed to zero. Conversion from TF32 to float is noop as TF32 is a valid F type.


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

- **Dst(vec_operand):** The destination operand. Operand class: general


- **Src0(vec_operand):** The first source operand. Operand class: general


#### Properties
- **Supported Types:** B,HF,UB.{PVC_XT+}F,UD
- **Source Modifier:** false




## Text
```
FCVT (<exec_size>) <dst> <src0>
```

## Notes





    - If Dst has HF type, Src0 must have UB type (which represents a BF8 value).
    - If Dst has UB type (which represents a BF8 value), Src0 must have HF type. NM (NoMask) mask control must be used.
    {PVC_XT+}- If Dst has F type, Src0 must have UD (as TF32 value); If Dst has UD type (as TF32 value), Src0 must have F type.

