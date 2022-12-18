#pragma once
#include "NetworkBase.h"

enum ConnectionError
{
	OK,
	VERSION_MISMATCH,
	SOCKET_ERROR,
};

struct NetClient : public NetClientInterface
{
	static ConnectionError Create(const char* ip, uint32_t port, NetClient** out);

	virtual ~NetClient();

	virtual bool IsConnected() const;

	virtual Connection* GetSelf();
	virtual Connection* GetConnection(uint16_t id);
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

	NetSocket socket;
	void* userData = nullptr;
private:
	NetClient() {};

};