/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.

Permission to use, copy, modify, and distribute this software is freely granted,
provided that this notice is preserved.

============================= end_copyright_notice ===========================*/

/* Sometimes it's necessary to define __LITTLE_ENDIAN explicitly
but these catch some common cases. */

#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x
#define __HIp(x) *(1+(int*)x)
#define __LOp(x) *(int*)x

#ifdef __STDC__
#define    __P(p)    p
#else
#define    __P(p)    ()
#endif

/*
* ANSI/POSIX
*/

#define    MAXFLOAT    ((float)3.40282346638528860e+38)

#define    HUGE        MAXFLOAT

/*
* set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
* (one may replace the following line by "#include <values.h>")
*/

#define X_TLOSS        1.41484755040568800000e+16

#define    DOMAIN        1
#define    SING        2
#define    OVERFLOW    3
#define    UNDERFLOW    4
#define    TLOSS        5
#define    PLOSS        6

extern double __scalbn_tmp __P((double, int));

extern int matherr __P((struct exception *));


/* ieee style elementary functions */
extern double __ieee754_sqrt __P((double));
extern double __ieee754_acos __P((double));
extern double __ieee754_acosh __P((double));
extern double __ieee754_log __P((double));
extern double __ieee754_atanh __P((double));
extern double __ieee754_asin __P((double));
extern double __ieee754_atan2 __P((double, double));
extern double __ieee754_exp __P((double));
extern double __ieee754_cosh __P((double));
extern double __ieee754_fmod __P((double, double));
extern double __ieee754_pow __P((double, double));
extern double __ieee754_lgamma_r __P((double, int *));
extern double __ieee754_gamma_r __P((double, int *));
extern double __ieee754_lgamma __P((double));
extern double __ieee754_gamma __P((double));
extern double __ieee754_log10 __P((double));
extern double __ieee754_sinh __P((double));
extern double __ieee754_hypot __P((double, double));
extern double __ieee754_j0 __P((double));
extern double __ieee754_j1 __P((double));
extern double __ieee754_y0 __P((double));
extern double __ieee754_y1 __P((double));
extern double __ieee754_jn __P((int, double));
extern double __ieee754_yn __P((int, double));
extern double __ieee754_remainder __P((double, double));
extern int    __ieee754_rem_pio2 __P((double, double*));

extern double __ieee754_scalb __P((double, int));

/* fdlibm kernel function */
extern double __kernel_standard __P((double, double, int));
extern double __kernel_sin __P((double, double, int));
extern double __kernel_cos __P((double, double));
extern double __kernel_tan __P((double, double, int));
extern int    __kernel_rem_pio2 __P((double*, double*, int, int, int, const int*));
