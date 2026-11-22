# Functions

| Group | Functions |
|-------|-----------|
| Roots / powers | `sqrt cbrt root(x,n) pow(x,y) exp exp2 expm1 ln log(x[,base]) log2 log10 hypot` |
| Trigonometry | `sin cos tan sec csc cot asin acos atan atan2(y,x) asec acsc acot` |
| Hyperbolic | `sinh cosh tanh asinh acosh atanh` |
| Rounding / sign | `abs floor ceil round trunc frac sign mod clamp(x,lo,hi)` |
| Special | `gamma fact erf erfc comb(n,k) perm(n,k) gcd lcm` |
| Aggregates | `min max sum avg` (any number of arguments) |
| Conversion | `deg(rad) rad(deg)` |

- `log(x)` is log base 10; `log(x, base)` uses the given base.
- Trigonometric functions interpret their argument in the current angle unit
  (radians by default). Inverse functions return results in that unit.
- `deg` and `rad` convert between the two units regardless of the current setting.
