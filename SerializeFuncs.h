#pragma once
#include <map>
#include <vector>
#include "OutputBuffer.h"
#include "InputBuffer.h"

namespace TK
{
	template<typename T>
	inline constexpr bool IsTriviallyPackable = std::is_arithmetic_v<T>;
	
	template<typename T, std::enable_if_t<IsTriviallyPackable<T>>* = nullptr>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, T Data)
	{
		Buffer.Push(&Data, sizeof(Data));
		return Buffer;
	}

	template<typename T, std::enable_if_t<IsTriviallyPackable<T>>* = nullptr>
	FInputBuffer& operator& (FInputBuffer& Buffer, T& Data)
	{
		Buffer.Pop(&Data, sizeof(Data));
		return Buffer;
	}

	template<typename T, std::enable_if_t<!IsTriviallyPackable<T>>* = nullptr>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, const T& Data)
	{
		Data.Serialize(Buffer);
		return Buffer;
	}

	template<typename T, std::enable_if_t<!IsTriviallyPackable<T>>* = nullptr>
	FInputBuffer& operator& (FInputBuffer& Buffer, T& Data)
	{
		Data.Serialize(Buffer);
		return Buffer;
	}
	
	inline FOutputBuffer& operator& (FOutputBuffer& Buffer, bool Data)
	{
		uint8_t Rep = Data? 1 : 0;
		return Buffer & Rep;
	}

	inline FInputBuffer& operator& (FInputBuffer& Buffer, bool& Data)
	{
		uint8_t Rep = 0;
		Buffer & Rep;

		if(Buffer.IsValidInput())
			Data = Rep != 0;

		return Buffer;	
	}

	template<typename T, int N>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, const T (&Data)[N])
	{
		if constexpr (IsTriviallyPackable<T>)
		{
			Buffer.Push(&Data, N * sizeof(T));
		}
		else
		{
			for (int i = 0; i < N; ++i)
			{
				Data[i].Serialize(Buffer);
			}
		}
		return Buffer;
	}

	template<typename T>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, const std::vector<T>& Data)
	{
		Buffer.PushAs<std::uint32_t>(Data.size());

		if constexpr (IsTriviallyPackable<T>)
		{
			Buffer.Push(Data.data(), Data.size() * sizeof(T));
		}
		else
		{
			for(const auto& Entry : Data)
			{
				Entry.Serialize(Buffer);
			}
		}
		return Buffer;
	}

	template<typename KeyType, typename ValueType>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, const std::map<KeyType, ValueType>& Data)
	{
		Buffer.PushAs<std::uint32_t>(Data.size());

		for(const auto& Pair : Data)
		{
			Pair.first.Serialize(Buffer);
			Pair.second.Serialize(Buffer);
		}
		return Buffer;
	}
}
