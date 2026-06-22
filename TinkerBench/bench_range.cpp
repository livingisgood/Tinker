// bench_range.cpp
//
// Benchmarks the range-query operations. This is where a span-based skiplist
// is meant to shine vs std::map (TK computes GetCountInRange in O(log n) via
// span arithmetic; std::map would need O(log n + answer)). Here TK and
// disque both have spans but only TK exposes a one-shot count, while disque
// must walk the range — so this bench also measures the cost of TK's
// count-without-walk optimization.

#include "bench_common.h"
#include "adapters/DisqueZSet.h"
#include "DataStructure/ZSet.h"
#include "benchmark/benchmark.h"

using TKZSet = TK::TZSet<int64_t, double>;

// ---------------------------------------------------------------------------
// GetFirstInLowerBound — range entry point
// ---------------------------------------------------------------------------

static void BM_TK_FirstInLowerBound(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TKZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    // A spread of lower-bound targets covering the value range.
    std::vector<double> Bounds;
    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_real_distribution<double> Dist(0.0, static_cast<double>(N));
    for (int i = 0; i < 256; ++i)
        Bounds.push_back(Dist(Rng));

    size_t i = 0;
    for (auto _ : State)
    {
        TK::TRangeBound<double> B{ Bounds[i & 255], false };
        auto [Node, Rank] = Z.GetFirstInLowerBound(B);
        benchmark::DoNotOptimize(Node);
        benchmark::DoNotOptimize(Rank);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_TK_FirstInLowerBound)->Range(1 << 10, 1 << 20);

static void BM_Disque_FirstInLowerBound(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TinkerBench::DisqueZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    std::vector<double> Bounds;
    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_real_distribution<double> Dist(0.0, static_cast<double>(N));
    for (int i = 0; i < 256; ++i)
        Bounds.push_back(Dist(Rng));

    size_t i = 0;
    for (auto _ : State)
    {
        auto* E = Z.GetFirstInLowerBound(Bounds[i & 255]);
        benchmark::DoNotOptimize(E);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_Disque_FirstInLowerBound)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// GetCountInRange — TK does it in O(log n); disque must walk the answer.
// This is the single most asymmetric operation in the comparison.
// ---------------------------------------------------------------------------

static void BM_TK_CountInRange(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TKZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    // Fixed (lower, upper) pairs covering ~10% slices of the value space.
    std::vector<std::pair<double, double>> Ranges;
    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_real_distribution<double> Dist(0.0, static_cast<double>(N));
    for (int i = 0; i < 128; ++i)
    {
        double A = Dist(Rng);
        double B = Dist(Rng);
        if (A > B) std::swap(A, B);
        Ranges.emplace_back(A, B);
    }

    size_t i = 0;
    for (auto _ : State)
    {
        const auto& [Lo, Hi] = Ranges[i & 127];
        TK::TRange<double> R{ { Lo, false }, { Hi, false } };
        auto C = Z.GetCountInRange(R);
        benchmark::DoNotOptimize(C);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_TK_CountInRange)->Range(1 << 10, 1 << 20);

static void BM_Disque_CountInRange(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TinkerBench::DisqueZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    std::vector<std::pair<double, double>> Ranges;
    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_real_distribution<double> Dist(0.0, static_cast<double>(N));
    for (int i = 0; i < 128; ++i)
    {
        double A = Dist(Rng);
        double B = Dist(Rng);
        if (A > B) std::swap(A, B);
        Ranges.emplace_back(A, B);
    }

    size_t i = 0;
    for (auto _ : State)
    {
        const auto& [Lo, Hi] = Ranges[i & 127];
        auto C = Z.GetCountInRange(Lo, Hi);
        benchmark::DoNotOptimize(C);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_Disque_CountInRange)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// EraseRange — bulk delete of a value slice
// ---------------------------------------------------------------------------

static void BM_TK_EraseRange(benchmark::State& State)
{
    const int64_t N = State.range(0);
    const double Lo = N * 0.25;
    const double Hi = N * 0.75; // erase the middle half
    for (auto _ : State)
    {
        State.PauseTiming();
        auto Data = TinkerBench::GenShuffled(N);
        TKZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        State.ResumeTiming();

        TK::TRange<double> R{ { Lo, false }, { Hi, false } };
        auto Removed = Z.EraseByRange(R);
        benchmark::DoNotOptimize(Z);
        benchmark::DoNotOptimize(Removed);
    }
    State.SetItemsProcessed(State.iterations() * N / 2);
}
BENCHMARK(BM_TK_EraseRange)->Range(1 << 10, 1 << 20);

static void BM_Disque_EraseRange(benchmark::State& State)
{
    const int64_t N = State.range(0);
    const double Lo = N * 0.25;
    const double Hi = N * 0.75;
    for (auto _ : State)
    {
        State.PauseTiming();
        auto Data = TinkerBench::GenShuffled(N);
        TinkerBench::DisqueZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        State.ResumeTiming();

        auto Removed = Z.EraseRange(Lo, Hi);
        benchmark::DoNotOptimize(Z);
        benchmark::DoNotOptimize(Removed);
    }
    State.SetItemsProcessed(State.iterations() * N / 2);
}
BENCHMARK(BM_Disque_EraseRange)->Range(1 << 10, 1 << 20);
