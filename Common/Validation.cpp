#include "Validation.h"



bool ValidateName(std::string& name)
{
	while (*(name.end() - 1) == ' ')
		name.pop_back();

	if (name.size() < 3 || name.size() > 30) return false;

	for (unsigned char c : name) if (c > 127) return false;

	return true;
}
bool ValidateServerID(std::string& server)
{
	while (*(server.end() - 1) == ' ')
		server.pop_back();

	if (server.size() < 5 || server.size() > 30) return false;
	for (unsigned char c : server) if (c > 127) return false;

	return true;
}
