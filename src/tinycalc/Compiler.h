#pragma once

#include <stdlib.h>
#include <string.h>
#include "Config.h"
#include "Expression.h"
#include "Functions.h"
#include "Types.h"

namespace TinyCalc {
namespace detail {

enum class Tok : uint8_t {
  End, Number, Ident, Function,
  Plus, Minus, Star, Slash, Percent, Caret, Bang,
  LParen, RParen, Comma
};

/**
 * @brief One lexical token produced by the Compiler's scanner.
 *
 * Holds the token type, its position in the text, and the value for numbers,
 * the name for identifiers and the function description for function calls.
 */
template <typename T>
struct Token {
  Tok type;
  size_t pos;
  T number;
  const FunctionInfo* function;  // built-in function, if builtin
  bool builtin;                  // false: a user function (looked up by name)
  char name[TINYCALC_MAX_NAME + 1];
};

/**
 * @class Compiler
 * @brief Parses a formula text and emits the RPN program.
 *
 * A scanner produces tokens on demand and a recursive descent parser generates
 * the instructions of an Expression directly, so no syntax tree is built. It
 * tracks the maximum stack depth needed by the program.
 *
 * Grammar, from lowest to highest precedence:
 * - expr    := term (('+' | '-') term)*
 * - term    := unary (('*' | '/' | '%' | implicit) unary)*
 * - unary   := ('-' | '+') unary | power
 * - power   := postfix ('^' unary)?   (right associative)
 * - postfix := primary '!'*
 * - primary := number | constant | variable | func '(' args ')' | '(' expr ')'
 *
 * @tparam T         number type
 * @tparam N         capacity of the target Expression
 * @tparam Resolver  type with `int find(const char*) const` returning the variable
 *                   slot, or -1 for unknown names
 */
template <typename T, size_t N, typename Resolver>
class Compiler {
 public:
  Compiler(const char* text, Expression<T, N>& out, const Resolver& resolver)
      : src_(text), pos_(0), out_(out), res_(resolver), err_(Error::None),
        errPos_(0), depth_(0), stack_(0), maxStack_(0) {
    cur_.type = Tok::End;
    cur_.pos = 0;
    cur_.number = T();
    cur_.function = nullptr;
    cur_.builtin = false;
    cur_.name[0] = '\0';
  }

  Error run(size_t& errorPos) {
    out_.clear();
    bool ok = advance();
    if (ok && cur_.type == Tok::End) ok = fail(Error::Empty, 0);
    if (ok) ok = parseExpression();
    // unit conversion: "expr to unit" expresses the value in that unit
    if (ok && cur_.type == Tok::Ident && isToKeyword()) {
      ok = advance();
      if (ok && cur_.type != Tok::Ident) {
        ok = fail(cur_.type == Tok::End ? Error::UnexpectedEnd : Error::Syntax, cur_.pos);
      }
      if (ok) {
        T factor = T();
        if (!lookupConstant<T>(cur_.name, factor)) {
          ok = fail(Error::UnknownIdentifier, cur_.pos);
        } else {
          ok = emitPush(factor) && emitBinary(OpCode::Div) && advance();
        }
      }
    }
    if (ok && cur_.type != Tok::End) {
      ok = fail(cur_.type == Tok::RParen ? Error::UnbalancedParenthesis
                                         : Error::UnexpectedToken,
                cur_.pos);
    }
    if (!ok) {
      errorPos = errPos_;
      return err_;
    }
    out_.setMaxStack(static_cast<size_t>(maxStack_));
    errorPos = 0;
    return Error::None;
  }

 private:
  // ---- error handling -------------------------------------------------
  bool fail(Error e, size_t pos) {
    if (err_ == Error::None) {  // keep the first error
      err_ = e;
      errPos_ = pos;
    }
    return false;
  }

  // ---- lexer ----------------------------------------------------------
  bool advance() {
    while (isSpace(src_[pos_])) ++pos_;
    cur_.pos = pos_;
    char c = src_[pos_];
    if (c == '\0') {
      cur_.type = Tok::End;
      return true;
    }
    if (isDigit(c) || (c == '.' && isDigit(src_[pos_ + 1]))) return scanNumber();
    if (isIdentStart(c)) return scanIdentifier();
    ++pos_;
    switch (c) {
      case '+': cur_.type = Tok::Plus; return true;
      case '-': cur_.type = Tok::Minus; return true;
      case '/': cur_.type = Tok::Slash; return true;
      case '%': cur_.type = Tok::Percent; return true;
      case '^': cur_.type = Tok::Caret; return true;
      case '!': cur_.type = Tok::Bang; return true;
      case '(': cur_.type = Tok::LParen; return true;
      case ')': cur_.type = Tok::RParen; return true;
      case ',': cur_.type = Tok::Comma; return true;
      case '*':  // '**' is accepted as an alias for '^'
        if (src_[pos_] == '*') {
          ++pos_;
          cur_.type = Tok::Caret;
        } else {
          cur_.type = Tok::Star;
        }
        return true;
      default:
        return fail(Error::Syntax, cur_.pos);
    }
  }

  bool scanNumber() {
    size_t start = pos_;
    while (isDigit(src_[pos_])) ++pos_;
    if (src_[pos_] == '.') {
      ++pos_;
      while (isDigit(src_[pos_])) ++pos_;
    }
    if (src_[pos_] == 'e' || src_[pos_] == 'E') {  // exponent only if digits follow
      size_t p = pos_ + 1;
      if (src_[p] == '+' || src_[p] == '-') ++p;
      if (isDigit(src_[p])) {
        pos_ = p;
        while (isDigit(src_[pos_])) ++pos_;
      }
    }
    size_t len = pos_ - start;
    char buf[32];
    if (len >= sizeof(buf)) return fail(Error::Syntax, start);
    memcpy(buf, src_ + start, len);
    buf[len] = '\0';
    cur_.type = Tok::Number;
    cur_.number = static_cast<T>(strtod(buf, nullptr));
    return true;
  }

  bool scanIdentifier() {
    size_t start = pos_;
    while (isIdentPart(src_[pos_])) ++pos_;
    size_t len = pos_ - start;
    if (len > TINYCALC_MAX_NAME) return fail(Error::UnknownIdentifier, start);
    memcpy(cur_.name, src_ + start, len);
    cur_.name[len] = '\0';

    size_t p = pos_;
    while (isSpace(src_[p])) ++p;
    if (src_[p] == '(') {  // identifier followed by '(' must be a function
      cur_.function = findFunction(cur_.name);  // nullptr: a user function
      cur_.builtin = cur_.function != nullptr;
      cur_.type = Tok::Function;
    } else {
      cur_.type = Tok::Ident;
    }
    return true;
  }

  // ---- parser ---------------------------------------------------------
  bool enter() {
    if (++depth_ > TINYCALC_MAX_NESTING) return fail(Error::NestingTooDeep, cur_.pos);
    return true;
  }
  void leave() { --depth_; }

  // the keyword of a unit conversion ("5 km to m")
  bool isToKeyword() const { return cur_.type == Tok::Ident && strcmp(cur_.name, "to") == 0; }

  static bool startsOperand(Tok t) {
    return t == Tok::Number || t == Tok::Ident || t == Tok::Function || t == Tok::LParen;
  }

  bool parseExpression() {
    if (!enter()) return false;
    if (!parseTerm()) return false;
    while (cur_.type == Tok::Plus || cur_.type == Tok::Minus) {
      OpCode op = cur_.type == Tok::Plus ? OpCode::Add : OpCode::Sub;
      if (!advance() || !parseTerm() || !emitBinary(op)) return false;
    }
    leave();
    return true;
  }

  bool parseTerm() {
    if (!parseUnary()) return false;
    for (;;) {
      OpCode op = OpCode::Mul;
      if (cur_.type == Tok::Star) {
        op = OpCode::Mul;
      } else if (cur_.type == Tok::Slash) {
        op = OpCode::Div;
      } else if (cur_.type == Tok::Percent) {
        op = OpCode::Mod;
      } else if (startsOperand(cur_.type) && !isToKeyword()) {  // implicit multiplication: 2pi, 3(x+1)
        if (!parseUnary() || !emitBinary(OpCode::Mul)) return false;
        continue;
      } else {
        return true;
      }
      if (!advance() || !parseUnary() || !emitBinary(op)) return false;
    }
  }

  bool parseUnary() {
    if (cur_.type == Tok::Minus || cur_.type == Tok::Plus) {
      bool negate = cur_.type == Tok::Minus;
      if (!enter() || !advance() || !parseUnary()) return false;
      leave();
      return !negate || emitUnary(OpCode::Neg);
    }
    return parsePower();
  }

  bool parsePower() {
    if (!parsePostfix()) return false;
    if (cur_.type == Tok::Caret) {
      if (!advance() || !parseUnary() || !emitBinary(OpCode::Pow)) return false;
    }
    return true;
  }

  bool parsePostfix() {
    if (!parsePrimary()) return false;
    while (cur_.type == Tok::Bang) {
      if (!advance() || !emitCall(Fn::Fact, 1)) return false;
    }
    return true;
  }

  bool parsePrimary() {
    switch (cur_.type) {
      case Tok::Number:
        return emitPush(cur_.number) && advance();
      case Tok::Ident:
        return parseIdentifier();
      case Tok::Function:
        return parseCall();
      case Tok::LParen: {
        size_t open = cur_.pos;
        if (!advance() || !parseExpression()) return false;
        if (cur_.type != Tok::RParen) {
          if (cur_.type == Tok::End) return fail(Error::UnbalancedParenthesis, open);
          return fail(Error::UnexpectedToken, cur_.pos);
        }
        return advance();
      }
      case Tok::End:
        return fail(Error::UnexpectedEnd, cur_.pos);
      default:
        return fail(Error::UnexpectedToken, cur_.pos);
    }
  }

  bool parseIdentifier() {
    T constant = T();
    if (lookupConstant<T>(cur_.name, constant)) return emitPush(constant) && advance();
    int slot = res_.find(cur_.name);
    if (slot < 0) return fail(Error::UnknownIdentifier, cur_.pos);
    return emit(OpCode::Load, static_cast<uint8_t>(slot), 0, T(), 1) && advance();
  }

  bool parseCall() {
    const FunctionInfo* info = cur_.function;
    size_t start = cur_.pos;
    char name[TINYCALC_MAX_NAME + 1];
    strcpy(name, cur_.name);
    if (!advance() || !advance()) return false;  // skip name and '('
    int argc = 0;
    if (cur_.type != Tok::RParen) {
      for (;;) {
        if (!parseExpression()) return false;
        ++argc;
        if (cur_.type != Tok::Comma) break;
        if (!advance()) return false;
      }
    }
    if (cur_.type != Tok::RParen) {
      if (cur_.type == Tok::End) return fail(Error::UnbalancedParenthesis, start);
      return fail(Error::UnexpectedToken, cur_.pos);
    }
    if (!advance()) return false;
    if (info != nullptr) {  // built-in function
      if (argc < info->minArgs || argc > info->maxArgs) {
        return fail(Error::WrongArgumentCount, start);
      }
      return emitCall(info->id, argc);
    }
    // user function: looked up by name, the number of parameters must match
    int slot = -1, params = 0;
    if (!res_.findUserFunction(name, slot, params)) return fail(Error::UnknownFunction, start);
    if (argc != params) return fail(Error::WrongArgumentCount, start);
    return emit(OpCode::UserCall, static_cast<uint8_t>(slot), static_cast<uint8_t>(argc),
                T(), 1 - argc);
  }

  // ---- code emission --------------------------------------------------
  bool emit(OpCode code, uint8_t index, uint8_t argc, T value, int stackDelta) {
    Op<T> op;
    op.code = code;
    op.index = index;
    op.argc = argc;
    op.value = value;
    if (!out_.append(op)) return fail(Error::TooComplex, cur_.pos);
    stack_ += stackDelta;
    if (stack_ > maxStack_) maxStack_ = stack_;
    return true;
  }
  bool emitPush(T v) { return emit(OpCode::Push, 0, 0, v, 1); }
  bool emitBinary(OpCode code) { return emit(code, 0, 0, T(), -1); }
  bool emitUnary(OpCode code) { return emit(code, 0, 0, T(), 0); }
  bool emitCall(Fn fn, int argc) {
    return emit(OpCode::Call, static_cast<uint8_t>(fn), static_cast<uint8_t>(argc),
                T(), 1 - argc);
  }

  // ---- character classes ----------------------------------------------
  static bool isDigit(char c) { return TinyCalc::detail::isDigit(c); }
  static bool isIdentStart(char c) { return TinyCalc::detail::isIdentStart(c); }
  static bool isIdentPart(char c) { return TinyCalc::detail::isIdentPart(c); }
  static bool isSpace(char c) { return TinyCalc::detail::isSpace(c); }

  const char* src_;
  size_t pos_;
  Token<T> cur_;
  Expression<T, N>& out_;
  const Resolver& res_;
  Error err_;
  size_t errPos_;
  int depth_;
  int stack_;
  int maxStack_;
};

}  // namespace detail
}  // namespace TinyCalc
