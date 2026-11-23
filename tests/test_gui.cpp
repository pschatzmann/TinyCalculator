// Desktop tests for the optional TinyMaterialDesign GUI (CalculatorGui).
// The key logic is exercised through press() and through each key button's
// onClick handler; the widgets are also attached to a screen and drawn.

#include <TinyCalculatorGui.h>

#include <cmath>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <set>
#include <string>

using tinymd::TINYMD_DEFAULT_RGB_T;
typedef CalculatorGui<TINYMD_DEFAULT_RGB_T, float> FloatGui;
typedef CalculatorGui<TINYMD_DEFAULT_RGB_T, double> DoubleGui;

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

template <typename Gui>
void type(Gui& gui, std::initializer_list<const char*> actions) {
  for (const char* a : actions) gui.press(a);
}

// index of the key with the given action on the basic page, or kKeyCount if none
static size_t keyIndex(const char* action) {
  for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
    const char* a = FloatGui::keys(0)[i].action;
    if (a != nullptr && std::strcmp(a, action) == 0) return i;
  }
  return FloatGui::kKeyCount;
}

static void testTypingAndEvaluation() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"2", "+", "3"});
  CHECK(gui.expression() == "2+3");
  CHECK(gui.resultText().empty());  // nothing is evaluated before "="

  gui.press("=");
  CHECK(gui.resultText() == "5");
  CHECK(gui.expression() == "2+3");

  // an operator after "=" continues from ans
  gui.press("*");
  gui.press("2");
  CHECK(gui.expression() == "ans*2");
  gui.press("=");
  CHECK(gui.resultText() == "10");

  // a digit after "=" starts a new expression
  gui.press("7");
  CHECK(gui.expression() == "7");
  CHECK(gui.resultText().empty());
}

static void testIncompleteExpressionShowsNoResult() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"2", "+"});
  CHECK(gui.expression() == "2+");
  gui.press("=");  // "=" on an incomplete expression reports an error
  CHECK(gui.resultText() == "unexpected end of expression");
}

static void testErrorsAreReported() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"1", "/", "0"});
  CHECK(gui.resultText().empty());  // not evaluated until "="
  gui.press("=");
  CHECK(gui.resultText() == "division by zero");
  CHECK(gui.expression() == "1/0");  // kept so the user can correct it

  gui.press("7");  // typing clears the error
  CHECK(gui.expression() == "1/07");
  CHECK(gui.resultText().empty());
  gui.press("=");
  CHECK(gui.resultText() == "0.1428571");  // 1/07 = 1/7, 7 significant digits
}

static void testClearAndDelete() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"1", "2", "3"});
  gui.press("del");
  CHECK(gui.expression() == "12");
  gui.press("clear");
  CHECK(gui.expression().empty());
  CHECK(gui.resultText().empty());

  gui.press("del");  // deleting on an empty expression is a no-op
  CHECK(gui.expression().empty());
}

static void testFunctionsAndAngleUnit() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"sqrt(", "1", "6", ")"});
  gui.press("=");
  CHECK(gui.resultText() == "4");

  gui.press("clear");
  CHECK(gui.calculator().angleUnit() == TinyCalc::AngleUnit::Radians);
  gui.press("angle");
  CHECK(gui.calculator().angleUnit() == TinyCalc::AngleUnit::Degrees);
  type(gui, {"sin(", "9", "0", ")"});
  gui.press("=");
  CHECK(gui.resultText() == "1");

  gui.press("angle");
  CHECK(gui.calculator().angleUnit() == TinyCalc::AngleUnit::Radians);
}

static void testKeyButtonsAreWired() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
    CHECK(static_cast<bool>(gui.key(i).onClick));
  }
  // click "2+3=" through the buttons themselves
  const char* seq[] = {"2", "+", "3", "="};
  for (const char* a : seq) {
    size_t i = keyIndex(a);
    CHECK(i < FloatGui::kKeyCount);
    if (i < FloatGui::kKeyCount) gui.key(i).onClick();
  }
  CHECK(gui.resultText() == "5");
}

static void testEveryKeyCanBePressed() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  for (size_t p = 0; p < FloatGui::kPageCount; ++p) {
    for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
      const char* action = FloatGui::keys(p)[i].action;
      if (action == nullptr) continue;  // unused slot
      CHECK(std::strlen(action) > 0);
      if (std::strcmp(action, "page") == 0) continue;
      gui.press(action);
    }
  }
  gui.press("back");
  gui.press("clear");
  CHECK(gui.expression().empty());
}

static void testPagesCycle() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  CHECK(gui.page() == 0);
  gui.press("page");
  CHECK(gui.page() == 1);
  for (size_t i = 0; i < FloatGui::kPageCount - 1; ++i) gui.press("page");
  CHECK(gui.page() == 0);  // wrapped around
}

// every function and constant of the parser has a key on some page
static void testAllFunctionsHaveKeys() {
  std::set<std::string> actions;
  for (size_t p = 0; p < FloatGui::kPageCount; ++p) {
    for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
      const char* a = FloatGui::keys(p)[i].action;
      if (a != nullptr) actions.insert(a);
    }
  }
  TinyCalc::FunctionInfo info;
  for (size_t i = 0; TinyCalc::functionAt(i, info); ++i) {
    CHECK(actions.count(std::string(info.name) + "(") == 1);
  }
  for (const char* c : {"pi", "e", "tau", "phi", "ans"}) {
    CHECK(actions.count(c) == 1);
  }
}

// a function key enters a usable call: "sqrt(" + argument + ")" evaluates
static void testFunctionKeysEvaluate() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"sqrt(", "1", "6", ")", "="});
  CHECK(gui.resultText() == "4");
  gui.press("clear");
  type(gui, {"min(", "3", ",", "1", ",", "2", ")", "="});
  CHECK(gui.resultText() == "1");
  gui.press("clear");
  type(gui, {"gcd(", "12", ",", "18", ")", "="});
  CHECK(gui.resultText() == "6");
}

// the Plot key samples x^2 over the default range -10 .. 10
static void testPlotSamplesFormula() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"x", "^", "2"});
  gui.press("plot");
  CHECK(gui.plotting());
  const std::vector<float>& s = gui.plotSamples();
  CHECK(s.size() == FloatGui::kPlotSamples);
  CHECK(std::fabs(s.front() - 100.0f) < 1e-3f);  // x = -10
  CHECK(std::fabs(s.back() - 100.0f) < 1e-3f);   // x = 10
  CHECK(std::fabs(s[s.size() / 2]) < 0.1f);      // x near 0
  CHECK(gui.expression() == "x^2");              // expression is kept
  CHECK(gui.calculator().variableCount() >= 1);
  gui.press("back");
  CHECK(!gui.plotting());
}

// undefined points (sqrt of a negative x) are NaN, which the chart draws as a gap
static void testPlotUndefinedPoints() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.setPlotRange(-4.0f, 4.0f);
  type(gui, {"sqrt(", "x", ")"});
  gui.press("plot");
  const std::vector<float>& s = gui.plotSamples();
  CHECK(std::isnan(s.front()));                 // x = -4
  CHECK(std::fabs(s.back() - 2.0f) < 1e-4f);    // x = 4
  gui.press("back");
}

static void testPlotErrors() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.press("plot");                            // nothing typed
  CHECK(!gui.plotting());
  CHECK(gui.resultText() == "nothing to plot");
  gui.press("clear");
  type(gui, {"2", "+"});
  gui.press("plot");                            // incomplete formula
  CHECK(!gui.plotting());
  CHECK(gui.resultText() == "unexpected end of expression");
}

static void testPlotRangeIgnoresInvalid() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.setPlotRange(5.0f, 5.0f);                 // empty range: ignored
  type(gui, {"x"});
  gui.press("plot");
  CHECK(std::fabs(gui.plotSamples().front() + 10.0f) < 1e-4f);  // still -10 .. 10
  gui.press("back");
}

// From and To entry: the numeric pad types into the selected field and the
// chart is redrawn when both fields hold a valid range
static void testPlotRangeEntry() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"x", "^", "2"});
  gui.press("plot");
  CHECK(gui.plotFrom() == "-10" && gui.plotTo() == "10");
  CHECK(std::fabs(gui.plotSamples().back() - 100.0f) < 1e-3f);

  gui.press("to");                              // select To
  gui.press("del");                             // "10" -> "1"
  gui.press("5");                               // "15"
  CHECK(gui.plotTo() == "15");
  CHECK(std::fabs(gui.plotSamples().back() - 225.0f) < 1e-2f);  // x = 15

  gui.press("from");                            // select From
  gui.press("del");
  gui.press("del");
  gui.press("del");                             // "" is not a valid range
  CHECK(gui.plotFrom().empty());
  CHECK(std::fabs(gui.plotSamples().back() - 225.0f) < 1e-2f);  // chart unchanged
  gui.press("0");                               // range 0 .. 15
  CHECK(std::fabs(gui.plotSamples().front()) < 1e-3f);          // x = 0
  gui.press("back");
  CHECK(!gui.plotting());
}

// a range with To not above From is not applied
static void testPlotRangeRejectsReversed() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"x"});
  gui.press("plot");
  gui.press("from");
  gui.press("del");
  gui.press("del");
  gui.press("del");                             // "-10" -> ""
  gui.press("9");                               // From = 9: valid, chart shows 9 .. 10
  CHECK(std::fabs(gui.plotSamples().front() - 9.0f) < 1e-4f);
  gui.press("9");                               // From = 99 > To = 10: rejected
  CHECK(gui.plotFrom() == "99");
  CHECK(std::fabs(gui.plotSamples().front() - 9.0f) < 1e-4f);  // last valid range kept
  gui.press("back");
}

static bool overlaps(const tinymd::Bounds& a, const tinymd::Bounds& b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

// the layout works for portrait, landscape and square areas, in keypad and plot mode
static void testLayoutAnySize() {
  const int sizes[][2] = {{240, 320}, {320, 240}, {480, 320}, {320, 480},
                          {160, 128}, {128, 160}, {400, 240}, {240, 240}};
  for (const auto& sz : sizes) {
    FloatGui gui(tinymd::Bounds(0, 0, sz[0], sz[1]));
  gui.enableAll();
    const tinymd::Bounds area = gui.bounds();
    for (int mode = 0; mode < 2; ++mode) {
      if (mode == 1) gui.beginPlot("x");
      size_t visible = 0;
      for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
        const tinymd::Bounds& k = gui.key(i).bounds;
        if (!gui.key(i).visible) continue;
        ++visible;
        CHECK(k.w > 0 && k.h > 0);
        CHECK(k.x >= area.x && k.y >= area.y);
        CHECK(k.x + k.w <= area.x + area.w && k.y + k.h <= area.y + area.h);
        for (size_t j = 0; j < i; ++j) {
          if (gui.key(j).visible) CHECK(!overlaps(k, gui.key(j).bounds));
        }
      }
      CHECK(visible == (mode == 1 ? 16u : 28u));
      gui.endPlot();
    }
  }
}

// setBounds() re-lays out the keys: landscape puts them on the right half
static void testSetBounds() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  const int portraitX = gui.key(7).bounds.x;
  CHECK(gui.key(0).bounds.y >= 100);            // keypad below the display
  gui.setBounds(tinymd::Bounds(0, 0, 320, 240));
  CHECK(gui.bounds().w == 320 && gui.bounds().h == 240);
  CHECK(gui.key(7).bounds.x > 160);             // right half
  CHECK(gui.key(7).bounds.x != portraitX || gui.key(7).bounds.y < 120);
  gui.setBounds(tinymd::Bounds(0, 0, 240, 320));
  CHECK(gui.key(0).bounds.y >= 100);            // back to portrait
}

static tinygpu::GestureEvent swipe(tinygpu::GestureType type, int x, int y) {
  tinygpu::GestureEvent e;
  e.type = type;
  e.phase = tinygpu::GesturePhase::kEnded;
  e.point.x = static_cast<int16_t>(x);
  e.point.y = static_cast<int16_t>(y);
  e.startPoint = e.point;
  return e;
}

// right swipe: next page; left swipe: previous page (wraps around)
static void testSwipePages() {
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.begin(screen);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeRight, 120, 14));  // empty area
  CHECK(gui.page() == 1);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeRight, 120, 14));
  CHECK(gui.page() == 2);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeLeft, 120, 14));
  CHECK(gui.page() == 1);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeLeft, 120, 14));
  CHECK(gui.page() == 0);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeLeft, 120, 14));
  CHECK(gui.page() == 4);                       // wraps to the last page (Units)

  // a swipe that starts on a key still changes the page
  const tinymd::Bounds k = gui.key(10).bounds;
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeRight,
                             k.x + k.w / 2, k.y + k.h / 2));
  CHECK(gui.page() == 0);
  gui.end();
}

// on the plot screen a right swipe does nothing, a left swipe returns to the keypad
static void testSwipeInPlot() {
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.begin(screen);
  gui.beginPlot("x");
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeRight, 120, 14));
  CHECK(gui.plotting());
  CHECK(gui.page() == 0);
  screen.handleGesture(swipe(tinygpu::GestureType::kSwipeLeft, 120, 14));
  CHECK(!gui.plotting());
  CHECK(gui.page() == 0);                       // closing the plot does not change the page
  gui.end();
}

// invalid key combinations are ignored; valid ones are kept
static void testInputRules() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"2", "+", "+"});  CHECK(gui.expression() == "2+");   // operator after operator
  type(gui, {"*", "/"});       CHECK(gui.expression() == "2+");   // "*" and "/" after "+" are ignored
  gui.press("clear");
  type(gui, {"2", "*", "/"});  CHECK(gui.expression() == "2*");
  gui.press("clear");
  type(gui, {"+", "*", ")"});  CHECK(gui.expression().empty());   // nothing valid at the start
  type(gui, {"-"});            CHECK(gui.expression() == "-");     // a sign is allowed
  gui.press("clear");
  type(gui, {"2", ")", "!", ","});  CHECK(gui.expression() == "2!");  // ")" and "," ignored, "!" valid
  gui.press("(");              CHECK(gui.expression() == "2!*(");  // "(" after an operand: product
  gui.press("clear");
  type(gui, {"(", "("});
  CHECK(gui.expression() == "((");
  type(gui, {"-", "3", ")", ")", "="});
  CHECK(gui.resultText() == "-3");
  gui.press("clear");
  type(gui, {"2", ".", ".", "5", "."});
  CHECK(gui.expression() == "2.5");              // a second "." in a number is ignored
  gui.press("clear");
  type(gui, {"2", "+", "-", "3", "="});
  CHECK(gui.resultText() == "-1");              // "+" then a sign is valid
  gui.press("clear");

  // a name followed by a digit or a function gets an explicit "*"
  type(gui, {"pi", "2"});  CHECK(gui.expression() == "pi*2");
  gui.press("clear");
  type(gui, {"pi", "sin(", "0", ")", "="});
  CHECK(gui.expression() == "pi*sin(0)");
  CHECK(gui.resultText() == "0");
  gui.press("clear");
  type(gui, {"2", "pi", "="});
  CHECK(std::fabs(std::atof(gui.resultText().c_str()) - 2.0 * 3.14159265) < 1e-3);

  // after "=", a key that cannot start an expression leaves the result on screen
  type(gui, {"2", "+", "3", "="});
  gui.press(")");
  CHECK(gui.resultText() == "5");
  CHECK(gui.expression() == "2+3");
  gui.press("*");
  CHECK(gui.expression() == "ans*");            // an operator continues with ans
  gui.press("clear");
}

#ifdef TINYCALC_USE_TINYFONTS
#include <TinyFontsGPU.h>

// the sketch-style font function: assigns TinyFonts to the theme's typography
static tinyfonts::TinyGPUFont<TINYMD_DEFAULT_RGB_T> g_small(tinyfonts::Lato16);
static tinyfonts::TinyGPUFont<TINYMD_DEFAULT_RGB_T> g_big(tinyfonts::Lato32);
static void sketchFonts(CalculatorTheme<TINYMD_DEFAULT_RGB_T>& t) {
  t.typography.label = &g_small;
  t.typography.headline = &g_big;
}

// TinyFonts assigned through a function: the calculator still works and draws
static void testTinyFonts() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.assignFonts(sketchFonts);
  CHECK(gui.theme().typography.label == &g_small);
  CHECK(gui.theme().typography.headline == &g_big);
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  gui.begin(screen);
  CHECK(screen.theme().typography.label == &g_small);  // the screen got the theme
  type(gui, {"2", "+", "3", "="});
  CHECK(gui.resultText() == "5");
  tinygpu::Surface<TINYMD_DEFAULT_RGB_T> surface(240, 320, tinygpu::FontRGB565);
  CHECK(surface.begin());
  screen.draw(surface);
  gui.end();
}
#endif

// themes: a theme is applied to the screen and the plot, and the calculator draws with it
static void testThemes() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  CalculatorTheme<TINYMD_DEFAULT_RGB_T> dark = tinymd::defaultDarkTheme<TINYMD_DEFAULT_RGB_T>();
  CalculatorTheme<TINYMD_DEFAULT_RGB_T> green = tinymd::greenTheme<TINYMD_DEFAULT_RGB_T>();
  gui.setTheme(dark);
  CHECK(gui.theme().colors.primary.getRed() == dark.colors.primary.getRed());
  gui.setTheme(green);
  CHECK(gui.theme().colors.primary.getRed() == green.colors.primary.getRed());
  CHECK(gui.theme().colors.primary.getGreen() == green.colors.primary.getGreen());

  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  gui.begin(screen);
  CHECK(screen.theme().colors.primary.getGreen() == green.colors.primary.getGreen());
  gui.setTheme(dark);                           // changing the theme while attached
  CHECK(screen.theme().colors.primary.getRed() == dark.colors.primary.getRed());
  gui.end();

  // a default-constructed GUI uses the default theme
  FloatGui plain(tinymd::Bounds(0, 0, 240, 320));
  plain.enableAll();
  CHECK(plain.theme().colors.primary.getRed() ==
        tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>().colors.primary.getRed());
}


// units, temperature functions and user functions entered with the keypad
static void testUnitsAndFunctionKeys() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"5", "km", "to", "m"});
  CHECK(gui.expression() == "5km to m");
  gui.press("=");
  CHECK(gui.resultText() == "5000");
  gui.press("clear");
  type(gui, {"1", "0", "0", "degC", "to", "degF", "="});
  CHECK(gui.resultText() == "212");
  gui.press("clear");

  // f(x) := x^2 defines f; "=" then reports it, and f can be called
  type(gui, {"f(", "x", ")", "def", "x", "^", "2"});
  CHECK(gui.expression() == "f(x)=x^2");
  gui.press("=");
  CHECK(gui.resultText() == "f defined");
  gui.press("clear");
  type(gui, {"f(", "3", ")", "="});
  CHECK(gui.resultText() == "9");

  // ":=" is only valid right after a function header
  gui.press("clear");
  type(gui, {"2", "def"});
  CHECK(gui.expression() == "2");
  gui.press("clear");
}

// by default only the basic keypad is shown; the other screens are enabled one by one
static void testBasicByDefault() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  CHECK(!gui.key(0).visible);                   // no page key
  size_t visible = 0;
  for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
    if (gui.key(i).visible) ++visible;
  }
  CHECK(visible == 27);
  gui.press("page");
  CHECK(gui.page() == 0);
  CHECK(!gui.isEnabled(GuiScreen::Trig) && !gui.isEnabled(GuiScreen::Plot));
  gui.press("plot");                            // the plot screen is off
  CHECK(!gui.plotting());
  CHECK(gui.resultText() == "plotting is not enabled");

  gui.enable(GuiScreen::Trig);
  CHECK(gui.isEnabled(GuiScreen::Trig));
  CHECK(gui.key(0).visible);                    // the page key appears
  gui.press("page");
  CHECK(gui.page() == 1);                       // Trig
  gui.press("page");
  CHECK(gui.page() == 0);                       // Math is off, so back to Basic
  gui.enable(GuiScreen::Plot);
  CHECK(gui.beginPlot("x"));
  gui.endPlot();
}

// index of a key with the given action on the Units page
static size_t unitKey(const char* action) {
  for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
    const char* a = FloatGui::keys(4)[i].action;
    if (a != nullptr && std::strcmp(a, action) == 0) return i;
  }
  return FloatGui::kKeyCount;
}

// after "to" only the units of the source's kind stay enabled
static void testUnitKeysAfterTo() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  for (int i = 0; i < 4; ++i) gui.press("page");
  CHECK(gui.page() == 4);
  const size_t km = unitKey("km"), m = unitKey("m"), kg = unitKey("kg");
  const size_t s = unitKey("s"), degF = unitKey("degF"), lb = unitKey("lb");

  type(gui, {"5", "km"});
  CHECK(!gui.isKeyFaded(kg) && !gui.isKeyFaded(m));       // before "to": all units
  gui.press("to");
  CHECK(gui.expression() == "5km to ");
  CHECK(!gui.isKeyFaded(m) && !gui.isKeyFaded(km));       // length
  CHECK(gui.isKeyFaded(kg) && gui.isKeyFaded(s) && gui.isKeyFaded(degF));
  gui.press("kg");                                        // refused: not a length
  CHECK(gui.expression() == "5km to ");
  gui.press("m");
  CHECK(gui.expression() == "5km to m");
  CHECK(gui.isKeyFaded(km) && gui.isKeyFaded(kg));    // conversion complete
  gui.press("=");
  CHECK(gui.resultText() == "5000");

  gui.press("clear");
  type(gui, {"5", "kg", "to"});
  CHECK(!gui.isKeyFaded(lb) && gui.isKeyFaded(m));      // mass
  gui.press("clear");
  type(gui, {"100", "degC", "to"});
  CHECK(!gui.isKeyFaded(degF) && gui.isKeyFaded(km));   // temperature
  gui.press("clear");
  gui.press("to");                                         // no unit before "to"
  CHECK(gui.expression().empty());
}

static void testDoublePrecision() {
  DoubleGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  type(gui, {"1", "/", "3"});
  gui.press("=");
  CHECK(gui.resultText() == "0.333333333333");  // 12 significant digits
}

static void testAttachAndDraw() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  gui.begin(screen);

  tinygpu::Surface<TINYMD_DEFAULT_RGB_T> surface(240, 320, tinygpu::FontRGB565);
  CHECK(surface.begin());
  type(gui, {"4", "*", "5"});
  screen.draw(surface);
  CHECK(!screen.isDirty());
}

// begin() adds the widgets once; end() removes them; begin() can follow end()
static void testLifecycle() {
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  CHECK(screen.childCount() == 0);
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  gui.begin(screen);
  const int widgets = screen.childCount();
  CHECK(widgets > 0);
  gui.begin(screen);                            // second call: no duplicates
  CHECK(screen.childCount() == widgets);
  gui.end();
  CHECK(screen.childCount() == 0);
  gui.end();                                    // second call: no-op
  CHECK(screen.childCount() == 0);
  gui.begin(screen);
  CHECK(screen.childCount() == widgets);
  gui.end();
}

// destroying an attached GUI removes its widgets from the screen
static void testDestructorDetaches() {
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  {
    FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
    gui.enableAll();
    gui.begin(screen);
    CHECK(screen.childCount() > 0);
  }
  CHECK(screen.childCount() == 0);
}

// beginPlot/endPlot open and close the plot screen from code
static void testBeginEndPlot() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  gui.enableAll();
  CHECK(gui.beginPlot("x^2"));
  CHECK(gui.plotting());
  CHECK(std::fabs(gui.plotSamples().back() - 100.0f) < 1e-3f);
  gui.endPlot();
  CHECK(!gui.plotting());
  CHECK(gui.expression() == "x^2");             // the expression is kept

  CHECK(!gui.beginPlot("2+"));                  // invalid: stays on the keypad
  CHECK(!gui.plotting());
  CHECK(gui.resultText() == "unexpected end of expression");
  CHECK(!gui.beginPlot(""));
  CHECK(gui.resultText() == "nothing to plot");
  gui.endPlot();                                // no-op when not plotting
}

int main() {
  testTypingAndEvaluation();
  testIncompleteExpressionShowsNoResult();
  testErrorsAreReported();
  testClearAndDelete();
  testFunctionsAndAngleUnit();
  testKeyButtonsAreWired();
  testEveryKeyCanBePressed();
  testPagesCycle();
  testAllFunctionsHaveKeys();
  testFunctionKeysEvaluate();
  testPlotSamplesFormula();
  testPlotUndefinedPoints();
  testPlotErrors();
  testPlotRangeIgnoresInvalid();
  testPlotRangeEntry();
  testPlotRangeRejectsReversed();
  testLifecycle();
  testDestructorDetaches();
  testBeginEndPlot();
  testLayoutAnySize();
  testSetBounds();
  testSwipePages();
  testSwipeInPlot();
  testInputRules();
  testUnitsAndFunctionKeys();
  testBasicByDefault();
  testUnitKeysAfterTo();
  testThemes();
#ifdef TINYCALC_USE_TINYFONTS
  testTinyFonts();
#endif
  testDoublePrecision();
  testAttachAndDraw();

  std::cout << g_checks << " GUI checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
