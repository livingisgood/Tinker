#pragma once

namespace TK
{
	template<typename T>
	inline constexpr bool IsBitwisePackable = std::is_arithmetic_v<T>;

	template<>
	inline constexpr bool IsBitwisePackable<float> = sizeof(float) == 4;

	template<>
	inline constexpr bool IsBitwisePackable<double> = sizeof(double) == 8;

	template<typename, typename StreamType, typename = void>
	inline constexpr bool CanBeSaved = false;

	template<typename T, typename StreamType>
	inline constexpr bool CanBeSaved<T, StreamType,
	std::void_t<decltype(std::declval<T>().Save(std::declval<StreamType&>()))>> = true;

	template<typename, typename StreamType, typename = void>
	inline constexpr bool CanBeLoaded = false;

	template<typename T, typename StreamType>
	inline constexpr bool CanBeLoaded<T, StreamType,
	std::void_t<decltype(std::declval<T>().Load(std::declval<StreamType&>()))>> = true;

	template<typename T, typename...ArgTypes>
	void Streaming(T& Stream, ArgTypes&&... Args)
	{
		(Stream & ... & std::forward<ArgTypes>(Args));
	}

	#define TK_SERIAL(...) \
	template<typename T>\
	void Load(T& Stream)\
	{\
		TK::Streaming(Stream, __VA_ARGS__);\
	}\
	template<typename T>\
	void Save(T& Stream) const\
	{\
		TK::Streaming(Stream, __VA_ARGS__);\
	}
}
