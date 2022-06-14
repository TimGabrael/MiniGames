#pragma once
#include <unordered_map>


class IniFile
{
public:
	IniFile() {};
	void LoadFile(const char* fileName);
	void Store(const char* fileName);

	void SetIntegerValue(const char* name, int val);
	void SetHexValue(const char* name, uint32_t hexVal);

	bool GetIntegerValue(const char* name, int* val);

private:
	std::unordered_map<std::string, std::string> attribs;
};