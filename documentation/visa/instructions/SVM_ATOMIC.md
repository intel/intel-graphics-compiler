<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SVM = 0x4e
  ATOMIC = 0x05

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x4e(SVM) | 0x05(ATOMIC) | Exec_size | Pred | Op | Addresses | Src0 | Src1 |
|           |              | Dst       |      |    |           |      |      |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                        if (ChEn[i]) {
                            // 2, 4, or 8 byte, atomic operation
                            dst[i] = *(addresses[i]);
                            *(addresses[i]) = op(*(addresses[i]), src0, src1);
                        }
                    }
```

## Description





```
    Performs 8 element scattered atomic "read-modify-write" operations to
    <addresses>. The values written depend on the atomic operation being
    performed, and the old values from the address are returned.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b000:  1 element (scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
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
  - {TGLLP+}Bit[6..5]: encodes the data width of the atomic operation

    - 0b00:  32-bit
    - 0b01:  16-bit
    - 0b10:  64-bit

- **Addresses(raw_operand):** The general variable storing the virtual addresses. The first 8 elements of the variable will be used, and they are in the unit of bytes. Each address must be Dword or Qword aligned depending on the type of Dst. Must have type UQ


- **Src0(raw_operand):** For the INC and DEC atomic operation it must be V0 (the null variable). For the other operations the first exec elements of the variable will be used as src0


- **Src1(raw_operand):** For the CMPXCHG and FCMPWR operation the first exec elements of the variable will be used as src1. For all other operations it must be V0


- **Dst(raw_operand):** The raw operand storing the results of the atomic operation. Dst is permitted to be V0, in which case no value will be returned


#### Properties




## Text
```



    [(<P>)] SVM_ATOMIC.<op>[.16|.64] (<exec_size>) <addresses> <dst> <src0> <src1>  // if neither .16 nor .64 is specified, 32-bit atomics is implied
```
## Notes





    -   For 16bit atomics, the source operand shall be in an unpacked form, and only the lower 16bit part of each dword is used. The writeback (if any) has the same unpacked layout.
    -   For 64bit atomics, src and dst operands must have 64-bit types if they are not null.

    -   Dst, Src0, and Src1, if present, must have the same type, and the type requirements vary based on the operation:
            - IMIN, IMAX: type D, Q
            - FMAX, FMIN, FCMPWR: type F
            - Other operations: type UD. UQ

    - **16bit Atomics:** the operand types remain the same as 32bit atomics. And lower 16 bits of each dword will be reinterpreted as its 16 bit counterpart. Each access is a word and must be word-aligned.

