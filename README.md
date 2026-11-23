# TinyCalculator

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![CMake](https://img.shields.io/badge/CMake-Supported-blue.svg)](https://github.com/pschatzmann/TinyCalculator/blob/main/CMakeLists.txt)
[![License: Apache](https://img.shields.io/badge/License-Apache-yellow.svg)](https://opensource.org/licenses/Apache-2.0)

Header-only formula parser and evaluator for Arduino and desktop C++, with
all common math functions. The number type is a template parameter: `float`
(default) or `double`.

- All common math functions, constants and expressions with implicit multiplication
- Units and conversion: `1000mm to m` is 1, `100 degC to degF` is 212
- User-defined functions: `f(x) = x^2 + 1`, then `f(3)` is 10
- No heap allocation, no exceptions, C++11
- Optional calculator GUI (TinyMaterialDesign) with the basic keypad and
  optional screens for trigonometry, math, special functions, units and plotting

```cpp
#include <TinyCalculator.h>

Calculator<float> calc;
calc.setVariable("x", 2.0f);

auto r = calc.evaluate("sin(pi/6) + x^3");   // r.value == 8.5
if (!r) Serial.println(errorText(r.error));
```

## Documentation

- [Usage](https://github.com/pschatzmann/TinyCalculator/wiki/Usage): the API, assignments, compile and run
- [Syntax](https://github.com/pschatzmann/TinyCalculator/wiki/Syntax): operators, precedence, implicit multiplication
- [Functions](https://github.com/pschatzmann/TinyCalculator/wiki/Functions): all functions and constants
- [Errors](https://github.com/pschatzmann/TinyCalculator/wiki/Errors): error codes and error positions
- [Configuration](https://github.com/pschatzmann/TinyCalculator/wiki/Configuration): size limits and macros
- [Calculator GUI](https://github.com/pschatzmann/TinyCalculator/wiki/Calculator-GUI): the optional keypad
- [Desktop build](https://github.com/pschatzmann/TinyCalculator/wiki/Desktop-Build): CMake, tests and examples
- [Notes](https://github.com/pschatzmann/TinyCalculator/wiki/Notes): limitations and tips
- [Class documentation (Doxygen)](https://pschatzmann.github.io/TinyCalculator/annotated.html): API reference for all classes

## Installation in Arduino

You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/TinyCalculator.git
```

I recommend to use git because you can easily update to the latest version just by executing the ```git pull``` command in the project folder.

The optional calculator GUI also needs [TinyMaterialDesign](https://github.com/pschatzmann/TinyMaterialDesign), [TinyGPU](https://github.com/pschatzmann/TinyGPU) and [TinyFonts](https://github.com/pschatzmann/TinyFonts), which are installed the same way.

