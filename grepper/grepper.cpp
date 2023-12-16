// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <filesystem>
#include "grepper.h"
using namespace std::filesystem;

using namespace std;

void visit(path p)
{
	std::cout << "in " << p << "\n";
}

//"C:\\Users\\gnils\\Documents\\_MyProj\\My LV Projects 2021\\find all vi references\\grep.exe" -rlwa --exclude-dir=.git	 --exclude-dir=.vs 'GetProcess or ThreadTimes.vi' 
// -rlwa --exclude-dir=.git --exclude-dir=.vs --search-in="C:\Users\gnils\Documents\_MyProj" "GetProcess or ThreadTimes.vi" 
int main(int argc, char* argv[])
{
	path pathToSearch(L"C:/Users/gnils/Documents/_MyProj");
	std::cout << "argc=" << argc << "\n";
	for (size_t i = 0; i < argc; i++)
	{
		basic_string <char> p(argv[i]);
		std::cout << p << "\n";
		if (p.starts_with("--search-in="))
			pathToSearch = p.erase(0, 12);

	}

	std::cout << "Search in " << pathToSearch << "\n";

	for (recursive_directory_iterator next(pathToSearch), end; next != end; ++next)
	{
		path p = next->path();
		if (p.string().find(".git") == string::npos)
			visit(p);
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
