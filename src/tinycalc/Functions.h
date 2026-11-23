#pragma once

// Built-in functions, constants, units and keywords.
//
// The tables are kept in flash (PROGMEM) on AVR boards. The lookups copy the
// entry they find into RAM, so callers never touch flash directly.

#include <string.h>
#include "Config.h"
#include "Math.h"
#include "Progmem.h"
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
 * the range of allowed argument counts. The name is at most 7 characters.
 */
struct FunctionInfo {
  char name[8];
  Fn id;
  uint8_t minArgs;
  uint8_t maxArgs;
};

// The table of built-in functions (in flash on AVR).
inline const FunctionInfo* functionTable(size_t& count) {
  static const FunctionInfo table[] TINYCALC_PROGMEM = {
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

// Copies entry i of the function table into out. False if i is out of range.
inline bool functionAt(size_t i, FunctionInfo& out) {
  size_t count = 0;
  const FunctionInfo* table = functionTable(count);
  if (i >= count) return false;
  TINYCALC_MEMCPY_P(&out, &table[i], sizeof(FunctionInfo));
  return true;
}

// Looks up a built-in function by name; copies its entry into out.
inline bool findFunction(const char* name, FunctionInfo& out) {
  size_t count = 0;
  functionTable(count);
  for (size_t i = 0; i < count; ++i) {
    functionAt(i, out);
    if (strcmp(out.name, name) == 0) return true;
  }
  return false;
}

// true if name is a built-in function
inline bool isFunctionName(const char* name) {
  FunctionInfo info;
  return findFunction(name, info);
}

// Units: the value of one unit in base units (metre, kilogram, second).
// "5 km" is 5000, and "x to mi" divides by the factor of mi.
struct UnitInfo {
  char name[4];
  double factor;
};

// The table of units (in flash on AVR).
inline const UnitInfo* unitTable(size_t& count) {
  static const UnitInfo table[] TINYCALC_PROGMEM = {
      {"m", 1.0},          {"cm", 0.01},         {"mm", 0.001},     {"km", 1000.0},
      {"mi", 1609.344},    {"ft", 0.3048},       {"in", 0.0254},    {"yd", 0.9144},
      {"kg", 1.0},         {"g", 0.001},         {"lb", 0.45359237}, {"oz", 0.028349523125},
      {"s", 1.0},          {"min", 60.0},        {"h", 3600.0},     {"day", 86400.0},
  };
  count = sizeof(table) / sizeof(table[0]);
  return table;
}

// Looks up a unit by name; returns its factor, or false.
inline bool findUnitFactor(const char* name, double& factor) {
  size_t count = 0;
  const UnitInfo* table = unitTable(count);
  for (size_t i = 0; i < count; ++i) {
    UnitInfo unit;
    TINYCALC_MEMCPY_P(&unit, &table[i], sizeof(UnitInfo));
    if (strcmp(unit.name, name) == 0) {
      factor = unit.factor;
      return true;
    }
  }
  return false;
}

// Temperature units: they are not linear (0 degC is not 0 K), so they are
// converted with "value unit to unit" rather than with a factor.
// Index: 0 degC, 1 degF, 2 K; -1 if the name is not a temperature unit.
static const char kDegC[] TINYCALC_PROGMEM = "degC";
static const char kDegF[] TINYCALC_PROGMEM = "degF";
static const char kKelvin[] TINYCALC_PROGMEM = "K";
static const char* const kTemperatureUnits[] TINYCALC_PROGMEM = {kDegC, kDegF, kKelvin};

inline int temperatureIndex(const char* name) {
  for (int i = 0; i < 3; ++i) {
    const char* unit = TINYCALC_READ_PTR(kTemperatureUnits[i]);
    if (TINYCALC_STRCMP_P(name, unit) == 0) return i;
  }
  return -1;
}

inline bool isTemperatureUnit(const char* name) { return temperatureIndex(name) >= 0; }

// The keyword of a unit conversion ("5 km to m").
static const char kToKeyword[] TINYCALC_PROGMEM = "to";

inline bool isKeyword(const char* name) { return TINYCALC_STRCMP_P(name, kToKeyword) == 0; }

// Looks up a built-in constant (pi, e, tau, phi) or unit (km, s, ...).
template <typename T>
inline bool lookupConstant(const char* name, T& value) {
  typedef Math<T> M;
  struct Constant {
    char name[4];
    uint8_t index;
  };
  static const Constant constants[] TINYCALC_PROGMEM = {
      {"pi", 0}, {"e", 1}, {"tau", 2}, {"phi", 3}};
  for (size_t i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i) {
    Constant c;
    TINYCALC_MEMCPY_P(&c, &constants[i], sizeof(Constant));
    if (strcmp(c.name, name) == 0) {
      switch (c.index) {
        case 0: value = M::pi(); break;
        case 1: value = M::e(); break;
        case 2: value = M::tau(); break;
        default: value = M::phi(); break;
      }
      return true;
    }
  }
  double factor = 0.0;
  if (findUnitFactor(name, factor)) {
    value = static_cast<T>(factor);
    return true;
  }
  return false;
}

}  // namespace TinyCalc
