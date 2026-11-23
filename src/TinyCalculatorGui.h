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
//   void setup() { app.begin(); gui.begin(app.screen()); }
//   void loop()  { app.update(); }

#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <string.h>
#include <string>
#include <vector>

#include "TinyCalculator.h"
#include "TinyMaterialDesign.h"

namespace TinyCalc {

/**
 * @brief The screens of the calculator that are not part of the basic
 * keypad. Each one is off by default and is switched on with
 * CalculatorGui::enable(), e.g. `gui.enable(GuiScreen::Trig);`.
 *
 *  - Trig: trigonometric, inverse and hyperbolic functions, deg/rad, e, tau, phi
 *  - Math: roots, exponentials, logarithms, rounding, aggregates, gcd/lcm
 *  - Special: gamma, factorial, erf, erfc, comb, perm
 *  - Units: unit keys, "to" conversions, user-defined functions
 *  - Plot: plotting of a function of x (Plot key and x key on the Basic page)
 */
enum class GuiScreen : uint8_t { Trig, Math, Special, Units, Plot };

/**
 * @brief Look of the calculator: the TinyMaterialDesign theme of all its
 * widgets. Its typography holds the fonts (label, body, title, headline), so
 * a TinyFonts font assigned there is used by the expression line (label) and
 * the result line (headline).
 */
template <typename RGB_T = tinymd::TINYMD_DEFAULT_RGB_T>
using CalculatorTheme = tinymd::MaterialTheme<RGB_T>;

/**
 * @brief Container for the calculator's widgets. Gestures go to the widget
 * under the touch first. A swipe that no widget handles reaches this
 * container, which reports left swipes through onSwipeLeft.
 */
template <typename RGB_T = tinymd::TINYMD_DEFAULT_RGB_T>
class CalculatorPanel : public tinymd::Container<RGB_T> {
 public:
  CalculatorPanel() = default;

  /// Called for a left / right swipe that no child handled.
  std::function<void()> onSwipeLeft;
  std::function<void()> onSwipeRight;

  bool onGesture(const tinygpu::GestureEvent& event) override {
    const bool handled = tinymd::Container<RGB_T>::onGesture(event);
    if (handled) return true;
    if (event.type == tinygpu::GestureType::kSwipeLeft && onSwipeLeft) {
      onSwipeLeft();
      return true;
    }
    if (event.type == tinygpu::GestureType::kSwipeRight && onSwipeRight) {
      onSwipeRight();
      return true;
    }
    return false;
  }
};

/**
 * @brief Calculator keypad with an expression line and a result line, and a
 * plot of the expression as a function of x.
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
 *  - "Plot" plots the expression as a function of x (see plotExpression());
 *    "x" enters the variable x.
 *
 * Plot screen: the From and To fields hold the x range (default -10 .. 10).
 * Tap a field, or use the From/To keys, to select it; the numeric pad types
 * into the selected field. The plot is redrawn whenever both fields hold a
 * valid range with To above From. "Back" returns to the keypad.
 */
template <typename RGB_T = tinymd::TINYMD_DEFAULT_RGB_T, typename Number = float>
class CalculatorGui {
 public:
  typedef Calculator<Number> Calc;
  static const size_t kKeyCount = 28;   // 7 rows x 4 columns per page
  static const size_t kPageCount = 5;
  static const size_t kPlotSamples = 160;  // points of a plot

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
  explicit CalculatorGui(tinymd::Bounds bounds,
                         const CalculatorTheme<RGB_T>& theme = tinymd::defaultTheme<RGB_T>())
      : theme_(theme),
        screen_(nullptr),
        exprLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kBody),
        resultLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kHeadline),
        fromField_(tinymd::Bounds(), "From"),
        toField_(tinymd::Bounds(), "To") {
    bounds_ = bounds;
    relayout();
    plot_.addSeries(theme_.colors.primary);
    panel_.onSwipeLeft = [this]() { swipeLeft(); };
    panel_.onSwipeRight = [this]() { swipeRight(); };
    fromField_.onFocus = [this]() { activeField_ = 0; };
    toField_.onFocus = [this]() { activeField_ = 1; };
    syncRangeFields();
    calc_.setVariable("x", Number(0));
    dimTheme_ = fadedTheme(theme_);
    applyMode();
    updateUnitKeys();
  }

  // Removes the widgets from the screen if the GUI is still attached. The
  // screen must still exist, so destroy the GUI before its screen (or call
  // end() first).
  ~CalculatorGui() { end(); }

  CalculatorGui(const CalculatorGui&) = delete;
  CalculatorGui& operator=(const CalculatorGui&) = delete;

  /// Adds all widgets to `screen`. Call once, e.g. from setup(). Calling it
  /// again while attached does nothing. The screen must outlive the GUI or
  /// end() must be called before the screen is destroyed.
  void begin(tinymd::Screen<RGB_T>& screen) {
    if (attached_) return;
    screen_ = &screen;
    screen.addWidget(panel_);
    panel_.addChild(exprLabel_);
    panel_.addChild(resultLabel_);
    for (size_t i = 0; i < kKeyCount; ++i) panel_.addChild(keys_[i]);
    panel_.addChild(plot_);
    panel_.addChild(fromField_);
    panel_.addChild(toField_);
    screen.setTheme(theme_);  // the calculator's theme applies to every widget
    attached_ = true;
    refresh();
  }

  /// Removes the widgets from the screen given to begin(). The GUI keeps its
  /// state, so begin() can attach it again later. Does nothing if not attached.
  void end() {
    if (!attached_) return;
    screen_->removeChild(panel_);
    panel_.clearChildren();
    screen_ = nullptr;
    attached_ = false;
  }

  /// Plots `expr` as a function of x and shows the plot screen. Returns false
  /// (and sets resultText()) if the expression is empty or invalid.
  bool beginPlot(const char* expr) {
    expr_ = expr != nullptr ? expr : "";
    showResult_ = false;
    error_.clear();
    plotExpression();
    refresh();
    return plotting_;
  }

  /// Leaves the plot screen and returns to the keypad. The expression is kept.
  void endPlot() {
    if (plotting_) closePlot();
  }

  /// Handles one key by its action string (see keys() and plotKeys()). Also
  /// used as the click handler of every key button.
  void press(const char* action) {
    if (action == nullptr || *action == '\0') return;
    std::string a = action;
    error_.clear();

    if (plotting_) {
      handlePlotKey(a);
      refresh();
      return;
    }

    if (a == "page") {
      changePage(1);  // to the next enabled page
    } else if (a == "plot") {
      plotExpression();
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
      // after "=", an operator continues with the result, anything else starts over
      std::string prefix = expr_;
      if (showResult_) {
        bool continues = a == "+" || a == "-" || a == "*" || a == "/" || a == "^" ||
                         a == "%" || a == "!" || a == "," || a == "to";
        prefix = continues ? "ans" : "";
      }
      if (!acceptable(a, prefix)) {  // invalid combination: the key is ignored
        refresh();
        return;
      }
      if (showResult_) {
        expr_ = prefix;
        showResult_ = false;
      }
      // "to" is a conversion, ":=" starts a body: both are written out in full
      const std::string token = a == "to" ? " to " : a == "def" ? "=" : a;
      if (needsProduct(expr_, token)) expr_ += "*";
      expr_ += token;
    }
    refresh();
  }

  // ---- key entry rules --------------------------------------------------
  // The keypad only accepts keys that continue the expression validly, e.g.
  // no operator after an operator, no ")" without "(", no second "." in a
  // number. Rejected keys are ignored.

  static char lastChar(const std::string& t) { return t.empty() ? '\0' : t[t.size() - 1]; }
  static bool isDigitChar(char c) { return c >= '0' && c <= '9'; }
  static bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
  static bool isOperatorChar(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%' || c == ',';
  }
  static bool isBinaryOp(const std::string& a) {
    return a == "+" || a == "*" || a == "/" || a == "^" || a == "%";
  }
  static bool isNameKey(const std::string& a) {
    return a == "pi" || a == "e" || a == "tau" || a == "phi" || a == "ans" || a == "x" ||
           isUnitName(a);
  }

  // The units (see unitTable() in Functions.h).
  static bool isUnitName(const std::string& a) {
    return a == "m" || a == "cm" || a == "mm" || a == "km" || a == "mi" || a == "ft" ||
           a == "in" || a == "yd" || a == "kg" || a == "g" || a == "lb" || a == "oz" ||
           a == "s" || a == "min" || a == "h" || a == "day" || a == "degC" ||
           a == "degF" || a == "K";
  }
  static bool isFunctionKey(const std::string& a) { return !a.empty() && a[a.size() - 1] == '('; }

  // number of '(' not yet closed
  static int openDepth(const std::string& t) {
    int depth = 0;
    for (char c : t) {
      if (c == '(') ++depth;
      if (c == ')' && depth > 0) --depth;
    }
    return depth;
  }

  // true if the number being typed already contains a '.'
  static bool numberHasDot(const std::string& t) {
    for (size_t i = t.size(); i-- > 0;) {
      if (t[i] == '.') return true;
      if (!isDigitChar(t[i])) return false;
    }
    return false;
  }

  // true if key `a` may follow the expression `t`
  // Dimension of a unit: 0 none, 1 length, 2 mass, 3 time, 4 temperature.
  static int unitDimension(const std::string& u) {
    if (u == "m" || u == "cm" || u == "mm" || u == "km" || u == "mi" || u == "ft" ||
        u == "in" || u == "yd") {
      return 1;
    }
    if (u == "kg" || u == "g" || u == "lb" || u == "oz") return 2;
    if (u == "s" || u == "min" || u == "h" || u == "day") return 3;
    if (u == "degC" || u == "degF" || u == "K") return 4;
    return 0;
  }

  // The letters at the end of t (the name just typed), e.g. "km" in "5 km".
  static std::string trailingWord(const std::string& t) {
    size_t b = t.size();
    while (b > 0 && isLetter(t[b - 1])) --b;
    return t.substr(b);
  }

  // Conversion state of an expression: a "to" is in it, the target unit is
  // still missing (awaiting), and the source unit before the "to".
  struct ToState {
    bool present;
    bool awaiting;
    std::string source;
  };

  static ToState toState(const std::string& t) {
    ToState st{false, false, ""};
    const size_t k = t.rfind(" to");
    if (k == std::string::npos) return st;
    const size_t after = k + 3;
    if (after < t.size() && t[after] != ' ') return st;  // "tox" is not the keyword
    st.present = true;
    st.awaiting = t.find_first_not_of(' ', after) == std::string::npos;
    st.source = trailingWord(t.substr(0, k));
    return st;
  }

  // True if the unit key may be pressed for this expression.
  static bool unitAllowed(const std::string& unit, const std::string& t) {
    const ToState st = toState(t);
    if (!st.present) return true;  // a unit can always be typed before "to"
    return st.awaiting && unitDimension(unit) != 0 &&
           unitDimension(unit) == unitDimension(st.source);
  }

  static bool acceptable(const std::string& a, const std::string& t) {
    const ToState conv = toState(t);
    if (conv.present) {  // after "to": only a unit of the same kind as the source
      return conv.awaiting && isUnitName(a) && unitAllowed(a, t);
    }
    if (a == "to") return unitDimension(trailingWord(t)) != 0;  // a unit comes first
    const char c = lastChar(t);
    const bool empty = t.empty();
    const bool open = c == '(';
    const bool op = isOperatorChar(c);
    // a number, name, ")" or "!" (a space after "to" is not an operand yet)
    const bool operand = !empty && !open && !op && c != ' ';

    if (a == "(" || isFunctionKey(a)) return true;  // a function is also a call start
    if (a == ")") return openDepth(t) > 0 && operand;
    if (a == "!") return operand && c != '.';
    if (a == ",") return openDepth(t) > 0 && operand;
    if (a == "-") return c != '-';  // subtraction or a sign, but never "--"
    if (a == ".") return !numberHasDot(t) && !isLetter(c) && c != ')' && c != '!';
    if (isBinaryOp(a) || a == "to") return operand;
    if (a == "def") return openDepth(t) == 0 && c == ')';  // a function header "f(x)" then "="
    if (isNameKey(a)) return c != '.';
    return true;  // digits
  }

  // an explicit "*" is needed when a name, ")" or "!" is followed by an operand
  // (so that "pi" then "2" is "pi*2", not the name "pi2")
  static bool needsProduct(const std::string& t, const std::string& a) {
    const char c = lastChar(t);
    const bool endsOperand = isLetter(c) || c == ')' || c == '!';
    const bool startsOperand = isDigitChar(a[0]) || isLetter(a[0]) || a[0] == '(';
    return endsOperand && startsOperand;
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

  /// The button of key `index` (0 .. kKeyCount-1) of the shown table, e.g.
  /// to fire its onClick in tests.
  tinymd::Button<RGB_T>& key(size_t index) { return keys_[index]; }

  /// True if the key of slot `index` is drawn faded: a unit that cannot follow
  /// the expression (after "to", only units of the same kind as the source).
  bool isKeyFaded(size_t index) const { return keyFaded(index); }

  /// Index of the current keypad page (0 .. kPageCount-1).
  size_t page() const { return page_; }

  /// Switches on a screen. Only the Basic keypad is shown until a screen is enabled.
  void enable(GuiScreen screen) {
    if (screen == GuiScreen::Plot) {
      plotEnabled_ = true;
    } else {
      pages_ = static_cast<uint8_t>(pages_ | (1u << pageOf(screen)));
    }
    applyMode();
    refresh();
  }

  /// True if the screen is switched on.
  bool isEnabled(GuiScreen screen) const {
    if (screen == GuiScreen::Plot) return plotEnabled_;
    return (pages_ & (1u << pageOf(screen))) != 0;
  }

  /// Switches on all screens.
  void enableAll() {
    enable(GuiScreen::Trig);
    enable(GuiScreen::Math);
    enable(GuiScreen::Special);
    enable(GuiScreen::Units);
    enable(GuiScreen::Plot);
  }

  /// The area the calculator occupies.
  tinymd::Bounds bounds() const { return bounds_; }

  /// Signature of a font function: it receives the theme and assigns the fonts
  /// of its typography (e.g. TinyFonts, tinyfonts::TinyGPUFont). Example:
  /// @code
  /// void myFonts(CalculatorTheme<RGB565>& t) {
  ///   t.typography.label = &smallFont;      // keys, chart axes, field labels
  ///   t.typography.body = &smallFont;       // expression line, chart text
  ///   t.typography.headline = &resultFont;  // result line
  /// }
  /// calculatorGui.assignFonts(myFonts);
  /// @endcode
  typedef void (*FontFunction)(CalculatorTheme<RGB_T>& theme);

  /// Lets a function defined in the sketch assign the fonts of the theme.
  /// The new fonts are applied at once and to the attached screen.
  void assignFonts(FontFunction fn) {
    if (fn == nullptr) return;
    fn(theme_);
    applyTheme();
  }

  /// Replaces the theme (colors, shapes, fonts) of the calculator.
  void setTheme(const CalculatorTheme<RGB_T>& theme) {
    theme_ = theme;
    applyTheme();
  }

  /// The theme in use.
  const CalculatorTheme<RGB_T>& theme() const { return theme_; }

  /// Changes the area the calculator occupies (e.g. after a rotation). Works
  /// for any size: a wide area gets the display left and the keys right, a
  /// tall area stacks them.
  void setBounds(tinymd::Bounds bounds) {
    bounds_ = bounds;
    relayout();
    if (screen_ != nullptr) screen_->invalidate();
  }

  /// Range of x used by the Plot key (default -10 .. 10). Ignored unless xMax > xMin.
  void setPlotRange(float xMin, float xMax) {
    if (xMax > xMin) {
      plotMin_ = xMin;
      plotMax_ = xMax;
      syncRangeFields();
    }
  }

  /// True while the plot is shown instead of the keypad.
  bool plotting() const { return plotting_; }

  /// Text of the From and To fields of the plot.
  std::string plotFrom() const { return fromText_; }
  std::string plotTo() const { return toText_; }

  /// The y values of the last plot (kPlotSamples values, NaN where undefined).
  const std::vector<float>& plotSamples() const { return samples_; }

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
        {"phi", "phi"},   {"^", "^"},      {"Plot", "plot"}, {"x", "x"},
    };
    static const KeyDef units[kKeyCount] = {
        {"", "page"},     {"to", "to"},     {"m", "m"},       {"cm", "cm"},
        {"mm", "mm"},     {"km", "km"},     {"mi", "mi"},     {"ft", "ft"},
        {"in", "in"},     {"yd", "yd"},     {"kg", "kg"},     {"g", "g"},
        {"lb", "lb"},     {"oz", "oz"},     {"s", "s"},       {"min", "min"},
        {"h", "h"},       {"day", "day"},   {"degC", "degC"}, {"degF", "degF"},
        {"K", "K"},       {"f", "f("},      {"p", "p("},      {"q", "q("},
        {"r", "r("},      {"x", "x"},       {":=", "def"},    {"=", "="},
    };
    switch (page % kPageCount) {
      case 1: return trig;
      case 2: return math;
      case 3: return special;
      case 4: return units;
      default: return basic;
    }
  }

  /// Numeric pad of the plot screen: From/To select a field, the digits and
  /// "-" and "." type into it, DEL deletes, Back returns to the keypad.
  // The basic keypad with the Plot screen on: "Plot" and "x" replace "," and "pi".
  static const KeyDef* basicPlotKeys() {
    static const KeyDef basicPlot[kKeyCount] = {
        {"", "page"},     {"RAD", "angle"}, {"(", "("},      {")", ")"},
        {"C", "clear"},   {"DEL", "del"},   {"^", "^"},      {"%", "%"},
        {"7", "7"},       {"8", "8"},       {"9", "9"},      {"/", "/"},
        {"4", "4"},       {"5", "5"},       {"6", "6"},      {"*", "*"},
        {"1", "1"},       {"2", "2"},       {"3", "3"},      {"-", "-"},
        {"0", "0"},       {".", "."},       {"!", "!"},      {"+", "+"},
        {"ans", "ans"},   {"x", "x"},       {"Plot", "plot"}, {"=", "="},
    };
    return basicPlot;
  }

  static const KeyDef* plotKeys() {
    static const KeyDef pad[kKeyCount] = {
        {"From", "from"}, {"To", "to"},     {"DEL", "del"},  {"Back", "back"},
        {"7", "7"},       {"8", "8"},       {"9", "9"},      {"-", "-"},
        {"4", "4"},       {"5", "5"},       {"6", "6"},      {".", "."},
        {"1", "1"},       {"2", "2"},       {"3", "3"},      {"0", "0"},
    };
    return pad;
  }

  /// Name shown on the page key.
  static const char* pageName(size_t page) {
    static const char* names[kPageCount] = {"Basic", "Trig", "Math", "Special", "Units"};
    return names[page % kPageCount];
  }

 private:
  static const int32_t kMargin = 8;     // outer margin
  static const int32_t kGap = 4;        // gap between keys and fields
  static const int32_t kExprH = 20;     // expression line (at most)
  static const int32_t kFieldH = 34;    // From / To fields (at most)

  // Positions all widgets for the current size (bounds_) and mode.
  //
  // Wide areas (landscape) put the display on the left and the keys on the
  // right; tall areas (portrait) stack the display above the keys. The keys
  // use the space the display does not need, so the keypad fills the area.
  void relayout() {
    panel_.bounds = bounds_;
    const int32_t x = bounds_.x, y = bounds_.y, w = bounds_.w, h = bounds_.h;
    const bool landscape = w > h;
    tinymd::Bounds display, pad;
    if (landscape) {
      const int32_t dw = (w - 2 * kMargin) / 2;
      display = tinymd::Bounds(x + kMargin, y + kMargin, dw, h - 2 * kMargin);
      pad = tinymd::Bounds(x + kMargin + dw + kGap, y + kMargin,
                           w - 2 * kMargin - dw - kGap, h - 2 * kMargin);
    } else {
      // the plot needs more height than the expression and result lines
      const int32_t dh = plotting_ ? (h * 11) / 20 : (h * 3) / 10;
      display = tinymd::Bounds(x + kMargin, y + kMargin, w - 2 * kMargin, dh);
      pad = tinymd::Bounds(x + kMargin, y + kMargin + dh + kGap,
                           w - 2 * kMargin, h - 2 * kMargin - dh - kGap);
    }

    if (plotting_) {
      const int32_t fh = min32(kFieldH, display.h / 4);
      // room for the field labels ("From", "To"), which are drawn above the fields
      const int32_t labelH = 18;
      plot_.bounds = tinymd::Bounds(display.x, display.y, display.w,
                                    display.h - fh - labelH - kGap);
      const int32_t fw = (display.w - kGap) / 2;
      const int32_t fy = display.y + display.h - fh;
      fromField_.bounds = tinymd::Bounds(display.x, fy, fw, fh);
      toField_.bounds = tinymd::Bounds(display.x + fw + kGap, fy, display.w - fw - kGap, fh);
      placeKeys(pad, 4);  // numeric pad
    } else {
      const int32_t eh = min32(kExprH, display.h / 3);
      exprLabel_.bounds = tinymd::Bounds(display.x, display.y, display.w, eh);
      resultLabel_.bounds = tinymd::Bounds(display.x, display.y + eh, display.w,
                                           display.h - eh);
      placeKeys(pad, 7);  // keypad
    }
  }

  static int32_t min32(int32_t a, int32_t b) { return a < b ? a : b; }

  // Places the key buttons in a grid of `rows` rows (4 columns) inside `area`.
  void placeKeys(tinymd::Bounds area, int32_t rows) {
    const int32_t cols = 4;
    int32_t keyW = (area.w - (cols - 1) * kGap) / cols;
    int32_t keyH = (area.h - (rows - 1) * kGap) / rows;
    if (keyW < 1) keyW = 1;
    if (keyH < 1) keyH = 1;
    for (size_t i = 0; i < kKeyCount; ++i) {
      keys_[i].onClick = [this, i]() { press(activeKeys()[i].action); };
      int32_t row = static_cast<int32_t>(i) / cols;
      int32_t col = static_cast<int32_t>(i) % cols;
      if (row >= rows) {
        // not on the grid: zero size, so hidden keys do not extend the scroll area
        keys_[i].bounds = tinymd::Bounds(bounds_.x, bounds_.y, 0, 0);
        continue;
      }
      keys_[i].bounds = tinymd::Bounds(area.x + col * (keyW + kGap),
                                       area.y + row * (keyH + kGap), keyW, keyH);
    }
  }

  // The key table shown now: the plot pad or the keypad page.
  const KeyDef* activeKeys() const {
    if (plotting_) return plotKeys();
    if (page_ == 0 && plotEnabled_) return basicPlotKeys();
    return keys(page_);
  }

  // Shows the keypad page.
  void refreshKeys() { showKeys(activeKeys()); }

  void showKeys(const KeyDef* defs) {
    for (size_t i = 0; i < kKeyCount; ++i) {
      const bool used = defs[i].action != nullptr && *defs[i].action != '\0' &&
                        !hiddenByScreens(defs[i].action);
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
    if (a == "angle" || a == "page" || a == "clear" || a == "del" || a == "back") {
      return tinymd::ButtonVariant::kText;
    }
    if (a.size() == 1 && (a[0] >= '0' && a[0] <= '9')) return tinymd::ButtonVariant::kOutlined;
    if (a == "." || a == "-") return tinymd::ButtonVariant::kOutlined;
    return tinymd::ButtonVariant::kTonal;
  }

  void toggleAngle() {
    bool deg = calc_.angleUnit() == AngleUnit::Degrees;
    calc_.setAngleUnit(deg ? AngleUnit::Radians : AngleUnit::Degrees);
    refreshKeys();  // updates the RAD/DEG label
  }

  void evaluate() {
    if (expr_.empty()) return;
    if (Calc::isDefinition(expr_.c_str())) {  // "f(x) = ...": defines a function
      typename Calc::Result r = calc_.evaluate(expr_.c_str());
      showResult_ = r.ok();
      if (r.ok()) {
        resultText_ = expr_.substr(0, expr_.find('(')) + " defined";
      } else {
        error_ = errorText(r.error);
      }
      return;
    }
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

  static std::string formatRange(float v) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%g", static_cast<double>(v));
    return buf;
  }

  // Copies the current range into the From/To texts and fields.
  void syncRangeFields() {
    fromText_ = formatRange(plotMin_);
    toText_ = formatRange(plotMax_);
    fromField_.setText(fromText_.c_str());
    toField_.setText(toText_.c_str());
  }

  // Parses a whole number text; false if empty or not a number.
  static bool parseNumber(const std::string& text, float& out) {
    if (text.empty() || text == "-" || text == ".") return false;
    char* end = nullptr;
    out = strtof(text.c_str(), &end);
    return end != nullptr && *end == '\0';
  }

  // Keys of the plot pad. The From/To fields report focus through onFocus.
  void handlePlotKey(const std::string& a) {
    if (a == "back") {
      closePlot();
      return;
    }
    if (a == "from") {
      fromField_.focus();
      return;
    }
    if (a == "to") {
      toField_.focus();
      return;
    }
    std::string& text = activeField_ == 0 ? fromText_ : toText_;
    if (a == "del") {
      if (!text.empty()) text.erase(text.size() - 1);
    } else if (a == "-") {
      if (text.empty()) text = "-";  // a minus sign only at the start
    } else if (a == ".") {
      if (text.find('.') == std::string::npos) text += ".";
    } else if (a.size() == 1 && a[0] >= '0' && a[0] <= '9') {
      if (text.size() < 9) text += a;
    } else {
      return;
    }
    fromField_.setText(fromText_.c_str());
    toField_.setText(toText_.c_str());
    applyRangeText();
  }

  // Redraws the plot if both fields hold a valid range (To above From).
  void applyRangeText() {
    float lo = 0.0f, hi = 0.0f;
    if (!parseNumber(fromText_, lo) || !parseNumber(toText_, hi) || hi <= lo) return;
    plotMin_ = lo;
    plotMax_ = hi;
    samplePlot();  // the expression compiled when the plot opened
  }

  // Samples the expression as a function of x over [plotMin_, plotMax_] and
  // shows it in the chart. Points where the expression is undefined become
  // NaN, which the chart draws as a gap.
  Error samplePlot() {
    typename Calc::Program prog;  // compiled once, run for every sample
    Error e = calc_.compile(expr_.c_str(), prog);
    if (e != Error::None) return e;
    samples_.assign(kPlotSamples, nanValue());
    for (size_t i = 0; i < kPlotSamples; ++i) {
      double x = plotMin_ + (plotMax_ - plotMin_) * static_cast<double>(i) /
                                static_cast<double>(kPlotSamples - 1);
      calc_.setVariable("x", static_cast<Number>(x));
      typename Calc::Result r = calc_.run(prog);
      if (r.ok()) samples_[i] = static_cast<float>(r.value);
    }
    calc_.setVariable("x", Number(0));  // leave x at 0 for the keypad

    char footer[48];
    snprintf(footer, sizeof(footer), "x from %g to %g", static_cast<double>(plotMin_),
             static_cast<double>(plotMax_));
    std::string title = "f(x) = " + expr_;
    plot_.setTitle(title.c_str());
    plot_.setFooter(footer);
    plot_.setXAxis(plotMin_, plotMax_, 5);
    plot_.setValues(0, samples_);
    return Error::None;
  }

  // Opens the plot screen for the current expression.
  void plotExpression() {
    if (!plotEnabled_) {
      error_ = "plotting is not enabled";
      return;
    }
    if (expr_.empty()) {
      error_ = "nothing to plot";
      return;
    }
    Error e = samplePlot();
    if (e != Error::None) {
      error_ = errorText(e);
      return;
    }
    plotting_ = true;
    syncRangeFields();
    fromField_.focus();  // selects the From field (activeField_ = 0)
    error_.clear();
    applyMode();
    refresh();
  }

  // Shows the enabled keypad page `step` pages away from the current one (wraps around).
  void changePage(int step) {
    page_ = nextPage(step);
    refreshKeys();
    refresh();
  }

  // The enabled page `step` steps away (step = +1 or -1); the Basic page is always enabled.
  size_t nextPage(int step) const {
    const int pages = static_cast<int>(kPageCount);
    size_t candidate = page_;
    for (int i = 0; i < pages; ++i) {
      candidate = static_cast<size_t>((static_cast<int>(candidate) + (step < 0 ? -1 : 1) + pages) % pages);
      if (candidate == 0 || (pages_ & (1u << candidate)) != 0) return candidate;
    }
    return 0;
  }

  // The keypad page of a screen (Plot has no page of its own).
  static size_t pageOf(GuiScreen screen) {
    switch (screen) {
      case GuiScreen::Trig: return 1;
      case GuiScreen::Math: return 2;
      case GuiScreen::Special: return 3;
      case GuiScreen::Units: return 4;
      default: return 0;
    }
  }

  // Keys that exist only with some screens switched on: the page key needs an
  // extra page, the Plot key needs the Plot screen.
  bool hiddenByScreens(const char* action) const {
    if (strcmp(action, "page") == 0) return pages_ == 0;
    if (strcmp(action, "plot") == 0) return !plotEnabled_;
    return false;
  }

  // Left swipe: leaves the plot screen, otherwise goes back one page.
  void swipeLeft() {
    if (plotting_) {
      closePlot();
    } else {
      changePage(-1);
    }
  }

  // Right swipe: goes forward one page (the plot screen has no pages).
  void swipeRight() {
    if (!plotting_) changePage(1);
  }

  void closePlot() {
    plotting_ = false;
    fromField_.blur();
    toField_.blur();
    applyMode();
    refresh();
  }

  // Shows the keypad or the plot, depending on plotting_.
  void applyMode() {
    if (plotting_) {
      showKeys(plotKeys());
    } else {
      refreshKeys();
    }
    relayout();
    exprLabel_.visible = !plotting_;
    resultLabel_.visible = !plotting_;
    plot_.visible = plotting_;
    fromField_.visible = plotting_;
    toField_.visible = plotting_;
  }

  static float nanValue() {
    volatile float zero = 0.0f;
    return zero / zero;
  }

  // Pushes the theme to the attached screen and the plot color.
  void applyTheme() {
    plot_.series(0).color = theme_.colors.primary;
    dimTheme_ = fadedTheme(theme_);
    if (screen_ != nullptr) screen_->setTheme(theme_);
    refresh();  // also updates the unit keys
  }

  // Unit keys that cannot follow the expression are faded out: they keep the
  // background colour of their theme but are drawn with the surface as the
  // "on-surface" colour, so they are lighter. press() refuses them.
  void updateUnitKeys() {
    const KeyDef* defs = activeKeys();
    for (size_t i = 0; i < kKeyCount; ++i) {
      const bool isUnit = defs[i].action != nullptr && isUnitName(defs[i].action);
      const bool allowed = !isUnit || unitAllowed(defs[i].action, expr_);
      keys_[i].enabled = true;  // drawn normally; the faded theme shows the state
      keys_[i].setTheme(allowed ? theme_ : dimTheme_);
      faded_[i] = !allowed;
    }
  }

  // Mixes colour c towards `toward` by a (0 = c, 1 = toward).
  static RGB_T fade(RGB_T c, RGB_T toward, float a) {
    auto mix = [a](int x, int y) {
      return static_cast<uint8_t>(x + (y - x) * a + 0.5f);
    };
    return RGB_T::fromRGB(mix(c.getRed(), toward.getRed()), mix(c.getGreen(), toward.getGreen()),
                          mix(c.getBlue(), toward.getBlue()));
  }

  // The theme of a faded unit key: its tonal background and text are lighter,
  // keeping their colour, so the key still shows its background colour.
  static CalculatorTheme<RGB_T> fadedTheme(const CalculatorTheme<RGB_T>& t) {
    CalculatorTheme<RGB_T> d = t;
    d.colors.secondaryContainer = fade(t.colors.secondaryContainer, t.colors.surface, 0.6f);
    d.colors.onSecondaryContainer = fade(t.colors.onSecondaryContainer, t.colors.surface, 0.5f);
    return d;
  }

  // True if the key in slot `index` is drawn faded (a unit that cannot be used now).
  bool keyFaded(size_t index) const { return faded_[index]; }

  void refresh() {
    std::string e = showResult_ ? expr_ + " =" : expr_;
    exprLabel_.setText(e.c_str());
    resultLabel_.setText(resultText().c_str());
    updateUnitKeys();
    if (screen_ != nullptr) screen_->invalidate();
  }

  tinymd::Bounds bounds_;
  CalculatorTheme<RGB_T> theme_;
  CalculatorTheme<RGB_T> dimTheme_;  // theme of the unit keys that cannot be used now
  tinymd::Screen<RGB_T>* screen_;
  Calc calc_;
  std::string expr_;
  std::string resultText_;
  std::string error_;
  bool showResult_ = false;
  size_t page_ = 0;
  tinymd::Label<RGB_T> exprLabel_;
  tinymd::Label<RGB_T> resultLabel_;
  CalculatorPanel<RGB_T> panel_;
  tinymd::Button<RGB_T> keys_[kKeyCount];
  bool faded_[kKeyCount] = {};  // per key: drawn faded (see updateUnitKeys())
  tinymd::LineChart<RGB_T> plot_;
  tinymd::TextField<RGB_T> fromField_;
  tinymd::TextField<RGB_T> toField_;
  std::string fromText_;
  std::string toText_;
  int activeField_ = 0;  // 0: From, 1: To
  std::vector<float> samples_;
  float plotMin_ = -10.0f;
  float plotMax_ = 10.0f;
  bool plotting_ = false;
  bool attached_ = false;
  uint8_t pages_ = 0;         // bit p: the keypad page p is enabled
  bool plotEnabled_ = false;  // the plot screen is enabled
};

}  // namespace TinyCalc
