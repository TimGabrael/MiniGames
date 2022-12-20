#pragma once
#include <unordered_map>


class IniFile
{
public:
	IniFile() {};
	bool LoadFile(const char* fileName);
	void Store(const char* fileName);

	void SetIntegerValue(const char* name, int val);
	void SetHexValue(const char* name, uint32_t hexVal);

	void SetStringValue(const char* name, const char* str);

	bool GetIntegerValue(const char* name, int* val);
	bool GetStringValue(const char* name, std::string& str);

private:
	std::unordered_map<std::string, std::string> attribs;
};