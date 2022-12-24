<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  DWORD_ATOMIC = 0x7d

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x7d(DWORD_ATOMIC) | Op   | Exec_size | Pred | Surface | Element_offset | Src0 |
|                    | Src1 | Dst       |      |         |                |      |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        // 2 or 4 byte, atomic operation
                        UD offset = element_offset[i];
                        dst[i] =  surface[offset];
                        surface[offset] = op(surface[offset], src0, src1);
                      }
                    }
```

## Description





```
    Performs <exec_size> element scattered word or dword (either integer or float) write atomically into <surface>. The values written depend on the atomic operation being performed, and the old values from the surface are written into <dst>.

    .. _table_DWORD_ATOMIC_OP:

    .. table:: **DWORD_ATOMIC_OP**

      | Bits    | Operation                                | Message Type      | Data Type             | Return Value       |
      | --- | ---| ---| ---| ---| ---|
      | 0b00000 | ADD: new = old + src0                    | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00001 | SUB: new = old - src0                    | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00010 | INC : new = old+1                        | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00011 | DEC: new = old-1                         | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00100 | MIN: new = min(old, src0)                | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00101 | MAX: new = max(old, src0)                | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00110 | XCHG: new = src0                         | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b00111 | CMPXCHG : new = (old==src1) ? src0 : old | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b01000 | AND: new = old & src0                    | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b01001 | OR: new = old | src0                     | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b01010 | XOR: new = old ^ src0                    | DWORD, SVM, TYPED | UW, UD, UQ (SVM only) | Old value          |
      | 0b01011 | IMIN: new = min(old, src0), signed       | DWORD, SVM, TYPED | W, D, Q (SVM only)    | Old value (signed) |
      | 0b01100 | IMAX: new = max(old, src0), signed       | DWORD, SVM, TYPED | W, D, Q (SVM only)    | Old value (signed) |
      | 0b01101 | PREDEC: new = old - 1                    | DWORD, SVM, TYPED | W, D, Q (SVM only)    | New value          |
      | 0b10000 | FMAX: new = max(old, src0)               | DWORD, SVM        | HF, F                 | Old value (float)  |
      | 0b10001 | FMIN: new = min(old, src0)               | DWORD, SVM        | HF, F                 | Old value (float)  |
      | 0b10010 | FCMPWR: new = (src0 == old) ? src1 : old | DWORD, SVM        | HF, F                 | Old value (float)  |
```


- **Op(ub):**

  - Bit[4..0]: encodes the atomic operation for this message. The following table lists the valid atomic operations, with new representing the value written to the surface and old the existing value in the surface.

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
  - {TGLLP+}Bit[5]: Encodes if this is a 16bit atomic operation. For 16bit atomics, the source operand shall be in an unpacked form, and only the lower 16bit part of each dword is used. The writeback (if any) has the same unpacked layout


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b000:  1 element (scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
    - 0b011:  8 elements
    - 0b100:  16 elements
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


- **Surface(ub):** Index of the surface variable. It must be a buffer. Valid values are:

  - 0: T0 - Shared Local Memory (SLM) access
  - 5: T255 - Stateless surface access

- **Element_offset(raw_operand):** The first num_elts elements will be used as the offsets into the surface, and they are in the unit of bytes. Must have type UD


- **Src0(raw_operand):** For the INC and DEC atomic operation it must be V0 (the null variable). For the other operations the first Exec_size elements of the variable will be used as src0


- **Src1(raw_operand):** For the CMPXCHG and FCMPWR operation the first Exec_size elements of the variable will be used as src1. For all other operations it must be V0


- **Dst(raw_operand):** The raw operand storing the results of the atomic operation. Dst is permitted to be V0, in which case no value will be returned


#### Properties
- **Out-of-bound Access:** On read: zeros are returned. On write: data is dropped.




## Text
```



    [(<P>)] DWORD_ATOMIC.<Op>[.16] (<Exec_size>) <Surface> <Element_offset> <Src0> <Src1> <Dst>

//op is the text form of one of the operations (ADD, SUB, etc.)
```
## Notes





    Dst, Src0, and Src1, if present, must have the same type, and the type requirements vary based on the operation:

        - IMIN, IMAX: type D,

        - FMAX, FMIN, FCMPWR: type F

        - Other operations: type UD


    - **16bit Atomics:** The operand types remain the same as 32bit atomics. And lower 16 bits of each dword will be reinterpreted as its 16 bit counterpart. Each access is a word and must be word-aligned.

    If more than one channel writes to the same address, they will be serialized and atomically updated, though the order is non-deterministic.

