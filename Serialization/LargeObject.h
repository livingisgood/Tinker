#pragma once
#include <cstdint>
#include <functional>
#include "Serialization/Message.h"
#include "Serialization/MemoryReader.h"

namespace TK
{
	namespace PacketFlag
	{
		constexpr uint8_t Begin = 0x1;
		constexpr uint8_t End = 0x2;
	}

	using FSender = std::function<void(FMessage&)>;

	void SendBulk(uint16_t MessageId, const char* Data, int Size, const FSender& Sender);

	class FBulkReceiver
	{
	public:

		using FHandler = std::function<void(FMemReader)>;

		enum class EState
		{
			Nothing,
			Inprogress,
			Completed,
		};

		void SetCompletionHandler(FHandler Handler) { CompletionHandler = std::move(Handler); }

		EState GetState() const { return State; }

		void Receive(FMemReader Reader);

		void Clear();

	private:

		EState State { EState::Nothing };
		FHandler CompletionHandler;
		TMemWriter<0> Writer;
	};
	
}
