#include "gtest/gtest.h"
#include "../DataStructure/SkipList.h"

// ─── Deterministic RandFunc helpers ────────────────────────────

struct FAlwaysLevel1
{
	bool operator()() const { return false; }
};

struct FAlwaysMaxLevel
{
	bool operator()() const { return true; }
};

struct FSequenceRand
{
	std::vector<bool> Sequence;
	mutable int CallIndex = 0;

	bool operator()() const
	{
		if (CallIndex < static_cast<int>(Sequence.size()))
			return Sequence[CallIndex++];
		return false;
	}
};

// Deterministic test list type (all nodes at level 1)
using FTestList = TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FAlwaysLevel1>;

// ─── Custom comparer: descending value order ───────────────────

struct FDescendingComparer
{
	int operator()(const int& A, const int& B) const
	{
		if (A > B) return -1;
		if (A < B) return 1;
		return 0;
	}
};

// ─── Construction & Basic Properties ───────────────────────────

TEST(SkipList_Construction, Default)
{
	FTestList List;
	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList_Construction, FullCustomParams)
{
	FAlwaysLevel1 Rand;
	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FAlwaysLevel1> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);
	EXPECT_EQ(List.GetSize(), 0);
}

TEST(SkipList_Construction, WithValueComparer)
{
	FDescendingComparer Cmp;
	TK::TSkipList<int, int, FDescendingComparer> List(Cmp);
	EXPECT_EQ(List.GetSize(), 0);
}

TEST(SkipList_Construction, MoveCtor)
{
	FTestList Src;
	Src.Insert(1, 10);
	Src.Insert(2, 20);

	FTestList Dst(std::move(Src));
	EXPECT_EQ(Dst.GetSize(), 2);
	EXPECT_TRUE(Src.IsEmpty());
	EXPECT_EQ(Dst[0]->Value, 10);
	EXPECT_EQ(Dst[1]->Value, 20);
}

TEST(SkipList_Construction, MoveAssignment)
{
	FTestList Src;
	Src.Insert(1, 10);

	FTestList Dst;
	Dst.Insert(5, 50);
	Dst = std::move(Src);

	EXPECT_EQ(Dst.GetSize(), 1);
	EXPECT_EQ(Dst[0]->Value, 10);
}

TEST(SkipList_Copy, IndependentCopy)
{
	FTestList Src;
	Src.Insert(3, 30);
	Src.Insert(1, 10);
	Src.Insert(2, 20);

	FTestList Copy(Src);
	EXPECT_EQ(Copy.GetSize(), 3);
	EXPECT_EQ(Copy[0]->Key, 1);
	EXPECT_EQ(Copy[1]->Key, 2);
	EXPECT_EQ(Copy[2]->Key, 3);

	Src.Insert(4, 40);
	EXPECT_EQ(Copy.GetSize(), 3);
}

TEST(SkipList_Copy, Assignment)
{
	FTestList Src;
	Src.Insert(1, 100);

	FTestList Dst;
	Dst.Insert(9, 900);
	Dst = Src;

	EXPECT_EQ(Dst.GetSize(), 1);
	EXPECT_EQ(Dst[0]->Key, 1);
	EXPECT_EQ(Dst[0]->Value, 100);
}

TEST(SkipList_Copy, SelfAssignment)
{
	FTestList List;
	List.Insert(1, 10);
	List = List; // NOLINT

	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_EQ(List[0]->Key, 1);
}

TEST(SkipList_Clear, AfterClear)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Clear();

	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList_Clear, ReuseAfterClear)
{
	FTestList List;
	List.Insert(1, 10);
	List.Clear();
	List.Insert(5, 50);

	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_EQ(List[0]->Key, 5);
}

TEST(SkipList_Swap, TwoNonEmpty)
{
	FTestList A;
	A.Insert(1, 10);
	A.Insert(3, 30);

	FTestList B;
	B.Insert(2, 20);

	A.Swap(B);

	EXPECT_EQ(A.GetSize(), 1);
	EXPECT_EQ(A[0]->Key, 2);
	EXPECT_EQ(B.GetSize(), 2);
	EXPECT_EQ(B[0]->Key, 1);
	EXPECT_EQ(B[1]->Key, 3);
}

TEST(SkipList_Swap, WithEmpty)
{
	FTestList A;
	A.Insert(1, 10);

	FTestList B;
	A.Swap(B);

	EXPECT_TRUE(A.IsEmpty());
	EXPECT_EQ(B.GetSize(), 1);
}

// ─── Insert ─────────────────────────────────────────────────────

TEST(SkipList_Insert, IntoEmpty)
{
	FTestList List;
	auto [bOk, Node] = List.Insert(1, 100);

	EXPECT_TRUE(bOk);
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_FALSE(List.IsEmpty());
}

TEST(SkipList_Insert, MultipleInOrder)
{
	FTestList List;
	for (int i = 0; i < 10; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.GetSize(), 10);
	for (int i = 0; i < 10; ++i)
	{
		EXPECT_EQ(List[i]->Key, i);
		EXPECT_EQ(List[i]->Value, i * 10);
	}
}

TEST(SkipList_Insert, DuplicateKeyValue)
{
	FTestList List;
	List.Insert(1, 100);
	auto [bOk, Node] = List.Insert(1, 100);

	EXPECT_FALSE(bOk);
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList_Insert, SameValueDifferentKey)
{
	FTestList List;
	List.Insert(5, 100);
	List.Insert(2, 100);

	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[0]->Key, 2);
	EXPECT_EQ(List[1]->Key, 5);
}

TEST(SkipList_Insert, LinkChainIntegrity)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto* N = List[0];
	EXPECT_EQ(N->GetPrev(), nullptr);
	EXPECT_EQ(N->GetNext()->Key, 2);
	EXPECT_EQ(N->GetNext()->GetPrev(), N);
	EXPECT_EQ(N->GetNext()->GetNext()->Key, 3);
	EXPECT_EQ(N->GetNext()->GetNext()->GetNext(), nullptr);
}

// ─── Erase ──────────────────────────────────────────────────────

TEST(SkipList_Erase, FromEmpty)
{
	FTestList List;
	EXPECT_FALSE(List.Erase(1, 100));
}

TEST(SkipList_Erase, Existing)
{
	FTestList List;
	List.Insert(1, 100);
	List.Insert(2, 200);

	EXPECT_TRUE(List.Erase(1, 100));
	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_EQ(List[0]->Key, 2);
}

TEST(SkipList_Erase, NonExisting)
{
	FTestList List;
	List.Insert(1, 100);
	EXPECT_FALSE(List.Erase(2, 200));
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList_Erase, Head)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	List.Erase(1, 10);

	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[0]->Key, 2);
	EXPECT_EQ(List[0]->GetPrev(), nullptr);
}

TEST(SkipList_Erase, Tail)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	List.Erase(3, 30);

	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[1]->Key, 2);
	EXPECT_EQ(List[1]->GetNext(), nullptr);
}

TEST(SkipList_Erase, OnlyElement)
{
	FTestList List;
	List.Insert(1, 100);
	List.Erase(1, 100);

	EXPECT_EQ(List.GetSize(), 0);
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList_Erase, ListLevelsShrink)
{
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, false, false, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	EXPECT_TRUE(List.Erase(1, 10));
	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_EQ(List[0]->Key, 2);
}

// ─── Update ─────────────────────────────────────────────────────

TEST(SkipList_Update, EmptyList)
{
	FTestList List;
	EXPECT_EQ(List.Update(1, 10, 20), nullptr);
}

TEST(SkipList_Update, NonExisting)
{
	FTestList List;
	List.Insert(1, 10);
	EXPECT_EQ(List.Update(1, 20, 30), nullptr);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList_Update, SameValue)
{
	FTestList List;
	List.Insert(1, 10);
	auto* Node = List.Update(1, 10, 10);

	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);
	EXPECT_EQ(List.GetSize(), 1);
}

TEST(SkipList_Update, InPlaceIncrease)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 30);

	auto* Node = List.Update(1, 10, 20);

	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[0]->Key, 1);
}

TEST(SkipList_Update, InPlaceDecrease)
{
	FTestList List;
	List.Insert(2, 10);
	List.Insert(1, 30);

	auto* Node = List.Update(1, 30, 20);

	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
}

TEST(SkipList_Update, ReinsertNeeded)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	auto* Node = List.Update(1, 10, 30);

	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 30);
	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[0]->Key, 2);
	EXPECT_EQ(List[1]->Key, 1);
}

// ─── At / operator[] ────────────────────────────────────────────

TEST(SkipList_At, First)
{
	FTestList List;
	List.Insert(3, 30);
	List.Insert(1, 10);
	List.Insert(2, 20);

	EXPECT_EQ(List.At(0)->Key, 1);
	EXPECT_EQ((*const_cast<const decltype(List)*>(&List))[0]->Key, 1);
}

TEST(SkipList_At, Last)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	EXPECT_EQ(List.At(2)->Key, 3);
}

TEST(SkipList_At, OutOfRange)
{
	FTestList List;
	List.Insert(1, 10);
	EXPECT_EQ(List.At(1), nullptr);
	EXPECT_EQ(List.At(-1), nullptr);
	EXPECT_EQ(List.At(100), nullptr);
}

TEST(SkipList_At, NearEndUsesBackwardSearch)
{
	FTestList List;
	for (int i = 0; i < 25; ++i)
		List.Insert(i, i);

	EXPECT_EQ(List.At(22)->Key, 22);
	EXPECT_EQ(List.At(24)->Key, 24);
}

TEST(SkipList_At, NearBeginningUsesForwardSearch)
{
	FTestList List;
	for (int i = 0; i < 25; ++i)
		List.Insert(i, i);

	EXPECT_EQ(List.At(0)->Key, 0);
	EXPECT_EQ(List.At(3)->Key, 3);
}

// ─── ContainsAnyInRange ─────────────────────────────────────────

TEST(SkipList_ContainsAnyInRange, Empty)
{
	FTestList List;
	TK::TRange<int> R = { {1, false}, {10, false} };
	EXPECT_FALSE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, LowerGreaterThanUpper)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	TK::TRange<int> R = { {30, false}, {10, false} };
	EXPECT_FALSE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, EqualBoundsExclusive)
{
	FTestList List;
	List.Insert(1, 10);

	TK::TRange<int> R1 = { {10, false}, {10, true} };
	EXPECT_FALSE(List.ContainsAnyInRange(R1));

	TK::TRange<int> R2 = { {10, true}, {10, false} };
	EXPECT_FALSE(List.ContainsAnyInRange(R2));
}

TEST(SkipList_ContainsAnyInRange, ElementExists)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	TK::TRange<int> R = { {15, false}, {25, false} };
	EXPECT_TRUE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, NoElement)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	TK::TRange<int> R = { {5, false}, {8, false} };
	EXPECT_FALSE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, AllElementsInRange)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	TK::TRange<int> R = { {1, false}, {100, false} };
	EXPECT_TRUE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, ExclusiveBoundExactMatch)
{
	FTestList List;
	List.Insert(1, 10);

	TK::TRange<int> R = { {10, true}, {20, false} };
	EXPECT_FALSE(List.ContainsAnyInRange(R));
}

TEST(SkipList_ContainsAnyInRange, AtBoundary)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 30);

	TK::TRange<int> R = { {10, false}, {30, false} };
	EXPECT_TRUE(List.ContainsAnyInRange(R));
}

// ─── GetFirstInLowerBound ───────────────────────────────────────

TEST(SkipList_GetFirstInLowerBound, EmptyList)
{
	FTestList List;
	auto [Node, Idx] = List.GetFirstInLowerBound({10, false});
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(Idx, -1);
}

TEST(SkipList_GetFirstInLowerBound, AllElementsBelow)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	auto [Node, Idx] = List.GetFirstInLowerBound({50, false});
	EXPECT_EQ(Node, nullptr);
}

TEST(SkipList_GetFirstInLowerBound, AllElementsAbove)
{
	FTestList List;
	List.Insert(1, 30);
	List.Insert(2, 40);

	auto [Node, Idx] = List.GetFirstInLowerBound({10, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Idx, 0);
}

TEST(SkipList_GetFirstInLowerBound, ExactInclusive)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetFirstInLowerBound({20, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetFirstInLowerBound, ExactExclusive)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetFirstInLowerBound({20, true});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
	EXPECT_EQ(Idx, 2);
}

TEST(SkipList_GetFirstInLowerBound, Middle)
{
	FTestList List;
	List.Insert(2, 20);
	List.Insert(1, 10);
	List.Insert(4, 40);
	List.Insert(3, 25);

	auto [Node, Idx] = List.GetFirstInLowerBound({22, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
}

TEST(SkipList_GetFirstInLowerBound, SingleElement)
{
	FTestList List;
	List.Insert(1, 10);

	auto [Node, Idx] = List.GetFirstInLowerBound({10, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Idx, 0);
}

// Regression test for bug: GetFirstInLowerBound returned nullptr when Next was null
// at a non-zero level, instead of descending to find nodes at lower levels.
// List: [K=1,V=10] at levels 0,1,2 and [K=2,V=20] at levels 0,1.
// Searching for {15, false} should find [2,20] at index 1, not nullptr.
TEST(SkipList_GetFirstInLowerBound, DescendThroughNullNext)
{
	FSequenceRand Rand;
	// First node: level 3 (true, true, false)
	// Second node: level 2 (true, false)
	Rand.Sequence = { true, true, false, true, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	auto [Node, Idx] = List.GetFirstInLowerBound({15, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(Idx, 1);
}

// First node at a high level, second at level 1.
// Tests descending through multiple levels of null Next.
TEST(SkipList_GetFirstInLowerBound, DescendThroughMultipleNullNexts)
{
	FSequenceRand Rand;
	// First node: level 4 (true, true, true, false)
	// Second node: level 1 (false)
	Rand.Sequence = { true, true, true, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	auto [Node, Idx] = List.GetFirstInLowerBound({15, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

// Three nodes: first at level 3, middle and last at level 1.
// Tests correct index tracking after advancing and descending through null Next.
TEST(SkipList_GetFirstInLowerBound, DescendWithCorrectIndex)
{
	FSequenceRand Rand;
	// First node: level 3 (true, true, false)
	// Second node: level 1 (false)
	// Third node: level 1 (false)
	Rand.Sequence = { true, true, false, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 5);
	List.Insert(2, 10);
	List.Insert(3, 15);

	auto [Node, Idx] = List.GetFirstInLowerBound({12, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
	EXPECT_EQ(Node->Value, 15);
	EXPECT_EQ(Idx, 2);
}

// Also test that exclusive lower bound works correctly after descent.
TEST(SkipList_GetFirstInLowerBound, ExclusiveBoundAfterDescent)
{
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	// Exclusive bound at 10 should also find [2,20] (since 20 > 10)
	auto [Node, Idx] = List.GetFirstInLowerBound({10, true});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetFirstInLowerBound, MultiLevel_NullNextAtNonZeroLevel)
{
	// A(v=10) at levels 0-2, B(v=20) at level 0 only.
	// LowerBound between A and B: at level 2, A->Next=null, Span=1,
	// but B(20) at level 0 still satisfies the bound.
	// This tests the fix for the Next=null-at-high-level bug.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	// First value >= 15 is B(20) at index 1
	auto [Node, Idx] = List.GetFirstInLowerBound({15, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetFirstInLowerBound, MultiLevel_SkipHighLevelNodes)
{
	// A(v=10) at levels 0-2, B(v=20) at levels 0-1, C(v=30) at level 0, D(v=40) at level 0.
	// Algorithm must skip through high-level nodes by descending when Next=null.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// First value >= 25 is C(30) at index 2
	auto [Node, Idx] = List.GetFirstInLowerBound({25, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
	EXPECT_EQ(Idx, 2);
}

// ─── GetLastInUpperBound ────────────────────────────────────────

TEST(SkipList_GetLastInUpperBound, EmptyList)
{
	FTestList List;
	auto [Node, Idx] = List.GetLastInUpperBound({10, false});
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(Idx, -1);
}

TEST(SkipList_GetLastInUpperBound, AllElementsAbove)
{
	FTestList List;
	List.Insert(1, 30);
	List.Insert(2, 40);

	auto [Node, Idx] = List.GetLastInUpperBound({10, false});
	EXPECT_EQ(Node, nullptr);
}

TEST(SkipList_GetLastInUpperBound, AllElementsBelow)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);

	auto [Node, Idx] = List.GetLastInUpperBound({50, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetLastInUpperBound, ExactInclusive)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetLastInUpperBound({20, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetLastInUpperBound, ExactExclusive)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetLastInUpperBound({20, true});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Idx, 0);
}

TEST(SkipList_GetLastInUpperBound, Middle)
{
	FTestList List;
	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(4, 40);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetLastInUpperBound({35, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
}

TEST(SkipList_GetLastInUpperBound, SingleElement)
{
	FTestList List;
	List.Insert(1, 10);

	auto [Node, Idx] = List.GetLastInUpperBound({10, false});
	EXPECT_NE(Node, nullptr);
	EXPECT_EQ(Idx, 0);
}

TEST(SkipList_GetLastInUpperBound, MultiLevel_TailInBounds)
{
	// A(v=10) at levels 0-2, B(v=20) at levels 0-1, C(v=30) at level 0.
	// Tail (30) is in bounds, so the tail shortcut fires.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);

	auto [Node, Idx] = List.GetLastInUpperBound({35, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 3);
	EXPECT_EQ(Idx, 2);
}

TEST(SkipList_GetLastInUpperBound, MultiLevel_MiddleIsLastInBounds)
{
	// A(v=10) at levels 0-2, B(v=20) at levels 0-1, C(v=30) at level 0, D(v=40) at level 0.
	// Tail is out of bounds. The correct last-in-bounds node is B(20) at index 1.
	// The algorithm must descend from level 2 (A->Next=null, Span=3) through
	// level 1 (A->Next=B, 20<=25) to find B.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Last value <= 25 is B(20) at index 1
	auto [Node, Idx] = List.GetLastInUpperBound({25, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

TEST(SkipList_GetLastInUpperBound, MultiLevel_FirstIsLastInBounds)
{
	// Same 4-node structure. Only A(10) is <= 15. The algorithm must skip
	// level 2 (A->Next=null, Span=3), then check B(20) at level 1 (out of bounds)
	// and return A.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false, false, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);
	List.Insert(3, 30);
	List.Insert(4, 40);

	// Last value <= 15 is A(10) at index 0
	auto [Node, Idx] = List.GetLastInUpperBound({15, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Idx, 0);
}

TEST(SkipList_GetLastInUpperBound, MultiLevel_NullNextAtNonZeroLevel)
{
	// Bug report scenario 1: A(v=10) at levels 0-2, B(v=20) at levels 0-1.
	// At level 2, A->Next=null, Span=1. UpperBound={15,false} -> V<=15.
	// A is the correct answer. This proves that returning Cur when
	// Next=null and Span=1 at level k>0 is correct.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, false, true, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	// Last value <= 15 is A(10) at index 0
	auto [Node, Idx] = List.GetLastInUpperBound({15, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Idx, 0);
}

TEST(SkipList_GetLastInUpperBound, MultiLevel_BugReportScenario2)
{
	// Bug report scenario 2:
	// A(v=10) at levels 0-3, B(v=15) at levels 0-1, C(v=20) at levels 0-1.
	// UpperBound={17,false} -> V<=17. Expected: B(15) at index 1.
	// At levels 2 and 3, A->Next=null, Span=2. The algorithm must descend
	// from level 3->2->1 to find B.
	FSequenceRand Rand;
	Rand.Sequence = { true, true, true, false, true, false, true, false };

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 15);
	List.Insert(3, 20);

	auto [Node, Idx] = List.GetLastInUpperBound({17, false});
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 2);
	EXPECT_EQ(Idx, 1);
}

// ─── Large data / Mixed operations ──────────────────────────────

TEST(SkipList_Edge, LargeInsertions)
{
	FTestList List;
	for (int i = 0; i < 1000; ++i)
		List.Insert(i, i);

	EXPECT_EQ(List.GetSize(), 1000);
	for (int i = 0; i < 1000; ++i)
		EXPECT_EQ(List[i]->Key, i);
}

TEST(SkipList_Edge, InsertAfterMove)
{
	FTestList Src;
	Src.Insert(1, 10);

	FTestList Dst(std::move(Src));
	Dst.Insert(2, 20);

	EXPECT_EQ(Dst.GetSize(), 2);
}

TEST(SkipList_Edge, ReinsertAfterErase)
{
	FTestList List;
	List.Insert(1, 10);
	List.Erase(1, 10);
	List.Insert(1, 10);

	EXPECT_EQ(List.GetSize(), 1);
	EXPECT_EQ(List[0]->Key, 1);
}

TEST(SkipList_Edge, MixedOperations)
{
	FTestList List;

	List.Insert(5, 50);
	List.Insert(3, 30);
	List.Insert(7, 70);
	List.Insert(1, 10);
	List.Insert(9, 90);

	EXPECT_EQ(List.GetSize(), 5);

	List.Erase(3, 30);
	List.Erase(7, 70);

	EXPECT_EQ(List.GetSize(), 3);
	EXPECT_EQ(List[0]->Key, 1);
	EXPECT_EQ(List[1]->Key, 5);
	EXPECT_EQ(List[2]->Key, 9);

	List.Update(5, 50, 60);
	EXPECT_EQ(List[1]->Value, 60);

	List.Insert(3, 30);
	List.Insert(4, 40);

	EXPECT_EQ(List.GetSize(), 5);
	EXPECT_EQ(List[2]->Key, 4);
	EXPECT_EQ(List[3]->Key, 5);
}

// ─── MaxLevel ───────────────────────────────────────────────────

TEST(SkipList_MaxLevel, Level1)
{
	FAlwaysLevel1 Rand;
	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FAlwaysLevel1, 1> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	List.Insert(1, 10);
	List.Insert(2, 20);

	EXPECT_EQ(List.GetSize(), 2);
	EXPECT_EQ(List[0]->Key, 1);
	EXPECT_EQ(List[1]->Key, 2);
}

TEST(SkipList_MaxLevel, MultiLevelOrdering)
{
	// Use a controlled sequence to create known multi-level structure
	FSequenceRand Rand;
	// Nodes: level1, level2, level1, level3, level1, ... (roughly)
	for (int i = 0; i < 100; ++i)
		Rand.Sequence.push_back((i % 4) == 1);

	TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand, 4> List(
		TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

	for (int i = 0; i < 100; ++i)
		List.Insert(i, i * 10);

	EXPECT_EQ(List.GetSize(), 100);
	for (int i = 0; i < 100; ++i)
	{
		EXPECT_EQ(List[i]->Key, i) << " at index " << i;
	}
}

// ─── IsEmpty ────────────────────────────────────────────────────

TEST(SkipList_IsEmpty, NewList)
{
	FTestList List;
	EXPECT_TRUE(List.IsEmpty());
}

TEST(SkipList_IsEmpty, AfterInsert)
{
	FTestList List;
	List.Insert(1, 10);
	EXPECT_FALSE(List.IsEmpty());
}

TEST(SkipList_IsEmpty, AfterClear)
{
	FTestList List;
	List.Insert(1, 10);
	List.Clear();
	EXPECT_TRUE(List.IsEmpty());
}

// ─── Move semantics with custom comparer ────────────────────────

TEST(SkipList_Move, PreservesComparer)
{
	TK::TSkipList<int, int, FDescendingComparer> List;
	List.Insert(1, 10);
	List.Insert(2, 30);
	List.Insert(3, 20);

	EXPECT_EQ(List[0]->Key, 2);
	EXPECT_EQ(List[1]->Key, 3);
	EXPECT_EQ(List[2]->Key, 1);

	auto Moved(std::move(List));
	EXPECT_EQ(Moved.GetSize(), 3);
	EXPECT_EQ(Moved[0]->Key, 2);
	EXPECT_EQ(Moved[1]->Key, 3);
	EXPECT_EQ(Moved[2]->Key, 1);
}

// ─── Multi-Level Find/Erase Bug Regression ──────────────────────
//
// The bug: Find() at a high level locates the target node, then for all
// lower levels just copies Cur (the position at the higher level) as the
// frontier. But Cur at a higher level may skip over intermediate nodes
// that only exist at lower levels, producing an incorrect frontier.
//
// Concrete scenario:
//   A(k=1,v=10) at height 3, B(k=2,v=15) at height 1, X(k=3,v=20) at height 3
//   Level 2: Head -> A -> X
//   Level 0: Head -> A -> B -> X
//   Erase(X) with buggy Find: Frontiers[0]=A instead of B.
//   A->link(0)->Next is set to X's successor = nullptr, orphaning B.

TEST(SkipList_Erase, MultiLevel_OrphanBug)
{
    FSequenceRand Rand;
    // A: height 3 (true, true, false)
    // B: height 1 (false)
    // X: height 3 (true, true, false)
    Rand.Sequence = { true, true, false, false, true, true, false };

    TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
        TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

    List.Insert(1, 10);   // A: height 3
    List.Insert(2, 15);   // B: height 1
    List.Insert(3, 20);   // X: height 3

    EXPECT_EQ(List.GetSize(), 3);

    // Erase the third node (high-level node with lower-level intermediary)
    EXPECT_TRUE(List.Erase(3, 20));

    // Size must be 2
    EXPECT_EQ(List.GetSize(), 2);

    // Both remaining nodes must be reachable by index
    EXPECT_EQ(List[0]->Key, 1);
    EXPECT_EQ(List[0]->Value, 10);
    EXPECT_EQ(List[1]->Key, 2);
    EXPECT_EQ(List[1]->Value, 15);

    // Verify forward level-0 chain
    auto* First = List.At(0);
    ASSERT_NE(First, nullptr);
    EXPECT_EQ(First->Key, 1);
    EXPECT_EQ(First->GetNext()->Key, 2);
    EXPECT_EQ(First->GetNext()->GetNext(), nullptr);

    // Verify backward chain
    EXPECT_EQ(First->GetPrev(), nullptr);
    EXPECT_EQ(First->GetNext()->GetPrev(), First);
}

// Verify that Update with reinsert (erase-then-insert) does not orphan
// intermediate nodes. Same structure as above but calls Update on X(3,20)
// with a new value that forces erase-then-reinsert.
TEST(SkipList_Update, MultiLevel_ReinsertPreservesAllNodes)
{
    FSequenceRand Rand;
    // A: height 3, B: height 1, X: height 3
    Rand.Sequence = { true, true, false, false, true, true, false };

    TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
        TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

    List.Insert(1, 10);   // A: height 3
    List.Insert(2, 15);   // B: height 1
    List.Insert(3, 20);   // X: height 3

    ASSERT_EQ(List.GetSize(), 3);

    // Update X's value from 20 to 5.  Value decreases from 20 to 5,
    // and X->Prev = B(15), and Comparer(B, 3, 5) = 1 (not < 0),
    // so in-place update fails and it falls through to Erase+Insert.
    // This triggers the buggy Find during Erase if the fix is absent.
    auto* Node = List.Update(3, 20, 5);
    ASSERT_NE(Node, nullptr);
    EXPECT_EQ(Node->Value, 5);

    // Size unchanged
    EXPECT_EQ(List.GetSize(), 3);

    // All 3 nodes must be reachable.  Order by value: 5 < 10 < 15
    EXPECT_EQ(List[0]->Key, 3);
    EXPECT_EQ(List[0]->Value, 5);
    EXPECT_EQ(List[1]->Key, 1);
    EXPECT_EQ(List[1]->Value, 10);
    EXPECT_EQ(List[2]->Key, 2);
    EXPECT_EQ(List[2]->Value, 15);

    // Forward chain
    EXPECT_EQ(List[0]->GetPrev(), nullptr);
    EXPECT_EQ(List[0]->GetNext(), List[1]);
    EXPECT_EQ(List[1]->GetNext(), List[2]);
    EXPECT_EQ(List[2]->GetNext(), nullptr);

    // Backward chain
    EXPECT_EQ(List[2]->GetPrev(), List[1]);
    EXPECT_EQ(List[1]->GetPrev(), List[0]);
}

// Asymmetric alternation stress test: 20 nodes where even-indexed inputs
// get height 3 and odd-indexed ones get height 1.  Erase 10 of the high-level
// nodes and verify every remaining node is still reachable and the chain is
// intact.  Without the Find fix, erasing even one high-level non-first node
// orphans the low-level intermediary after it.
TEST(SkipList_Erase, MultiLevel_StressTest)
{
    FSequenceRand Rand;
    // Even-indexed inputs: height 3 (true, true, false)
    // Odd-indexed inputs:  height 1 (false)
    for (int i = 0; i < 20; ++i)
    {
        if (i % 2 == 0)
        {
            Rand.Sequence.push_back(true);
            Rand.Sequence.push_back(true);
            Rand.Sequence.push_back(false);
        }
        else
        {
            Rand.Sequence.push_back(false);
        }
    }

    TK::TSkipList<int, int, TK::TSkipListDefaultComparer<int>, std::less<int>, FSequenceRand> List(
        TK::TSkipListDefaultComparer<int>{}, std::less<int>{}, Rand);

    for (int i = 0; i < 20; ++i)
        List.Insert(i, i * 10);

    ASSERT_EQ(List.GetSize(), 20);

    // Erase all even keys (0, 2, 4, ..., 18) -- these are the height-3 nodes.
    // The first node (key=0) doesn't trigger the bug; all others do.
    for (int i = 0; i < 20; i += 2)
    {
        EXPECT_TRUE(List.Erase(i, i * 10));
    }

    EXPECT_EQ(List.GetSize(), 10);

    // All remaining nodes (odd keys: 1, 3, 5, ..., 19) must be reachable
    // by index and in correct order.
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(List[i]->Key, 2 * i + 1);
        EXPECT_EQ(List[i]->Value, (2 * i + 1) * 10);
    }

    // Walk the entire forward level-0 chain
    auto* Node = List.At(0);
    ASSERT_NE(Node, nullptr);
    EXPECT_EQ(Node->Key, 1);

    for (int i = 1; i < 10; ++i)
    {
        Node = Node->GetNext();
        ASSERT_NE(Node, nullptr);
        EXPECT_EQ(Node->Key, 2 * i + 1);
    }
    EXPECT_EQ(Node->GetNext(), nullptr);

    // Walk the entire backward chain
    for (int i = 8; i >= 0; --i)
    {
        Node = Node->GetPrev();
        ASSERT_NE(Node, nullptr);
        EXPECT_EQ(Node->Key, 2 * i + 1);
    }
    EXPECT_EQ(Node->GetPrev(), nullptr);
}
