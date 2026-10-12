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

This week stays strictly within C++20 — no C++26 features. In the broader
weekly plan, C++26 work shows up much later (reflection, contracts,
sender/receiver, roughly weeks 12+), so Week 4's scope is the ranges library
on its own, done well.

## 2. Learning Objectives Mapped to the Project

| Goal | Where it shows up in the project |
|---|---|
| Learn the ranges library and view/adaptor model | Core pipeline: read → filter → transform → take/drop → summarize |
| Replace manual loops with composable pipelines | Direct refactor of the Week 1/2 loop-based parser |
| Distinguish owning containers vs. lazy views | Explicit separation between "load" (owning) and "process" (view) stages |

## 3. What Needs to Be Done

This is the concrete task list — the "what," not the "how." Each item maps
onto a specific module in Sections 4–5 below.

1. **Lock down the data model** — `LogLevel` enum class, `LogEntry` struct
   (level, timestamp, source/module, message, line number). This has to be
   stable before anything else, since it's the type every view in the
   pipeline will eventually produce.
2. **Loading strategy: eager `vector<string>`** (decided — matches the
   Week 1/2 baseline). `LogSource` reads the whole file up front into an
   owning container. Because numbering has to happen while the file is
   still being walked sequentially (see task #3), the container is really
   `vector<RawLine>` — each line paired with its original position at the
   moment it's read, not bare `string`. This keeps the eager/owning stage
   isolated to `log_loading`, so everything downstream still composes as
   lazy views over that one owning container.
3. **Line numbering lives in `log_loading`, not `log_parsing`** —
   `RawLine { size_t line_number; string text; }` is produced once, as
   each line is read, and carried through the rest of the pipeline.
   `parse_line` stays pure (no state, no I/O) but now takes a `RawLine`
   instead of bare `string_view`, and copies `.line_number` straight into
   `LogEntry` — it doesn't compute or guess the number itself. This
   matters because filtering happens before parsing (task #4): numbering
   downstream of a filter would number "the 3rd warning," not "line 3 of
   the file," which isn't what `LogEntry::line_number` is supposed to mean.
4. **Write the filter predicates separately from the parser** — a cheap
   predicate on raw lines ("does this look like a warning/error before we
   bother parsing it"), plus predicates for the other scoped filters
   (by severity, by timestamp range, by module/source — see Section 9).
   Named, testable predicates rather than inline lambdas pay off once you
   want to unit test them in isolation.
5. **Build the pipeline as composition, not a do-everything class** — a
   small set of functions that each return a view, meant to be chained:
   a stage that filters to warnings/errors, a stage that parses matching
   lines, optional windowing stages. Resist wrapping this in one big
   `Pipeline` class with a dozen methods — that fights the adaptor model
   rather than embracing it (see Section 6).
6. **Implement the summary/aggregation step** — the terminal ("sink")
   stage where the lazy view actually gets consumed, via `ranges::for_each`
   or manual accumulation into an `unordered_map<LogLevel, size_t>`. This
   is also where "top errors" / most-frequent-pattern reporting lives, if
   you take on that stretch feature (Section 9).
7. **Write the report formatter** — takes the finished counts, produces
   printed output. Nothing range-specific here; keep it presentation-only.
8. **Build the loop-based baseline** — needs to exist before or alongside
   the ranges version, since the benchmark/comparison task depends on
   having both to compare.
9. **Time + compare** — rough `std::chrono` wrap around both
   implementations on the same input, plus a short written note on what
   changed: readability, where laziness actually saved work, whether the
   pipeline was harder to debug. Also note, concretely, where eager
   materialization happened vs. where views stayed lazy end-to-end — this
   is one of the week's explicit learning goals.
10. **Write the end-of-week note** — per the overall plan's convention:
    what changed, what felt useful, what still feels unclear. Keep this
    short; it's meant to be a quick retro, not a report.

## 4. Modules / Components

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
     the whole file into `std::vector<RawLine>`, pairing each line's text
     with its 1-based position in the file as it's read.
   - This is the only module that performs I/O, the only module that owns
     memory in the eager sense, and — because numbering requires walking
     the file in order — the only correct place for `line_number` to be
     assigned. Everything downstream operates on lazy views over this one
     owning container, so the owning-vs-lazy boundary sits at exactly this
     module's edge.

3. **`log_parsing`**
   - Contains the field-extraction logic: turning a `RawLine` into a
     `LogEntry`, or into `std::optional<LogEntry>` for lines that don't
     match the expected format. `line_number` is copied through from the
     `RawLine`, not recomputed — `log_parsing` has no concept of position
     in the file, only of individual line content.
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
       errors" or "skip the first N lines as a header/warm-up period",
     - optional scoped filters by severity, timestamp range, or module
       (Section 9 stretch features).
   - This module is where the "manual loop → pipeline" rewrite actually
     lives, and where the loop-based vs. ranges-based comparison will be
     drawn most directly.

5. **`log_summary`**
   - Consumes the pipeline output and produces aggregate results: counts by
     level, counts by source/module, or similar, plus optionally a
     top-N most-frequent-pattern report. This is the terminal ("sink")
     stage where lazy views finally get evaluated (e.g. via
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

## 5. Class-Level Layout and Interactions

Deliberately light on classes — parsing and pipeline logic are free
functions/namespaces rather than stateful objects, because that's the idiom
ranges actually wants: composable functions returning views, not objects
that "own" a pipeline.

```
LogLevel (enum class)
    DEBUG, INFO, WARNING, ERROR
    + to_string(LogLevel) -> string_view
    + from_string(string_view) -> optional<LogLevel>

RawLine (struct — plain data, pairs text with its original file position)
    size_t    line_number    // 1-based, assigned once, while reading
    string    text

LogEntry (struct — plain data, no behavior)
    LogLevel level
    string    timestamp
    string    source
    string    message
    size_t    line_number    // copied from RawLine.line_number during parsing, never recomputed

LogSource (class — owns the I/O boundary; eager)
    - opens/reads the file
    + lines() -> vector<RawLine>   // reads the whole file up front, numbering each line as it's read
    (this is the ONLY class that touches the filesystem/istream, and the
     ONLY place line_number is assigned — by construction, not by a counter
     threaded through later stages)

LogParser (free functions, not a class — it's stateless)
    + parse_line(const RawLine&) -> optional<LogEntry>   // copies .line_number into LogEntry, doesn't compute it
    + looks_like_warning_or_error(const RawLine&) -> bool   // cheap pre-filter; inspects .text only

LogPipeline (free functions returning views — not a class)
    + significant_lines(range<RawLine>)  -> filter(looks_like_warning_or_error)
    + parsed_entries(range<RawLine>)     -> significant_lines | transform(parse_line) | filter(has_value) | transform(*)
    + by_level(range<LogEntry>, LogLevel)        -> filter    // stretch: severity scope
    + in_time_range(range<LogEntry>, lo, hi)     -> filter    // stretch: timestamp scope
    + by_module(range<LogEntry>, string_view)    -> filter    // stretch: module scope
    + windowed(range<LogEntry>, n, skip)         -> drop(skip) | take(n)     // optional, only if reporting needs it

LogSummary (class — the one place laziness gets consumed)
    - counts: unordered_map<LogLevel, size_t>
    - pattern_counts: unordered_map<string, size_t>   // only if doing top-N stretch feature
    + LogSummary::from(range<LogEntry>)  -> builds itself via for_each/accumulation
    + count_for(LogLevel) -> size_t
    + total() -> size_t
    + top_n(size_t) -> vector<pair<string, size_t>>   // optional stretch

ReportFormatter (class or free function — presentation only)
    + format(const LogSummary&) -> string
    + print(const LogSummary&, ostream&)
```

**How they interact:** `LogSource` eagerly reads the file once and produces
an owning `vector<RawLine>`, numbering each line as it's read — this is the
only point in the whole pipeline where a position/counter is assigned →
`LogPipeline` functions compose lazy views over that vector and hand back a
`LogEntry` view (nothing is filtered, parsed, or copied until this view is
actually iterated) → `LogSummary::from(...)` is the terminal call that pulls
values through the pipeline and materializes counts, with each `LogEntry`
already carrying the correct `line_number` from its originating `RawLine`
→ `ReportFormatter` reads the finished `LogSummary` and produces output.
Dependency direction is strictly left-to-right in that list —
`LogEntry`/`LogLevel`/`RawLine` are the only things everyone depends on;
`LogSource` knows nothing about parsing; `LogPipeline` knows nothing about
I/O or summarization; `LogSummary` knows nothing about formatting.

This also means filtering never corrupts the line count: because
`line_number` is fixed at read time, a `LogEntry` that survives
`significant_lines | parsed_entries` (and any of the stretch filters)
always reports its true position in the original file, not its position
among survivors.

If you find yourself wanting a `Pipeline` class with a fluent
`.filter().transform().take()` interface, that's really just re-deriving
what `|` composition already gives you for free — worth resisting unless
you specifically want the practice of writing your own adaptor.

## 6. Library Structuring

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
- **`log_loading` is the only module that performs I/O** (and owns memory,
  in the eager case). Everything downstream of it operates on views over
  that data, which keeps the "owning/sourcing vs. lazy" distinction visible
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

## 7. Implementation Notes / Considerations

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
- **Line numbering**: assigned exactly once, in `log_loading`, as part of
  building `vector<RawLine>` — never recomputed downstream. This is a
  direct consequence of going eager: since the whole file is already being
  walked in order to build the vector, numbering is free at that point and
  doesn't need its own pass or a counter threaded through later stages.
- **Staying in scope**: this week is deliberately C++20-only. No
  `std::generator`, no C++23/26 ranges additions — those belong to later
  weeks in the plan. The constraint is useful on its own: it forces the
  owning-vs-lazy distinction to be made with only C++20 tools, which is
  exactly what the week is meant to teach.

## 8. Suggested Order of Work

1. Stand up `log_types` (including `RawLine`) and `log_loading` (read file
   → `vector<RawLine>`, numbering lines as they're read) with a tiny
   sample log fixture.
2. Port/rewrite the Week 1/2 parsing logic into `log_parsing`, taking a
   `RawLine` and returning `std::optional<LogEntry>`, with `line_number`
   copied straight through.
3. Build the loop-based version of the full pipeline first (load → loop →
   filter → parse → count) as the baseline for the benchmark task.
4. Build `log_pipeline` using `views::filter`/`views::transform`, then layer
   in `views::take`/`views::drop` where they fit a real reporting need.
5. Build `log_summary` as the terminal consumer of the pipeline.
6. Wire both versions into `main`/`bench` for side-by-side output and
   timing, and write up the readability/behavior comparison.
7. Add unit tests for parsing, pipeline filtering, and summary correctness.
8. (Stretch, time permitting) Add severity/timestamp/module filtering and
   top-N pattern reporting from Section 9, using the same `log_pipeline`
   style rather than one-off code.
9. Write the short end-of-week note (task #10).

## 9. Scope for This Week (from the overall plan's suggested features)

The broader weekly plan lists these as suggested features for the log
analyzer specifically — treat the first two as core scope and the rest as
stretch goals if time allows within the 5–7 hour weekly budget:

- **Filter by severity, timestamp, or module** — core scope; this is what
  `by_level`, `in_time_range`, and `by_module` in Section 5 are for.
- **Extract tokens and aggregate counts** — core scope; this is
  `log_parsing` + `log_summary`.
- **Output top errors or suspicious patterns** — stretch; a `top_n` on
  `LogSummary` that ranks by frequency.
- **Compare eager materialization with lazy views** — core scope; this is
  the Task 3 benchmark, and should explicitly show a case where an eager
  `vector` was built partway through the pipeline (e.g. for the loop-based
  version) versus a view that stayed lazy until `log_summary` consumed it.
