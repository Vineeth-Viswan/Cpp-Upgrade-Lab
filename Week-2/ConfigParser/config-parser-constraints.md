# Config Parser: Scope and Constraints

This document narrows the Config Parser portion of the original Week 2 plan into a small C++17 learning exercise. It does not add features beyond that plan.

## 1. Learning goal

- Return results by value.
- Represent expected failures as `std::variant<Success, Error>`.
- Do not use mutable output parameters.
- Do not use exceptions for ordinary tokenization or parsing failures.
- Keep interfaces suitable for a later change to `std::expected<Success, Error>` without redesigning the parser or document model.

## 2. Supported features

Only these features from the original plan are supported:

- Sections
- Key-value pairs
- Comments
- Whitespace trimming
- Integer, Boolean, string, and array values

File writing, formatting preservation, interpolation, nested sections, and other configuration-language extensions are out of scope.

## 3. Format constraints

### Sections

- A section header occupies its own line and has the form `[section_name]`.
- The section name must not be empty.
- Whitespace inside the brackets is trimmed.
- Every key-value pair must belong to a section.
- Repeating a section name is an error.
- Section names are case-sensitive.

### Key-value pairs

- A pair has the form `key = value`; the first `=` is the separator.
- The key must not be empty.
- Whitespace around the key and value is trimmed.
- Keys are case-sensitive.
- Repeating a key within a section is an error.

### Comments and blank lines

- A comment is a full line whose first non-whitespace character is `;`.
- Inline comments are not supported.
- Blank lines and comment lines are ignored.

### Scalar values

- An integer is a signed base-10 whole number within the range of `std::int64_t`.
- A Boolean is exactly `true` or `false` in lowercase.
- Any other non-array value is a string after trimming.
- An empty value represents an empty string.
- Quoting and escape sequences are not supported.

### Arrays

- An array has the single-line form `[value1, value2, value3]`.
- Arrays are flat; nested arrays are not supported.
- An array must contain at least one element.
- All elements must have the same scalar type.
- Whitespace around elements is trimmed.
- A string array element cannot contain a comma because quoting is not supported.

## 4. Results and errors

- Tokenization returns either a token sequence or a structured error by value.
- Parsing returns either a complete document or a structured error by value.
- Processing stops at the first error; collecting multiple errors is out of scope.
- An error contains a category, line number, column number when available, and a short message.
- File-loading failures are distinct from syntax failures.

## 5. Parsed document

- A successful parse produces a generic document organized by sections and keys.
- Values retain their parsed type: integer, Boolean, string, or array.
- The document owns all strings and values and offers read-only lookup.
- The document must not depend on the lifetime of input text or tokenizer data.
- Original comments, whitespace, and formatting do not need to be preserved.

## 6. Application settings layer

The application-settings conversion layer from the original plan is omitted. No application-specific schema is defined, and the generic typed document is enough to practice value-oriented parsing and error handling.

If concrete application settings are introduced later, their validation and conversion can be a separate consumer of the document without changing the tokenizer or parser.

## 7. Testing boundary

Tests cover token positions, valid and malformed sections, key-value pairs, trimming, comments, blank lines, all supported value types, arrays, empty input, duplicates, keys outside sections, integer overflow, malformed arrays, and successful document lookup.

CLI integration and the unrelated Week 1 utilities are outside this parser-specific scope.
