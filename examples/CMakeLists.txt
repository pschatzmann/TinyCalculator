# Plain C++ REPL: no Arduino API needed.
add_executable(tinycalc_repl Repl/main.cpp)
target_link_libraries(tinycalc_repl PRIVATE TinyCalculator)

# Basic Arduino sketch, built for the desktop via Arduino-Emulator (Serial
# reads from stdin).
arduino_sketch(tinycalc_basic Basic/Basic.ino LIBRARIES TinyCalculator)

if(TINYCALC_BUILD_GUI)
  # Calculator keypad with TinyMaterialDesign, in an SDL2 window (240x320).
  arduino_sketch(tinycalc_gui Gui/Gui.ino
                 LIBRARIES TinyCalculator TinyMaterialDesign SDL2::SDL2)
  # ARDUINO is defined by the Arduino IDE; TinyMaterialDesign selects its
  # Arduino code paths (millis(), namespaces) on it.
  target_compile_definitions(tinycalc_gui PRIVATE ARDUINO=10819
                             TINYGPU_AUTO_NAMESPACE TINYMD_AUTO_NAMESPACE)
  set_target_properties(tinycalc_gui PROPERTIES CXX_STANDARD 17)
  target_link_libraries(tinycalc_gui TinyFonts::TinyFonts)
endif()
