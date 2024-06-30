#pragma once

namespace TK
{
	template<typename T>
	inline constexpr bool IsTriviallyPackable = std::is_arithmetic_v<T>;

	template<typename T, typename...ArgTypes>
	void Streaming(T& Stream, ArgTypes&&... Args)
	{
		((Stream & Args), ...);
	}

	#define TK_SERIAL(...) \
	template<typename T>\
	void Serialize(T& Stream)\
	{\
		Streaming(Stream, __VA_ARGS__);\
	}
}
