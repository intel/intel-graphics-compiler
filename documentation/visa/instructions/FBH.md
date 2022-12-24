<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FBH = 0x2f

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x2f(FBH) | Exec_size | Pred | Dst | Src0 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        UD cnt = 0;
                        if ( src0 is unsigned ) {
                          UD udScalar = src0[i];
                          while ( (udScalar & (1 << 31)) == 0 && cnt != 32 ) {
                            cnt ++;
                            udScalar = udScalar << 1;
                          }
                          dst[i] = src0[i] == 0 ? 0xFFFFFFFF : cnt;
                        }
                        else { // src0 is signed.
                          D dScalar = src0[i];
                          bit cval = dScalar[31];
                          while ((dScalar & (1 << 31)) == cval && cnt != 32 ) {
                            cnt ++;
                            dScalar = dScalar << 1;
                          }
                          dst[i] = (src0[i] == 0xFFFFFFFF) || (src0[i] == 0) ? 0xFFFFFFFF : cnt;
                        }
                      }
                    }
```

## Description





```
    Performs component-wise tracking of the first bit set from the MSB side in <src0> and stores the result into <dst>.

    The instruction behaves differently depending on the type and value of each component in src0:

    - If src0[i] is unsigned, the instruction counts the number of leading zeros.  If src0[i] has no bit set, the value 0xFFFFFFFF is returned.
    - If src0[i] is signed and >=0, the instruction counts the number of leading zeros.  If src0[i] is zero, the value 0xFFFFFFFF is returned.
    - If src0[i] is signed and negative, the instruction counts the number of leading ones.
    - If src0[i] is -1, the value 0xFFFFFFFF is returned.
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


- **Dst(vec_operand):** The destination operand. Must have type UD. Operand class: general,indirect


- **Src0(vec_operand):** The first source operand. Must have type D,UD. Operand class: general,indirect,immediate


#### Properties
- **Source Modifier:** false




## Text
```
[(<P>)] FBH (<exec_size>) <dst> <src0>
```

## Notes





