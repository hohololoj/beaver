#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>

#include "strhasiw.h"

#define INDEX_START (1<<14)

typedef struct{
	WCHAR name[260];
	int nameLen;
	BOOL isDir;
	UINT64 parentId;
}MFTDataRecord; // 536 байтов

typedef struct{
	DWORD start;
	DWORD end;
}ThreadArgs;

#define PERFORMANCE_DECLARATION()\
	LARGE_INTEGER frequency, start, end;\
    double start_ns, end_ns;\
    QueryPerformanceFrequency(&frequency);
#define PERFORMANCE_START()\
	QueryPerformanceCounter(&start);
#define PERFORMANCE_END()\
	QueryPerformanceCounter(&end);
#define PERFORMANCE_CALC()\
	start_ns = (double)start.QuadPart * 1000000000.0 / frequency.QuadPart;\
    end_ns = (double)end.QuadPart * 1000000000.0 / frequency.QuadPart;

BYTE* buffer = NULL;
DWORD bufferSize = 0;

MFTDataRecord* mftDataRecords = NULL;
DWORD cMftDataRecords = 0;

int CPUs = 0;
WCHAR* target = NULL;
WCHAR* path = NULL;
int pathLen = 0;

HANDLE* threads = NULL;
ThreadArgs* thrArgs = NULL;
UINT64 parentDirId;
CRITICAL_SECTION cs;

void getLastFileId(){
	
	HANDLE hDir = CreateFileW(
		path,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
	);

	if(hDir == INVALID_HANDLE_VALUE){
		exit(GetLastError());
	}

	FILE_ID_INFO fidInfo = {0};

	if(!GetFileInformationByHandleEx(
		hDir,
		FileIdInfo,
		&fidInfo,
		sizeof(fidInfo)
	)){
		CloseHandle(hDir);
		exit(__LINE__);
	}

	CloseHandle(hDir);

	memcpy(&parentDirId, &fidInfo.FileId, 8);
	parentDirId &= 0xFFFFFFFFFFFF;
}

void makeTable(){

	HANDLE hVol = CreateFileW(
		L"\\\\.\\C:", 
        GENERIC_READ, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS, 
        NULL
	);

	if (hVol == INVALID_HANDLE_VALUE) {
		// printf("Volume open error\n");
        exit(__LINE__);
    }

	NTFS_VOLUME_DATA_BUFFER volData = { 0 };
    DWORD bRead = 0;

	BOOL ok = DeviceIoControl(
		hVol,
        FSCTL_GET_NTFS_VOLUME_DATA,  // Управляющий код
        NULL, 0,                      // Входной буфер не нужен
        &volData, sizeof(volData), // Выходной буфер строго фиксированного размера
        &bRead,
        NULL
	);

	if(!ok){
		// printf("failed to read NTFS_VOLUME_DATA, error: %lu\n", GetLastError());
		exit(__LINE__);
	}
	
	DWORD mftLen = volData.MftValidDataLength.QuadPart;

	HANDLE hMft = CreateFileW(
        L"\\\\?\\C:\\$MFT", 
        FILE_READ_ATTRIBUTES, // Разрешено для системных метафайлов
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
	
	if (hMft == INVALID_HANDLE_VALUE) {
        // printf("Failed to open $MFT via ID. Error: %lu\n", GetLastError());
        exit(__LINE__);
    }

	STARTING_VCN_INPUT_BUFFER inputVcn;
	inputVcn.StartingVcn.QuadPart = 0;

	bufferSize = mftLen;
	buffer = (BYTE*)malloc(bufferSize);

	ok = DeviceIoControl(
        hMft,
        FSCTL_GET_RETRIEVAL_POINTERS,
        &inputVcn,
        sizeof(inputVcn),
        buffer,
        bufferSize,
        &bRead,
        NULL
    );

	if (!ok && GetLastError() != ERROR_MORE_DATA) {
        // printf("Couldn't read retrieval points: %lu\n", GetLastError());
        exit(__LINE__);
    }

	BYTE* extentsBuffer = (BYTE*)malloc(bRead);
	memcpy(extentsBuffer, buffer, bRead);

	RETRIEVAL_POINTERS_BUFFER* rpb = (RETRIEVAL_POINTERS_BUFFER*)extentsBuffer;
	DWORD bLen;
	UINT64 pointer = 0;;
	LARGE_INTEGER pos;
	UINT64 startingVcn = rpb->StartingVcn.QuadPart;
	for(DWORD i = 0; i < rpb->ExtentCount; i++){

		if(rpb->Extents[i].Lcn.QuadPart == -1){goto next_extent;}
		bLen = (rpb->Extents[i].NextVcn.QuadPart - startingVcn)*volData.BytesPerCluster;		

		pos.QuadPart = rpb->Extents[i].Lcn.QuadPart * volData.BytesPerCluster;
		SetFilePointerEx(hVol, pos, NULL, FILE_BEGIN);

		ok = ReadFile(hVol, (BYTE*)(buffer+pointer), bLen, &bRead, NULL);
		if(!ok){
			exit(__LINE__);
		}
		
		pointer += bLen;
		next_extent:{
			startingVcn = rpb->Extents[i].NextVcn.QuadPart;
		}
	}

	CloseHandle(hMft);
	CloseHandle(hVol);

	DWORD offset = volData.BytesPerFileRecordSegment;
	DWORD cRecords = pointer / volData.BytesPerFileRecordSegment;

	mftDataRecords = (MFTDataRecord*)malloc(cRecords*sizeof(MFTDataRecord));
	
	BYTE* record = NULL;

	for(DWORD i = 0; i < cRecords; i++){

		record = buffer + (i * offset);

		DWORD signature = *(DWORD*)record;
		if(signature != 0x454C4946){ //[F][I][L][E]
			continue;
		}

		WORD flags = *(WORD*)(record+22);
		if(!(flags & 0x0001)){continue;} //не используемая запись

		mftDataRecords[i].isDir = (flags & 0x0002) != 0;

		WORD attrOffset = *(WORD*)(record + 20);
		BYTE* attr = record + attrOffset;
		BYTE* recordEnd = record + offset;

		while(1){

			if (attr >= recordEnd) {
    		    // printf("CRASH at record %lu, attr overflow!\n", i);
    		    break;
    		}

			DWORD attrType = *(DWORD *)(attr + 0);

			if (attrType == 0xFFFFFFFF){ //конец списка
				break;
			}
			if (attrType == 0x30){
				
				BYTE* content = attr + *(WORD*)(attr + 0x14);
				BYTE nameSpace = *(BYTE*)(content + 65);
				if(nameSpace == 0x02){goto next_iteration;} //DOS (короткие имена) - скипаем

				mftDataRecords[i].parentId = *(UINT64*)(content + 0) & 0xFFFFFFFFFFFF;
				mftDataRecords[i].nameLen = *(BYTE*)(content + 64);
				WCHAR* namePtr = (WCHAR*)(content + 66);
			

				memcpy(mftDataRecords[i].name, namePtr, mftDataRecords[i].nameLen*sizeof(WCHAR));
				// printf("nameLen: %lu\n", nameLength);
				mftDataRecords[i].name[mftDataRecords[i].nameLen] = L'\0';
				
			}

			next_iteration:{
				DWORD attrLen = *(DWORD*)(attr + 4);
				if (attrLen == 0){break;}

				attr += attrLen;
			}
		}

		cMftDataRecords++;

	}
	free(buffer);
}

//-1 Том не существует
int getVolSizeGiB(WCHAR volName){
	
	WCHAR lpath[] = L"X:\\";
	lpath[0] = volName;
	ULARGE_INTEGER uli;
	if(!GetDiskFreeSpaceExW(lpath, NULL, &uli, NULL)){return -1;}

	return (int)(uli.QuadPart>>30);
}

// void scanTable(){
// 	WCHAR* fullName = (WCHAR*)_alloca(INDEX_START*sizeof(WCHAR));
// 	if(!fullName){
// 		// printf("fullname malloc fail, error: %lu", GetLastError());
// 		exit(__LINE__);
// 	}
// 	fullName[(INDEX_START) - 1] = L'\0';
// 	fullName[(INDEX_START) - 2] = L'>';
// 	fullName[(INDEX_START) - 3] = L'2';
// 	fullName[(INDEX_START) - 4] = L'<';
// 	int index = INDEX_START-5;

// 	UINT64 nextParent;
// 	for(DWORD i = 5; i < cMftDataRecords; i++){
// 		if(mftDataRecords[i].parentId == 0){continue;}

// 		if(StrHasIWBruteA(mftDataRecords[i].name, target)){ //Нашли файл по названию

// 			if(mftDataRecords[i].isDir){
// 				fullName[index--] = L'>';
// 				fullName[index--] = L'1';
// 				fullName[index--] = L'<';
// 			}
// 			memcpy(fullName+(index-mftDataRecords[i].nameLen+1), mftDataRecords[i].name, mftDataRecords[i].nameLen*sizeof(WCHAR));
// 			index -= (mftDataRecords[i].nameLen);
// 			fullName[index--] = L'\\';

// 			nextParent = mftDataRecords[i].parentId;
// 			while(1){
					
// 				if(nextParent == parentDirId){
// 					int pathLen = wcslen(path);
// 					memcpy(fullName+(index-pathLen+2), path, pathLen*sizeof(WCHAR));
// 					index -= pathLen;
// 					printf("%ls\n",(WCHAR*)(fullName+index+2));
// 					fflush(stdout);
// 					break;
// 				}
// 				else if(nextParent == 5 && parentDirId != 5){
// 					break;
// 				}

// 				if (index < 0) {
// 				    // printf("ERROR: index underflow!\n");
// 				    break;
// 				}
				
// 				memcpy((fullName+(index-mftDataRecords[nextParent].nameLen+1)), mftDataRecords[nextParent].name, (mftDataRecords[nextParent].nameLen)*sizeof(WCHAR));
// 				index -= (mftDataRecords[nextParent].nameLen);

// 				fullName[index--] = L'\\';
// 				nextParent = mftDataRecords[nextParent].parentId;

// 			};
// 			index = INDEX_START-5;
// 		}
// 	}
// 	printf("<3>\n");
// }

unsigned int __stdcall parseTable(void* bytes){
	DWORD start = ((ThreadArgs*)bytes)->start;
	DWORD end = ((ThreadArgs*)bytes)->end;
	WCHAR* resBuf = (WCHAR*)_alloca(1025*sizeof(WCHAR));
	int resBufIndex = 0;

	WCHAR* fullName = (WCHAR*)_alloca(INDEX_START*sizeof(WCHAR));
	if(!fullName){
		// printf("fullname malloc fail, error: %lu", GetLastError());
		exit(__LINE__);
	}
	// fullName[(INDEX_START) - 1] = L'\0';
	fullName[(INDEX_START) - 1] = L'>';
	fullName[(INDEX_START) - 2] = L'2';
	fullName[(INDEX_START) - 3] = L'<';
	int index = INDEX_START-4;

	UINT64 nextParent;
	for(DWORD i = start; i < end; i++){
		if(mftDataRecords[i].parentId == 0){continue;}

		if(StrHasIWBruteA(mftDataRecords[i].name, target)){ //Нашли файл по названию

			if(mftDataRecords[i].isDir){
				fullName[index--] = L'>';
				fullName[index--] = L'1';
				fullName[index--] = L'<';
			}
			memcpy(fullName+(index-mftDataRecords[i].nameLen+1), mftDataRecords[i].name, mftDataRecords[i].nameLen*sizeof(WCHAR));
			index -= (mftDataRecords[i].nameLen);
			fullName[index--] = L'\\';

			nextParent = mftDataRecords[i].parentId;
			while(1){
					
				if(nextParent == parentDirId){
					memcpy(fullName+(index-pathLen+2), path, pathLen*sizeof(WCHAR));
					index -= pathLen;

					if(resBufIndex + (INDEX_START-(index+2)) > 1024){
						resBuf[resBufIndex] = L'\0';
						EnterCriticalSection(&cs);
							printf("%ls\n", resBuf);
							fflush(stdout);
						LeaveCriticalSection(&cs);
						resBufIndex = 0;
					}
					memcpy((resBuf+resBufIndex), (WCHAR*)(fullName+index+2), (INDEX_START-(index+2))*sizeof(WCHAR));
					resBufIndex += (INDEX_START-index-2);

					// EnterCriticalSection(&cs);
					// 	printf("%ls\n",(WCHAR*)(fullName+index+2));
					// 	fflush(stdout);
					// LeaveCriticalSection(&cs);

					break;
				}
				else if(nextParent == 5 && parentDirId != 5){
					break;
				}

				if (index < 0) {
				    // printf("ERROR: index underflow!\n");
				    break;
				}
				
				memcpy((fullName+(index-mftDataRecords[nextParent].nameLen+1)), mftDataRecords[nextParent].name, (mftDataRecords[nextParent].nameLen)*sizeof(WCHAR));
				index -= (mftDataRecords[nextParent].nameLen);

				fullName[index--] = L'\\';
				nextParent = mftDataRecords[nextParent].parentId;

			};
			index = INDEX_START-4;
		}
	}
	if(resBufIndex != 0){
		EnterCriticalSection(&cs);
			printf("%ls\n", resBuf);
			fflush(stdout);
		LeaveCriticalSection(&cs);
	}
	return 0;
}

void initThreads(){
	DWORD remainder = 0;
	DWORD unit = 0;

	remainder = cMftDataRecords % CPUs;
	unit = cMftDataRecords / CPUs;

	DWORD index = 0;
	for(int i = 0; i < CPUs; i++){
		
		if(remainder != 0 && i == 0){
			thrArgs[0].start = 5;
			thrArgs[0].end = unit+remainder;
			index += unit+remainder;
		}
		else{
			thrArgs[i].start = index;
			thrArgs[i].end = index+unit;
			index+=unit;
		}

		threads[i] = (HANDLE)_beginthreadex(
			NULL,
			0,
			parseTable,
			&thrArgs[i],
			0,
			NULL
		);
	}
}

int wmain(int argc, WCHAR* argv[]){

	path = argv[1];
	pathLen = wcslen(path);
	getLastFileId();
	target = (WCHAR*)argv[2];

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	CPUs = sysInfo.dwNumberOfProcessors*2;

	threads = (HANDLE*)_alloca(CPUs * sizeof(HANDLE));
	thrArgs = (ThreadArgs*)_alloca(CPUs * sizeof(ThreadArgs));
	
	InitializeCriticalSection(&cs);
		makeTable();
		initThreads();
		WaitForMultipleObjects(CPUs, threads, TRUE, INFINITE);
		printf("<3>\n");
	DeleteCriticalSection(&cs);

	// scanTable();
	fflush(stdout);

	return 0;
}