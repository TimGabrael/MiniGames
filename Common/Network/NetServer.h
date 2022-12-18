#pragma once
#include "NetworkBase.h"
#include "../Plugins/ServerPlugin.h"




struct NetServer : public NetServerInterface
{
	static NetServer* Create(const char* ip, uint32_t port);

	virtual ~NetServer();

	virtual Connection* GetConnection(uint16_t id);
	virtual void SetCallback(ServerPacketFunction fn, uint16_t packetID);

	virtual void SetJoinCallback(ServerJoinCallbackFunction fn);
	virtual void SetDisconnectcallback(ServerDisconnectCallbackFunction fn);

	virtual bool SendData(Connection* conn, const void* data, uint32_t size, uint32_t flags);
	virtual bool SendData(uint16_t id, const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;

	virtual void* GetUserData() const { return userData; };
	virtual void SetUserData(void* userData) { this->userData = userData; };

	void Poll();

	// use after Poll, this is seperate as both client and server should call it only once
	static void RunCallbacks();

	NetSocket socket;
	HSteamNetPollGroup group = 0;
	void* userData = nullptr;
};
