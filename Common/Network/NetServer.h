#pragma once
#include "Networking.h"
#include "../Plugins/ServerPlugin.h"
#include "../Plugins/ServerPlugin.h"

class ServerPlugin;
struct ServerData
{
	UDPServerSocket* sock;
	ServerPlugin* plugin;

	
};


void Server_SetLobbyPlugin(ServerData* server);