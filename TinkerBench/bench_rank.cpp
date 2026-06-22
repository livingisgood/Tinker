// bench_rank.cpp
//
// Benchmarks the rank/index operations — the core value-add of a span-based
// skiplist (what makes it a ZSet rather than a plain sorted container):
//   - At(Index):    O(log n) by-span descent.
//   - GetRank(Key): O(log n) by-span accumulation.
//
// Both structures use the same algorithm family here, so this measures
// implementation overhead (pointer layout, loop tightness, branch profile)
// rather than asymptotic differences.

#include "bench_common.h"
#include "adapters/DisqueZSet.h"
#include "DataStructure/ZSet.h"
#include "benchmark/benchmark.h"

using TKZSet = TK::TZSet<int64_t, double>;

// ---------------------------------------------------------------------------
// At(Index) — random access by rank
// ---------------------------------------------------------------------------

static void BM_TK_AtByRank(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TKZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    // Pre-generated rank targets, uniformly spread + fixed seed.
    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_int_distribution<int64_t> Dist(0, N - 1);
    std::vector<int64_t> Targets;
    for (int i = 0; i < 4096; ++i)
        Targets.push_back(Dist(Rng));

    size_t i = 0;
    for (auto _ : State)
    {
        auto* Node = Z.FindByRank(Targets[i & (Targets.size() - 1)]);
        benchmark::DoNotOptimize(Node);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_TK_AtByRank)->Range(1 << 10, 1 << 20);

static void BM_Disque_AtByRank(benchmark::State& State)
{
    const int64_t N = State.range(0);
    auto Data = TinkerBench::GenShuffled(N);
    TinkerBench::DisqueZSet Z;
    for (auto [k, v] : Data)
        Z.Insert(k, v);

    std::mt19937 Rng(TinkerBench::kSeed);
    std::uniform_int_distribution<int64_t> Dist(0, N - 1);
    std::vector<int64_t> Targets;
    for (int i = 0; i < 4096; ++i)
        Targets.push_back(Dist(Rng));

    size_t i = 0;
    for (auto _ : State)
    {
        auto* E = Z.At(Targets[i & (Targets.size() - 1)]);
        benchmark::DoNotOptimize(E);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_Disque_AtByRank)->Range(1 << 10, 1 << 20);

// ---------------------------------------------------------------------------
// GetRank(Key) — rank lookup by member
// ---------------------------------------------------------------------------

static void BM_TK_GetRank(benchmark::State& State)
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
        const auto& Q = Queries[i % Queries.size()];
        int64_t R = Z.GetRank(Q.first);
        benchmark::DoNotOptimize(R);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_TK_GetRank)->Range(1 << 10, 1 << 20);

static void BM_Disque_GetRank(benchmark::State& State)
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
        int64_t R = Z.GetRank(Q.first, Q.second);
        benchmark::DoNotOptimize(R);
        ++i;
    }
    State.SetItemsProcessed(State.iterations());
}
BENCHMARK(BM_Disque_GetRank)->Range(1 << 10, 1 << 20);
