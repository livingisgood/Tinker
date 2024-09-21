#pragma once
#include <cstdint>
#include <random>

namespace TK
{
	struct FSkipListLevelRand
	{
		bool operator()() const
		{
			static std::mt19937 Engine(std::random_device{}());
			std::uniform_int_distribution<int> Dist(0, 3);
			return Dist(Engine) == 0;
		}
	};
	
	template <typename T>
	struct TSkipListDefaultComparer
	{
		int operator()(const T& A, const T& B) const // in c++20 we could use operator <=>
		{
			std::less<T> Comparer{};
			if (Comparer(A, B))
				return -1;

			return Comparer(B, A) ? 1 : 0;
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

		explicit TSkipNode(const K& InKey, const V& InValue) : Key(InKey), Value(InValue) {}

		FLink* GetLink(int Level)
		{
			return reinterpret_cast<FLink*>(Links) + Level;
		}
		
		static TSkipNode* Create(const K& InKey, const V& InValue, int Levels)
		{
			void* Memory = std::malloc(sizeof(TSkipNode) + (Levels - 1) * sizeof(FLink));
			TSkipNode* Node = new(Memory)TSkipNode(InKey, InValue);
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

	private:

		struct FLocation
		{
			NodeType* Node {nullptr};
			NodeType* Frontiers[MaxLevel];
			int NodeHeight {0};
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
			for (int i = 0; i < MaxLevel; ++i)
			{
				typename TSkipNode<K, V>::FLink Link = Head->GetLink(i);
				Link->Next = nullptr;
				Link.Span = 0;
			}
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

			FrontNode Frontier[MaxLevel];
			for (int i = 0; i < MaxLevel; ++i)
			{
				Frontier[i]->Node = Head;
				Frontier[i]->HeadOffset = 0;
			}

			NodeType* CursorFronts[MaxLevel];
			for (int i = 0; i < MaxLevel; ++i)
			{
				CursorFronts[i] = Other.Head;
			}

			NodeType* Cursor = Other.Head;
			for (int Offset = 1; Offset < Other.Size + 1; ++Offset)
			{
				Cursor = Cursor->GetLink(0)->Next;

				int CursorHeight = 0;
				for (int i = 0; i < MaxLevel; ++i)
				{
					if (CursorFronts[i]->GetLink(i)->Next == Cursor)
					{
						CursorFronts[i] = Cursor;
						++CursorHeight;
					}
					else
					{
						break;
					}
				}

				NodeType* NewNode = NodeType::Create(Cursor->UserData, CursorHeight);
				NewNode->Prev = Frontier[0]->Node;

				for (int i = 0; i < CursorHeight; ++i)
				{
					NodeLink* PrevLink = Frontier[i]->Node->GetLink(i);
					PrevLink->Next = NewNode;
					PrevLink->Span = Offset - Frontier[i]->HeadOffset;

					Frontier[i]->Node = NewNode;
					Frontier[i]->HeadOffset = Offset;
				}
			}

			Tail = Frontier[0].Node;
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
		
		// return [whether insertion is succeeded, inserted node or existed node]
		// caller should guarantee the key is not already existed.
		std::pair<bool, NodeType*> Insert(const K& Key, const V& Value)
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
							return {false, Next};

						if (Order < 0)
						{
							Cur = Next;
							CurRank += Cur->GetLink(i)->Span;
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

			NodeType* NewNode = NodeType::Create(Key, Value, Level);

			for (int i = 0; i < Level; ++i)
			{
				NodeLink* Link = Frontier[i]->GetLink(i);
				NodeLink* NewLink = NewNode->GetLink(i);

				NewLink->Next = Link->Next;
				SizeType RankOffset = CurRank - FrontierRanks[i];
				NewLink->Span = Link->Span - RankOffset;

				Link->Next = NewNode;
				Link->Span = RankOffset + 1;
			}

			for (int i = Level; i < ListLevels; ++i)
			{
				Frontier[i]->GetLink(i)->Span += 1;
			}

			NewNode->Prev = Cur == Head ? nullptr : Cur;
			if (NodeType* NextNode = Cur->GetLink(0)->Next)
				NextNode->Prev = NewNode;
			else
				Tail = NewNode;

			++Size;
			return {true, NewNode};
		}

		bool Erase(const K& Key, const V& Value)
		{
			if (Size == 0)
				return false;

			FLocation Location = Find(Key, Value);
			if (Location.Node == nullptr)
				return false;

			Erase(Location);
			return true;
		}

		NodeType* Update(const K& Key, const V& CurrentValue, const V& NewValue)
		{
			if (Size == 0)
				return nullptr;
			
			FLocation Location = Find(Key, CurrentValue);
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
			return Insert(Key, NewValue).second;	
		}

		bool ContainsAnyInRange(const TRange<V>& Range) const
		{
			if (IsEmpty())
				return false;
			
			{
				auto Order = ValueComparer(Range.LowerBound, Range.UpperBound);
				if (Order > 0)
					return false;

				if (Order == 0 && (Range.LowerBound.bExclusive || Range.UpperBound.bExclusive))
					return false;
			}

			if (OutOfLowerBound(Head->GetLink(0)->Next->Value, Range.LowerBound))
				return false;

			if (OutOfUpperBound(Tail->Value, Range.UpperBound))
				return false;

			return true;
		}

		  
		
	private:
		
		int GetRandomLevel()
		{
			int Level = 1;
			if (RandFunc() && Level < MaxLevel)
			{
				++Level;
			}
			return Level;
		}
		
		int Comparer(const NodeType* Node, const K& Key, const V& Value)
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

		FLocation Find(const K& Key, const V& Value)
		{
			FLocation Location;
			Location.Node = nullptr;
			
			NodeType* Cur = Head;
			for (int i = ListLevels - 1; i >= 0; --i)
			{
				if (Location.Node == nullptr)
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
							{
								Location.Node = Next;
								Location.NodeHeight = i + 1;
							}
						}
						Location.Frontiers[i] = Cur;
						break;
					}
				}
				else
				{
					Location.Frontiers[i] = Cur;
				}
			}
			return Location;
		}

		void Erase(const FLocation& Location)
		{
			for (int i = 0; i < ListLevels; ++i)
			{
				NodeType* Prev = Location.Frontiers[i];
				auto PrevLink = Prev->GetLink(i);

				if (i < Location.NodeHeight)
				{
					auto TargetLink = Location.Node->GetLink(i);
					
					NodeType* Next = TargetLink->Next;
					PrevLink->Next = Next;
					PrevLink->Span = PrevLink->Span + TargetLink->Span - 1;
				}
				else
				{
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
	};
}
