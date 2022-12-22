#pragma once
#include "NetworkBase.h"
#include "../Plugins/Server/ServerPlugin.h"
#include "BaseMessages.pb.h"



struct NetServer : public NetServerInterface
{
	static NetServer* Create(const char* ip, uint32_t port);

	virtual ~NetServer();

	virtual ServerConnection* GetConnection(uint16_t id);
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID);
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID);

	virtual void SetJoinCallback(ServerJoinCallbackFunction fn);
	virtual void SetClientStateCallback(ServerClientStateChangeCallbackFunction fn);
	virtual void SetDisconnectCallback(ServerDisconnectCallbackFunction fn);

	virtual bool SendData(ServerConnection* conn, uint16_t packetID, const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;


	void Poll();


	// Verify ServerData state, needs to have ServerData set as userdata
	bool CheckConnectionStateAndSend(ServerConnection* c);

	struct ServerData* data = nullptr;
	NetSocketServer socket;
private:

	// Sends on mismatch
	bool CheckConnectionStateAndSendInternal(ServerConnection* c, AppState s, uint16_t pluginID);
	
	NetServer() {};

	static void SteamNetServerConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);
	ServerConnection* GetNew();
	ServerConnection* GetFromNetworkConnection(HSteamNetConnection conn);
	void CloseConnection(ServerConnection* c);

	uint16_t GetClientID(ServerConnection* conn);
	
	static bool ClientJoinPacketCallback(ServerData* s, ServerConnection* client, base::ClientJoin* join, int size);
	static bool ClientStatePacketCallback(ServerData* s, ServerConnection* client, base::ClientState* state, int size);
	static bool ClientAdminKickPacketCallback(ServerData* s, ServerConnection* client, base::ClientAdminKick* kick, int size);

	bool hasAdmin() const;

	struct CallbackInfo
	{
		DeserializationFunc deserializer;
		ServerPacketFunction receiver;
	};
	HSteamNetPollGroup group = 0;
	ServerJoinCallbackFunction joinCB = nullptr;
	ServerClientStateChangeCallbackFunction stateCB = nullptr;
	ServerDisconnectCallbackFunction disconnectCB = nullptr;
	std::vector<CallbackInfo> callbacks;
	std::vector<char> tempStorage;

};

struct ServerData
{
	ServerData(const char* ip, uint32_t port);
	~ServerData();
	void SetLobbyState();
	void Update(float dt);


	struct InternalNetServerInfo
	{
		NetServer* net = nullptr; 
		ServerPlugin* plugin = nullptr;
	} info;

	struct ClientVoteData
	{
		uint16_t clientID;
		uint16_t pluginID;
	};
	struct LobbyData
	{
		std::vector<ClientVoteData> votes;
		float timer = 0.0f;
		bool timerRunning = false;
	}lobbyData;

	struct Plugin
	{
		ServerPlugin* pl = nullptr;
		std::string id;
		uint16_t pluginID = 0xFFFF;
	};

	std::vector<Plugin> plugins;

	float updateInterval_ms = 1.0f;
	uint16_t activePluingID = 0xFFFF;
};
