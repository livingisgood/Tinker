#pragma once
#include <cstdint>
#include <type_traits>

namespace TK
{
	class FOutputBuffer
	{
	public:

		using SizeType = std::int64_t;
		
		virtual ~FOutputBuffer() = default;

		FOutputBuffer(const FOutputBuffer&) = default;
		FOutputBuffer(FOutputBuffer&&) = default;

		FOutputBuffer& operator=(const FOutputBuffer&) = default;
		FOutputBuffer& operator=(FOutputBuffer&&) = default;
		
		void Push(const void* Source, SizeType Length)
		{
			if(bValidOutput)
				bValidOutput = PushImpl(Source, Length);
		}

		template<typename T, typename U, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
		void PushAs(U Data)
		{
			T AsInt = static_cast<T>(Data);
			Push(&AsInt, sizeof(T));
		}
		
		bool IsValidOutput() const { return bValidOutput; }

	protected:

		virtual bool PushImpl(const void* Source, SizeType Length) = 0;
		
		bool bValidOutput {true};
	};
}
