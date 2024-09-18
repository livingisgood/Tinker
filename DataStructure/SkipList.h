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

	template<typename T>
	struct FSkipListDefaultComparer
	{
		int operator()(const T& A, const T& B) const // in c++20 we could use operator <=>
		{
			std::less<T> Comparer {};
			if (Comparer(A, B))
				return -1;

			return Comparer(B, A)? 1 : 0;
		}
	};
	
	template<typename T>
	struct TSkipNode
	{
		using SizeType = std::uint32_t;
		
		struct FLink
		{
			TSkipNode* Next {nullptr};
			SizeType Span {0};
		};

		T UserData;
		TSkipNode* Prev {nullptr};
		FLink Links[1] {};  // zero length array is not allowed in c++

		explicit TSkipNode(const T& Data) : UserData(Data) {}

		FLink* GetLink(int Level)
		{
			return reinterpret_cast<FLink*>(Links) + Level;
		}
		
		static TSkipNode* Create(const T& Data, int Levels)
		{
			void* Memory = std::malloc(sizeof(TSkipNode) + (Levels - 1) * sizeof(FLink));
			TSkipNode* Node = new (Memory)TSkipNode(Data);
			return Node;
		}

		static void Free(TSkipNode* Node)
		{
			Node->~TSkipNode();
			std::free(Node);
		}
	};
	
	template<
		typename T,
		typename ComparerType = FSkipListDefaultComparer<T>,
		typename RandFuncT = FSkipListLevelRand,
		int MaxLevel = 32>
	class TSkipList
	{
		static_assert(MaxLevel >= 1);

	public:
		
		using NodeType = TSkipNode<T>;
		using NodeLink = typename TSkipNode<T>::FLink;
		using SizeType = typename TSkipNode<T>::SizeType;

		TSkipList(const ComparerType& InComparer, const RandFuncT& InRandFuc) :
			Comparer(InComparer), RandFunc(InRandFuc)
		{
			for (int i = 0; i < MaxLevel; ++i)
			{
				Head->GetLink(i)->Next = nullptr;
				Head->GetLink(i)->Span = 0;
			}
		}

		TSkipList() : TSkipList(ComparerType(), RandFuncT()) {}
		explicit TSkipList(const ComparerType& InComparer) : TSkipList(InComparer, RandFuncT()) {}
		
		TSkipList(const TSkipList& Other) : TSkipList(Other.Comparer, Other.RandFunc)
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

		TSkipList(TSkipList&& Other) noexcept: TSkipList()
		{
			Swap(Other);
		}

		TSkipList& operator= (const TSkipList& Other)  // NOLINT(bugprone-unhandled-self-assignment) false positive.
		{
			if (this == &Other)
				return *this;
			
			TSkipList Tmp(Other);
			Swap(Tmp);
			return *this;
		}

		TSkipList& operator= (TSkipList&& Other) noexcept
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

			ComparerType Tmp(Other.Comparer);
			Other.Comparer = Comparer;
			Comparer = Tmp;

			RandFuncT TmpRands(Other.RandFunc);
			Other.RandFunc = RandFunc;
			RandFunc = TmpRands;
		}

		// first member of the return pair indicates whether insertion is succeeded.
		// second member is the node been inserted or is already existed.
		std::pair<bool, NodeType*> Insert(const T& Data)
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
						auto Order = Comparer(Next->UserData, Data);
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
				for(int i = ListLevels; i < Level; ++i)
				{
					Frontier[i] = Head;
					FrontierRanks[i] = 0;
				}
				ListLevels = Level;
			}

			NodeType* NewNode = NodeType::Create(Data, Level);
			
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

			NewNode->Prev = Cur == Head? nullptr : Cur;
			if (NodeType* NextNode = Cur->GetLink(0)->Next)
				NextNode->Prev = NewNode;
			else
				Tail = NewNode;

			++Size;
			return {true, NewNode};
		}

		bool Erase(const T& Data)
		{
			NodeType* Frontiers[MaxLevel];

			NodeType* Cur = Head;
			for(int i = ListLevels - 1; i >= 0; --i)
			{
				while (true)
				{
						
				}
			}
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

		int ListLevels {0};
		SizeType Size {0};
		
		NodeType* Head {NodeType::Create(T{}, MaxLevel)};
		NodeType* Tail {nullptr};
		ComparerType Comparer;
		RandFuncT RandFunc;
	};
}