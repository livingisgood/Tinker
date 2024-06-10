#pragma once
#include <type_traits>

namespace TK
{
	class FOutputBuffer
	{
	public:

		using SizeType = std::int64_t;
		
		FOutputBuffer() = default;
		virtual ~FOutputBuffer() = default;

		FOutputBuffer(const FOutputBuffer&) = default;
		FOutputBuffer(FOutputBuffer&&) = default;

		FOutputBuffer& operator=(const FOutputBuffer&) = default;
		FOutputBuffer& operator=(FOutputBuffer&&) = default;
		
		void Push(const void* source, SizeType length)
		{
			if(bValid)
				bValid = PushImpl(source, length);
		}

		template<typename T, typename U, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
		void PushAs(U Data)
		{
			T AsInt = static_cast<T>(Data);
			Push(&AsInt, sizeof(T));
		}
		
		bool IsValid() const { return bValid; }

	protected:

		virtual bool PushImpl(const void* source, SizeType length) = 0;
		
		bool bValid {true};
	};
}
