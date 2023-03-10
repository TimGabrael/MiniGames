#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "CommonCollection.h"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>




#define MIN_NAME_LENGTH 5
#define MAX_NAME_LENGTH 30
#define MAX_PLAYERS 0xFF
#define NET_VERSION 0
#define MAX_MESSAGE_LENGTH 4096


enum class ConnectionState : uint8_t
{
	Disconnected,
	Connected,
	Connecting,
};
enum class AppState : uint8_t
{
	INVALID,
	LOBBY,
	PLUGIN,
};
enum ClientBaseMessages : uint16_t
{
	Client_Join,
	Client_State,
	Client_Adminkick,
	NUM_CLIENT_BASE_MESSAGES,
};
enum ServerBaseMessages : uint16_t
{
	Server_ClientInfo,
	Server_Plugin,
	Server_SetState,
	NUM_SERVER_BASE_MESSAGES,
};

enum ClientLobbyMessages : uint16_t
{
	Client_LobbyAdminTimer = NUM_CLIENT_BASE_MESSAGES,
	Client_LobbyVote,
};

enum ServerLobbyMessages : uint16_t
{
	Server_LobbyTimer = NUM_SERVER_BASE_MESSAGES,
	Server_LobbyVote,
};

struct ClientConnection
{
	std::string name;
	uint16_t id = 0;
	bool isAdmin = false;
	ConnectionState isConnected = ConnectionState::Disconnected;
};
struct ServerConnection
{
	std::string name;
	HSteamNetConnection conn = 0;
	uint16_t id = 0;
	uint16_t activePlugin = -1;
	bool isAdmin = false;
	ConnectionState isConnected = ConnectionState::Disconnected;
	AppState state = AppState::INVALID;
};

struct NetResult
{
	std::string reason;
	bool success;
};



typedef void*(__stdcall* DeserializationFunc)(char* packet, int packetSize);


struct NetServerInterface;
struct NetServerInfo;

typedef bool(__stdcall* ServerPacketFunction)(NetServerInfo* info, ServerConnection* client, void* packet, int packetSize);
typedef NetResult(__stdcall* ServerJoinCallbackFunction)(NetServerInfo* info, ServerConnection* client);
typedef void(__stdcall* ServerDisconnectCallbackFunction)(NetServerInfo* info, ServerConnection* client);
typedef void(__stdcall* ServerClientStateChangeCallbackFunction)(NetServerInfo* info, ServerConnection* client);


struct NetClientInterface;
struct ApplicationData;

typedef bool(__stdcall* ClientPacketFunction)(ApplicationData* app, void* packet, int packetSize);
typedef void(__stdcall* ClientInfoCallbackFunction)(ApplicationData* app, ClientConnection* client);
typedef void(__stdcall* ClientDisconnectCallbackFunction)(ApplicationData* app, ClientConnection* client);



enum SendFlags
{
	Send_Unreliable = 0,
	Send_NoNagle = 1,
	Send_NoDelay = 4,
	Send_Reliable = 8,
	Send_CurrentThread = 16,
};

namespace google {namespace protobuf { class Message; }; };

struct NetServerInterface
{
	virtual ~NetServerInterface() { };

	virtual ServerConnection* GetConnection(uint16_t id) = 0;
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID) = 0;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) = 0;
	
	virtual void SetJoinCallback(ServerJoinCallbackFunction fn) = 0;
	virtual void SetClientStateCallback(ServerClientStateChangeCallbackFunction fn) = 0;
	virtual void SetDisconnectCallback(ServerDisconnectCallbackFunction fn) = 0;

    virtual bool SerializeAndStore(uint16_t packetID, const google::protobuf::Message* msg) = 0;
    virtual bool SerializeAndStore(uint16_t packetID, const void* data, uint32_t data_size) = 0;
    virtual bool SendData(ServerConnection* conn, uint32_t flags) = 0;
    virtual bool SendDataRaw(ServerConnection* conn, const void* data, uint32_t data_size, uint32_t flags) = 0;

	virtual bool IsP2P() const = 0;

	// Verify ServerData state, needs to have ServerData set as userdata
	virtual bool CheckConnectionStateAndSend(ServerConnection* c) = 0;

};
struct NetClientInterface
{
	virtual ~NetClientInterface() { };

	virtual bool IsConnected() const = 0;

	virtual ClientConnection* GetSelf() = 0;
	virtual ClientConnection* GetConnection(uint16_t id) = 0;
	virtual void SetCallback(ClientPacketFunction fn, uint16_t packetID) = 0;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) = 0;

	virtual void SetClientInfoCallback(ClientInfoCallbackFunction fn) = 0;
	virtual void SetDisconnectCallback(ClientDisconnectCallbackFunction fn) = 0;

	virtual bool SendDataRaw(uint16_t packetID, const void* data, uint32_t size, uint32_t flags) = 0;
	virtual bool SendData(uint16_t packetID, google::protobuf::Message* msg, uint32_t flags) = 0;

	virtual bool IsP2P() const = 0;

};

struct NetServerInfo
{
	NetServerInterface* net = nullptr;
	struct ServerPlugin* plugin = nullptr;
};


struct NetSocketServer
{
	HSteamListenSocket socket;
	ISteamNetworkingSockets* networking;
	ServerConnection connected[MAX_PLAYERS];
};
struct NetSocketClient
{
	HSteamListenSocket socket = 0;
	ISteamNetworkingSockets* networking = nullptr;
	ClientConnection connected[MAX_PLAYERS];
};

