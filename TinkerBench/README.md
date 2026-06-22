# TinkerBench

A performance comparison between **`TK::TZSet`** (this project's span-based
skiplist + dict dual-index) and **disque's `skiplist.c`** — the standalone
extract of Redis's skiplist algorithm (same author, same `span` fields, no
internal-coupling).

Built on [Google Benchmark](https://github.com/google/benchmark) (v1.8.5,
vendored as source — no CMake, no package manager required).

---

## Why these two?

`TK::TZSet` and disque's skiplist implement the **same algorithm family**
(span-augmented probabilistic skip list), so the comparison isolates
*implementation* differences (pointer layout, loop tightness, the dict
acceleration TK layers on top) rather than asymptotic class.

`std::map` is deliberately *not* included: it cannot do O(log n) rank access,
so it would only be a baseline for basic ops, not a peer for the ZSet story.

## What's compared

| Operation | TK API | Disque equivalent | Note |
|-----------|--------|-------------------|------|
| Insert (sequential) | `TZSet::Insert` | `DisqueZSet::Insert` | build cost |
| Insert (shuffled) | same | same | realistic order |
| Find by key | `FindByKey` (O(1) via dict) | `FindByKey` (O(log n), no dict) | **asymmetric by design** — isolates dict benefit |
| Erase (half) | `Erase` | `Erase` | single-point delete |
| **At by rank** | `FindByRank` (O(log n)) | `At` (O(log n)) | core ZSet op |
| **GetRank** | `GetRank` | `GetRank` | span accumulation |
| First in lower bound | `GetFirstInLowerBound` | `GetFirstInLowerBound` | range entry |
| **Count in range** | `GetCountInRange` (O(log n)) | `GetCountInRange` (O(log n + k)) | **TK's one-shot-count win** |
| Erase range | `EraseByRange` | `EraseRange` | bulk delete |
| Forward iterate | range-for | level-0 walk | scan cost |

Data sizes: `2^10 … 2^20` (1024 to 1M elements), via `->Range(1<<10, 1<<20)`.

## Fairness guarantees

- **Identical input data**: a fixed seed (`0x533D1C`) generates the key set;
  both structures receive byte-for-byte identical inserts/queries.
- **Excluded setup**: `PauseTiming`/`ResumeTiming` wraps table construction in
  the erase-range / erase-half benchmarks so only the operation under test
  is measured.
- **Same machine, same build config**: run both in the same Release binary;
  only one PRNG seed, so level distributions are statistically comparable.

## Build

The project is a Visual Studio `.vcxproj` (no CMake). Open `Tinker.sln` and
build `TinkerBench` (Release x64 recommended for meaningful numbers), or from
the command line:

```cmd
msbuild TinkerBench\TinkerBench.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%CD%\\"
```

Requirements: VS 2022 (toolset v143), Windows 10 SDK. No external dependencies
to install — both Google Benchmark and disque's skiplist are vendored under
`third_party\`.

## Run

```cmd
:: All benchmarks at all sizes (default min-time):
TinkerBench\bin\Release\TinkerBench.exe

:: Filter to a single operation family:
TinkerBench.exe --benchmark_filter=BM_TK_AtByRank|BM_Disque_AtByRank

:: Shorter runs during development:
TinkerBench.exe --benchmark_min_time=0.5s

:: JSON output for comparison tools:
TinkerBench.exe --benchmark_format=json --benchmark_out=results.json

:: Reproducibility: pin thread + disable cpu scaling effects:
TinkerBench.exe --benchmark_repetitions=5 --benchmark_report_aggregates_only=true
```

## Interpreting results

A few things to keep in mind when reading the table:

- **`FindByKey` is asymmetric on purpose.** TK wins hard here (O(1) dict vs
  O(log n) skiplist walk). That's the *value of the dual-index design*, not
  a skiplist algorithm difference. If you want to compare the raw skiplist
  find cost, mentally subtract the dict advantage.
- **`GetCountInRange` should favor TK.** TK computes the count from span
  arithmetic in O(log n) without walking the range; disque must walk every
  matching node (O(log n + k)). The gap widens with range width `k`.
- **`At`/`GetRank` should be close.** Same algorithm; differences are
  implementation overhead (TK's templated node layout vs disque's C struct,
  branch profile, inlining).
- **Insert** differences are usually dominated by allocator behavior (TK's
  per-node `malloc` + flexible array vs disque's per-node `zmalloc`). Both
  are single-allocation-per-node, so expect them within ~30% of each other.

## Project layout

```
TinkerBench\
├── TinkerBench.vcxproj          # MSBuild project (Application, /MD, C++17)
├── TinkerBench.vcxproj.filters
├── bench_main.cpp               # Google Benchmark entry point
├── bench_common.h               # shared fixed-seed data generators
├── bench_insert.cpp             # Insert (sequential + shuffled)
├── bench_rank.cpp               # At(rank) + GetRank
├── bench_range.cpp              # LowerBound / CountInRange / EraseRange
├── bench_erase.cpp              # FindByKey / EraseHalf / Iterate
├── adapters\
│   ├── DisqueShim.cpp           # zmalloc/zfree -> malloc/free
│   └── DisqueZSet.h             # disque skiplist C++ adapter
└── third_party\
    ├── disque\                  # skiplist.{c,h} (BSD-3, +rank helpers)
    └── benchmark\               # google/benchmark v1.8.5 source
```

## Licenses

- `TK::TZSet` — this project's license.
- `third_party\disque\skiplist.{c,h}` — BSD-3-Clause (Salvatore Sanfilippo /
  Pieter Noordhuis, vendored from antirez/disque).
- `third_party\benchmark\` — Apache-2.0 (Google).
