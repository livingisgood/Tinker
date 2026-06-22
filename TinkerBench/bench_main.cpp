// bench_main.cpp
//
// Entry point for the TinkerBench application. Delegates to Google Benchmark
// for argument parsing, iteration, and result reporting.
//
// Run with no args  : runs all benchmarks at all registered sizes.
// Run with filters  : --benchmark_filter=BM_TK_.*
// Output to JSON    : --benchmark_format=json --benchmark_out=results.json
// Show time in ns   : --benchmark_time_unit=ns

#include "benchmark/benchmark.h"

int main(int argc, char** argv)
{
    // Default to a tighter min-time so the suite finishes faster during
    // development; users can override with --benchmark_min_time on the CLI.
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
