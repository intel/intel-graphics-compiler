<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  TYPED_ATOMIC = 0x73

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x73(TYPED_ATOMIC) | Op | Exec_size | Pred | Surface | U   | V |
|                    | R  | LOD       | Src0 | Src1    | Dst |   |


## Semantics


```

    word or dword atomic "read-modify-write" operations.
```

## Description





```
    Performs 8 element scatteredword or dword write atomically into <surface>. The values written depend on the atomic operation being performed, and the old values from the surface are returned.
```


- **Op(ub):**

  - Bit[4..0]: encodes the atomic operation for this message

    - 0b00000:  add
    - 0b00001:  sub
    - 0b00010:  inc
    - 0b00011:  dec
    - 0b00100:  min
    - 0b00101:  max
    - 0b00110:  xchg
    - 0b00111:  cmpxchg
    - 0b01000:  and
    - 0b01001:  or
    - 0b01010:  xor
    - 0b01011:  imin
    - 0b01100:  imax
    - 0b01101:  predec
    - 0b10000:  fmax
    - 0b10001:  fmin
    - 0b10010:  fcmpwr
  - {TGLLP+}Bit[5]: encodes if this is a 16bit atomic operation. For 16bit atomics, the source operand shall be in an unpacked form, and only the lower 16bit part of each dword is used. The writeback (if any) has the same unpacked layout


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b011:  8 elements
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


- **Surface(ub):** Index of the surface variable


- **U(raw_operand):** The first exec_size elements contain the U offset. Must have type UD


- **V(raw_operand):** The first exec_size elements contain the V offset. Must have type UD


- **R(raw_operand):** The first exec_size elements contain the R offset. Must have type UD


- **LOD(raw_operand):** The first exec_size elements contain the LOD. Must have type UD


- **Src0(raw_operand):** For the INC and DEC atomic operation it must be V0 (the null variable). For the other operations the first <exec_size> elements of the variable will be used as src0 in ` <#anchor-484>`__ `Table 12 <#anchor-435>`__. For IMIN and IMAX it must have type D, for all other operations it must have type UD


- **Src1(raw_operand):** For the CMPXCHG operation the first <exec_size> elements of the variable will be used as src1 ` <#anchor-435>`__. The operand must have type UD. For all other  operations it must be V0


- **Dst(raw_operand):** The raw operand of a general variable storing the results of the atomic operation. For IMIN and IMAX it must have type D, for all other operations it must have type UD. Dst is permitted to be V0 (null variable), in which case no value will be returned


#### Properties
- **SIMD Control Flow:** execution mask controls the active channels
- **Out-of-bound Access:** On read: zeros are returned. On write: data is dropped.




## Text
```





[(<P>)] TYPED_ATOMIC.<op>[.16] (<exec_size>) <surface> <u> <v> <r> <lod> <src0> <src1> <dst>

//op is the text string of one of the operations (ADD, SUB, etc.)
```
## Notes






    The table below summarizes the offsets for each surface type.

        +----------------+-------------------+--------------------+--------------------+-------+
        | Surface Type   | U                 | V                  | R                  | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+
        | 1D             | X pixel address   | N/A (must be V0)   | N/A (must be V0)   | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+
        | 1D_array       | X pixel address   | Array index        | N/A (must be V0)   | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+
        | 2D             | X pixel address   | Y pixel address    | N/A (must be V0)   | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+
        | 2D_array       | X pixel address   | Y pixel address    | Array index        | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+
        | 3D             | X pixel address   | Y pixel address    | Z pixel address    | LOD   |
        +----------------+-------------------+--------------------+--------------------+-------+

    **16bit Atomics:** the operand types remain the same as 32bit atomics. And lower 16 bits of each dword will be reinterpreted as its 16 bit counterpart. Each access is a word and must be word-aligned.

