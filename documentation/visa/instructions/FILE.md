<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FILE = 0x51

## Format

| | |
| --- | --- |
| 0x51(FILE) | File_name |


## Semantics


```

      Source file name for the subsequent instructions.
```

## Description





    Specify the source file name for the subsequent instruction. The source
    file name remains in effect until the next FILE instruction changes it.
    This instruction is used for debugging purposes.


- **File_name(ud):** index to the string storing the name of the kernel, with a maximum length of 255 characters


#### Properties




## Text
```



    FILE <file_name>

    //<file_name> is a constant string.
```
## Notes





