#pragma once

// Tunable compile-time limits. Define them before including the library to override.

#define TINYCALC_VERSION_MAJOR 1
#define TINYCALC_VERSION_MINOR 0
#define TINYCALC_VERSION_PATCH 0

// Maximum number of RPN instructions in a compiled expression
#ifndef TINYCALC_MAX_OPS
#define TINYCALC_MAX_OPS 64
#endif

// Maximum number of user variables (the built-in "ans" variable counts as one)
#ifndef TINYCALC_MAX_VARIABLES
#define TINYCALC_MAX_VARIABLES 8
#endif

// Maximum length of identifiers (functions, constants, variables)
#ifndef TINYCALC_MAX_NAME
#define TINYCALC_MAX_NAME 12
#endif

// Maximum number of user-defined functions (e.g. f(x) = x^2)
#ifndef TINYCALC_MAX_FUNCTIONS
#define TINYCALC_MAX_FUNCTIONS 4
#endif

// Maximum number of parameters of a user-defined function
#ifndef TINYCALC_MAX_PARAMS
#define TINYCALC_MAX_PARAMS 3
#endif

// Maximum nesting of parentheses / unary operators (guards recursion depth)
#ifndef TINYCALC_MAX_NESTING
#define TINYCALC_MAX_NESTING 32
#endif
