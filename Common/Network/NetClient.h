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
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID);

	virtual void SetJoinCallback(ClientJoinCallbackFunction fn);
	virtual void SetDisconnectCallback(ClientDisconnectCallbackFunction fn);

	virtual bool SendData(uint16_t packetID, const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;


	virtual void* GetUserData() const { return userData; };
	virtual void SetUserData(void* userData) { this->userData = userData; };


	void Poll();


	NetSocketClient socket;
	void* userData = nullptr;
private:

	static void SteamNetClientConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static bool ServerJoinCallback(NetClient* c, base::ServerJoin* join, int size);

	ClientConnection* local = nullptr;
	ClientDisconnectCallbackFunction disconnectCB = nullptr;
	ClientJoinCallbackFunction joinCB = nullptr;
	struct CallbackInfo
	{
		DeserializationFunc deserializer = nullptr;
		ClientPacketFunction receiver = nullptr;
	};
	std::vector<CallbackInfo> callbacks;
	std::vector<char> tempStorage;
	NetClient() {};

};