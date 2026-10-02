# Week 4 — Log Analyzer: Implementation Plan (C++20 Ranges & Views)

## 1. Project Summary

The goal for Week 4 is to build a **log analyzer** that reads log lines, filters
them down to warnings/errors, extracts structured fields from each matching
line, and produces summary counts — all expressed as a **composed range
pipeline** rather than a sequence of manual loops.

This project exists to build fluency with the C++20 ranges library
(`<ranges>`, `<algorithm>`, `<iterator>`) and specifically to internalize the
**view/adaptor model**: pipelines built from lazy, non-owning views
(`views::filter`, `views::transform`, `views::take`, `views::drop`, etc.)
that only produce values when iterated, as opposed to eagerly-built owning
containers (`std::vector`, `std::string`).

Where possible, this reuses the log-parsing or reporting logic from an
earlier week (Week 1 or Week 2) as the "before" implementation, and this
project becomes the "after" — a refactor that replaces hand-rolled loops
with a declarative pipeline.

A secondary deliverable is a short, informal **benchmark/comparison**
between the loop-based version and the ranges-based version. The purpose is
not performance optimization — it's to build intuition for what ranges cost
or save in terms of readability, compile time, and (roughly) runtime
behavior.

## 2. Learning Objectives Mapped to the Project

| Goal | Where it shows up in the project |
|---|---|
| Learn the ranges library and view/adaptor model | Core pipeline: read → filter → transform → take/drop → summarize |
| Replace manual loops with composable pipelines | Direct refactor of the Week 1/2 loop-based parser |
| Distinguish owning containers vs. lazy views | Explicit separation between "load" (owning) and "process" (view) stages |

## 3. Modules / Components

The project is organized as a small set of focused headers (and matching
source/test files), consistent with the header-oriented style used in the
Week 3 concepts library. No code is included here — this section describes
responsibilities and boundaries only.

1. **`log_types`**
   - Defines the vocabulary types the rest of the pipeline operates on:
     a raw log line abstraction (or just `std::string_view` usage
     conventions), a `LogLevel` enum (e.g. INFO/WARNING/ERROR/DEBUG), and a
     `LogEntry` struct representing the parsed/extracted fields (timestamp,
     level, source/module, message).
   - This module owns the *data model*, not the parsing logic.

2. **`log_loading`**
   - Responsible for the one owning, eager step in the pipeline: reading
     lines from a file (or stream/istream) into an owning container
     (e.g. `std::vector<std::string>`).
   - This is intentionally kept separate and eager, so the boundary between
     "owning data" and "lazy view over data" is explicit and easy to point
     to when discussing the view/adaptor model.

3. **`log_parsing`**
   - Contains the field-extraction logic: turning a raw line
     (`std::string`/`std::string_view`) into a `LogEntry`, or into
     `std::optional<LogEntry>` for lines that don't match the expected
     format.
   - Designed to be used inside `views::transform` (and paired with a
     `views::filter` for lines that fail to parse), so this logic is a
     small, pure, composable function rather than something embedded in a
     loop body.

4. **`log_pipeline`**
   - The heart of the Week 4 work: free functions (or a small set of
     range-adaptor-returning helpers) that compose `views::filter`,
     `views::transform`, `views::take`, and `views::drop` into named,
     reusable pipeline stages, e.g.:
     - a stage that filters raw lines down to warnings/errors,
     - a stage that maps matching lines to `LogEntry` values,
     - optional windowing stages (`take`/`drop`) for things like "first N
       errors" or "skip the first N lines as a header/warm-up period".
   - This module is where the "manual loop → pipeline" rewrite actually
     lives, and where the loop-based vs. ranges-based comparison will be
     drawn most directly.

5. **`log_summary`**
   - Consumes the pipeline output and produces aggregate results: counts by
     level, counts by source/module, or similar. This is the terminal
     ("sink") stage where lazy views finally get evaluated (e.g. via
     `ranges::for_each`, `ranges::distance`, or accumulation into a map).
   - Kept separate from `log_pipeline` so the plan clearly distinguishes
     "building a lazy computation" from "forcing/consuming it."

6. **`main` / CLI driver**
   - Wires the modules together for a runnable demo: load lines →
     build the pipeline → summarize → print a report.
   - Also hosts (or calls into) the loop-based counterpart used for the
     Task 3 benchmark/comparison, so both implementations are runnable
     from the same entry point for side-by-side inspection.

7. **`tests`**
   - Unit tests per module (parsing correctness, pipeline filtering
     correctness, summary correctness), plus one small comparison test or
     scratch file used for the loop-vs-ranges benchmark task.

## 4. Library Structuring

Suggested directory layout (mirrors the header-oriented structure from the
Week 3 project, adapted for this week's scope):

```
week4-log-analyzer/
├── include/
│   └── loganalyzer/
│       ├── log_types.hpp
│       ├── log_loading.hpp
│       ├── log_parsing.hpp
│       ├── log_pipeline.hpp
│       └── log_summary.hpp
├── src/
│   └── main.cpp
├── tests/
│   ├── test_log_parsing.*
│   ├── test_log_pipeline.*
│   └── test_log_summary.*
├── data/
│   └── sample.log          (small fixture log file for manual runs/tests)
├── bench/
│   └── loop_vs_ranges.*     (Task 3: side-by-side loop vs. ranges comparison)
└── CMakeLists.txt
```

Design intent behind this structuring:
- **`log_types` has no dependencies** on the other modules — it's the
  shared vocabulary everything else builds on.
- **`log_loading` is the only module that performs I/O and owns memory**
  in the eager sense. Everything downstream of it operates on views over
  that owned data, which keeps the "owning vs. lazy" distinction visible
  at the architecture level, not just in isolated code snippets.
- **`log_pipeline` depends only on `log_types` and `log_parsing`**, and
  exposes range-returning functions rather than containers, so callers can
  keep composing (`take`, `drop`, further `filter`/`transform`) before
  anything is materialized.
- **`log_summary` is the only place results get materialized** (counted,
  copied into a map, etc.), keeping the "when does laziness end" question
  answerable by pointing at one module.
- **`bench/` is separate from `tests/`** since its purpose is
  understanding/comparison (per the task's explicit "not for premature
  optimization" framing), not correctness verification.

## 5. Implementation Notes / Considerations

- **Header-only vs. compiled**: Given the Week 3 project was header-only,
  this week can either continue that convention for `log_pipeline` and
  `log_parsing` (template-heavy, view-composing code is often natural as
  header-only), or split declarations/definitions where a component (like
  `log_loading`, which is mostly non-template I/O code) doesn't need to be.
  This is a small decision to make during implementation, not up front.
- **`views::filter` + `views::transform` ordering**: filtering before
  transforming (raw line → filter on warnings/errors → transform into
  `LogEntry`) avoids paying the parsing cost for lines that will be
  discarded anyway — a good concrete example of laziness paying off, worth
  calling out in the write-up.
- **Handling unparseable lines**: decide whether `log_parsing` should skip,
  log, or propagate lines that don't match the expected format — this
  affects whether `log_pipeline` needs an extra `filter` stage after
  `transform` to drop `std::nullopt` results (e.g. via `views::filter` on
  an `optional`, or `views::transform` + `views::join`/filter combo).
- **`take`/`drop` usage**: these are easiest to demonstrate meaningfully as
  "first N errors" or "skip a warm-up window of lines" rather than forcing
  artificial use — the plan should use them where they naturally fit the
  reporting use case, not just to check a box.
- **Comparing against Week 1/2**: before writing new logic, review the
  earlier week's parser/reporting code to identify the exact loop(s) being
  replaced, so the "rewrite" task is a direct, honest comparison rather
  than a from-scratch reimplementation.
- **Benchmark scope**: keep this lightweight — a rough timing comparison
  (e.g. `std::chrono` around both implementations on the same input) and a
  short written note on readability/compile-time/behavior differences is
  sufficient; this is explicitly a learning exercise, not a performance
  investigation.
- **Build setup**: reuse the CMake conventions already established in
  earlier weeks' projects (out-of-source build dir, tests wired via
  CTest) rather than introducing a new build pattern this week.

## 6. Suggested Order of Work

1. Stand up `log_types` and a minimal `log_loading` (read file → vector of
   strings) with a tiny sample log fixture.
2. Port/rewrite the Week 1/2 parsing logic into `log_parsing`, returning
   `std::optional<LogEntry>` per line.
3. Build the loop-based version of the full pipeline first (load → loop →
   filter → parse → count) as the baseline for the benchmark task.
4. Build `log_pipeline` using `views::filter`/`views::transform`, then layer
   in `views::take`/`views::drop` where they fit a real reporting need.
5. Build `log_summary` as the terminal consumer of the pipeline.
6. Wire both versions into `main`/`bench` for side-by-side output and
   timing, and write up the readability/behavior comparison.
7. Add unit tests for parsing, pipeline filtering, and summary correctness.
