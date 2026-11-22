# Configuration

Define these before including the library to change the limits:

| Macro | Default | Meaning |
|-------|---------|---------|
| `TINYCALC_MAX_OPS` | 64 | Max instructions per compiled expression |
| `TINYCALC_MAX_VARIABLES` | 8 | Max variables (including `ans`) |
| `TINYCALC_MAX_NAME` | 12 | Max identifier length |
| `TINYCALC_MAX_NESTING` | 32 | Max parenthesis / unary nesting depth |
| `TINYCALC_NO_USING_NAMESPACE` | not set | Do not add `using namespace TinyCalc;` |

`Calculator<T, MaxOps, MaxVars>` takes `MaxOps` and `MaxVars` as template
arguments as well.
