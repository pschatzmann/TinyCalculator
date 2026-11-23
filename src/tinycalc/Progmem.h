#pragma once

// Flash (PROGMEM) abstraction. On AVR boards the library's constant tables
// (function names, unit names, constants, error texts, keywords) are kept in
// flash instead of RAM. On all other platforms these macros are plain
// variables and copies.

#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define TINYCALC_PROGMEM PROGMEM
// copies n bytes from flash (src) into RAM (dst)
#define TINYCALC_MEMCPY_P(dst, src, n) memcpy_P((dst), (src), (n))
// compares a RAM string with a flash string
#define TINYCALC_STRCMP_P(ram, flash) strcmp_P((ram), (flash))
// reads a pointer element of a flash array
#define TINYCALC_READ_PTR(p) ((const char*)pgm_read_ptr(&(p)))
#else
#define TINYCALC_PROGMEM
#define TINYCALC_MEMCPY_P(dst, src, n) memcpy((dst), (src), (n))
#define TINYCALC_STRCMP_P(ram, flash) strcmp((ram), (flash))
#define TINYCALC_READ_PTR(p) (p)
#endif
