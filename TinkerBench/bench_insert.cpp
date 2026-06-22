// bench_insert.cpp
//
// Benchmarks the Insert path for both structures across data sizes and
// insertion orders. Insert is the most representative "build" operation and
// stresses the skiplist's level generation + span maintenance.
//
// Two flavors per structure:
//   - Sequential insert (worst case for some skiplist variants; best for cache)
//   - Shuffled insert  (realistic random-arrival; stresses balancing)

#include "bench_common.h"
#include "adapters/DisqueZSet.h"
#include "DataStructure/ZSet.h"
#include "benchmark/benchmark.h"

using TKZSet = TK::TZSet<int64_t, double>;

// ---------------------------------------------------------------------------
// TK::TZSet
// ---------------------------------------------------------------------------

static void BM_TK_Insert_Sequential(benchmark::State& State)
{
    const int64_t N = State.range(0);
    for (auto _ : State)
    {
        TKZSet Z;
        for (int64_t i = 0; i < N; ++i)
            Z.Insert(i, static_cast<double>(i));
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_TK_Insert_Sequential)->Range(1 << 10, 1 << 20);

static void BM_TK_Insert_Shuffled(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    for (auto _ : State)
    {
        TKZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_TK_Insert_Shuffled)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// DisqueZSet (Redis algorithm)
// ---------------------------------------------------------------------------

static void BM_Disque_Insert_Sequential(benchmark::State& State)
{
    const int64_t N = State.range(0);
    for (auto _ : State)
    {
        TinkerBench::DisqueZSet Z;
        for (int64_t i = 0; i < N; ++i)
            Z.Insert(i, static_cast<double>(i));
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_Disque_Insert_Sequential)->Range(1 << 10, 1 << 20);

static void BM_Disque_Insert_Shuffled(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    for (auto _ : State)
    {
        TinkerBench::DisqueZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_Disque_Insert_Shuffled)->Range(1 << 10, 1 << 20);
