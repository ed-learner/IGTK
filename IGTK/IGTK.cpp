// IGTK.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "ConstexprSnippets.h"
#include <iostream>



int main()
{
	{
		// compute this at compile time
		const int fact4 = FactFunc(4);
		std::cout << fact4 << std::endl;
	}
	{
		// now check if expression can be used as a template argument
		IntStruct<FactFunc(4)> fact4;
		std::cout << fact4.eValue << std::endl;
	}
	{
		// now check the old template way of computing the factorial recursively at compile time
		std::cout << FactStruct<4>::eValue << std::endl;
	}

	{
		// SFINAE
		std::cout << HasMember<DummyWithData>::Yes << std::endl;
		std::cout << HasMember<DummyWithMember>::Yes << std::endl;
		// constexpr + decltype SFINAE
		std::cout << CheckHasMember<DummyWithData>() << std::endl;
		std::cout << CheckHasMember<DummyWithMember>() << std::endl;
	}
	std::system("pause");
    return 0;
}

