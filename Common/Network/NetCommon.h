#pragma once
#include "NetworkBase.h"

#define INVALID_ID 0xFFFF


enum NameValidationResult
{
	Name_Ok,
	Name_ErrSmall,
	Name_ErrLarge,
	Name_ErrSymbol,
};

uint32_t ParseIP(const char* ip);
NameValidationResult ValidateName(const std::string& name);