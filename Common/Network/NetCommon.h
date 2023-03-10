#pragma once
#include "NetworkBase.h"
#include "BaseMessages.pb.h"

#define INVALID_ID 0xFFFF


enum NameValidationResult
{
	Name_Ok,
	Name_ErrSmall,
	Name_ErrLarge,
	Name_ErrSymbol,
};


bool InitNetworking();

uint32_t ParseIP(const char* ip);
NameValidationResult ValidateName(const std::string& name);

void NetRunCallbacks();
