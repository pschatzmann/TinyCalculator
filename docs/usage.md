# Usage

The header brings the library names into the global namespace
(`using namespace TinyCalc;`). Define `TINYCALC_NO_USING_NAMESPACE` before
including it to turn that off and use the `TinyCalc::` prefix instead.

```cpp
#include <TinyCalculator.h>

Calculator<float> calc;     // or Calculator<double>
calc.setVariable("x", 2.0f);

auto r = calc.evaluate("sin(pi/6) + x^3");   // r.value == 8.5
if (r) {
  Serial.println(r.value);
} else {
  Serial.println(errorText(r.error));  // r.errorPos gives the position
}

calc.evaluate("y = 4*x");   // assignment
calc.evaluate("ans * 2");   // "ans" holds the last result

// compile once, run many times
Calculator<float>::Program prog;
calc.compile("x^2 + 1", prog);
for (int i = 0; i < 3; ++i) {
  calc.setVariable("x", float(i));
  calc.run(prog);
}

calc.setAngleUnit(AngleUnit::Degrees);  // trig functions use degrees
```

- `evaluate()` compiles and runs an expression or an assignment (`name = expr`).
  It also stores the result in `ans`.
- `compile()` / `run()` separate parsing from evaluation, so one program can be
  run with many variable values.
- `setVariable()`, `getVariable()` and `clearVariables()` manage user variables.
