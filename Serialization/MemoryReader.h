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
		
		virtual SizeType GetUnreadBytes() const override { return Size - Offset; }

	protected:
		
		virtual bool PopImpl(const void* Dest, SizeType Length) override;

		const void* Buffer {nullptr};
		SizeType Size {0};
		SizeType Offset {0};
	};
}
