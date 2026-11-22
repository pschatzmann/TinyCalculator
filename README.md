# TinyCalculator

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![CMake](https://img.shields.io/badge/CMake-Supported-blue.svg)](https://github.com/pschatzmann/TinyCalculator/blob/main/CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/license/mit)

Header-only formula parser and evaluator for Arduino and desktop C++, with
all common math functions. The number type is a template parameter: `float`
(default) or `double`.

```cpp
#include <TinyCalculator.h>

Calculator<float> calc;
calc.setVariable("x", 2.0f);

auto r = calc.evaluate("sin(pi/6) + x^3");   // r.value == 8.5
if (!r) Serial.println(errorText(r.error));
```


## Documentation

- [Usage](docs/usage.md): the API, assignments, compile and run
- [Syntax](docs/syntax.md): operators, precedence, implicit multiplication
- [Functions](docs/functions.md): all functions and constants
- [Errors](docs/errors.md): error codes and error positions
- [Configuration](docs/configuration.md): size limits and macros
- [Calculator GUI](docs/gui.md): the optional keypad
- [Desktop build](docs/desktop-build.md): CMake, tests and examples
- [Notes](docs/notes.md): limitations and tips
- [Class documentation (Doxygen)](https://pschatzmann.github.io/TinyCalculator/annotated.html): API reference for all classes

## Installation (Arduino)

1. Download this repository as a ZIP file, or clone it into your Arduino
   libraries folder: `git clone https://github.com/pschatzmann/TinyCalculator.git`
2. In the Arduino IDE, choose **Sketch → Include Library → Add .ZIP Library**
   (for the ZIP file) or restart the IDE after cloning.
3. Examples are under **File → Examples → TinyCalculator**.

The optional calculator GUI also needs
[TinyMaterialDesign](https://github.com/pschatzmann/TinyMaterialDesign) and
[TinyGPU](https://github.com/pschatzmann/TinyGPU), installed the same way.
