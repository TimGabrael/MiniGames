#include "NetServer.h"

struct LobbyPlugin : public ServerPlugin
{
	virtual ~LobbyPlugin() {};
	virtual void Initialize()
	{

	}
	virtual void Update(float dt)
	{

	}
	virtual SERVER_PLUGIN_INFO GetInfo() const
	{
		SERVER_PLUGIN_INFO info;
		info.updateTimer = -1.0f;
		info.allowLateJoin = true;
		return info;
	}

};

void Server_SetBaseCallbacks(ServerData* server)
{
}