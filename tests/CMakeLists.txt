add_executable(test_tinycalc test_tinycalc.cpp)
target_link_libraries(test_tinycalc PRIVATE TinyCalculator)
target_compile_options(test_tinycalc PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME tinycalc COMMAND test_tinycalc)

if(TINYCALC_BUILD_GUI)
  add_executable(test_gui test_gui.cpp)
  target_link_libraries(test_gui PRIVATE TinyCalculator TinyMaterialDesign
                        arduino_emulator SDL2::SDL2)
  target_compile_definitions(test_gui PRIVATE ARDUINO=10819 TINYGPU_AUTO_NAMESPACE TINYMD_AUTO_NAMESPACE)
  target_compile_features(test_gui PRIVATE cxx_std_17)
  add_test(NAME tinycalc_gui COMMAND test_gui)
endif()
