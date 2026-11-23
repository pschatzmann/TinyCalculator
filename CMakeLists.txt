cmake_minimum_required(VERSION 3.14)

project(TinyCalculator
  VERSION 1.0.0
  DESCRIPTION "Header-only formula parser and evaluator for Arduino and C++"
  LANGUAGES CXX)

# Sub-projects (Arduino-Emulator, TinyMaterialDesign) call project() themselves
# and overwrite PROJECT_VERSION, so keep our own copy.
set(TINYCALC_VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)
include(FetchContent)

# ---- header-only target -------------------------------------------------
add_library(TinyCalculator INTERFACE)
add_library(TinyCalculator::TinyCalculator ALIAS TinyCalculator)

target_include_directories(TinyCalculator INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(TinyCalculator INTERFACE cxx_std_11)

# ---- options ----------------------------------------------------------------
set(_tinycalc_top_level OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(_tinycalc_top_level ON)
endif()

option(TINYCALC_BUILD_TESTS "Build the desktop unit tests" ${_tinycalc_top_level})
option(TINYCALC_BUILD_EXAMPLES "Build the desktop examples (REPL, Basic sketch)" ${_tinycalc_top_level})
option(TINYCALC_BUILD_GUI "Build the TinyMaterialDesign GUI example and its tests (needs SDL2)" ${_tinycalc_top_level})
set(TINYCALC_TINYMD_DIR "" CACHE PATH
    "Local TinyMaterialDesign checkout. Default: ../TinyMaterialDesign if present, otherwise fetched from GitHub")

# ---- Arduino-Emulator: arduino_sketch() and the Arduino API on the desktop ---
# TinyGPU's headers include Arduino core headers, so the GUI and the Arduino
# sketches both need it. Fetched from GitHub; override with
# -DFETCHCONTENT_SOURCE_DIR_ARDUINO_EMULATOR=/path/to/checkout.
if(TINYCALC_BUILD_EXAMPLES OR TINYCALC_BUILD_GUI)
  set(USE_HTTPS OFF CACHE BOOL "" FORCE)  # avoids building wolfSSL
  # Pinned to the revision TinyMaterialDesign is tested with; newer revisions
  # change how TinyGPU picks its millis()/delay() emulation.
  FetchContent_Declare(arduino_emulator
    GIT_REPOSITORY https://github.com/pschatzmann/Arduino-Emulator.git
    GIT_TAG 9f8ecf8)
  FetchContent_MakeAvailable(arduino_emulator)
endif()

# ---- TinyMaterialDesign (+ TinyGPU, SDL2) for the GUI ---------------------------
# TinyMaterialDesign fetches TinyGPU itself; override with
# -DFETCHCONTENT_SOURCE_DIR_TINYGPU=/path/to/TinyGPU.
if(TINYCALC_BUILD_GUI)
  find_package(SDL2 REQUIRED)

  if(NOT TINYCALC_TINYMD_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../TinyMaterialDesign/CMakeLists.txt")
    set(TINYCALC_TINYMD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../TinyMaterialDesign")
  endif()

  # Only the library is needed here, not its own examples and tests.
  set(TMD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(TMD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  if(TINYCALC_TINYMD_DIR)
    add_subdirectory("${TINYCALC_TINYMD_DIR}" "${CMAKE_BINARY_DIR}/TinyMaterialDesign")
  else()
    FetchContent_Declare(tinymaterialdesign
      GIT_REPOSITORY https://github.com/pschatzmann/TinyMaterialDesign.git
      GIT_TAG main)
    FetchContent_MakeAvailable(tinymaterialdesign)
  endif()
endif()

# ---- TinyFonts (optional): text in the GUI -------------------------------------
# Enables TinyFonts for the GUI's expression and result lines. Needs the
# TinyFonts sources: a sibling ../TinyFonts checkout or -DTINYCALC_TINYFONTS_DIR=...
option(TINYCALC_USE_TINYFONTS "Use TinyFonts for the GUI text" OFF)
if(TINYCALC_BUILD_GUI)
  # the GUI example always uses TinyFonts
  set(TINYCALC_USE_TINYFONTS ON CACHE BOOL "" FORCE)
endif()
set(TINYCALC_TINYFONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../TinyFonts" CACHE PATH
    "Local TinyFonts checkout (used when TINYCALC_USE_TINYFONTS is ON)")
if(TINYCALC_USE_TINYFONTS)
  if(NOT EXISTS "${TINYCALC_TINYFONTS_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "TINYCALC_USE_TINYFONTS is ON but TinyFonts was not found at ${TINYCALC_TINYFONTS_DIR}")
  endif()
  # only the header-only target: no tests, no examples
  set(TINYFONTS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(TINYFONTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory("${TINYCALC_TINYFONTS_DIR}" "${CMAKE_BINARY_DIR}/TinyFonts")
endif()

# ---- tests / examples -----------------------------------------------------------
if(TINYCALC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(TINYCALC_BUILD_EXAMPLES OR TINYCALC_BUILD_GUI)
  add_subdirectory(examples)
endif()

# ---- installation / package config ---------------------------------------------
include(CMakePackageConfigHelpers)

install(TARGETS TinyCalculator EXPORT TinyCalculatorTargets)
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(EXPORT TinyCalculatorTargets
        FILE TinyCalculatorTargets.cmake
        NAMESPACE TinyCalculator::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyCalculator)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyCalculatorConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/TinyCalculatorConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyCalculator)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/TinyCalculatorConfigVersion.cmake
  VERSION ${TINYCALC_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/TinyCalculatorConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/TinyCalculatorConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyCalculator)
