#pragma once

// TinyCalculator - header-only formula parser and evaluator for Arduino and
// desktop C++ (C++11). Number type is a template parameter (float by default,
// double supported).

#include "tinycalc/Config.h"
#include "tinycalc/Types.h"
#include "tinycalc/Math.h"
#include "tinycalc/Functions.h"
#include "tinycalc/Expression.h"
#include "tinycalc/Compiler.h"
#include "tinycalc/Evaluator.h"
#include "tinycalc/Calculator.h"

// Make the library names available without the TinyCalc:: prefix.
// Define TINYCALC_NO_USING_NAMESPACE before including this header to disable.
#ifndef TINYCALC_NO_USING_NAMESPACE
using namespace TinyCalc;
#endif
