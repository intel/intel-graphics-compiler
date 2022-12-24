<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  LIFETIME = 0x7b

## Format

| | | |
| --- | --- | --- |
| 0x7b(LIFETIME) | properties | variable |


## Semantics


```

      Marks the start/end of a variable's lifetime.
```

## Description





    Specifies either the start or the end of a variable's lifetime.


- **properties(ub):**

  - Bit[0]: indicates whether this is the start or the end

    - 0b0:  start
    - 0b1:  end
  - Bit[5..4]: specifies the variable class

    - 0b00:  general variable
    - 0b01:  address variable
    - 0b10:  predicate variable

- **variable(ud):**


#### Properties




## Text
```



    LIFETIME.start <variable>

    LIFETIME.end <variable>
```
## Notes





    The behavior is undefined if the variable or one of its aliases is
    referenced directly or indirectly outside the lifetime markers.

