#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <process.h>
#include <io.h>

UINT64 thrstat = 0;
CRITICAL_SECTION g_sc;
// LONG64 pushed_tasks = 0;

// FILE* log_file = NULL;
// FILE* res_file = NULL;
// FILE* dir_log = NULL;

wchar_t** tasks = NULL;

LONG64 task_count = 0;
LONG64 task_cap = 0;

wchar_t* targ;

// void logThread(UINT8 id, char* message){
// 	fprintf(log_file, "\nTH#%d: %s", id, message);
// 	fflush(stdout);
// }
// void logThreadEx(UINT8 id, wchar_t* message, wchar_t* str){
// 	fprintf(log_file, "\nTH#%d: %ls%ls", id, message, str);
// 	fflush(log_file);
// }
// void logRes(char* result){
// 	fprintf(res_file, "\n%s", result);
// }

void pushTask(const wchar_t* task){
	EnterCriticalSection(&g_sc);
	
		if(task_count == task_cap){
			task_cap *=2;
			tasks = realloc(tasks, task_cap * sizeof(wchar_t*));
		}

		tasks[task_count] = malloc((wcslen(task)+1)*sizeof(wchar_t));
		if(tasks[task_count] == NULL){
			exit(__LINE__);
		}
		wcscpy(tasks[task_count], task);

		task_count++;
		// pushed_tasks++;

	LeaveCriticalSection(&g_sc);
	return;
}

void popTask(wchar_t** addr){
	EnterCriticalSection(&g_sc);
	
		if(task_count == 0){
			*addr = NULL;
		}
		else{
			wchar_t* popped = tasks[--task_count];
			int len = wcslen(popped);
			*addr = malloc((len + 1) * sizeof(wchar_t));
			memcpy(*addr, popped, (len+1)*sizeof(wchar_t));
			free(popped);
		}
		
	LeaveCriticalSection(&g_sc);
}

boolean cmp(wchar_t* str){
	return StrStrIW(str, targ) != NULL;
}

unsigned __stdcall taskRunner(void* param){
	const UINT8 id = (UINT8)(uintptr_t)param;

	WIN32_FIND_DATAW findData;
	HANDLE hFind;

	while(1){
		wchar_t* dir;
		popTask(&dir);
		if(dir == NULL){
			InterlockedAnd64(&thrstat, ~(1ULL << id));
			if(thrstat == 0){
				// logThread(id, "crashing");
				break;
			}
			else{
				// logThread(id, "sleeping");
				Sleep(1);
				continue;
			}
		}
		else{
			// logThreadEx(id, "got dir: ", dir);
			// fprintf(log_file, "\nTH#%d: popped dir len = %zu", id, wcslen(dir));
			// logThreadEx(id, L"scanning: ", dir);
			InterlockedOr64(&thrstat, 1ULL << id);
		}
		
		size_t len = wcslen(dir);
		wchar_t* searchDir = malloc((len + 2)*sizeof(wchar_t));
		if(searchDir == NULL){
			exit(__LINE__);
		}

		memcpy(searchDir, dir, len*sizeof(wchar_t));
		searchDir[len] = L'*';
		searchDir[len+1] = L'\0';

		hFind = FindFirstFileW(searchDir, &findData);

		if(hFind == INVALID_HANDLE_VALUE){
		    free(searchDir);
		    continue;
		}

		do{
			wchar_t* fileName = findData.cFileName;
			size_t name_len = wcslen(fileName);

			if(wcscmp(fileName, L".") == 0 || wcscmp(fileName, L"..") == 0){continue;} //отсечение "." ".." из списка
			UINT8 is_dir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if(is_dir){ //если папка

				size_t dirlen = len + wcslen(fileName);

				wchar_t* newDirname = malloc((dirlen+2)*sizeof(wchar_t));
				if(newDirname == NULL){
					exit(__LINE__);
				}
				
				memcpy(newDirname, dir, len*sizeof(wchar_t));
				memcpy(newDirname + len, fileName, name_len*sizeof(wchar_t));
				// fprintf(log_file, "\nTH#%d: len: %d", id, len*(sizeof(wchar_t)));
				// fprintf(log_file, "\nTH#%d: name_len: %d", id, name_len*(sizeof(wchar_t)));
				newDirname[dirlen] = L'/';
				newDirname[dirlen+1] = L'\0';
				// logThreadEx(id, L"newDirname after adding fileName: ", newDirname);

				// logThreadEx(id, L"pushing: ", newDirname);
				pushTask(newDirname);
				free(newDirname);
			}

			//Проверка названия на совпадение
			if(cmp(fileName)){
				// wprintf(L"%ls%ls<%d><2>", dir, fileName, is_dir);
				char utf8_dir[1024], utf8_file[1024];

    			WideCharToMultiByte(CP_UTF8, 0, dir, -1, utf8_dir, sizeof(utf8_dir), NULL, NULL);
    			WideCharToMultiByte(CP_UTF8, 0, fileName, -1, utf8_file, sizeof(utf8_file), NULL, NULL);
				// fprintf("\n%s%s", dir, fileName);

				char buffer[2048];
				int len = snprintf(buffer, sizeof(buffer), "%s%s<%d><2>", utf8_dir, utf8_file, is_dir);
				fwrite(buffer, 1, len, stdout);
				fflush(stdout);

				// printf("%ls%ls<%d><2>", dir, fileName, is_dir);
				// fflush(stdout);

				// logThreadEx(id, L"dir: ", dir);
				// logThreadEx(id, L"fileName: ", fileName);
				// fprintf(log_file, "\nTH#%d: result: %ls%ls<%d>", id, dir, fileName, (int)is_dir);
			}

		}while(FindNextFileW(hFind, &findData));

		free(searchDir);

		FindClose(hFind);
	}
	return 0;
}

void initThreads(DWORD maxthreads, HANDLE* threads){
	for(int i = 0; i < maxthreads; i++){
		threads[i] = (HANDLE)_beginthreadex(
			NULL, 0,
			taskRunner,
			(void*)(intptr_t)i,
			0, NULL
		);
		if(threads[i] == 0){
			exit(__LINE__);
		}
	}
}

int main(int argc, char* argv[]){

	// log_file = fopen("debug_log.txt", "w");
	// if(!log_file){
	// 	printf("\nerr open log file");
	// 	exit(__LINE__);
	// }
	// res_file = fopen("result.txt", "w");
	// if(!res_file){
	// 	printf("\nerr open res file");
	// 	exit(__LINE__);
	// }
	// dir_log = fopen("dir_log.txt", "w");
	// if(!dir_log){
	// 	printf("\nerr open dir file");
	// 	exit(__LINE__);
	// }

	// setbuf(stdout, NULL);

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	DWORD maxthreads = sysinfo.dwNumberOfProcessors;
	// DWORD maxthreads = 1;
	
	HANDLE threads = alloca(maxthreads* sizeof(HANDLE));

	// char* buffer = malloc(256 * 1024 * 1024);
	_setmode(_fileno(stdout), 0x8000);
	// _setmode(_fileno(stdout), 0x20000);
	// setvbuf(stdout, buffer, _IOFBF, 256 * 1024 * 1024);
	
	tasks = malloc(1000 * sizeof(wchar_t*));
	if(tasks == NULL){
		exit(__LINE__);
	}
	task_cap = 1000;

	InitializeCriticalSection(&g_sc);
	
		int size = MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, NULL, 0);
		wchar_t* searchStr = (wchar_t*)malloc(size * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, searchStr, size);
		targ = searchStr;
		
		int utf8_len = strlen(argv[1]);
		int wchar_size = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);
		
		wchar_t* basetask = (wchar_t*)malloc((wchar_size + 1) * sizeof(wchar_t));
		if (basetask == NULL) {
		    exit(__LINE__);
		}
		MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, basetask, wchar_size);
		int current_len = wcslen(basetask);
		basetask[current_len] = L'/';
		basetask[current_len + 1] = L'\0';
		pushTask(basetask);
		free(basetask);

		// LARGE_INTEGER frequency, start, end;
		// QueryPerformanceFrequency(&frequency);
		// QueryPerformanceCounter(&start);

		initThreads(maxthreads, threads);
		WaitForMultipleObjects(maxthreads, threads, TRUE, INFINITE);
		printf("<3>");

		// QueryPerformanceCounter(&end);  

		// double elapsed = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
		// printf("\n=== EXECUTION TIME ===\n");
		// printf("Scanning took: %.2f ms\n", elapsed);

		fflush(stdout);

	DeleteCriticalSection(&g_sc);
	return 0;
}