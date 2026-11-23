#pragma once

#include <stddef.h>
#include <stdint.h>
#include "Expression.h"
#include "Functions.h"
#include "Math.h"
#include "Types.h"

namespace TinyCalc {
namespace detail {

template <typename T>
T toRadians(T v, bool degrees) {
  return degrees ? v * (Math<T>::pi() / Math<T>::c(180)) : v;
}

template <typename T>
T fromRadians(T v, bool degrees) {
  return degrees ? v * (Math<T>::c(180) / Math<T>::pi()) : v;
}

template <typename T>
Error powChecked(T a, T b, T& out) {
  if (a == T() && b < T()) return Error::DivisionByZero;
  if (a < T() && !Math<T>::isInteger(b)) return Error::Domain;
  out = Prim<T>::pow(a, b);
  return Error::None;
}

// Gamma for x that is not a pole; integers use an exact product.
template <typename T>
Error gammaChecked(T x, T& out) {
  typedef Math<T> M;
  if (M::isInteger(x)) {
    if (x <= T()) return Error::Domain;
    if (x > M::c(171)) return Error::Overflow;
    T r = M::c(1);
    for (int i = 2; static_cast<T>(i) < x; ++i) r *= static_cast<T>(i);
    out = r;
    return Error::None;
  }
  out = M::gamma(x);
  return Error::None;
}

// Evaluates one built-in function. `a` points to n >= minArgs arguments.
template <typename T>
Error callFunction(Fn fn, const T* a, size_t n, AngleUnit unit, T& out) {
  typedef Math<T> M;
  typedef Prim<T> P;
  const bool deg = unit == AngleUnit::Degrees;
  const T x = a[0];
  const T y = n > 1 ? a[1] : T();
  const T zero = M::c(0);
  const T one = M::c(1);
  T r = zero;

  switch (fn) {
    case Fn::Abs: r = M::absolute(x); break;
    case Fn::Sqrt:
      if (x < zero) return Error::Domain;
      r = P::sqrt(x);
      break;
    case Fn::Cbrt: r = M::cbrt(x); break;
    case Fn::Root:  // x^(1/y), real odd roots of negative numbers allowed
      if (y == zero) return Error::DivisionByZero;
      if (x < zero) {
        if (!M::isInteger(y) || P::fmod(y, M::c(2)) == zero) return Error::Domain;
        r = -P::pow(-x, one / y);
      } else if (x == zero) {
        if (y < zero) return Error::DivisionByZero;
        r = zero;
      } else {
        r = P::pow(x, one / y);
      }
      break;
    case Fn::Exp: r = P::exp(x); break;
    case Fn::Exp2: r = M::exp2(x); break;
    case Fn::Expm1: r = M::expm1(x); break;
    case Fn::Ln:
      if (x <= zero) return Error::Domain;
      r = P::log(x);
      break;
    case Fn::Log:  // log(x) = log10(x), log(x, base)
      if (x <= zero) return Error::Domain;
      if (n == 1) {
        r = P::log10(x);
      } else {
        if (y <= zero || y == one) return Error::Domain;
        r = P::log(x) / P::log(y);
      }
      break;
    case Fn::Log2:
      if (x <= zero) return Error::Domain;
      r = M::log2(x);
      break;
    case Fn::Log10:
      if (x <= zero) return Error::Domain;
      r = P::log10(x);
      break;
    case Fn::Pow: return powChecked(x, y, out);

    case Fn::Sin: r = P::sin(toRadians(x, deg)); break;
    case Fn::Cos: r = P::cos(toRadians(x, deg)); break;
    case Fn::Tan: r = P::tan(toRadians(x, deg)); break;
    case Fn::Asin:
      if (M::absolute(x) > one) return Error::Domain;
      r = fromRadians(P::asin(x), deg);
      break;
    case Fn::Acos:
      if (M::absolute(x) > one) return Error::Domain;
      r = fromRadians(P::acos(x), deg);
      break;
    case Fn::Atan: r = fromRadians(P::atan(x), deg); break;
    case Fn::Atan2:  // atan2(y, x) with the first argument as y
      if (x == zero && y == zero) return Error::Domain;
      r = fromRadians(P::atan2(x, y), deg);
      break;

    case Fn::Sinh: r = M::sinh(x); break;
    case Fn::Cosh: r = M::cosh(x); break;
    case Fn::Tanh: r = M::tanh(x); break;
    case Fn::Asinh: r = M::asinh(x); break;
    case Fn::Acosh:
      if (x < one) return Error::Domain;
      r = M::acosh(x);
      break;
    case Fn::Atanh:
      if (M::absolute(x) >= one) return Error::Domain;
      r = M::atanh(x);
      break;

    case Fn::Sec: {
      T c = P::cos(toRadians(x, deg));
      if (c == zero) return Error::DivisionByZero;
      r = one / c;
      break;
    }
    case Fn::Csc: {
      T s = P::sin(toRadians(x, deg));
      if (s == zero) return Error::DivisionByZero;
      r = one / s;
      break;
    }
    case Fn::Cot: {
      T s = P::sin(toRadians(x, deg));
      if (s == zero) return Error::DivisionByZero;
      r = P::cos(toRadians(x, deg)) / s;
      break;
    }
    case Fn::Asec:
      if (M::absolute(x) < one) return Error::Domain;
      r = fromRadians(P::acos(one / x), deg);
      break;
    case Fn::Acsc:
      if (M::absolute(x) < one) return Error::Domain;
      r = fromRadians(P::asin(one / x), deg);
      break;
    case Fn::Acot:
      r = x == zero ? fromRadians(M::pi() / M::c(2), deg)
                    : fromRadians(P::atan(one / x), deg);
      break;

    case Fn::Floor: r = P::floor(x); break;
    case Fn::Ceil: r = P::ceil(x); break;
    case Fn::Round: r = M::roundHalf(x); break;
    case Fn::Trunc: r = M::trunc(x); break;
    case Fn::Frac: r = M::frac(x); break;
    case Fn::Sign: r = M::sign(x); break;
    case Fn::Mod:  // floored modulo: result has the sign of y
      if (y == zero) return Error::DivisionByZero;
      r = x - y * P::floor(x / y);
      break;
    case Fn::Hypot: r = M::hypot(x, y); break;

    case Fn::Gamma: return gammaChecked(x, out);
    case Fn::Fact: return gammaChecked(x + one, out);  // x! = gamma(x + 1)
    case Fn::Erf: r = M::erf(x); break;
    case Fn::Erfc: r = M::erfc(x); break;

    case Fn::Comb:
    case Fn::Perm: {  // n choose k, n permute k
      if (!M::isNonNegativeInteger(x) || !M::isNonNegativeInteger(y) || y > x) {
        return Error::Domain;
      }
      if (x > M::c(1000000.0)) return Error::Overflow;
      int nn = static_cast<int>(x);
      int k = static_cast<int>(y);
      r = one;
      if (fn == Fn::Comb) {
        for (int i = 1; i <= k; ++i) {
          r = r * static_cast<T>(nn - k + i) / static_cast<T>(i);
        }
      } else {
        for (int i = 0; i < k; ++i) r *= static_cast<T>(nn - i);
      }
      break;
    }
    case Fn::Gcd:
    case Fn::Lcm: {
      if (!M::isInteger(x) || !M::isInteger(y)) return Error::Domain;
      if (M::absolute(x) > M::c(2147483647.0) || M::absolute(y) > M::c(2147483647.0)) {
        return Error::Domain;
      }
      uint32_t a1 = static_cast<uint32_t>(M::absolute(x));
      uint32_t b1 = static_cast<uint32_t>(M::absolute(y));
      uint32_t p = a1, q = b1;
      while (q != 0) {  // Euclid
        uint32_t t = p % q;
        p = q;
        q = t;
      }
      if (fn == Fn::Gcd) {
        r = static_cast<T>(p);
      } else {
        r = p == 0 ? zero : (M::absolute(x) / static_cast<T>(p)) * M::absolute(y);
      }
      break;
    }

    case Fn::Min:
    case Fn::Max:
    case Fn::Sum:
    case Fn::Avg: {
      T acc = a[0];
      for (size_t i = 1; i < n; ++i) {
        if (fn == Fn::Min) {
          if (a[i] < acc) acc = a[i];
        } else if (fn == Fn::Max) {
          if (a[i] > acc) acc = a[i];
        } else {
          acc += a[i];
        }
      }
      r = fn == Fn::Avg ? acc / M::c(static_cast<double>(n)) : acc;
      break;
    }
    case Fn::Clamp: {
      T hi = a[2];
      if (y > hi) return Error::Domain;
      r = x < y ? y : (x > hi ? hi : x);
      break;
    }
    case Fn::Deg: r = x * (M::c(180) / M::pi()); break;
    case Fn::Rad: r = x * (M::pi() / M::c(180)); break;
    default:
      return Error::UnknownFunction;
  }
  out = r;
  return Error::None;
}

template <typename T>
Error applyBinary(OpCode code, T a, T b, T& out) {
  switch (code) {
    case OpCode::Add: out = a + b; return Error::None;
    case OpCode::Sub: out = a - b; return Error::None;
    case OpCode::Mul: out = a * b; return Error::None;
    case OpCode::Div:
      if (b == T()) return Error::DivisionByZero;
      out = a / b;
      return Error::None;
    case OpCode::Mod:  // C-style remainder (sign of dividend)
      if (b == T()) return Error::DivisionByZero;
      out = Prim<T>::fmod(a, b);
      return Error::None;
    case OpCode::Pow: return powChecked(a, b, out);
    default: return Error::Syntax;
  }
}

// What a running program can see: the variables, the bodies of the user
// functions, the angle unit and how deeply user functions are nested.
template <typename T, size_t N>
struct Environment {
  const T* vars;
  size_t varCount;
  const Expression<T, N>* functions;  // bodies of the user functions
  size_t functionCount;
  AngleUnit unit;
  size_t depth;
};

// Runs a compiled program. `frame` holds the parameters of the running user
// function (none at the top level).
template <typename T, size_t N>
Error evaluateProgram(const Expression<T, N>& ex, const Environment<T, N>& env,
                      const T* frame, size_t frameCount, T& result) {
  typedef Math<T> M;
  T stack[N];
  size_t sp = 0;
  for (size_t i = 0; i < ex.size(); ++i) {
    const Op<T>& op = ex[i];
    Error err = Error::None;
    switch (op.code) {
      case OpCode::Push:
        stack[sp++] = op.value;
        break;
      case OpCode::Load:
        if (op.index & kFrameBit) {  // a parameter of a user function
          const size_t k = op.index & ~kFrameBit;
          if (k >= frameCount) return Error::UnknownIdentifier;
          stack[sp++] = frame[k];
        } else {
          if (op.index >= env.varCount) return Error::UnknownIdentifier;
          stack[sp++] = env.vars[op.index];
        }
        break;
      case OpCode::Neg:
        stack[sp - 1] = -stack[sp - 1];
        break;
      case OpCode::Call: {
        sp -= op.argc;
        T r = T();
        err = callFunction(static_cast<Fn>(op.index), stack + sp, op.argc, env.unit, r);
        stack[sp++] = r;
        break;
      }
      case OpCode::UserCall: {
        if (op.index >= env.functionCount) return Error::UnknownFunction;
        if (env.depth >= TINYCALC_MAX_FUNCTIONS) return Error::NestingTooDeep;
        sp -= op.argc;
        Environment<T, N> inner = env;
        inner.depth = env.depth + 1;
        T r = T();
        err = evaluateProgram(env.functions[op.index], inner, stack + sp, op.argc, r);
        stack[sp++] = r;
        break;
      }
      default: {  // binary operators
        --sp;
        T r = T();
        err = applyBinary(op.code, stack[sp - 1], stack[sp], r);
        stack[sp - 1] = r;
        break;
      }
    }
    if (err != Error::None) return err;
    if (!M::isFinite(stack[sp - 1])) {
      return M::isNaN(stack[sp - 1]) ? Error::Domain : Error::Overflow;
    }
  }
  if (sp != 1) return Error::Syntax;
  result = stack[0];
  return Error::None;
}

}  // namespace detail
}  // namespace TinyCalc
