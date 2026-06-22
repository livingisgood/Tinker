// DisqueZSet.h
//
// Thin C++ adapter around disque/skiplist.c (Redis's skiplist algorithm) that
// exposes an API surface comparable to TK::TZSet for benchmark purposes.
//
// Design notes:
//   - disque's skiplist sorts by a caller-provided compare() on void* obj.
//     We make obj a heap-allocated Entry { int64_t Key; double Value; } and
//     compare by Value first, then Key — matching TK::TZSet's (Value, Key)
//     ordering so both structures hold identical logical content.
//   - rank/range ops use the span-based helpers (skiplistGetRank /
//     skiplistAtRank) we appended to disque's skiplist.c — they are NOT part
//     of the original disque API but follow the same Redis idiom.
//   - disque has no O(1) member lookup (no dict). FindByKey is therefore
//     O(log n) via skiplistFind, NOT O(1) like TZSet. This asymmetry is
//     intentional: it isolates the *skiplist algorithm* cost from the *dict
//     acceleration* that TZSet layers on top. A separate bench target can
//     measure the dict benefit if desired.

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include "../third_party/disque/skiplist.h"

namespace TinkerBench
{

struct DisqueEntry
{
    int64_t Key;
    double Value;
};

// Comparison: Value ascending, ties broken by Key ascending — mirrors
// TK::TSkipListDefaultComparer<double> + std::less<int64_t>.
inline int DisqueCompare(const void* A, const void* B)
{
    const DisqueEntry* Ea = static_cast<const DisqueEntry*>(A);
    const DisqueEntry* Eb = static_cast<const DisqueEntry*>(B);

    if (Ea->Value < Eb->Value) return -1;
    if (Ea->Value > Eb->Value) return 1;
    if (Ea->Key < Eb->Key) return -1;
    if (Ea->Key > Eb->Key) return 1;
    return 0;
}

class DisqueZSet
{
public:
    DisqueZSet() { Sl = skiplistCreate(&DisqueCompare); }

    ~DisqueZSet()
    {
        // Free every owned Entry, then the list itself.
        skiplistNode* x = Sl->header->level[0].forward;
        while (x)
        {
            delete static_cast<DisqueEntry*>(x->obj);
            x = x->level[0].forward;
        }
        skiplistFree(Sl);
    }

    DisqueZSet(const DisqueZSet&) = delete;
    DisqueZSet& operator=(const DisqueZSet&) = delete;

    // Returns true if inserted, false if duplicate (disque ignores dups).
    bool Insert(int64_t Key, double Value)
    {
        auto* E = new DisqueEntry{ Key, Value };
        skiplistNode* Node = skiplistInsert(Sl, E);
        if (!Node)
        {
            delete E; // duplicate, disque returned NULL
            return false;
        }
        return true;
    }

    bool Erase(int64_t Key, double Value)
    {
        DisqueEntry Query{ Key, Value };

        // disque's skiplistDelete frees the node but NOT the obj (ownership
        // stays with the caller). So we must locate the node, grab its obj,
        // free the obj, then delete the node. The extra locate walk is O(log n)
        // — same order as the delete itself — so it does not change the
        // complexity story of the benchmark.
        skiplistNode* x = Sl->header;
        for (int i = Sl->level - 1; i >= 0; --i)
        {
            while (x->level[i].forward &&
                   DisqueCompare(x->level[i].forward->obj, &Query) < 0)
                x = x->level[i].forward;
        }
        x = x->level[0].forward;
        if (x && DisqueCompare(x->obj, &Query) == 0)
        {
            delete static_cast<DisqueEntry*>(x->obj);
            int Removed = skiplistDelete(Sl, &Query);
            return Removed != 0;
        }
        return false;
    }

    // O(log n) — no dict. Asymmetric with TZSet::FindByKey on purpose.
    const DisqueEntry* FindByKey(int64_t Key, double Value) const
    {
        DisqueEntry Query{ Key, Value };
        void* Obj = skiplistFind(Sl, &Query);
        return static_cast<DisqueEntry*>(Obj);
    }

    // 0-based rank of (Key, Value), or -1 if not found.
    int64_t GetRank(int64_t Key, double Value) const
    {
        DisqueEntry Query{ Key, Value };
        unsigned long R = skiplistGetRank(Sl, &Query);
        return R ? static_cast<int64_t>(R) - 1 : -1; // 1-based -> 0-based
    }

    // 0-based index access. Returns nullptr if out of range.
    const DisqueEntry* At(int64_t Index) const
    {
        if (Index < 0 || static_cast<unsigned long>(Index) >= Sl->length)
            return nullptr;
        skiplistNode* N = skiplistAtRank(Sl, static_cast<unsigned long>(Index) + 1);
        return N ? static_cast<DisqueEntry*>(N->obj) : nullptr;
    }

    size_t GetSize() const { return Sl->length; }

    // Direct access for iteration benchmarks (the C struct is exposed since
    // the benchmark walks the level-0 forward chain directly to mirror
    // TK::TZSet's range-for cost). Returns the internal skiplist pointer.
    skiplist* SlAccess() { return Sl; }
    const skiplist* SlAccess() const { return Sl; }

    // First entry whose (Value,Key) >= LowerBound (inclusive semantics).
    // Walks from head skipping entries strictly less than the bound.
    const DisqueEntry* GetFirstInLowerBound(double LowerValue) const
    {
        DisqueEntry Query{ INT64_MIN, LowerValue };
        skiplistNode* x = Sl->header;
        for (int i = Sl->level - 1; i >= 0; --i)
        {
            while (x->level[i].forward &&
                   DisqueCompare(x->level[i].forward->obj, &Query) < 0)
                x = x->level[i].forward;
        }
        x = x->level[0].forward;
        return x ? static_cast<DisqueEntry*>(x->obj) : nullptr;
    }

    // Last entry whose Value <= UpperValue (inclusive).
    const DisqueEntry* GetLastInUpperBound(double UpperValue) const
    {
        DisqueEntry Query{ INT64_MAX, UpperValue };
        skiplistNode* x = Sl->header;
        for (int i = Sl->level - 1; i >= 0; --i)
        {
            while (x->level[i].forward &&
                   DisqueCompare(x->level[i].forward->obj, &Query) <= 0)
                x = x->level[i].forward;
        }
        return x != Sl->header ? static_cast<DisqueEntry*>(x->obj) : nullptr;
    }

    // Count entries with Value in [LowerValue, UpperValue].
    // disque has no span shortcut for this, so it's O(log n + answer). This
    // is the honest cost of the original algorithm; TK::TZSet does it in
    // O(log n) via span arithmetic, which this comparison will expose.
    size_t GetCountInRange(double LowerValue, double UpperValue) const
    {
        size_t Count = 0;
        const DisqueEntry* First = GetFirstInLowerBound(LowerValue);
        if (!First) return 0;
        // Walk forward until Value > UpperValue.
        DisqueEntry Query{ INT64_MIN, LowerValue };
        skiplistNode* x = Sl->header;
        for (int i = Sl->level - 1; i >= 0; --i)
        {
            while (x->level[i].forward &&
                   DisqueCompare(x->level[i].forward->obj, &Query) < 0)
                x = x->level[i].forward;
        }
        x = x->level[0].forward;
        while (x && static_cast<DisqueEntry*>(x->obj)->Value <= UpperValue)
        {
            ++Count;
            x = x->level[0].forward;
        }
        return Count;
    }

    // Erase all entries with Value in [LowerValue, UpperValue].
    size_t EraseRange(double LowerValue, double UpperValue)
    {
        size_t Removed = 0;
        // Collect then remove: disque's delete invalidates forward pointers,
        // so we walk level 0 collecting matching objs, then delete each.
        std::vector<DisqueEntry> ToRemove;
        skiplistNode* x = Sl->header->level[0].forward;
        while (x && static_cast<DisqueEntry*>(x->obj)->Value < LowerValue)
            x = x->level[0].forward;
        while (x)
        {
            auto* E = static_cast<DisqueEntry*>(x->obj);
            if (E->Value > UpperValue) break;
            ToRemove.push_back(*E);
            x = x->level[0].forward;
        }
        for (const auto& E : ToRemove)
        {
            if (Erase(E.Key, E.Value))
                ++Removed;
        }
        return Removed;
    }

private:
    skiplist* Sl;
};

} // namespace TinkerBench
