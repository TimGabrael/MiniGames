#include "NetCommon.h"

uint32_t ParseIP(const char* ip)
{
	uint32_t ipAddr = 0;
	
	int ipLen = strnlen(ip, 100);
	int curIdx = 0;
	for (int i = 0; i < ipLen; i++)
	{
		if (curIdx > 4) return 0;

		if (i == 0)
		{
			const int val = atoi(ip + i);
			if (val > 255 || val < 0) return 0;
			ipAddr |= val << (8 * curIdx);
			curIdx++;
		}
		else if (ip[i] == '.')
		{
			const int val = atoi(ip + i + 1);
			if (val > 255 || val < 0) return 0;
			ipAddr |= val << (8 * curIdx);
			curIdx++;
		}
	}
	
	return ipAddr;
}
NameValidationResult ValidateName(const std::string& name)
{
	if (name.size() < MIN_NAME_LENGTH) return Name_ErrSmall;
	if (name.size() > MAX_NAME_LENGTH) return Name_ErrLarge;
	
	for (auto c : name)
	{

	}

	return Name_Ok;
}