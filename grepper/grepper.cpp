// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <list>
#include <windows.h>

using namespace std::filesystem;

using namespace std;

int count_files = 0;
int count_found = 0;
int bad_files = 0;

list<wstring> excludedDirectories;
int firstCharToSearch;
wstring stringToSearch;

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
		if (tolower(*c++) == firstCharToSearch)
		{
			size_t i = 1;
			for (; i < stringToSearch.length(); i++)
			{
				if (c >= end) break;
				if (tolower(*c++) != stringToSearch[i]) break;
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

	wstring pathToSearch;

	if (argc < 3)
	{
		usage();
		return 1;
	}

	for (size_t i = 1; i < argc; i++)
	{
		string p(argv[i]);

		if (p.compare("-h") == 0)
		{
			usage();
			return 1;
		}
		if (p.compare("-v")==0)
			verbose = true;

		if (p.starts_with("--search-in="))
		{
			auto d = p.erase(0, 12);
			wstring wstr(d.begin(), d.end());
			pathToSearch = wstr;
		}

		if (p.starts_with("--exclude-dir="))
		{
			auto d(p.erase(0, 14));
			wstring wstr(d.begin(), d.end());
			excludedDirectories.push_back(wstr);
		}

		if (i == static_cast<unsigned long long>(argc) - 1)
		{
			stringToSearch = wstring(p.begin(), p.end());
		}
	}

	if (pathToSearch.empty())
	{
		std::cout << "ERROR: option --search-in is required\n";

		usage();
		return 1;
	}

	if (verbose) std::wcout << "Search in " << pathToSearch << "\n";
	if (verbose) std::wcout << "Search for " << stringToSearch << "\n";

	if (verbose) for (auto e : excludedDirectories)
		wcout << "Excluding " << e << "\n";

	std::transform(stringToSearch.begin(), stringToSearch.end(), stringToSearch.begin(),
		[](unsigned char c) { return std::tolower(c); });

	firstCharToSearch = stringToSearch[0];

	int count = 0; //only for breakpoint condition
	for (const directory_entry& dir_entry : recursive_directory_iterator(pathToSearch, directory_options::skip_permission_denied))
	{
		count++;
		try {
			if (!dir_entry.exists()) continue;
			if (dir_entry.is_symlink()) continue;
			if (!dir_entry.is_regular_file()) continue;
			if (dir_entry.file_size()==0) continue;
				
			path p = dir_entry.path();
			bool exclude = false;
			for (auto e : excludedDirectories)
			{
				try 
				{
					if (p.wstring().find(e) != string::npos)
					{
						exclude = true;
						break;
					}
				}
				catch (std::exception const& x) {
					std::cout << "Exception1 while iterating directory." + std::string(x.what()) << "\n";
					exclude = true;
					break;
				}
				catch (...) {
					std::cout << "Unknown1 exception while iterating directory.\n";
					exclude = true;
					break;
				}
			}

			if (exclude) continue;

			if (!filesystem::is_empty(p))
				visit(p);
		}
		catch (std::exception const& e) {
			std::cout << "Exception while iterating directory." + std::string(e.what()) << "\n";
		}
		catch (...) {
			std::cout << "Unknown exception while iterating directory.\n";
		}
	}

	while (numThreads)
	{
		Sleep(100);
	}

	if (verbose) std::cout << "Found " << count_found << " files\n";
	if (verbose) std::cout << "Searched in " << count_files << " files\n";
	if (verbose) std::cout << "            " << bad_files << " bad files\n";

}
