<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  LABEL = 0x31

## Format

| | |
| --- | --- |
| 0x31(LABEL) | Label |


## Semantics


```

                    Declare a basic block.
```

## Description





```
    Create a new basic block starting at <label>.
```


- **Label(uw):** Index of the label variable for the subroutine. It must be a block label


#### Properties




## Text
```
LABEL <label>
```

## Notes





    The instruction itself is a nop. Label marks the start of the basic block, and may be used as the target of a JMP instruction.
    There is no instruction to mark the end of a basic block; a basic block ends when we reach another control flow instruction.

