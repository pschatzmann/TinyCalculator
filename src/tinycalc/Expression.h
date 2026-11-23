#pragma once

#include <stddef.h>
#include <stdint.h>
#include "Config.h"

namespace TinyCalc {

// Instructions of the compiled (postfix / RPN) program.
enum class OpCode : uint8_t {
  Push,  // push value
  Load,  // push variable slot `index`
  Add, Sub, Mul, Div, Mod, Pow,  // binary: pop 2, push 1
  Neg,   // unary minus
  Call,  // call built-in function `index` with `argc` arguments
  UserCall  // call user function `index` with `argc` arguments
};

// Load index with this bit set refers to a parameter of the running user
// function (the low bits are the parameter number) instead of a variable.
static const uint8_t kFrameBit = 0x80;

/**
 * @brief One instruction of a compiled program.
 *
 * The meaning of `index`, `argc` and `value` depends on `code` (see OpCode).
 */
template <typename T>
struct Op {
  OpCode code;
  uint8_t index;  // variable slot (Load) or Fn id (Call)
  uint8_t argc;   // argument count (Call)
  T value;        // literal (Push)
};

/**
 * @class Expression
 * @brief A compiled formula: a fixed-capacity RPN (postfix) program.
 *
 * Produced by Calculator::compile() and run by Calculator::run(). The storage
 * is inside the object, so no heap allocation is needed.
 *
 * @tparam T         number type
 * @tparam Capacity  maximum number of instructions
 */
template <typename T = float, size_t Capacity = TINYCALC_MAX_OPS>
class Expression {
  static_assert(Capacity > 0, "Expression capacity must be > 0");

 public:
  Expression() : count_(0), maxStack_(0) {}

  void clear() {
    count_ = 0;
    maxStack_ = 0;
  }
  bool append(const Op<T>& op) {
    if (count_ >= Capacity) return false;
    ops_[count_++] = op;
    return true;
  }
  void setMaxStack(size_t n) { maxStack_ = n; }

  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  size_t maxStack() const { return maxStack_; }
  static size_t capacity() { return Capacity; }
  const Op<T>& operator[](size_t i) const { return ops_[i]; }

 private:
  Op<T> ops_[Capacity];
  size_t count_;
  size_t maxStack_;
};

}  // namespace TinyCalc
