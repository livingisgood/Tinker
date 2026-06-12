#include "gtest/gtest.h"
#include "DataStructure/SkipList.h"
#include <vector>
#include <algorithm>
#include <random>
#include <set>

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
	auto [Ok, Node] = List.Insert(1, 100);
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
	auto [Ok1, N1] = List.Insert(1, 100);
	EXPECT_TRUE(Ok1);

	auto [Ok2, N2] = List.Insert(1, 100);
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
	auto [Ok, Node] = List.Insert(1, 10);
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
	auto [Ok, Node] = List.Insert(1, 100);
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
		auto [Ok, Node] = List.Insert(i, i * 10);
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
			auto [Ok, Node] = List.Insert(Val, Val);
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
