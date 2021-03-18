<!---======================= begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ==========================-->

 

## Opcode

  RAW_SEND = 0x5d

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x5d(RAW_SEND) | Modifiers | Exec_size | Pred | ExMsgDesc | NumSrc | NumDst |
|                | Desc      | Src       | Dst  |           |        |        |


## Semantics




      A direct method for generating a GEN native send message

## Description



    A raw message send that will be translated into a GEN send instruction

- **Modifiers(ub):** 
 
  - Bit[0]: Determines whether send or sendc should be used
 
    - 0b0:  raw_send 
    - 0b1:  raw_sendc
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

- **ExMsgDesc(ud):** Extended message descriptor for the send

- **NumSrc(ub):** Number of GRF registers for the send message payload. Valid values are  [1-15]

- **NumDst(ub):** Number of GRF registers for the send message response. Valid values are  [0-16]

- **Desc(scalar):** Message descriptor for the send. Must have type UD

- **Src(raw_operand):** The raw operand of a general variable storing the send message payload

- **Dst(raw_operand):** The raw operand of a general variable storing send message response

#### Properties


## Text
```
    

		[(<P>)] {raw_send|raw_sendc} (<exec_size>) <ExMsgDesc> <NumSrc> <NumDst> <Desc> <Src> <Dst>
```



## Notes



    This instruction is intended for expert users who want to access a fixed
    function that is currently not available in vISA. As such, it is the
    user's responsibility to ensure that the message descriptor, shared
    function id, together with the message source and destination form a
    correct send instruction for the given GEN platform. The JIT-compiler
    simply translates it into a GEN send instruction without any error
    check.
