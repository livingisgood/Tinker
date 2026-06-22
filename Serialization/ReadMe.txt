Tinker serialization usages:

most of the time, you could use TK_SERIAL marco:

struct FDummyUserData
{
	bool bChecked;
	int ID;
	float Position[3];

	TK_SERIAL(bChecked, ID, Position)
};

sometimes you have to handle save and load differently, in such case just
define Save and Load for your type.

struct FDummyUserData2
{
	template<typename T>
	void Save(T& Stream) const
	{
        		
	}

	template<typename T>
	void Load(T& Stream)
	{
		
	}
};

sometimes you want to support serialization for types that you cannot modify its code.
you could define two functions Save and Load outside.

struct FDummyUserData3
{
	
};

template<typename T>
void Save(T& Stream, const FDummyUserData3& Data)
{
	
}

template<typename T>
void Load(T& Stream, FDummyUserData3& Data)
{
	
}

void Test()
{
    FDummyUserData Source;
    TK::FMemWriter Writer;
    Writer & Source; // save data into Writer, could call Save(Writer, Source) instead
    
    FDummyUserData Copy;
    TK::FMemReader Reader(Writer.GetBuffer(), Writer.GetSize());
    Reader & Copy; // load data from Reader, could call Load(Reader, Copy) instead      
}


Cross-platform notes
--------------------

The serialized bytes are the raw in-memory representation of each value
(no endianness conversion is applied). For this to be safe across machines,
the following assumptions must hold between the writing side and the
reading side:

1. Integer width and signedness must match.
   Prefer fixed-width integer types (std::int32_t, std::uint64_t, ...) over
   the platform-dependent int / long / size_t, so that both sides agree on
   the exact number of bytes written.

2. Floating-point types use their native memory layout.
   IsBitwisePackable<float> / IsBitwisePackable<double> are guarded by a
   sizeof check (4 / 8 bytes respectively) and fail to compile on platforms
   that do not meet it, forcing the user to provide a custom Save/Load.
   Note: this only verifies the width, not that the layout is IEEE-754.
   Every mainstream platform today provides IEC-559 floats, so in practice
   this is fine.

3. Both sides must share the same byte order.
   All mainstream targets (x86 / x64, ARM in little-endian mode, Apple
   Silicon, AWS Graviton, etc.) are little-endian, so communication among
   PC, Mac, iOS, Android and common cloud hosts works out of the box.
   Big-endian machines (e.g. IBM z, classic MIPS routers, some PowerPC /
   SPARC systems) are rare today; if you ever need to talk to one, do the
   endian conversion on the caller side before pushing into the stream.

