// bench_common.h
//
// Shared utilities for the TinkerBench project:
//   - Fixed-seed PRNG so TK::TZSet and DisqueZSet see identical data.
//   - A key/value set generator (sequential or shuffled).
//   - Sanity-check helpers used by benchmark fixtures.

#pragma once

#include <cstdint>
#include <random>
#include <vector>
#include <algorithm>

namespace TinkerBench
{

// Deterministic seed: same across TK and disque benches so the input sets
// are byte-for-byte identical. The only fair way to compare two structures.
inline constexpr uint32_t kSeed = 0x533D1C;

// Generate `count` (Key, Value) pairs where Value = Key * 1.0. The ordering
// is (Value, Key) and since Value==Key here it collapses to Key ordering,
// which is the simplest "all-distinct, fully ordered" dataset.
inline std::vector<std::pair<int64_t, double>> GenSequential(int64_t Count)
{
    std::vector<std::pair<int64_t, double>> Out;
    Out.reserve(static_cast<size_t>(Count));
    for (int64_t i = 0; i < Count; ++i)
        Out.emplace_back(i, static_cast<double>(i));
    return Out;
}

// Same set as GenSequential but shuffled with a fixed seed — models the more
// realistic "random arrival order" insertion pattern that stresses the
// skiplist's balancing.
inline std::vector<std::pair<int64_t, double>> GenShuffled(int64_t Count)
{
    auto Out = GenSequential(Count);
    std::mt19937 Rng(kSeed);
    std::shuffle(Out.begin(), Out.end(), Rng);
    return Out;
}

// A fixed-shuffle query set: used so that Find/Erase targets are identical
// across TK and disque. We shuffle the full key range so lookups hit every
// node, not just the head.
inline std::vector<std::pair<int64_t, double>> GenQuerySet(int64_t Count)
{
    return GenShuffled(Count);
}

} // namespace TinkerBench
