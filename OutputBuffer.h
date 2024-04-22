#pragma once
#include <type_traits>

namespace TK
{
	class FOutputBuffer
	{
	public:
		
		FOutputBuffer() = default;
		virtual ~FOutputBuffer() = default;

		FOutputBuffer(const FOutputBuffer&) = default;
		FOutputBuffer(FOutputBuffer&&) = default;

		FOutputBuffer& operator=(const FOutputBuffer&) = default;
		FOutputBuffer& operator=(FOutputBuffer&&) = default;
		
		void Push(const void* source, int length)
		{
			if(bValid)
				bValid = PushImpl(source, length);
		}

		template<typename T, typename U, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
		void PushAs(U Input)
		{
			T Rep = static_cast<T>(Input);
			Push(&Rep, sizeof(Rep));
		}

		bool IsValid() const { return bValid; }

	protected:

		virtual bool PushImpl(const void* source, int length) = 0;
		
		bool bValid {true};
	};

	template<typename T, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
	FOutputBuffer& operator& (FOutputBuffer& Buffer, T Data)
	{
		Buffer.Push(&Data, sizeof(Data));
		return Buffer;
	}

	inline FOutputBuffer& operator& (FOutputBuffer& Buffer, bool Data)
	{
		uint8_t Rep = Data? 1 : 0;
		return Buffer & Rep;
	}
}
