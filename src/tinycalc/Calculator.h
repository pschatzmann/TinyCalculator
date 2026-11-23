#pragma once

#include <string.h>
#include "Compiler.h"
#include "Config.h"
#include "Evaluator.h"
#include "Expression.h"
#include "Functions.h"
#include "Types.h"

namespace TinyCalc {

/**
 * @class Calculator
 * @brief High-level calculator: variables, angle unit, compile and evaluate.
 *
 * Holds a table of named variables (plus the built-in variable `ans`), the
 * angle unit, a scratch program and the user-defined functions. Use evaluate()
 * for a single formula, an assignment ("y = x*3") or a function definition
 * ("f(x) = x^2 + 1"). Use compile() + run() to evaluate one formula repeatedly
 * with different variable values.
 *
 * Unit conversion: units are constants (km = 1000 m, min = 60 s, ...), so
 * "5 km" is 5000, and "expr to unit" divides by the unit: "5 km to mi".
 *
 * @code
 * TinyCalc::Calculator<float> calc;
 * calc.setVariable("x", 2.0f);
 * auto r = calc.evaluate("x^2 + sin(pi/2)");   // r.value == 5
 * if (!r) Serial.println(TinyCalc::errorText(r.error));
 * @endcode
 *
 * @tparam T        number type: float (default) or double
 * @tparam MaxOps   maximum number of instructions of a compiled expression
 * @tparam MaxVars  maximum number of variables, including `ans`
 */
template <typename T = float, size_t MaxOps = TINYCALC_MAX_OPS,
          size_t MaxVars = TINYCALC_MAX_VARIABLES>
class Calculator {
  static_assert(MaxVars > 0 && MaxVars < kFrameBit, "MaxVars must be in 1..127");
  static const size_t kMaxFunctions = TINYCALC_MAX_FUNCTIONS > 0 ? TINYCALC_MAX_FUNCTIONS : 1;
  static const size_t kMaxParams = TINYCALC_MAX_PARAMS > 0 ? TINYCALC_MAX_PARAMS : 1;
  static const size_t kMaxValueText = 96;  // longest value before a temperature unit

 public:
  typedef Expression<T, MaxOps> Program;

  /**
   * @brief Outcome of evaluate() or run(): the value, the error code and the
   * error position in the input text.
   */
  struct Result {
    T value;
    Error error;
    size_t errorPos;  // position in the input text if error != None
    bool ok() const { return error == Error::None; }
    explicit operator bool() const { return ok(); }
    /// Converts the value to another number type, e.g. float(r), (double)r
    /// or static_cast<double>(r). The result is not checked for errors.
    template <typename U>
    explicit operator U() const {
      return static_cast<U>(value);
    }
    /// Same as the cast operator: r.as<double>().
    template <typename U>
    U as() const {
      return static_cast<U>(value);
    }
  };

  Calculator() : count_(0), angle_(AngleUnit::Radians), fnCount_(0) {
    addSlot("ans", T());
  }

  // ---- settings -------------------------------------------------------
  void setAngleUnit(AngleUnit unit) { angle_ = unit; }
  AngleUnit angleUnit() const { return angle_; }

  // ---- variables ------------------------------------------------------
  Error setVariable(const char* name, T value) {
    if (!detail::isIdentifier(name)) return Error::InvalidName;
    if (isReserved(name)) return Error::ReservedName;
    int i = indexOf(name);
    if (i >= 0) {
      values_[i] = value;
      return Error::None;
    }
    if (count_ >= MaxVars) return Error::TooManyVariables;
    addSlot(name, value);
    return Error::None;
  }

  bool getVariable(const char* name, T& value) const {
    int i = indexOf(name);
    if (i < 0) return false;
    value = values_[i];
    return true;
  }

  // Removes all user variables (keeps "ans"). Programs compiled before
  // referencing removed variables will report UnknownIdentifier.
  void clearVariables() {
    count_ = 1;
    values_[0] = T();
  }

  size_t variableCount() const { return count_; }

  // ---- compile / evaluate --------------------------------------------
  // Compiles an expression once; the program can then be run many times
  // with different variable values via run().
  Error compile(const char* text, Program& program, size_t* errorPos = nullptr) const {
    Resolver resolver = {this};
    size_t pos = 0;
    detail::Compiler<T, MaxOps, Resolver> compiler(text, program, resolver);
    Error e = compiler.run(pos);
    if (errorPos != nullptr) *errorPos = pos;
    return e;
  }

  Result run(const Program& program) const {
    Result r = makeResult(T(), Error::None, 0);
    r.error = detail::evaluateProgram<T, MaxOps>(program, environment(), nullptr, 0, r.value);
    return r;
  }

  // ---- user-defined functions -------------------------------------------
  // Defines (or redefines) a function from text of the form "name(a, b) = body".
  // The body may use the parameters, variables, constants, built-in functions
  // and functions defined before it. A function cannot call itself.
  Error define(const char* text) {
    const char* p = text;
    skipSpaces(p);
    char name[TINYCALC_MAX_NAME + 1];
    if (!parseName(p, name)) return Error::InvalidName;
    skipSpaces(p);
    if (*p != '(') return Error::Syntax;
    ++p;

    char params[kMaxParams][TINYCALC_MAX_NAME + 1];
    size_t np = 0;
    skipSpaces(p);
    if (*p != ')') {
      for (;;) {
        skipSpaces(p);
        if (np >= kMaxParams || !parseName(p, params[np])) return Error::InvalidName;
        T dummy = T();
        if (lookupConstant<T>(params[np], dummy) || findFunction(params[np]) != nullptr) {
          return Error::ReservedName;
        }
        for (size_t k = 0; k < np; ++k) {
          if (strcmp(params[k], params[np]) == 0) return Error::InvalidName;  // duplicate
        }
        ++np;
        skipSpaces(p);
        if (*p != ',') break;
        ++p;
      }
    }
    if (*p != ')') return Error::Syntax;
    ++p;
    skipSpaces(p);
    if (*p != '=' || p[1] == '=') return Error::Syntax;
    const char* body = p + 1;

    if (isReserved(name)) return Error::ReservedName;
    int slot = userFunctionIndex(name);  // an existing function is redefined
    if (slot < 0) slot = static_cast<int>(fnCount_);  // else a new one, if there is room
    BodyResolver resolver = {this, params, np, slot};
    Program compiled;
    size_t pos = 0;
    detail::Compiler<T, MaxOps, BodyResolver> compiler(body, compiled, resolver);
    Error e = compiler.run(pos);
    if (e != Error::None) return e;
    if (static_cast<size_t>(slot) >= kMaxFunctions) return Error::TooManyFunctions;

    fnBodies_[slot] = compiled;
    strncpy(fnNames_[slot], name, TINYCALC_MAX_NAME);
    fnNames_[slot][TINYCALC_MAX_NAME] = '\0';
    fnParams_[slot] = static_cast<uint8_t>(np);
    if (static_cast<size_t>(slot) == fnCount_) ++fnCount_;
    return Error::None;
  }

  // True if the text is a function definition "name(...) = ...".
  static bool isDefinition(const char* text) {
    const char* p = text;
    skipSpaces(p);
    char name[TINYCALC_MAX_NAME + 1];
    if (!parseName(p, name)) return false;
    skipSpaces(p);
    if (*p != '(') return false;
    while (*p != '\0' && *p != ')') ++p;
    if (*p != ')') return false;
    ++p;
    skipSpaces(p);
    return *p == '=' && p[1] != '=';
  }

  size_t functionCount() const { return fnCount_; }
  bool hasFunction(const char* name) const { return userFunctionIndex(name) >= 0; }

  // Removes all user-defined functions. Programs compiled with them become invalid.
  void clearFunctions() { fnCount_ = 0; }

  // Evaluates "expr", "name = expr" or a function definition "f(x) = expr".
  // Sets "ans" on success. A definition has no value.
  Result evaluate(const char* text) {
    if (isDefinition(text)) return makeResult(T(), define(text), 0);
    bool converted = false;
    Result conv = temperatureConversion(text, converted);
    if (converted) {
      if (conv.ok()) values_[0] = conv.value;  // ans
      return conv;
    }
    char target[TINYCALC_MAX_NAME + 1];
    const char* expr = text;
    bool assign = splitAssignment(text, target, expr);

    Result r = evaluateExpression(expr);
    if (!r.ok()) {
      r.errorPos += static_cast<size_t>(expr - text);
      return r;
    }
    if (assign) {
      Error e = setVariable(target, r.value);
      if (e != Error::None) return makeResult(T(), e, 0);
    }
    values_[0] = r.value;  // ans
    return r;
  }

 private:
  /**
   * @brief Adapter that lets the Compiler look up variable slots of this calculator.
   */
  struct Resolver {
    const Calculator* calc;
    int find(const char* name) const { return calc->indexOf(name); }
    bool findUserFunction(const char* name, int& slot, int& params) const {
      return calc->findUser(name, slot, params, -1);
    }
  };

  // Resolver for the body of a function being defined: parameters are frame
  // slots, and the function itself (`self`) cannot be called.
  struct BodyResolver {
    const Calculator* calc;
    const char (*paramNames)[TINYCALC_MAX_NAME + 1];
    size_t paramCount;
    int self;
    int find(const char* name) const {
      for (size_t k = 0; k < paramCount; ++k) {
        if (strcmp(paramNames[k], name) == 0) return kFrameBit | static_cast<int>(k);
      }
      return calc->indexOf(name);
    }
    bool findUserFunction(const char* name, int& slot, int& params) const {
      return calc->findUser(name, slot, params, self);
    }
  };

  static void skipSpaces(const char*& p) {
    while (detail::isSpace(*p)) ++p;
  }

  // Reads an identifier at p (advancing p); false if there is none or it is too long.
  static bool parseName(const char*& p, char* out) {
    if (!detail::isIdentStart(*p)) return false;
    size_t len = 0;
    while (detail::isIdentPart(p[len])) ++len;
    if (len > TINYCALC_MAX_NAME) return false;
    memcpy(out, p, len);
    out[len] = '\0';
    p += len;
    return true;
  }

  // Constants, units, built-in functions and the keyword "to" cannot be redefined.
  static bool isReserved(const char* name) {
    T dummy = T();
    return lookupConstant<T>(name, dummy) || findFunction(name) != nullptr ||
           isTemperatureUnit(name) || strcmp(name, "to") == 0;
  }

  // Temperature scale: kelvin = value * scale + offset.
  static bool temperatureScale(const char* unit, T& scale, T& offset) {
    if (strcmp(unit, "K") == 0) {
      scale = T(1);
      offset = T(0);
    } else if (strcmp(unit, "degC") == 0) {
      scale = T(1);
      offset = T(273.15);
    } else if (strcmp(unit, "degF") == 0) {
      scale = T(0.5555555555555556);  // 5/9
      offset = T(255.3722222222222);  // 459.67 * 5/9
    } else {
      return false;
    }
    return true;
  }

  // Copies text[start, end) into out (NUL-terminated). False if it does not fit.
  static bool copyRange(const char* start, const char* end, char* out, size_t capacity) {
    const size_t len = static_cast<size_t>(end - start);
    if (len + 1 > capacity) return false;
    memcpy(out, start, len);
    out[len] = '\0';
    return true;
  }

  // Recognises "<value> <from> to <to>" with temperature units. Sets handled
  // to true if the text has this form and converts it.
  Result temperatureConversion(const char* text, bool& handled) {
    handled = false;
    const char* p = text + strlen(text);
    while (p > text && detail::isSpace(p[-1])) --p;
    const char* toEnd = p;
    while (p > text && detail::isIdentStart(p[-1])) --p;  // letters: "100degC" is 100 degC
    const char* toStart = p;
    char to[TINYCALC_MAX_NAME + 1];
    if (!copyRange(toStart, toEnd, to, sizeof(to)) || !isTemperatureUnit(to)) {
      return makeResult(T(), Error::None, 0);
    }
    // the keyword "to"
    while (p > text && detail::isSpace(p[-1])) --p;
    if (p - text < 2 || p[-2] != 't' || p[-1] != 'o') return makeResult(T(), Error::None, 0);
    const char* keyword = p - 2;
    if (keyword > text && detail::isIdentPart(keyword[-1])) return makeResult(T(), Error::None, 0);
    p = keyword;
    while (p > text && detail::isSpace(p[-1])) --p;
    const char* fromEnd = p;
    while (p > text && detail::isIdentStart(p[-1])) --p;
    char from[TINYCALC_MAX_NAME + 1];
    if (!copyRange(p, fromEnd, from, sizeof(from))) return makeResult(T(), Error::None, 0);
    handled = true;
    if (!isTemperatureUnit(from)) return makeResult(T(), Error::Syntax, 0);  // no source unit
    const char* valueEnd = p;
    while (valueEnd > text && detail::isSpace(valueEnd[-1])) --valueEnd;
    if (valueEnd == text) return makeResult(T(), Error::Syntax, 0);  // no value
    char value[kMaxValueText];
    if (!copyRange(text, valueEnd, value, sizeof(value))) {
      return makeResult(T(), Error::TooComplex, 0);
    }
    T s1 = T(), o1 = T(), s2 = T(), o2 = T();
    temperatureScale(from, s1, o1);
    temperatureScale(to, s2, o2);
    Result v = evaluateExpression(value);
    if (!v.ok()) return v;
    const T kelvin = v.value * s1 + o1;
    return makeResult((kelvin - o2) / s2, Error::None, 0);
  }

  int userFunctionIndex(const char* name) const {
    for (size_t i = 0; i < fnCount_; ++i) {
      if (strcmp(fnNames_[i], name) == 0) return static_cast<int>(i);
    }
    return -1;
  }

  // Finds user function `name` (except slot `exclude`).
  bool findUser(const char* name, int& slot, int& params, int exclude) const {
    int i = userFunctionIndex(name);
    if (i < 0 || i == exclude) return false;
    slot = i;
    params = fnParams_[i];
    return true;
  }

  detail::Environment<T, MaxOps> environment() const {
    detail::Environment<T, MaxOps> env;
    env.vars = values_;
    env.varCount = count_;
    env.functions = fnBodies_;
    env.functionCount = fnCount_;
    env.unit = angle_;
    env.depth = 0;
    return env;
  }

  static Result makeResult(T value, Error error, size_t pos) {
    Result r;
    r.value = value;
    r.error = error;
    r.errorPos = pos;
    return r;
  }

  Result evaluateExpression(const char* text) {
    size_t pos = 0;
    Error e = compile(text, scratch_, &pos);
    if (e != Error::None) return makeResult(T(), e, pos);
    return run(scratch_);
  }

  // Detects "identifier = rest" (but not "==").
  static bool splitAssignment(const char* text, char* target, const char*& rhs) {
    const char* p = text;
    while (detail::isSpace(*p)) ++p;
    if (!detail::isIdentStart(p[0])) return false;
    size_t len = 0;
    while (detail::isIdentPart(p[len])) ++len;
    if (len > TINYCALC_MAX_NAME) return false;
    memcpy(target, p, len);
    target[len] = '\0';
    const char* q = p + len;
    while (detail::isSpace(*q)) ++q;
    if (*q != '=' || q[1] == '=') return false;
    rhs = q + 1;
    return true;
  }

  void addSlot(const char* name, T value) {
    strncpy(names_[count_], name, TINYCALC_MAX_NAME);
    names_[count_][TINYCALC_MAX_NAME] = '\0';
    values_[count_] = value;
    ++count_;
  }

  int indexOf(const char* name) const {
    for (size_t i = 0; i < count_; ++i) {
      if (strcmp(names_[i], name) == 0) return static_cast<int>(i);
    }
    return -1;
  }

  char names_[MaxVars][TINYCALC_MAX_NAME + 1];
  T values_[MaxVars];
  size_t count_;
  AngleUnit angle_;
  Program scratch_;
  char fnNames_[kMaxFunctions][TINYCALC_MAX_NAME + 1];
  uint8_t fnParams_[kMaxFunctions];
  Program fnBodies_[kMaxFunctions];
  size_t fnCount_;
};

}  // namespace TinyCalc
