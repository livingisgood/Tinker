#include "gtest/gtest.h"
#include "DataStructure/SkipList.h"
#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <numeric>

// ============================================================================
// Test Helpers
// ============================================================================

// Deterministic random func: always returns false => all nodes have level 1
struct FAlwaysFalse { bool operator()() const { return false; } };

// Deterministic random func: always returns true => all nodes reach MaxLevel
struct FAlwaysTrue { bool operator()() const { return true; } };

// Deterministic random func: pre-defined bool sequence to control levels
struct FMockRandFunc
{
	std::vector<bool> Results;
	mutable size_t NextIndex = 0;

	explicit FMockRandFunc(std::vector<bool> InResults) : Results(std::move(InResults)) {}

	bool operator()() const
	{
		if (NextIndex < Results.size())
			return Results[NextIndex++];
		return false;
	}
};

// A value-comparer that reverses order (descending by value)
struct FReverseValueComparer
{
	int operator()(const int& A, const int& B) const
	{
		if (A > B) return -1;
		if (A < B) return 1;
		return 0;
	}
};

// Convenience type aliases
using IntSkipList = TK::TSkipList<int, int>;
using FlatSkipList = TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FAlwaysFalse>;
using TallSkipList = TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FAlwaysTrue, 4>;
using DescSkipList = TK::TSkipList<int, int, FReverseValueComparer>;

// Helper: verify list forward & backward iteration matches expected values in order
template<typename SkipListType>
void VerifyListOrder(const SkipListType& List, const std::vector<int>& ExpectedValues)
{
	ASSERT_EQ(List.GetSize(), static_cast<typename SkipListType::SizeType>(ExpectedValues.size()));

	if (ExpectedValues.empty())
	{
		EXPECT_TRUE(List.IsEmpty());
		return;
	}

	// Forward iteration from At(0)
	auto* Node = List.At(0);
	ASSERT_NE(Node, nullptr);
	for (size_t i = 0; i < ExpectedValues.size(); ++i)
	{
		ASSERT_NE(Node, nullptr) << "Forward: node is null at index " << i;
		EXPECT_EQ(Node->Value, ExpectedValues[i]) << "Forward: value mismatch at index " << i;
		Node = Node->GetNext();
	}
	EXPECT_EQ(Node, nullptr) << "Forward: expected null after last element";

	// Backward iteration from the tail
	Node = List.At(static_cast<int>(ExpectedValues.size()) - 1);
	for (int i = static_cast<int>(ExpectedValues.size()) - 1; i >= 0; --i)
	{
		ASSERT_NE(Node, nullptr) << "Backward: node is null at index " << i;
		EXPECT_EQ(Node->Value, ExpectedValues[i]) << "Backward: value mismatch at index " << i;
		Node = Node->GetPrev();
	}
	EXPECT_EQ(Node, nullptr) << "Backward: expected null before first element";
}

// Helper: verify At() returns correct value for every index
template<typename SkipListType>
void VerifyRandomAccess(const SkipListType& List, const std::vector<int>& ExpectedValues)
{
	for (size_t i = 0; i < ExpectedValues.size(); ++i)
	{
		auto* Node = List.At(static_cast<int>(i));
		ASSERT_NE(Node, nullptr) << "At(" << i << ") returned null";
		EXPECT_EQ(Node->Value, ExpectedValues[i]) << "At(" << i << ") value mismatch";
		// operator[] should match At
		auto* Node2 = List[static_cast<int>(i)];
		EXPECT_EQ(Node2, Node) << "operator[] vs At mismatch at index " << i;
	}
}

// ============================================================================
// 1. Basic Insert & Erase
// ============================================================================

TEST(SkipList, ConstructDefault)
{
	IntSkipList List;
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, ConstructWithValueComparer)
{
	DescSkipList List(FReverseValueComparer{});
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, InsertSingle)
{
	IntSkipList List;
	auto [Node, Ok] = List.Insert(1, 100);
	EXPECT_TRUE(Ok);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_FALSE(List.IsEmpty());
}

TEST(SkipList, InsertMultiple_AscendingOrder)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	EXPECT_EQ(List.GetSize(), 3);

	// Ordered by value: 10, 20, 30
	VerifyListOrder(List, {10, 20, 30});
	VerifyRandomAccess(List, {10, 20, 30});
}

TEST(SkipList, InsertMultiple_UnsortedInput)
{
	IntSkipList List;
	List.Insert(3, 30);
	List.Insert(1, 10);
	List.Insert(2, 20);
	EXPECT_EQ(List.GetSize(), 3);

	// Must be in sorted order by value regardless of insertion order
	VerifyListOrder(List, {10, 20, 30});
	VerifyRandomAccess(List, {10, 20, 30});
}

TEST(SkipList, InsertDuplicate_ExactKeyValue)
{
	IntSkipList List;
	auto [N1, Ok1] = List.Insert(1, 100);
	EXPECT_TRUE(Ok1);

	auto [N2, Ok2] = List.Insert(1, 100);
	EXPECT_FALSE(Ok2);
	EXPECT_EQ(N2, N1); // returns the existing node
	EXPECT_EQ(List.GetSize(), 1); // size unchanged
}

TEST(SkipList, InsertSameValue_DifferentKey)
{
	IntSkipList List;
	// Same value 100, different keys 1 and 2
	// KeyComparer (std::less) breaks the tie: key 1 < key 2
	List.Insert(1, 100);
	List.Insert(2, 100);
	EXPECT_EQ(List.GetSize(), 2);

	// Both have value 100; ordered by key: (1,100) then (2,100)
	auto* First = List.At(0);
	EXPECT_EQ(First->Key, 1);
	EXPECT_EQ(First->Value, 100);

	auto* Second = List.At(1);
	EXPECT_EQ(Second->Key, 2);
	EXPECT_EQ(Second->Value, 100);
}

TEST(SkipList, EraseExisting_First)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	bool Ok = List.Erase(1, 10);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {20, 30});
}

TEST(SkipList, EraseExisting_Middle)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	bool Ok = List.Erase(2, 20);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 30});
}

TEST(SkipList, EraseExisting_Last)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	bool Ok = List.Erase(3, 30);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
}

TEST(SkipList, EraseNonExisting_KeyNotFound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	bool Ok = List.Erase(3, 30);
	EXPECT_FALSE(Ok);
	EXPECT_EQ(List.GetSize(), 2);
}

TEST(SkipList, EraseNonExisting_ValueNotFound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	bool Ok = List.Erase(1, 20); // key=1 has value=10, not 20
	EXPECT_FALSE(Ok);
	EXPECT_EQ(List.GetSize(), 2);
}

TEST(SkipList, EraseFromEmpty)
{
	IntSkipList List;
	bool Ok = List.Erase(1, 10);
	EXPECT_FALSE(Ok);
	EXPECT_EQ(List.GetSize(), 0);
}

TEST(SkipList, EraseAll_UntilEmpty)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_TRUE(List.Erase(1, 10));
	EXPECT_TRUE(List.Erase(2, 20));
	EXPECT_TRUE(List.Erase(3, 30));

	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseThenInsert_SameKey)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	List.Erase(1, 10);
	auto [Node, Ok] = List.Insert(1, 10);
	EXPECT_TRUE(Ok);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 10);
	VerifyListOrder(List, {10, 20});
}

// ============================================================================
// 2. Value Semantics (Copy, Move, Swap, Clear)
// ============================================================================

TEST(SkipList, CopyConstructor)
{
	IntSkipList Original;
	Original.Insert(1, 10);
	Original.Insert(2, 20);
	Original.Insert(3, 30);

	IntSkipList Copy(Original);
	EXPECT_EQ(Copy.GetSize(), 3);
	VerifyListOrder(Copy, {10, 20, 30});

	// Verify deep copy: modify original, copy unchanged
	Original.Erase(2, 20);
	EXPECT_EQ(Original.GetSize(), 2);
	EXPECT_EQ(Copy.GetSize(), 3);
	VerifyListOrder(Copy, {10, 20, 30});
}

TEST(SkipList, CopyConstructor_Empty)
{
	IntSkipList Original;
	IntSkipList Copy(Original);
	EXPECT_EQ(Copy.GetSize(), 0);
	EXPECT_TRUE(Copy.IsEmpty());
}

TEST(SkipList, CopyAssignment)
{
	IntSkipList Original;
	Original.Insert(1, 10);
	Original.Insert(2, 20);

	IntSkipList Copy;
	Copy.Insert(3, 30); // existing data should be freed
	Copy = Original;

	EXPECT_EQ(Copy.GetSize(), 2);
	VerifyListOrder(Copy, {10, 20});
}

TEST(SkipList, CopyAssignment_SelfAssignment)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	List = List; // NOLINT: self-assignment test
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
}

TEST(SkipList, MoveConstructor)
{
	IntSkipList Original;
	Original.Insert(1, 10);
	Original.Insert(2, 20);

	IntSkipList Moved(std::move(Original));
	EXPECT_EQ(Moved.GetSize(), 2);
	VerifyListOrder(Moved, {10, 20});

	// Original should be in a valid but empty state (moved-from)
	EXPECT_EQ(Original.GetSize(), 0);
	EXPECT_TRUE(Original.IsEmpty());
}

TEST(SkipList, MoveAssignment)
{
	IntSkipList Original;
	Original.Insert(1, 10);
	Original.Insert(2, 20);

	IntSkipList Moved;
	Moved.Insert(3, 99);
	Moved = std::move(Original);

	EXPECT_EQ(Moved.GetSize(), 2);
	VerifyListOrder(Moved, {10, 20});
}

TEST(SkipList, Clear)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	List.Clear();
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());

	// List should be usable after Clear
	auto [Node, Ok] = List.Insert(1, 100);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList, Swap)
{
	IntSkipList A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntSkipList B;
	B.Insert(3, 30);
	B.Insert(4, 40);
	B.Insert(5, 50);

	A.Swap(B);

	EXPECT_EQ(A.GetSize(), 3);
	EXPECT_EQ(B.GetSize(), 2);
	VerifyListOrder(A, {30, 40, 50});
	VerifyListOrder(B, {10, 20});
}

// ============================================================================
// 3. Ordering & Comparers
// ============================================================================

TEST(SkipList, ValueBasedOrdering)
{
	// Key order differs from value order
	// Insert: key=1->val=300, key=2->val=100, key=3->val=200
	IntSkipList List;
	List.Insert(1, 300);
	List.Insert(2, 100);
	List.Insert(3, 200);

	// Sorted by value: 100, 200, 300
	VerifyListOrder(List, {100, 200, 300});

	// Verify the keys are correct at each position
	EXPECT_EQ(List.At(0)->Key, 2);
	EXPECT_EQ(List.At(1)->Key, 3);
	EXPECT_EQ(List.At(2)->Key, 1);
}

TEST(SkipList, KeyTiebreaker)
{
	// Same value, different keys => ordered by key
	IntSkipList List;
	List.Insert(3, 100);
	List.Insert(1, 100);
	List.Insert(2, 100);

	EXPECT_EQ(List.GetSize(), 3);
	EXPECT_EQ(List.At(0)->Key, 1);
	EXPECT_EQ(List.At(1)->Key, 2);
	EXPECT_EQ(List.At(2)->Key, 3);
}

TEST(SkipList, ReverseValueComparer)
{
	DescSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Reverse ordering by value: 30, 20, 10
	EXPECT_EQ(List.GetSize(), 3);
	EXPECT_EQ(List.At(0)->Value, 30);
	EXPECT_EQ(List.At(1)->Value, 20);
	EXPECT_EQ(List.At(2)->Value, 10);

	// Verify prev/next links with reverse ordering
	auto* First = List.At(0);
	ASSERT_NE(First, nullptr);
	EXPECT_EQ(First->GetPrev(), nullptr);

	auto* Second = First->GetNext();
	ASSERT_NE(Second, nullptr);
	EXPECT_EQ(Second->Value, 20);
	EXPECT_EQ(Second->GetPrev(), First);

	auto* Third = Second->GetNext();
	ASSERT_NE(Third, nullptr);
	EXPECT_EQ(Third->Value, 10);
	EXPECT_EQ(Third->GetNext(), nullptr);
}

// ============================================================================
// 4. Random Access (At, operator[])
// ============================================================================

TEST(SkipList, At_FirstAndLast)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto* First = List.At(0);
	ASSERT_NE(First, nullptr);
	EXPECT_EQ(First->Value, 10);

	auto* Last = List.At(2);
	ASSERT_NE(Last, nullptr);
	EXPECT_EQ(Last->Value, 30);
}

TEST(SkipList, At_Middle)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 10);
	}
	// Values are 0, 10, 20, ... 190 at indices 0..19
	for (int i = 0; i < 20; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(Node->Value, i * 10) << "i=" << i;
	}
}

TEST(SkipList, At_OutOfBounds)
{
	IntSkipList List;
	List.Insert(1, 10);

	EXPECT_EQ(List.At(-1), nullptr);
	EXPECT_EQ(List.At(1), nullptr);   // size=1, index 1 is out of bounds
	EXPECT_EQ(List.At(100), nullptr);
}

TEST(SkipList, At_Empty)
{
	IntSkipList List;
	EXPECT_EQ(List.At(0), nullptr);
	EXPECT_EQ(List.At(-1), nullptr);
}

TEST(SkipList, At_NearSearchFront)
{
	// "near search" threshold is 10 elements
	IntSkipList List;
	for (int i = 0; i < 15; ++i)
	{
		List.Insert(i, i);
	}

	// First 10 elements use near search from front
	for (int i = 0; i < 10; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Value, i);
	}
}

TEST(SkipList, At_NearSearchBack)
{
	IntSkipList List;
	for (int i = 0; i < 15; ++i)
	{
		List.Insert(i, i);
	}

	// Last 10 elements use near search from back (Size - Index < 10)
	for (int i = 5; i < 15; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Value, i);
	}
}

TEST(SkipList, At_SkipSearchMiddle)
{
	// Elements in the middle use the full skip-list search
	IntSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i);
	}

	// Middle elements (not near front/back) use skip search
	for (int i = 10; i < 40; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(Node->Value, i) << "i=" << i;
	}
}

TEST(SkipList, OperatorBracket)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	auto* Node = List[0];
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);

	Node = List[1];
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
}

// ============================================================================
// 5. Range Queries
// ============================================================================

TEST(SkipList, ContainsAnyInRange_EmptyList)
{
	IntSkipList List;
	TK::TRange<int> Range = {{10, false}, {20, false}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_Found)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	TK::TRange<int> Range = {{15, false}, {25, false}};
	EXPECT_TRUE(List.ContainsAnyInRange(Range)); // 20 is in [15, 25]
}

TEST(SkipList, ContainsAnyInRange_NotFound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	TK::TRange<int> Range = {{30, false}, {40, false}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_ExclusiveLowerBound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// [10, 20] with exclusive lower => (10, 20] => only 20
	TK::TRange<int> Range = {{10, true}, {20, false}};
	EXPECT_TRUE(List.ContainsAnyInRange(Range)); // 20 is in (10, 20]
}

TEST(SkipList, ContainsAnyInRange_ExclusiveUpperBound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// [10, 20) => only 10
	TK::TRange<int> Range = {{10, false}, {20, true}};
	EXPECT_TRUE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_BothExclusive_Empty)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// (10, 20) => no elements strictly between 10 and 20
	TK::TRange<int> Range = {{10, true}, {20, true}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_InvalidRange)
{
	IntSkipList List;
	List.Insert(1, 10);

	// Lower > Upper
	TK::TRange<int> Range = {{20, false}, {10, false}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_EqualBounds_Inclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// [20, 20] contains 20
	TK::TRange<int> Range = {{20, false}, {20, false}};
	EXPECT_TRUE(List.ContainsAnyInRange(Range));
}

TEST(SkipList, ContainsAnyInRange_EqualBounds_Exclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// [20, 20) or (20, 20] or (20, 20) — all empty
	TK::TRange<int> Range1 = {{20, false}, {20, true}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range1));

	TK::TRange<int> Range2 = {{20, true}, {20, false}};
	EXPECT_FALSE(List.ContainsAnyInRange(Range2));
}

TEST(SkipList, GetFirstInLowerBound_Inclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Lower bound inclusive: >= 25
	TK::TRangeBound<int> Bound = {25, false};
	auto [Node, Index] = List.GetFirstInLowerBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 30);
	EXPECT_EQ(Index, 2); // 0-based index
}

TEST(SkipList, GetFirstInLowerBound_Exclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Lower bound exclusive: > 20
	TK::TRangeBound<int> Bound = {20, true};
	auto [Node, Index] = List.GetFirstInLowerBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 30);
	EXPECT_EQ(Index, 2);
}

TEST(SkipList, GetFirstInLowerBound_FirstElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// >= 5 — first element satisfies
	TK::TRangeBound<int> Bound = {5, false};
	auto [Node, Index] = List.GetFirstInLowerBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);
	EXPECT_EQ(Index, 0);
}

TEST(SkipList, GetFirstInLowerBound_OutOfRange)
{
	IntSkipList List;
	List.Insert(1, 10);

	// > 100 — no element satisfies
	TK::TRangeBound<int> Bound = {100, false};
	auto [Node, Index] = List.GetFirstInLowerBound(Bound);
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(Index, -1);
}

TEST(SkipList, GetLastInUpperBound_Inclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Upper bound inclusive: <= 25
	TK::TRangeBound<int> Bound = {25, false};
	auto [Node, Index] = List.GetLastInUpperBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(Index, 1);
}

TEST(SkipList, GetLastInUpperBound_Exclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Upper bound exclusive: < 30
	TK::TRangeBound<int> Bound = {30, true};
	auto [Node, Index] = List.GetLastInUpperBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(Index, 1);
}

TEST(SkipList, GetLastInUpperBound_LastElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// <= 100 — last element satisfies
	TK::TRangeBound<int> Bound = {100, false};
	auto [Node, Index] = List.GetLastInUpperBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(Index, 1);
}

TEST(SkipList, GetLastInUpperBound_OutOfRange)
{
	IntSkipList List;
	List.Insert(1, 10);

	// < 5 — no element satisfies
	TK::TRangeBound<int> Bound = {5, false};
	auto [Node, Index] = List.GetLastInUpperBound(Bound);
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(Index, -1);
}

// ============================================================================
// 6. Update
// ============================================================================

TEST(SkipList, Update_SameValue)
{
	IntSkipList List;
	List.Insert(1, 10);

	auto* Node = List.Update(1, 10, 10);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList, Update_NewValue_StaysInPlace)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Change 20 to 25 — still between 10 and 30, no repositioning needed
	auto* Node = List.Update(2, 20, 25);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 25);
	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {10, 25, 30});
}

TEST(SkipList, Update_NewValue_RequiresReposition)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Change 20 to 5 — must move before 10
	auto* Node = List.Update(2, 20, 5);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 5);
	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {5, 10, 30});
}

TEST(SkipList, Update_NotFound)
{
	IntSkipList List;
	List.Insert(1, 10);

	auto* Node = List.Update(2, 20, 30); // key=2 not in list
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(List.GetSize(), 1);

	Node = List.Update(1, 20, 30); // key=1 has value=10, not 20
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList, Update_WrongCurrentValue)
{
	IntSkipList List;
	List.Insert(1, 10);

	// Current value doesn't match actual value
	auto* Node = List.Update(1, 99, 50);
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(List.GetSize(), 1);
	VerifyListOrder(List, {10});
}

// ============================================================================
// 7. Flat SkipList (all nodes at level 1) — linked list behavior
// ============================================================================

TEST(SkipListFlat, InsertAndIterate)
{
	FlatSkipList List;
	for (int i = 0; i < 100; ++i)
	{
		auto [Node, Ok] = List.Insert(i, i * 10);
		EXPECT_TRUE(Ok);
	}

	EXPECT_EQ(List.GetSize(), 100);
	for (int i = 0; i < 100; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Value, i * 10);
	}
}

TEST(SkipListFlat, EraseRebuild)
{
	FlatSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i);
	}

	// Erase even values
	for (int i = 0; i < 50; i += 2)
	{
		EXPECT_TRUE(List.Erase(i, i));
	}

	EXPECT_EQ(List.GetSize(), 25);
	// Odd values remaining
	for (int i = 0; i < 25; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Value, i * 2 + 1);
	}
}

// ============================================================================
// 8. Multi-level SkipList (all nodes at MaxLevel)
// ============================================================================

TEST(SkipListTall, InsertAndVerify)
{
	// All nodes at height 4 with MaxLevel=4
	// This creates a skip list where every node appears at every level
	TallSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 10);
	}

	EXPECT_EQ(List.GetSize(), 30);
	VerifyListOrder(List, [&]() {
		std::vector<int> v(30);
		for (int i = 0; i < 30; ++i) v[i] = i * 10;
		return v;
	}());
	VerifyRandomAccess(List, [&]() {
		std::vector<int> v(30);
		for (int i = 0; i < 30; ++i) v[i] = i * 10;
		return v;
	}());
}

TEST(SkipListTall, EraseAndVerify)
{
	TallSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 10);
	}

	// Erase a few elements
	List.Erase(5, 50);
	List.Erase(10, 100);
	List.Erase(15, 150);

	EXPECT_EQ(List.GetSize(), 17);

	// Verify remaining elements are in order and accessible
	for (int i = 0; i < 17; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		// The value at index i should increase monotonically
	}
}

// ============================================================================
// 9. Edge Cases & Stress Tests
// ============================================================================

TEST(SkipList, LargeNumberOfElements)
{
	IntSkipList List;
	const int N = 1000;
	for (int i = 0; i < N; ++i)
	{
		List.Insert(i, i);
	}
	EXPECT_EQ(List.GetSize(), N);

	// Verify random access for all elements
	for (int i = 0; i < N; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "At(" << i << ") returned null";
		EXPECT_EQ(Node->Value, i) << "At(" << i << ") value mismatch";
	}
}

TEST(SkipList, InsertReverseOrder)
{
	IntSkipList List;
	for (int i = 999; i >= 0; --i)
	{
		List.Insert(i, i);
	}
	EXPECT_EQ(List.GetSize(), 1000);
	for (int i = 0; i < 1000; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Value, i);
	}
}

TEST(SkipList, RepeatedEraseAndInsert)
{
	IntSkipList List;
	for (int i = 0; i < 100; ++i)
	{
		List.Insert(i, i);
	}

	// Erase all
	for (int i = 0; i < 100; ++i)
	{
		List.Erase(i, i);
	}
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());

	// Re-insert
	for (int i = 0; i < 100; ++i)
	{
		List.Insert(i, i * 2);
	}
	EXPECT_EQ(List.GetSize(), 100);
	for (int i = 0; i < 100; ++i)
	{
		ASSERT_NE(List.At(i), nullptr);
		EXPECT_EQ(List.At(i)->Value, i * 2);
	}
}

TEST(SkipList, EraseFirstRepeatedly)
{
	IntSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i);
	}

	// Repeatedly erase the first element
	for (int i = 0; i < 50; ++i)
	{
		ASSERT_TRUE(List.Erase(i, i));
		EXPECT_EQ(List.GetSize(), 50 - i - 1);

		if (List.GetSize() > 0)
		{
			auto* First = List.At(0);
			ASSERT_NE(First, nullptr);
			EXPECT_EQ(First->GetPrev(), nullptr);
		}
	}
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseLastRepeatedly)
{
	IntSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i);
	}

	// Repeatedly erase the last element
	for (int i = 49; i >= 0; --i)
	{
		ASSERT_TRUE(List.Erase(i, i));
		EXPECT_EQ(List.GetSize(), i);

		if (List.GetSize() > 0)
		{
			auto* Last = List.At(List.GetSize() - 1);
			ASSERT_NE(Last, nullptr);
			EXPECT_EQ(Last->GetNext(), nullptr);
		}
	}
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, PrevPointerConsistency_AfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Erase middle and check prev pointers
	List.Erase(2, 20);

	// Node with value 30 should now have prev pointing to node with value 10
	auto* Node30 = List.At(1);
	ASSERT_NE(Node30, nullptr);
	EXPECT_EQ(Node30->Value, 30);
	ASSERT_NE(Node30->GetPrev(), nullptr);
	EXPECT_EQ(Node30->GetPrev()->Value, 10);
}

TEST(SkipList, SingleElement)
{
	IntSkipList List;
	List.Insert(1, 42);

	EXPECT_EQ(List.GetSize(), 1);
	auto* Node = List.At(0);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 42);
	EXPECT_EQ(Node->GetPrev(), nullptr);
	EXPECT_EQ(Node->GetNext(), nullptr);

	// Erase the only element
	EXPECT_TRUE(List.Erase(1, 42));
	EXPECT_TRUE(List.IsEmpty());
	EXPECT_EQ(List.At(0), nullptr);
}

TEST(SkipList, InsertAfterClear)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Clear();

	// Insert new elements after clearing
	List.Insert(3, 30);
	List.Insert(4, 40);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {30, 40});
}

// ============================================================================
// 10. Mock Random Function — controlled multi-level structure
// ============================================================================

TEST(SkipListMock, SpecificLevels)
{
	// Generate levels: 1, 3, 1, 2, 1
	// Level 1: false
	// Level 3: true, true, false
	// Level 1: false
	// Level 2: true, false
	// Level 1: false
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	// These 5 inserts will use the pre-determined levels
	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	EXPECT_EQ(List.GetSize(), 5);
	VerifyListOrder(List, {10, 20, 30, 40, 50});
	VerifyRandomAccess(List, {10, 20, 30, 40, 50});
}

TEST(SkipListMock, MultiLevel_EraseHighest)
{
	// Same level sequence as above: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	// Erase the tallest node (height 3)
	EXPECT_TRUE(List.Erase(2, 20));
	EXPECT_EQ(List.GetSize(), 4);
	VerifyListOrder(List, {10, 30, 40, 50});
	VerifyRandomAccess(List, {10, 30, 40, 50});
}

TEST(SkipListMock, MultiLevel_EraseLowest)
{
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	// Erase a level-1 node (height 1)
	EXPECT_TRUE(List.Erase(3, 30));
	EXPECT_EQ(List.GetSize(), 4);
	VerifyListOrder(List, {10, 20, 40, 50});
	VerifyRandomAccess(List, {10, 20, 40, 50});
}

// ============================================================================
// 11. Span Consistency (via At verification after mixed operations)
// ============================================================================

TEST(SkipList, SpanConsistency_AfterMixedOps)
{
	IntSkipList List;
	std::set<int> Ref; // reference sorted set

	std::mt19937 Rng(42); // fixed seed for reproducibility
	std::uniform_int_distribution<int> ValDist(0, 10000);
	std::uniform_int_distribution<int> OpDist(0, 1);

	for (int Round = 0; Round < 200; ++Round)
	{
		int Val = ValDist(Rng);
		if (OpDist(Rng) == 0)
		{
			// Insert
			auto [Node, Ok] = List.Insert(Val, Val);
			if (Ok)
				Ref.insert(Val);
		}
		else
		{
			// Erase
			if (List.Erase(Val, Val))
				Ref.erase(Val);
		}
	}

	EXPECT_EQ(List.GetSize(), static_cast<int>(Ref.size()));

	// Verify At() matches reference set
	std::vector<int> RefVec(Ref.begin(), Ref.end());
	for (size_t i = 0; i < RefVec.size(); ++i)
	{
		auto* Node = List.At(static_cast<int>(i));
		ASSERT_NE(Node, nullptr) << "At(" << i << ") null, ref size=" << RefVec.size();
		EXPECT_EQ(Node->Value, RefVec[i]) << "At(" << i << ") mismatch";
	}
}

TEST(SkipList, SpanConsistency_AfterEraseOnly)
{
	IntSkipList List;
	for (int i = 0; i < 100; ++i)
	{
		List.Insert(i, i);
	}

	// Erase specific indices to force span recalculation
	List.Erase(10, 10);
	List.Erase(50, 50);
	List.Erase(90, 90);

	// Verify remaining elements via At()
	for (int i = 0; i < 97; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "At(" << i << ") null";
		// Values should be strictly increasing
		if (i > 0)
		{
			EXPECT_GT(Node->Value, List.At(i - 1)->Value);
		}
	}
}

// ============================================================================
// 12. GetFirstInLowerBound & GetLastInUpperBound — Returned Index
// ============================================================================

TEST(SkipList, LowerBound_IndexCorrectness)
{
	IntSkipList List;
	for (int i = 0; i < 100; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, 20, ..., 990
	}

	// Check that returned index matches At()
	for (int BoundVal = 0; BoundVal <= 1000; BoundVal += 10)
	{
		TK::TRangeBound<int> Bound = {BoundVal, false};
		auto [Node, Index] = List.GetFirstInLowerBound(Bound);

		if (BoundVal > 990)
		{
			EXPECT_EQ(Node, nullptr);
			EXPECT_EQ(Index, -1);
		}
		else
		{
			ASSERT_NE(Node, nullptr) << "BoundVal=" << BoundVal;
			EXPECT_EQ(Node, List.At(Index)) << "BoundVal=" << BoundVal;
			EXPECT_GE(Node->Value, BoundVal) << "BoundVal=" << BoundVal;
		}
	}
}

TEST(SkipList, UpperBound_IndexCorrectness)
{
	IntSkipList List;
	for (int i = 0; i < 100; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, 20, ..., 990
	}

	for (int BoundVal = -1; BoundVal <= 1000; BoundVal += 10)
	{
		TK::TRangeBound<int> Bound = {BoundVal, false};
		auto [Node, Index] = List.GetLastInUpperBound(Bound);

		if (BoundVal < 0)
		{
			EXPECT_EQ(Node, nullptr);
			EXPECT_EQ(Index, -1);
		}
		else
		{
			ASSERT_NE(Node, nullptr) << "BoundVal=" << BoundVal;
			EXPECT_EQ(Node, List.At(Index)) << "BoundVal=" << BoundVal;
			EXPECT_LE(Node->Value, BoundVal) << "BoundVal=" << BoundVal;
		}
	}
}

TEST(SkipList, LowerBound_FirstElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Bound <= 10 means first element satisfies (inclusive)
	TK::TRangeBound<int> Bound = {0, false};
	auto [Node, Index] = List.GetFirstInLowerBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);
	EXPECT_EQ(Index, 0);
}

TEST(SkipList, UpperBound_LastElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Bound >= 30 means last element satisfies (inclusive)
	TK::TRangeBound<int> Bound = {50, false};
	auto [Node, Index] = List.GetLastInUpperBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 30);
	EXPECT_EQ(Index, 2); // last index
}

// ============================================================================
// 13. GetRank
// ============================================================================

TEST(SkipList, GetRank_FirstElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_EQ(List.GetRank(1, 10), 0);
}

TEST(SkipList, GetRank_MiddleElement)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10);
	}
	// Values: 0, 10, 20, ..., 90
	EXPECT_EQ(List.GetRank(5, 50), 5);
}

TEST(SkipList, GetRank_LastElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_EQ(List.GetRank(3, 30), 2);
}

TEST(SkipList, GetRank_AfterInsert)
{
	IntSkipList List;
	List.Insert(1, 10);  // rank 0
	List.Insert(3, 30);  // rank 1
	List.Insert(2, 20);  // inserts between → rank 1, pushes 30 to rank 2

	EXPECT_EQ(List.GetRank(1, 10), 0);
	EXPECT_EQ(List.GetRank(2, 20), 1);
	EXPECT_EQ(List.GetRank(3, 30), 2);
}

TEST(SkipList, GetRank_AfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	List.Erase(2, 20); // remove middle element

	EXPECT_EQ(List.GetRank(1, 10), 0);
	EXPECT_EQ(List.GetRank(3, 30), 1);
	EXPECT_EQ(List.GetRank(4, 40), 2);
}

TEST(SkipList, GetRank_NotExist_WrongKey)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	EXPECT_EQ(List.GetRank(3, 30), -1); // key 3 not in list
}

TEST(SkipList, GetRank_NotExist_WrongValue)
{
	IntSkipList List;
	List.Insert(1, 10);

	EXPECT_EQ(List.GetRank(1, 99), -1); // key 1 has value 10, not 99
}

TEST(SkipList, GetRank_EmptyList)
{
	IntSkipList List;
	EXPECT_EQ(List.GetRank(1, 10), -1);
}

TEST(SkipList, GetRank_WithValueBasedOrdering)
{
	// Ordering is by value first, key second
	IntSkipList List;
	List.Insert(3, 300); // value 300, key 3
	List.Insert(1, 100); // value 100, key 1
	List.Insert(2, 200); // value 200, key 2

	// Sorted order: (1,100), (2,200), (3,300)
	EXPECT_EQ(List.GetRank(1, 100), 0);
	EXPECT_EQ(List.GetRank(2, 200), 1);
	EXPECT_EQ(List.GetRank(3, 300), 2);
}

TEST(SkipList, GetRank_SameValue_DifferentKey)
{
	// Same value, different keys → tiebreaker by key
	IntSkipList List;
	List.Insert(3, 100);
	List.Insert(1, 100);
	List.Insert(2, 100);

	// Sorted: (1,100), (2,100), (3,100)
	EXPECT_EQ(List.GetRank(1, 100), 0);
	EXPECT_EQ(List.GetRank(2, 100), 1);
	EXPECT_EQ(List.GetRank(3, 100), 2);
}

TEST(SkipList, GetRank_ConsistencyWithAt)
{
	IntSkipList List;
	std::mt19937 Rng(12345);
	std::uniform_int_distribution<int> Dist(0, 1000);

	for (int i = 0; i < 100; ++i)
	{
		int Val = Dist(Rng);
		List.Insert(Val, Val); // duplicate-safe: if already exists, Insert returns false
	}

	// Verify GetRank returns the same index as At
	for (int i = 0; i < List.GetSize(); ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(List.GetRank(Node->Key, Node->Value), i) << "i=" << i;
	}
}

TEST(SkipList, GetRank_AfterClear)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Clear();

	EXPECT_EQ(List.GetRank(1, 10), -1);
}

TEST(SkipListFlat, GetRank_AllLevelOne)
{
	FlatSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i * 10);
	}

	// All nodes at level 1 (linked list); verify all ranks
	for (int i = 0; i < 50; ++i)
	{
		EXPECT_EQ(List.GetRank(i, i * 10), i);
	}
}

TEST(SkipListTall, GetRank_MultiLevel)
{
	TallSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 10);
	}

	// All nodes at MaxLevel=4; verify ranks with skip-list structure
	for (int i = 0; i < 30; ++i)
	{
		EXPECT_EQ(List.GetRank(i, i * 10), i);
	}
}

TEST(SkipListMock, GetRank_ControlledLevels)
{
	// Levels: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1, rank 0
	List.Insert(2, 20); // level 3, rank 1
	List.Insert(3, 30); // level 1, rank 2
	List.Insert(4, 40); // level 2, rank 3
	List.Insert(5, 50); // level 1, rank 4

	for (int i = 0; i < 5; ++i)
	{
		EXPECT_EQ(List.GetRank(i + 1, (i + 1) * 10), i);
	}
}

// ============================================================================
// 14. Find
// ============================================================================

TEST(SkipList, Find_FirstElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto* Node = List.Find(1, 10);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 10);
}

TEST(SkipList, Find_MiddleElement)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10);
	}

	auto* Node = List.Find(5, 50);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 5);
	EXPECT_EQ(Node->Value, 50);
}

TEST(SkipList, Find_LastElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto* Node = List.Find(3, 30);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
	EXPECT_EQ(Node->Value, 30);
}

TEST(SkipList, Find_NotExist_WrongKey)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	EXPECT_EQ(List.Find(3, 30), nullptr);
}

TEST(SkipList, Find_NotExist_WrongValue)
{
	IntSkipList List;
	List.Insert(1, 10);

	EXPECT_EQ(List.Find(1, 99), nullptr);
}

TEST(SkipList, Find_EmptyList)
{
	IntSkipList List;
	EXPECT_EQ(List.Find(1, 10), nullptr);
}

TEST(SkipList, Find_AfterClear)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Clear();

	EXPECT_EQ(List.Find(1, 10), nullptr);
}

TEST(SkipList, Find_AfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	List.Erase(2, 20);

	EXPECT_EQ(List.Find(2, 20), nullptr);
	EXPECT_NE(List.Find(1, 10), nullptr);
	EXPECT_NE(List.Find(3, 30), nullptr);
}

TEST(SkipList, Find_ConsistencyWithAt)
{
	IntSkipList List;
	std::mt19937 Rng(54321);
	std::uniform_int_distribution<int> Dist(0, 500);

	for (int i = 0; i < 80; ++i)
	{
		int Val = Dist(Rng);
		List.Insert(Val, Val);
	}

	// Find should return the same pointer as At(GetRank)
	for (int i = 0; i < List.GetSize(); ++i)
	{
		auto* NodeByAt = List.At(i);
		ASSERT_NE(NodeByAt, nullptr) << "i=" << i;
		auto* NodeByFind = List.Find(NodeByAt->Key, NodeByAt->Value);
		EXPECT_EQ(NodeByFind, NodeByAt) << "i=" << i;
	}
}

TEST(SkipList, Find_WithValueBasedOrdering)
{
	IntSkipList List;
	List.Insert(3, 300); // value 300, key 3
	List.Insert(1, 100); // value 100, key 1
	List.Insert(2, 200); // value 200, key 2

	auto* Node = List.Find(1, 100);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(Node->GetPrev(), nullptr); // first in sorted order

	Node = List.Find(3, 300);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 300);
	EXPECT_EQ(Node->GetNext(), nullptr); // last in sorted order
}

TEST(SkipList, Find_SameValue_DifferentKey)
{
	IntSkipList List;
	List.Insert(3, 100);
	List.Insert(1, 100);
	List.Insert(2, 100);

	// All have value 100; distinguish by key
	auto* Node1 = List.Find(1, 100);
	auto* Node2 = List.Find(2, 100);
	auto* Node3 = List.Find(3, 100);

	ASSERT_NE(Node1, nullptr);
	ASSERT_NE(Node2, nullptr);
	ASSERT_NE(Node3, nullptr);
	EXPECT_NE(Node1, Node2);
	EXPECT_NE(Node2, Node3);
}

TEST(SkipList, Find_NextPrevAfterFind)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// Find middle and traverse neighbors
	auto* Node = List.Find(3, 30);
	ASSERT_NE(Node, nullptr);

	auto* Prev = Node->GetPrev();
	auto* Next = Node->GetNext();
	ASSERT_NE(Prev, nullptr);
	ASSERT_NE(Next, nullptr);
	EXPECT_EQ(Prev->Value, 20);
	EXPECT_EQ(Next->Value, 40);
}

TEST(SkipList, Find_DuplicateInsert_ReturnsExistingNode)
{
	IntSkipList List;
	auto [InsertedNode, Ok1] = List.Insert(1, 100);
	EXPECT_TRUE(Ok1);

	auto* FoundNode = List.Find(1, 100);
	EXPECT_EQ(FoundNode, InsertedNode);
}

TEST(SkipListFlat, Find_AllLevelOne)
{
	FlatSkipList List;
	for (int i = 0; i < 50; ++i)
	{
		List.Insert(i, i * 10);
	}

	for (int i = 0; i < 50; ++i)
	{
		auto* Node = List.Find(i, i * 10);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(Node->Value, i * 10) << "i=" << i;
	}

	EXPECT_EQ(List.Find(0, 99), nullptr);
	EXPECT_EQ(List.Find(99, 990), nullptr);
}

TEST(SkipListTall, Find_MultiLevel)
{
	TallSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 10);
	}

	for (int i = 0; i < 30; ++i)
	{
		auto* Node = List.Find(i, i * 10);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(Node->Value, i * 10);
	}
}

TEST(SkipListMock, Find_ControlledLevels)
{
	// Levels: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	// Verify each can be found
	for (int i = 1; i <= 5; ++i)
	{
		auto* Node = List.Find(i, i * 10);
		ASSERT_NE(Node, nullptr) << "Key=" << i;
		EXPECT_EQ(Node->Key, i);
		EXPECT_EQ(Node->Value, i * 10);
	}

	// Non-existing
	EXPECT_EQ(List.Find(1, 99), nullptr);
	EXPECT_EQ(List.Find(99, 10), nullptr);
}

// ============================================================================
// 15. Erase Range
// ============================================================================

TEST(SkipList, EraseRange_EmptyList)
{
	IntSkipList List;
	TK::TRange<int> Range = {{10, false}, {20, false}};
	EXPECT_EQ(List.Erase(Range), 0);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseRange_AllElementsAboveRange)
{
	IntSkipList List;
	List.Insert(1, 30);
	List.Insert(2, 40);
	List.Insert(3, 50);

	// Range entirely below all elements
	TK::TRange<int> Range = {{10, false}, {20, false}};
	EXPECT_EQ(List.Erase(Range), 0);
	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {30, 40, 50});
}

TEST(SkipList, EraseRange_AllElementsBelowRange)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// Range entirely above all elements
	TK::TRange<int> Range = {{30, false}, {40, false}};
	EXPECT_EQ(List.Erase(Range), 0);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
}

TEST(SkipList, EraseRange_NoElementsInGap)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 50);
	List.Insert(4, 60);

	// Range falls in gap between 20 and 50
	TK::TRange<int> Range = {{25, false}, {45, false}};
	EXPECT_EQ(List.Erase(Range), 0);
	EXPECT_EQ(List.GetSize(), 4);
	VerifyListOrder(List, {10, 20, 50, 60});
}

TEST(SkipList, EraseRange_SingleElement_Inclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// Erase exactly one element: 30
	TK::TRange<int> Range = {{30, false}, {30, false}};
	EXPECT_EQ(List.Erase(Range), 1);
	EXPECT_EQ(List.GetSize(), 4);
	VerifyListOrder(List, {10, 20, 40, 50});
	VerifyRandomAccess(List, {10, 20, 40, 50});
}

TEST(SkipList, EraseRange_SingleElement_ExclusiveLower)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// (20, 30] — only 30 qualifies
	TK::TRange<int> Range = {{20, true}, {30, false}};
	EXPECT_EQ(List.Erase(Range), 1);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
}

TEST(SkipList, EraseRange_SingleElement_ExclusiveUpper)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// [20, 30) — only 20 qualifies
	TK::TRange<int> Range = {{20, false}, {30, true}};
	EXPECT_EQ(List.Erase(Range), 1);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 30});
}

TEST(SkipList, EraseRange_BothExclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// (10, 30) — only 20 qualifies
	TK::TRange<int> Range = {{10, true}, {30, true}};
	EXPECT_EQ(List.Erase(Range), 1);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 30});
}

TEST(SkipList, EraseRange_MultipleElements_Inclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// [20, 40] — erases 20, 30, 40
	TK::TRange<int> Range = {{20, false}, {40, false}};
	EXPECT_EQ(List.Erase(Range), 3);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 50});
	VerifyRandomAccess(List, {10, 50});
}

TEST(SkipList, EraseRange_FromBeginning)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Erase everything <= 25 — removes 10, 20
	TK::TRange<int> Range = {{0, false}, {25, false}};
	EXPECT_EQ(List.Erase(Range), 2);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {30, 40});
	VerifyRandomAccess(List, {30, 40});
}

TEST(SkipList, EraseRange_ToEnd)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Erase everything >= 25 — removes 30, 40
	TK::TRange<int> Range = {{25, false}, {50, false}};
	EXPECT_EQ(List.Erase(Range), 2);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
	VerifyRandomAccess(List, {10, 20});
}

TEST(SkipList, EraseRange_AllElements)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Range covers all elements
	TK::TRange<int> Range = {{0, false}, {100, false}};
	EXPECT_EQ(List.Erase(Range), 3);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
	EXPECT_EQ(List.At(0), nullptr);
}

TEST(SkipList, EraseRange_InvalidRange_LowerGreaterThanUpper)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	TK::TRange<int> Range = {{30, false}, {10, false}};
	EXPECT_EQ(List.Erase(Range), 0);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});
}

TEST(SkipList, EraseRange_InvalidRange_EqualExclusive)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// [20, 20) — empty range
	TK::TRange<int> Range1 = {{20, false}, {20, true}};
	EXPECT_EQ(List.Erase(Range1), 0);

	// (20, 20] — empty range
	TK::TRange<int> Range2 = {{20, true}, {20, false}};
	EXPECT_EQ(List.Erase(Range2), 0);

	// (20, 20) — empty range
	TK::TRange<int> Range3 = {{20, true}, {20, true}};
	EXPECT_EQ(List.Erase(Range3), 0);

	EXPECT_EQ(List.GetSize(), 2);
}

TEST(SkipList, EraseRange_PrevPointersAfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// Erase middle: 20, 30, 40
	TK::TRange<int> Range = {{20, false}, {40, false}};
	List.Erase(Range);

	// 50's prev should now be 10
	auto* Node50 = List.At(1);
	ASSERT_NE(Node50, nullptr);
	EXPECT_EQ(Node50->Value, 50);
	ASSERT_NE(Node50->GetPrev(), nullptr);
	EXPECT_EQ(Node50->GetPrev()->Value, 10);

	// 10's next should be 50
	auto* Node10 = List.At(0);
	ASSERT_NE(Node10, nullptr);
	EXPECT_EQ(Node10->Value, 10);
	ASSERT_NE(Node10->GetNext(), nullptr);
	EXPECT_EQ(Node10->GetNext()->Value, 50);
}

TEST(SkipList, EraseRange_PrevPointer_EraseFromBeginning)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Erase first two elements
	TK::TRange<int> Range = {{0, false}, {20, false}};
	List.Erase(Range);

	// Remaining element (30) should have prev == nullptr
	auto* Node30 = List.At(0);
	ASSERT_NE(Node30, nullptr);
	EXPECT_EQ(Node30->Value, 30);
	EXPECT_EQ(Node30->GetPrev(), nullptr);
}

TEST(SkipList, EraseRange_PrevPointer_EraseToEnd)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Erase last two elements
	TK::TRange<int> Range = {{20, false}, {40, false}};
	List.Erase(Range);

	// Remaining element (10) should be the only one
	EXPECT_EQ(List.GetSize(), 1);
	auto* Node10 = List.At(0);
	ASSERT_NE(Node10, nullptr);
	EXPECT_EQ(Node10->Value, 10);
	EXPECT_EQ(Node10->GetNext(), nullptr);
}

TEST(SkipList, EraseRange_InsertAfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// Erase middle
	TK::TRange<int> Range = {{20, false}, {40, false}};
	List.Erase(Range);

	// Insert new element in the gap
	auto [Node, Ok] = List.Insert(6, 25);
	EXPECT_TRUE(Ok);
	ASSERT_NE(Node, nullptr);

	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {10, 25, 50});
	VerifyRandomAccess(List, {10, 25, 50});
}

TEST(SkipList, EraseRange_SizeUpdatedCorrectly)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10);
	}
	EXPECT_EQ(List.GetSize(), 10);

	// Erase [30, 60] → values 30, 40, 50, 60 → 4 elements
	TK::TRange<int> Range = {{30, false}, {60, false}};
	List.Erase(Range);
	EXPECT_EQ(List.GetSize(), 6);

	// Erase remaining
	TK::TRange<int> Range2 = {{0, false}, {100, false}};
	List.Erase(Range2);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseRange_ReinsertSameValues)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	TK::TRange<int> Range = {{15, false}, {25, false}};
	EXPECT_EQ(List.Erase(Range), 1); // erases 20
	EXPECT_EQ(List.GetSize(), 2);

	// Re-insert with same key-value
	auto [Node, Ok] = List.Insert(2, 20);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {10, 20, 30});
}

TEST(SkipList, EraseRange_AtCorrectAfterPartialErase)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, 20, ..., 190
	}

	// Erase values 40-120 → indices 4-12 → 9 elements
	TK::TRange<int> Range = {{40, false}, {120, false}};
	List.Erase(Range);

	EXPECT_EQ(List.GetSize(), 11);

	// Verify remaining elements via At()
	// Should be: 0,10,20,30,130,140,150,160,170,180,190
	std::vector<int> Expected = {0, 10, 20, 30, 130, 140, 150, 160, 170, 180, 190};
	VerifyListOrder(List, Expected);
	VerifyRandomAccess(List, Expected);
}

TEST(SkipList, EraseRange_TailUpdatedWhenErasingTail)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Erase the last element
	TK::TRange<int> Range = {{30, false}, {30, false}};
	List.Erase(Range);

	EXPECT_EQ(List.GetSize(), 2);
	auto* Last = List.At(1);
	ASSERT_NE(Last, nullptr);
	EXPECT_EQ(Last->Value, 20);
	EXPECT_EQ(Last->GetNext(), nullptr); // should be the new tail
}

TEST(SkipList, EraseRange_MultipleRanges_Sequential)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10); // 0, 10, 20, ..., 90
	}

	// Erase [10, 30]
	TK::TRange<int> Range1 = {{10, false}, {30, false}};
	EXPECT_EQ(List.Erase(Range1), 3); // removes 10, 20, 30
	VerifyListOrder(List, {0, 40, 50, 60, 70, 80, 90});

	// Erase [60, 80]
	TK::TRange<int> Range2 = {{60, false}, {80, false}};
	EXPECT_EQ(List.Erase(Range2), 3); // removes 60, 70, 80
	VerifyListOrder(List, {0, 40, 50, 90});

	// Erase the rest
	TK::TRange<int> Range3 = {{0, false}, {100, false}};
	EXPECT_EQ(List.Erase(Range3), 4);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseRange_ContainsAnyInRange_AfterErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Erase [20, 30]
	TK::TRange<int> EraseRange = {{20, false}, {30, false}};
	List.Erase(EraseRange);

	// Now [25, 35] should return false (no elements in range)
	TK::TRange<int> QueryRange = {{25, false}, {35, false}};
	EXPECT_FALSE(List.ContainsAnyInRange(QueryRange));

	// [5, 15] should still contain 10
	TK::TRange<int> QueryRange2 = {{5, false}, {15, false}};
	EXPECT_TRUE(List.ContainsAnyInRange(QueryRange2));
}

TEST(SkipListFlat, EraseRange)
{
	FlatSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 5); // values: 0, 5, 10, ..., 95
	}

	// Erase [25, 60] → values 25,30,35,40,45,50,55,60 → 8 elements
	TK::TRange<int> Range = {{25, false}, {60, false}};
	EXPECT_EQ(List.Erase(Range), 8);
	EXPECT_EQ(List.GetSize(), 12);

	// Verify remaining elements
	std::vector<int> Expected;
	for (int i = 0; i < 20; ++i)
	{
		int v = i * 5;
		if (v < 25 || v > 60)
			Expected.push_back(v);
	}
	EXPECT_EQ(Expected.size(), 12u);
	VerifyListOrder(List, Expected);
	VerifyRandomAccess(List, Expected);
}

TEST(SkipListTall, EraseRange)
{
	TallSkipList List;
	for (int i = 0; i < 15; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, ..., 140
	}

	// Erase [30, 90] → 30,40,50,60,70,80,90 → 7 elements
	TK::TRange<int> Range = {{30, false}, {90, false}};
	EXPECT_EQ(List.Erase(Range), 7);
	EXPECT_EQ(List.GetSize(), 8);

	std::vector<int> Expected = {0, 10, 20, 100, 110, 120, 130, 140};
	VerifyListOrder(List, Expected);
	VerifyRandomAccess(List, Expected);
}

TEST(SkipListMock, EraseRange_ControlledLevels)
{
	// Levels: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	// Erase [25, 45] → removes 30, 40 (both have diff levels)
	TK::TRange<int> Range = {{25, false}, {45, false}};
	EXPECT_EQ(List.Erase(Range), 2);
	EXPECT_EQ(List.GetSize(), 3);

	VerifyListOrder(List, {10, 20, 50});
	VerifyRandomAccess(List, {10, 20, 50});
}

// ============================================================================
// 16. GetCountInRange
// ============================================================================

TEST(SkipList, GetCountInRange_EmptyList)
{
	IntSkipList List;
	TK::TRange<int> Range = {{10, false}, {20, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 0);
}

TEST(SkipList, GetCountInRange_NoElementsInRange)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// Range above all elements
	TK::TRange<int> Range = {{30, false}, {40, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 0);
}

TEST(SkipList, GetCountInRange_AllElementsInRange)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Range covers everything
	TK::TRange<int> Range = {{0, false}, {100, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 3);
}

TEST(SkipList, GetCountInRange_PartialRange)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, ..., 90
	}

	// [25, 65] → values 30, 40, 50, 60 → 4 elements
	TK::TRange<int> Range = {{25, false}, {65, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 4);
}

TEST(SkipList, GetCountInRange_ExclusiveBounds)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// (10, 40) → only 20, 30 → 2 elements
	TK::TRange<int> Range = {{10, true}, {40, true}};
	EXPECT_EQ(List.GetCountInRange(Range), 2);
}

TEST(SkipList, GetCountInRange_SingleElement_ExactMatch)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// [20, 20] → exactly 1
	TK::TRange<int> Range = {{20, false}, {20, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 1);
}

TEST(SkipList, GetCountInRange_InclusiveVsExclusive_Difference)
{
	IntSkipList List;
	for (int i = 0; i <= 10; ++i)
	{
		List.Insert(i, i * 10); // values: 0, 10, ..., 100
	}

	// Inclusive [20, 80] → 20,30,40,50,60,70,80 → 7
	TK::TRange<int> RangeIncl = {{20, false}, {80, false}};
	EXPECT_EQ(List.GetCountInRange(RangeIncl), 7);

	// Exclusive (20, 80) → 30,40,50,60,70 → 5
	TK::TRange<int> RangeExcl = {{20, true}, {80, true}};
	EXPECT_EQ(List.GetCountInRange(RangeExcl), 5);
}

TEST(SkipList, GetCountInRange_BeforeAndAfterErase)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10); // 0, 10, ..., 90
	}

	// Before erase: [40, 60] → 3 elements
	TK::TRange<int> Range = {{40, false}, {60, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 3);

	// Erase [30, 50] → removes 30, 40, 50
	TK::TRange<int> EraseRange = {{30, false}, {50, false}};
	List.Erase(EraseRange);

	// After erase: [40, 60] → only 60 → 1 element
	EXPECT_EQ(List.GetCountInRange(Range), 1);
}

TEST(SkipList, GetCountInRange_CountMatchesManual)
{
	IntSkipList List;
	std::mt19937 Rng(99);
	std::uniform_int_distribution<int> Dist(0, 100);

	for (int i = 0; i < 50; ++i)
	{
		List.Insert(Dist(Rng), Dist(Rng));
	}

	// Query a random range and verify against manual count via At()
	TK::TRange<int> Range = {{30, false}, {60, false}};
	int Count = List.GetCountInRange(Range);

	int ManualCount = 0;
	for (int i = 0; i < List.GetSize(); ++i)
	{
		auto* Node = List.At(i);
		if (Node->Value >= 30 && Node->Value <= 60)
			++ManualCount;
	}
	EXPECT_EQ(Count, ManualCount);
}

TEST(SkipListFlat, GetCountInRange)
{
	FlatSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 2); // 0, 2, 4, ..., 58
	}

	// [20, 40] → values 20,22,24,...,40 → 11 elements
	TK::TRange<int> Range = {{20, false}, {40, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 11);
}

TEST(SkipListTall, GetCountInRange)
{
	TallSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 5); // 0, 5, 10, ..., 95
	}

	// [25, 75] → 25,30,...,75 → 11 elements
	TK::TRange<int> Range = {{25, false}, {75, false}};
	EXPECT_EQ(List.GetCountInRange(Range), 11);
}

// ============================================================================
// 17. Range-based for loop
// ============================================================================

TEST(SkipList, RangeFor_BasicForwardIteration)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(SkipList, RangeFor_EmptyList)
{
	IntSkipList List;
	int Count = 0;
	for (const auto& Node : List)
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);
}

TEST(SkipList, RangeFor_SingleElement)
{
	IntSkipList List;
	List.Insert(1, 42);

	int Count = 0;
	for (const auto& Node : List)
	{
		EXPECT_EQ(Node.Value, 42);
		EXPECT_EQ(Node.Key, 1);
		++Count;
	}
	EXPECT_EQ(Count, 1);
}

TEST(SkipList, RangeFor_AccessKeyAndValue)
{
	IntSkipList List;
	List.Insert(100, 10);
	List.Insert(200, 20);
	List.Insert(300, 30);

	std::vector<std::pair<int, int>> Pairs;
	for (const auto& Node : List)
	{
		Pairs.emplace_back(Node.Key, Node.Value);
	}
	ASSERT_EQ(Pairs.size(), 3u);
	// Sorted by value, so value order: 10, 20, 30
	EXPECT_EQ(Pairs[0], (std::pair<int, int>{100, 10}));
	EXPECT_EQ(Pairs[1], (std::pair<int, int>{200, 20}));
	EXPECT_EQ(Pairs[2], (std::pair<int, int>{300, 30}));
}

TEST(SkipList, RangeFor_ReverseValueOrder)
{
	DescSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	// Descending: 30, 20, 10
	EXPECT_EQ(Values, (std::vector<int>{30, 20, 10}));
}

TEST(SkipList, RangeFor_AfterInsertAndErase)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	// Erase middle
	List.Erase(3, 30);
	// Erase first
	List.Erase(1, 10);

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{20, 40, 50}));

	// Insert back
	List.Insert(6, 35);

	Values.clear();
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{20, 35, 40, 50}));
}

TEST(SkipList, RangeFor_AfterClear)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Clear();

	int Count = 0;
	for (const auto& Node : List)
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);

	// Re-insert after clear
	List.Insert(3, 30);
	List.Insert(4, 40);

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{30, 40}));
}

TEST(SkipList, RangeFor_LargeList)
{
	IntSkipList List;
	const int N = 500;
	for (int i = 0; i < N; ++i)
	{
		List.Insert(i, i * 10);
	}

	int Count = 0;
	int PrevValue = -1;
	for (const auto& Node : List)
	{
		EXPECT_GT(Node.Value, PrevValue) << "values must be monotonically increasing";
		PrevValue = Node.Value;
		++Count;
	}
	EXPECT_EQ(Count, N);
}

TEST(SkipList, RangeFor_BreakMidIteration)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 10);
	}

	int Count = 0;
	for (const auto& Node : List)
	{
		if (Node.Value >= 100)
			break;
		++Count;
	}
	// Values: 0,10,20,...,90 → 10 elements before 100
	EXPECT_EQ(Count, 10);
}

TEST(SkipList, RangeFor_ContinueSkip)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10); // 0, 10, 20, ..., 90
	}

	int SumEven = 0;
	for (const auto& Node : List)
	{
		if (Node.Value % 20 != 0) // skip non-multiples of 20
			continue;
		SumEven += Node.Value;
	}
	// Multiples of 20: 0, 20, 40, 60, 80 → sum = 200
	EXPECT_EQ(SumEven, 200);
}

TEST(SkipList, RangeFor_MatchesManualIteration)
{
	IntSkipList List;
	std::mt19937 Rng(777);
	std::uniform_int_distribution<int> Dist(0, 500);

	for (int i = 0; i < 60; ++i)
	{
		int Val = Dist(Rng);
		List.Insert(Val, Val);
	}

	// Collect via range-for
	std::vector<int> ViaRangeFor;
	for (const auto& Node : List)
	{
		ViaRangeFor.push_back(Node.Value);
	}

	// Collect via At()
	std::vector<int> ViaAt;
	for (int i = 0; i < List.GetSize(); ++i)
	{
		ViaAt.push_back(List.At(i)->Value);
	}

	EXPECT_EQ(ViaRangeFor, ViaAt);
}

TEST(SkipList, RangeFor_MatchesExplicitIterators)
{
	IntSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 3);
	}

	// Range-for
	std::vector<int> ViaRangeFor;
	for (const auto& Node : List)
	{
		ViaRangeFor.push_back(Node.Value);
	}

	// Explicit begin/end
	std::vector<int> ViaIterators;
	for (auto It = List.begin(); It != List.end(); ++It)
	{
		ViaIterators.push_back(It->Value);
	}

	EXPECT_EQ(ViaRangeFor, ViaIterators);
}

TEST(SkipList, RangeFor_UsingCbeginCend)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	std::vector<int> Values;
	for (auto It = List.cbegin(); It != List.cend(); ++It)
	{
		Values.push_back(It->Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(SkipList, RangeFor_UsingRbeginRend)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Explicit reverse iteration
	std::vector<int> Values;
	for (auto It = List.rbegin(); It != List.rend(); ++It)
	{
		Values.push_back(It->Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{30, 20, 10}));
}

TEST(SkipList, RangeFor_UsingCrbeginCrend)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	std::vector<int> Values;
	for (auto It = List.crbegin(); It != List.crend(); ++It)
	{
		Values.push_back(It->Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{30, 20, 10}));
}

TEST(SkipList, RangeFor_WithCopySemantics)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// auto (by value copy) — should still work
	std::vector<int> Values;
	for (auto Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(SkipList, RangeFor_PostIncrementIterator)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// Test post-increment explicitly
	auto It = List.begin();
	auto OldIt = It++;
	EXPECT_EQ(OldIt->Value, 10);
	EXPECT_EQ(It->Value, 20);

	// Post-decrement
	auto It2 = It; // points to 20
	auto OldIt2 = It2--;
	EXPECT_EQ(OldIt2->Value, 20);
	EXPECT_EQ(It2->Value, 10);
}

TEST(SkipList, RangeFor_MultipleTraversals)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	// First traversal
	int Count1 = 0;
	for (const auto& Node : List)
	{
		(void)Node;
		++Count1;
	}
	EXPECT_EQ(Count1, 3);

	// Second traversal — iterators should still be valid
	int Count2 = 0;
	for (const auto& Node : List)
	{
		(void)Node;
		++Count2;
	}
	EXPECT_EQ(Count2, 3);
}

TEST(SkipList, RangeFor_DereferenceOperator)
{
	IntSkipList List;
	List.Insert(1, 100);

	auto It = List.begin();
	ASSERT_NE(It, List.end());

	// operator* returns const NodeType&
	const auto& NodeRef = *It;
	EXPECT_EQ(NodeRef.Key, 1);
	EXPECT_EQ(NodeRef.Value, 100);

	// operator-> returns const NodeType*
	EXPECT_EQ(It->Key, 1);
	EXPECT_EQ(It->Value, 100);
}

TEST(SkipList, RangeFor_BeginEqualsEnd_EmptyList)
{
	IntSkipList List;
	EXPECT_EQ(List.begin(), List.end());
	EXPECT_EQ(List.cbegin(), List.cend());
	EXPECT_EQ(List.rbegin(), List.rend());
	EXPECT_EQ(List.crbegin(), List.crend());
}

TEST(SkipList, RangeFor_BeginNotEqualsEnd_NonEmpty)
{
	IntSkipList List;
	List.Insert(1, 10);

	EXPECT_NE(List.begin(), List.end());
	EXPECT_NE(List.cbegin(), List.cend());
	EXPECT_NE(List.rbegin(), List.rend());
	EXPECT_NE(List.crbegin(), List.crend());
}

TEST(SkipList, RangeFor_IteratorCopyAndAssign)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// Copy construction
	auto It1 = List.begin();
	auto It2(It1);
	EXPECT_EQ(It1, It2);
	EXPECT_EQ(It2->Value, 10);

	// Copy assignment
	IntSkipList::const_iterator It3;
	It3 = It1;
	EXPECT_EQ(It3->Value, 10);

	// Default constructed iterator
	IntSkipList::const_iterator DefaultIt;
	IntSkipList EmptyList;
	EXPECT_EQ(DefaultIt, EmptyList.end());
}

// ============================================================================
// 18. STL Algorithm Compatibility
// ============================================================================

TEST(SkipList, STL_ForEach)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10);
	}

	int Sum = 0;
	std::for_each(List.begin(), List.end(), [&Sum](const auto& Node) {
		Sum += Node.Value;
	});
	// Values: 0+10+20+...+90 = 450
	EXPECT_EQ(Sum, 450);
}

TEST(SkipList, STL_FindIf)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	auto It = std::find_if(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value > 25;
	});
	ASSERT_NE(It, List.end());
	EXPECT_EQ(It->Value, 30);
	EXPECT_EQ(It->Key, 3);
}

TEST(SkipList, STL_FindIf_NotFound)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	auto It = std::find_if(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value > 100;
	});
	EXPECT_EQ(It, List.end());
}

TEST(SkipList, STL_Accumulate)
{
	IntSkipList List;
	for (int i = 1; i <= 5; ++i)
	{
		List.Insert(i, i * 10);
	}

	int Sum = std::accumulate(List.begin(), List.end(), 0,
		[](int Acc, const auto& Node) { return Acc + Node.Value; });
	// 10+20+30+40+50 = 150
	EXPECT_EQ(Sum, 150);
}

TEST(SkipList, STL_Distance)
{
	IntSkipList List;
	for (int i = 0; i < 25; ++i)
	{
		List.Insert(i, i);
	}

	auto Dist = std::distance(List.begin(), List.end());
	EXPECT_EQ(Dist, 25);

	// Distance between specific iterators
	auto First = List.begin();
	auto Second = std::next(First, 10);
	EXPECT_EQ(std::distance(First, Second), 10);
}

TEST(SkipList, STL_CountIf)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
	{
		List.Insert(i, i * 5); // 0, 5, 10, ..., 95
	}

	int Count = std::count_if(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value % 10 == 0; // multiples of 10
	});
	// Multiples of 10: 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 → 10
	EXPECT_EQ(Count, 10);
}

TEST(SkipList, STL_AnyOf)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_TRUE(std::any_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value == 20;
	}));

	EXPECT_FALSE(std::any_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value == 99;
	}));
}

TEST(SkipList, STL_AllOf)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, 50);
	}

	EXPECT_TRUE(std::all_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value == 50;
	}));

	EXPECT_FALSE(std::all_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Key < 5;
	}));
}

TEST(SkipList, STL_NoneOf)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
	{
		List.Insert(i, i * 10);
	}

	EXPECT_TRUE(std::none_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value < 0;
	}));

	EXPECT_FALSE(std::none_of(List.begin(), List.end(), [](const auto& Node) {
		return Node.Value > 50;
	}));
}

TEST(SkipList, STL_NextPrev)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);
	List.Insert(5, 50);

	auto Mid = std::next(List.begin(), 2);
	ASSERT_NE(Mid, List.end());
	EXPECT_EQ(Mid->Value, 30);

	auto Prev = std::prev(Mid);
	EXPECT_EQ(Prev->Value, 20);

	auto Next = std::next(Mid);
	EXPECT_EQ(Next->Value, 40);
}

TEST(SkipList, STL_Transform)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	std::vector<int> Values(3);
	std::transform(List.begin(), List.end(), Values.begin(), [](const auto& Node) {
		return Node.Value;
	});
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(SkipList, STL_MinMax)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto MinIt = std::min_element(List.begin(), List.end(),
		[](const auto& A, const auto& B) { return A.Value < B.Value; });
	ASSERT_NE(MinIt, List.end());
	EXPECT_EQ(MinIt->Value, 10);

	auto MaxIt = std::max_element(List.begin(), List.end(),
		[](const auto& A, const auto& B) { return A.Value < B.Value; });
	ASSERT_NE(MaxIt, List.end());
	EXPECT_EQ(MaxIt->Value, 30);
}

// ============================================================================
// 19. Range-based for with Flat, Tall, and Mock SkipLists
// ============================================================================

TEST(SkipListFlat, RangeFor_AllLevelOne)
{
	FlatSkipList List;
	for (int i = 0; i < 30; ++i)
	{
		List.Insert(i, i * 2); // 0, 2, 4, ..., 58
	}

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	ASSERT_EQ(Values.size(), 30u);
	for (int i = 0; i < 30; ++i)
	{
		EXPECT_EQ(Values[i], i * 2);
	}
}

TEST(SkipListTall, RangeFor_MultiLevel)
{
	TallSkipList List;
	for (int i = 0; i < 25; ++i)
	{
		List.Insert(i, i * 5); // 0, 5, 10, ..., 120
	}

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	ASSERT_EQ(Values.size(), 25u);
	for (int i = 0; i < 25; ++i)
	{
		EXPECT_EQ(Values[i], i * 5);
	}
}

TEST(SkipListMock, RangeFor_ControlledLevels)
{
	// Levels: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30, 40, 50}));
}

// ============================================================================
// 20. Range-based for — const correctness & edge cases
// ============================================================================

TEST(SkipList, RangeFor_ConstList)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	const auto& ConstList = List;
	std::vector<int> Values;
	for (const auto& Node : ConstList)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(SkipList, RangeFor_NestedLoops)
{
	IntSkipList ListA;
	ListA.Insert(1, 10);
	ListA.Insert(2, 20);

	IntSkipList ListB;
	ListB.Insert(3, 30);
	ListB.Insert(4, 40);

	// Nested range-for should work independently
	std::vector<std::pair<int, int>> Pairs;
	for (const auto& A : ListA)
	{
		for (const auto& B : ListB)
		{
			Pairs.emplace_back(A.Value, B.Value);
		}
	}
	ASSERT_EQ(Pairs.size(), 4u);
	EXPECT_EQ(Pairs[0], (std::pair<int, int>{10, 30}));
	EXPECT_EQ(Pairs[1], (std::pair<int, int>{10, 40}));
	EXPECT_EQ(Pairs[2], (std::pair<int, int>{20, 30}));
	EXPECT_EQ(Pairs[3], (std::pair<int, int>{20, 40}));
}

TEST(SkipList, RangeFor_IteratorEqualityAfterMutation)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	// After inserting a new element, begin() and end() still form a valid range
	List.Insert(3, 15); // inserts between 10 and 20

	std::vector<int> Values;
	for (const auto& Node : List)
	{
		Values.push_back(Node.Value);
	}
	EXPECT_EQ(Values, (std::vector<int>{10, 15, 20}));
}

TEST(SkipList, RangeFor_SumOfKeys)
{
	IntSkipList List;
	List.Insert(10, 1);
	List.Insert(20, 2);
	List.Insert(30, 3);

	int KeySum = 0;
	for (const auto& Node : List)
	{
		KeySum += Node.Key;
	}
	// Sorted by value: (10,1), (20,2), (30,3) → keys: 10, 20, 30
	EXPECT_EQ(KeySum, 60);
}

// ============================================================================
// 21. EraseByRank
// ============================================================================

TEST(SkipList, EraseByRank_SingleElement)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10); // 0, 10, ..., 90

	EXPECT_EQ(List.EraseByRank(3, 3), 1);
	EXPECT_EQ(List.GetSize(), 9);
	// Remaining: 0, 10, 20, 40, 50, 60, 70, 80, 90
	VerifyListOrder(List, {0, 10, 20, 40, 50, 60, 70, 80, 90});
}

TEST(SkipList, EraseByRank_MultipleElements)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.EraseByRank(2, 5), 4); // erases 20, 30, 40, 50
	EXPECT_EQ(List.GetSize(), 6);
	VerifyListOrder(List, {0, 10, 60, 70, 80, 90});
}

TEST(SkipList, EraseByRank_FromBeginning)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.EraseByRank(0, 2), 3); // erases 0, 10, 20
	EXPECT_EQ(List.GetSize(), 7);
	VerifyListOrder(List, {30, 40, 50, 60, 70, 80, 90});

	// Verify first element is now 30
	auto* First = List.At(0);
	ASSERT_NE(First, nullptr);
	EXPECT_EQ(First->Value, 30);
	EXPECT_EQ(First->GetPrev(), nullptr);
}

TEST(SkipList, EraseByRank_ToEnd)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.EraseByRank(7, 9), 3); // erases 70, 80, 90
	EXPECT_EQ(List.GetSize(), 7);
	VerifyListOrder(List, {0, 10, 20, 30, 40, 50, 60});

	// Verify last element is now 60
	auto* Last = List.At(6);
	ASSERT_NE(Last, nullptr);
	EXPECT_EQ(Last->Value, 60);
	EXPECT_EQ(Last->GetNext(), nullptr);
}

TEST(SkipList, EraseByRank_AllElements)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.EraseByRank(0, 9), 10);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
	EXPECT_EQ(List.At(0), nullptr);
}

TEST(SkipList, EraseByRank_EmptyList)
{
	IntSkipList List;
	EXPECT_EQ(List.EraseByRank(0, 5), 0);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseByRank_SingleElementList)
{
	IntSkipList List;
	List.Insert(1, 100);

	EXPECT_EQ(List.EraseByRank(0, 0), 1);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseByRank_ToRankClamped)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10); // 0, 10, 20, 30, 40

	// ToRank >= Size should be clamped to Size - 1
	EXPECT_EQ(List.EraseByRank(3, 100), 2); // erases 30, 40 (indices 3-4)
	EXPECT_EQ(List.GetSize(), 3);
	VerifyListOrder(List, {0, 10, 20});
}

TEST(SkipList, EraseByRank_FromRankClamped)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// FromRank < 0 should be clamped to 0
	EXPECT_EQ(List.EraseByRank(-5, 2), 3); // erases 0, 10, 20
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {30, 40});
}

TEST(SkipList, EraseByRank_InvalidRange_FromRankGreaterThanToRank)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.EraseByRank(5, 3), 0);
	EXPECT_EQ(List.GetSize(), 10);
	VerifyListOrder(List, {0, 10, 20, 30, 40, 50, 60, 70, 80, 90});
}

TEST(SkipList, EraseByRank_OutOfBounds_FromRankTooLarge)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// FromRank >= Size should return 0
	EXPECT_EQ(List.EraseByRank(5, 10), 0);
	EXPECT_EQ(List.EraseByRank(100, 200), 0);
	EXPECT_EQ(List.GetSize(), 5);
}

TEST(SkipList, EraseByRank_OutOfBounds_ToRankNegative)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// ToRank < 0 should return 0
	EXPECT_EQ(List.EraseByRank(0, -1), 0);
	EXPECT_EQ(List.GetSize(), 5);
}

TEST(SkipList, EraseByRank_FirstElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_EQ(List.EraseByRank(0, 0), 1);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {20, 30});

	auto* First = List.At(0);
	ASSERT_NE(First, nullptr);
	EXPECT_EQ(First->GetPrev(), nullptr);
}

TEST(SkipList, EraseByRank_LastElement)
{
	IntSkipList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_EQ(List.EraseByRank(2, 2), 1); // index 2 = last element (30)
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 20});

	auto* Last = List.At(1);
	ASSERT_NE(Last, nullptr);
	EXPECT_EQ(Last->GetNext(), nullptr);
}

TEST(SkipList, EraseByRank_SequentialRanges)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	// Erase [2, 4] → removes 20, 30, 40
	EXPECT_EQ(List.EraseByRank(2, 4), 3);
	VerifyListOrder(List, {0, 10, 50, 60, 70, 80, 90});

	// Erase [0, 1] → removes 0, 10 (now at the front)
	EXPECT_EQ(List.EraseByRank(0, 1), 2);
	VerifyListOrder(List, {50, 60, 70, 80, 90});

	// Erase [3, 4] → removes 80, 90 (now at the end)
	EXPECT_EQ(List.EraseByRank(3, 4), 2);
	VerifyListOrder(List, {50, 60, 70});
}

TEST(SkipList, EraseByRank_PrevPointerAfterErase)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	// Erase middle: ranks 3-6 → removes 30, 40, 50, 60
	List.EraseByRank(3, 6);

	// Node with value 70 (now at index 3) should have prev = node with value 20
	auto* Node70 = List.At(3);
	ASSERT_NE(Node70, nullptr);
	EXPECT_EQ(Node70->Value, 70);
	ASSERT_NE(Node70->GetPrev(), nullptr);
	EXPECT_EQ(Node70->GetPrev()->Value, 20);

	// Node with value 20 should have next = node 70
	auto* Node20 = List.At(2);
	ASSERT_NE(Node20, nullptr);
	EXPECT_EQ(Node20->Value, 20);
	ASSERT_NE(Node20->GetNext(), nullptr);
	EXPECT_EQ(Node20->GetNext()->Value, 70);
}

TEST(SkipList, EraseByRank_PrevPointer_EraseFromBeginning)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// Erase first 3 elements
	List.EraseByRank(0, 2);

	// Remaining first element should have prev == nullptr
	auto* First = List.At(0);
	ASSERT_NE(First, nullptr);
	EXPECT_EQ(First->Value, 30);
	EXPECT_EQ(First->GetPrev(), nullptr);
}

TEST(SkipList, EraseByRank_PrevPointer_EraseToEnd)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// Erase last 2 elements
	List.EraseByRank(3, 4);

	// Remaining last element should have next == nullptr
	auto* Last = List.At(2);
	ASSERT_NE(Last, nullptr);
	EXPECT_EQ(Last->Value, 20);
	EXPECT_EQ(Last->GetNext(), nullptr);
}

TEST(SkipList, EraseByRank_InsertAfterErase)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	// Erase middle
	List.EraseByRank(3, 6); // removes 30, 40, 50, 60
	EXPECT_EQ(List.GetSize(), 6);

	// Insert new elements — should properly integrate
	auto [Node1, Ok1] = List.Insert(11, 35);
	EXPECT_TRUE(Ok1);
	auto [Node2, Ok2] = List.Insert(12, 45);
	EXPECT_TRUE(Ok2);

	EXPECT_EQ(List.GetSize(), 8);
	VerifyListOrder(List, {0, 10, 20, 35, 45, 70, 80, 90});
}

TEST(SkipList, EraseByRank_VerifyAtAfterErase)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
		List.Insert(i, i * 5); // 0, 5, 10, ..., 95

	// Erase ranks 5-12 → removes values at those positions
	List.EraseByRank(5, 12); // removes 8 elements

	EXPECT_EQ(List.GetSize(), 12);

	// Verify remaining elements via At() are in order
	for (int i = 0; i < 12; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "At(" << i << ") returned null";
		if (i > 0)
		{
			auto* Prev = List.At(i - 1);
			EXPECT_LT(Prev->Value, Node->Value) << "value not monotonically increasing at index " << i;
		}
	}
}

TEST(SkipList, EraseByRank_LargeNumberOfElements)
{
	IntSkipList List;
	const int N = 500;
	for (int i = 0; i < N; ++i)
		List.Insert(i, i);

	// Erase a chunk in the middle
	EXPECT_EQ(List.EraseByRank(100, 399), 300);
	EXPECT_EQ(List.GetSize(), 200);

	// Verify remaining elements via At()
	for (int i = 0; i < 200; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "At(" << i << ") null";
	}
	// First element: 0, last element: 499
	EXPECT_EQ(List.At(0)->Value, 0);
	EXPECT_EQ(List.At(199)->Value, 499);
}

TEST(SkipList, EraseByRank_HandlerCalledForEachErasedNode)
{
	IntSkipList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	std::vector<int> ErasedValues;
	auto Handler = [&ErasedValues](const IntSkipList::NodeType* Node) {
		ErasedValues.push_back(Node->Value);
	};

	List.EraseByRank(2, 5, Handler); // erases values 20, 30, 40, 50

	EXPECT_EQ(ErasedValues.size(), 4u);
	EXPECT_EQ(ErasedValues[0], 20);
	EXPECT_EQ(ErasedValues[1], 30);
	EXPECT_EQ(ErasedValues[2], 40);
	EXPECT_EQ(ErasedValues[3], 50);
}

TEST(SkipList, EraseByRank_ZeroElementsToErase_FromRankEqualsToRankPlusOne)
{
	// This test verifies that when the clamped/adjusted ranks produce no-op,
	// e.g. FromRank = ToRank + 1 after clamping, no elements are erased.
	// Actually FromRank > ToRank is checked first and returns 0.
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// erases elements 3-1 which is invalid
	EXPECT_EQ(List.EraseByRank(3, 1), 0);
	EXPECT_EQ(List.GetSize(), 5);
}

TEST(SkipListFlat, EraseByRank)
{
	FlatSkipList List;
	for (int i = 0; i < 20; ++i)
		List.Insert(i, i * 5); // 0, 5, 10, ..., 95

	// Erase [4, 12] → 9 elements (values 20 through 60)
	EXPECT_EQ(List.EraseByRank(4, 12), 9);
	EXPECT_EQ(List.GetSize(), 11);

	std::vector<int> Expected;
	for (int i = 0; i < 20; ++i)
	{
		int v = i * 5;
		if (i < 4 || i > 12)
			Expected.push_back(v);
	}
	VerifyListOrder(List, Expected);
	VerifyRandomAccess(List, Expected);
}

TEST(SkipListTall, EraseByRank)
{
	TallSkipList List;
	for (int i = 0; i < 15; ++i)
		List.Insert(i, i * 10); // 0, 10, ..., 140

	// Erase [3, 9] → 7 elements (values 30 through 90)
	EXPECT_EQ(List.EraseByRank(3, 9), 7);
	EXPECT_EQ(List.GetSize(), 8);

	std::vector<int> Expected = {0, 10, 20, 100, 110, 120, 130, 140};
	VerifyListOrder(List, Expected);
	VerifyRandomAccess(List, Expected);
}

TEST(SkipListMock, EraseByRank_ControlledLevels)
{
	// Levels: 1, 3, 1, 2, 1
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1, rank 0
	List.Insert(2, 20); // level 3, rank 1
	List.Insert(3, 30); // level 1, rank 2
	List.Insert(4, 40); // level 2, rank 3
	List.Insert(5, 50); // level 1, rank 4

	// Erase middle: ranks 1-3 → removes 20, 30, 40
	EXPECT_EQ(List.EraseByRank(1, 3), 3);
	EXPECT_EQ(List.GetSize(), 2);
	VerifyListOrder(List, {10, 50});
	VerifyRandomAccess(List, {10, 50});
}

TEST(SkipListMock, EraseByRank_EraseHighestLevelNode)
{
	FMockRandFunc Mock({false, true, true, false, false, true, false, false});

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FMockRandFunc, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Mock);

	List.Insert(1, 10); // level 1
	List.Insert(2, 20); // level 3
	List.Insert(3, 30); // level 1
	List.Insert(4, 40); // level 2
	List.Insert(5, 50); // level 1

	// Erase the tallest node (level 3) at rank 1
	EXPECT_EQ(List.EraseByRank(1, 1), 1);
	EXPECT_EQ(List.GetSize(), 4);
	VerifyListOrder(List, {10, 30, 40, 50});
	VerifyRandomAccess(List, {10, 30, 40, 50});
}

TEST(SkipList, EraseByRank_AllFromRankNegative_EraseAll)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// Both FromRank and ToRank bounded by clamping:
	// FromRank=-10 clamped to 0, ToRank=100 clamped to 4 → erases everything
	EXPECT_EQ(List.EraseByRank(-10, 100), 5);
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList, EraseByRank_BothClamped_EdgeCase)
{
	IntSkipList List;
	for (int i = 0; i < 5; ++i)
		List.Insert(i, i * 10);

	// FromRank negative, ToRank negative → FromRank=0, ToRank=-1 → FromRank > ToRank → return 0
	EXPECT_EQ(List.EraseByRank(-5, -1), 0);
	EXPECT_EQ(List.GetSize(), 5);
}

TEST(SkipList, EraseByRank_VerifySpans_AfterPartialErase)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
		List.Insert(i, i * 10); // 0, 10, ..., 190

	// Erase first 5 elements
	List.EraseByRank(0, 4);

	// Verify At() works for all remaining elements
	for (int i = 0; i < 15; ++i)
	{
		auto* Node = List.At(i);
		ASSERT_NE(Node, nullptr) << "At(" << i << ") null";
		EXPECT_EQ(Node->Value, (i + 5) * 10);
	}
}

TEST(SkipList, EraseByRank_RepeatedEraseAndInsert)
{
	IntSkipList List;
	for (int i = 0; i < 20; ++i)
		List.Insert(i, i);

	// Erase middle
	EXPECT_EQ(List.EraseByRank(5, 14), 10);
	EXPECT_EQ(List.GetSize(), 10);

	// Re-insert erased values
	for (int i = 5; i < 15; ++i)
	{
		auto [Node, Ok] = List.Insert(i, i);
		EXPECT_TRUE(Ok);
	}

	EXPECT_EQ(List.GetSize(), 20);
	VerifyListOrder(List, [&]() {
		std::vector<int> v(20);
		for (int i = 0; i < 20; ++i) v[i] = i;
		return v;
	}());
}
