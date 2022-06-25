#include "FileQuery.h"

#ifdef ANDROID	// ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
FileContent LoadFileContent(void* backendData, const char* filePath)
{
	FileContent outContent{ 0 };
	
	AAssetManager* manager = (AAssetManager*)backendData;
	AAsset* asset = AAssetManager_open(manager, filePath, AASSET_MODE_UNKNOWN);
	if (asset == nullptr) return outContent;

	int sz = AAsset_getLength(asset);
	outContent.data = new unsigned char[sz];
	outContent.size = sz;
	AAsset_read(asset, outContent.data, sz);

	AAsset_close(asset);

	return outContent;
	
}
#else // WINDOWS,LINUX,IOS
#ifdef _WIN32
#define OPENFILE(ptr, file) fopen_s(&ptr, file, "rb");
#define READFILE(fptr, buf, size) fread_s(buf, size, 1, size, fptr);

#else
#define OPENFILE(ptr, file) ptr = fopen_s(file, "rb");
#define READFILE(fptr, buf, size) fread(buf, 1, size, fptr);
#endif
#include <fstream>
FileContent LoadFileContent(void* backendData, const char* filePath)
{
	FileContent outContent{ 0 };
	FILE* f;
	OPENFILE(f, filePath);
	if (!f) return outContent; // file not found
	fseek(f, 0L, SEEK_END);
	size_t sz = ftell(f);
	outContent.data = new unsigned char[sz];
	outContent.size = sz;
	fseek(f, 0L, SEEK_SET);

	READFILE(f, outContent.data, sz);
	fclose(f);

	outContent.filename = filePath;
	
	return outContent;
}
#endif