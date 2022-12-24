<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FADDR = 0x50

## Format

| | | |
| --- | --- | --- |
| 0x50(FADDR) | Func_id | Dst |


## Semantics


```

                    Take the address of function <func_id> and store it to <dst>
```

## Description





```
    Take the address of function <func_id> and store it to <dst>.
```


- **Func_id(uw):** id of function name string. It is used to index into the kernel/function's string_pool table


- **Dst(scalar):** The destination operand. Must have type UD,UQ


#### Properties




## Text
```



    FADDR <func_name> <dst> // // <func_name> = string_pool[<func_id>]
```
## Notes





    The function address is represented as a 64-bit unsigned integer.

