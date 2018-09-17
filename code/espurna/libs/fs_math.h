/**
 * This code is available at
 * http://www.mindspring.com/~pfilandr/C/fs_math/
 * and it is believed to be public domain.
 */

/* BEGIN fs_math.h */
/*
** Portable freestanding code.
*/
#ifndef H_FS_MATH_H
#define H_FS_MATH_H

double fs_sqrt(double x);
double fs_log(double x);
double fs_log10(double x);
/*
** exp(x) = 1 + x + x^2/2! + x^3/3! + ...
*/
double fs_exp(double x);
double fs_modf(double value, double *iptr);
double fs_fmod(double x, double y);
double fs_pow(double x, double y);
double fs_cos(double x);
/*
** C99
*/
double fs_log2(double x);
double fs_exp2(double x);
long double fs_powl(long double x, long double y);
long double fs_sqrtl(long double x);
long double fs_logl(long double x);
long double fs_expl(long double x);
long double fs_cosl(long double x);
long double fs_fmodl(long double x, long double y);

#endif

/* END fs_math.h */

#if 0

/*
> > Anybody know where I can get some source code for a
> > reasonably fast double
> > precision square root algorithm in C.
> > I'm looking for one that is not IEEE
> > compliant as I am running on a Z/OS mainframe.
> >
> > I would love to use the standard library but
> > unfortunatly I'm using a
> > stripped down version of C that looses the the runtime library
> > (we have to write our own).
>
> long double Ssqrt(long double x)
> {
>     long double a, b;
>     size_t c;

size_t is a bug here.
c needs to be a signed type:
    long c;

>     if (x > 0) {
>         c = 0;
>         while (x > 4) {
>             x /= 4;
>             ++c;
>         }
>         while (1.0 / 4 > x) {
>             x *= 4;
>             --c;
>         }
>         a = x;
>         b = ((4 > a) + a) / 2;

Not a bug, but should be:
    b = (1 + a) / 2;

>         do {
>             x = b;
>             b = (a / x + x) / 2;
>         } while (x > b);
>         if (c > 0) {

The above line is why c needs to be a signed type,
otherwise the decremented values of c, are greater than zero,
and the function won't work if the initial value of x
is less than 0.25

>             while (c--) {
>                 x *= 2;
>             }
>         } else {
>             while (c++) {
>                 x /= 2;
>             }
>         }
>     }
>     return x;
> }

>
> >
> > That algorithm was actually 4 times slower
> > then the one below, and more
> > code. It was accurate though.
> >
>
> Sorry Pete, I wasn't looking very carefully.
> When I converted your function
> to double precision it's was much quicker, the best I've seen yet.

*/

#endif
