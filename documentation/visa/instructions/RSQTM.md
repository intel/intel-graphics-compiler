<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  RSQTM = 0x9c

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x9c(RSQTM) | Exec_size | Pred | Dst | PredDst | Src0 |


## Semantics


```

                    for (i = 0; i < exec_size; ++i){
                      if (ChEn[i]) {
                        dst[i] = rsqtm(src0[i])
                        preddst[i] = (rsqtm(src0[i]) == NAN/INF/ZERO)
                      }
                    }
```

## Description





```
    Use rsqtm math macro to computes component-wise reciprocal square root of src0 and stores the results in <dst> and set <preddst>
    to EO (early out) of rsqtm. If a bit in <preddst> is set, its corresponding <dst> is a special number (NAN/INF/ZERO); otherwise
    <dst> is an initial approximation of the reciprocal sqaure root, and the further refinement on the apporoximation is needed to get
    a result of expected precision.

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


- **PredDst(vec_operand):** The predicate destination operand. Operand class: predicate


- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate


#### Properties
- **Supported Types:** DF,F
- **Source Modifier:** arithmetic




## Text
```



    [(<P>)] RSQTM (<exec_size>) <dst> <preddst> <src0>
```
## Notes





```
    The instruction is intended to be used in a library function to provide different implementations of some math functions,
    such as, a double reciprocal square root of precision 2ulp.
    Saturation is not supported.

```

