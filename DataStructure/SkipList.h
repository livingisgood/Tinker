#pragma once
#include <iterator>
#include <random>
#include <utility>

#if __cplusplus >= 202002L || (defined(_MSC_VER) && _MSVC_LANG >= 202002L)
#include <compare>
#endif

namespace TK
{
	struct FSkipListLevelRand
	{
		bool operator()() const
		{
			thread_local std::mt19937 Engine(std::random_device{}());
			std::uniform_int_distribution<int> Dist(0, 3);
			return Dist(Engine) == 0;
		}
	};

	template <typename T>
	struct TSkipListDefaultComparer
	{
		int operator()(const T& A, const T& B) const
		{
#if __cplusplus >= 202002L || (defined(_MSC_VER) && _MSVC_LANG >= 202002L)
			auto Ordering = A <=> B;
			return Ordering < 0 ? -1 : (Ordering > 0 ? 1 : 0);
#else
			std::less<T> Less{};
			if (Less(A, B))
				return -1;
			return Less(B, A) ? 1 : 0;
#endif
		}
	};

	template <typename T>
	struct TRangeBound
	{
		T Value;
		bool bExclusive;
	};

	template <typename T>
	struct TRange
	{
		TRangeBound<T> LowerBound;
		TRangeBound<T> UpperBound;
	};

	template <typename K, typename V>
	struct TSkipNode
	{
		using SizeType = int;

		struct FLink
		{
			TSkipNode* Next{nullptr};
			SizeType Span{0};
		};

		K Key;
		V Value;
		TSkipNode* Prev{nullptr};
		FLink Links[1]{}; // zero length array is not allowed in c++

		template <typename KeyT, typename ValueT>
		explicit TSkipNode(KeyT&& InKey, ValueT&& InValue)
			: Key(std::forward<KeyT>(InKey)), Value(std::forward<ValueT>(InValue)) {}

		FLink* GetLink(int Level)
		{
			return reinterpret_cast<FLink*>(Links) + Level;
		}

		const FLink* GetLink(int Level) const
		{
			return reinterpret_cast<const FLink*>(Links) + Level;
		}

		TSkipNode* GetPrev() const { return Prev; }
		TSkipNode* GetNext() const { return GetLink(0)->Next; }

		template <typename KeyT, typename ValueT>
		static TSkipNode* Create(KeyT&& InKey, ValueT&& InValue, int Levels)
		{
			void* Memory = std::malloc(sizeof(TSkipNode) + (Levels - 1) * sizeof(FLink));
			TSkipNode* Node = new(Memory)TSkipNode(std::forward<KeyT>(InKey), std::forward<ValueT>(InValue));

			for (int i = 0; i < Levels; ++i)
			{
				FLink* Link = Node->GetLink(i);
				Link->Next = nullptr;
				Link->Span = 0;
			}

			return Node;
		}

		static void Free(TSkipNode* Node)
		{
			Node->~TSkipNode();
			std::free(Node);
		}
	};

	template<
		typename K,
		typename V,
		typename ValueComparerType = TSkipListDefaultComparer<V>,
		typename KeyComparerType = std::less<K>,
		typename RandFuncT = FSkipListLevelRand,
		int MaxLevel = 32>
	class TSkipList
	{
		static_assert(MaxLevel >= 1);

	public:

		using NodeType = TSkipNode<K, V>;
		using NodeLink = typename TSkipNode<K, V>::FLink;
		using SizeType = typename TSkipNode<K, V>::SizeType;

		class FIterator
		{
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = NodeType;
			using difference_type = std::ptrdiff_t;
			using pointer = const NodeType*;
			using reference = const NodeType&;

			FIterator() = default;
			explicit FIterator(pointer InNode) : Node(InNode) {}

			reference operator*() const { return *Node; }
			pointer   operator->() const { return Node; }

			FIterator& operator++() { Node = Node->GetNext(); return *this; }
			FIterator  operator++(int) { auto Tmp = *this; Node = Node->GetNext(); return Tmp; }

			FIterator& operator--() { Node = Node->GetPrev(); return *this; }
			FIterator  operator--(int) { auto Tmp = *this; Node = Node->GetPrev(); return Tmp; }

			bool operator==(const FIterator& Rhs) const { return Node == Rhs.Node; }
			bool operator!=(const FIterator& Rhs) const { return Node != Rhs.Node; }

		private:
			pointer Node {nullptr};
		};

		class FReverseIterator
		{
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = NodeType;
			using difference_type = std::ptrdiff_t;
			using pointer = const NodeType*;
			using reference = const NodeType&;

			FReverseIterator() = default;
			explicit FReverseIterator(pointer InNode) : Node(InNode) {}

			reference operator*() const { return *Node; }
			pointer   operator->() const { return Node; }

			FReverseIterator& operator++() { Node = Node->GetPrev(); return *this; }
			FReverseIterator  operator++(int) { auto Tmp = *this; Node = Node->GetPrev(); return Tmp; }

			FReverseIterator& operator--() { Node = Node->GetNext(); return *this; }
			FReverseIterator  operator--(int) { auto Tmp = *this; Node = Node->GetNext(); return Tmp; }

			bool operator==(const FReverseIterator& Rhs) const { return Node == Rhs.Node; }
			bool operator!=(const FReverseIterator& Rhs) const { return Node != Rhs.Node; }

		private:
			pointer Node {nullptr};
		};

		struct FDefaultEraseHandler
		{
			void operator() (const NodeType*) {}
		};

	private:

		struct FLocation
		{
			NodeType* Node {nullptr};
			NodeType* Frontiers[MaxLevel] {};
			int NodeHeight {0};
		};

		struct FPos
		{
			NodeType* Node;
			int Index;
		};

		int ListLevels {0};
		SizeType Size {0};

		NodeType* Head {NodeType::Create(K{}, V{}, MaxLevel)};
		NodeType* Tail {nullptr};

		ValueComparerType ValueComparer;
		KeyComparerType KeyComparer;

		RandFuncT RandFunc;

	public:

		TSkipList(const ValueComparerType& InValueComparer, const KeyComparerType& InKeyComparer,
			const RandFuncT& InRandFunc) :
		ValueComparer(InValueComparer), KeyComparer(InKeyComparer), RandFunc(InRandFunc)
		{
		}

		TSkipList() : TSkipList(ValueComparerType{}, KeyComparerType{}, RandFuncT{}) {}

		explicit TSkipList(const ValueComparerType& InValueComparer) :
			TSkipList(InValueComparer, KeyComparerType{}, RandFuncT{}) {}

		TSkipList(const TSkipList& Other) : TSkipList(Other.ValueComparer, Other.KeyComparer, Other.RandFunc)
		{
			ListLevels = Other.ListLevels;
			Size = Other.Size;

			struct FrontNode
			{
				NodeType* Node;
				SizeType HeadOffset;
			};

			FrontNode InsertFronts[MaxLevel];
			for (int i = 0; i < MaxLevel; ++i)
			{
				InsertFronts[i].Node = Head;
				InsertFronts[i].HeadOffset = 0;
			}

			NodeType* CopyFronts[MaxLevel];
			for (int i = 0; i < MaxLevel; ++i)
			{
				CopyFronts[i] = Other.Head;
			}

			NodeType* Cursor = Other.Head;
			for (int Offset = 1; Offset < Other.Size + 1; ++Offset)
			{
				Cursor = Cursor->GetLink(0)->Next;

				int CursorHeight = 0;
				for (int i = 0; i < MaxLevel; ++i)
				{
					if (CopyFronts[i]->GetLink(i)->Next == Cursor)
					{
						CopyFronts[i] = Cursor;
						++CursorHeight;
					}
					else
					{
						break;
					}
				}

				NodeType* NewNode = NodeType::Create(Cursor->Key, Cursor->Value, CursorHeight);

				if (Offset > 1)
					NewNode->Prev = InsertFronts[0].Node;

				for (int i = 0; i < CursorHeight; ++i)
				{
					NodeLink* PrevLink = InsertFronts[i].Node->GetLink(i);
					PrevLink->Next = NewNode;
					PrevLink->Span = Offset - InsertFronts[i].HeadOffset;

					InsertFronts[i].Node = NewNode;
					InsertFronts[i].HeadOffset = Offset;
				}
			}

			Tail = InsertFronts[0].Node;
			if (Tail == Head)
				Tail = nullptr;
		}

		TSkipList(TSkipList&& Other) noexcept : TSkipList()
		{
			Swap(Other);
		}

		TSkipList& operator=(const TSkipList& Other) // NOLINT(bugprone-unhandled-self-assignment) false positive.
		{
			if (this == &Other)
				return *this;

			TSkipList Tmp(Other);
			Swap(Tmp);
			return *this;
		}

		TSkipList& operator=(TSkipList&& Other) noexcept
		{
			Swap(Other);
			return *this;
		}

		~TSkipList()
		{
			NodeType* Current = Head;
			while (Current)
			{
				NodeType* Next = Current->GetLink(0)->Next;
				NodeType::Free(Current);
				Current = Next;
			}
		}

		void Swap(TSkipList& Other) noexcept
		{
			std::swap(ListLevels, Other.ListLevels);
			std::swap(Size, Other.Size);
			std::swap(Head, Other.Head);
			std::swap(Tail, Other.Tail);

			{
				ValueComparerType Tmp(Other.ValueComparer);
				Other.ValueComparer = ValueComparer;
				ValueComparer = Tmp;
			}

			{
				KeyComparerType Tmp(Other.KeyComparer);
				Other.KeyComparer = KeyComparer;
				KeyComparer = Tmp;
			}

			{
				RandFuncT Tmp(Other.RandFunc);
				Other.RandFunc = RandFunc;
				RandFunc = Tmp;
			}
		}

		SizeType GetSize() const { return Size; }
		bool IsEmpty() const { return Size == 0; }
		const NodeType* GetFirst() const { return Head->GetNext(); }
		const NodeType* GetLast() const { return Tail; }

		using const_iterator = FIterator;
		using const_reverse_iterator = FReverseIterator;

		const_iterator begin()  const { return const_iterator(GetFirst()); }
		const_iterator end()    const { return const_iterator(nullptr); }
		const_iterator cbegin() const { return begin(); }
		const_iterator cend()   const { return end(); }

		const_reverse_iterator rbegin()  const { return const_reverse_iterator(GetLast()); }
		const_reverse_iterator rend()    const { return const_reverse_iterator(nullptr); }
		const_reverse_iterator crbegin() const { return rbegin(); }
		const_reverse_iterator crend()   const { return rend(); }

		void Clear()
		{
			TSkipList EmptyList(ValueComparer, KeyComparer, RandFunc);
			Swap(EmptyList);
		}

		// return [inserted node or existed node, whether insertion is succeeded]
		// Forwards both key and value, so callers may move rvalues into the list
		// (e.g. List.Insert(std::move(Key), std::move(Value))) with zero copies,
		// while lvalues still bind and copy as before.
		template <typename KeyT, typename ValueT>
		std::pair<const NodeType*, bool> Insert(KeyT&& Key, ValueT&& Value)
		{
			NodeType* Frontier[MaxLevel];
			SizeType FrontierRanks[MaxLevel];

			NodeType* Cur = Head;
			SizeType CurRank = 0;

			for (int i = ListLevels - 1; i >= 0; --i)
			{
				while (true)
				{
					NodeType* Next = Cur->GetLink(i)->Next;

					if (Next)
					{
						auto Order = Comparer(Next, Key, Value);
						if (Order == 0)
							return {Next, false};

						if (Order < 0)
						{
							CurRank += Cur->GetLink(i)->Span;
							Cur = Next;
							continue;
						}
					}

					Frontier[i] = Cur;
					FrontierRanks[i] = CurRank;
					break;
				}
			}

			int Level = GetRandomLevel();

			if (Level > ListLevels)
			{
				for (int i = ListLevels; i < Level; ++i)
				{
					Frontier[i] = Head;
					FrontierRanks[i] = 0;
				}
				ListLevels = Level;
			}

			NodeType* NewNode = NodeType::Create(std::forward<KeyT>(Key), std::forward<ValueT>(Value), Level);

			for (int i = 0; i < Level; ++i)
			{
				NodeLink* Link = Frontier[i]->GetLink(i);
				NodeLink* NewLink = NewNode->GetLink(i);

				NewLink->Next = Link->Next;
				SizeType RankOffset = CurRank - FrontierRanks[i];

				if (Link->Next)
					NewLink->Span = Link->Span - RankOffset;
				else
					NewLink->Span = 0;

				Link->Next = NewNode;
				Link->Span = RankOffset + 1;
			}

			for (int i = Level; i < ListLevels; ++i)
			{
				NodeLink* Link = Frontier[i]->GetLink(i);
				if (Link->Next)
					Link->Span += 1;
			}

			NewNode->Prev = Cur == Head ? nullptr : Cur;
			if (NodeType* NextNode = NewNode->GetLink(0)->Next)
				NextNode->Prev = NewNode;
			else
				Tail = NewNode;

			++Size;
			return {NewNode, true};
		}

		bool Erase(const K& Key, const V& Value)
		{
			if (Size == 0)
				return false;

			FLocation Location = FindLocation(Key, Value);
			if (Location.Node == nullptr)
				return false;

			Erase(Location);
			return true;
		}

		template <typename EraseHandler = FDefaultEraseHandler>
		SizeType Erase(const TRange<V>& Range, EraseHandler Handler = FDefaultEraseHandler {})
		{
			if (IsEmpty())
				return 0;

			int Order = ValueComparer(Range.LowerBound.Value, Range.UpperBound.Value);
			if (Order > 0)
				return 0;

			if (Order == 0 && (Range.LowerBound.bExclusive || Range.UpperBound.bExclusive))
				return 0;

			if (OutOfUpperBound(Head->GetLink(0)->Next->Value, Range.UpperBound))
				return 0;

			if (OutOfLowerBound(Tail->Value, Range.LowerBound))
				return 0;

			FPos EraseBegin[MaxLevel] {};
			FPos EraseUntil[MaxLevel] {};

			FPos From { Head, - 1 };
			FPos To { nullptr, -1 };

			for (int i = ListLevels - 1; i >= 0; --i)
			{
				From = FindFrontier(From, i, Range.LowerBound);
				EraseBegin[i] = From;

				To = FindLastInRange(From, i, Range.UpperBound);
				EraseUntil[i] = GetNext(To, i);
			}

			return EraseCommit(EraseBegin, EraseUntil, To.Index - From.Index, Handler);
		}

		template <typename EraseHandler = FDefaultEraseHandler>
		SizeType EraseByRank(SizeType FromRank, SizeType ToRank, EraseHandler Handler = FDefaultEraseHandler {})
		{
			if (IsEmpty())
				return 0;

			if (FromRank > Size - 1 || ToRank < 0 || FromRank > ToRank)
				return 0;

			if (ToRank > Size - 1)
				ToRank = Size - 1;

			if (FromRank < 0)
				FromRank = 0;

			FPos EraseBegin[MaxLevel] {};
			FPos EraseUntil[MaxLevel] {};
			
			FPos From { Head, - 1 };
			FPos To { nullptr, -1 };

			for (int i = ListLevels - 1; i >= 0; --i)
			{
				From = FindFrontier(From, i, FromRank);
				EraseBegin[i] = From;
				
				To = FindLastInRange(From, i, ToRank);
				EraseUntil[i] = GetNext(To, i);
			}

			return EraseCommit(EraseBegin, EraseUntil, To.Index - From.Index, Handler);
		}

		template <typename ValueT>
		const NodeType* Update(const K& Key, const V& CurrentValue, ValueT&& NewValue)
		{
			if (Size == 0)
				return nullptr;

			FLocation Location = FindLocation(Key, CurrentValue);
			if (Location.Node == nullptr)
				return nullptr;

			auto Order = ValueComparer(CurrentValue, NewValue);
			if (Order == 0)
				return Location.Node;

			if (Order < 0)
			{
				NodeType* Next = Location.Node->GetLink(0)->Next;
				if (Next == nullptr || Comparer(Next, Key, NewValue) > 0)
				{
					Location.Node->Value = NewValue;
					return Location.Node;
				}
			}
			else
			{
				NodeType* Prev = Location.Node->Prev;
				if (Prev == nullptr || Comparer(Prev, Key, NewValue) < 0)
				{
					Location.Node->Value = NewValue;
					return Location.Node;
				}
			}

			Erase(Location);
			return Insert(Key, std::forward<ValueT>(NewValue)).first;
		}

		bool ContainsAnyInRange(const TRange<V>& Range) const
		{
			if (IsEmpty())
				return false;

			{
				auto Order = ValueComparer(Range.LowerBound.Value, Range.UpperBound.Value);
				if (Order > 0)
					return false;

				if (Order == 0 && (Range.LowerBound.bExclusive || Range.UpperBound.bExclusive))
					return false;
			}

			if (OutOfUpperBound(Head->GetLink(0)->Next->Value, Range.UpperBound))
				return false;

			if (OutOfLowerBound(Tail->Value, Range.LowerBound))
				return false;

			auto Result = GetFirstInLowerBound(Range.LowerBound);

			if (Result.first)
			{
				return !OutOfUpperBound(Result.first->Value, Range.UpperBound);
			}

			return false;
		}

		SizeType GetCountInRange(const TRange<V>& Range) const
		{
			auto FirstInRange = GetFirstInLowerBound(Range.LowerBound);
			if (FirstInRange.first == nullptr)
				return 0;

			auto LastInRange = GetLastInUpperBound(Range.UpperBound);
			if (LastInRange.first == nullptr)
				return 0;

			return LastInRange.second - FirstInRange.second + 1;
		}

		// Index starts from zero
		const NodeType* At(SizeType Index) const
		{
			if (Index >= Size || Index < 0)
				return nullptr;

			constexpr SizeType NearSearch = 10;

			if (Index < NearSearch)
			{
				NodeType* Node = Head->GetNext();
				for (SizeType i = 0; i < Index; ++i)
				{
					Node = Node->GetNext();
				}
				return Node;
			}

			if (Size - Index < NearSearch)
			{
				NodeType* Node = Tail;
				for (SizeType i = 0; i < Size - Index - 1; ++i)
				{
					Node = Node->Prev;
				}
				return Node;
			}

			NodeType* Cur = Head;
			int Levels = ListLevels - 1;
			SizeType RemainSteps = Index + 1;

			while (true)
			{
				NodeLink* Link = Cur->GetLink(Levels);
				NodeType* Next = Link->Next;

				if (RemainSteps == Link->Span)
					return Next;

				if (RemainSteps > Link->Span && Next != nullptr)
				{
					Cur = Next;
					RemainSteps -= Link->Span;
				}
				else
				{
					--Levels;
				}
			}
		}

		// Index starts from zero
		const NodeType* operator[] (SizeType Index) const
		{
			return At(Index);
		}

		const NodeType* Find(const K& Key, const V& Value) const
		{
			NodeType* Cur = Head;
			for (int i = ListLevels - 1; i >= 0; --i)
			{
				while (true)
				{
					NodeType* Next = Cur->GetLink(i)->Next;
					if (Next)
					{
						auto Order = Comparer(Next, Key, Value);
						if (Order < 0)
						{
							Cur = Next;
							continue;
						}

						if (Order == 0)
							return Next;
					}
					break;
				}
			}
			return nullptr;
		}

		// find first node that satisfies this LowerBound, return this node and it's index (if existed)
		std::pair<const NodeType*, SizeType> GetFirstInLowerBound(const TRangeBound<V>& LowerBound) const
		{
			if (IsEmpty() || OutOfLowerBound(Tail->Value, LowerBound))
				return { nullptr, SizeType(-1) };

			NodeType* Cur = Head;
			SizeType Index = 0;
			int Levels = ListLevels - 1;

			while (true)
			{
				NodeLink* Link = Cur->GetLink(Levels);
				SizeType Span = Link->Span;
				NodeType* Next = Link->Next;

				if (Next && OutOfLowerBound(Next->Value, LowerBound))
				{
					Index += Span;
					Cur = Next;
				}
				else
				{
					if (Span == 1)
						return { Next, Index };

					--Levels;
				}
			}
		}

		// find last node that satisfies this UpperBound, return this node and it's index (if existed)
		std::pair<const NodeType*, SizeType> GetLastInUpperBound(const TRangeBound<V>& UpperBound) const
		{
			if (IsEmpty() || OutOfUpperBound(Head->GetNext()->Value, UpperBound))
				return { nullptr, SizeType(-1) };

			if (!OutOfUpperBound(Tail->Value, UpperBound))
				return { Tail, Size - 1 };

			NodeType* Cur = Head;
			SizeType Index = 0;
			int Levels = ListLevels - 1;

			while (true)
			{
				NodeLink* Link = Cur->GetLink(Levels);
				SizeType Span = Link->Span;
				NodeType* Next = Link->Next;

				if (Next && !OutOfUpperBound(Next->Value, UpperBound))
				{
					Index += Span;
					Cur = Next;
				}
				else
				{
					if (Span == 1)
						return { Cur, Index - 1 };

					--Levels;
				}
			}
		}

		// return the rank by element's key and value. rank is 0-based.
		// if such element is not existed, -1 is returned.
		SizeType GetRank(const K& Key, const V& Value) const
		{
			if (IsEmpty())
				return -1;

			SizeType Rank = -1;
			NodeType* Cur = Head;

			for (int i = ListLevels - 1; i >= 0; --i)
			{
				while (true)
				{
					NodeType* Next = Cur->GetLink(i)->Next;
					if (Next)
					{
						int Order = Comparer(Next, Key, Value);
						if (Order < 0)
						{
							Rank += Cur->GetLink(i)->Span;
							Cur = Next;
							continue;
						}

						if (Order == 0)
						{
							Rank += Cur->GetLink(i)->Span;
							return Rank;
						}
					}
					break;
				}
			}
			return -1;
		}

	private:

		int GetRandomLevel()
		{
			int Level = 1;
			while (RandFunc() && Level < MaxLevel)
			{
				++Level;
			}
			return Level;
		}

		int Comparer(const NodeType* Node, const K& Key, const V& Value) const
		{
			auto ValueOrder = ValueComparer(Node->Value, Value);
			if (ValueOrder < 0)
				return -1;

			if (ValueOrder > 0)
				return 1;

			if (KeyComparer(Node->Key, Key))
				return -1;

			if (KeyComparer(Key, Node->Key))
				return 1;

			return 0;
		}

		FLocation FindLocation(const K& Key, const V& Value) const
		{
			FLocation Location;
			Location.Node = nullptr;

			NodeType* Cur = Head;
			for (int i = ListLevels - 1; i >= 0; --i)
			{
				while (true)
				{
					NodeType* Next = Cur->GetLink(i)->Next;
					if (Next)
					{
						auto Order = Comparer(Next, Key, Value);
						if (Order < 0)
						{
							Cur = Next;
							continue;
						}

						if (Order == 0 && Location.Node == nullptr)
						{
							Location.Node = Next;
							Location.NodeHeight = i + 1;
						}
					}
					Location.Frontiers[i] = Cur;
					break;
				}
			}
			return Location;
		}

		void Erase(const FLocation& Location)
		{
			if (Location.Node == nullptr)
				return;

			for (int i = 0; i < ListLevels; ++i)
			{
				NodeType* Prev = Location.Frontiers[i];
				NodeLink* PrevLink = Prev->GetLink(i);

				if (i < Location.NodeHeight)
				{
					NodeLink* TargetLink = Location.Node->GetLink(i);

					NodeType* Next = TargetLink->Next;
					PrevLink->Next = Next;
					PrevLink->Span = Next? PrevLink->Span + TargetLink->Span - 1 : 0;
				}
				else
				{
					if (PrevLink->Next)
						PrevLink->Span -= 1;
				}
			}

			if (Location.NodeHeight == ListLevels)
			{
				int TotalLevels = ListLevels;
				for (int i = ListLevels - 1; i >= 0; --i)
				{
					if (Head->GetLink(i)->Next == nullptr)
					{
						--TotalLevels;
					}
					else
					{
						break;
					}
				}
				ListLevels = TotalLevels;
			}

			if (NodeType* Next = Location.Node->GetLink(0)->Next)
			{
				Next->Prev = Location.Node->Prev;
			}

			if (Location.Node == Tail)
			{
				Tail = Location.Node->Prev;
			}

			--Size;

			NodeType::Free(Location.Node);
		}

		bool OutOfLowerBound(const V& Value, const TRangeBound<V>& Bound) const
		{
			auto Order = ValueComparer(Value, Bound.Value);
			return Bound.bExclusive? Order <= 0 : Order < 0;
		}

		bool OutOfUpperBound(const V& Value, const TRangeBound<V>& Bound) const
		{
			auto Order = ValueComparer(Value, Bound.Value);
			return Bound.bExclusive? Order >= 0 : Order > 0;
		}

		bool InRange(const V& Value, const TRange<V>& Range) const
		{
			return !OutOfLowerBound(Value, Range.LowerBound) && !OutOfUpperBound(Value, Range.UpperBound);
		}

		FPos GetNext(const FPos& Pos, int Level) const
		{
			NodeLink* Link = Pos.Node->GetLink(Level);
			return { Link->Next,  Pos.Index + Link->Span };
		}

		FPos FindFrontier(const FPos& Pos, int Level, const TRangeBound<V>& LowerBound) const
		{
			FPos Cur = Pos;
			while (true)
			{
				FPos Next = GetNext(Cur, Level);
				if (Next.Node && OutOfLowerBound(Next.Node->Value, LowerBound))
				{
					Cur = Next;
				}
				else
				{
					return Cur;
				}
			}
		}
		
		FPos FindFrontier(const FPos& Pos, int Level, int Rank) const
		{
			FPos Cur = Pos;
			while (true)
			{
				FPos Next = GetNext(Cur, Level);
				if (Next.Node && Next.Index < Rank)
				{
					Cur = Next;
				}
				else
				{
					return Cur;
				}
			}
		}		
		
		FPos FindLastInRange(const FPos& Pos, int Level, const TRangeBound<V>& UpperBound) const
		{
			FPos Cur = Pos;
			while (true)
			{
				FPos Next = GetNext(Cur, Level);
				if (Next.Node && !OutOfUpperBound(Next.Node->Value, UpperBound))
				{
					Cur = Next;
				}
				else
				{
					return Cur;
				}
			}
		}
		
		FPos FindLastInRange(const FPos& Pos, int Level, int Rank) const
		{
			FPos Cur = Pos;
			while (true)
			{
				FPos Next = GetNext(Cur, Level);
				if (Next.Node && Next.Index <= Rank)
				{
					Cur = Next;
				}
				else
				{
					return Cur;
				}
			}
		}
		
		template <typename EraseHandler>
		SizeType EraseCommit(FPos (&EraseBegin)[MaxLevel], FPos (&EraseUntil)[MaxLevel],
		                     SizeType Removed, EraseHandler& Handler)
		{
			NodeType* ToErase = EraseBegin[0].Node->GetNext();
			NodeType* EraseStop = EraseUntil[0].Node;
			while (ToErase != EraseStop)
			{
				NodeType* Node = ToErase;
				ToErase = ToErase->GetNext();
				Handler(Node);
				NodeType::Free(Node);
			}

			for (int i = 0; i < ListLevels; ++i)
			{
				NodeLink* Link = EraseBegin[i].Node->GetLink(i);
				if (Link->Next)
				{
					Link->Next = EraseUntil[i].Node;

					if (Link->Next)
						Link->Span = EraseUntil[i].Index - EraseBegin[i].Index - Removed;
					else
						Link->Span = 0;
				}
			}

			if (EraseUntil[0].Node)
				EraseUntil[0].Node->Prev = EraseBegin[0].Node == Head ? nullptr : EraseBegin[0].Node;

			if (EraseUntil[0].Node == nullptr)
				Tail = EraseBegin[0].Node;

			int Height = ListLevels;
			for (int i = ListLevels - 1; i >= 0; --i)
			{
				if (EraseBegin[i].Node == Head && EraseUntil[i].Node == nullptr)
					--Height;
				else
					break;
			}
			ListLevels = Height;
			Size -= Removed;

			return Removed;
		}
	};
}
