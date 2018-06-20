/**
 * This code is available at
 * http://www.mindspring.com/~pfilandr/C/fs_math/
 * and it is believed to be public domain.
 */

/* BEGIN fs_math.c */

#include "fs_math.h"

#include <float.h>
/*
** pi == (atan(1.0 / 3) + atan(1.0 / 2)) * 4
*/
static double fs_pi(void);
static long double fs_pil(void);

double fs_sqrt(double x)
{
    int n;
    double a, b;

    if (x > 0 && DBL_MAX >= x) {
        for (n = 0; x > 2; x /= 4) {
            ++n;
        }
        while (0.5 > x) {
            --n;
            x *= 4;
        }
        a = x;
        b = (1 + x) / 2;
        do {
            x = b;
            b = (a / x + x) / 2;
        } while (x > b);
        while (n > 0) {
            x *= 2;
            --n;
        }
        while (0 > n) {
            x /= 2;
            ++n;
        }
    } else {
        if (x != 0) {
            x = DBL_MAX;
        }
    }
    return x;
}

double fs_log(double x)
{
    int n;
    double a, b, c, epsilon;
    static double A, B, C;
    static int initialized;

    if (x > 0 && DBL_MAX >= x) {
        if (!initialized) {
            initialized = 1;
            A = fs_sqrt(2);
            B = A / 2;
            C = fs_log(A);
        }
        for (n = 0; x > A; x /= 2) {
            ++n;
        }
        while (B > x) {
            --n;
            x *= 2;
        }
        a = (x - 1) / (x + 1);
        x = C * n + a;
        c = a * a;
        n = 1;
        epsilon = DBL_EPSILON * x;
        if (0 > a) {
            if (epsilon > 0) {
                epsilon = -epsilon;
            }
            do {
                n += 2;
                a *= c;
                b = a / n;
                x += b;
            } while (epsilon > b);
        } else {
            if (0 > epsilon) {
                epsilon = -epsilon;
            }
            do {
                n += 2;
                a *= c;
                b = a / n;
                x += b;
            } while (b > epsilon);
        }
        x *= 2;
    } else {
        x = -DBL_MAX;
    }
    return x;
}

double fs_log10(double x)
{
    static double log_10;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        log_10 = fs_log(10);
    }
    return x > 0 && DBL_MAX >= x ? fs_log(x) / log_10 : fs_log(x);
}

double fs_exp(double x)
{
    unsigned n, square;
    double b, e;
    static double x_max, x_min, epsilon;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        x_max = fs_log(DBL_MAX);
        x_min = fs_log(DBL_MIN);
        epsilon = DBL_EPSILON / 4;
    }
    if (x_max >= x && x >= x_min) {
        for (square = 0; x > 1; x /= 2) {
            ++square;
        }
        while (-1 > x) {
            ++square;
            x /= 2;
        }
        e = b = n = 1;
        do {
            b /= n++;
            b *= x;
            e += b;
            b /= n++;
            b *= x;
            e += b;
        } while (b > epsilon);
        while (square-- != 0) {
            e *= e;
        }
    } else {
        e = x > 0 ? DBL_MAX : 0;
    }
    return e;
}

double fs_modf(double value, double *iptr)
{
    double a, b;
    const double c = value;

    if (0 > c) {
        value = -value;
    }
    if (DBL_MAX >= value) {
        for (*iptr = 0; value >= 1; value -= b) {
            a = value / 2;
            b = 1;
            while (a >= b) {
                b *= 2;
            }
            *iptr += b;
        }
    } else {
        *iptr = value;
        value = 0;
    }
    if (0 > c) {
        *iptr = -*iptr;
        value = -value;
    }
    return value;
}

double fs_fmod(double x, double y)
{
    double a, b;
    const double c = x;

    if (0 > c) {
        x = -x;
    }
    if (0 > y) {
        y = -y;
    }
    if (y != 0 && DBL_MAX >= y && DBL_MAX >= x) {
        while (x >= y) {
            a = x / 2;
            b = y;
            while (a >= b) {
                b *= 2;
            }
            x -= b;
        }
    } else {
        x = 0;
    }
    return 0 > c ? -x : x;
}

double fs_pow(double x, double y)
{
    double p = 0;

    if (0 > x && fs_fmod(y, 1) == 0) {
        if (fs_fmod(y, 2) == 0) {
            p =  fs_exp(fs_log(-x) * y);
        } else {
            p = -fs_exp(fs_log(-x) * y);
        }
    } else {
        if (x != 0 || 0 >= y) {
            p =  fs_exp(fs_log( x) * y);
        }
    }
    return p;
}

static double fs_pi(void)
{
    unsigned n;
    double a, b, epsilon;
    static double p;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        epsilon = DBL_EPSILON / 4;
        n = 1;
        a = 3;
        do {
            a /= 9;
            b  = a / n;
            n += 2;
            a /= 9;
            b -= a / n;
            n += 2;
            p += b;
        } while (b > epsilon);
        epsilon = DBL_EPSILON / 2;
        n = 1;
        a = 2;
        do {
            a /= 4;
            b  = a / n;
            n += 2;
            a /= 4;
            b -= a / n;
            n += 2;
            p += b;
        } while (b > epsilon);
        p *= 4;
    }
    return p;
}

double fs_cos(double x)
{
    unsigned n;
    int negative, sine;
    double a, b, c;
    static double pi, two_pi, half_pi, third_pi, epsilon;
    static int initialized;

    if (0 > x) {
        x = -x;
    }
    if (DBL_MAX >= x) {
        if (!initialized) {
            initialized = 1;
            pi          = fs_pi();
            two_pi      = 2 * pi;
            half_pi     = pi / 2;
            third_pi    = pi / 3;
            epsilon     = DBL_EPSILON / 2;
        }
        if (x > two_pi) {
            x = fs_fmod(x, two_pi);
        }
        if (x > pi) {
            x = two_pi - x;
        }
        if (x > half_pi) {
            x = pi - x;
            negative = 1;
        } else {
            negative = 0;
        }
        if (x > third_pi) {
            x = half_pi - x;
            sine = 1;
        } else {
            sine = 0;
        }
        c = x * x;
        x = n = 0;
        a = 1;
        do {
            b  = a;
            a *= c;
            a /= ++n;
            a /= ++n;
            b -= a;
            a *= c;
            a /= ++n;
            a /= ++n;
            x += b;
        } while (b > epsilon);
        if (sine) {
            x = fs_sqrt((1 - x) * (1 + x));
        }
        if (negative) {
            x = -x;
        }
    } else {
        x = -DBL_MAX;
    }
    return x;
}

double fs_log2(double x)
{
    static double log_2;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        log_2 = fs_log(2);
    }
    return x > 0 && DBL_MAX >= x ? fs_log(x) / log_2 : fs_log(x);
}

double fs_exp2(double x)
{
    static double log_2;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        log_2 = fs_log(2);
    }
    return fs_exp(x * log_2);
}

long double fs_powl(long double x, long double y)
{
    long double p;

    if (0 > x && fs_fmodl(y, 1) == 0) {
        if (fs_fmodl(y, 2) == 0) {
            p =  fs_expl(fs_logl(-x) * y);
        } else {
            p = -fs_expl(fs_logl(-x) * y);
        }
    } else {
        if (x != 0 || 0 >= y) {
            p =  fs_expl(fs_logl( x) * y);
        } else {
            p = 0;
        }
    }
    return p;
}

long double fs_sqrtl(long double x)
{
    long int n;
    long double a, b;

    if (x > 0 && LDBL_MAX >= x) {
        for (n = 0; x > 2; x /= 4) {
            ++n;
        }
        while (0.5 > x) {
            --n;
            x *= 4;
        }
        a = x;
        b = (1 + x) / 2;
        do {
            x = b;
            b = (a / x + x) / 2;
        } while (x > b);
        while (n > 0) {
            x *= 2;
            --n;
        }
        while (0 > n) {
            x /= 2;
            ++n;
        }
    } else {
        if (x != 0) {
            x = LDBL_MAX;
        }
    }
    return x;
}

long double fs_logl(long double x)
{
    long int n;
    long double a, b, c, epsilon;
    static long double A, B, C;
    static int initialized;

    if (x > 0 && LDBL_MAX >= x) {
        if (!initialized) {
            initialized = 1;
            B = 1.5;
            do {
                A = B;
                B = 1 / A + A / 2;
            } while (A > B);
            B /= 2;
            C = fs_logl(A);
        }
        for (n = 0; x > A; x /= 2) {
            ++n;
        }
        while (B > x) {
            --n;
            x *= 2;
        }
        a = (x - 1) / (x + 1);
        x = C * n + a;
        c = a * a;
        n = 1;
        epsilon = LDBL_EPSILON * x;
        if (0 > a) {
            if (epsilon > 0) {
                epsilon = -epsilon;
            }
            do {
                n += 2;
                a *= c;
                b = a / n;
                x += b;
            } while (epsilon > b);
        } else {
            if (0 > epsilon) {
                epsilon = -epsilon;
            }
            do {
                n += 2;
                a *= c;
                b = a / n;
                x += b;
            } while (b > epsilon);
        }
        x *= 2;
    } else {
        x = -LDBL_MAX;
    }
    return x;
}

long double fs_expl(long double x)
{
    long unsigned n, square;
    long double b, e;
    static long double x_max, x_min, epsilon;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        x_max = fs_logl(LDBL_MAX);
        x_min = fs_logl(LDBL_MIN);
        epsilon = LDBL_EPSILON / 4;
    }
    if (x_max >= x && x >= x_min) {
        for (square = 0; x > 1; x /= 2) {
            ++square;
        }
        while (-1 > x) {
            ++square;
            x /= 2;
        }
        e = b = n = 1;
        do {
            b /= n++;
            b *= x;
            e += b;
            b /= n++;
            b *= x;
            e += b;
        } while (b > epsilon);
        while (square-- != 0) {
            e *= e;
        }
    } else {
        e = x > 0 ? LDBL_MAX : 0;
    }
    return e;
}

static long double fs_pil(void)
{
    long unsigned n;
    long double a, b, epsilon;
    static long double p;
    static int initialized;

    if (!initialized) {
        initialized = 1;
        epsilon = LDBL_EPSILON / 4;
        n = 1;
        a = 3;
        do {
            a /= 9;
            b  = a / n;
            n += 2;
            a /= 9;
            b -= a / n;
            n += 2;
            p += b;
        } while (b > epsilon);
        epsilon = LDBL_EPSILON / 2;
        n = 1;
        a = 2;
        do {
            a /= 4;
            b  = a / n;
            n += 2;
            a /= 4;
            b -= a / n;
            n += 2;
            p += b;
        } while (b > epsilon);
        p *= 4;
    }
    return p;
}

long double fs_cosl(long double x)
{
    long unsigned n;
    int negative, sine;
    long double a, b, c;
    static long double pi, two_pi, half_pi, third_pi, epsilon;
    static int initialized;

    if (0 > x) {
        x = -x;
    }
    if (LDBL_MAX >= x) {
        if (!initialized) {
            initialized = 1;
            pi          = fs_pil();
            two_pi      = 2 * pi;
            half_pi     = pi / 2;
            third_pi    = pi / 3;
            epsilon     = LDBL_EPSILON / 2;
        }
        if (x > two_pi) {
            x = fs_fmodl(x, two_pi);
        }
        if (x > pi) {
            x = two_pi - x;
        }
        if (x > half_pi) {
            x = pi - x;
            negative = 1;
        } else {
            negative = 0;
        }
        if (x > third_pi) {
            x = half_pi - x;
            sine = 1;
        } else {
            sine = 0;
        }
        c = x * x;
        x = n = 0;
        a = 1;
        do {
            b  = a;
            a *= c;
            a /= ++n;
            a /= ++n;
            b -= a;
            a *= c;
            a /= ++n;
            a /= ++n;
            x += b;
        } while (b > epsilon);
        if (sine) {
            x = fs_sqrtl((1 - x) * (1 + x));
        }
        if (negative) {
            x = -x;
        }
    } else {
        x = -LDBL_MAX;
    }
    return x;
}

long double fs_fmodl(long double x, long double y)
{
    long double a, b;
    const long double c = x;

    if (0 > c) {
        x = -x;
    }
    if (0 > y) {
        y = -y;
    }
    if (y != 0 && LDBL_MAX >= y && LDBL_MAX >= x) {
        while (x >= y) {
            a = x / 2;
            b = y;
            while (a >= b) {
                b *= 2;
            }
            x -= b;
        }
    } else {
        x = 0;
    }
    return 0 > c ? -x : x;
}

/* END fs_math.c */
