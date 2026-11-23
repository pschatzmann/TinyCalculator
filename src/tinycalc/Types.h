#pragma once

#include <stddef.h>
#include <stdint.h>
#include "Config.h"
#include "Progmem.h"

#if defined(__AVR__)
class __FlashStringHelper;  // declared by the Arduino core
#endif

namespace TinyCalc {

enum class Error : uint8_t {
  None = 0,
  Empty,
  Syntax,
  UnexpectedToken,
  UnexpectedEnd,
  UnbalancedParenthesis,
  UnknownIdentifier,
  UnknownFunction,
  WrongArgumentCount,
  DivisionByZero,
  Domain,
  Overflow,
  TooComplex,
  NestingTooDeep,
  TooManyVariables,
  InvalidName,
  ReservedName,
  TooManyFunctions,
};

enum class AngleUnit : uint8_t { Radians = 0, Degrees = 1 };

// The texts of the error codes, in the order of Error (in flash on AVR).
static const char kErr00[] TINYCALC_PROGMEM = "OK";
static const char kErr01[] TINYCALC_PROGMEM = "empty expression";
static const char kErr02[] TINYCALC_PROGMEM = "syntax error";
static const char kErr03[] TINYCALC_PROGMEM = "unexpected token";
static const char kErr04[] TINYCALC_PROGMEM = "unexpected end of expression";
static const char kErr05[] TINYCALC_PROGMEM = "unbalanced parenthesis";
static const char kErr06[] TINYCALC_PROGMEM = "unknown identifier";
static const char kErr07[] TINYCALC_PROGMEM = "unknown function";
static const char kErr08[] TINYCALC_PROGMEM = "wrong number of arguments";
static const char kErr09[] TINYCALC_PROGMEM = "division by zero";
static const char kErr10[] TINYCALC_PROGMEM = "argument out of domain";
static const char kErr11[] TINYCALC_PROGMEM = "result overflow";
static const char kErr12[] TINYCALC_PROGMEM = "expression too complex";
static const char kErr13[] TINYCALC_PROGMEM = "nesting too deep";
static const char kErr14[] TINYCALC_PROGMEM = "too many variables";
static const char kErr15[] TINYCALC_PROGMEM = "invalid name";
static const char kErr16[] TINYCALC_PROGMEM = "name is reserved";
static const char kErr17[] TINYCALC_PROGMEM = "too many functions";
static const char kUnknownError[] TINYCALC_PROGMEM = "unknown error";
static const char* const kErrorTexts[] TINYCALC_PROGMEM = {kErr00, kErr01, kErr02, kErr03, kErr04, kErr05, kErr06, kErr07, kErr08, kErr09, kErr10, kErr11, kErr12, kErr13, kErr14, kErr15, kErr16, kErr17};

// The text of an error code. On AVR the pointer refers to flash: print it with
// errorTextFlash(), or copy it with strcpy_P().
inline const char* errorText(Error e) {
  const size_t i = static_cast<size_t>(e);
  if (i >= sizeof(kErrorTexts) / sizeof(kErrorTexts[0])) {
    return TINYCALC_READ_PTR(kUnknownError);
  }
  return TINYCALC_READ_PTR(kErrorTexts[i]);
}

#if defined(__AVR__)
// errorText() as a flash string for Print (Serial.print(errorTextFlash(e))).
inline const ::__FlashStringHelper* errorTextFlash(Error e) {
  return reinterpret_cast<const ::__FlashStringHelper*>(errorText(e));
}
#else
inline const char* errorTextFlash(Error e) { return errorText(e); }
#endif

namespace detail {
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
inline bool isIdentStart(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
inline bool isIdentPart(char c) { return isIdentStart(c) || isDigit(c); }
inline bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// true if s is a valid identifier that fits into TINYCALC_MAX_NAME characters
inline bool isIdentifier(const char* s) {
  if (!isIdentStart(s[0])) return false;
  size_t len = 0;
  while (isIdentPart(s[len])) ++len;
  return s[len] == '\0' && len <= TINYCALC_MAX_NAME;
}
}  // namespace detail

}  // namespace TinyCalc
