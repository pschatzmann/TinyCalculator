# Notes

- On AVR boards `double` is 32-bit, so `Calculator<double>` costs nothing extra there.
- Function and constant names are looked up with `strcmp` in RAM tables. That is
  fine for this size, but on very small AVR boards you may want to move the
  tables to `PROGMEM`.
- `Math<T>` also exposes the numeric functions directly for use outside the parser.
- On the desktop, Arduino-Emulator's `Serial.available()` reports only input
  that is already buffered, so `tinycalc_basic` does not react to typed input.
  Use `tinycalc_repl` for interactive terminal use.
