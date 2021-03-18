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

  FADDR = 0x50

## Format

| | |
| --- | --- |
| 0x50(FADDR) | Func_id | Dst |


## Semantics




                    Take the address of function <func_id> and store it to <dst>

## Description


    Take the address of function <func_id> and store it to <dst>.

- **Func_id(uw):** id of function name string. It is used to index into the kernel/function's string_pool table

- **Dst(scalar):** The destination operand. Must have type UD,UQ

#### Properties


## Text
```
    

		FADDR <func_name> <dst>	// // <func_name> = string_pool[<func_id>]
```



## Notes



    The function address is represented as a 64-bit unsigned integer.
