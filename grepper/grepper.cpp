// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <filesystem>
#include "grepper.h"
#include <fstream>
#include <list>

using namespace std::filesystem;

using namespace std;

int count_files = 0;
int count_found = 0;
int bad_files = 0;

list<basic_string<char>> excludedDirectories;
int firstCharToSearch;
basic_string<char> stringToSearch;

void visit(path p)
{
	if (is_regular_file(p))
	{
		//std::cout << "in " << p << "\n";
		count_files++;
		ifstream ifs(p, ios_base::in|ios_base::binary, _SH_DENYNO);
		if (!ifs.bad())
		{
			// Dump the contents of the file to cout.
			//cout << ifs.rdbuf();
			while (!ifs.eof())
			{

				if (ifs.get() == firstCharToSearch)
				{
					size_t i = 1;
					for ( ; i < stringToSearch.length(); i++)
					{
						if (ifs.get() != stringToSearch[i])
							break;

					}
					if (i == stringToSearch.length())
					{
						count_found++;
						cout << p <<"\n";
						break;
					}
				}
			}

			ifs.close();
		} else 
		{
			bad_files++;
		}

	}
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
		if (p.starts_with("--exclude-dir="))
		{
			basic_string <char> e (p.erase(0, 14));
			excludedDirectories.push_back(e);
		}

		if (i == static_cast<unsigned long long>(argc) - 1)
		{
			stringToSearch = p;
			firstCharToSearch = stringToSearch[0];
		}
	}

	std::cout << "Search in " << pathToSearch << "\n";

	for (auto e : excludedDirectories)
		std::cout << "Excluding " << e << "\n";

	for (recursive_directory_iterator next(pathToSearch), end; next != end; ++next)
	{
		path p = next->path();
		bool exclude = false;
		for (auto e : excludedDirectories)
			if (p.string().find(e) != string::npos)
			{
				exclude = true;
				break;
			}
		
		if (exclude) continue;

		visit(p);
	}

	std::cout << "Found " << count_found << " files\n";
	std::cout << "Searched in " << count_files << " files\n";
	std::cout << "            " << bad_files << " bad files\n";

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
