# Week 3 - C++20 Concepts: Constrained Algorithms Application

## Project Summary

`ConstrainedAlgApp` is a small C++20 console application that demonstrates constrained, generic algorithms over `std::vector` and other ranges. The algorithms are implemented as header-only function templates in the `include` folder and exercised by `source/Main.cpp`.

The project focuses on expressing template requirements with standard library concepts and `requires`-style constraints. The examples are intentionally small so the constraints and their effect on callers remain easy to inspect.

## Goals Mapping

| Weekly goal | Implementation in this project |
| --- | --- |
| Learn concepts and constrained templates | `Filter.h`, `Format.h`, and `PrintRange.h` use C++20 concepts or concept-based constraints. |
| Replace unconstrained templates with clearer requirements | `Filter` and `Search` constrain predicates with `std::predicate`; `Format` constrains values with `ConvertibleToString`; `PrintRange` constrains its argument with `std::ranges::range`. |
| Think in terms of compile-time API contracts | Invalid predicate, range, or formatting types are rejected during template instantiation rather than failing as arbitrary operations deep in an algorithm. |

## Project Layout

```text
ConstrainedAlgApp/
├── ConstrainedAlgApp.slnx
├── ConstrainedAlgApp.vcxproj
├── include/
│   ├── Filter.h       - filters a vector using a constrained predicate
│   ├── Format.h       - formats vector elements using std::to_string
│   ├── Logger.h       - thread-safe console logger with levels and formatting
│   ├── PrintRange.h   - prints any std::ranges::range
│   └── Search.h       - finds the first vector element matching a predicate
└── source/
    └── Main.cpp       - example program and concept/algorithm usage
```

The Visual Studio project adds `include` to the include search path and compiles with `stdcpp20`. This is an executable project, not a separately packaged library: the reusable algorithm code is header-only, while `Main.cpp` provides the application entry point.

## Implemented Components

### `Search.h`

`Search` accepts a `const std::vector<T>&` and a predicate satisfying `std::predicate<const T&>`. It uses `std::find_if` and returns `std::optional<T>` containing the first match, or `std::nullopt` when there is no match.

This is predicate-based linear search. It is not a binary search and does not require ordering or comparison operators on `T`.

### `Filter.h`

`Filter` accepts a `const std::vector<T>&` and a predicate satisfying `std::predicate<const T&>`. It returns `std::optional<std::vector<T>>` containing all matching elements.

The current example filters `{1, 2, 3, 4, 5}` to produce the even values. The result is unwrapped with `.value()` before being passed to `Format`.

### `Format.h`

`Format` defines the local `ConvertibleToString` concept. A value must support `std::to_string` with a result convertible to `std::string`. The function formats a `std::vector<T>` using a caller-provided delimiter, defaulting to `", "`.

The example formats the integer vector with `" | "` and formats the filtered vector with the default delimiter.

### `PrintRange.h`

`PrintRange` accepts any type satisfying `std::ranges::range` and writes each value followed by a space and a newline. `Main.cpp` demonstrates it with `std::vector<int>`, `std::array<int, 3>`, and `std::string`.

Unlike `Search`, `Filter`, and `Format`, this function is not limited to `std::vector`.

### `Logger.h`

`Logger` is an application utility rather than an algorithm constraint. It provides:

- `DEBUG`, `INFO`, `WARNING`, and `ERROR` levels.
- A configurable minimum level protected by a mutex.
- Environment-based initialization through `DEBUG` and `LOG_LEVEL`.
- Timestamped output and `std::format`-based formatting.
- Color output when the console supports it.

`Main.cpp` initializes the logger and uses it to report the search, filtering, and formatting examples.

## Concepts and Constraints Used

The implementation currently uses standard library concepts rather than a shared custom `concepts.hpp`:

- `std::predicate<const T&>` constrains the callable passed to `Search` and `Filter`.
- `ConvertibleToString` constrains the element type passed to `Format`.
- `std::ranges::range` constrains the input passed to `PrintRange`.
- `std::convertible_to<std::string>` is used inside `ConvertibleToString` to verify the result type of `std::to_string`.

There is no `Sortable` concept in the current program. The algorithms do not sort and do not require `<`, `==`, or three-way comparison from their element types.

## Example Program Flow 

`source/Main.cpp` performs the following operations:

1. Initializes `Logger` from the environment and logs the application start.
2. Searches a vector of integers for `3` with a lambda predicate.
3. Filters the vector to retain even numbers.
4. Formats the filtered values and the original vector.
5. Prints a vector, an array, and a string through the generic range printer.

The file also contains a `NoComparison` type whose comparison operators are deleted. The commented example is useful for demonstrating that the current predicate-based `Search` API does not need comparison operators unless the caller's predicate uses them.

## Build and Run

Build the `ConstrainedAlgApp` project from `ConstrainedAlgApp.slnx` or `ConstrainedAlgApp.vcxproj` with the Visual Studio C++20 toolset. The configured project includes the `include` directory and has Debug and Release configurations for Win32 and x64.

No CMake target, Catch2 test target, or separate diagnostics executable currently exists. Validation is performed by building and running the console application, plus compile-time checks from the constrained templates.

## Follow-up Work

The current implementation satisfies the basic C++20 concepts demonstration. Possible next steps, if the project grows, are:

1. Generalize `Search`, `Filter`, and `Format` from `std::vector` to `std::ranges::input_range` or another appropriate range concept.
2. Return a plain `std::vector<T>` from `Filter`, unless an empty result needs to be distinguished from an error.
3. Move shared concepts into a dedicated header only when more than one algorithm needs the same custom contract.
4. Add focused tests or a separate diagnostics sample if compiler-error comparisons become part of the exercise.
