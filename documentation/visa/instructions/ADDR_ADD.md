<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  ADDR_ADD = 0x28

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x28(ADDR_ADD) | Exec_size | Dst | Src0 | Src1 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = src0[i] + src1[i];
                      }
                    }
```

## Description





```
    Adds the byte address <src1> to the address <src0> and stores the result into <dst>.
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

- **Dst(vec_operand):** The destination operand. Operand class: address


- **Src0(vec_operand):** The first source operand. Operand class: address,general,state


- **Src1(vec_operand):** The second source operand. Must have type UW. Operand class: general,immediate


#### Properties
- **Source Modifier:** arithmetic




## Text
```
ADDR_ADD (<exec_size>) <dst> <src0> <src1>
```

## Notes





```
    If src0 is a general operand, the byte address of the general variable is taken and the row and column offset are added to it to produce the address value. In this scenario Src0's region must be <0;1,0>, implying that all channels receive the same value for Src0. If src0 is a state operand, the byte address of the state variable (one of surface/sampler) is taken and the offset is then added to it to produce the address value. The result value in Dst must point to the same variable as Src0 (i.e., if src0 points to an element in v1, src0 + src1 must point to another element in v1). Predication is not supported for this instruction. Src0 must neither be a pre-defined variable nor a pre-defined surface, except for V13(%arg) and V14(%retval).

    It is up to the front-end compiler to ensure that resulting address has the right alignment before it is used in an indirect operand. As far as the finalizer is concerned it is just adding two integers representing GRF byte offsets.
```

