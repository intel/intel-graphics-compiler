<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Data Types

Fundamental Data Types
======================

The virtual ISA uses the native data types supported by the GEN
architecture, plus a new **bool** type to support predication. The bool
type has one-bit size and can support values of either 0 (false) or 1
(true). It may only be used by predicate variables (see [Execution Model](3_execution_model.md)).
Following table lists the supported fundamental
data types together with their binary and text format in a virtual ISA
object file:


| Data Type                      | Size(bits)| Range                                                                                                                          | Binary Format| Text Format|
| --- | --- | --- | --- | --- |
| unsigned double word            | 32         | [0, 2<sup>32</sup>-1]                                                                                                           | 0b0000        | UD          |
| signed double word              | 32         | [-2<sup>31</sup>, 2<sup>31</sup>-1]                                                                                             | 0b0001        | D           |
| unsigned word                   | 16         | [0, 65535]                                                                                                                      | 0b0010        | UW          |
| signed word                     | 16         | [-32768, 32767]                                                                                                                 | 0b0011        | W           |
| unsigned byte                   | 8          | [0, 255]                                                                                                                        | 0b0100        | UB          |
| signed byte                     | 8          | [-128, 127]                                                                                                                     | 0b0101        | B           |
| double precision floating point | 64         | [-(2-2<sup>-52</sup>)x2<sup>1023</sup> ... -2<sup>-1074</sup>, 0.0, 2<sup>-1074</sup> ... (2-2<sup>-52</sup>)x2<sup>1023</sup>] | 0b0110        | DF          |
| single precision floating point | 32         | [-(2-2<sup>-23</sup>)x2<sup>127</sup> ... -2<sup>-149</sup>, 0.0, 2<sup>-149</sup> ... (2-2<sup>-23</sup>)x2<sup>127</sup>]     | 0b0111        | F           |
| packed integer vector           | 32         | [-8,7]                                                                                                                          | 0b1000        | V           |
| packed floating vector          | 32         | [-31...-0.125, 0, 0.125...31]                                                                                                   | 0b1001        | VF          |
| boolean                         | 1          | [0, 1]                                                                                                                          | 0b1010        | BOOL        |
| unsigned quad word              | 64         | [0, 2<sup>64</sup>-1]                                                                                                           | 0b1011        | UQ          |
| packed unsigned integer vector  | 32         | [0, 15]                                                                                                                         | 0b1100        | UV          |
| signed quad word                | 64         | [-2<sup>63</sup>, 2<sup>63</sup>-1]                                                                                             | 0b1101        | Q           |
| half precision floating point   | 16         | [-(2-2<sup>-10</sup>)x2<sup>15</sup> ... -2<sup>-25</sup>, 0.0, 2<sup>-25</sup> ...  (2-2<sup>-10</sup>)x2<sup>15</sup>]        | 0b1110        | HF          |
| bfloat16                        | 16         | [-(2-2<sup>-7</sup>)x2<sup>127</sup> ... -2<sup>-134</sup>, 0.0, 2<sup>-134</sup> ...  (2-2<sup>-7</sup>)x2<sup>127</sup>]      | 0b1111        | BF          |


The F and DF data type follow the single precision and double precision
floating point formats specified in the IEEE standard 754-1985. The HF
data type follows the binary16 format specified in IEEE standard
754-2008. The packed signed half-byte integer vector type (V) consists
of eight signed half-byte integers in one dword, with each element
having a range of \[-8, 7\]. The packed 8-bit restricted float vector
type (VF) consists of four 8-bit restricted floats in one dword, and
rules on how to convert the restricted floats into single precision
floats may be found in \[1\]. The packed unsigned half-byte integer
vector type (UV) consists of eight unsigned half-byte integers in one
dword, with each element having a range of \[0, 15\].

The binary data formats for floating point numbers are in the form
**numOfSign - numOfExponent - numOfMantissa** (mantisa aka fraction), as shown below:

-   DF: 1-11-52 (sign [63:63], exponent [62:52], mantissa [51:0]).
-    F: 1-8-23 (sign [31:31], exponent [30:23], Mantissa [22:0]).
-   HF: 1-5-10 (sign [15:15], exponent [14:10], Mantissa [9:0]).
-   BF: 1-8-7 (sign [15:15], exponent [14:7], Mantissa [6:0]).
    Also known as bfloat16 and not an IEEE standard yet.

Floating Point Mode
===================

vISA currently supports two floating point computation mode, IEEE and
ALT. The floating point mode may be changed at execution time by writing
to the control register.

IEEE Floating Point Mode
------------------------

In the IEEE mode, vISA floating point operations honor the IEEE-754
standard with the following deviations:

-   Half float (HF) denorms are flushed to sign-preserved zero on input
    and output of any floating-point mathematical operation.
-   F and DF denorms are kept or flushed in mathematical operations
    based on the Single/Double Precision Denorm Mode in the Control
    Register.

Finally, fused operations (FMA, DP4, DP3, etc.) may produce intermediate
results out of 32-bit float range, but whose final results would be
within 32-bit float range if intermediate results were kept at greater
precision. In this case, implementations are permitted to produce either
the correct result, or else +inf. Thus, compatibility between a fused
operation, such as FMA, with the unfused equivalent, MUL followed by ADD
in this case, is not guaranteed.

ALT (alternative) Single Floating Point Mode
--------------------------------------------

The ALT mode is intended for legacy DirectX 9 applications that should
not pass NaN, infinity, and denorm values as input, and the graphics
hardware must also not generate them as computation results. ALT mode is
supported only for single precision floating point. Here is the complete
list of the differences of legacy graphics mode from the relaxed
IEEE-754 floating point mode.

-   Any +/- INF result must be flushed to +/- fmax, instead of being
    output as +/- INF.
-   Certain mathematical instructions (log, rsq, and sqrt)
    take the absolute value of the sources before
    computation to avoid generating INF and NaN results.

Type Conversion
===============

All virtual ISA operands have a known type. The type of all operands,
except for indirect operands, can be
obtained from their associated variable declarations. Indirect operands
have an explicitly associated type. For arithmetic and logic
instructions, sources can have mixed integer types but cannot have mixed
integer and float types. For the purpose of
establishing the execution type, the V type is compatible with all
signed integer types, the UV type is compatible with all unsigned
integer types, and the VF type is compatible with the F type.

If an instruction has floating-point execution type, its destination
type must also be the same as the execution type. If an instruction has
integer execution type, its destination type may be any of the integer
types. For integer execution types, extra precision is provided within
the hardware; if the value of the arithmetic operation is represented
with greater precision than required by the destination type, implicit
type conversion happens even if destination type is the same as the
execution type. Explicit type conversion (e.g., between integer and
floating-point types) may be performed through the **MOV** instruction.

Float to Integer
----------------

Floating-point to integer conversion is done by discarding the
fractional part (i.e., rounding toward zero). If the resulting value is
larger (or smaller) than the largest (or smallest) value that can be
represented by the integer type, the conversion result takes the largest
(or smallest) integer value. The conversion rules are summarized below,
where Imax is the largest representable value by the integer type while
Imin is the smallest representable value by the integer type.

Floating-Point to Integer Conversion Results Signed Integer Types

| FP Value                    | Integer Result |
| --- | --- |
|  qNaN                       | 0 |
|  sNaN                       | 0 |
|  +inf                       | Imax (refer to table&lt;table_Datatypes&gt; for each type's value) |
|  f &gt; Imax                | Imax (refer to table&lt;table_Datatypes&gt; for each type's value) |
|  Imin &lt;= f &lt;= Imax    | f |
|  f &lt; Imin                | Imin (refer to table&lt;table_Datatypes&gt; for each type's value) |
|  -inf                       | Imin (refer to table&lt;table_Datatypes&gt; for each type's value) |

Floating-Point to Integer Conversion Results Unsigned Integer Types

| FP Value                    | Integer Result |
| --- | --- |
| qNaN                        | 0 |
| sNaN                        | 0 |
| +inf                        | Imax (refer to table&lt;table_Datatypes&gt; for each type's value) |
| f &gt; Imax                 | Imax (refer to table&lt;table_Datatypes&gt; for each type's value) |
| 0 &lt;= f &lt;= Imax        | f |
| f = -0                      | 0 |
| f = - denorm                | 0 |
| fmax &lt;= f &lt;= fmin     | qNaN Indefinite |
| -inf                        | qNaN Indefinite |

Integer to Integer with Same or Higher Precision
------------------------------------------------

Converting an unsigned integer type to another type with higher
precision is based on zero extension. Converting a signed integer to
another type with higher precision is based on sign extension. If the
source and destination type have the same precision, the bits will not
be modified.

Integer to Integer with Lower Precision
---------------------------------------

Converting an integer type to another with lower precision is based on
bit truncation. Only the lower bits are kept regardless of the
signed-ness of the source and destination type.

Integer to Float
----------------

Converting an integer type to a floating point type rounds to the
closest representable floating-point value. By default "round to nearest
even" is performed.

Float to Float with Lower Precision
-----------------------------------

Converting a floating-point number to another type with lower precision
(DF -&gt; F, DF -&gt; HF, F -&gt; HF) uses the round to zero rounding
mode.

| Source Float (F, DF)    |  Destination Float (HF, F) |
| --- | --- |
| -inf                    |  -inf |
| -finite                 |  -finite/-denorm/-0 |
| -denorm                 |  -0 |
| -0                      |  -0 |
| +0                      |  +0 |
| +denorm                 |  +0 |
| +finite                 |  +finite/+denorm/+0 |
| +inf                    |  +inf |
| NaN                     |  NaN |

Float to Float with Higher Precision
------------------------------------

Converting a floating-point number to another type with higher precision
(HF -&gt; F, HF -&gt; DF, F -&gt; DF) will produce a precise
representation of the input.

| Source Float (HF, F)   |  Destination Float (F, DF) |
| --- | --- |
| -inf                   |  -inf |
| -finite                |  -finite |
| -denorm                |  -finite |
| -0                     |  -0 |
| +0                     |  +0 |
| +denorm                |  +finite |
| +finite                |  +finite |
| +inf                   |  +inf |
| NaN                    |  NaN |

Saturation
==========

The virtual ISA supports destination saturation for most arithmetic
instructions as well as the MOV instruction. Saturation is a clamping
operation that converts any data that is outside the *saturation target
range* for the destination data type to the *closest represented* value
with the target range. If the destination type is float, saturation
target range is \[0.0, 1.0\]. Any floating-point value greater than 1.0
(including +inf) saturates to 1.0, while any negative floating value
(including -inf) saturates to 0.0. NaN saturates to 0.0. Floating point
values between 0.0 and 1.0 are unchanged by saturation.

For integer data types, the maximum range for the given numerical data
type is the saturation target range. Specifically, if integer arithmetic
overflows, with saturation the result will be either the largest or the
smallest representable value of the destination type. Following table
lists the saturation target range for data types that
support saturation.

| Destination Type    | Saturation Target Range (inclusive)   |
| --- | --- |
| Half Float (HF)     | [0.0, 1.0]                            |
| Float (F)           | [0.0, 1.0]                            |
| Double Float (DF)   | [0.0, 1.0]                            |
| Byte (UB)           | [0, 255]                              |
| Signed Byte (B)     | [-128, 127]                           |
| Word (UW)           | [0, 65535]                            |
| Signed Word (W)     | [-32768, 32767]                       |
| Double Word (UD)    | [0, 2<sup>32</sup>-1]                 |
| Signed Double (D)   | [-2<sup>31</sup>, 2<sup>31</sup>-1]   |
| Quad Word (UQ)      | [0, 2<sup>64</sup>-1]                 |
| Signed Quad (Q)     | [-2<sup>63</sup>, 2<sup>63</sup>-1]   |

