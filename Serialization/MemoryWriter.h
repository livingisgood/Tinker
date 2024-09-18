#pragma once
#include <variant>
#include <vector>
#include <array>
#include "Serialization/OutStream.h"
#include "TinkerAssert.h"

namespace TK
{
	template<int N>
	struct TInlinedMemory
	{
		static_assert(N > 0, "N Must be positive.");
		
		std::array<char, N> Data;
		int Size {0};
	};
	
	template<int N>
	struct TMemWriterStorage
	{
		using Type = std::variant<TInlinedMemory<N>, std::vector<char>>;
	};

	template<>
	struct TMemWriterStorage<0>
	{
		using Type = std::vector<char>;
	};

	template<int N = 64>
	class TMemWriter : public FOutStream
	{
		static_assert(N >= 0, "N cannot be negative.");
		
	public:

		std::size_t GetSize() const
		{
			if constexpr (N == 0)
			{
				return Storage.size();
			}
			else
			{
				return Inlined()? std::get<0>(Storage).Size : std::get<1>(Storage).size();
			}
		}
		
		const char* GetBuffer() const
		{
			if constexpr (N == 0)
			{
				return Storage.data();
			}
			else
			{
				return Inlined()? std::get<0>(Storage).Data.data() : std::get<1>(Storage).data();
			}
		}

		char* GetBuffer()
		{
			if constexpr (N == 0)
			{
				return Storage.data();
			}
			else
			{
				return Inlined()? std::get<0>(Storage).Data.data() : std::get<1>(Storage).data();
			}
		}
		
		std::size_t GetCapacity() const
		{
			if constexpr (N == 0)
			{
				return Storage.capacity();
			}
			else
			{
				return Inlined()? N : std::get<1>(Storage).capacity();
			}
		}

		void Clear()
		{
			bValidOutput = true;
			
			if constexpr (N == 0)
			{
				Storage.clear();	
			}
			else
			{
				if (Inlined())
				{
					std::get<0>(Storage).Size = 0;
				}
				else
				{
					std::get<1>(Storage).clear();
				}
			}
		}

		void Reserve(std::size_t Capacity)
		{
			if constexpr (N == 0)
			{
				Storage.reserve(Capacity);
			}
			else
			{
				if (Inlined())
				{
					if (Capacity > N)
					{
						std::vector<char> Heap;
						Heap.reserve(Capacity);

						const char* CurrentBuffer = std::get<0>(Storage).Data.data();
						int CurrentSize = std::get<0>(Storage).Size;
						
						Heap.insert(Heap.end(), CurrentBuffer, CurrentBuffer + CurrentSize);
						Storage = std::move(Heap);
					}
				}
				else
				{
					std::get<1>(Storage).reserve(Capacity);
				}
			}
		}

		// if TargetSize is not smaller than current size, this function has no effect.
		// otherwise, buffer will keep only the first TargetSize bytes. capacity is not changed.
		void ShrinkTo(std::size_t TargetSize)
		{
			std::size_t CurrentSize = GetSize();
			if (CurrentSize <= TargetSize)
				return;

			if constexpr (N == 0)
			{
				Storage.resize(TargetSize);
			}
			else
			{
				if (Inlined())
				{
					std::get<0>(Storage).Size = static_cast<int>(TargetSize);		
				}
				else
				{
					std::get<1>(Storage).resize(TargetSize);
				}
			}
		}
		
	protected:

		bool Inlined() const
		{
			if constexpr (N == 0)
			{
				return false;
			}
			else
			{
				return Storage.index() == 0;
			}
		}
		
		virtual bool PushImpl(const void* Source, SizeType Length) override
		{
			if (Length <= 0)
				return true;

			using StorageSizeType = std::vector<char>::size_type;
			constexpr uint64_t MaxCapacity = static_cast<uint64_t>(std::numeric_limits<StorageSizeType>::max());
			
			bool bOverflow = static_cast<uint64_t>(Length) + GetSize() > MaxCapacity;
			TK_ASSERT(!bOverflow);

			if (bOverflow)
				return false;

			auto Begin = static_cast<const char*>(Source);
			auto End = Begin + Length;
			
			if constexpr (N == 0)
			{
				Storage.insert(Storage.end(), Begin, End);
				return true;
			}
			else
			{
				if (Inlined())
				{
					std::uint64_t Current = GetSize();
					std::uint64_t Required = Current + static_cast<std::uint64_t>(Length);
					if (Required < Current)
						return false;

					auto& Container = std::get<0>(Storage);
					char* Buffer = Container.Data.data();
						
					if (Required <= N)
					{
						std::memcpy(Buffer + Current, Source, static_cast<std::size_t>(Length));
						Container.Size += static_cast<int>(Length);	
					}
					else
					{
						std::vector<char> Heap;
						Heap.reserve(static_cast<StorageSizeType>(Required));
						Heap.insert(Heap.end(), Buffer, Buffer + Current);
						Heap.insert(Heap.end(), Begin, End);

						Storage = std::move(Heap);
					}
				}
				else
				{
					auto& Vec = std::get<1>(Storage);
					Vec.insert(Vec.end(), Begin, End);
				}
			}
			return true;
		}

		typename TMemWriterStorage<N>::Type Storage { };
	};

}
