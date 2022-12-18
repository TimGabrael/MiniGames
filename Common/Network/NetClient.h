#pragma once
#include "NetCommon.h"



struct NetClient : public NetClientInterface
{
	static JoinResult Create(const char* ip, uint32_t port, const std::string& name, NetClient** out);

	virtual ~NetClient();

	virtual bool IsConnected() const;

	virtual ClientConnection* GetSelf();
	virtual ClientConnection* GetConnection(uint16_t id);
	virtual void SetCallback(ClientPacketFunction fn, uint16_t packetID);

	virtual void SetJoinCallback(ClientJoinCallbackFunction fn);
	virtual void SetDisconnectcallback(ClientDisconnectCallbackFunction fn);

	virtual bool SendData(const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;


	virtual void* GetUserData() const { return userData; };
	virtual void SetUserData(void* userData) { this->userData = userData; };


	void Poll();

	// use after Poll, this is seperate as both client and server should call it only once
	static void RunCallbacks();

	NetSocketClient socket;
	ClientConnection* local = nullptr;
	void* userData = nullptr;
private:
	NetClient() {};

};