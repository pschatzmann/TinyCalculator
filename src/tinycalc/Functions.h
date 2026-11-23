#pragma once

#include <string.h>
#include "Config.h"
#include "Math.h"
#include "Types.h"

namespace TinyCalc {

// Identifiers of all built-in functions.
enum class Fn : uint8_t {
  Abs, Sqrt, Cbrt, Root, Exp, Exp2, Expm1, Ln, Log, Log2, Log10, Pow,
  Sin, Cos, Tan, Asin, Acos, Atan, Atan2,
  Sinh, Cosh, Tanh, Asinh, Acosh, Atanh,
  Sec, Csc, Cot, Asec, Acsc, Acot,
  Floor, Ceil, Round, Trunc, Frac, Sign, Mod, Hypot,
  Gamma, Fact, Erf, Erfc, Comb, Perm, Gcd, Lcm,
  Min, Max, Sum, Avg, Clamp, Deg, Rad
};

/**
 * @brief Table entry describing one built-in function: its name, its Fn id and
 * the range of allowed argument counts.
 */
struct FunctionInfo {
  const char* name;
  Fn id;
  uint8_t minArgs;
  uint8_t maxArgs;
};

inline const FunctionInfo* functionTable(size_t& count) {
  static const FunctionInfo table[] = {
      {"abs", Fn::Abs, 1, 1},       {"sqrt", Fn::Sqrt, 1, 1},
      {"cbrt", Fn::Cbrt, 1, 1},     {"root", Fn::Root, 2, 2},
      {"exp", Fn::Exp, 1, 1},       {"exp2", Fn::Exp2, 1, 1},
      {"expm1", Fn::Expm1, 1, 1},   {"ln", Fn::Ln, 1, 1},
      {"log", Fn::Log, 1, 2},       {"log2", Fn::Log2, 1, 1},
      {"log10", Fn::Log10, 1, 1},   {"pow", Fn::Pow, 2, 2},
      {"sin", Fn::Sin, 1, 1},       {"cos", Fn::Cos, 1, 1},
      {"tan", Fn::Tan, 1, 1},       {"asin", Fn::Asin, 1, 1},
      {"acos", Fn::Acos, 1, 1},     {"atan", Fn::Atan, 1, 1},
      {"atan2", Fn::Atan2, 2, 2},   {"sinh", Fn::Sinh, 1, 1},
      {"cosh", Fn::Cosh, 1, 1},     {"tanh", Fn::Tanh, 1, 1},
      {"asinh", Fn::Asinh, 1, 1},   {"acosh", Fn::Acosh, 1, 1},
      {"atanh", Fn::Atanh, 1, 1},   {"sec", Fn::Sec, 1, 1},
      {"csc", Fn::Csc, 1, 1},       {"cot", Fn::Cot, 1, 1},
      {"asec", Fn::Asec, 1, 1},     {"acsc", Fn::Acsc, 1, 1},
      {"acot", Fn::Acot, 1, 1},     {"floor", Fn::Floor, 1, 1},
      {"ceil", Fn::Ceil, 1, 1},     {"round", Fn::Round, 1, 1},
      {"trunc", Fn::Trunc, 1, 1},   {"frac", Fn::Frac, 1, 1},
      {"sign", Fn::Sign, 1, 1},     {"mod", Fn::Mod, 2, 2},
      {"hypot", Fn::Hypot, 2, 2},   {"gamma", Fn::Gamma, 1, 1},
      {"fact", Fn::Fact, 1, 1},     {"erf", Fn::Erf, 1, 1},
      {"erfc", Fn::Erfc, 1, 1},     {"comb", Fn::Comb, 2, 2},
      {"perm", Fn::Perm, 2, 2},     {"gcd", Fn::Gcd, 2, 2},
      {"lcm", Fn::Lcm, 2, 2},       {"min", Fn::Min, 1, 32},
      {"max", Fn::Max, 1, 32},      {"sum", Fn::Sum, 1, 32},
      {"avg", Fn::Avg, 1, 32},      {"clamp", Fn::Clamp, 3, 3},
      {"deg", Fn::Deg, 1, 1},       {"rad", Fn::Rad, 1, 1},
  };
  count = sizeof(table) / sizeof(table[0]);
  return table;
}

inline const FunctionInfo* findFunction(const char* name) {
  size_t count = 0;
  const FunctionInfo* table = functionTable(count);
  for (size_t i = 0; i < count; ++i) {
    if (strcmp(table[i].name, name) == 0) return &table[i];
  }
  return nullptr;
}

// Units: the value of one unit in base units (metre, kilogram, second).
// "5 km" is 5000, and "x to mi" divides by the factor of mi.
struct UnitInfo {
  const char* name;
  double factor;
};

inline const UnitInfo* unitTable(size_t& count) {
  static const UnitInfo table[] = {
      {"m", 1.0},          {"cm", 0.01},         {"mm", 0.001},     {"km", 1000.0},
      {"mi", 1609.344},    {"ft", 0.3048},       {"in", 0.0254},    {"yd", 0.9144},
      {"kg", 1.0},         {"g", 0.001},         {"lb", 0.45359237}, {"oz", 0.028349523125},
      {"s", 1.0},          {"min", 60.0},        {"h", 3600.0},     {"day", 86400.0},
  };
  count = sizeof(table) / sizeof(table[0]);
  return table;
}

// Temperature units: they are not linear (0 degC is not 0 K), so they are
// converted with "value unit to unit" rather than with a factor.
inline bool isTemperatureUnit(const char* name) {
  return strcmp(name, "degC") == 0 || strcmp(name, "degF") == 0 || strcmp(name, "K") == 0;
}

// Built-in constants (pi, e, tau, phi) and units (km, s, ...)
template <typename T>
inline bool lookupConstant(const char* name, T& value) {
  typedef Math<T> M;
  if (strcmp(name, "pi") == 0) value = M::pi();
  else if (strcmp(name, "e") == 0) value = M::e();
  else if (strcmp(name, "tau") == 0) value = M::tau();
  else if (strcmp(name, "phi") == 0) value = M::phi();
  else {
    size_t count = 0;
    const UnitInfo* units = unitTable(count);
    for (size_t i = 0; i < count; ++i) {
      if (strcmp(units[i].name, name) == 0) {
        value = static_cast<T>(units[i].factor);
        return true;
      }
    }
    return false;
  }
  return true;
}

}  // namespace TinyCalc
