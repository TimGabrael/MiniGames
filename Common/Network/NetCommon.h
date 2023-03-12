#pragma once
#include "NetworkBase.h"

#ifdef _WIN32
// SHUT UP PROTOBUF I DON'T CARE ABOUT YOUR WARNINGS!
#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 4251)
#pragma warning(disable: 4146)
#pragma warning(disable: 4267)
#endif
#include "BaseMessages.pb.h"
#ifdef _WIN32
#pragma warning(pop)
#endif
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

