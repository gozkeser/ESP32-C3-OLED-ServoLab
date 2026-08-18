# Project Rules & Standards

## General Guidelines
- Use design patterns where possible keeping balance between complexity and modularity.
- Open Close Principle is very important. Apply where possible.
- Always try to use interfaces to break dependencies.
- No direct logging in business logic. Uses logger interface.

## Technical Constraints
- Try to keep number of lines of code in a function below 40.
- Dynamic memory allocation can be used but avoid where possible. Keep allocations and deallocations minimal.
- Follow best practices.
- Each class must have its own header file. Put implementations also in header file, avoid creating cpp files.

## Coding Style
- All code, comments, and documentation must be in English.
- No hard coded variable initializers and limits.
- Follow best practices.
- Each function and its arguments must have a short description.
- Each class, structure and enum must have a short description.

## Naming Conventions
- No specific constraints. Follow best practices.

## Header Inclusions
- To improve portability of source code avoid using <Arduino.h>. Instead, use standart C++ libraries.