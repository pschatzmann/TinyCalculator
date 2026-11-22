// Desktop tests for the optional TinyMaterialDesign GUI (CalculatorGui).
// The key logic is exercised through press() and through each key button's
// onClick handler; the widgets are also attached to a screen and drawn.

#include <TinyCalculatorGui.h>

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
  type(gui, {"2", "+"});
  CHECK(gui.expression() == "2+");
  gui.press("=");  // "=" on an incomplete expression reports an error
  CHECK(gui.resultText() == "unexpected end of expression");
}

static void testErrorsAreReported() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
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
  for (size_t p = 0; p < FloatGui::kPageCount; ++p) {
    for (size_t i = 0; i < FloatGui::kKeyCount; ++i) {
      const char* action = FloatGui::keys(p)[i].action;
      if (action == nullptr) continue;  // unused slot
      CHECK(std::strlen(action) > 0);
      if (std::strcmp(action, "page") == 0) continue;
      gui.press(action);
    }
  }
  gui.press("clear");
  CHECK(gui.expression().empty());
}

static void testPagesCycle() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
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
  size_t count = 0;
  const TinyCalc::FunctionInfo* table = TinyCalc::functionTable(count);
  for (size_t i = 0; i < count; ++i) {
    CHECK(actions.count(std::string(table[i].name) + "(") == 1);
  }
  for (const char* c : {"pi", "e", "tau", "phi", "ans"}) {
    CHECK(actions.count(c) == 1);
  }
}

// a function key enters a usable call: "sqrt(" + argument + ")" evaluates
static void testFunctionKeysEvaluate() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  type(gui, {"sqrt(", "1", "6", ")", "="});
  CHECK(gui.resultText() == "4");
  gui.press("clear");
  type(gui, {"min(", "3", ",", "1", ",", "2", ")", "="});
  CHECK(gui.resultText() == "1");
  gui.press("clear");
  type(gui, {"gcd(", "12", ",", "18", ")", "="});
  CHECK(gui.resultText() == "6");
}

static void testDoublePrecision() {
  DoubleGui gui(tinymd::Bounds(0, 0, 240, 320));
  type(gui, {"1", "/", "3"});
  gui.press("=");
  CHECK(gui.resultText() == "0.333333333333");  // 12 significant digits
}

static void testAttachAndDraw() {
  FloatGui gui(tinymd::Bounds(0, 0, 240, 320));
  tinymd::Screen<TINYMD_DEFAULT_RGB_T> screen(tinymd::defaultTheme<TINYMD_DEFAULT_RGB_T>());
  gui.attach(screen);

  tinygpu::Surface<TINYMD_DEFAULT_RGB_T> surface(240, 320, tinygpu::FontRGB565);
  CHECK(surface.begin());
  type(gui, {"4", "*", "5"});
  screen.draw(surface);
  CHECK(!screen.isDirty());
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
  testDoublePrecision();
  testAttachAndDraw();

  std::cout << g_checks << " GUI checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
