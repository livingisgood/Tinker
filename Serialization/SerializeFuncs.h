#pragma once
#include <string_view>
#include "OutStream.h"
#include "InStream.h"
#include "SerializeDefines.h"
#include "TinkerAssert.h"

namespace TK
{
	template <typename OutStreamType, typename T>
	void ToStream(OutStreamType& Stream, const T& Data)
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
	void FromStream(InStreamType& Stream, T& Data)
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

	template<>
	inline void ToStream(FOutStream& Stream, const bool& Data)
	{
		Stream.PushAs<std::uint8_t>(Data);
	}

	template<>
	inline void FromStream(FInStream& Stream, bool& Data)
	{
		Stream.PopAs<std::uint8_t>(Data);
	}

	template<typename T>
	FOutStream& operator& (FOutStream& Stream, const T& Data)
	{
		ToStream(Stream, Data);
		return Stream;
	}

	template<typename T>
	FInStream& operator& (FInStream& Stream, T& Data)
	{
		FromStream(Stream, Data);
		return Stream;
	}
}
