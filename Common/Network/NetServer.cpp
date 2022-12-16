#include "NetServer.h"
#include "Plugins/PluginCommon.h"

static int __stdcall ClientStartCallback(UDPServerSocket* sock, UDPServerSocket::ClientData* client, Client::VoteStartPacket* pack, int size);
static int __stdcall ClientVoteCallback(UDPServerSocket* sock, UDPServerSocket::ClientData* client, Client::VotePacket* pack, int size);
static bool __stdcall JoinClientFunction(UDPServerSocket* socket, UDPServerSocket::ClientData* client);
static void __stdcall DisconnectClientFunction(UDPServerSocket* socket, UDPServerSocket::ClientData* client);
struct LobbyPlugin : public ServerPlugin
{
	LobbyPlugin(UDPServerSocket* sock)
	{
		socket = sock;
	}
	virtual ~LobbyPlugin() {};
	virtual void Initialize()
	{
		// add Test Plugin
		available.push_back({ 0 });
		memcpy(available.at(0).id, "a3fV-6giK-10Eb-2rdT", 19);

		this->socket->AddPacketFunction((ServerPacketFunction)ClientStartCallback, Client::LobbyPacketID::Client_START);
		this->socket->AddPacketFunction((ServerPacketFunction)ClientVoteCallback, Client::LobbyPacketID::CLIENT_VOTE);

		this->socket->SetJoinCallback((JoinCallbackFunction)JoinClientFunction);
		this->socket->SetDisconnectCallback(((DisconnectCallbackFunction)DisconnectClientFunction));
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
	struct PluginVoteData
	{
		char id[19];
		uint32_t numVotes;
	};
	std::vector<PluginVoteData> available;

};
static int __stdcall ClientStartCallback(UDPServerSocket* sock, UDPServerSocket::ClientData* client, Client::VoteStartPacket* pack, int size)
{
	if (size != sizeof(Client::VoteStartPacket)) return -1;
	


	return sizeof(Client::VoteStartPacket);
}
static int __stdcall ClientVoteCallback(UDPServerSocket* sock, UDPServerSocket::ClientData* client, Client::VotePacket* pack, int size)
{
	if (size != sizeof(Client::VotePacket)) return -1;
	ServerData* server = (ServerData*)sock->GetUserData();
	LobbyPlugin* plugin = (LobbyPlugin*)server->plugin;


	return sizeof(Client::VotePacket);
}
static bool __stdcall JoinClientFunction(UDPServerSocket* socket, UDPServerSocket::ClientData* client)
{
	ServerData* server = (ServerData*)socket->GetUserData();
	LobbyPlugin* plugin = (LobbyPlugin*)server->plugin;
	for (uint16_t i = 0; i < plugin->available.size(); i++)
	{
		Server::PluginData data;
		data.sequenceNumber = socket->sequenceNumber;
		data.packetID = Server::LobbyPacketID::SERVER_AVAILABLE_PLUGIN;
		data.pluginSessionID = i;
		memcpy(data.pluginID, plugin->available.at(i).id, 19);
		socket->SendData(&data, sizeof(Server::PluginData), client);
	}

	return true;
}
static void __stdcall DisconnectClientFunction(UDPServerSocket* socket, UDPServerSocket::ClientData* client)
{

}


void Server_SetLobbyPlugin(ServerData* server)
{
	server->sock->SetUserData(server);
	server->plugin = new LobbyPlugin(server->sock);
	server->plugin->Initialize();
}