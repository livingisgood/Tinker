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

