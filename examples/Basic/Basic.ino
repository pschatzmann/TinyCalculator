// TinyCalculator basic example: evaluate formulas typed on the Serial monitor.
#include <Arduino.h>

// Smaller limits for an Uno-class board (2 KB RAM). Defaults are 64 and 4.
#define TINYCALC_MAX_OPS 32
#define TINYCALC_MAX_FUNCTIONS 2

#include <TinyCalculator.h>

Calculator<float> calc;  // use Calculator<double> for double precision

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
  }
  calc.setVariable("x", 2.0f);
  Serial.println("TinyCalculator ready. Try: sin(pi/6) + x^3, y = 4*x, ans*2");
}

void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  Calculator<float>::Result r = calc.evaluate(line.c_str());
  if (r) {
    Serial.println(r.value, 6);
  } else {
    Serial.print("error: ");
    Serial.print(errorTextFlash(r.error));
    Serial.print(" at ");
    Serial.println(r.errorPos);
  }
}
