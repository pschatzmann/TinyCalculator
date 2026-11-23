#pragma once

#include <stddef.h>
#include <stdint.h>
#include "Config.h"

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

inline const char* errorText(Error e) {
  switch (e) {
    case Error::None: return "OK";
    case Error::Empty: return "empty expression";
    case Error::Syntax: return "syntax error";
    case Error::UnexpectedToken: return "unexpected token";
    case Error::UnexpectedEnd: return "unexpected end of expression";
    case Error::UnbalancedParenthesis: return "unbalanced parenthesis";
    case Error::UnknownIdentifier: return "unknown identifier";
    case Error::UnknownFunction: return "unknown function";
    case Error::WrongArgumentCount: return "wrong number of arguments";
    case Error::DivisionByZero: return "division by zero";
    case Error::Domain: return "argument out of domain";
    case Error::Overflow: return "result overflow";
    case Error::TooComplex: return "expression too complex";
    case Error::NestingTooDeep: return "nesting too deep";
    case Error::TooManyVariables: return "too many variables";
    case Error::InvalidName: return "invalid name";
    case Error::ReservedName: return "name is reserved";
    case Error::TooManyFunctions: return "too many functions";
  }
  return "unknown error";
}

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
