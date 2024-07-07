#pragma once
#include <iostream>
#include <string_view>
#include <vector>
#include <map>
#include "OutStream.h"
#include "InStream.h"
#include "MemoryReader.h"
#include "SerializeDefines.h"
#include "TinkerAssert.h"

namespace TK
{
	template <typename OutStreamType, typename T>
	void Save(OutStreamType& Stream, const T& Data)
	{
		if constexpr (IsTriviallyPackable<T>)
		{
			Stream.Push(&Data, sizeof(Data));
		}
		else
		{
			Data.Serialize(Stream);		
		}
	}

	template <typename InStreamType, typename T>
	void Load(InStreamType& Stream, T& Data)
	{
		if constexpr (IsTriviallyPackable<T>)
		{
			Stream.Pop(&Data, sizeof(Data));	
		}
		else
		{
			Data.Serialize(Stream);		
		}
	}
	
	template<typename StreamType, typename T, std::enable_if_t<std::is_base_of_v<FOutStream, StreamType>>* = nullptr>
	StreamType& operator& (StreamType& Stream, const T& Data)
	{
		Save(Stream, Data);
		return Stream;
	}

	template<typename StreamType, typename T, std::enable_if_t<std::is_base_of_v<FInStream, StreamType>>* = nullptr>
	StreamType& operator& (StreamType& Stream, T& Data)
	{
		Load(Stream, Data);
		return Stream;
	}

	template<typename SizeType>
	bool IsSizeOverflow(SizeType Size)
	{
		constexpr auto MaxAllowed = std::numeric_limits<std::uint32_t>::max();
		return static_cast<std::uint64_t>(Size) > static_cast<std::uint64_t>(MaxAllowed);
	}

	inline void Save(FOutStream& Stream, const bool& Data)
	{
		Stream.PushAs<std::uint8_t>(Data);
	}

	inline void Load(FInStream& Stream, bool& Data)
	{
		Stream.PopAs<std::uint8_t>(Data);
	}

	template<typename T, int N>
	void Save(FOutStream& Stream, const T(&Data)[N])
	{
		if constexpr (IsTriviallyPackable<T>)
		{
			Stream.Push(Data, sizeof(T) * N);
		}
		else
		{
			for (int i = 0; i < N; ++i)
			{
				Save(Stream, Data[i]);
			}
		}
	}

	template<typename T, int N>
	void Load(FInStream& Stream, T(&Data)[N])
	{
		if constexpr (IsTriviallyPackable<T>)
		{
			Stream.Pop(Data, sizeof(T) * N);
		}
		else
		{
			for (int i = 0; i < N; ++i)
			{
				Load(Stream, Data[i]);
			}
		}
	}
	
	template<typename T, typename SizeType>
    void Save(FOutStream& Stream, const T* Data, SizeType Num)
    {
		if (Num < 0)
			return;

		if (IsSizeOverflow(Num))
		{
			Stream.MarkOutputInvalid();
			TK_ASSERT(false);
			return;
		}

		Stream.PushAs<std::uint32_t>(Num);
		if constexpr (IsTriviallyPackable<T>)
		{
			Stream.Push(Data, Num * sizeof(T));
		}
		else
		{
			for (int i = 0; i < Num; ++i)
			{
				Save(Stream, Data[i]);
			}
		}
    }

	template<typename CharType>
	void Save(FOutStream& Stream, const std::basic_string_view<CharType>& Data)
	{
		Save<CharType, typename std::basic_string_view<CharType>::size_type>
		(Stream, Data.data(), Data.size());
	}

	template<typename CharType>
	void Load(FInStream& Stream, std::basic_string<CharType>& Data)
	{
		if (!Stream.IsValidInput())
			return;

		std::uint32_t CharsNum = Stream.Pop<std::uint32_t>();
		if (!Stream.EnsureEnoughBytes(CharsNum * sizeof(CharType)))
		{
			Stream.MarkInputInvalid();
			TK_ASSERT(false);
			return;
		}

		Data.resize(CharsNum);
		Stream.Pop(Data.data(), CharsNum * sizeof(CharType));
	}

	template<typename CharType>
	void Load(FMemReader& Stream, std::basic_string<CharType>& Data)
	{
		if (!Stream.IsValidInput())
			return;

		std::uint32_t CharsNum = Stream.Pop<std::uint32_t>();
		if (!Stream.EnsureEnoughBytes(CharsNum * sizeof(CharType)))
		{
			Stream.MarkInputInvalid();
			TK_ASSERT(false);
			return;
		}

		Data.clear();
		Data.append(static_cast<const CharType*>(Stream.GetReadPos()), CharsNum);
		Stream.Advance(CharsNum * sizeof(CharType));
	}

	template<typename T>
	void Save(FOutStream& Stream, const std::vector<T>& Data)
	{
		Save<T, typename std::vector<T>::size_type>(Stream, Data.data(), Data.size());		
	}

	template<typename T>
	void Load(FInStream& Stream, std::vector<T>& Data)
	{
		if (!Stream.IsValidInput())
			return;

		std::uint32_t Num = Stream.Pop<std::uint32_t>();
		Data.clear();

		if constexpr (IsTriviallyPackable<T>)
		{
			if (!Stream.EnsureEnoughBytes(Num * sizeof(T)))
			{
				Stream.MarkInputInvalid();
				TK_ASSERT(false);
				return;
			}

			Data.resize(Num);
			Stream.Pop(Data.data(), Num * sizeof(T));
		}
		else
		{
			for (std::uint32_t i = 0; i < Num; ++i)
			{
				T Item;
				Load(Stream, Item);

				if (Stream.IsValidInput())
				{
					Data.push_back(Item);
				}
				else
				{
					break;
				}
			}
		}
	}

	template<typename T>
	void Load(FMemReader& Stream, std::vector<T>& Data)
	{
		if (!Stream.IsValidInput())
			return;

		std::uint32_t Num = Stream.Pop<std::uint32_t>();
		Data.clear();

		if constexpr (IsTriviallyPackable<T>)
		{
			if (!Stream.EnsureEnoughBytes(Num * sizeof(T)))
			{
				Stream.MarkInputInvalid();
				TK_ASSERT(false);
				return;
			}

			T* Start = static_cast<T*>(Stream.GetReadPos());
			T* End = Start + Num;
			
			Data.insert(Data.end(), Start, End);
			Stream.Advance(Num * sizeof(T));
		}
		else
		{
			for (std::uint32_t i = 0; i < Num; ++i)
			{
				T Item;
				Load(Stream, Item);

				if (Stream.IsValidInput())
				{
					Data.push_back(Item);
				}
				else
				{
					return;
				}
			}
		}
	}

	template<typename K, typename V>
	void Save(FOutStream& Stream, const std::map<K, V>& Data)
	{
		if (IsSizeOverflow(Data.size()))
		{
			Stream.MarkOutputInvalid();
			TK_ASSERT(false);
			return;
		}

		for (const auto& Pair : Data)
		{
			Save(Stream, Pair.first);
			Save(Stream, Pair.second);
		}
	}

	template<typename K, typename V>
	void Load(FInStream& Stream, std::map<K, V>& Data)
	{
		if (!Stream.IsValidInput())
			return;

		Data.clear();

		std::uint32_t Num = Stream.Pop<std::uint32_t>();
		for (std::uint32_t i = 0; i < Num; ++i)
		{
			K Key;
			V Value;

			Load(Stream, Key);
			Load(Stream, Value);

			if (!Stream.IsValidInput())
				return;

			auto [_, bSucceeded] = Data.emplace(Key, Value);
			if (!bSucceeded)
			{
				Stream.MarkInputInvalid();
				return;
			}
		}
	}
}
