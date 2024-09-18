#include "MemoryReader.h"
#include <cstring>
#include <limits>

namespace TK
{
	bool FMemReader::PopImpl(void* Dest, SizeType Length)
	{
		if (Length > GetUnreadBytes() || Length < 0)
			return false;

		if constexpr (sizeof(uint64_t) > sizeof(std::size_t))
		{
			if (static_cast<uint64_t>(Length) > std::numeric_limits<std::size_t>::max())
				return false;
		}
		
		std::memcpy(Dest, static_cast<const char*>(Buffer) + Offset, static_cast<std::size_t>(Length));
		Offset += Length;
		return true;
	}
}
