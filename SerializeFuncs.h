#pragma once
#include "OutputBuffer.h"
#include <type_traits>

namespace TK
{
	template<typename T, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, T Data)
	{
		Buffer.Push(&Data, sizeof(Data));
		return Buffer;
	}

	template<typename T, std::enable_if_t<!std::is_arithmetic_v<T>>* = nullptr>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, const T& Data)
	{
		Data.Serialize(Buffer);
		return Buffer;
	}

	inline FOutputBuffer& operator& (FOutputBuffer& Buffer, bool Data)
	{
		uint8_t Rep = Data? 1 : 0;
		return Buffer & Rep;
	}
}
