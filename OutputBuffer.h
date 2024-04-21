#pragma once

namespace TK
{
	template<int N = 0>
	class FOutputBuffer
	{
		static_assert(N >= 0);
		using FByte = char;
		
	public:

		bool Push(const void* src, int len)
		{
			if(len <= 0)
				return true;

			int Required = Size + len;
			if(Required < Size)
				return false;

			if(Capacity < Required)
			{
				const int Suggested = Size + Size / 2;
				Capacity = Suggested > Required? Suggested : Required;

				if(Heap)
				{
					//Heap = static_cast<FByte*>(std::realloc(Heap, Capacity));
				}
			}
		}

	private:

		int Size {0};
		int Capacity {N};

		FByte InlinedBuffer[N];
		FByte* Heap {nullptr};
	};
}
