#pragma once

#include <math.h>

namespace TinyCalc {

namespace detail {

/**
 * @brief Primitive C library math functions, generic version.
 *
 * Computes in double precision and converts the result to T. Specialized for
 * float below.
 */
template <typename T>
struct Prim {
#define TC_PRIM_GEN1(fn) \
  static T fn(T x) { return static_cast<T>(::fn(static_cast<double>(x))); }
#define TC_PRIM_GEN2(fn) \
  static T fn(T x, T y) { \
    return static_cast<T>(::fn(static_cast<double>(x), static_cast<double>(y))); \
  }
  TC_PRIM_GEN1(sin) TC_PRIM_GEN1(cos) TC_PRIM_GEN1(tan)
  TC_PRIM_GEN1(asin) TC_PRIM_GEN1(acos) TC_PRIM_GEN1(atan)
  TC_PRIM_GEN1(exp) TC_PRIM_GEN1(log) TC_PRIM_GEN1(log10) TC_PRIM_GEN1(sqrt)
  TC_PRIM_GEN1(floor) TC_PRIM_GEN1(ceil) TC_PRIM_GEN1(fabs)
  TC_PRIM_GEN2(atan2) TC_PRIM_GEN2(pow) TC_PRIM_GEN2(fmod)
#undef TC_PRIM_GEN1
#undef TC_PRIM_GEN2
};

/**
 * @brief float specialization of detail::Prim: uses the single-precision C
 * functions (sinf, cosf, ...).
 */
template <>
struct Prim<float> {
#define TC_PRIM_F1(fn) static float fn(float x) { return ::fn##f(x); }
#define TC_PRIM_F2(fn) static float fn(float x, float y) { return ::fn##f(x, y); }
  TC_PRIM_F1(sin) TC_PRIM_F1(cos) TC_PRIM_F1(tan)
  TC_PRIM_F1(asin) TC_PRIM_F1(acos) TC_PRIM_F1(atan)
  TC_PRIM_F1(exp) TC_PRIM_F1(log) TC_PRIM_F1(log10) TC_PRIM_F1(sqrt)
  TC_PRIM_F1(floor) TC_PRIM_F1(ceil) TC_PRIM_F1(fabs)
  TC_PRIM_F2(atan2) TC_PRIM_F2(pow) TC_PRIM_F2(fmod)
#undef TC_PRIM_F1
#undef TC_PRIM_F2
};

}  // namespace detail

/**
 * @class Math
 * @brief Mathematical functions and constants for the number type T.
 *
 * All functions are static. They are built on the C library primitives in
 * detail::Prim, so the float version uses the single-precision functions.
 * Functions with a restricted domain (sqrt, ln, asin, ...) do not check it here;
 * the evaluator reports Error::Domain before calling them.
 *
 * @tparam T  number type: float or double
 */
template <typename T>
class Math {
 public:
  typedef detail::Prim<T> P;

  static T c(double v) { return static_cast<T>(v); }

  // constants
  static T pi() { return c(3.14159265358979323846); }
  static T e() { return c(2.71828182845904523536); }
  static T tau() { return c(6.28318530717958647692); }
  static T phi() { return c(1.61803398874989484820); }

  // classification
  static bool isFinite(T x) { return (x - x) == c(0); }
  static bool isNaN(T x) { return x != x; }
  static bool isInteger(T x) { return isFinite(x) && P::floor(x) == x; }
  static bool isNonNegativeInteger(T x) { return isInteger(x) && x >= c(0); }

  // rounding and sign
  static T absolute(T x) { return P::fabs(x); }
  static T sign(T x) { return x > c(0) ? c(1) : (x < c(0) ? c(-1) : c(0)); }
  static T trunc(T x) { return x < c(0) ? -P::floor(-x) : P::floor(x); }
  static T roundHalf(T x) {  // half away from zero
    return x < c(0) ? -P::floor(-x + c(0.5)) : P::floor(x + c(0.5));
  }
  static T frac(T x) { return x - trunc(x); }

  // hyperbolic functions (built on exp, which every C library provides)
  static T sinh(T x) { return (P::exp(x) - P::exp(-x)) / c(2); }
  static T cosh(T x) { return (P::exp(x) + P::exp(-x)) / c(2); }
  static T tanh(T x) {
    if (x > c(20)) return c(1);
    if (x < c(-20)) return c(-1);
    T e2 = P::exp(c(2) * x);
    return (e2 - c(1)) / (e2 + c(1));
  }
  static T asinh(T x) {
    T ax = absolute(x);
    T r = P::log(ax + P::sqrt(ax * ax + c(1)));
    return x < c(0) ? -r : r;
  }
  static T acosh(T x) { return P::log(x + P::sqrt(x * x - c(1))); }  // x >= 1
  static T atanh(T x) { return c(0.5) * P::log((c(1) + x) / (c(1) - x)); }  // |x| < 1

  // roots and exponentials
  static T cbrt(T x) {
    if (x == c(0)) return c(0);
    T a = absolute(x);
    T y = P::exp(P::log(a) / c(3));
    y = y - (y * y * y - a) / (c(3) * y * y);  // one Newton step
    return x < c(0) ? -y : y;
  }
  static T exp2(T x) { return P::pow(c(2), x); }
  static T expm1(T x) {
    if (absolute(x) < c(1e-4)) return x + x * x / c(2) + x * x * x / c(6);
    return P::exp(x) - c(1);
  }
  static T log2(T x) { return P::log(x) / c(0.693147180559945309417232121458); }

  static T hypot(T x, T y) {
    T a = absolute(x), b = absolute(y);
    if (a < b) {
      T t = a;
      a = b;
      b = t;
    }
    if (a == c(0)) return c(0);
    T r = b / a;
    return a * P::sqrt(c(1) + r * r);
  }

  // Gamma function. Caller must exclude non-positive integers.
  // Reflection for x < 0.5; otherwise the argument is shifted up to >= 15
  // using gamma(x + 1) = x * gamma(x), and the Stirling series for ln(gamma)
  // is evaluated (absolute error ~1e-14 in double precision).
  static T gamma(T x) {
    if (x < c(0.5)) return pi() / (P::sin(pi() * x) * gamma(c(1) - x));
    T shift = c(1);
    while (x < c(15)) {
      shift /= x;
      x += c(1);
    }
    T z = x;
    T z2 = z * z;
    T lnG = (z - c(0.5)) * P::log(z) - z + c(0.918938533204672741780329736406);  // ln(sqrt(2pi))
    lnG += (c(1) / c(12)) / z
         - (c(1) / c(360)) / (z * z2)
         + (c(1) / c(1260)) / (z * z2 * z2)
         - (c(1) / c(1680)) / (z * z2 * z2 * z2);
    return P::exp(lnG) * shift;
  }

  // Error function. Uses a series with only positive terms, so there is no
  // cancellation; for |x| > 6 the result is +-1 to working precision.
  static T erf(T x) {
    if (x == c(0)) return c(0);
    T ax = absolute(x);
    if (ax > c(6)) return x < c(0) ? c(-1) : c(1);
    const T x2 = c(2) * ax * ax;
    T term = ax, sum = ax;
    for (int n = 1; n < 400; ++n) {
      term *= x2 / c(2 * n + 1);
      sum += term;
      if (term <= sum * c(1e-9)) break;
    }
    T r = c(1.1283791670955126) * P::exp(-ax * ax) * sum;  // 2/sqrt(pi)
    return x < c(0) ? -r : r;
  }
  static T erfc(T x) { return c(1) - erf(x); }
};

}  // namespace TinyCalc
