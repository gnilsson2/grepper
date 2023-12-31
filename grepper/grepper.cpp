// grepper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <list>
#include <windows.h>
#include <mutex>

using namespace std::filesystem;

using namespace std;

int count_files = 0;
int count_found = 0;
int bad_files = 0;

list<wstring> excludedDirectories;
list<wstring> includedFiles;
string stringToSearch;

atomic_ullong numThreads = 0;
atomic<bool> threadStarted{ false };

mutex cout_mutex;
bool search(char*& c, long long size)
{
	char* end = c + size;

	while (c < end)
	{
		if (tolower(*c++) == stringToSearch[0])
		{
			size_t i = 1;
			for (; i < stringToSearch.length(); i++)
			{
				if (tolower(*c++) != stringToSearch[i]) break;
			}

			if (i == stringToSearch.length())
			{
				return true;
			}
		}
	};
	return false;
}

DWORD WINAPI process(LPVOID lpParam)
{
	numThreads++;
	threadStarted = true;
	threadStarted.notify_one();

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
		numThreads--;
		return 1;
	}

	HANDLE fileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (fileMap == NULL)
	{
		bad_files++;
		CloseHandle(hFile);
		LocalFree(lpParam);
		numThreads--;
		return 1;
	}

	LPVOID map = MapViewOfFile(fileMap, FILE_MAP_READ, 0, 0, 0);
	if (map == NULL)
	{
		bad_files++;
		CloseHandle(fileMap);
		CloseHandle(hFile);
		LocalFree(lpParam);
		numThreads--;
		return 1;
	}

	long long size=0;
	GetFileSizeEx(hFile,(PLARGE_INTEGER) & size);

	char* c = (char*)map;
	bool found = search(c, size-stringToSearch.length()+1);

	if (found)
	{
		count_found++;
		wstring ws((LPCWSTR)lpParam);
		std::lock_guard<std::mutex> lock(cout_mutex);
		cout << string(ws.begin(), ws.end()) << "\n";
	}


	UnmapViewOfFile(map);
	CloseHandle(fileMap);
	CloseHandle(hFile);
	LocalFree(lpParam);
	numThreads--;
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

	threadStarted = false;
	// Create the thread to begin execution on its own.
	HANDLE hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		process,       // thread function name
		pData,          // argument to thread function 
		0,                      // use default creation flags 
		0);   // returns the thread identifier 

	// Check the return value for success.
	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 

	if (hThread == NULL)
	{
		ExitProcess(3);
	}
	SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);

	threadStarted.wait(false);
	
	CloseHandle(hThread);
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		TerminateProcess(GetCurrentProcess(),1);
		return TRUE;
	default:
		return FALSE;
	}
}

void usage()
{
	std::cout << "Usage: grepper [options] string path\n";
	std::cout << "Search for string in all files recursively in path.\n";
	std::cout << "\n";
	std::cout << "   --exclude-dir=string\tRepeat for each directory to exclude\n";
	std::cout << "   string\t\tText to be searched for\n";
	std::cout << "   path\t\t\tWhere to search recursively\n";
	std::cout << "\n";
	std::cout << "Example:\n";
	std::cout << "  grepper --exclude-dir =.git --exclude-dir =.vs \"My vi.vi\" \"C:\\Users\\gnils\\Documents\\_MyProj\"\n";

}

// -v --exclude-dir=.git --exclude-dir=.vs "GetProcess or ThreadTimes.vi" "C:\Users\gnils\Documents\_MyProj"
// grep [OPTION...] PATTERNS [FILE...]

int main(int argc, char* argv[])
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	bool verbose = false;

	path pathToSearch;

	if (argc < 2)
	{
		usage();
		return 1;
	}

	for (size_t i = 1; i < argc; i++)
	{
		string p(argv[i]);

		if (p.compare("-h") == 0 || p.compare("/H") == 0 || p.compare("/h") == 0 || p.compare("/?") == 0)
		{
			usage();
			return 1;
		}
		if (p.compare("-v") == 0 || p.compare("/V") == 0 || p.compare("/v") == 0)
		{
			verbose = true;
			continue;
		}

		if (p.starts_with("--exclude-dir="))
		{
			string d(p.erase(0, 14));
			wstring wstr(d.begin(), d.end());
			excludedDirectories.push_back(wstr);
			continue;
		}
		if (p.starts_with("/X:") || p.starts_with("/x:"))
		{
			string d(p.erase(0, 3));
			wstring wstr(d.begin(), d.end());
			excludedDirectories.push_back(wstr);
			continue;
		}

		if (!p.starts_with('-') && !p.starts_with('/') && stringToSearch.empty())
		{
			stringToSearch = string(p.begin(), p.end());
			continue;
		}

		if (!p.starts_with('-') && !p.starts_with('/') && !stringToSearch.empty())
		{
			std::wstring::size_type f = p.find('*');
			if (f != string::npos)
			{
				wstring wstr(p.begin(), p.begin()+f);
				pathToSearch = wstr;
				wstring wstr1(p.begin() + f + 1,p.end());
				includedFiles.push_back(wstr1);
				continue;
			}
			wstring wstr(p.begin(), p.end());
			pathToSearch = wstr;
			continue;
		}
	}

	if (pathToSearch.empty())
	{
		std::cout << "ERROR: parameter path is required\n";

		usage();
		return 1;
	}

	if (verbose) std::wcout << "Search in " << pathToSearch.wstring() << "\n";
	if (verbose) std::cout << "Search for " << stringToSearch << "\n";

	if (verbose) for (wstring e : excludedDirectories) wcout << "Excluding " << e << "\n";
	if (verbose) for (wstring e : includedFiles) wcout << "Including " << e << "\n";

	std::transform(stringToSearch.begin(), stringToSearch.end(), stringToSearch.begin(),
		[](unsigned char c) { return std::tolower(c); });

	try
	{
		for (const directory_entry& dir_entry : recursive_directory_iterator(pathToSearch, directory_options::skip_permission_denied))
		{
			Sleep(0);
			try {
				if (!dir_entry.exists()) continue;
				if (dir_entry.is_symlink()) continue;
				if (!dir_entry.is_regular_file()) continue;
				if (dir_entry.file_size() == 0) continue;

				path p = dir_entry.path();
				bool exclude = false;
				for (wstring e : excludedDirectories)
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
						std::cerr << "Exception1 while iterating directory." + std::string(x.what()) << "\n";
						exclude = true;
						break;
					}
					catch (...) {
						std::cerr << "Unknown1 exception while iterating directory.\n";
						exclude = true;
						break;
					}
				}

				for (wstring e : includedFiles)
				{
					try
					{
						if (p.wstring().find(e) == string::npos)
						{
							exclude = true;
							break;
						}
					}
					catch (std::exception const& x) {
						std::cerr << "Exception1 while iterating directory." + std::string(x.what()) << "\n";
						exclude = true;
						break;
					}
					catch (...) {
						std::cerr << "Unknown1 exception while iterating directory.\n";
						exclude = true;
						break;
					}
				}

				if (exclude) continue;

				if (!filesystem::is_empty(p))
					visit(p);
			}
			catch (std::exception const& e) {
				UNREFERENCED_PARAMETER(e);
				//std::cerr << "Exception while iterating directory." + std::string(e.what()) << "\n";
			}
			catch (...) {
				std::cerr << "Unknown exception while iterating directory.\n";
			}
		}
	}
	catch (std::exception const& e) {
		UNREFERENCED_PARAMETER(e);
		std::cerr << "Exception while iterating directory." + std::string(e.what()) << "\n";
		return 1;
	}
	while (numThreads)
	{
		Sleep(100);
	};

	if (verbose) std::cout << "Found " << count_found << " files\n";
	if (verbose) std::cout << "Searched in " << count_files << " files\n";
	if (verbose) std::cout << "Unable to read " << bad_files << " files\n";

	if (verbose)
	{
		cerr.precision(3);
		cerr << "\n";
		__int64 cre = 0, exit = 0, kern = 0, user = 0;
		GetProcessTimes(GetCurrentProcess(), (LPFILETIME)& cre, (LPFILETIME)&exit, (LPFILETIME)&kern, (LPFILETIME)&user);
		__int64 now = 0;
		SYSTEMTIME systime;
		GetSystemTime(&systime);
		SystemTimeToFileTime(&systime, (LPFILETIME)&now);
		cerr << "real " << fixed << (now - cre) * 100e-9 << " \n";
		cerr << "user " << fixed << user * 100e-9 << " \n";
		cerr << "sys  " << fixed << kern * 100e-9 << " \n";

		ULONG64 CycleTime;
		QueryProcessCycleTime(GetCurrentProcess(), &CycleTime);
		cerr << "Giga Cpu cycles " << CycleTime * 1e-9 << " \n";
	}
}
