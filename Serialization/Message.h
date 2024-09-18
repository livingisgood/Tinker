#pragma once
#include "Serialization/MemoryWriter.h"
#include <limits>

namespace TK
{
	class FMessage : public TMemWriter<64>
	{
	public:
		
		using FHeaderSizeType = uint16_t;
		constexpr static int MaxSize = 30000;
		
		static_assert(MaxSize <= std::numeric_limits<FHeaderSizeType>::max(),
			"FMessage::MaxSize is too large.");

#pragma pack(1)
		struct FHeader
		{
			FHeaderSizeType Size;
			uint16_t Tag;
		};
#pragma pack()

		explicit FMessage(uint16_t Tag)
		{
			FHeader Header { 0, Tag };
			Push(&Header, sizeof(Header));
		}

		template<typename EnumType>
		explicit FMessage(EnumType Tag) : FMessage(static_cast<uint16_t>(Tag)) {}
		
		constexpr static int GetHeadSize() { return static_cast<int>(sizeof(FHeader)); }

		template<typename ...ArgTypes>
		void Pack(ArgTypes&&... Args)
		{
			using namespace TK;
			((*this) & ... & std::forward<ArgTypes>(Args));
		}

		bool OverFlow() const
		{
			auto Size = GetSize();
			return Size > MaxSize;
		}

		bool UpdateHeader()
		{
			if (OverFlow())
			{
				TK_ASSERT(false);
				return false;
			}

			reinterpret_cast<FHeader*>(GetBuffer())->Size = static_cast<FHeaderSizeType>(GetSize());
			return true;
		}

		void ClearContent()
		{
			ShrinkTo(sizeof(FHeaderSizeType));
		}

		FHeader* GetHeader() { return reinterpret_cast<FHeader*>(GetBuffer()); }
		
		const FHeader* GetHeader() const { return reinterpret_cast<const FHeader*>(GetBuffer()); }

		void SetSizeLimit(int Limit)
		{
			if (0 < Limit && Limit <= MaxSize)
				MaxSizeAllowed = Limit;
		}

		int GetAvailableSpace() const { return MaxSizeAllowed - static_cast<int>(GetSize()); }

	private:

		int MaxSizeAllowed { MaxSize };	
	};
}