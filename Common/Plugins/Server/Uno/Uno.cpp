#include "Uno.h"

SERVER_PLUGIN_EXPORT_DEFINITION(Uno);


void Uno::Initialize(NetServerInfo* s)
{
	
}

void Uno::Update(float dt)
{

}

SERVER_PLUGIN_INFO Uno::GetInfo() const
{
	static SERVER_PLUGIN_INFO info;
	memcpy(info.ID, UNO_PLUGIN_ID, sizeof(info.ID));
	info.updateTimer = 16.0f;
	info.allowLateJoin = false;
	return info;
}

void Uno::CleanUp()
{
}
