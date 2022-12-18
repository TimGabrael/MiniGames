#pragma once
#include "NetworkBase.h"
#include "../Plugins/ServerPlugin.h"




struct NetServer : public NetServerInterface
{
	static NetServer* Create(const char* ip, uint32_t port);

	virtual ~NetServer();

	virtual ServerConnection* GetConnection(uint16_t id);
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID);
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID);

	virtual void SetJoinCallback(ServerJoinCallbackFunction fn);
	virtual void SetDisconnectCallback(ServerDisconnectCallbackFunction fn);

	virtual bool SendData(ServerConnection* conn, uint16_t packetID, const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;

	virtual void* GetUserData() const { return userData; };
	virtual void SetUserData(void* userData) { this->userData = userData; };

	void Poll();

	NetSocketServer socket;
	HSteamNetPollGroup group = 0;
	void* userData = nullptr;

private:
	static void SteamNetServerConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);
	ServerConnection* GetNew();
	uint16_t GetClientID(ServerConnection* conn);
	

	struct CallbackInfo
	{
		DeserializationFunc deserializer;
		ServerPacketFunction receiver;
	};
	ServerJoinCallbackFunction joinCB = nullptr;
	ServerDisconnectCallbackFunction disconnectCB = nullptr;
	std::vector<CallbackInfo> callbacks;
	std::vector<char> tempStorage;

};
