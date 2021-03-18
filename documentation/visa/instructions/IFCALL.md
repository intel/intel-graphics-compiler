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

  IFCALL = 0x4f

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x4f(IFCALL) | Exec_size | Pred | Func_addr | Arg_size | Return_size |


## Semantics




                    Calls the function located at <func_addr>

## Description


    Calls the function located at <func_addr> based on the predicate value.

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

- **Func_addr(scalar):** A scalar value representing the address of the function being called. Must have type UD

- **Arg_size(ub):** Argument size in unit of GRF. Valid values are  [0-%arg]

- **Return_size(ub):** Return size in unit of GRF. Valid values are  [0-%retval]

#### Properties


## Text
```
    

		[(<P>)] IFCALL (<exec_size>) <func_id> <arg_size> <return_size>
```



## Notes




    If <exec_size> is one: Execution jumps to the function if the predicate is true. The call mask will be initialized to all ones at function entry. Scalar calls must be marked with {NoMask}.

    If <exec_size> is greater than one: The call is executed if any of the active channels are predicated. At function entry, the call mask will be initialized to the set of active channels that are predicated.

    <func_addr> is a 32-bit unsigned integer value that represents the address of a vISA function. The FADDR instruction may be used to obtain the value of a vISA function's address.
    The behavior is undefined if <func_addr> is not the address of any of the functions. 

    If the call is executed, this instruction copies the first <arg_size> GRFs of %arg from the caller to the callee, destroying its contents in the caller during the process. It also copies the value of stack (%sp) and frame pointer (%fp) from the caller to the callee. When the call returns, the first <return_size> GRFs of %retval holds the return values. It is an error if <arg_size> and <return_size> do not match the callee's declared values.
