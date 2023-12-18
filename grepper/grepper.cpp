// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <thread>
#include <iostream>
#include <string>
#include <filesystem>
#include "grepper.h"
#include <fstream>
#include <list>
#include <functional>

using namespace std::filesystem;

using namespace std;

int count_files = 0;
int count_found = 0;
int bad_files = 0;

bool ignoreCase = false;

list<basic_string<char>> excludedDirectories;
int firstCharToSearch;
basic_string<char> stringToSearch;

allocator<char> myAllocator;

void process(char* s)
{
	path p(s);
	//myAllocator.deallocate(s, strlen(s));
	ifstream ifs(p, ios_base::in | ios_base::binary, _SH_DENYNO);

	std::function<int(int)> cased = [](int x) {
		if (ignoreCase) return tolower(x);
		return x;
		};

	if (!ifs.bad())
	{
		// Dump the contents of the file to cout.
		//cout << ifs.rdbuf();
		while (!ifs.eof())
		{

			if (cased(ifs.get()) == firstCharToSearch)
			{
				size_t i = 1;
				for (; i < stringToSearch.length(); i++)
				{
					if (cased(ifs.get()) != stringToSearch[i])
						break;

				}
				if (i == stringToSearch.length())
				{
					count_found++;
					cout << p.generic_string() << "\n";
					break;
				}
			}
		}

		ifs.close();
	}
	else
	{
		bad_files++;
	}


}
void visit(path p)
{
	if (is_regular_file(p))
	{
		//std::cout << "in " << p << "\n";
		count_files++;

		std::u8string path_string{ p.u8string() };
		char* str = myAllocator.allocate(path_string.length()+1);
		for (size_t i = 0; i < path_string.length(); i++)
		{
			str[i] = path_string[i];
		}
		str[path_string.length()] = 0;
		std::jthread t{ process, str };
		t.detach();
	}
}

void usage()
{
	std::cout << "Usage: grepper [options] string\n";
	std::cout << "   --search-in=string\tWhere to search recursively\n";
	std::cout << "   --exclude-dir=string\n";
	std::cout << "   string\tText to be searched for\n";
	std::cout << "Example:\n";
	std::cout << "  grepper --exclude-dir =.git --exclude-dir =.vs --search-in=\"C:\\Users\\gnils\\Documents\\_MyProj\" \"My vi.vi\"\n";

}
//"C:\\Users\\gnils\\Documents\\_MyProj\\My LV Projects 2021\\find all vi references\\grep.exe" -rlwa --exclude-dir=.git	 --exclude-dir=.vs 'GetProcess or ThreadTimes.vi' 
// -rlwa --exclude-dir=.git --exclude-dir=.vs --search-in="C:\Users\gnils\Documents\_MyProj" "GetProcess or ThreadTimes.vi" 
int main(int argc, char* argv[])
{
	bool verbose = false;

	path pathToSearch;


	if (argc < 3)
	{
		usage();
		return 1;
	}
	//std::cout << "argc=" << argc << "\n";
	for (size_t i = 0; i < argc; i++)
	{
		basic_string <char> p(argv[i]);

		//std::cout << p << "\n";
		
		if (p.compare("-h") == 0)
		{
			usage();
			return 1;
		}
		if (p.compare("-v")==0)
			verbose = true;

		if (p.compare("-i") == 0)
			ignoreCase = true;

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
		}
	}

	if (pathToSearch.empty())
	{
		std::cout << "ERROR: option --search-in is required\n";

		usage();
		return 1;
	}


	if (verbose) std::cout << "Search in " << pathToSearch << "\n";

	if (verbose) for (auto e : excludedDirectories)
		std::cout << "Excluding " << e << "\n";

	if (ignoreCase)
	{
		std::transform(stringToSearch.begin(), stringToSearch.end(), stringToSearch.begin(),
			[](unsigned char c) { return std::tolower(c); });
	}

	firstCharToSearch = stringToSearch[0];

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

		if (is_regular_file(p))
			if (!filesystem::is_empty(p))
				visit(p);
	}

	if (verbose) std::cout << "Found " << count_found << " files\n";
	if (verbose) std::cout << "Searched in " << count_files << " files\n";
	if (verbose) std::cout << "            " << bad_files << " bad files\n";

}
