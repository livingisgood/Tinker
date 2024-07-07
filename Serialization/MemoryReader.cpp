#include "MemoryReader.h"
#include <cstring>

namespace TK
{
	bool FMemReader::PopImpl(void* Dest, SizeType Length)
	{
		if (Length > GetUnreadBytes())
			return false;

		Offset += Length;
		std::memcpy(Dest, Buffer, Length);
		return true;
	}
}
