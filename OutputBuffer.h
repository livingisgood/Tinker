#pragma once

namespace TK
{
	class FOutputBuffer
	{
	public:
		
		FOutputBuffer() = default;
		virtual ~FOutputBuffer() = default;

		FOutputBuffer(const FOutputBuffer&) = default;
		FOutputBuffer(FOutputBuffer&&) = default;

		FOutputBuffer& operator=(const FOutputBuffer&) = default;
		FOutputBuffer& operator=(FOutputBuffer&&) = default;
		
		void Push(const void* source, int length)
		{
			if(bValid)
				bValid = PushImpl(source, length);
		}
		
		bool IsValid() const { return bValid; }

	protected:

		virtual bool PushImpl(const void* source, int length) = 0;
		
		bool bValid {true};
	};
}
