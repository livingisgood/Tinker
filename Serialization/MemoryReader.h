#pragma once
#include "InStream.h"

namespace TK
{
	class FMemReader : public FInStream
	{
	public:
		
		FMemReader() = default;
		FMemReader(const void* Source, SizeType Length):
			Buffer(Source), Size(Length) {}
		
		virtual ~FMemReader() override = default;
		
		FMemReader(const FMemReader&) = default;
		FMemReader(FMemReader&&) = default;

		FMemReader& operator=(const FMemReader&) = default;
		FMemReader& operator=(FMemReader&&) = default;
		
		SizeType GetUnreadBytes() const { return Size - Offset; }
		const void* GetReadPos() const { return Buffer; }

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
