<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SUBROUTINE = 0x30

## Format

| | |
| --- | --- |
| 0x30(SUBROUTINE) | Label |


## Semantics


```

                    Declare a subroutine.
```

## Description





```
    Marks a new subroutine starting at <label>.

    The instruction itself is a nop. Label marks the start of the subroutine, and may be used as the target of a CALL instruction.
```


- **Label(uw):** Index of the label variable for the subroutine.  It must be a subroutine label


#### Properties




## Text
```



    SUBROUTINE <label>
```
## Notes





    There is no instruction to mark the end of a subroutine; a subroutine ends when we reach another SUBROUTINE  instruction or the end of file.

