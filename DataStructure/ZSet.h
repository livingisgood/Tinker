// ReSharper disable CppClangTidyCppcoreguidelinesMissingStdForward false positive
#pragma once
#include <optional>
#include <unordered_map>
#include "SkipList.h"
#include "TinkerAssert.h"

namespace TK
{
	template<
		typename K,
		typename V,
		typename ValueComparerType = TSkipListDefaultComparer<V>,
		typename KeyComparerType = std::less<K>,
		typename RandFuncT = FSkipListLevelRand,
		int MaxLevel = 32>
	class TZSet
	{
	public:

		using ListType = TSkipList<K, V, ValueComparerType, KeyComparerType, RandFuncT, MaxLevel>;
		using NodeType = typename ListType::NodeType;
		using SizeType = typename ListType::SizeType;
		using MapType = std::unordered_map<K, NodeType const*>;

		using const_iterator = typename ListType::FIterator;
		using const_reverse_iterator = typename ListType::FReverseIterator;
		
	private:
		
		struct FEraser
		{
			FEraser(TZSet* InSet) : Set(InSet) {}
			TZSet* Set; 
			
			void operator() (const NodeType* Node) const
			{
				Set->K2V.erase(Node->Key);
			}
		};
		
	public:
		
		TZSet() = default;
		~TZSet() = default;

		explicit TZSet(const ValueComparerType& InValueComparer)
			: SortedVList(InValueComparer) {}

		TZSet(const ValueComparerType& InValueComparer, const KeyComparerType& InKeyComparer,
			const RandFuncT& InRandFunc)
			: SortedVList(InValueComparer, InKeyComparer, InRandFunc) {}

		TZSet(const TZSet& Other) : SortedVList(Other.SortedVList)
		{
			K2V.reserve(Other.K2V.bucket_count());
			for (const auto& Node : SortedVList)
			{
				K2V.emplace(Node.Key, &Node);
			}
		}

		TZSet(TZSet&& Other) noexcept : TZSet()
		{
			Swap(Other);
		}

		TZSet& operator= (const TZSet& Other)
		{
			if (this == &Other)
				return *this;

			TZSet Tmp(Other);
			Swap(Tmp);
			return *this;
		}

		TZSet& operator= (TZSet&& Other) noexcept
		{
			Swap(Other);
			return *this;
		}

		void Swap(TZSet& Other) noexcept
		{
			K2V.swap(Other.K2V);
			SortedVList.Swap(Other.SortedVList);
		}

		friend void swap(TZSet& A, TZSet& B) noexcept
		{
			A.Swap(B);
		}

		SizeType GetSize() const { return SortedVList.GetSize(); }
		bool IsEmpty() const { return GetSize() == 0; }
		const NodeType* GetFirst() const { return SortedVList.GetFirst(); }
		const NodeType* GetLast() const { return SortedVList.GetLast(); }

		const_iterator begin()  const { return const_iterator(GetFirst()); }
		const_iterator end()    const { return const_iterator(nullptr); }
		const_iterator cbegin() const { return begin(); }
		const_iterator cend()   const { return end(); }

		const_reverse_iterator rbegin()  const { return const_reverse_iterator(GetLast()); }
		const_reverse_iterator rend()    const { return const_reverse_iterator(nullptr); }
		const_reverse_iterator crbegin() const { return rbegin(); }
		const_reverse_iterator crend()   const { return rend(); }

		const NodeType* FindByKey(const K& Key) const { auto it = K2V.find(Key); return it == K2V.end()? nullptr : it->second; }
		bool Contains(const K& Key) const { return K2V.find(Key) != K2V.end(); }

		const NodeType* FindByRank(SizeType Index) const { return SortedVList.At(Index); }
		
		// return -1 if Key is not existed.
		SizeType GetRank(const K& Key) const
		{
			auto It = K2V.find(Key);
			if (It == K2V.end())
				return -1;
			
			return SortedVList.GetRank(Key, It->second->Value);
		}

		std::pair<const NodeType*, SizeType> GetFirstInLowerBound(const TRangeBound<V>& LowerBound) const
		{
			return SortedVList.GetFirstInLowerBound(LowerBound);
		}

		std::pair<const NodeType*, SizeType> GetLastInUpperBound(const TRangeBound<V>& UpperBound) const
		{
			return SortedVList.GetLastInUpperBound(UpperBound);
		}

		bool ContainsAnyInRange(const TRange<V>& ValueRange) const { return SortedVList.ContainsAnyInRange(ValueRange); }
		SizeType GetCountInRange(const TRange<V>& ValueRange) const { return SortedVList.GetCountInRange(ValueRange); }

		// return [node pointer, whether a new insertion occurred]
		// - {newNode, true}  : Key was not found, a new node was inserted
		// - {existingNode, false} : Key already existed, value unchanged, existing node returned
		template <typename KeyT, typename ValueT>
		std::pair<const NodeType*, bool> Insert(KeyT&& Key, ValueT&& Value)
		{
			auto [It, Inserted] = K2V.try_emplace(std::forward<KeyT>(Key), nullptr);

			if (!Inserted)
				return { It->second, false };

			auto Result = SortedVList.Insert(It->first, std::forward<ValueT>(Value));
			TK_ASSERT(Result.second);
			It->second = Result.first;
			return Result;
		}

		template <typename ValueT>
		const NodeType* Update(const K& Key, ValueT&& NewValue)
		{
			auto It = K2V.find(Key);
			if (It == K2V.end())
				return nullptr;

			It->second = SortedVList.Update(Key, It->second->Value, std::forward<ValueT>(NewValue));
			TK_ASSERT(It->second);
			return It->second;
		}

		// return [node pointer, whether a new insertion occurred]
		// - {newNode, true}  : Key was not found, a new node was inserted
		// - {existingNode, false} : Key already existed, its value was updated in-place
		template <typename KeyT, typename ValueT>
		std::pair<const NodeType*, bool> InsertOrUpdate(KeyT&& Key, ValueT&& Value)
		{
			auto [It, Inserted] = K2V.try_emplace(std::forward<KeyT>(Key), nullptr);

			if (Inserted)
			{
				auto Result = SortedVList.Insert(It->first, std::forward<ValueT>(Value));
				TK_ASSERT(Result.second);
				It->second = Result.first;
				return Result;
			}

			It->second = SortedVList.Update(It->first, It->second->Value, std::forward<ValueT>(Value));
			TK_ASSERT(It->second);
			return { It->second, false };
		}

		bool Erase(const K& Key)
		{
			auto It = K2V.find(Key);
			if (It == K2V.end())
				return false;

			bool Result = SortedVList.Erase(Key, It->second->Value);
			TK_ASSERT(Result);

			K2V.erase(It);
			return Result;
		}

		SizeType EraseByRange(const TRange<V>& Range)
		{
			return SortedVList.Erase(Range, FEraser(this));
		}

		SizeType EraseByRank(int FromRank, int ToRank)
		{
			return SortedVList.EraseByRank(FromRank, ToRank, FEraser(this));
		}
		
		SizeType EraseByRank(int Rank)
		{
			return SortedVList.EraseByRank(Rank, Rank, FEraser(this));
		}
		
		std::optional<std::pair<K, V>> PopFirst()
		{
			const NodeType* First = GetFirst();
			if (!First)
				return std::nullopt;
			
			std::pair<K, V> Result { First->Key, First->Value };
			SortedVList.EraseByRank(0, FEraser(this));
			
			return Result;
		}
		
		std::optional<std::pair<K, V>> PopLast()
		{
			const NodeType* Last = GetLast();
			if (!Last)
				return std::nullopt;
			
			std::pair<K, V> Result { Last->Key, Last->Value };
			SortedVList.EraseByRank(GetSize() - 1, FEraser(this));
			
			return Result;
		}
		
		void Clear()
		{
			K2V.clear();
			SortedVList.Clear();
		}

	private:

		MapType K2V;
		ListType SortedVList;
	};
}
