#pragma once
#include <cstdint>
#include <type_traits>

namespace TK
{
	class FInputBuffer
	{
	public:

		using SizeType = std::int64_t;

		virtual ~FInputBuffer() = default;

		FInputBuffer(const FInputBuffer&) = default;
		FInputBuffer(FInputBuffer&&) = default;

		FInputBuffer& operator=(const FInputBuffer&) = default;
		FInputBuffer& operator=(FInputBuffer&&) = default;

		void Pop(const void* Dest, SizeType Length)
		{
			if(bValidInput)
				bValidInput = PopImpl(Dest, Length);
		}

		template<typename T, typename U, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
		void PopAs(U& Data)
		{
			T Value;
			Pop(&Value, sizeof(T));

			if(bValidInput)
				Data = static_cast<U>(Value);
		}

		bool IsValidInput() const { return bValidInput; }
	
	protected:

		virtual bool PopImpl(const void* Dest, SizeType Length) = 0;

		bool bValidInput {true};
	};
}
