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
 * angle unit and a scratch program. Use evaluate() for a single formula or an
 * assignment, and compile() + run() to evaluate one formula repeatedly with
 * different variable values.
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
  static_assert(MaxVars > 0, "MaxVars must be > 0");

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

  Calculator() : count_(0), angle_(AngleUnit::Radians) {
    addSlot("ans", T());
  }

  // ---- settings -------------------------------------------------------
  void setAngleUnit(AngleUnit unit) { angle_ = unit; }
  AngleUnit angleUnit() const { return angle_; }

  // ---- variables ------------------------------------------------------
  Error setVariable(const char* name, T value) {
    if (!detail::isIdentifier(name)) return Error::InvalidName;
    T dummy = T();
    if (lookupConstant<T>(name, dummy) || findFunction(name) != nullptr) {
      return Error::ReservedName;
    }
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
    r.error = detail::evaluateProgram<T, MaxOps>(program, values_, count_, angle_, r.value);
    return r;
  }

  // Evaluates "expr" or "name = expr". Sets "ans" on success.
  Result evaluate(const char* text) {
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
  };

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
};

}  // namespace TinyCalc
