// Interactive desktop calculator built on TinyCalculator.
// Type expressions, "name = expr" assignments, or "quit".

#include <iomanip>
#include <iostream>
#include <string>

#include "TinyCalculator.h"

int main() {
  Calculator<double> calc;
  std::cout << "TinyCalculator REPL (double). Type 'quit' to exit.\n";

  std::string line;
  while (std::cout << "> " && std::getline(std::cin, line)) {
    if (line == "quit" || line == "exit") break;
    if (line.empty()) continue;

    auto r = calc.evaluate(line.c_str());
    if (r.ok()) {
      std::cout << std::setprecision(12) << r.value << "\n";
    } else {
      std::cout << "error: " << errorText(r.error) << " at position "
                << r.errorPos << "\n";
    }
  }
  return 0;
}
