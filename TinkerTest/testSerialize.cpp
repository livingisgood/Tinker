#include "gtest/gtest.h"
#include <iostream>

struct Foo
{
	Foo() = default;
	Foo(const Foo&)
	{
		std::cout << "Foo is copy constructed.\n"; 
	}

	Foo(Foo&&) noexcept
	{
		std::cout << "Foo is move constructed.\n";
	}

	Foo& operator=(const Foo& Other)
	{
		Foo Tmp(Other);
		
		Swap(Tmp);
		return *this;
	}
	
	void Swap(Foo&) noexcept
	{
		
	}
};

TEST(SFINAE, FunctionCall)
{
	Foo A;
	A = Foo();
}


