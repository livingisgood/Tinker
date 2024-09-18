#pragma once
#include "Serialization/InStream.h"


namespace TK
{
	class FMemReader : public FInStream
	{
	public:
		
		FMemReader() = default;

		template<typename InSizeType>
		FMemReader(const void* Source, InSizeType Length) : Buffer(Source), Size(static_cast<SizeType>(Length)) {}
		
		virtual ~FMemReader() override = default;
		
		FMemReader(const FMemReader&) = default;
		FMemReader(FMemReader&&) = default;

		FMemReader& operator=(const FMemReader&) = default;
		FMemReader& operator=(FMemReader&&) = default;

		void Init(const void* Source, SizeType Length)
		{
			FMemReader Tmp(Source, Length);
			*this = Tmp;
		}
		
		SizeType GetUnreadBytes() const { return Size - Offset; }
		const void* GetReadPos() const { return static_cast<const char*>(Buffer) + Offset; }

		virtual bool EnsureEnoughBytes(SizeType BytesNum) const override
		{
			return GetUnreadBytes() >= BytesNum;
		}

		void Advance(SizeType Bytes)
		{
			Offset += Bytes;
			if (Offset > Size)
				bValidInput = false;
		}

	protected:
		
		virtual bool PopImpl(void* Dest, SizeType Length) override;

		const void* Buffer {nullptr};
		SizeType Size {0};
		SizeType Offset {0};
	};
}
