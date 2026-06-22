// bench_erase.cpp
//
// Benchmarks the single-point lookup + erase paths and the full forward
// iteration. These are the "shared baseline" operations every sorted
// container supports — the comparison here is pure implementation overhead.
//
// Note on FindByKey asymmetry (intentional):
//   - TK::TZSet::FindByKey is O(1) via the K2V dict.
//   - DisqueZSet::FindByKey is O(log n) (no dict).
//   This isolates the dict-acceleration benefit, NOT the skiplist algorithm.
//   A "TK skiplist-only" mode (using TSkipList directly) would be the
//   apples-to-apples comparison for the find path.

#include "bench_common.h"
#include "adapters/DisqueZSet.h"
#include "DataStructure/ZSet.h"
#include "benchmark/benchmark.h"

using TKZSet = TK::TZSet<int64_t, double>;

// ---------------------------------------------------------------------------
// FindByKey
// ---------------------------------------------------------------------------

static void BM_TK_FindByKey(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TKZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    auto Queries = TinkerBench::GenQuerySet(N);
    size_t i = 0;
    for (auto _ : State)
    {
        auto* Node = Z.FindByKey(Queries[i % Queries.size()].first);
        benchmark::DoNotOptimize(Node);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_TK_FindByKey)->Range(1 << 10, 1 << 20);

static void BM_Disque_FindByKey(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TinkerBench::DisqueZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    auto Queries = TinkerBench::GenQuerySet(N);
    size_t i = 0;
    for (auto _ : State)
    {
        const auto& Q = Queries[i % Queries.size()];
        auto* E = Z.FindByKey(Q.first, Q.second);
        benchmark::DoNotOptimize(E);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_Disque_FindByKey)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// Erase (single, shuffled) — erase half the elements one at a time
// ---------------------------------------------------------------------------

static void BM_TK_EraseHalf(benchmark::State& State)
{
    const int64_t N = State.range(0);
    for (auto _ : State)
    {
        State.PauseTiming();
        auto Data = TinkerBench::GenShuffled(N);
        TKZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        auto ToErase = TinkerBench::GenQuerySet(N / 2);
        State.ResumeTiming();

        for (const auto& [k, v] : ToErase)
            Z.Erase(k);
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * (N / 2));
}
BENCHMARK(BM_TK_EraseHalf)->Range(1 << 10, 1 << 20);

static void BM_Disque_EraseHalf(benchmark::State& State)
{
    const int64_t N = State.range(0);
    for (auto _ : State)
    {
        State.PauseTiming();
        auto Data = TinkerBench::GenShuffled(N);
        TinkerBench::DisqueZSet Z;
        for (auto [k, v] : Data)
            Z.Insert(k, v);
        auto ToErase = TinkerBench::GenQuerySet(N / 2);
        State.ResumeTiming();

        for (const auto& [k, v] : ToErase)
            Z.Erase(k, v);
        benchmark::DoNotOptimize(Z);
    }
    State.SetItemsProcessed(State.iterations() * (N / 2));
}
BENCHMARK(BM_Disque_EraseHalf)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// Forward iteration over the whole list
// ---------------------------------------------------------------------------

static void BM_TK_Iterate(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TKZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    for (auto _ : State)
    {
        int64_t Sum = 0;
        for (const auto& Node : Z)
            Sum += Node.Value;
        benchmark::DoNotOptimize(Sum);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_TK_Iterate)->Range(1 << 10, 1 << 20);

static void BM_Disque_Iterate(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TinkerBench::DisqueZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    for (auto _ : State)
    {
        int64_t Sum = 0;
        // Walk disque's level-0 forward chain directly.
        skiplistNode* x = Z.SlAccess()->header->level[0].forward;
        while (x)
        {
            Sum += static_cast<TinkerBench::DisqueEntry*>(x->obj)->Value;
            x = x->level[0].forward;
        }
        benchmark::DoNotOptimize(Sum);
    }
    State.SetItemsProcessed(State.iterations() * N);
}
BENCHMARK(BM_Disque_Iterate)->Range(1 << 10, 1 << 20);
