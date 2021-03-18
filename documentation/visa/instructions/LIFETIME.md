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

  LIFETIME = 0x7b

## Format

| | |
| --- | --- |
| 0x7b(LIFETIME) | properties | variable |


## Semantics




      Marks the start/end of a variable's lifetime.

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
