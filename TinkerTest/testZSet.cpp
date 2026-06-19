#include "gtest/gtest.h"
#include "DataStructure/ZSet.h"
#include <vector>
#include <algorithm>
#include <set>
#include <string>

// ============================================================================
// Convenience type aliases
// ============================================================================

using IntZSet = TK::TZSet<int, int>;

struct FReverseValueComparer
{
	int operator()(const int& A, const int& B) const
	{
		if (A > B) return -1;
		if (A < B) return 1;
		return 0;
	}
};

using DescZSet = TK::TZSet<int, int, FReverseValueComparer>;

// ============================================================================
// 1. Construction
// ============================================================================

TEST(ZSet, ConstructDefault)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
	EXPECT_EQ(ZS.GetFirst(), nullptr);
	EXPECT_EQ(ZS.GetLast(), nullptr);
}

TEST(ZSet, ConstructWithValueComparer)
{
	DescZSet ZS(FReverseValueComparer{});
	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
}

TEST(ZSet, CopyConstructor)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);
	A.Insert(3, 30);

	IntZSet B(A);
	EXPECT_EQ(B.GetSize(), 3);

	// Deep copy: modify original, copy unchanged
	A.Erase(2);
	EXPECT_EQ(A.GetSize(), 2);
	EXPECT_EQ(B.GetSize(), 3);
	EXPECT_TRUE(B.Contains(2));
}

TEST(ZSet, CopyConstructor_Empty)
{
	IntZSet A;
	IntZSet B(A);
	EXPECT_EQ(B.GetSize(), 0);
	EXPECT_TRUE(B.IsEmpty());
}

TEST(ZSet, MoveConstructor)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntZSet B(std::move(A));
	EXPECT_EQ(B.GetSize(), 2);
	EXPECT_TRUE(B.Contains(1));
	EXPECT_TRUE(B.Contains(2));

	// Moved-from should be empty
	EXPECT_EQ(A.GetSize(), 0);  // NOLINT(bugprone-use-after-move)
	EXPECT_TRUE(A.IsEmpty());
}

TEST(ZSet, CopyAssignment)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntZSet B;
	B.Insert(3, 30);
	B = A;

	EXPECT_EQ(B.GetSize(), 2);
	EXPECT_TRUE(B.Contains(1));
	EXPECT_FALSE(B.Contains(3));
}

TEST(ZSet, CopyAssignment_SelfAssignment)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	A = A; // NOLINT: self-assignment test
	EXPECT_EQ(A.GetSize(), 2);
	EXPECT_TRUE(A.Contains(1));
}

TEST(ZSet, MoveAssignment)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntZSet B;
	B.Insert(3, 30);
	B = std::move(A);

	EXPECT_EQ(B.GetSize(), 2);
	EXPECT_TRUE(B.Contains(1));
	EXPECT_FALSE(B.Contains(3));
}

TEST(ZSet, Swap)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntZSet B;
	B.Insert(3, 30);
	B.Insert(4, 40);
	B.Insert(5, 50);

	A.Swap(B);

	EXPECT_EQ(A.GetSize(), 3);
	EXPECT_EQ(B.GetSize(), 2);
	EXPECT_TRUE(A.Contains(3));
	EXPECT_TRUE(B.Contains(1));
}

TEST(ZSet, FriendSwap)
{
	IntZSet A;
	A.Insert(1, 10);

	IntZSet B;
	B.Insert(2, 20);

	using std::swap;
	swap(A, B);

	EXPECT_EQ(A.GetSize(), 1);
	EXPECT_TRUE(A.Contains(2));
	EXPECT_EQ(B.GetSize(), 1);
	EXPECT_TRUE(B.Contains(1));
}

// ============================================================================
// 2. Insert
// ============================================================================

TEST(ZSet, Insert_Single)
{
	IntZSet ZS;
	auto [Node, Ok] = ZS.Insert(1, 100);
	EXPECT_TRUE(Ok);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(ZS.GetSize(), 1);
	EXPECT_FALSE(ZS.IsEmpty());
}

TEST(ZSet, Insert_Multiple)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);
	EXPECT_EQ(ZS.GetSize(), 3);

	// Verify order by value: 10, 20, 30
	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(ZSet, Insert_DuplicateKey)
{
	IntZSet ZS;
	auto [N1, Ok1] = ZS.Insert(1, 100);
	EXPECT_TRUE(Ok1);

	auto [N2, Ok2] = ZS.Insert(1, 200); // same key, different value
	EXPECT_FALSE(Ok2);
	EXPECT_EQ(N2, N1);             // returns existing node
	EXPECT_EQ(N2->Value, 100);    // value unchanged
	EXPECT_EQ(ZS.GetSize(), 1);   // size unchanged
}

TEST(ZSet, Insert_UnsortedInput)
{
	IntZSet ZS;
	ZS.Insert(3, 30);
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	EXPECT_EQ(ZS.GetSize(), 3);

	// Must be in sorted order by value
	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(ZSet, Insert_ReversedValueOrder)
{
	DescZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// Descending by value
	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{30, 20, 10}));
}

// ============================================================================
// 3. Find & Contains
// ============================================================================

TEST(ZSet, Find_Existing)
{
	IntZSet ZS;
	ZS.Insert(1, 100);
	ZS.Insert(2, 200);

	auto* Node = ZS.FindByKey(1);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Key, 1);
	EXPECT_EQ(Node->Value, 100);
}

TEST(ZSet, Find_NonExisting)
{
	IntZSet ZS;
	ZS.Insert(1, 100);

	EXPECT_EQ(ZS.FindByKey(2), nullptr);
}

TEST(ZSet, Find_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.FindByKey(1), nullptr);
}

TEST(ZSet, Find_AfterErase)
{
	IntZSet ZS;
	ZS.Insert(1, 100);
	ZS.Insert(2, 200);
	ZS.Erase(1);

	EXPECT_EQ(ZS.FindByKey(1), nullptr);
	EXPECT_NE(ZS.FindByKey(2), nullptr);
}

TEST(ZSet, Contains)
{
	IntZSet ZS;
	EXPECT_FALSE(ZS.Contains(1));

	ZS.Insert(1, 100);
	EXPECT_TRUE(ZS.Contains(1));
	EXPECT_FALSE(ZS.Contains(2));

	ZS.Erase(1);
	EXPECT_FALSE(ZS.Contains(1));
}

// ============================================================================
// 4. Update
// ============================================================================

TEST(ZSet, Update_Existing)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// Update in-place (no reposition)
	auto* Updated = ZS.Update(2, 25);
	ASSERT_NE(Updated, nullptr);
	EXPECT_EQ(Updated->Value, 25);
	EXPECT_EQ(ZS.GetSize(), 3);

	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{10, 25, 30}));
}

TEST(ZSet, Update_RequiresReposition)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// Change 20 to 5 — must move before 10
	auto* Updated = ZS.Update(2, 5);
	ASSERT_NE(Updated, nullptr);
	EXPECT_EQ(Updated->Value, 5);
	EXPECT_EQ(ZS.GetSize(), 3);

	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{5, 10, 30}));

	// Find should still work after reposition
	auto* Found = ZS.FindByKey(2);
	EXPECT_EQ(Found, Updated);
}

TEST(ZSet, Update_NonExisting)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	auto* Node = ZS.Update(99, 50);
	EXPECT_EQ(Node, nullptr);
	EXPECT_EQ(ZS.GetSize(), 1);
}

TEST(ZSet, Update_SameValue)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	auto* Node = ZS.Update(1, 10);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 10);
	EXPECT_EQ(ZS.GetSize(), 1);
}

// ============================================================================
// 5. InsertOrUpdate
// ============================================================================

TEST(ZSet, InsertOrUpdate_InsertsWhenKeyNotFound)
{
	IntZSet ZS;
	auto [Node, Inserted] = ZS.InsertOrUpdate(1, 100);
	EXPECT_TRUE(Inserted);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 100);
	EXPECT_EQ(ZS.GetSize(), 1);
}

TEST(ZSet, InsertOrUpdate_UpdatesWhenKeyExists)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	auto [Node, Inserted] = ZS.InsertOrUpdate(1, 15);
	EXPECT_FALSE(Inserted);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 15);
	EXPECT_EQ(ZS.GetSize(), 2);

	// Verify K2V still points to same node
	EXPECT_EQ(ZS.FindByKey(1), Node);
}

TEST(ZSet, InsertOrUpdate_RepositionOnUpdate)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	auto [Node, Inserted] = ZS.InsertOrUpdate(2, 5);
	EXPECT_FALSE(Inserted);
	EXPECT_EQ(Node->Value, 5);

	// Find should still work after reposition
	EXPECT_EQ(ZS.FindByKey(2), Node);
}

// ============================================================================
// 6. Erase (single key)
// ============================================================================

TEST(ZSet, Erase_Existing)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	EXPECT_TRUE(ZS.Erase(2));
	EXPECT_EQ(ZS.GetSize(), 2);
	EXPECT_FALSE(ZS.Contains(2));

	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{10, 30}));
}

TEST(ZSet, Erase_NonExisting)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	EXPECT_FALSE(ZS.Erase(99));
	EXPECT_EQ(ZS.GetSize(), 1);
}

TEST(ZSet, Erase_Empty)
{
	IntZSet ZS;
	EXPECT_FALSE(ZS.Erase(1));
	EXPECT_EQ(ZS.GetSize(), 0);
}

TEST(ZSet, Erase_All)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	EXPECT_TRUE(ZS.Erase(1));
	EXPECT_TRUE(ZS.Erase(2));
	EXPECT_TRUE(ZS.Erase(3));

	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
}

TEST(ZSet, Erase_ThenReinsert)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Erase(1);

	auto [Node, Ok] = ZS.Insert(1, 20);
	EXPECT_TRUE(Ok);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(ZS.GetSize(), 1);
	EXPECT_TRUE(ZS.Contains(1));
}

// ============================================================================
// 7. EraseByRange
// ============================================================================

TEST(ZSet, EraseByRange_MultipleElements)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10); // 0, 10, ..., 90

	TK::TRange<int> Range = {{20, false}, {50, false}};
	EXPECT_EQ(ZS.EraseByRange(Range), 4); // erases 20, 30, 40, 50
	EXPECT_EQ(ZS.GetSize(), 6);

	// Verify K2V also updated
	EXPECT_FALSE(ZS.Contains(2)); // value 20
	EXPECT_FALSE(ZS.Contains(3)); // value 30
	EXPECT_FALSE(ZS.Contains(4)); // value 40
	EXPECT_FALSE(ZS.Contains(5)); // value 50
	EXPECT_TRUE(ZS.Contains(0));
	EXPECT_TRUE(ZS.Contains(9));
}

TEST(ZSet, EraseByRange_Empty)
{
	IntZSet ZS;
	TK::TRange<int> Range = {{10, false}, {20, false}};
	EXPECT_EQ(ZS.EraseByRange(Range), 0);
}

TEST(ZSet, EraseByRange_OutOfRange)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	TK::TRange<int> Range = {{30, false}, {40, false}};
	EXPECT_EQ(ZS.EraseByRange(Range), 0);
	EXPECT_EQ(ZS.GetSize(), 2);
}

// ============================================================================
// 8. EraseByRank
// ============================================================================

TEST(ZSet, EraseByRank_SingleElement)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	EXPECT_EQ(ZS.EraseByRank(3, 3), 1);
	EXPECT_EQ(ZS.GetSize(), 9);
	EXPECT_FALSE(ZS.Contains(3));
}

TEST(ZSet, EraseByRank_MultipleElements)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	EXPECT_EQ(ZS.EraseByRank(2, 5), 4); // erases 20, 30, 40, 50
	EXPECT_EQ(ZS.GetSize(), 6);

	// K2V sync check
	EXPECT_FALSE(ZS.Contains(2));
	EXPECT_FALSE(ZS.Contains(5));
	EXPECT_TRUE(ZS.Contains(0));
	EXPECT_TRUE(ZS.Contains(6));
}

TEST(ZSet, EraseByRank_AllElements)
{
	IntZSet ZS;
	for (int i = 0; i < 5; ++i)
		ZS.Insert(i, i * 10);

	EXPECT_EQ(ZS.EraseByRank(0, 4), 5);
	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
}

TEST(ZSet, EraseByRank_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.EraseByRank(0, 5), 0);
}

// ============================================================================
// 9. Clear
// ============================================================================

TEST(ZSet, Clear)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	ZS.Clear();
	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
	EXPECT_FALSE(ZS.Contains(1));
	EXPECT_EQ(ZS.FindByKey(1), nullptr);

	// Reusable after clear
	auto [Node, Ok] = ZS.Insert(1, 100);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(ZS.GetSize(), 1);
}

// ============================================================================
// 10. Rank (random access)
// ============================================================================

TEST(ZSet, Rank_ValidIndices)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	for (int i = 0; i < 10; ++i)
	{
		auto* Node = ZS.FindByRank(i);
		ASSERT_NE(Node, nullptr) << "Rank(" << i << ") null";
		EXPECT_EQ(Node->Value, i * 10);
	}
}

TEST(ZSet, Rank_OutOfBounds)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	EXPECT_EQ(ZS.FindByRank(-1), nullptr);
	EXPECT_EQ(ZS.FindByRank(1), nullptr);
	EXPECT_EQ(ZS.FindByRank(100), nullptr);
}

TEST(ZSet, Rank_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.FindByRank(0), nullptr);
}

// ============================================================================
// 11. Range queries
// ============================================================================

TEST(ZSet, GetCountInRange)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10); // 0, 10, ..., 90

	TK::TRange<int> Range = {{25, false}, {65, false}};
	EXPECT_EQ(ZS.GetCountInRange(Range), 4); // 30, 40, 50, 60
}

TEST(ZSet, ContainsAnyInRange)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	TK::TRange<int> Range1 = {{15, false}, {25, false}};
	EXPECT_TRUE(ZS.ContainsAnyInRange(Range1)); // 20 is in range

	TK::TRange<int> Range2 = {{40, false}, {50, false}};
	EXPECT_FALSE(ZS.ContainsAnyInRange(Range2));
}

TEST(ZSet, GetFirstInLowerBound)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);
	ZS.Insert(4, 40);

	TK::TRangeBound<int> Bound = {25, false};
	auto [Node, Index] = ZS.GetFirstInLowerBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 30);
	EXPECT_EQ(Index, 2);
}

TEST(ZSet, GetLastInUpperBound)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);
	ZS.Insert(4, 40);

	TK::TRangeBound<int> Bound = {25, false};
	auto [Node, Index] = ZS.GetLastInUpperBound(Bound);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 20);
	EXPECT_EQ(Index, 1);
}

// ============================================================================
// 12. Iteration
// ============================================================================

TEST(ZSet, RangeFor_ForwardIteration)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	std::vector<int> Values;
	for (const auto& Node : ZS)
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{10, 20, 30}));
}

TEST(ZSet, RangeFor_Empty)
{
	IntZSet ZS;
	int Count = 0;
	for (const auto& Node : ZS)
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);
}

TEST(ZSet, ReverseIteration)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	std::vector<int> Values;
	for (auto It = ZS.rbegin(); It != ZS.rend(); ++It)
		Values.push_back(It->Value);
	EXPECT_EQ(Values, (std::vector<int>{30, 20, 10}));
}

TEST(ZSet, BeginEnd_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.begin(), ZS.end());
}

TEST(ZSet, BeginEnd_NonEmpty)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	EXPECT_NE(ZS.begin(), ZS.end());
}

// ============================================================================
// 13. K2V / SortedVList consistency
// ============================================================================

TEST(ZSet, K2V_Sync_AfterInsert)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	// Find returns the same pointer as iteration
	auto* Found1 = ZS.FindByKey(1);
	auto* Found2 = ZS.FindByKey(2);
	ASSERT_NE(Found1, nullptr);
	ASSERT_NE(Found2, nullptr);

	bool Found1InList = false;
	bool Found2InList = false;
	for (const auto& Node : ZS)
	{
		if (&Node == Found1) Found1InList = true;
		if (&Node == Found2) Found2InList = true;
	}
	EXPECT_TRUE(Found1InList);
	EXPECT_TRUE(Found2InList);
}

TEST(ZSet, K2V_Sync_AfterErase)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	ZS.Erase(3);
	ZS.Erase(7);

	for (int i = 0; i < 10; ++i)
	{
		if (i == 3 || i == 7)
			EXPECT_FALSE(ZS.Contains(i)) << "i=" << i;
		else
			EXPECT_TRUE(ZS.Contains(i)) << "i=" << i;
	}
}

TEST(ZSet, K2V_Sync_AfterUpdate)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// Update with reposition
	ZS.Update(2, 5);

	auto* Found = ZS.FindByKey(2);
	ASSERT_NE(Found, nullptr);
	EXPECT_EQ(Found->Value, 5);

	// Verify the pointer in K2V matches the node in the list
	bool FoundInList = false;
	for (const auto& Node : ZS)
	{
		if (&Node == Found) FoundInList = true;
	}
	EXPECT_TRUE(FoundInList);
}

TEST(ZSet, K2V_Sync_AfterInsertOrUpdate)
{
	IntZSet ZS;
	ZS.InsertOrUpdate(1, 10);
	ZS.InsertOrUpdate(1, 20); // update

	auto* Found = ZS.FindByKey(1);
	ASSERT_NE(Found, nullptr);
	EXPECT_EQ(Found->Value, 20);

	bool FoundInList = false;
	for (const auto& Node : ZS)
	{
		if (&Node == Found) FoundInList = true;
	}
	EXPECT_TRUE(FoundInList);
}

TEST(ZSet, K2V_Sync_AfterEraseByRange)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	TK::TRange<int> Range = {{20, false}, {50, false}};
	ZS.EraseByRange(Range);

	// All remaining elements should be findable
	for (const auto& Node : ZS)
	{
		EXPECT_EQ(ZS.FindByKey(Node.Key), &Node);
	}
}

TEST(ZSet, K2V_Sync_AfterEraseByRank)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	ZS.EraseByRank(2, 6);

	for (const auto& Node : ZS)
	{
		EXPECT_EQ(ZS.FindByKey(Node.Key), &Node);
	}
}

TEST(ZSet, K2V_Sync_AfterClear)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	ZS.Clear();

	for (int i = 0; i < 10; ++i)
		EXPECT_FALSE(ZS.Contains(i));
}

// ============================================================================
// 14. Large dataset
// ============================================================================

TEST(ZSet, LargeNumberOfElements)
{
	IntZSet ZS;
	const int N = 1000;
	for (int i = 0; i < N; ++i)
		ZS.Insert(i, i);

	EXPECT_EQ(ZS.GetSize(), N);

	// Verify all elements via Find
	for (int i = 0; i < N; ++i)
	{
		auto* Node = ZS.FindByKey(i);
		ASSERT_NE(Node, nullptr) << "i=" << i;
		EXPECT_EQ(Node->Value, i);
	}

	// Verify ordered iteration
	int PrevValue = -1;
	for (const auto& Node : ZS)
	{
		EXPECT_GT(Node.Value, PrevValue);
		PrevValue = Node.Value;
	}
}

TEST(ZSet, RepeatedInsertErase)
{
	IntZSet ZS;
	std::set<int> Ref;

	for (int Round = 0; Round < 100; ++Round)
	{
		int Val = Round;
		if (Round % 3 == 0)
		{
			auto [Node, Ok] = ZS.Insert(Val, Val);
			if (Ok) Ref.insert(Val);
		}
		else if (Round % 3 == 1)
		{
			if (ZS.Erase(Val - 1))
				Ref.erase(Val - 1);
		}
	}

	EXPECT_EQ(ZS.GetSize(), static_cast<int>(Ref.size()));
	for (int v : Ref)
		EXPECT_TRUE(ZS.Contains(v));
}

// ============================================================================
// 15. Edge cases
// ============================================================================

TEST(ZSet, InsertAfterClear)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Clear();

	ZS.Insert(3, 30);
	ZS.Insert(4, 40);
	EXPECT_EQ(ZS.GetSize(), 2);
	EXPECT_TRUE(ZS.Contains(3));
	EXPECT_TRUE(ZS.Contains(4));
}

TEST(ZSet, GetFirst_GetLast)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.GetFirst(), nullptr);
	EXPECT_EQ(ZS.GetLast(), nullptr);

	ZS.Insert(1, 10);
	EXPECT_EQ(ZS.GetFirst(), ZS.FindByKey(1));
	EXPECT_EQ(ZS.GetLast(), ZS.FindByKey(1));

	ZS.Insert(2, 20);
	EXPECT_EQ(ZS.GetFirst()->Value, 10);
	EXPECT_EQ(ZS.GetLast()->Value, 20);
}

TEST(ZSet, MoveSemantics_PreservesK2V)
{
	IntZSet A;
	A.Insert(1, 10);
	A.Insert(2, 20);

	IntZSet B(std::move(A));

	// B should have functional K2V
	EXPECT_TRUE(B.Contains(1));
	EXPECT_TRUE(B.Contains(2));
	EXPECT_EQ(B.FindByKey(1)->Value, 10);
}

// ============================================================================
// 16. GetRank
// ============================================================================

TEST(ZSet, GetRank_FirstElement)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	EXPECT_EQ(ZS.GetRank(1), 0);
}

TEST(ZSet, GetRank_MiddleElement)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10); // 0, 10, ..., 90

	EXPECT_EQ(ZS.GetRank(5), 5);
}

TEST(ZSet, GetRank_LastElement)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	EXPECT_EQ(ZS.GetRank(3), 2);
}

TEST(ZSet, GetRank_AfterInsert)
{
	IntZSet ZS;
	ZS.Insert(1, 10);  // rank 0
	ZS.Insert(3, 30);  // rank 1
	ZS.Insert(2, 20);  // inserts between → rank 1, pushes 30 to rank 2

	EXPECT_EQ(ZS.GetRank(1), 0);
	EXPECT_EQ(ZS.GetRank(2), 1);
	EXPECT_EQ(ZS.GetRank(3), 2);
}

TEST(ZSet, GetRank_AfterErase)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);
	ZS.Insert(4, 40);

	ZS.Erase(2); // remove middle

	EXPECT_EQ(ZS.GetRank(1), 0);
	EXPECT_EQ(ZS.GetRank(3), 1);
	EXPECT_EQ(ZS.GetRank(4), 2);
}

TEST(ZSet, GetRank_AfterUpdate)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// 20 → 5 repositioned before 10
	ZS.Update(2, 5);

	EXPECT_EQ(ZS.GetRank(2), 0); // value 5 → rank 0
	EXPECT_EQ(ZS.GetRank(1), 1); // value 10 → rank 1
	EXPECT_EQ(ZS.GetRank(3), 2); // value 30 → rank 2
}

TEST(ZSet, GetRank_NonExisting)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	EXPECT_EQ(ZS.GetRank(99), -1);
}

TEST(ZSet, GetRank_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.GetRank(1), -1);
}

TEST(ZSet, GetRank_ConsistencyWithFindByRank)
{
	IntZSet ZS;
	for (int i = 0; i < 50; ++i)
		ZS.Insert(i, i * 3);

	for (int i = 0; i < 50; ++i)
	{
		auto Rank = ZS.GetRank(i);
		ASSERT_NE(Rank, -1) << "i=" << i;
		auto* Node = ZS.FindByRank(Rank);
		ASSERT_NE(Node, nullptr);
		EXPECT_EQ(Node->Key, i) << "i=" << i;
	}
}

// ============================================================================
// 17. PopFirst / PopLast
// ============================================================================

TEST(ZSet, PopFirst_NonEmpty)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	auto Result = ZS.PopFirst();
	ASSERT_TRUE(Result.has_value());
	auto [Key, Value] = *Result;
	EXPECT_EQ(Key, 1);
	EXPECT_EQ(Value, 10);
	EXPECT_EQ(ZS.GetSize(), 2);
	EXPECT_FALSE(ZS.Contains(1));
}

TEST(ZSet, PopFirst_Empty)
{
	IntZSet ZS;
	auto Result = ZS.PopFirst();
	EXPECT_FALSE(Result.has_value());
	EXPECT_EQ(ZS.GetSize(), 0);
}

TEST(ZSet, PopFirst_SingleElement)
{
	IntZSet ZS;
	ZS.Insert(1, 100);

	auto Result = ZS.PopFirst();
	ASSERT_TRUE(Result.has_value());
	EXPECT_EQ(Result->first, 1);
	EXPECT_EQ(Result->second, 100);

	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
	EXPECT_EQ(ZS.FindByKey(1), nullptr);
}

TEST(ZSet, PopFirst_ThenInsert)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	auto Result = ZS.PopFirst();
	ASSERT_TRUE(Result.has_value());
	EXPECT_EQ(Result->second, 10);

	// Re-insert the popped key with a new value
	auto [Node, Ok] = ZS.Insert(1, 15);
	EXPECT_TRUE(Ok);
	EXPECT_EQ(ZS.GetSize(), 2);
	EXPECT_EQ(ZS.FindByKey(1)->Value, 15);
}

TEST(ZSet, PopFirst_K2VSync)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	for (int i = 0; i < 5; ++i)
	{
		auto Result = ZS.PopFirst();
		ASSERT_TRUE(Result.has_value());
	}

	// Remaining 5 elements should all be findable
	for (const auto& Node : ZS)
	{
		EXPECT_EQ(ZS.FindByKey(Node.Key), &Node);
	}
	EXPECT_EQ(ZS.GetSize(), 5);
}

TEST(ZSet, PopLast_NonEmpty)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	auto Result = ZS.PopLast();
	ASSERT_TRUE(Result.has_value());
	auto [Key, Value] = *Result;
	EXPECT_EQ(Key, 3);  // value 30 = largest
	EXPECT_EQ(Value, 30);
	EXPECT_EQ(ZS.GetSize(), 2);
	EXPECT_FALSE(ZS.Contains(3));
}

TEST(ZSet, PopLast_Empty)
{
	IntZSet ZS;
	auto Result = ZS.PopLast();
	EXPECT_FALSE(Result.has_value());
}

TEST(ZSet, PopLast_SingleElement)
{
	IntZSet ZS;
	ZS.Insert(1, 100);

	auto Result = ZS.PopLast();
	ASSERT_TRUE(Result.has_value());
	EXPECT_EQ(Result->first, 1);

	EXPECT_EQ(ZS.GetSize(), 0);
	EXPECT_TRUE(ZS.IsEmpty());
}

TEST(ZSet, PopLast_DescendingOrder)
{
	DescZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	// Descending: 30, 20, 10. PopLast = smallest value = 10
	auto Result = ZS.PopLast();
	ASSERT_TRUE(Result.has_value());
	EXPECT_EQ(Result->second, 10);
	EXPECT_EQ(ZS.GetSize(), 2);
}

TEST(ZSet, PopFirst_PopLast_Alternating)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10); // 0, 10, ..., 90

	auto First = ZS.PopFirst();
	ASSERT_TRUE(First.has_value());
	EXPECT_EQ(First->second, 0);

	auto Last = ZS.PopLast();
	ASSERT_TRUE(Last.has_value());
	EXPECT_EQ(Last->second, 90);

	auto First2 = ZS.PopFirst();
	ASSERT_TRUE(First2.has_value());
	EXPECT_EQ(First2->second, 10);

	auto Last2 = ZS.PopLast();
	ASSERT_TRUE(Last2.has_value());
	EXPECT_EQ(Last2->second, 80);

	EXPECT_EQ(ZS.GetSize(), 6);
}

// ============================================================================
// 18. EraseByRank (single element)
// ============================================================================

TEST(ZSet, EraseByRank_SingleArg)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	EXPECT_EQ(ZS.EraseByRank(3), 1);
	EXPECT_EQ(ZS.GetSize(), 9);
	EXPECT_FALSE(ZS.Contains(3));
}

TEST(ZSet, EraseByRank_SingleArg_First)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	EXPECT_EQ(ZS.EraseByRank(0), 1);
	EXPECT_EQ(ZS.GetSize(), 1);
	EXPECT_FALSE(ZS.Contains(1));
}

TEST(ZSet, EraseByRank_SingleArg_Last)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);

	EXPECT_EQ(ZS.EraseByRank(1), 1);
	EXPECT_EQ(ZS.GetSize(), 1);
	EXPECT_FALSE(ZS.Contains(2));
}

TEST(ZSet, EraseByRank_SingleArg_Empty)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.EraseByRank(0), 0);
}

// ============================================================================
// 19. FindByKey / FindByRank
// ============================================================================

TEST(ZSet, FindByKey_Existing)
{
	IntZSet ZS;
	ZS.Insert(1, 100);
	ZS.Insert(2, 200);

	auto* Node = ZS.FindByKey(1);
	ASSERT_NE(Node, nullptr);
	EXPECT_EQ(Node->Value, 100);
}

TEST(ZSet, FindByKey_NonExisting)
{
	IntZSet ZS;
	EXPECT_EQ(ZS.FindByKey(1), nullptr);
}

TEST(ZSet, FindByRank_FirstAndLast)
{
	IntZSet ZS;
	ZS.Insert(1, 10);
	ZS.Insert(2, 20);
	ZS.Insert(3, 30);

	EXPECT_EQ(ZS.FindByRank(0)->Value, 10);
	EXPECT_EQ(ZS.FindByRank(1)->Value, 20);
	EXPECT_EQ(ZS.FindByRank(2)->Value, 30);
}

TEST(ZSet, FindByRank_OutOfBounds)
{
	IntZSet ZS;
	ZS.Insert(1, 10);

	EXPECT_EQ(ZS.FindByRank(-1), nullptr);
	EXPECT_EQ(ZS.FindByRank(1), nullptr);
}

TEST(ZSet, FindByRank_Vs_FindByKey)
{
	IntZSet ZS;
	for (int i = 0; i < 20; ++i)
		ZS.Insert(i, i * 5);

	for (int i = 0; i < 20; ++i)
	{
		auto* ByRank = ZS.FindByRank(i);
		auto* ByKey  = ZS.FindByKey(ByRank->Key);
		EXPECT_EQ(ByRank, ByKey) << "i=" << i;
	}
}

// ============================================================================
// 20. FEraser — K2V sync in range operations
// ============================================================================

TEST(ZSet, EraseByRange_K2VSync)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	TK::TRange<int> Range = {{2, false}, {7, false}};
	ZS.EraseByRange(Range);

	for (int i = 0; i < 10; ++i)
	{
		if (i >= 2 && i <= 7)
			EXPECT_FALSE(ZS.Contains(i)) << "i=" << i;
		else
			EXPECT_TRUE(ZS.Contains(i)) << "i=" << i;
	}
}

TEST(ZSet, EraseByRank_K2VSync)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	ZS.EraseByRank(3, 6);

	for (int i = 0; i < 10; ++i)
	{
		if (i >= 3 && i <= 6)
			EXPECT_FALSE(ZS.Contains(i)) << "i=" << i;
		else
			EXPECT_TRUE(ZS.Contains(i)) << "i=" << i;
	}
}

TEST(ZSet, PopFirst_K2VSync_AllPopped)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i);

	while (!ZS.IsEmpty())
	{
		auto Result = ZS.PopFirst();
		ASSERT_TRUE(Result.has_value());
		EXPECT_FALSE(ZS.Contains(Result->first));
	}
	EXPECT_EQ(ZS.GetSize(), 0);
}

// ============================================================================
// 21. Range views: Reverse / IterateRange / IterateRank
// ============================================================================

TEST(ZSet, Reverse_RangeFor)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	std::vector<int> Values;
	for (const auto& Node : ZS.Reverse())
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{90, 80, 70, 60, 50, 40, 30, 20, 10, 0}));
}

TEST(ZSet, Reverse_Empty)
{
	IntZSet ZS;
	int Count = 0;
	for (const auto& Node : ZS.Reverse())
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);
}

TEST(ZSet, IterateRange_Partial)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	TK::TRange<int> Range = {{25, false}, {65, false}};
	std::vector<int> Values;
	for (const auto& Node : ZS.IterateRange(Range))
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{30, 40, 50, 60}));
}

TEST(ZSet, IterateRange_NoMatch)
{
	IntZSet ZS;
	for (int i = 0; i < 5; ++i)
		ZS.Insert(i, i * 10);

	TK::TRange<int> Range = {{60, false}, {80, false}};
	int Count = 0;
	for (const auto& Node : ZS.IterateRange(Range))
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);
}

TEST(ZSet, IterateRank_Partial)
{
	IntZSet ZS;
	for (int i = 0; i < 10; ++i)
		ZS.Insert(i, i * 10);

	std::vector<int> Values;
	for (const auto& Node : ZS.IterateRank(3, 6))
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{30, 40, 50, 60}));
}

TEST(ZSet, IterateRank_Single)
{
	IntZSet ZS;
	for (int i = 0; i < 5; ++i)
		ZS.Insert(i, i * 10);

	std::vector<int> Values;
	for (const auto& Node : ZS.IterateRank(2, 2))
		Values.push_back(Node.Value);
	EXPECT_EQ(Values, (std::vector<int>{20}));
}

TEST(ZSet, IterateRank_Empty)
{
	IntZSet ZS;
	int Count = 0;
	for (const auto& Node : ZS.IterateRank(0, 5))
	{
		(void)Node;
		++Count;
	}
	EXPECT_EQ(Count, 0);
}
