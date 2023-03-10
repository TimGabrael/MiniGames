#pragma once
#include "NetworkBase.h"
#include "../Plugins/Server/ServerPlugin.h"
#include "BaseMessages.pb.h"



struct NetServer : public NetServerInterface
{
	static NetServer* Create(const char* ip, uint32_t port);

	virtual ~NetServer();

	virtual ServerConnection* GetConnection(uint16_t id) override;
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID) override;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) override;

	virtual void SetJoinCallback(ServerJoinCallbackFunction fn) override;
	virtual void SetClientStateCallback(ServerClientStateChangeCallbackFunction fn) override;
	virtual void SetDisconnectCallback(ServerDisconnectCallbackFunction fn) override;

    virtual bool SerializeAndStore(uint16_t packetID, const google::protobuf::Message* msg) override;
    virtual bool SerializeAndStore(uint16_t packetID, const void* data, uint32_t data_size) override;
    virtual bool SendData(ServerConnection* conn, uint32_t flags) override;
    virtual bool SendDataRaw(ServerConnection* conn, const void* data, uint32_t data_size, uint32_t flags) override;


	virtual bool IsP2P() const override;


	void Poll();


	// Verify ServerData state, needs to have ServerData set as userdata
	virtual bool CheckConnectionStateAndSend(ServerConnection* c) override;

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
	uint8_t tempStorage[MAX_MESSAGE_LENGTH + 20];
    size_t storedSize;

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
