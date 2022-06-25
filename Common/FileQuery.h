#pragma once


struct FileContent
{
	const char* filename;
	unsigned char* data;
	int size;
};


FileContent LoadFileContent(void* backendData, const char* filePath);