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


enum class ConnectionState : uint8_t
{
	Disconnected,
	Connected,
	Connecting,
};
enum class ClientState : uint8_t
{
	INVALID,
	LOBBY,
	PLUGIN,
};
enum ClientBaseMessages : uint16_t
{
	Client_Join,
	Client_State,
};
enum ServerBaseMessages : uint16_t
{
	Server_Join,
	Server_Plugin,
	Server_SetState,
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
	ClientState state = ClientState::INVALID;
};

struct JoinResult
{
	std::string reason;
	bool success;
};

struct NetServerInterface;
struct NetClientInterface;

typedef void*(__stdcall* DeserializationFunc)(char* packet, int packetSize);

typedef bool(__stdcall* ServerPacketFunction)(NetServerInterface* socket, ServerConnection* client, void* packet, int packetSize);
typedef JoinResult(__stdcall* ServerJoinCallbackFunction)(NetServerInterface* socket, ServerConnection* client);
typedef void(__stdcall* ServerDisconnectCallbackFunction)(NetServerInterface* socket, ServerConnection* client);


typedef bool(__stdcall* ClientPacketFunction)(NetClientInterface* socket, void* packet, int packetSize);
typedef void(__stdcall* ClientJoinCallbackFunction)(NetClientInterface* socket, ClientConnection* client);
typedef void(__stdcall* ClientDisconnectCallbackFunction)(NetClientInterface* socket, ClientConnection* client);

enum SendFlags
{
	Send_Unreliable = 0,
	Send_NoNagle = 1,
	Send_NoDelay = 4,
	Send_Reliable = 8,
	Send_CurrentThread = 16,
};

struct NetServerInterface
{
	virtual ~NetServerInterface() { };

	virtual ServerConnection* GetConnection(uint16_t id) = 0;
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID) = 0;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) = 0;
	
	virtual void SetJoinCallback(ServerJoinCallbackFunction fn) = 0;
	virtual void SetDisconnectCallback(ServerDisconnectCallbackFunction fn) = 0;

	virtual bool SendData(ServerConnection* conn, uint16_t packetID, const void* data, uint32_t size, uint32_t flags) = 0;

	virtual bool IsP2P() const = 0;

	virtual void* GetUserData() const = 0;
	virtual void SetUserData(void* userData) = 0;
};
struct NetClientInterface
{
	virtual ~NetClientInterface() { };

	virtual bool IsConnected() const = 0;

	virtual ClientConnection* GetSelf() = 0;
	virtual ClientConnection* GetConnection(uint16_t id) = 0;
	virtual void SetCallback(ClientPacketFunction fn, uint16_t packetID) = 0;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) = 0;

	virtual void SetJoinCallback(ClientJoinCallbackFunction fn) = 0;
	virtual void SetDisconnectCallback(ClientDisconnectCallbackFunction fn) = 0;

	virtual bool SendData(uint16_t packetID, const void* data, uint32_t size, uint32_t flags) = 0;

	virtual bool IsP2P() const = 0;

	virtual void* GetUserData() const = 0;
	virtual void SetUserData(void* userData) = 0;
};



struct NetSocketServer
{
	HSteamListenSocket socket;
	ISteamNetworkingSockets* networking;
	ServerConnection connected[MAX_PLAYERS];
};
struct NetSocketClient
{
	HSteamListenSocket socket;
	ISteamNetworkingSockets* networking;
	ClientConnection connected[MAX_PLAYERS];
};

