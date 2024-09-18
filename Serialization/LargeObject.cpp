#include "LargeObject.h"
#include "Serialization/SerializeFuncs.h" // ReSharper disable once CppUnusedIncludeDirective

namespace TK
{
	void SendBulk(uint16_t MessageId, const char* Data, int Size, const FSender& Sender)
	{
		FMessage Message(MessageId);

		int BytesSent = 0;
		while (BytesSent < Size)
		{
			Message.ClearContent();

			uint8_t Status = 0;
			if (BytesSent == 0)
				Status |= PacketFlag::Begin;

			const int AvailableSpace = Message.GetAvailableSpace() - static_cast<int>(sizeof(Status));
			if (AvailableSpace <= 0)
			{
				TK_ASSERT(false);
				return;
			}

			const int RemainedBytes = Size - BytesSent;

			if (RemainedBytes <= AvailableSpace)
			{
				Status |= PacketFlag::End;
			}

			Message.Pack(Status);

			const int Transferred = std::min(RemainedBytes, AvailableSpace);
			Message.Push(Data + BytesSent, Transferred);

			BytesSent += Transferred;
			Sender(Message);
		}
	}

	void FBulkReceiver::Receive(FMemReader Reader)
	{
		uint8_t Status = Reader.Pop<uint8_t>();

		if (!Reader.IsValidInput())
			return;

		if (Status & PacketFlag::Begin)
		{
			Writer.Clear();
		}

		Writer.Push(Reader.GetReadPos(), Reader.GetUnreadBytes());

		if (Status & PacketFlag::End)
		{
			State = EState::Completed;

			if (CompletionHandler)
				CompletionHandler(FMemReader(Writer.GetBuffer(), Writer.GetSize()));
		}
		else
		{
			State = EState::Inprogress;
		}
	}

	void FBulkReceiver::Clear()
	{
		State = EState::Nothing;
		Writer.Clear();
	}
}
