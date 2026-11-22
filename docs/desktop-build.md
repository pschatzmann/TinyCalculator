# Desktop build (CMake)

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The first configure fetches TinyGPU and Arduino-Emulator from GitHub, and the
GUI needs SDL2 installed.

## Targets

| Target | What it is |
|--------|------------|
| `test_tinycalc` | Core library tests (float and double) |
| `test_gui` | GUI key logic and drawing tests (needs `TINYCALC_BUILD_GUI`) |
| `tinycalc_repl` | Interactive C++ calculator in the terminal |
| `tinycalc_basic` | `examples/Basic/Basic.ino` built for the desktop |
| `tinycalc_gui` | `examples/Gui/Gui.ino` in a 240x320 SDL2 window (needs `TINYCALC_BUILD_GUI`) |

```sh
./build/examples/tinycalc_gui      # opens the calculator window
./build/examples/tinycalc_repl     # type expressions, "quit" to exit
```

## Options

| Option | Default | Meaning |
|--------|---------|---------|
| `TINYCALC_BUILD_TESTS` | ON | Unit tests |
| `TINYCALC_BUILD_EXAMPLES` | ON | REPL and Basic sketch |
| `TINYCALC_BUILD_GUI` | ON | GUI example and GUI tests (requires SDL2) |
| `TINYCALC_TINYMD_DIR` | `../TinyMaterialDesign` if present | Local TinyMaterialDesign checkout; otherwise fetched from GitHub |
| `FETCHCONTENT_SOURCE_DIR_TINYGPU` | fetched | Local TinyGPU checkout |
| `FETCHCONTENT_SOURCE_DIR_ARDUINO_EMULATOR` | fetched | Local Arduino-Emulator checkout |

Arduino-Emulator is pinned to commit `9f8ecf8`. Newer revisions change how
TinyGPU selects its `millis()` emulation and do not build with the current
TinyMaterialDesign. The GUI targets define `ARDUINO`, as the Arduino IDE does,
because TinyMaterialDesign selects its Arduino code paths on that macro.

## Using TinyCalculator from another CMake project

```cmake
find_package(TinyCalculator 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE TinyCalculator::TinyCalculator)
```
