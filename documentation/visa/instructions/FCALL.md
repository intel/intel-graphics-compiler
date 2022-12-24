<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FCALL = 0x67

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x67(FCALL) | Exec_size | Pred | Func_id | Arg_size | Return_size |


## Semantics


```

                    Calls the function <func_id>.
```

## Description





```
    Calls the function <func_id> based on the predicate value.

    If <exec_size> is one: Execution jumps to the function if the predicate is true. The call mask will be initialized to all ones at function entry. Scalar calls must be marked with {NoMask}.

    If <exec_size> is greater than one: The call is executed if any of the active channels are predicated. At function entry, the call mask will be initialized to the set of active channels that are predicated.

    If the call is executed, this instruction copies the first <arg_size> GRFs of %arg from the caller to the callee, destroying its contents in the caller during the process. It also copies the value of stack (%sp) and frame pointer (%fp) from the caller to the callee. When the call returns, the first <return_size> GRFs of %retval holds the return values. It is an error if <arg_size> and <return_size> do not match the callee's declared values.
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


- **Func_id(uw):** Id of the function name string. It is used to index into the kernel/function's string_pool table


- **Arg_size(ub):** Argument size in unit of GRF. Valid values are  [0-sizeof(%arg)]


- **Return_size(ub):** Return size in unit of GRF. Valid values are  [0-sizeof(%retval)]


#### Properties




## Text
```



    [(<P>)] FCALL (<exec_size>) <func_name> <arg_size> <return_size> // <func_name> = string_pool[<func_id>]
```
## Notes





