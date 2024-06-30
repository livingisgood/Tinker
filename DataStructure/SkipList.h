#pragma once
#include <cstdint>
#include <list>
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
	struct TSkipNode
	{
		using SizeType = std::uint32_t;
		
		struct FLink
		{
			TSkipNode* Prev {nullptr};
			TSkipNode* Next {nullptr};
			SizeType OffsetToNext {0};
		};

		T UserData;
		FLink* Levels; 
		int Height;
		
		TSkipNode(const T& Data, int NodeHeight) : UserData(Data), Levels(new FLink[NodeHeight]), Height(NodeHeight) {}
		~TSkipNode() { delete[] Levels; }

		TSkipNode(const TSkipNode&) = delete;
		TSkipNode(TSkipNode&&) = delete;

		TSkipNode& operator=(const TSkipNode&) = delete;
		TSkipNode& operator=(TSkipNode&&) = delete;

		TSkipNode* GetNext(int Level) const { return Levels[Level].Next; }
		
		void SetNext(int Level, TSkipNode* Node)
		{
			Levels[Level].Next = Node;
			Node->Levels[Level].Prev = Node;
		}
		
		void SetOffsetToNext(int Level, SizeType Offset) { Levels[Level].OffsetToNext = Offset; }
		
		TSkipNode* GetPrev(int Level) const { return Levels[Level].Prev; }
		
		void SetPrev(int Level, TSkipNode* Node)
		{
			Levels[Level].Prev = Node;
			Node->Levels[Level].Next = Node;
		}
	};
	
	template<
		typename T,
		typename ComparerType = std::less<T>,
		typename RandFuncT = FSkipListLevelRand,
		int MaxLevel = 32>
	class TSkipList
	{
		static_assert(MaxLevel >= 1);

	public:
		
		using Node = TSkipNode<T>;
		using NodeLink = typename TSkipNode<T>::FLink;
		using SizeType = typename TSkipNode<T>::SizeType;

		TSkipList(const ComparerType& InComparer, const RandFuncT& InRandFuc) :
			Head(new Node(T{}, MaxLevel)), Tail(new Node(T{}, MaxLevel)), Comparer(InComparer), RandFunc(InRandFuc)
		{
			for (int i = 0; i < MaxLevel; ++i)
			{
				Head->SetNext(i, Tail);
			}
		}

		TSkipList() : TSkipList(ComparerType(), RandFuncT()) {}
		explicit TSkipList(const ComparerType& InComparer) : TSkipList(InComparer, RandFuncT()) {}

		TSkipList(const TSkipList& Other) : TSkipList(Other.Comparer, Other.RandFunc)
		{
			ListLevelNum = Other.ListLevelNum;
			Size = Other.Size;

			struct FrontNodeInfo
			{
				Node* Pos;
				SizeType Index;
			};

			FrontNodeInfo FrontNodes[MaxLevel];
			for (int i = 0; i < MaxLevel; ++i)
			{
				FrontNodes[i].Pos = Head;
				FrontNodes[i].Index = 0;
			}
			
			Node* Cursor = Other.Head;
			for (SizeType NodeIndex = 1; NodeIndex < Other.Size + 1; ++NodeIndex)
			{
				Cursor = Cursor->GetNext(0);
				
				Node* NewNode = new Node(Cursor->UserData, Cursor->Height);
				for (int i = 0; i < Cursor->Height; ++i)
				{
					Node* FrontNode = FrontNodes[i].Pos;
					FrontNode->SetNext(i, NewNode);
					FrontNode->SetOffsetToNext(NodeIndex - FrontNodes[i].Index);
					
					FrontNodes[i].Pos = NewNode;
					FrontNodes[i].Index = NodeIndex;
				}
			}

			for (int i = 0; i < MaxLevel; ++i)
			{
				Node* FrontNode = FrontNodes[i].Pos;
				FrontNode->SetNext(i, Tail);
				FrontNode->SetOffsetToNext(i, Other.Size + 1 - FrontNodes[i].Index);
			}
		}

		void Swap(TSkipList& Other) noexcept
		{
			std::swap(ListLevelNum, Other.ListLevelNum);
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

		void Test()
		{
			std::list<int> myList;
			myList.push_back(3);
		}
	
	private:

		int ListLevelNum {0};
		SizeType Size {0};
		
		Node* Head;
		Node* Tail;
		ComparerType Comparer;
		RandFuncT RandFunc;
	};
}