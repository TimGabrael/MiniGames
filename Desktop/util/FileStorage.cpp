#include "FileStorage.h"
#include <fstream>
#include <string>
#include <sstream>



bool IniFile::LoadFile(const char* fileName)
{
	std::ifstream file(fileName);
	if (!file.is_open()) return false;

	std::string line;
	while (file >> line)
	{
		const size_t equal = line.find('='); // has value inside
		if (equal != -1LL) 
		{
			std::string name = line.substr(0, equal);
			std::string val = line.substr(equal + 1, line.size() - equal + 1);
			attribs[name] = val;
		}
	}
	file.close();
	return true;
}

void IniFile::Store(const char* fileName)
{
	std::ofstream file(fileName);
	if (!file.is_open()) return;


	for (const auto& pair : attribs)
	{
		file << pair.first << "=" << pair.second << "\n";
	}
}

void IniFile::SetIntegerValue(const char* name, int val)
{
	attribs[name] = std::to_string(val);
}
void IniFile::SetHexValue(const char* name, uint32_t hexVal)
{
	std::stringstream ss;
	ss << "0x" << std::hex << hexVal;
	attribs[name] = ss.str();
}
void IniFile::SetStringValue(const char* name, const char* str)
{
	attribs[name] = str;
}

bool IniFile::GetIntegerValue(const char* name, int* val)
{
	if (attribs.find(name) == attribs.end()) return false;	// key does not exist

	const std::string& valStr = attribs[name];
	const size_t xOffset = valStr.find('x');	// if it has a x it is a hex string
	std::stringstream ss;


	if (xOffset == -1LL)
	{
		ss << valStr;
		if (ss.fail()) return false;
		ss >> *val;
	}
	else
	{
		ss << std::hex << valStr;
		if (ss.fail()) return false;
		ss >> *val;
	}
	return true;
}
bool IniFile::GetStringValue(const char* name, std::string& str)
{
	if (attribs.find(name) == attribs.end()) return false;

	str = attribs[name];
	return true;
}