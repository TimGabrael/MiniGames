#include "NetworkBase.h"


static constexpr char validSpecialCharacters[] = {
	'_', ',', ' ', '!', ';', ':', '.'
};

JOIN_ERROR ValidatePlayerName(const char name[MAX_NAME_LENGTH])
{
	JOIN_ERROR err = JOIN_ERROR::JOIN_OK;
	for (int i = 0; i < MAX_NAME_LENGTH; i++)
	{
		if (name[i] == '\00')
		{
			if (i < MIN_NAME_LENGTH) err = JOIN_ERROR::JOIN_NAME_SHORT;
			break;
		}
		bool isValid = false;

		if (((name[i] < 'Z' && name[i] >= 'A')
			|| (name[i] < 'z' && name[i] >= 'a')
			|| (name[i] < '9' && name[i] >= '0')))
		{
			isValid = true;
		}
		else
		{
			for (int i = 0; i < sizeof(validSpecialCharacters); i++)
			{
				if (name[i] == validSpecialCharacters[i])
				{
					isValid = true;
					break;
				}
			}
		}
		
		if (!isValid) {
			err = JOIN_ERROR::JOIN_NAME_INVALID_CHARACTER;
			break;
		}
	}
	return err;
}
JOIN_ERROR ValidatePlayerName(const std::string& name)
{
	if (name.size() < 3 || name.size() > MAX_NAME_LENGTH) return JOIN_ERROR::JOIN_NAME_SHORT;
	char charName[MAX_NAME_LENGTH];
	memcpy(charName, name.c_str(), name.size());
	return ValidatePlayerName(charName);

}