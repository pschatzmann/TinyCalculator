# Errors

`Calculator::evaluate()` returns a `Result` with `value`, `error` and
`errorPos` (the position in the input). `errorText(error)` returns a
description.

| Error | Meaning |
|-------|---------|
| `Empty` | empty expression |
| `Syntax`, `UnexpectedToken`, `UnexpectedEnd` | malformed expression |
| `UnbalancedParenthesis` | missing or extra parenthesis |
| `UnknownIdentifier`, `UnknownFunction` | unknown variable or function name |
| `WrongArgumentCount` | function called with the wrong number of arguments |
| `DivisionByZero` | division, modulo or pole by zero |
| `Domain` | argument outside the function's domain (e.g. `sqrt(-1)`, `ln(0)`) |
| `Overflow` | result too large for the number type |
| `TooComplex`, `NestingTooDeep` | expression exceeds the configured limits |
| `TooManyVariables`, `InvalidName`, `ReservedName` | variable table errors |
