/**
 * @file Gui.ino
 * @brief TinyCalculator with a TinyMaterialDesign keypad (CalculatorGui).
 *
 * The same source runs on an ESP32 display (DefaultBoard) and, on the
 * desktop, in a 240x320 SDL2 window - see TinyMaterialDesignConfig.h.
 * Tap the keys with the mouse; the expression is shown above the result.
 *
 * Fonts: TinyFonts gives the expression and result lines smooth text. The
 * sketch assigns them with a font function (calculatorFonts) to the theme.
 */
#include <TinyCalculatorGui.h>
#include <TinyFontsGPU.h>

tinyfonts::TinyGPUFont<RGB565> smallFont(tinyfonts::Lato12);  // keys, labels, chart text
tinyfonts::TinyGPUFont<RGB565> resultFont(tinyfonts::Lato32); // result line
// Same panel size as the TinyMaterialDesign examples.
App<RGB565> app(DefaultBoard);
CalculatorGui<RGB565, float> calculatorGui(Bounds(0, 0, DefaultBoard.width(), DefaultBoard.height()));

// Sketch-defined font function: called by the calculator with its theme.
void calculatorFonts(CalculatorTheme<RGB565>& theme) {
  theme.typography.label = &smallFont;     // keys, chart axes, field labels
  theme.typography.body = &smallFont;      // expression line, chart title and footer
  theme.typography.headline = &resultFont; // result line
}


void setup() {
  Serial.begin(115200);
  app.begin();
  calculatorGui.assignFonts(calculatorFonts);
  calculatorGui.begin(app.screen());
}

void loop() {
  app.update();
}
