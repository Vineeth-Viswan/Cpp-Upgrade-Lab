# ConfigParser Design and Implementation Review

## Scope

This review covers the `Week-2/ConfigParser/ConfigParser` implementation against:

- [config-parser-plan.md](config-parser-plan.md)
- [config-parser-constraints.md](config-parser-constraints.md)
- The C++ source, public headers, and Visual Studio project configuration

The review is organized by impact. Findings marked **High** can produce incorrect parse results or violate the documented input contract. Findings marked **Medium** affect maintainability, testability, or portability.

## Findings

### High: The tokenizer API and implementation disagree with the design

The plan describes a tokenizer that consumes configuration text and returns tokens, while [Tokenizer.h](ConfigParser/include/Tokenizer.h#L18) and [Tokenizer.cpp](ConfigParser/source/Tokenizer.cpp#L7) accept a string and interpret it as a file path. This couples lexical analysis to filesystem access and makes the tokenizer difficult to unit-test with an in-memory string. It also makes file-loading errors part of the tokenizer rather than a separate I/O boundary.

**Recommendation:** Add a file-loading layer that returns `std::variant<std::string, Error>` and make `Tokenizer::Tokenize` accept `std::string_view` or `const std::string&` containing file contents. Keep path handling in the CLI or a dedicated `ConfigFileReader`.

### High: The supported comment syntax is wrong

The constraints require a full-line comment whose first non-whitespace character is `#`. [Tokenizer.cpp](ConfigParser/source/Tokenizer.cpp#L24) recognizes `;` instead. A valid `# comment` is therefore reported as an invalid line, while an unsupported `; comment` is accepted.

**Recommendation:** Implement the documented `#` syntax and add tests for leading whitespace, `#`, `;`, and inline comment text. If both syntaxes are intended, change the constraints explicitly and test both.

### High: Duplicate sections and keys are silently overwritten

The constraints require repeated section names and repeated keys within a section to be errors. [Parser.cpp](ConfigParser/source/Parser.cpp#L158) reuses an existing section, and [Parser.cpp](ConfigParser/source/Parser.cpp#L211) assigns into the map without checking whether the key already exists. The second value silently replaces the first one.

**Recommendation:** Use `find` or `contains` before insertion and return a structured duplicate error at the section/key location. Use `emplace` where insertion success itself is the check.

### High: The value model cannot represent the specified types

[Types.h](ConfigParser/include/Types.h#L45) stores integers as `int` and arrays as `std::vector<std::string>`. The constraints require signed `std::int64_t` integers and homogeneous arrays whose elements retain their scalar type. The current model cannot distinguish an integer array from a string array or represent a boolean array.

**Recommendation:** Define a recursive or layered value model, for example a scalar variant plus explicitly typed array alternatives, such as `std::vector<std::int64_t>`, `std::vector<bool>`, and `std::vector<std::string>`. Use `std::int64_t` in the scalar variant and include `<cstdint>`.

### High: Array validation is incomplete and invalid input is discarded

[Parser.cpp](ConfigParser/source/Parser.cpp#L59) drops empty elements instead of reporting them. Consequently `[]`, `[1,,2]`, `[1,]`, and `[,1]` can produce an empty or shortened array even though arrays must contain at least one element and malformed elements must be rejected. The function also returns strings without parsing element types, so mixed arrays such as `[1, true]` are accepted.

**Recommendation:** Parse arrays as a dedicated operation returning either a typed array or `Error`. Preserve empty elements while splitting so leading, trailing, and repeated commas are detectable. Parse every element using the scalar parser, require all element types to match, and reject nested brackets.

### High: Integer parsing violates the range and syntax contract

[Parser.cpp](ConfigParser/source/Parser.cpp#L28) uses `std::stoi`, which limits values to `int` and may accept a valid numeric prefix without requiring the entire input to be consumed. Overflow is caught and converted into a string, although the constraints explicitly require integer overflow to be reported as an error.

**Recommendation:** Parse into `std::int64_t` with a complete-consumption check. Return a structured error for overflow and malformed numeric input when the value is intended to be an integer. Add boundary tests for `INT64_MIN`, `INT64_MAX`, one beyond each boundary, and suffixes such as `12abc`.

### Medium: Boolean parsing is more permissive than specified

The constraints require exactly lowercase `true` and `false`. [Parser.cpp](ConfigParser/source/Parser.cpp#L9) lowercases the input first, so `TRUE`, `True`, and `FALSE` are accepted as booleans.

**Recommendation:** Compare directly with `true` and `false`. If case-insensitive values are desired, update the format contract rather than silently broadening it.

### Medium: Section syntax is not fully validated

[Tokenizer.cpp](ConfigParser/source/Tokenizer.cpp#L35) accepts the first `]` it finds and ignores all text after it. It also accepts an empty section name, despite the constraint that a section name must not be empty. Examples such as `[http] trailing` and `[]` should be rejected.

**Recommendation:** Trim the section name, reject it when empty, require the closing bracket to be the final non-whitespace character, and report the exact offending column.

### Medium: Key trimming is asymmetric

[Tokenizer.cpp](ConfigParser/source/Tokenizer.cpp#L57) trims trailing whitespace from the key but not leading whitespace. The documented `key = value` behavior says whitespace around the key is trimmed, so `  port = 8080` becomes a key containing leading spaces.

**Recommendation:** Centralize a `Trim` helper and apply it consistently to section names, keys, values, and array elements.

### Medium: Token positions are inconsistent and mostly zero-based

[Tokenizer.cpp](ConfigParser/source/Tokenizer.cpp#L21) and related token creation use column `0` for keys, comments, and blank lines, while the equals and value positions are calculated from string offsets. [Types.h](ConfigParser/include/Types.h#L21) does not document whether positions are zero- or one-based.

**Recommendation:** Define one position convention, preferably one-based line and column numbers for user-facing diagnostics. Store a source span or at least a consistent start column for every token. Add assertions/tests for tabs and leading whitespace.

### Medium: The parser has hidden mutable state that is not used

[Tokenizer.h](ConfigParser/include/Tokenizer.h#L21) stores `tokenList`, but `Tokenize` returns the local vector and no other method reads the member. [Parser.h](ConfigParser/include/Parser.h#L15) similarly stores `document`, while `Parse` builds a local `doc`. These members add state without providing behavior and make object reuse and thread-safety less obvious.

**Recommendation:** Remove both members and use stateless operations. Make the classes non-stateful value-oriented services, or make the state explicit if incremental parsing is a future requirement.

### Medium: The document exposes implementation containers instead of a read-only domain API

[Types.h](ConfigParser/include/Types.h#L42) and [Types.h](ConfigParser/include/Types.h#L43) expose mutable `std::map` aliases. This permits consumers to mutate parsed data without validation and offers no lookup abstraction or not-found behavior, despite the plan calling for read-only lookup.

**Recommendation:** Introduce `Document` and `Section` classes with private storage and const lookup methods returning `std::optional<std::reference_wrapper<const Value>>` or a suitable const pointer/view. Keep insertion and duplicate validation inside the parser.

### Medium: The parser accepts unexpected token streams

[Parser.cpp](ConfigParser/source/Parser.cpp#L148) only handles blank lines, comments, section headers, and keys. Standalone `Equals`, `Value`, or an unknown token type is ignored rather than producing an error. This weakens the parser/tokenizer contract and makes malformed token sequences appear valid.

**Recommendation:** Make the parser exhaustive over `TokenType`. After handling the valid forms, return an internal syntax error for unexpected tokens. Prefer a token representation where a complete key-value entry is one parser-level construct, reducing index-based lookahead.

### Medium: File path handling is not portable or controllable from the CLI

[Main.cpp](ConfigParser/source/Main.cpp#L14) ignores `argc`/`argv` and always opens `test_config.ini`. The path is relative to the process working directory, not the executable or source directory, so launching from another directory fails.

**Recommendation:** Accept an input path as a command-line argument, validate it, and use `std::filesystem::path`. Keep the sample file as a test fixture or explicit default rather than silently depending on the current directory.

### Medium: The project configuration contradicts the C++17 exercise

[ConfigParser.vcxproj](ConfigParser/ConfigParser.vcxproj#L88) selects `stdcpp20`, while [config-parser-constraints.md](config-parser-constraints.md) defines a C++17 exercise. [Logger.h](ConfigParser/include/Logger.h#L7) also includes and uses `std::format`, which is a C++20 facility.

**Recommendation:** Decide whether this project targets C++17 or C++20. For a C++17 exercise, replace `std::format` in the logger with a compatible formatting approach or isolate the logger behind a small interface. For a C++20 target, update the plan and constraints to state that requirement.

### Low: Error and token types need stronger domain semantics

[Types.h](ConfigParser/include/Types.h#L21) uses `int` for line and column values and [Types.h](ConfigParser/include/Types.h#L36) defines `ValueType` without using it. The global namespace also exposes generic names such as `Token`, `Error`, `Value`, and `Document`.

**Recommendation:** Put the model in a project namespace, use `std::size_t` or a dedicated `SourcePosition` type for positions, remove unused `ValueType`, and add an error category enum distinguishing I/O, lexical, syntax, duplicate, and value errors.

## Architecture assessment

The intended pipeline is sound for a small learning project:

```text
file path -> file reader -> source text -> tokenizer -> tokens -> parser -> immutable document
```

The current implementation collapses the first two stages and performs value conversion inside parser helpers. A cleaner boundary would be:

1. `ConfigFileReader`: filesystem-only concerns and I/O errors.
2. `Tokenizer`: source text to lexical tokens; no filesystem or logging dependency.
3. `Parser`: token sequence to document structure; section/key rules and duplicate checks.
4. `ValueParser`: scalar and array conversion with precise value errors.
5. `Document`: owned, read-only configuration data and lookup operations.
6. CLI adapter: arguments, logging, error presentation, and process exit codes.

Each layer should return a value-or-error result. Logging should occur at the application boundary so library code remains deterministic and reusable in tests.

## Recommended implementation order

1. Align the format contract: comment marker, boolean case rules, section validation, position convention, and C++ language version.
2. Replace the public type aliases with namespaced value types and a typed `int64_t` value model.
3. Separate file loading from tokenization and remove parser/tokenizer mutable members.
4. Add strict scalar and array parsing with structured errors and no silent element dropping.
5. Add duplicate detection and exhaustive token validation.
6. Add read-only document lookup methods.
7. Add CLI path handling and keep logging at the CLI boundary.
8. Add automated tests before further refactoring.

## Minimum test matrix

### Tokenizer

- Empty input and whitespace-only input
- `#` comments with leading whitespace
- Unsupported `;` comments
- Valid and invalid section headers, including `[]` and trailing text
- Keys with leading/trailing whitespace
- Missing `=` and empty keys
- One-based line/column positions

### Parser and values

- Key outside a section
- Duplicate sections and duplicate keys
- Exact lowercase booleans
- Signed `int64_t` boundaries and overflow
- Strings, empty strings, and values containing `=` after the first separator
- Empty arrays, trailing commas, repeated commas, nested arrays
- Homogeneous integer, boolean, and string arrays
- Mixed-type arrays
- Unexpected token sequences

### Document and integration

- Successful const lookup
- Missing section/key behavior
- Ownership after source text and token storage are destroyed
- File-loading failure versus syntax failure
- CLI input path and exit status

## Positive aspects

- The code uses `std::variant` for the primary success/error boundary.
- The document owns copied strings and values rather than referring to input storage.
- The parser avoids exceptions as its public control-flow mechanism.
- The implementation is small enough to refactor incrementally.
- The existing plan and constraints provide a useful target contract; the next step is to make the implementation enforce it consistently.
