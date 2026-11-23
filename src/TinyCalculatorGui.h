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
#include <string.h>
#include <string>
#include <vector>

#include "TinyCalculator.h"
#include "TinyMaterialDesign.h"

namespace TinyCalc {

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
  static const size_t kPageCount = 4;
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
  explicit CalculatorGui(tinymd::Bounds bounds)
      : screen_(nullptr),
        exprLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kLabel),
        resultLabel_(tinymd::Bounds(), "", tinymd::TypographyRole::kHeadline),
        fromField_(tinymd::Bounds(), "From"),
        toField_(tinymd::Bounds(), "To") {
    bounds_ = bounds;
    relayout();
    plot_.addSeries(colorFromHex<RGB_T>(0x6750A4));
    fromField_.onFocus = [this]() { activeField_ = 0; };
    toField_.onFocus = [this]() { activeField_ = 1; };
    syncRangeFields();
    calc_.setVariable("x", Number(0));
    applyMode();
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
    screen.addWidget(exprLabel_);
    screen.addWidget(resultLabel_);
    for (size_t i = 0; i < kKeyCount; ++i) screen.addWidget(keys_[i]);
    screen.addWidget(plot_);
    screen.addWidget(fromField_);
    screen.addWidget(toField_);
    attached_ = true;
    refresh();
  }

  /// Removes the widgets from the screen given to begin(). The GUI keeps its
  /// state, so begin() can attach it again later. Does nothing if not attached.
  void end() {
    if (!attached_) return;
    screen_->removeChild(exprLabel_);
    screen_->removeChild(resultLabel_);
    for (size_t i = 0; i < kKeyCount; ++i) screen_->removeChild(keys_[i]);
    screen_->removeChild(plot_);
    screen_->removeChild(fromField_);
    screen_->removeChild(toField_);
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
      page_ = (page_ + 1) % kPageCount;
      refreshKeys();
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

  /// The button of key `index` (0 .. kKeyCount-1) of the shown table, e.g.
  /// to fire its onClick in tests.
  tinymd::Button<RGB_T>& key(size_t index) { return keys_[index]; }

  /// Index of the current keypad page (0 .. kPageCount-1).
  size_t page() const { return page_; }

  /// The area the calculator occupies.
  tinymd::Bounds bounds() const { return bounds_; }

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
    switch (page % kPageCount) {
      case 1: return trig;
      case 2: return math;
      case 3: return special;
      default: return basic;
    }
  }

  /// Numeric pad of the plot screen: From/To select a field, the digits and
  /// "-" and "." type into it, DEL deletes, Back returns to the keypad.
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
    static const char* names[kPageCount] = {"Basic", "Trig", "Math", "Special"};
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
      const int32_t dh = plotting_ ? h / 2 : (h * 3) / 10;
      display = tinymd::Bounds(x + kMargin, y + kMargin, w - 2 * kMargin, dh);
      pad = tinymd::Bounds(x + kMargin, y + kMargin + dh + kGap,
                           w - 2 * kMargin, h - 2 * kMargin - dh - kGap);
    }

    if (plotting_) {
      const int32_t fh = min32(kFieldH, display.h / 4);
      plot_.bounds = tinymd::Bounds(display.x, display.y, display.w,
                                    display.h - fh - kGap);
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
  const KeyDef* activeKeys() const { return plotting_ ? plotKeys() : keys(page_); }

  // Shows the keypad page.
  void refreshKeys() { showKeys(keys(page_)); }

  void showKeys(const KeyDef* defs) {
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

  void refresh() {
    std::string e = showResult_ ? expr_ + " =" : expr_;
    exprLabel_.setText(e.c_str());
    resultLabel_.setText(resultText().c_str());
    if (screen_ != nullptr) screen_->invalidate();
  }

  tinymd::Bounds bounds_;
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
};

}  // namespace TinyCalc
