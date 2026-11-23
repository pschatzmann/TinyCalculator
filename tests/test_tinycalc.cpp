// Desktop unit tests for TinyCalculator. No external test framework needed.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "TinyCalculator.h"

using TinyCalc::AngleUnit;
using TinyCalc::Calculator;
using TinyCalc::Error;
using TinyCalc::errorText;

static const double kPi = 3.14159265358979323846;
static const double kE = 2.71828182845904523536;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    ++g_checks;                                                              \
    if (!(cond)) {                                                           \
      ++g_failures;                                                          \
      std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond  \
                << "\n";                                                     \
    }                                                                        \
  } while (0)

// relative tolerance depending on the number type
template <typename T>
double tolerance() {
  return sizeof(T) <= 4 ? 2e-4 : 1e-9;
}

template <typename T>
bool nearly(double actual, double expected) {
  double tol = tolerance<T>() * (1.0 + std::fabs(expected));
  return std::fabs(actual - expected) <= tol;
}

// Evaluates expr and checks the value.
template <typename T>
void expectValue(Calculator<T>& calc, const char* expr, double expected) {
  ++g_checks;
  auto r = calc.evaluate(expr);
  if (!r.ok()) {
    ++g_failures;
    std::cerr << "expected value for '" << expr << "' but got error '"
              << errorText(r.error) << "' at " << r.errorPos << "\n";
    return;
  }
  if (!nearly<T>(static_cast<double>(r.value), expected)) {
    ++g_failures;
    std::cerr << "'" << expr << "' = " << static_cast<double>(r.value)
              << ", expected " << expected << "\n";
  }
}

// Evaluates expr and checks the error code.
template <typename T>
void expectError(Calculator<T>& calc, const char* expr, Error expected) {
  ++g_checks;
  auto r = calc.evaluate(expr);
  if (r.ok() || r.error != expected) {
    ++g_failures;
    std::cerr << "'" << expr << "' expected error '" << errorText(expected)
              << "' but got " << (r.ok() ? std::string("a value") : errorText(r.error))
              << "\n";
  }
}

template <typename T>
void testBasicArithmetic() {
  Calculator<T> calc;
  expectValue(calc, "1+2*3", 7);
  expectValue(calc, "(1+2)*3", 9);
  expectValue(calc, "7/2", 3.5);
  expectValue(calc, "10 - 4 - 3", 3);            // left associative
  expectValue(calc, "2^3^2", 512);              // right associative
  expectValue(calc, "-2^2", -4);                // unary minus binds looser than ^
  expectValue(calc, "2^-1", 0.5);
  expectValue(calc, "2**10", 1024);             // ** alias
  expectValue(calc, "--3", 3);
  expectValue(calc, "+4", 4);
  expectValue(calc, "10 % 4", 2);
  expectValue(calc, "-7 % 3", -1);              // C-style remainder
  expectValue(calc, "mod(-7, 3)", 2);           // floored modulo
  expectValue(calc, "2^0.5", std::sqrt(2.0));
  expectValue(calc, "  1 +\t2  ", 3);           // whitespace
}

template <typename T>
void testNumberLiterals() {
  Calculator<T> calc;
  expectValue(calc, "2e3", 2000);
  expectValue(calc, "2.5e-1", 0.25);
  expectValue(calc, ".5", 0.5);
  expectValue(calc, "1.", 1);
  expectValue(calc, "2E+2", 200);
}

template <typename T>
void testImplicitMultiplicationAndFactorial() {
  Calculator<T> calc;
  expectValue(calc, "2(3+4)", 14);
  expectValue(calc, "(1+1)(2+2)", 8);
  expectValue(calc, "2pi", 2 * kPi);
  expectValue(calc, "2sin(pi/2)", 2);
  expectValue(calc, "2e", 2 * kE);            // 'e' not followed by digits
  expectValue(calc, "2^3pi", 8 * kPi);  // implicit * binds looser than ^
  expectValue(calc, "5!", 120);
  expectValue(calc, "0!", 1);
  expectValue(calc, "(3+2)!", 120);
  expectValue(calc, "-3!", -6);                // factorial binds tighter than unary minus
  expectValue(calc, "fact(4)", 24);
  expectValue(calc, "gamma(5)", 24);
  expectValue(calc, "gamma(0.5)", std::sqrt(kPi));
  expectValue(calc, "fact(-0.5)", std::sqrt(kPi));  // x! = gamma(x+1)
}

template <typename T>
void testConstants() {
  Calculator<T> calc;
  expectValue(calc, "pi", kPi);
  expectValue(calc, "e", kE);
  expectValue(calc, "tau", 2 * kPi);
  expectValue(calc, "phi", (1 + std::sqrt(5.0)) / 2);
}

template <typename T>
void testRootsAndExponentials() {
  Calculator<T> calc;
  expectValue(calc, "sqrt(16)", 4);
  expectValue(calc, "sqrt(2)", std::sqrt(2.0));
  expectValue(calc, "cbrt(27)", 3);
  expectValue(calc, "cbrt(-8)", -2);
  expectValue(calc, "root(27,3)", 3);
  expectValue(calc, "root(-8,3)", -2);
  expectValue(calc, "root(16,4)", 2);
  expectValue(calc, "exp(0)", 1);
  expectValue(calc, "exp(1)", kE);
  expectValue(calc, "exp2(3)", 8);
  expectValue(calc, "expm1(0)", 0);
  expectValue(calc, "expm1(1e-6)", 1e-6);
  expectValue(calc, "ln(e)", 1);
  expectValue(calc, "ln(10)", std::log(10.0));
  expectValue(calc, "log(1000)", 3);            // log = log10 with one argument
  expectValue(calc, "log(8, 2)", 3);            // log base 2 of 8
  expectValue(calc, "log2(8)", 3);
  expectValue(calc, "log10(0.01)", -2);
  expectValue(calc, "pow(2,10)", 1024);
  expectValue(calc, "hypot(3,4)", 5);
  expectValue(calc, "abs(-3.5)", 3.5);
}

template <typename T>
void testTrigonometryRadians() {
  Calculator<T> calc;
  calc.setAngleUnit(AngleUnit::Radians);
  expectValue(calc, "sin(0)", 0);
  expectValue(calc, "sin(pi/2)", 1);
  expectValue(calc, "cos(0)", 1);
  expectValue(calc, "cos(pi)", -1);
  expectValue(calc, "tan(pi/4)", 1);
  expectValue(calc, "asin(1)", kPi / 2);
  expectValue(calc, "acos(0)", kPi / 2);
  expectValue(calc, "atan(1)", kPi / 4);
  expectValue(calc, "atan2(1, 1)", kPi / 4);
  expectValue(calc, "atan2(-1, -1)", -3 * kPi / 4);
  expectValue(calc, "sec(0)", 1);
  expectValue(calc, "csc(pi/2)", 1);
  expectValue(calc, "cot(pi/4)", 1);
  expectValue(calc, "asec(1)", 0);
  expectValue(calc, "acsc(1)", kPi / 2);
  expectValue(calc, "acot(1)", kPi / 4);
  expectValue(calc, "acot(0)", kPi / 2);
}

template <typename T>
void testTrigonometryDegrees() {
  Calculator<T> calc;
  calc.setAngleUnit(AngleUnit::Degrees);
  expectValue(calc, "sin(90)", 1);
  expectValue(calc, "cos(180)", -1);
  expectValue(calc, "sin(30)", 0.5);
  expectValue(calc, "asin(1)", 90);
  expectValue(calc, "acos(-1)", 180);
  expectValue(calc, "atan(1)", 45);
  expectValue(calc, "atan2(1,1)", 45);
  expectValue(calc, "acot(1)", 45);
  // conversions are independent of the angle mode
  expectValue(calc, "deg(pi)", 180);
  expectValue(calc, "rad(180)", kPi);
  calc.setAngleUnit(AngleUnit::Radians);
  expectValue(calc, "sin(pi/6)", 0.5);
}

template <typename T>
void testHyperbolic() {
  Calculator<T> calc;
  expectValue(calc, "sinh(0)", 0);
  expectValue(calc, "sinh(1)", std::sinh(1.0));
  expectValue(calc, "cosh(0)", 1);
  expectValue(calc, "cosh(1)", std::cosh(1.0));
  expectValue(calc, "tanh(0)", 0);
  expectValue(calc, "tanh(1)", std::tanh(1.0));
  expectValue(calc, "asinh(1)", std::asinh(1.0));
  expectValue(calc, "acosh(2)", std::acosh(2.0));
  expectValue(calc, "atanh(0.5)", std::atanh(0.5));
}

template <typename T>
void testRoundingAndSign() {
  Calculator<T> calc;
  expectValue(calc, "floor(-2.5)", -3);
  expectValue(calc, "ceil(-2.5)", -2);
  expectValue(calc, "round(2.5)", 3);      // half away from zero
  expectValue(calc, "round(-2.5)", -3);
  expectValue(calc, "round(2.4)", 2);
  expectValue(calc, "trunc(-2.7)", -2);
  expectValue(calc, "frac(2.75)", 0.75);
  expectValue(calc, "sign(-3)", -1);
  expectValue(calc, "sign(0)", 0);
  expectValue(calc, "sign(4)", 1);
}

template <typename T>
void testSpecialFunctions() {
  Calculator<T> calc;
  expectValue(calc, "erf(0)", 0);
  expectValue(calc, "erf(1)", 0.8427007929497149);
  expectValue(calc, "erf(-1)", -0.8427007929497149);
  expectValue(calc, "erf(10)", 1);
  expectValue(calc, "erfc(0)", 1);
  expectValue(calc, "comb(5,2)", 10);
  expectValue(calc, "comb(10,0)", 1);
  expectValue(calc, "perm(5,2)", 20);
  expectValue(calc, "gcd(12,18)", 6);
  expectValue(calc, "gcd(0,7)", 7);
  expectValue(calc, "lcm(4,6)", 12);
  expectValue(calc, "lcm(0,5)", 0);
}

template <typename T>
void testVariadicFunctions() {
  Calculator<T> calc;
  expectValue(calc, "min(3,1,2)", 1);
  expectValue(calc, "max(3,1,2)", 3);
  expectValue(calc, "sum(1,2,3)", 6);
  expectValue(calc, "avg(1,2,3)", 2);
  expectValue(calc, "max(-4)", -4);
  expectValue(calc, "clamp(5,0,3)", 3);
  expectValue(calc, "clamp(-5,0,3)", 0);
  expectValue(calc, "clamp(2,0,3)", 2);
  expectValue(calc, "2*max(1,4)+1", 9);
}

template <typename T>
void testVariablesAndAssignment() {
  Calculator<T> calc;
  CHECK(calc.setVariable("x", T(4)) == Error::None);
  expectValue(calc, "x^2", 16);
  expectValue(calc, "x*x - 2x", 8);

  auto r = calc.evaluate("y = x * 3");
  CHECK(r.ok());
  T y = T();
  CHECK(calc.getVariable("y", y));
  CHECK(nearly<T>(static_cast<double>(y), 12));

  expectValue(calc, "y / 4", 3);
  expectValue(calc, "ans", 3);                 // last result is stored in ans
  expectValue(calc, "ans * 2", 6);

  // assignment through evaluate() also updates an existing variable
  calc.evaluate("x = 10");
  expectValue(calc, "x + 1", 11);

  // comparison "==" is not an assignment
  expectError(calc, "x == 10", Error::Syntax);  // '=' is not an operator
}

template <typename T>
void testCompileOnceRunMany() {
  Calculator<T> calc;
  calc.setVariable("x", T(0));  // variables must exist before compiling
  typename Calculator<T>::Program prog;
  CHECK(calc.compile("x*x + 1", prog) == Error::None);
  CHECK(prog.size() > 0);
  CHECK(prog.maxStack() >= 2);

  for (int i = 0; i < 5; ++i) {
    calc.setVariable("x", T(i));
    auto r = calc.run(prog);
    CHECK(r.ok());
    CHECK(nearly<T>(static_cast<double>(r.value), i * i + 1.0));
  }
}

template <typename T>
void testErrors() {
  Calculator<T> calc;
  expectError(calc, "", Error::Empty);
  expectError(calc, "   ", Error::Empty);
  expectError(calc, "1/0", Error::DivisionByZero);
  expectError(calc, "5 % 0", Error::DivisionByZero);
  expectError(calc, "0^-1", Error::DivisionByZero);
  expectError(calc, "mod(5,0)", Error::DivisionByZero);
  expectError(calc, "tan(pi/2)*0+1/sin(0)", Error::DivisionByZero);
  expectError(calc, "sqrt(-1)", Error::Domain);
  expectError(calc, "ln(0)", Error::Domain);
  expectError(calc, "log(-2)", Error::Domain);
  expectError(calc, "asin(2)", Error::Domain);
  expectError(calc, "acos(-1.5)", Error::Domain);
  expectError(calc, "acosh(0.5)", Error::Domain);
  expectError(calc, "atanh(1)", Error::Domain);
  expectError(calc, "(-2)^0.5", Error::Domain);
  expectError(calc, "(-2)!", Error::Domain);
  expectError(calc, "gamma(0)", Error::Domain);
  expectError(calc, "comb(3,5)", Error::Domain);
  expectError(calc, "gcd(1.5,2)", Error::Domain);
  expectError(calc, "1e400", Error::Overflow);
  expectError(calc, "10^400", Error::Overflow);
  expectError(calc, "200!", Error::Overflow);
  expectError(calc, "2 +", Error::UnexpectedEnd);
  expectError(calc, "(2+3", Error::UnbalancedParenthesis);
  expectError(calc, "2+3)", Error::UnbalancedParenthesis);
  expectError(calc, "3 $ 4", Error::Syntax);
  expectError(calc, "foo(2)", Error::UnknownFunction);
  expectError(calc, "y+1", Error::UnknownIdentifier);
  expectError(calc, "sin()", Error::WrongArgumentCount);
  expectError(calc, "sqrt(1,2)", Error::WrongArgumentCount);
  expectError(calc, "clamp(1,2)", Error::WrongArgumentCount);
  expectError(calc, "1,2", Error::UnexpectedToken);
  expectError(calc, "pi = 3", Error::ReservedName);
  expectError(calc, "sin = 3", Error::ReservedName);
  expectError(calc, "clamp(5,4,1)", Error::Domain);
}

template <typename T>
void testErrorPosition() {
  Calculator<T> calc;
  auto r = calc.evaluate("1 + 2 $");
  CHECK(!r.ok());
  CHECK(r.error == Error::Syntax);
  CHECK(r.errorPos == 6);

  r = calc.evaluate("z = 1 + q");
  CHECK(!r.ok());
  CHECK(r.error == Error::UnknownIdentifier);
  CHECK(r.errorPos == 8);
}

template <typename T>
void testBoolConversion() {
  Calculator<T> calc;
  auto good = calc.evaluate("1+1");
  auto bad = calc.evaluate("1/0");
  CHECK(static_cast<bool>(good));
  CHECK(!static_cast<bool>(bad));
}

template <typename T>
void testLimits() {
  // nesting depth is guarded
  Calculator<T> calc;
  std::string deep(TINYCALC_MAX_NESTING + 5, '(');
  deep += "1";
  deep += std::string(TINYCALC_MAX_NESTING + 5, ')');
  expectError(calc, deep.c_str(), Error::NestingTooDeep);

  // too many instructions
  std::string big;
  for (int i = 0; i < TINYCALC_MAX_OPS; ++i) big += "1+";
  big += "1";
  expectError(calc, big.c_str(), Error::TooComplex);

  // variable table limits and name validation
  Calculator<T> vars;
  size_t free = TINYCALC_MAX_VARIABLES - vars.variableCount();
  for (size_t i = 0; i < free; ++i) {
    char name[8];
    std::snprintf(name, sizeof(name), "v%u", static_cast<unsigned>(i));
    CHECK(vars.setVariable(name, T(1)) == Error::None);
  }
  CHECK(vars.setVariable("overflow", T(1)) == Error::TooManyVariables);
  CHECK(vars.setVariable("1abc", T(1)) == Error::InvalidName);
  CHECK(vars.setVariable("a b", T(1)) == Error::InvalidName);
  CHECK(vars.setVariable("cos", T(1)) == Error::ReservedName);
}

template <typename T>
void testClearAndAns() {
  Calculator<T> calc;
  calc.setVariable("k", T(5));
  calc.clearVariables();
  CHECK(calc.variableCount() == 1);
  expectError(calc, "k", Error::UnknownIdentifier);
  expectValue(calc, "2+2", 4);
  expectValue(calc, "ans*3", 12);
}

template <typename T>
void testUnits() {
  Calculator<T> calc;
  expectValue(calc, "5 km", 5000);
  expectValue(calc, "1 h to min", 60);
  expectValue(calc, "2 min to s", 120);
  expectValue(calc, "1 ft to in", 12);
  expectValue(calc, "1 kg to g", 1000);
  expectValue(calc, "5 km to mi", 5000.0 / 1609.344);
  expectValue(calc, "5 km + 300 m to m", 5300);
  expectValue(calc, "2 * 3 km to m", 6000);
  expectValue(calc, "1 day to h", 24);
  expectValue(calc, "1 lb to kg", 0.45359237);
  expectError(calc, "5 km to foo", Error::UnknownIdentifier);
  expectError(calc, "5 to", Error::UnexpectedEnd);
  expectError(calc, "to m", Error::UnknownIdentifier);
  // units are reserved names
  CHECK(calc.setVariable("km", T(1)) == Error::ReservedName);
  CHECK(calc.setVariable("to", T(1)) == Error::ReservedName);
}

template <typename T>
void testTemperatures() {
  Calculator<T> calc;
  expectValue(calc, "100 degC to degF", 212);
  expectValue(calc, "32 degF to degC", 0);
  expectValue(calc, "0 degC to K", 273.15);
  expectValue(calc, "273.15 K to degC", 0);
  expectValue(calc, "(10 + 15) degC to degF", 77);
  expectValue(calc, "-40 degC to degF", -40);
  expectValue(calc, "100 degC to degC", 100);
  expectError(calc, "25 to degF", Error::Syntax);          // the source needs a unit
  expectError(calc, "25 degC to m", Error::UnknownIdentifier);
  CHECK(calc.setVariable("degC", T(1)) == Error::ReservedName);
  CHECK(calc.setVariable("K", T(1)) == Error::ReservedName);
}

template <typename T>
void testUserFunctions() {
  Calculator<T> calc;
  CHECK(calc.evaluate("f(x) = x^2 + 1").ok());
  CHECK(calc.hasFunction("f"));
  CHECK(calc.functionCount() == 1);
  expectValue(calc, "f(3)", 10);
  expectValue(calc, "f(2) + f(1)", 7);
  expectValue(calc, "f(f(1))", 5);                 // 1 -> 2 -> 5

  // several parameters, and a function that calls an earlier one
  CHECK(calc.evaluate("mul(a, b) = a*b - 1").ok());
  expectValue(calc, "mul(3, 4)", 11);
  CHECK(calc.evaluate("twice(x) = f(x) * 2").ok());
  expectValue(calc, "twice(2)", 10);

  // parameters shadow variables with the same name
  calc.setVariable("x", T(100));
  expectValue(calc, "f(1)", 2);
  expectValue(calc, "x", 100);

  // redefinition replaces the body
  CHECK(calc.evaluate("f(x) = x").ok());
  expectValue(calc, "f(3)", 3);
  CHECK(calc.functionCount() == 3);

  // a function with a unit conversion in its body
  CHECK(calc.evaluate("km2m(v) = v * 1 km").ok());
  expectValue(calc, "km2m(2)", 2000);

  // errors
  expectError(calc, "f(1, 2)", Error::WrongArgumentCount);
  expectError(calc, "zz(1)", Error::UnknownFunction);
  expectError(calc, "sin(x) = x", Error::ReservedName);
  expectError(calc, "km(x) = x", Error::ReservedName);
  expectError(calc, "r(x) = r(x)", Error::UnknownFunction);   // no recursion
  expectError(calc, "q(x) = ", Error::Empty);
  expectError(calc, "q(x, x) = x", Error::InvalidName);       // duplicate parameter
  expectError(calc, "q(x) == 1", Error::Syntax);

  // the function table is limited
  calc.clearFunctions();
  CHECK(calc.functionCount() == 0);
  for (int i = 0; i < TINYCALC_MAX_FUNCTIONS; ++i) {
    char def[16];
    std::snprintf(def, sizeof(def), "u%d(x) = x", i);
    CHECK(calc.evaluate(def).ok());
  }
  expectError(calc, "extra(x) = x", Error::TooManyFunctions);
  CHECK(calc.evaluate("u0(x) = x + 1").ok());   // redefining is still allowed
  calc.clearFunctions();
  CHECK(!calc.hasFunction("f"));
  expectError(calc, "f(1)", Error::UnknownFunction);
  CHECK(!Calculator<T>::isDefinition("a == b"));
  CHECK(Calculator<T>::isDefinition("f(x) = x"));
  CHECK(!Calculator<T>::isDefinition("y = x"));
}

// compile once: a program that calls a user function, run with new variables
template <typename T>
void testUserFunctionCompiled() {
  Calculator<T> calc;
  calc.setVariable("x", T(0));
  CHECK(calc.evaluate("f(y) = y * y").ok());
  typename Calculator<T>::Program prog;
  CHECK(calc.compile("f(x) + 1", prog) == Error::None);
  for (int i = 1; i <= 3; ++i) {
    calc.setVariable("x", T(i));
    auto r = calc.run(prog);
    CHECK(r.ok());
    CHECK(nearly<T>(static_cast<double>(r.value), i * i + 1.0));
  }
}

template <typename T>
void testResultCasts() {
  Calculator<T> calc;
  auto r = calc.evaluate("1/4");
  CHECK(r.ok());
  CHECK(static_cast<double>(r) == 0.25);
  CHECK(static_cast<float>(r) == 0.25f);
  CHECK(static_cast<T>(r) == T(0.25));
  CHECK((double)r == 0.25);
  CHECK(float(r) == 0.25f);
  CHECK(r.template as<double>() == 0.25);
  CHECK(r.template as<int>() == 0);
  CHECK(static_cast<bool>(r));  // bool conversion still works
  CHECK(r.template as<int>() == 0 && !(calc.evaluate("1/0")));

  auto big = calc.evaluate("7*6");
  CHECK(static_cast<int>(big) == 42);
  CHECK(big.template as<long>() == 42);
}

template <typename T>
void runAll(const char* label) {
  std::cout << "running suite for " << label << "\n";
  testBasicArithmetic<T>();
  testNumberLiterals<T>();
  testImplicitMultiplicationAndFactorial<T>();
  testConstants<T>();
  testRootsAndExponentials<T>();
  testTrigonometryRadians<T>();
  testTrigonometryDegrees<T>();
  testHyperbolic<T>();
  testRoundingAndSign<T>();
  testSpecialFunctions<T>();
  testVariadicFunctions<T>();
  testVariablesAndAssignment<T>();
  testCompileOnceRunMany<T>();
  testErrors<T>();
  testErrorPosition<T>();
  testBoolConversion<T>();
  testResultCasts<T>();
  testUnits<T>();
  testTemperatures<T>();
  testUserFunctions<T>();
  testUserFunctionCompiled<T>();
  testLimits<T>();
  testClearAndAns<T>();
}

int main() {
  runAll<float>("float (default)");
  runAll<double>("double");

  std::cout << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
