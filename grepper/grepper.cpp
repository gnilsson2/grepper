// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <filesystem>
#include <list>
#include <functional>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

using namespace std::filesystem;

using namespace std;

int count_files = 0;
int count_found = 0;
int bad_files = 0;

bool ignoreCase = false;

list<basic_string<char>> excludedDirectories;
int firstCharToSearch;
basic_string<char> stringToSearch;


std::function<int(int)> cased = [](int x) {
	if (ignoreCase) return tolower(x);
	return x;
	};

volatile long numThreads = 0;

DWORD WINAPI process(LPVOID lpParam)
{
	InterlockedIncrement(&numThreads);

	HANDLE hFile;
	hFile = CreateFile((LPCWSTR)lpParam,                // file to open
		GENERIC_READ,           // open for reading
		FILE_SHARE_READ| FILE_SHARE_WRITE|FILE_SHARE_DELETE,        // share 
		NULL,                   // default security
		OPEN_EXISTING,          // existing file only
		FILE_FLAG_SEQUENTIAL_SCAN,   // overlapped operation
		NULL);                  // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		bad_files++;
		CloseHandle(hFile);
		LocalFree(lpParam);
		InterlockedDecrement(&numThreads);
		return 1;
	}

	HANDLE fileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (fileMap == NULL)
	{
		bad_files++;
		CloseHandle(hFile);
		LocalFree(lpParam);
		InterlockedDecrement(&numThreads);
		return 1;
	}

	LPVOID map = MapViewOfFile(fileMap, FILE_MAP_READ, 0, 0, 0);
	if (map == NULL)
	{
		bad_files++;
		CloseHandle(fileMap);
		CloseHandle(hFile);
		LocalFree(lpParam);
		InterlockedDecrement(&numThreads);
		return 1;
	}

	char* c = (char*)map;
	long long size;
	GetFileSizeEx(hFile,(PLARGE_INTEGER) & size);
	char* end = c + size;
	while (c < end)
	{
		if (cased(*c++) == firstCharToSearch)
		{
			size_t i = 1;
			for (; i < stringToSearch.length(); i++)
			{
				if (c >= end) break;
				if (cased(*c++) != stringToSearch[i]) break;
			}

			if (i == stringToSearch.length())
			{
				count_found++;
				wstring ws((LPCWSTR)lpParam);
				cout << string(ws.begin(), ws.end()) << "\n";
				break;
			}
		}

	};
	UnmapViewOfFile(map);
	CloseHandle(fileMap);
	CloseHandle(hFile);
	LocalFree(lpParam);
	InterlockedDecrement(&numThreads);
	return 0;
}

void visit(path p)
{
	count_files++;

	wchar_t* s = (wchar_t*)p.c_str();
	wchar_t* pData = (LPWSTR)LocalAlloc(LPTR, 2*wcslen(s)+2);

	if (pData == NULL)
	{
		// If the array allocation fails, the system is out of memory
		// so there is no point in trying to print an error message.
		// Just terminate execution.
		ExitProcess(2);
	}

	// Generate unique data for each thread to work with.
	size_t i;
	for (i = 0; i < wcslen(s); i++)
	{
		pData[i] = s[i];
	}
	pData[i] = 0;

	// Create the thread to begin execution on its own.
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		process,       // thread function name
		pData,          // argument to thread function 
		0,                      // use default creation flags 
		&dwThreadId);   // returns the thread identifier 


	// Check the return value for success.
	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 

	if (hThread == NULL)
	{
		//ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}
	//WaitForSingleObject(hThread, INFINITE); //debug
	CloseHandle(hThread);
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

// -i -v --exclude-dir=.git --exclude-dir=.vs --search-in="C:\Users\gnils\Documents\_MyProj" "GetProcess or ThreadTimes.vi"

int main(int argc, char* argv[])
{
	bool verbose = false;

	path pathToSearch;

	if (argc < 3)
	{
		usage();
		return 1;
	}
	for (size_t i = 0; i < argc; i++)
	{
		basic_string <char> p(argv[i]);

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
	if (verbose) std::cout << "Search for " << stringToSearch << "\n";

	if (verbose) for (basic_string <char> e : excludedDirectories)
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
		for (basic_string <char> e : excludedDirectories)
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

	while (numThreads)
	{
		Sleep(100);
	}

	if (verbose) std::cout << "Found " << count_found << " files\n";
	if (verbose) std::cout << "Searched in " << count_files << " files\n";
	if (verbose) std::cout << "            " << bad_files << " bad files\n";

}
