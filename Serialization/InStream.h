#pragma once
#include <cstdint>
#include <type_traits>	// ReSharper disable once CppUnusedIncludeDirective, false positive

namespace TK
{
	class FInStream
	{
	public:
		using SizeType = std::int64_t;

		FInStream() = default;
		virtual ~FInStream() = default;

		FInStream(const FInStream&) = default;
		FInStream(FInStream&&) = default;

		FInStream& operator=(const FInStream&) = default;
		FInStream& operator=(FInStream&&) = default;

		void Pop(void* Dest, SizeType Length)
		{
			if (bValidInput)
				bValidInput = PopImpl(Dest, Length);
		}

		template <typename T, typename U, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
		void PopAs(U& Data)
		{
			T Value;
			Pop(&Value, sizeof(T));

			if (bValidInput)
				Data = static_cast<U>(Value);
		}

		template <typename T>
		T Pop()
		{
			T Value{};

			if (bValidInput)
				*this & Value;

			return Value;
		}

		bool IsValidInput() const { return bValidInput; }

		void MarkInputInvalid() { bValidInput = false; }
		
		virtual bool EnsureEnoughBytes(SizeType BytesNum) const = 0;
	
	protected:
		
		virtual bool PopImpl(void* Dest, SizeType Length) = 0;

		bool bValidInput{true};
	};

#define TK_CHECK_INPUT(Stream) if (!(Stream).IsValidInput()) return;
}
