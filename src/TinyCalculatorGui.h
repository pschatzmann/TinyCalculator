#pragma once

// Optional GUI for TinyCalculator, built on TinyMaterialDesign.
//
// This header is NOT included by TinyCalculator.h - include it explicitly
// when you want the GUI. It requires the TinyMaterialDesign library (and
// therefore TinyGPU) to be available.
//
//   #include <TinyCalculatorGui.h>
//
//   App<RGB565> app(DefaultBoard);
//   CalculatorGui<RGB565, float> gui(Bounds(0, 0, 240, 320));
//   void setup() { app.begin(); gui.attach(app.screen()); }
//   void loop()  { app.update(); }

#include <stdio.h>
#include <string.h>
#include <string>

#include "TinyCalculator.h"
#include "TinyMaterialDesign.h"

namespace TinyCalc {

/**
 * @brief Calculator keypad with an expression line and a result line.
 *
 * The GUI owns a Calculator<Number>, so every key press goes through the
 * same parser and evaluator as the rest of the library. The class is
 * independent of the display: press() drives it directly, which allows
 * testing the key logic without a window.
 *
 * The keypad has several pages. The top-left key shows the current page
 * name and cycles through the pages:
 *  - Basic: digits, operators, parentheses, "ans", "pi"
 *  - Trig: trigonometric, inverse and hyperbolic functions, deg/rad, e, tau, phi
 *  - Math: roots, exponentials, logarithms, rounding, min/max/sum/avg, gcd/lcm
 *  - Special: gamma, factorial, erf, erfc, comb, perm and a few operators
 * Every supported function and constant has a key.
 *
 * Key behaviour:
 *  - digits, operators and function keys append to the expression
 *  - "=" evaluates the expression; the result stays in the result line and
 *    "ans" holds it for the next calculation (an operator after "=" starts
 *    from "ans")
 *  - "C" clears everything, "DEL" removes the last character
 *  - "RAD"/"DEG" toggles the angle unit used by the trigonometric functions
 *  - the expression is evaluated only when "=" is pressed
 */
template <typename RGB_T = tinymd::TINYMD_DEFAULT_RGB_T, typename Number = float>
class CalculatorGui {
 public:
  typedef Calculator<Number> Calc;
  static const size_t kKeyCount = 28;   // 7 rows x 4 columns per page
  static const size_t kPageCount = 4;

  /// @brief Key definition: the label shown on the key and the action it performs.
  ///
  /// `action` is what the key does (see press()); an empty or missing action
  /// marks an unused slot, which is hidden.
  struct KeyDef {
    const char* label;
    const char* action;
  };

  /// `bounds` is the area the calculator occupies. The layout adapts to any
  /// size; 240x320 is the default panel size of the desktop/ESP32 examples.
  explicit CalculatorGui(tinymd::Bounds bounds)
      : screen_(nullptr),
        exprLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kLabel),
        resultLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kHeadline) {
    layout(bounds);
    refreshKeys();
  }

  CalculatorGui(const CalculatorGui&) = delete;
  CalculatorGui& operator=(const CalculatorGui&) = delete;

  /// Registers all widgets with the screen. Call once, e.g. from setup().
  void attach(tinymd::Screen<RGB_T>& screen) {
    screen_ = &screen;
    screen.addWidget(exprLabel_);
    screen.addWidget(resultLabel_);
    for (size_t i = 0; i < kKeyCount; ++i) screen.addWidget(keys_[i]);
    refresh();
  }

  /// Handles one key by its action string (see keys()). Also used as the
  /// click handler of every key button.
  void press(const char* action) {
    if (action == nullptr || *action == '\0') return;
    std::string a = action;
    error_.clear();

    if (a == "page") {
      page_ = (page_ + 1) % kPageCount;
      refreshKeys();
    } else if (a == "angle") {
      toggleAngle();
    } else if (a == "clear") {
      expr_.clear();
      showResult_ = false;
    } else if (a == "del") {
      if (showResult_) {
        expr_.clear();
        showResult_ = false;
      } else if (!expr_.empty()) {
        expr_.erase(expr_.size() - 1);
      }
    } else if (a == "=") {
      evaluate();
    } else {
      // digits, operators, functions and constants
      if (showResult_) {
        // an operator after "=" continues with the result
        bool continues = a == "+" || a == "-" || a == "*" || a == "/" || a == "^" ||
                         a == "%" || a == "!" || a == ",";
        expr_ = continues ? "ans" : "";
        showResult_ = false;
      }
      expr_ += a;
    }
    refresh();
  }

  /// Expression as typed so far.
  const std::string& expression() const { return expr_; }

  /// Text currently shown in the result line: the result after "=", or the
  /// error of the last "=" (empty otherwise).
  std::string resultText() const {
    return showResult_ ? resultText_ : error_;
  }

  /// The calculator used by the keypad (variables, angle unit, ...).
  Calc& calculator() { return calc_; }

  /// The button of key `index` (0 .. kKeyCount-1) on the current page, e.g.
  /// to fire its onClick in tests.
  tinymd::Button<RGB_T>& key(size_t index) { return keys_[index]; }

  /// Index of the current keypad page (0 .. kPageCount-1).
  size_t page() const { return page_; }

  /// Key table of one page (kPageCount pages, kKeyCount entries each).
  static const KeyDef* keys(size_t page = 0) {
    // Entries that are not listed are zero-initialized, i.e. unused.
    static const KeyDef basic[kKeyCount] = {
        {"", "page"},     {"RAD", "angle"}, {"(", "("},      {")", ")"},
        {"C", "clear"},   {"DEL", "del"},   {"^", "^"},      {"%", "%"},
        {"7", "7"},       {"8", "8"},       {"9", "9"},      {"/", "/"},
        {"4", "4"},       {"5", "5"},       {"6", "6"},      {"*", "*"},
        {"1", "1"},       {"2", "2"},       {"3", "3"},      {"-", "-"},
        {"0", "0"},       {".", "."},       {"!", "!"},      {"+", "+"},
        {"ans", "ans"},   {",", ","},       {"pi", "pi"},    {"=", "="},
    };
    static const KeyDef trig[kKeyCount] = {
        {"", "page"},     {"sin", "sin("},  {"cos", "cos("},  {"tan", "tan("},
        {"asin", "asin("}, {"acos", "acos("}, {"atan", "atan("}, {"atan2", "atan2("},
        {"sec", "sec("},  {"csc", "csc("},  {"cot", "cot("},  {"asec", "asec("},
        {"acsc", "acsc("}, {"acot", "acot("}, {"sinh", "sinh("}, {"cosh", "cosh("},
        {"tanh", "tanh("}, {"asinh", "asinh("}, {"acosh", "acosh("}, {"atanh", "atanh("},
        {"deg", "deg("},  {"rad", "rad("},  {"e", "e"},      {"tau", "tau"},
        {"phi", "phi"},   {"(", "("},       {")", ")"},      {"ans", "ans"},
    };
    static const KeyDef math[kKeyCount] = {
        {"", "page"},     {"sqrt", "sqrt("}, {"cbrt", "cbrt("}, {"root", "root("},
        {"pow", "pow("},  {"exp", "exp("},  {"exp2", "exp2("}, {"expm1", "expm1("},
        {"ln", "ln("},    {"log", "log("},  {"log2", "log2("}, {"log10", "log10("},
        {"abs", "abs("},  {"floor", "floor("}, {"ceil", "ceil("}, {"round", "round("},
        {"trunc", "trunc("}, {"frac", "frac("}, {"sign", "sign("}, {"mod", "mod("},
        {"hypot", "hypot("}, {"clamp", "clamp("}, {"min", "min("}, {"max", "max("},
        {"sum", "sum("},  {"avg", "avg("},  {"gcd", "gcd("},  {"lcm", "lcm("},
    };
    static const KeyDef special[kKeyCount] = {
        {"", "page"},     {"gamma", "gamma("}, {"fact", "fact("}, {"erf", "erf("},
        {"erfc", "erfc("}, {"comb", "comb("}, {"perm", "perm("}, {"%", "%"},
        {"!", "!"},       {"(", "("},       {")", ")"},      {",", ","},
        {"ans", "ans"},   {"pi", "pi"},     {"e", "e"},      {"tau", "tau"},
        {"phi", "phi"},   {"^", "^"},
    };
    switch (page % kPageCount) {
      case 1: return trig;
      case 2: return math;
      case 3: return special;
      default: return basic;
    }
  }

  /// Name shown on the page key.
  static const char* pageName(size_t page) {
    static const char* names[kPageCount] = {"Basic", "Trig", "Math", "Special"};
    return names[page % kPageCount];
  }

 private:
  // Position the widgets inside `b`: expression line, result line, keypad.
  void layout(tinymd::Bounds b) {
    const int32_t m = 8;  // outer margin
    const int32_t g = 4;  // gap between keys
    const int32_t exprH = 20, resultH = 44;
    const int32_t top = b.y + m + exprH + resultH + g;  // keypad top
    const int32_t cols = 4, rows = 7;
    const int32_t keyW = (b.w - 2 * m - (cols - 1) * g) / cols;
    const int32_t keyH = (b.y + b.h - m - top - (rows - 1) * g) / rows;

    exprLabel_.bounds = tinymd::Bounds(b.x + m, b.y + m, b.w - 2 * m, exprH);
    resultLabel_.bounds = tinymd::Bounds(b.x + m, b.y + m + exprH, b.w - 2 * m, resultH);

    for (size_t i = 0; i < kKeyCount; ++i) {
      int32_t row = static_cast<int32_t>(i) / cols;
      int32_t col = static_cast<int32_t>(i) % cols;
      keys_[i].bounds = tinymd::Bounds(b.x + m + col * (keyW + g),
                                       top + row * (keyH + g), keyW, keyH);
      keys_[i].onClick = [this, i]() { press(keys(page_)[i].action); };
    }
  }

  // Shows the key table of the current page.
  void refreshKeys() {
    const KeyDef* defs = keys(page_);
    for (size_t i = 0; i < kKeyCount; ++i) {
      const bool used = defs[i].action != nullptr && *defs[i].action != '\0';
      keys_[i].visible = used;
      if (!used) continue;
      keys_[i].setLabel(labelOf(defs[i]));
      keys_[i].setVariant(variantFor(defs[i].action));
    }
  }

  const char* labelOf(const KeyDef& d) const {
    if (strcmp(d.action, "page") == 0) return pageName(page_);
    if (strcmp(d.action, "angle") == 0) {
      return calc_.angleUnit() == AngleUnit::Degrees ? "DEG" : "RAD";
    }
    return d.label;
  }

  static tinymd::ButtonVariant variantFor(const char* action) {
    std::string a = action;
    if (a == "=") return tinymd::ButtonVariant::kFilled;
    if (a == "angle" || a == "page" || a == "clear" || a == "del") {
      return tinymd::ButtonVariant::kText;
    }
    if (a.size() == 1 && (a[0] >= '0' && a[0] <= '9')) return tinymd::ButtonVariant::kOutlined;
    if (a == ".") return tinymd::ButtonVariant::kOutlined;
    return tinymd::ButtonVariant::kTonal;
  }

  void toggleAngle() {
    bool deg = calc_.angleUnit() == AngleUnit::Degrees;
    calc_.setAngleUnit(deg ? AngleUnit::Radians : AngleUnit::Degrees);
    refreshKeys();  // updates the RAD/DEG label
  }

  void evaluate() {
    if (expr_.empty()) return;
    typename Calc::Result r = calc_.evaluate(expr_.c_str());  // also updates "ans"
    if (r.ok()) {
      resultText_ = format(r.value);
      showResult_ = true;
    } else {
      showResult_ = false;
      error_ = errorText(r.error);
    }
  }

  static std::string format(Number v) {
    // 7 significant digits for float, 12 for double
    char buf[32];
    if (sizeof(Number) <= 4) {
      snprintf(buf, sizeof(buf), "%.7g", static_cast<double>(v));
    } else {
      snprintf(buf, sizeof(buf), "%.12g", static_cast<double>(v));
    }
    return buf;
  }

  void refresh() {
    std::string e = showResult_ ? expr_ + " =" : expr_;
    exprLabel_.setText(e.c_str());
    resultLabel_.setText(resultText().c_str());
    if (screen_ != nullptr) screen_->invalidate();
  }

  tinymd::Screen<RGB_T>* screen_;
  Calc calc_;
  std::string expr_;
  std::string resultText_;
  std::string error_;
  bool showResult_ = false;
  size_t page_ = 0;
  tinymd::Label<RGB_T> exprLabel_;
  tinymd::Label<RGB_T> resultLabel_;
  tinymd::Button<RGB_T> keys_[kKeyCount];
};

}  // namespace TinyCalc
