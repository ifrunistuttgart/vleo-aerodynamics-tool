# AeroSat
Aerodnamic simulation of VLEO satellites based on computer graphics and GSI models.

## Project strcuture

## logging
for logging spdlog is used across the project.

## naming conventions
### General
The naming convention is inspired by pyhtons PEP8 naming convention, since C++ Core guidelines don't suggest a special naming convention, except of using snake_case over CamelCase.
- Class names: PascalCase, for example `Satellite`
- Interfaces: PascalCase with an `I` prefix, for example `ISatellite`
- Member Variables: snake_case
- Functions: snake_case, for example `calculate_drag`
- variables: snake_case, for example `drag_coefficient`
- constants: UPPER_SNAKE_CASE, for example `PI`
### Units:
- `__`to seperate the name and the unit
- `_per_` to seperate the numerator and denominator of a unit, for example `__m_per_s`
- exponents are directly after the unit, for example: `__m2`
