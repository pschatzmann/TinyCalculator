/**
 * @file Gui.ino
 * @brief TinyCalculator with a TinyMaterialDesign keypad (CalculatorGui).
 *
 * The same source runs on an ESP32 display (DefaultBoard) and, on the
 * desktop, in a 240x320 SDL2 window - see TinyMaterialDesignConfig.h.
 * Tap the keys with the mouse; the expression is shown above the result.
 */
#include <TinyCalculatorGui.h>

// Same panel size as the TinyMaterialDesign examples.
App<RGB565> app(DefaultBoard);
CalculatorGui<RGB565, float> calculatorGui(Bounds(0, 0, 240, 320));

void setup() {
  Serial.begin(115200);
  app.begin();
  calculatorGui.attach(app.screen());
}

void loop() {
  app.update();
}
