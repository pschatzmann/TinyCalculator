# Syntax

| Kind | Examples |
|------|----------|
| Operators | `+ - * / % ^` (also `**`), unary `-` `+`, postfix factorial `!` |
| Precedence | `^` is right associative and binds tighter than unary minus: `-2^2 = -4` |
| Implicit multiplication | `2pi`, `2(3+4)`, `2sin(x)`, `(1+1)(2+2)` |
| Numbers | `12`, `.5`, `1.`, `2e3`, `2.5E-1` |
| Constants | `pi`, `e`, `tau`, `phi` |
| Variables | `x`, `y = x*3`, `ans` (last result) |

`%` is the C-style remainder (`-7 % 3 = -1`); `mod(a, b)` is the floored
modulo (`mod(-7, 3) = 2`).

See [functions.md](functions.md) for the list of functions.
