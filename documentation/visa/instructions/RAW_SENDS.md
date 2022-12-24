<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  RAW_SENDS = 0x7a

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x7a(RAW_SENDS) | Modifiers | Exec_size | Pred | SFID | NumSrc0 | NumSrc1 |
|                 | NumDst    | ExMsgDesc | Desc | Src0 | Src1    | Dst     |


## Semantics


```

      A direct method for generating a GEN native split send message
```

## Description





    A raw message send that will be translated into a GEN split send instruction


- **Modifiers(ub):**

  - Bit[0]: Determines whether sends or sendsc should be used

    - 0b0:  raw_sends
    - 0b1:  raw_sendsc
  - Bit[1]: Determines whether EOT is set or not


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


- **SFID(ub):** Shared function Id targeted by this send message. Valid values are  [0-15]


- **NumSrc0(ub):** Number of GRF registers for the send message payload 0


- **NumSrc1(ub):** Number of GRF registers for the send message payload 1


- **NumDst(ub):** Number of GRF registers for the send message response


- **ExMsgDesc(scalar):** Extended message descriptor for the send. Must have type UD


- **Desc(scalar):** Message descriptor for the send. Must have type UD


- **Src0(raw_operand):** The general variable storing the send  message payload 0


- **Src1(raw_operand):** The general variable storing the send message payload 1


- **Dst(raw_operand):** The raw operand of a general variable storing send message response


#### Properties




## Text
```



    [(<P>)] {raw_sends|raw_sendsc}[_eot] <sfid> <numSrc0> <numSrc1> <numDst> (<exec_size>) <ExMsgDesc> <Desc> <Src0> <Src1> <Dst>
```
## Notes





