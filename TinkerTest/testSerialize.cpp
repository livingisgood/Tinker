#include "gtest/gtest.h"
#include <typeinfo>
#include "../Serialization/MemoryReader.h"
#include "../Serialization/SerializeFuncs.h"
#include "Serialization/MemoryWriter.h"
#include <string_view>

namespace TEST_SER
{
	struct FDummy
	{
		bool bChecked;
		int ID;
		float Position[3];

		TK_SERIAL(bChecked, ID, Position)

		void Check(const FDummy& Other)
		{
			EXPECT_EQ(bChecked, Other.bChecked);
			EXPECT_EQ(ID, Other.ID);

			for (int i = 0; i < 3; ++i)
			{
				EXPECT_FLOAT_EQ(Position[i], Other.Position[i]);
			}
		}
	};

	struct FDummy2
	{
		std::wstring NickName;

		TK_SERIAL(NickName)

		void Check(const FDummy2& Other)
		{
			EXPECT_EQ(NickName, Other.NickName);
		}
	};

	struct FUserData
	{
		std::string Name;
		std::vector<int> Scores;
		std::map<std::string, int> Attributes;
		FDummy Dummy;
		FDummy2 Dummy2;

		TK_SERIAL(Name, Scores, Attributes, Dummy, Dummy2)

		void Check(const FUserData& Other)
		{
			EXPECT_EQ(Name, Other.Name);
			EXPECT_EQ(Scores, Other.Scores);
			EXPECT_EQ(Attributes, Other.Attributes);
			Dummy.Check(Other.Dummy);
			Dummy2.Check(Other.Dummy2);
		}
	};
}


struct FDummyUserData2
{
	template <typename T>
	void Save(T& Stream) const
	{
	}

	template <typename T>
	void Load(T& Stream)
	{
	}
};

struct FDummyUserData3
{
};

template <typename T>
void Save(T& Stream, const FDummyUserData3& Data)
{
}

template <typename T>
void Load(T& Stream, FDummyUserData3& Data)
{
}

TEST(Serialize, FDummy)
{
	TEST_SER::FUserData Source;
	Source.Name = "Hello";
	Source.Scores = {3, 2, 1, 0};
	Source.Attributes = {{"HP", 100}, {"Attack", 3}};
	Source.Dummy.bChecked = false;
	Source.Dummy.ID = 1001;
	Source.Dummy.Position[0] = 0.1f;
	Source.Dummy.Position[1] = 2.0f;
	Source.Dummy.Position[2] = -2;
	Source.Dummy2.NickName = L"张";

	TK::TMemWriter Writer;
	Writer & Source;

	TK::FMemReader Reader(Writer.GetBuffer(), Writer.GetSize());
	TEST_SER::FUserData Copy;
	Reader & Copy;

	Source.Check(Copy);
}
